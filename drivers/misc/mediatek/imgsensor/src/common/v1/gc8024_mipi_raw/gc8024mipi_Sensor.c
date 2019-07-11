/*****************************************************************************
 *
 * Filename:
 * ---------
 *     GC8024mipi_Sensor.c
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

//#include "kd_camera_hw.h"
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "gc8024mipi_Sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "gc8024mipi_Sensor.c"
#define LOG_1 LOG_INF("GC8024\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)        pr_err(PFX "[%s](%d) " format, __FUNCTION__, __LINE__, ##args)

//#define IMAGE_NORMAL_MIRROR 
#define IMAGE_H_MIRROR 
//#define IMAGE_V_MIRROR 
//#define IMAGE_HV_MIRROR 

#ifdef IMAGE_NORMAL_MIRROR
#define MIRROR 		  0xd4
#define PreStartY	  0x03
#define PreStartX     0x05
#define CapStartY     0x05
#define CapStartX     0x09
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 		  0xd5
#define PreStartY	  0x03
#define PreStartX     0x02
#define CapStartY     0x05
#define CapStartX     0x04

#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 		  0xd6
#define PreStartY     0x04
#define PreStartX     0x05
#define CapStartY     0x04
#define CapStartX     0x09

#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR  	  0xd7
#define PreStartY	  0x02
#define PreStartX     0x02
#define CapStartY     0x06
#define CapStartX     0x06

#endif

#define ANALOG_GAIN_1 64   // 1.00x
#define ANALOG_GAIN_2 91   // 1.42x
#define ANALOG_GAIN_3 127  // 1.98x 
#define ANALOG_GAIN_4 181  // 2.82x
#define ANALOG_GAIN_5 252  // 3.94x
#define ANALOG_GAIN_6 357  // 5.57x

#define GC8024_MIPI_USE_OTP
//#define GC8024MIPI_4LANE


static kal_uint8 LH_wei[3][108] = {
	{	
		8,	7,	8,	8,	8,	8,
		8,	7,	6,	5,	7,	8,
		8,	7,	4,	6,	6,	8,
		7,	5,	4,	5,	5,	7,
		7,	6,	5,	5,	6,	7,
		8,	7,	7,	8,	7,	8,
		7,	6,	6,	6,	6,	7,
		7,	6,	6,	5,	5,	7,
		7,	6,	5,	5,	5,	7,
		6,	4,	4,	4,	5,	7,
		6,	5,	5,	4,	5,	6,
		7,	6,	6,	6,	6,	7,
		6,	6,	6,	6,	6,	6,
		6,	6,	5,	4,	5,	6,
		6,	5,	4,	4,	5,	6,
		5,	4,	4,	4,	4,	5,
		5,	5,	4,	4,	4,	5,
		7,	6,	5,	5,	6,	7
	},
	{
		8,	 8,	7,	8,	7,	8,
		7,	 7,	6,	7,	7,	7,
		7,	 6,	5,	6,	8,	8,
		6,	 6,	6,	5,	6,	7,
		7,	 7,	7,	7,	7,	7,
		8,	 7,	8,	8,	7,	8,
		7,	 6,	6,	7,	7,	7,
		6,	 6,	5,	5,	6,	7,
		6,	 5,	4,	5,	6,	7,
		6,	 5,	4,	4,	5,	6,
		6,	 6,	5,	5,	6,	6,
		7,	 6,	7,	7,	7,	7,
		6,	 6,	6,	6,	6,	7,
		6,	 6,	5,	5,	6,	6,
		5,	 5,	4,	4,	6,	6,
		6,	 5,	4,	4,	5,	6,
		6,	 5,	5,	5,	5,	6,
		7,	 6,	6,	6,	6,	7,
	},
	{	
		8,	7,	7,	7,	7,	8,
		8,	7,	6,	6,	6,	8,
		7,	6,	4,	5,	6,	8,
		6,	5,	5,	5,	5,	6,
		7,	6,	6,	6,	6,	7,
		8,	7,	7,	7,	7,	8,
		8,	7,	6,	7,	7,	8,
		7,	6,	6,	5,	6,	8,
		7,	5,	5,	5,	5,	7,
		6,	5,	5,	5,	5,	6,
		7,	5,	5,	5,	5,	7,
		8,	6,	7,	7,	6,	8,
		8,	7,	6,	6,	6,	8,
		7,	6,	6,	5,	5,	7,
		6,	5,	5,	5,	5,	7,
		6,	5,	5,	5,	5,	6,
		6,	5,	5,	5,	5,	6,
		8,	6,	6,	6,	6,	8
	},
};		

static int16_t para_medium[3][108] = {
	{
		128,  -6,  29,	28, -10, 128,
		 12, -10,  19,	-2,   8,   3,
		 41,  15,	0,	 5,   5,  26,
		 -7,   0,	1,	-3,   2, -11,
		  2,   2,	1,	 2,  -2,  10,
		128,  -3,	2, -19,  13, 128,
		 -9,  -1,  -5,	-3,   3,  -9,
		 -1,  -1,  -3,	 1,  -3,   5,
		 -9,  -5,	1,	-1,   1,  14,
		 -4,   0,	0,	 1,  -1,  -8,
		 -6,  -2,	0,	 0,   3,  -1,
		-13,  -4,  -3,	-3,  -9, -16,
		 -2,   3, -10,	 6,  -4,  -7,
		  7,   3,  -3,	-1,   1,  -5,
		 -5,  -3,	1,	 1,  -2,   2,
		  1,   1,  -1,	 0,  -1,  -1,
		 -3,   0,	0,	 0,   0,   0,
		  4,  -1,	3,	 0,  -2,  -3
	},
	{
		155, -26,  84,   6,  -4, 162,
		-46, -50,  30,   7,   7,   1,
		 17,  33,  -2,   5,  10,  28,
		 -1,  -2,  -2,  -2,  -4, -12,
		 -9,  -6,  -7, -10,  -7,   9,
		144,  -4, -13, -16,   5, 160,
		-25,   9, -23,   0,   6, -24,
		 15,   8,  -6,  -1,   1,   2,
		 -5, -10,   4,  -2,   2,  -3,
		  4,   0,   1,   0,   1,   7,
		  0,   2,   1,   3,   2,  -9,
		-21,  -2,   5,   6,  -5, -17,
		-18,   5, -17,  -1,  -2, -18,
		  3,   9,  -5,  -2,  -3,  -3,
		  0,  -2,   3,  -1,  -5,  -5,
		  1,   2,   0,  -1,   3,   0,
		  3,   3,   3,   3,   1,  -1,
		 -8,   0,   7,   6,   3, -11,
	},
	{
		128,  -3,  27,	 0,   0, 128,
		  0, -13,  26,	 0,   0,   0,
		 33,  23,  -5,	 0,   0,   0,
		  0,   0,	0,	 0,   0,   0,
		  0,   0,	0,	 0,   0,   0,
		128,   1,	0,	 0,   0, 128,
		  0,  -1,  -7,	 1,   0,   1,
		  3,  -1,  -8,	 0,   0,   0,
		 -5,  -7,	3,	 0,   0,   0,
		  0,   0,	0,	 0,   0,   0,
		  0,   0,	0,	 0,   0,   0,
		  1,   0,	0,	 0,   0,   0,
		-25,   4, -13,	 0,   1, -26,
		  3,   5,  -5,	 0,  -1,   0,
		  3,  -2,	3,	 0,   0,  -1,
		  0,   0,	0,	 0,   0,   0,
		  0,   0,	0,	 0,  -1,   0,
		-26,   0,  -1,	 0,   0, -26
	}
};



static DEFINE_SPINLOCK(imgsensor_drv_lock);
kal_bool GC8024DuringTestPattern = KAL_FALSE;

static kal_uint8 PreorCap = 0;
static kal_uint16 TxLowTh = 150;
static kal_uint8 lsc_version = 0;

static kal_uint8 gainlevel = 0;
static kal_uint8 Val28[2][4] =
{
	{0xb0,0xb0,0xc0,0xdf}, //preview
	{0x30,0x30,0x40,0x5f}, //capture
};
static imgsensor_info_struct imgsensor_info = {
    .sensor_id = GC8024_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0xcf44e4eb,        //checksum value for Camera Auto Test
    
    .pre = {
        .pclk = 40500000,                //record different mode's pclk
        .linelength = 2160,                //record different mode's linelength
        .framelength = 625,              //623->625  20171027        //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1632,        //record different mode's width of grabwindow
        .grabwindow_height = 1224,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,         //100->300  20171027
    },

    .cap = {
        .pclk = 40500000,
        .linelength = 4320,
        .framelength = 625,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 3264,
        .grabwindow_height = 2448,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 150,            //100->150 20171027
    },
    .cap1 = {                            //capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
        .pclk = 40500000,
        .linelength = 4320,
        .framelength = 625,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 3264,
        .grabwindow_height = 2448,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 150, 
    },
    .normal_video = {
          .pclk = 40500000,
        .linelength = 2160,
        .framelength = 625,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1632,
        .grabwindow_height = 1224,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300, 
    },
    .hs_video = {
          .pclk = 40500000,
        .linelength = 2160,
        .framelength = 625,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1632,
        .grabwindow_height = 1224,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
    },
    .slim_video = {
        .pclk = 40500000,
        .linelength = 2160,
        .framelength = 625,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1632,
        .grabwindow_height = 1224,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
    },	   

    .margin = 0,            //sensor framelength & shutter margin
    .min_shutter = 1,        //min shutter
    .max_frame_length = 0x7fff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 3,      //support sensor mode num

    .cap_delay_frame = 2,        //enter capture delay frame num
    .pre_delay_frame = 2,         //enter preview delay frame num
    .video_delay_frame = 2,        //enter video delay frame num
    .hs_video_delay_frame = 2,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 2,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_6MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
#ifdef GC8024MIPI_4LANE
    .mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
#else	
    .mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
#endif
    .i2c_addr_table = {0x6e, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_NORMAL,                //mirrorflip information
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3ED,                    //current shutter
    .gain = 0x40,                        //current gain
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .ihdr_en = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x6e,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{	
	{ 3280, 2464,	 0,    0, 3280, 2464, 1640,  1232,	  5,	2, 1632,  1224, 	 0,    0, 1632,  1224}, // Preview 
	{ 3280, 2464,	 0,    0, 3280, 2464, 3280,  2464,	  5,	2, 3264,  2448, 	 0,    0, 3264,  2448}, // capture 
	{ 3280, 2464,	 0,    0, 3280, 2464, 1640,  1232,	  5,	2, 1632,  1224, 	 0,    0, 1632,  1224}, // video 
	{ 3280, 2464,	 0,    0, 3280, 2464, 1640,  1232,	  5,	2, 1632,  1224, 	 0,    0, 1632,  1224}, // hight speed video 
	{ 3280, 2464,	 0,    0, 3280, 2464, 1640,  1232,	  5,	2, 1632,  1224, 	 0,    0, 1632,  1224}  // slim video 
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[1] = {(char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 1, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;

}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
#if 1
		char pu_send_cmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};
		iWriteRegI2C(pu_send_cmd, 2, imgsensor.i2c_write_id);
#else
		iWriteReg((u16)addr, (u32)para, 2, imgsensor.i2c_write_id);
#endif

}

#ifdef GC8024_MIPI_USE_OTP
#define RG_TYPICAL   			172	//0x00ae  modified when write WB !!!
#define BG_TYPICAL				175	//modified when write WB!!!
#define WINDOW_WIDTH  			0x0cd0 //3280 max effective pixels
#define WINDOW_HEIGHT 			0x099c //2460
#define DD_PARAM_QTY 			108
#define LSC_WIDTH 				6
#define LSC_HEIGHT 				6

typedef struct otp_gc8024{
	kal_uint16 module_id;
	kal_uint16 flag;
	kal_uint16 lens_id;
	kal_uint16 vcm_id;
	kal_uint16 vcm_start_position;
	kal_uint16 vcm_driver_id;
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 module_info1;
	kal_uint16 module_info2;
	kal_uint16 module_info3;
	kal_uint16 module_info4;
	kal_uint16 module_info5;	
	kal_uint16 rg_gain;
	kal_uint16 bg_gain;
	kal_uint16 wb_flag;// 0:Empty 1:Success 2:Invalid
	kal_uint16 dd_param_x[DD_PARAM_QTY];
	kal_uint16 dd_param_y[DD_PARAM_QTY];
	kal_uint16 dd_param_type[DD_PARAM_QTY];
	kal_uint16 dd_cnt;
	kal_uint16 dd_flag;// 0:Empty 1:Success 2:Invalid
	kal_uint16 lsc_flag;// 0:Empty 1:Success 2:Invalid
	kal_uint8 lsc_r[LSC_WIDTH][LSC_HEIGHT];
	kal_uint8 lsc_g[LSC_WIDTH][LSC_HEIGHT];
	kal_uint8 lsc_b[LSC_WIDTH][LSC_HEIGHT];	
}gc8024_otp;

static gc8024_otp gc8024_otp_info;

typedef enum{
	otp_page0=0,
	otp_page1,
}otp_page;

typedef enum{
	otp_close=0,
	otp_open,
}otp_state;


static kal_uint8 gc8024_read_otp(kal_uint8 addr)
{
	kal_uint8 value;

	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xd5,addr);
	write_cmos_sensor(0xf3,0x20);
	value = read_cmos_sensor(0xd7);

	return value;
}

static void gc8024_otp_select_page(otp_page otp_select_page)
{
	kal_uint8 page;
	
	write_cmos_sensor(0xfe,0x00);
	page = read_cmos_sensor(0xd4);

	switch(otp_select_page)
	{
	case otp_page0:
		page = page & 0xfb;
		break;
	case otp_page1:
		page = page | 0x04;
		break;
	default:
		break;
	}

	mdelay(5);
	write_cmos_sensor(0xd4,page);	

}

void func_revDPC(kal_uint8  *arrar, kal_uint8 ref_value, kal_uint8 *com)
{
	short data_base;
	int i,j;
	data_base = ref_value;
	com[2*6+2] = arrar[2*6+2] + data_base;
	com[1*6+2] = arrar[1*6+2] + com[2*6+2];
	com[2*6+1] = arrar[2*6+1] + com[1*6+2];

	com[0*6+2] = arrar[0*6+2] + com[2*6+1];
	com[1*6+1] = arrar[1*6+1] + com[0*6+2];
	com[2*6+0] = arrar[2*6+0] + com[1*6+1];

	com[0*6+1] = arrar[0*6+1] + com[2*6+0];
	com[1*6+0] = arrar[1*6+0] + com[0*6+1];
	com[0*6+0] = arrar[0*6+0]; 

	for (i = 0; i < 3; ++i)
	{
		for (j = 3; j < 6; ++j)
		{
			com[i*6+j] = arrar[i*6+j] + com[i*6+(5-j)];
		}
	}

	for (i = 3; i < 6; ++i)
	{
		for (j = 0; j < 6; ++j)
		{
			com[i*6+j] = arrar[i*6+j] + com[(5-i)*6+j];
		}
	}

	com[5*6+0] = arrar[5*6+0];  
	com[0*6+5] = arrar[0*6+5]; 
	com[5*6+5] = arrar[5*6+5];  
}

static void gc8024_gcore_read_otp_info_new(void)
{
	kal_uint8 flag1;
	kal_uint8 flag2;
//	kal_uint8 index;
	kal_uint8 i,j,cnt=0;
//	kal_uint8 info_start_add,wb_start_add;
	kal_uint8 total_number=0; 
	kal_uint8 check_dd_flag,type;
	kal_uint8 dd0=0,dd1=0,dd2=0;
	kal_uint16 x,y,temp;
	kal_uint16 integral[108] = {0};
	kal_uint8 ref_value = 60;	
	kal_uint8 OtpLsc[84] = {0};//82
	kal_uint8 com_test_step1[108] = {0};
	kal_uint8 com_test_step2[108] = {0};
	kal_uint8 com_test_result[108] = {0};			
	kal_uint8 Lsc[108]= {0};
	kal_uint8 addr;

	memset(&gc8024_otp_info,0,sizeof(gc8024_otp));
	gc8024_otp_select_page(otp_page1);
	flag1 = gc8024_read_otp(0x7f);
	
	gc8024_otp_select_page(otp_page0);
	
	/*Group 1*/
	if((gc8024_read_otp(0x00)|0x03) == 0x57)
	{
		LOG_INF("%s(%d) GC8024_OTP use Group 1 \n",__FUNCTION__,__LINE__);
        gc8024_otp_info.lsc_flag = 0x01;
		gc8024_otp_info.module_id = gc8024_read_otp(0x01);
		temp = gc8024_read_otp(0x04);
		gc8024_otp_info.rg_gain = ((temp&0xc)<<6)+gc8024_read_otp(0x02);
		gc8024_otp_info.bg_gain = ((temp&0x3)<<8)+gc8024_read_otp(0x03);
		addr = 0x05;
		if((flag1&0x30) == 0x20) /*new version 84_kr*/
		{
			lsc_version = 2;
			ref_value = 72;
		}
		else if ((flag1&0x30) == 0x10)/*new version 84*/
		{
			lsc_version = 1;
			ref_value = 60;
		}
		else if ((flag1&0x30) == 0x00)/*old version*/
		{
			lsc_version = 0;
			ref_value = 60;
		}
		for (cnt = 0; cnt < 82; addr++, cnt++)
		{
			OtpLsc[cnt] = gc8024_read_otp(addr);
			//LOG_INF("GC8024_OTP_Shading P%d:0x%x,OtpLsc[%d]=0x%x=%d\n",addr/0x80,addr%0x80,cnt,OtpLsc[cnt],OtpLsc[cnt]);			
		}
		if(0 != lsc_version)
		{
			OtpLsc[82] = gc8024_read_otp(0x7c);
			OtpLsc[83] = gc8024_read_otp(0x7d);
		}
	}
	else 
	{
		/*Group2*/
		LOG_INF("%s(%d) GC8024_OTP use Group 2 \n",__FUNCTION__,__LINE__);
		gc8024_otp_select_page(otp_page1);
		if((gc8024_read_otp(0x36)|0x30) == 0x75)
		{
            gc8024_otp_info.lsc_flag = 0x01;
			gc8024_otp_info.module_id = gc8024_read_otp(0x75);
			temp = gc8024_read_otp(0x78);
			gc8024_otp_info.rg_gain = ((temp&0xc)<<6)+gc8024_read_otp(0x76);
			gc8024_otp_info.bg_gain = ((temp&0x3)<<8)+gc8024_read_otp(0x77);
			if((flag1&0xc0) == 0x80) /*new version 84_kr*/
			{
				lsc_version = 2;
				ref_value = 72;
			}
			else if ((flag1&0xc0) == 0x40)/*new version 84*/
			{
				lsc_version = 1;
				ref_value = 60;
			}
			else if ((flag1&0xc0) == 0x00)/*old version*/
			{
				lsc_version = 0;
				ref_value = 60;
			}
				gc8024_otp_select_page(otp_page0);
			addr = 0x57;
			for (cnt = 0; cnt < 82; addr++, cnt++)
			{
				if (addr==0x73)
				{
					gc8024_otp_select_page(otp_page1);				
				}
				OtpLsc[cnt] = gc8024_read_otp(addr%0x73);
				//LOG_INF("GC8024_OTP_Shading P%d:0x%x,OtpLsc[%d]=0x%x=%d\n",addr/0x80,addr%0x80,cnt,OtpLsc[cnt],OtpLsc[cnt]);			
			}
			if(0 != lsc_version)
			{	
				gc8024_otp_select_page(otp_page0);	
				OtpLsc[82] = gc8024_read_otp(0x7e);
				OtpLsc[83] = gc8024_read_otp(0x7f);
			}
		}
		else /*Empty or Invalid*/
		{
			LOG_INF("%s(%d) GC8024_OTP  Group 1 2 all Invalid \n",__FUNCTION__,__LINE__);
			gc8024_otp_info.flag = 1;
            gc8024_otp_info.lsc_flag = 0;
		}
	}
	LOG_INF("GC8024_OTP module id = 0x%x\n",gc8024_otp_info.module_id);
	LOG_INF("GC8024_OTP r/g gain = 0x%x\n",gc8024_otp_info.rg_gain);
	LOG_INF("GC8024_OTP b/g gain = 0x%x\n",gc8024_otp_info.bg_gain);
	if(!gc8024_otp_info.flag)
	{
		integral[0] = LH_wei[lsc_version][0];
		for (i = 1; i < 108; ++i)
		{
			integral[i] = integral[i-1] + LH_wei[lsc_version][i];
		}
		/*82B to 108B = R'''/G'''/B'''*/
		//LOG_INF("GC8024_OTP_Shading 82B to 108B\n");
		com_test_result[0] |= OtpLsc[0];		
		if (com_test_result[0] & (1 << (LH_wei[lsc_version][0]-1)))
		{
			com_test_result[0] |= ~((1 << (LH_wei[lsc_version][0]-1))-1);
		}
		else
		{
			com_test_result[0] &= (1 << (LH_wei[lsc_version][0]-1))-1;
		}
		//LOG_INF("GC8024_OTP_Shading com_test_result[0]=%d=0x%x\n",com_test_result[0],com_test_result[0]);
		
		for (i = 1; i < 108; ++i)
		{
			int c0 = integral[i-1] / 8;
			int r0 = integral[i-1] % 8;
			int c1 = integral[i] / 8;
			int r1 = integral[i] % 8;
			if (c1 > c0 && r1)
			{
				com_test_result[i] |= (OtpLsc[c0] >> r0) & ((1 << (8-r0))-1);
				com_test_result[i] |= OtpLsc[c1] << (8-r0); 
			}
			else
			{
				com_test_result[i] |= (OtpLsc[c0] >> r0);
			}
			if (com_test_result[i] & (1 << (LH_wei[lsc_version][i]-1)))
			{
				com_test_result[i] |= ~((1 << (LH_wei[lsc_version][i]-1))-1);
			}
			else
			{
				com_test_result[i] &= (1 << (LH_wei[lsc_version][i]-1))-1;
			}
			//LOG_INF("GC8024_OTP_Shading com_test_result[%d]=0x%x=%d\n",i,com_test_result[i],com_test_result[i] );						
		}
		
		/*R''/B''/G''=R'''/B'''/G'''+para_medium*/
		//LOG_INF("GC8024_OTP_Shading +para_medium\n");
		for (i = 0; i < 108; ++i)
		{
			com_test_step2[i] = com_test_result[i] + para_medium[lsc_version][i];/*chart 1.7 tavis*/			
		}
		
		/*R'=R'',G'=G''+R' , B'=B''+G' */
		//LOG_INF("GC8024_OTP_Shading R'=R'',G'=G''+R' , B'=B''+G\n");
		for (i = 0; i < 36; ++i)
		{
			com_test_step1[i] = com_test_step2[i];//R	
			com_test_step1[36+i] = com_test_step2[i] + com_test_step2[36+i];//G
			com_test_step1[72+i] = com_test_step1[36+i] + com_test_step2[72+i];//B
		}
		/*R/G/B*/
		//LOG_INF("GC8024_OTP_Shading func_revDPC()\n");
		func_revDPC(&com_test_step1[0], ref_value, &Lsc[0]);
		func_revDPC(&com_test_step1[36], ref_value,&Lsc[36]);
		func_revDPC(&com_test_step1[72], ref_value, &Lsc[72]);
		/*1D to 2D*/
		//LOG_INF("GC8024_OTP_Shading 1D to 2D\n");
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_r[i][j] = Lsc[6*i+j];
				//LOG_INF("GC8024_OTP_Shading lsc_r[%d][%d]= 0x%x =%d\n",i,j,gc8024_otp_info.lsc_r[i][j],gc8024_otp_info.lsc_r[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{			
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_g[i][j] = Lsc[36+6*i+j];
				//LOG_INF("GC8024_OTP_Shading lsc_g[%d][%d]= 0x%x=%d \n",i,j,gc8024_otp_info.lsc_g[i][j],gc8024_otp_info.lsc_g[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_b[i][j] = Lsc[72+6*i+j];
				//LOG_INF("GC8024_OTP_Shading lsc_b[%d][%d]= 0x%x=%d \n",i,j,gc8024_otp_info.lsc_b[i][j],gc8024_otp_info.lsc_b[i][j]);
			}
		}	
	}

	
	gc8024_otp_select_page(otp_page1);

	flag2 = gc8024_read_otp(0x36);
	//LOG_INF("GC8024_OTP flag2 = 0x%x\n",flag2);
	LOG_INF("%s(%d) GC8024_OTP flag2 = 0x%x  \n",__FUNCTION__,__LINE__,flag2);
	/*DD*/
    
    if(flag2 == 0x55)
    {
		LOG_INF("GC8024_OTP_DD is Valid !!\n");
		total_number = gc8024_read_otp(0x37) + gc8024_read_otp(0x38);		
		LOG_INF("GC8024_OTP_DD total_number = %d\n",total_number);

		cnt=0;
		
		for(i=0; i<total_number; i++)
		{
			check_dd_flag = gc8024_read_otp(0x39 + 4*i + 3);
				
			if(check_dd_flag&0x10)
			{//Read OTP
				type = check_dd_flag&0x0f;
		
				dd0 = gc8024_read_otp(0x39 + 4*i);
				dd1 = gc8024_read_otp(0x39 + 4*i + 1); 	
				dd2 = gc8024_read_otp(0x39 + 4*i + 2);
				x = ((dd1&0x0f)<<8) + dd0;
				y = (dd2<<4) + ((dd1&0xf0)>>4);
				LOG_INF("GC8024_OTP_DD type = %d , x = %d , y = %d \n",type,x,y);
				LOG_INF("GC8024_OTP_DD dd0 = %d , dd1 = %d , dd2 = %d \n",dd0,dd1,dd2);
				if(type == 3)
				{
					for(j=0; j<4; j++)
					{
						gc8024_otp_info.dd_param_x[cnt] = x;
						gc8024_otp_info.dd_param_y[cnt] = y + j;
						gc8024_otp_info.dd_param_type[cnt++] = 2;
					}
				}
				else
				{
					gc8024_otp_info.dd_param_x[cnt] = x;
					gc8024_otp_info.dd_param_y[cnt] = y;
					gc8024_otp_info.dd_param_type[cnt++] = type;
				}
			}
			else
			{
				LOG_INF("GC8024_OTP_DD check_id[%d] = %x,checkid error!!\n",i,check_dd_flag);
			}
		}
		gc8024_otp_info.dd_cnt = cnt;
		gc8024_otp_info.dd_flag = 0x01;
    }
    else
    {
		LOG_INF("GC8024_OTP_DD is Empty !!\n");
		gc8024_otp_info.dd_flag = 0x00;
    }

}

static void gc8024_gcore_read_otp_info_basic(void)
{
	kal_uint8 flag1,flag2;
	kal_uint8 index,i,j,cnt=0;
	kal_uint8 info_start_add,wb_start_add;
	kal_uint8 total_number=0; 
	kal_uint8 check_dd_flag,type;
	kal_uint8 dd0=0,dd1=0,dd2=0;
	kal_uint16 x,y;
	memset(&gc8024_otp_info,0,sizeof(gc8024_otp));

	gc8024_otp_select_page(otp_page0);
	flag1 = gc8024_read_otp(0x00);

/*LSC*/
	switch((flag1>>6)&0x03)
	{
	case 0x00:
		LOG_INF("GC8024_OTP OTP Shading is Empty !!\n");
		gc8024_otp_info.lsc_flag = 0x00;
		break;
	case 0x01:
		LOG_INF("GC8024_OTP_Shading is Valid !!\n");
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_r[i][j] = gc8024_read_otp(0x01 + 6*i + j);
				LOG_INF("GC8024_OTP_Shading lsc_r_date[%d][%d]= 0x%x \n",i,j,gc8024_otp_info.lsc_r[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{			
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_g[i][j] = gc8024_read_otp(0x01 + 36 + 6*i + j);
				LOG_INF("GC8024_OTP_Shading lsc_g_date[%d][%d]= 0x%x \n",i,j,gc8024_otp_info.lsc_g[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				gc8024_otp_info.lsc_b[i][j] = gc8024_read_otp(0x01 + 72 + 6*i + j);
				LOG_INF("GC8024_OTP_Shading lsc_b_date[%d][%d]= 0x%x \n",i,j,gc8024_otp_info.lsc_b[i][j]);
			}
		}
		gc8024_otp_info.lsc_flag = 0x01;
		break;
	case 0x02:
	case 0x03:	
		LOG_INF("GC8024_OTP_Shading is Invalid !!\n");
		gc8024_otp_info.lsc_flag = 0x02;
		break;
	default :
		break;
	}

/*WB*/
	for(index=0;index<2;index++)
	{
		switch((flag1>>(4 - 2 * index))&0x03)
		{
		case 0x00:
			LOG_INF("GC8024_OTP_WB group%d is Empty !!\n", index + 1);
			gc8024_otp_info.wb_flag = gc8024_otp_info.wb_flag|0x00;
			break;
		case 0x01:
			wb_start_add = 0x6d + index * 2;
			gc8024_otp_info.rg_gain = gc8024_read_otp(wb_start_add);
			gc8024_otp_info.bg_gain = gc8024_read_otp(wb_start_add + 1); 

			if((0==gc8024_otp_info.rg_gain)||(0==gc8024_otp_info.bg_gain))
			{
				gc8024_otp_info.wb_flag = gc8024_otp_info.wb_flag|0x02;
				LOG_INF("GC8024_OTP_WB group%d is Invalid,rg/bg =0 !!\n", index + 1);				
			}	
			else
			{
				gc8024_otp_info.wb_flag = gc8024_otp_info.wb_flag|0x01;
				LOG_INF("GC8024_OTP_WB group%d is Valid !!\n", index + 1);				
			}

			break;
		case 0x02:
		case 0x03:	
			LOG_INF("GC8024_OTP_WB group%d is Invalid !!\n", index + 1);			
			gc8024_otp_info.wb_flag = gc8024_otp_info.wb_flag|0x02;
			break;
		default :
			break;
		}
	}
	
	gc8024_otp_select_page(otp_page1);
	flag2 = gc8024_read_otp(0x00);
	
	LOG_INF("GC8024_OTP flag2 = 0x%x\n",flag2);

/*DD*/
	switch((flag2>>2)&0x03)
	{
	case 0x00:
		LOG_INF("GC8024_OTP_DD is Empty !!\n");
		gc8024_otp_info.dd_flag = 0x00;
		break;
	case 0x01:
		LOG_INF("GC8024_OTP_DD is Valid !!\n");
		
		total_number = gc8024_read_otp(0x0f) + gc8024_read_otp(0x10);
		
		LOG_INF("GC8024_OTP_DD total_number = %d\n",total_number);
		for(i=0; i<total_number; i++)
		{
			check_dd_flag = gc8024_read_otp(0x11 + 4*i + 3);
				
			if(check_dd_flag&0x10)
			{//Read OTP
				type = check_dd_flag&0x0f;
		
				dd0 = gc8024_read_otp(0x11 + 4*i);
				dd1 = gc8024_read_otp(0x11 + 4*i + 1); 	
				dd2 = gc8024_read_otp(0x11 + 4*i + 2);
				x = ((dd1&0x0f)<<8) + dd0;
				y = (dd2<<4) + ((dd1&0xf0)>>4);
				LOG_INF("GC8024_OTP_DD type = %d , x = %d , y = %d \n",type,x,y);
				LOG_INF("GC8024_OTP_DD dd0 = %d , dd1 = %d , dd2 = %d \n",dd0,dd1,dd2);
				if(type == 3)
				{
					for(j=0; j<4; j++)
					{
						gc8024_otp_info.dd_param_x[cnt] = x;
						gc8024_otp_info.dd_param_y[cnt] = y + j;
						gc8024_otp_info.dd_param_type[cnt++] = 2;
					}
				}
				else
				{
					gc8024_otp_info.dd_param_x[cnt] = x;
					gc8024_otp_info.dd_param_y[cnt] = y;
					gc8024_otp_info.dd_param_type[cnt++] = type;
				}
		
			}
			else
			{
				LOG_INF("GC8024_OTP_DD check_id[%d] = %x,checkid error!!\n",i,check_dd_flag);
			}
		}
		gc8024_otp_info.dd_cnt = cnt;
		gc8024_otp_info.dd_flag = 0x01;
		break;
	case 0x02:
	case 0x03:	
		LOG_INF("GC8024_OTP_DD is Invalid !!\n");
		gc8024_otp_info.dd_flag = 0x02;
		break;
	default :
		break;
	}

/*Module Info*/
	for(index=0;index<2;index++)
	{
		switch((flag2>>(6 - 2 * index))&0x03)
		{
		case 0x00:
			LOG_INF("GC8024_OTP_INFO group%d is Empty !!\n", index + 1);
			break;
		case 0x01:
			LOG_INF("GC8024_OTP_INFO group%d is Valid !!\n", index + 1);
			info_start_add = 0x01 + index * 7;
			gc8024_otp_info.module_id = gc8024_read_otp(info_start_add);
			gc8024_otp_info.lens_id = gc8024_read_otp(info_start_add + 1); 
			gc8024_otp_info.vcm_driver_id = gc8024_read_otp(info_start_add + 2)&0x0f;
			gc8024_otp_info.vcm_start_position = gc8024_read_otp(info_start_add + 2)&0xf0;
			gc8024_otp_info.vcm_id = gc8024_read_otp(info_start_add + 3);
			gc8024_otp_info.year = gc8024_read_otp(info_start_add + 4);
			gc8024_otp_info.month = gc8024_read_otp(info_start_add + 5);
			gc8024_otp_info.day = gc8024_read_otp(info_start_add + 6);
			break;
		case 0x02:
		case 0x03:	
			LOG_INF("GC8024_OTP_INFO group%d is Invalid !!\n", index + 1);
			break;
		default :
			break;
		}
		
	}
	
	/*print otp information*/
	LOG_INF("GC8024_OTP_INFO module_id=0x%x\n",gc8024_otp_info.module_id);
	LOG_INF("GC8024_OTP_INFO lens_id=0x%x\n",gc8024_otp_info.lens_id);
	LOG_INF("GC8024_OTP_INFO vcm_id=0x%x\n",gc8024_otp_info.vcm_id);
	LOG_INF("GC8024_OTP_INFO vcm_driver_id=0x%x\n",gc8024_otp_info.vcm_driver_id);
	LOG_INF("GC8024_OTP_INFO vcm_start_position=0x%x\n",gc8024_otp_info.vcm_start_position);
	LOG_INF("GC8024_OTP_INFO data=%d-%d-%d\n",gc8024_otp_info.year,gc8024_otp_info.month,gc8024_otp_info.day);

}

static void gc8024_gcore_read_otp_info_2dd(void)
{
	kal_uint8 flag1,flag2,flag;
	kal_uint8 index,i,j,cnt=0;
	kal_uint8 dd_start_add,info_start_add;
	kal_uint8 total_number=0; 
	kal_uint8 check_dd_flag,type;
	kal_uint8 dd0=0,dd1=0,dd2=0;
	kal_uint16 x,y;
	memset(&gc8024_otp_info,0,sizeof(gc8024_otp));

/*DD*/
	gc8024_otp_select_page(otp_page0);
	flag1 = gc8024_read_otp(0x00);//[7:6]DD G2
	LOG_INF("GC8024_OTP_DD flag1 = 0x%x\n",flag1);
	
	gc8024_otp_select_page(otp_page1);
	flag2 = gc8024_read_otp(0x00);//[3:2]DD G1
	LOG_INF("GC8024_OTP_DD flag2 = 0x%x\n",flag2);

	if(((flag1>>6)&0x03)==0x01)
	{
		LOG_INF("GC8024_OTP_DD Group2 is Valid !!\n");	
		
		gc8024_otp_select_page(otp_page0);
		flag = flag1>>6;
		dd_start_add = 0x03;	
		total_number = gc8024_read_otp(0x01) + gc8024_read_otp(0x02);
		LOG_INF("GC8024_OTP_DD Group2 total_number=%d\n",total_number);
	}
	else if(((flag2>>2)&0x03)==0x01)
	{
		LOG_INF("GC8024_OTP_DD Group1 is Valid !!\n");	

		gc8024_otp_select_page(otp_page1);
		flag = flag2>>2;
		dd_start_add = 0x11;
		total_number = gc8024_read_otp(0x0f) + gc8024_read_otp(0x10);
		LOG_INF("GC8024_OTP_DD Group1 total_number=%d\n",total_number);
	}	
	else
	{
		LOG_INF("GC8024_OTP_DD is Empty or Invaild!!\n");
		flag = 0;
		gc8024_otp_info.dd_flag = 0x00;		
	}

	if((flag&0x03)==1)
	{
		for(i=0; i<total_number; i++)
		{
			check_dd_flag = gc8024_read_otp(dd_start_add  + 4*i + 3);
				
			if(check_dd_flag&0x10)
			{//Read OTP
				type = check_dd_flag&0x0f;
		
				dd0 = gc8024_read_otp(dd_start_add  + 4*i);
				dd1 = gc8024_read_otp(dd_start_add  + 4*i + 1);	
				dd2 = gc8024_read_otp(dd_start_add  + 4*i + 2);
				x = ((dd1&0x0f)<<8) + dd0;
				y = (dd2<<4) + ((dd1&0xf0)>>4);
				LOG_INF("GC8024_OTP_DD type = %d , x = %d , y = %d \n",type,x,y);
				LOG_INF("GC8024_OTP_DD dd0 = %d , dd1 = %d , dd2 = %d \n",dd0,dd1,dd2);
				if(type == 3)
				{
					for(j=0; j<4; j++)
					{
						gc8024_otp_info.dd_param_x[cnt] = x;
						gc8024_otp_info.dd_param_y[cnt] = y + j;
						gc8024_otp_info.dd_param_type[cnt++] = 2;
					}
				}
				else
				{
					gc8024_otp_info.dd_param_x[cnt] = x;
					gc8024_otp_info.dd_param_y[cnt] = y;
					gc8024_otp_info.dd_param_type[cnt++] = type;
				}
		
			}
			else
			{
				LOG_INF("GC8024_OTP_DD check_id[%d] = %x,checkid error!!\n",i,check_dd_flag);
			}
		}
		gc8024_otp_info.dd_cnt = cnt;
		gc8024_otp_info.dd_flag = 0x01;
	}
	
/*Module Info*/
	for(index=0;index<2;index++)
	{
		switch((flag2>>(6 - 2 * index))&0x03)
		{
		case 0x00:
			LOG_INF("GC8024_OTP_INFO group%d is Empty !!\n", index + 1);
			break;
		case 0x01:
			LOG_INF("GC8024_OTP_INFO group%d is Valid !!\n", index + 1);
			info_start_add = 0x01 + index * 7;
			gc8024_otp_info.module_id = gc8024_read_otp(info_start_add);
			gc8024_otp_info.lens_id = gc8024_read_otp(info_start_add + 1); 
			gc8024_otp_info.vcm_driver_id = gc8024_read_otp(info_start_add + 2)&0x0f;
			gc8024_otp_info.vcm_start_position = gc8024_read_otp(info_start_add + 2)&0xf0;
			gc8024_otp_info.vcm_id = gc8024_read_otp(info_start_add + 3);
			gc8024_otp_info.year = gc8024_read_otp(info_start_add + 4);
			gc8024_otp_info.month = gc8024_read_otp(info_start_add + 5);
			gc8024_otp_info.day = gc8024_read_otp(info_start_add + 6);
			break;
		case 0x02:
		case 0x03:	
			LOG_INF("GC8024_OTP_INFO group%d is Invalid !!\n", index + 1);
			break;
		default :
			break;
		}
		
	}	
	/*print otp information*/
	LOG_INF("GC8024_OTP_INFO module_id=0x%x\n",gc8024_otp_info.module_id);
	LOG_INF("GC8024_OTP_INFO lens_id=0x%x\n",gc8024_otp_info.lens_id);
	LOG_INF("GC8024_OTP_INFO vcm_id=0x%x\n",gc8024_otp_info.vcm_id);
	LOG_INF("GC8024_OTP_INFO vcm_driver_id=0x%x\n",gc8024_otp_info.vcm_driver_id);
	LOG_INF("GC8024_OTP_INFO vcm_start_position=0x%x\n",gc8024_otp_info.vcm_start_position);
	LOG_INF("GC8024_OTP_INFO data=%d-%d-%d\n",gc8024_otp_info.year,gc8024_otp_info.month,gc8024_otp_info.day); 		
}

static void gc8024_gcore_update_dd(void)
{
	kal_uint16 i=0,j=0,n=0,m=0,s=0,e=0;
	kal_uint16 temp_x=0,temp_y=0;
	kal_uint8 flag=0;
	kal_uint8 temp_type=0;
	kal_uint8 temp_val0,temp_val1,temp_val2;
	/*TODO*/

	if(0x01 ==gc8024_otp_info.dd_flag)
	{
#if defined(IMAGE_NORMAL_MIRROR)
		for(i=0; i<gc8024_otp_info.dd_cnt; i++)
		{
			if(gc8024_otp_info.dd_param_type[i]==0)
			{	gc8024_otp_info.dd_param_x[i]= WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i] + 1;	}
			else if(gc8024_otp_info.dd_param_type[i]==1)
			{	gc8024_otp_info.dd_param_x[i]= WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i] - 1;	}
			else
			{	gc8024_otp_info.dd_param_x[i]= WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i];	}
		}
#elif defined(IMAGE_H_MIRROR)
		//do nothing
#elif defined(IMAGE_V_MIRROR)
		for(i=0; i<gc8024_otp_info.dd_cnt; i++)
		{
			if(gc8024_otp_info.dd_param_type[i]==0)
			{	
				gc8024_otp_info.dd_param_x[i] = WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i]+1;
				gc8024_otp_info.dd_param_y[i] = WINDOW_HEIGHT - gc8024_otp_info.dd_param_y[i]+1;
			}
			else if(gc8024_otp_info.dd_param_type[i]==1)
			{
				gc8024_otp_info.dd_param_x[i] = WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i] - 1;
				gc8024_otp_info.dd_param_y[i] = WINDOW_HEIGHT - gc8024_otp_info.dd_param_y[i] + 1;
			}
			else
			{
				gc8024_otp_info.dd_param_x[i] = WINDOW_WIDTH - gc8024_otp_info.dd_param_x[i] ;
				gc8024_otp_info.dd_param_y[i] = WINDOW_HEIGHT - gc8024_otp_info.dd_param_y[i] + 1;
			}
		}
#elif defined(IMAGE_HV_MIRROR)
		for(i=0; i<gc8024_otp_info.dd_cnt; i++)
		{	gc8024_otp_info.dd_param_y[i]= WINDOW_HEIGHT - gc8024_otp_info.dd_param_y[i] + 1;	}
#endif

		//y
		for(i=0 ; i< gc8024_otp_info.dd_cnt-1; i++) 
		{
			for(j = i+1; j < gc8024_otp_info.dd_cnt; j++) 
			{  
				if(gc8024_otp_info.dd_param_y[i] > gc8024_otp_info.dd_param_y[j])  
				{  
					temp_x = gc8024_otp_info.dd_param_x[i] ; gc8024_otp_info.dd_param_x[i] = gc8024_otp_info.dd_param_x[j] ;  gc8024_otp_info.dd_param_x[j] = temp_x;
					temp_y = gc8024_otp_info.dd_param_y[i] ; gc8024_otp_info.dd_param_y[i] = gc8024_otp_info.dd_param_y[j] ;  gc8024_otp_info.dd_param_y[j] = temp_y;
					temp_type = gc8024_otp_info.dd_param_type[i] ; gc8024_otp_info.dd_param_type[i] = gc8024_otp_info.dd_param_type[j]; gc8024_otp_info.dd_param_type[j]= temp_type;
				} 
			}
		
		}
		
		//x
		for(i=0; i<gc8024_otp_info.dd_cnt; i++)
		{
			if(gc8024_otp_info.dd_param_y[i]==gc8024_otp_info.dd_param_y[i+1])
			{
				s=i++;
				while((gc8024_otp_info.dd_param_y[s] == gc8024_otp_info.dd_param_y[i+1])&&(i<gc8024_otp_info.dd_cnt-1))
					i++;
				e=i;

				for(n=s; n<e; n++)
				{
					for(m=n+1; m<e+1; m++)
					{
						if(gc8024_otp_info.dd_param_x[n] > gc8024_otp_info.dd_param_x[m])
						{
							temp_x = gc8024_otp_info.dd_param_x[n] ; gc8024_otp_info.dd_param_x[n] = gc8024_otp_info.dd_param_x[m] ;  gc8024_otp_info.dd_param_x[m] = temp_x;
							temp_y = gc8024_otp_info.dd_param_y[n] ; gc8024_otp_info.dd_param_y[n] = gc8024_otp_info.dd_param_y[m] ;  gc8024_otp_info.dd_param_y[m] = temp_y;
							temp_type = gc8024_otp_info.dd_param_type[n] ; gc8024_otp_info.dd_param_type[n] = gc8024_otp_info.dd_param_type[m]; gc8024_otp_info.dd_param_type[m]= temp_type;
						}
					}
				}

			}

		}

		
		//write SRAM
		write_cmos_sensor(0xfe,0x01);
		write_cmos_sensor(0xbe,0x00);
		write_cmos_sensor(0xa9,0x01);

		for(i=0; i<gc8024_otp_info.dd_cnt; i++)
		{
			temp_val0 = gc8024_otp_info.dd_param_x[i]& 0x00ff;
			temp_val1 = ((gc8024_otp_info.dd_param_y[i]<<4)& 0x00f0) + ((gc8024_otp_info.dd_param_x[i]>>8)&0X000f);
			temp_val2 = (gc8024_otp_info.dd_param_y[i]>>4) & 0xff;
			write_cmos_sensor(0xaa,i);
			write_cmos_sensor(0xac,temp_val0);
			write_cmos_sensor(0xac,temp_val1);
			write_cmos_sensor(0xac,temp_val2);
			while((i < gc8024_otp_info.dd_cnt - 1) && (gc8024_otp_info.dd_param_x[i]==gc8024_otp_info.dd_param_x[i+1]) && (gc8024_otp_info.dd_param_y[i]==gc8024_otp_info.dd_param_y[i+1]))
				{
					flag = 1;
					i++;
				}
			if(flag)
				write_cmos_sensor(0xac,0x02);
			else
				write_cmos_sensor(0xac,gc8024_otp_info.dd_param_type[i]);
 
			LOG_INF("GC8024_OTP_DD val0 = 0x%x , val1 = 0x%x , val2 = 0x%x \n",temp_val0,temp_val1,temp_val2);
			LOG_INF("GC8024_OTP_DD x = %d , y = %d \n",((temp_val1&0x0f)<<8) + temp_val0,(temp_val2<<4) + ((temp_val1&0xf0)>>4));	
		}

		write_cmos_sensor(0xbe,0x01);
		write_cmos_sensor(0xfe,0x00);
	}

}


static void gc8024_gcore_update_awb(void)
{
	kal_uint32 r_gain_current = 0 , g_gain_current = 0 , b_gain_current = 0 , base_gain = 0;
	kal_uint32 r_gain = 0 , g_gain = 0 , b_gain = 0 ;

	if(!gc8024_otp_info.flag)
	{
		r_gain_current = 256 * RG_TYPICAL /gc8024_otp_info.rg_gain;
		b_gain_current = 256 * BG_TYPICAL/gc8024_otp_info.bg_gain;
		g_gain_current = 256;

		base_gain = (r_gain_current<b_gain_current) ? r_gain_current : b_gain_current;
		base_gain = (base_gain<g_gain_current) ? base_gain : g_gain_current;

		r_gain = 0x80 * r_gain_current / base_gain;
		g_gain = 0x80 * g_gain_current / base_gain;
		b_gain = 0x80 * b_gain_current / base_gain;
		
		LOG_INF("GC8024_OTP_WB R/G=0x%x,B/G=0x%x",gc8024_otp_info.rg_gain,gc8024_otp_info.bg_gain);
		LOG_INF("GC8024 OTP r_gain_current = 0x%x , b_gain_current = 0x%x , base_gain = 0x%x \n",r_gain_current,b_gain_current,base_gain);
		LOG_INF("GC8024 OTP r_gain = 0x%x , g_gain = 0x%x , b_gain = 0x%x \n",r_gain,g_gain,b_gain);

		write_cmos_sensor(0xfe,0x00);
		write_cmos_sensor(0xb8,g_gain);
		write_cmos_sensor(0xb9,g_gain);
		write_cmos_sensor(0xba,r_gain);
		write_cmos_sensor(0xbb,r_gain);
		write_cmos_sensor(0xbc,b_gain);
		write_cmos_sensor(0xbd,b_gain);
		write_cmos_sensor(0xbe,g_gain);
		write_cmos_sensor(0xbf,g_gain);
	}

}

static void gc8024_gcore_update_lsc(void)
{
	kal_uint8 i, j, cnter = 0;
	LOG_INF("gc8024_otp_info.lsc_flag=%d\n",gc8024_otp_info.lsc_flag);
	/*TODO*/
	if(0x01 == gc8024_otp_info.lsc_flag)
	{
		write_cmos_sensor(0xfe,0x00);	
		write_cmos_sensor(0x8d,0x03);//lsc_sram_clock
		write_cmos_sensor(0xfe,0x01);
		write_cmos_sensor(0xc9,0x01);
				
#ifdef IMAGE_NORMAL_MIRROR
		cnter = 0;
		write_cmos_sensor(0xcf,0x08);//R
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				write_cmos_sensor(0xca,cnter++);
				write_cmos_sensor(0xcc,gc8024_otp_info.lsc_r[i][j-1]);
			}
		}
		cnter = 0;
		write_cmos_sensor(0xcf,0x04);//G
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				write_cmos_sensor(0xca,cnter++);
				write_cmos_sensor(0xcc,gc8024_otp_info.lsc_g[i][j-1]);
			}
		}
		cnter = 0;
		write_cmos_sensor(0xcf,0x02);//B
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				write_cmos_sensor(0xca,cnter++);
				write_cmos_sensor(0xcc,gc8024_otp_info.lsc_b[i][j-1]);
			}
		}

#endif
				
#ifdef IMAGE_H_MIRROR
			cnter = 0;
			write_cmos_sensor(0xcf,0x08);//R
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_r[i][j]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x04);//G
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_g[i][j]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x02);//B
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_b[i][j]);
				}
			}
			
#endif
				
#ifdef IMAGE_V_MIRROR
			cnter = 0;
			write_cmos_sensor(0xcf,0x08);//R
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_r[i-1][j-1]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x04);//G
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_g[i-1][j-1]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x02);//B
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_b[i-1][j-1]);
				}
			}		
#endif
				
#ifdef IMAGE_HV_MIRROR
			cnter = 0;
			write_cmos_sensor(0xcf,0x08);//R
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_r[i-1][j]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x04);//G
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_g[i-1][j]);
				}
			}
			cnter = 0;
			write_cmos_sensor(0xcf,0x02);//B
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					write_cmos_sensor(0xca,cnter++);
					write_cmos_sensor(0xcc,gc8024_otp_info.lsc_b[i-1][j]);
				}
			}		
#endif

			write_cmos_sensor(0xcf,0x01);

			if(lsc_version == 2)
				write_cmos_sensor(0xbc,0xa0);
			else
				write_cmos_sensor(0xbc,0x80);

			write_cmos_sensor(0xa0,0x61);//[0]LSC Enable	
			write_cmos_sensor(0xa3,0x4f);
			write_cmos_sensor(0xfe,0x00);
	}
}

static void gc8024_gcore_update_otp(void)
{
	gc8024_gcore_update_dd();
	gc8024_gcore_update_awb();
	gc8024_gcore_update_lsc();
}


static void gc8024_gcore_enable_otp(otp_state state)
{
	kal_uint8 otp_clk,otp_en;
	otp_clk = read_cmos_sensor(0xfa);
	otp_en= read_cmos_sensor(0xd4);	
	if(state)	
	{ 
		otp_clk = otp_clk | 0x40;
		otp_en = otp_en | 0x80;
		mdelay(5);
		write_cmos_sensor(0xfa,otp_clk);	// 0xfa[6]:OTP_CLK_en
		write_cmos_sensor(0xd4,otp_en);	// 0xd4[7]:OTP_en	
	
		LOG_INF("Enter GC8024_MIPI_enable_OTP: Enable OTP!\n");		
	}
	else			
	{
		otp_en = otp_en & 0x7f;
		otp_clk = otp_clk & 0xbf;
		mdelay(5);
		write_cmos_sensor(0xd4,otp_en);
		write_cmos_sensor(0xfa,otp_clk);

		LOG_INF("Enter GC8024_MIPI_enable_OTP: Disable OTP!\n");
	}

}


static void gc8024_gcore_identify_otp(void)
{
	kal_uint8 otp_version;
	
#ifdef GC8024MIPI_4LANE
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x9d);
	write_cmos_sensor(0xf8,0x08);
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x85);
	write_cmos_sensor(0xfc,0xca);
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x18,0x07);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfc,0xce);
#else	
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x95); 
	write_cmos_sensor(0xf8,0x08); 
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x84);
	write_cmos_sensor(0xfc,0xce); 
#endif		

	gc8024_gcore_enable_otp(otp_open);
	
	gc8024_otp_select_page(otp_page1);
	otp_version = gc8024_read_otp(0x7f);
	LOG_INF("GC8024_OTP_ID version=%d\n",otp_version);

	if(((otp_version>>2)&0x03)==1)	
	{
		LOG_INF("GC8024_OTP_ID otp_version=new\n");	
		gc8024_gcore_read_otp_info_new();		
	}
	else if((otp_version&0x03)==1)
	{		
		LOG_INF("GC8024_OTP_ID version=2dd\n");	
		gc8024_gcore_read_otp_info_2dd();	
	}
	else
	{
		LOG_INF("GC8024_OTP_ID version=basic\n");	
		gc8024_gcore_read_otp_info_basic();
	}
	
	gc8024_gcore_enable_otp(otp_close);
}

#endif


static void set_dummy(void)
{
	kal_uint32 vb = 24;
	kal_uint32 basic_line = 2476;

	vb = imgsensor.frame_length*4 - basic_line;

	vb = (vb > 24 )? vb : 24;
	write_cmos_sensor(0x07, (vb >> 8) & 0x1F);
    write_cmos_sensor(0x08, vb & 0xFF);
	
}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor(0xf0) << 8) | read_cmos_sensor(0xf1));
}

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

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
    //kal_uint8 shutter_temp;
    kal_uint16 realtime_fps = 0;
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
    shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
    shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
	
	//set_max_framerate(realtime_fps, 0);
	
	// Update Shutter	

	shutter = shutter << 2;

	write_cmos_sensor(0xfe, 0x00);
	
	if(shutter <= TxLowTh)
  		write_cmos_sensor(0x30, 0xfc);
  	else 
		write_cmos_sensor(0x30, 0xf8);

	//Update Shutter
	//write_cmos_sensor(0xfe, 0x00);	
	write_cmos_sensor(0x03, (shutter>>8) & 0x7F);
	write_cmos_sensor(0x04, shutter & 0xFF);

    LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */


/*
static kal_uint16 gain2reg(const kal_uint16 gain)
{
    kal_uint16 reg_gain = 0x0000;

    reg_gain = ((gain / BASEGAIN) << 4) + ((gain % BASEGAIN) * 16 / BASEGAIN);
    reg_gain = reg_gain & 0xFFFF;
    return (kal_uint16)reg_gain;
}*/

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
	kal_uint16 iReg,temp; 	

	iReg = gain;
	
	if(iReg < 0x40)
		iReg = 0x40;
	
	if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))
	{
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0x21, 0x0b);
		write_cmos_sensor(0xd8, 0x20);		
		//analog gain
		write_cmos_sensor(0xb6, 0x00);
		temp = iReg;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 1x, GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 0;
	}
	else if((ANALOG_GAIN_2<= iReg)&&(iReg < ANALOG_GAIN_3))
	{
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0x21, 0x0b);
		write_cmos_sensor(0xd8, 0x20);
		//analog gain
		write_cmos_sensor(0xb6, 0x01); 
		temp = 64*iReg/ANALOG_GAIN_2;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 1.375x , GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 1;
	}
	else if((ANALOG_GAIN_3<= iReg)&&(iReg < ANALOG_GAIN_4))
	{
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0x21, 0x10);
		write_cmos_sensor(0xd8, 0x20);		
		//analog gain
		write_cmos_sensor(0xb6, 0x02);
		temp = 64*iReg/ANALOG_GAIN_3;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 1.891x , GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 1;
	}
	else if((ANALOG_GAIN_4<= iReg)&&(iReg < ANALOG_GAIN_5))
	{
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0x21, 0x10);
		write_cmos_sensor(0xd8, 0x20);	
		//analog gain
		write_cmos_sensor(0xb6, 0x03);
		temp = 64*iReg/ANALOG_GAIN_4;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 2.625x , GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 2;
	}
	else if((ANALOG_GAIN_5<= iReg)&&(iReg < ANALOG_GAIN_6))
	{
		write_cmos_sensor(0xfe, 0x00); 
		write_cmos_sensor(0x21, 0x10);
		write_cmos_sensor(0xd8, 0x18);	
		//analog gain
		write_cmos_sensor(0xb6, 0x04);
		temp = 64*iReg/ANALOG_GAIN_5;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 3.734x , GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 2;
	}
	else
	{
		write_cmos_sensor(0xfe, 0x00);
		write_cmos_sensor(0x21, 0x12);
		write_cmos_sensor(0xd8, 0x12);			
		//analog gain
		write_cmos_sensor(0xb6,  0x05);
		temp = 64*iReg/ANALOG_GAIN_6;
		write_cmos_sensor(0xb1, temp>>6);
		write_cmos_sensor(0xb2, (temp<<2)&0xfc);
		LOG_INF("GC8024MIPI analogic gain 5.250x , GC8024MIPI add pregain = %d\n",temp);
		gainlevel = 3;
	}
	write_cmos_sensor(0x28,Val28[PreorCap][gainlevel]);
	return gain;

}    /*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
    LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);

}


/*
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

}
*/
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
	/*System*/
#ifdef GC8024MIPI_4LANE
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x9d);
	write_cmos_sensor(0xf8,0x08);
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x85);
	write_cmos_sensor(0xfc,0xca);
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x18,0x07);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfc,0xce);
#else	
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x95); 
	write_cmos_sensor(0xf8,0x08); 
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x84);
	write_cmos_sensor(0xfc,0xce); 
#endif	
	/*Analog&CISCTL*/
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0x03,0x08);
	write_cmos_sensor(0x04,0xca);
	write_cmos_sensor(0x05,0x02);
	write_cmos_sensor(0x06,0x1c);
	write_cmos_sensor(0x07,0x00);
	write_cmos_sensor(0x08,0x18);
	write_cmos_sensor(0x09,0x00);
	write_cmos_sensor(0x0a,0x14); 
	write_cmos_sensor(0x0b,0x00);
	write_cmos_sensor(0x0c,0x10); 
	write_cmos_sensor(0x0d,0x09);
	write_cmos_sensor(0x0e,0x9c); 
	write_cmos_sensor(0x0f,0x0c);
	write_cmos_sensor(0x10,0xd0); 
	write_cmos_sensor(0x17,MIRROR); 	
	write_cmos_sensor(0x18,0x02); 	
	write_cmos_sensor(0x19,0x0b);
	write_cmos_sensor(0x1a,0x19);	
	write_cmos_sensor(0x1c,0x0c);
	write_cmos_sensor(0x1d,0x11);//20160510 add
	write_cmos_sensor(0x21,0x12);
	write_cmos_sensor(0x23,0xb0);
	write_cmos_sensor(0x28,0xdf);
	
	write_cmos_sensor(0x29,0xd4); 
	write_cmos_sensor(0x2f,0x4c); 
	write_cmos_sensor(0x30,0xf8);
	write_cmos_sensor(0xcd,0x9a);
	write_cmos_sensor(0xce,0xfd);
	write_cmos_sensor(0xd0,0xd2);
	write_cmos_sensor(0xd1,0xa8);
	write_cmos_sensor(0xd3,0x35);
	write_cmos_sensor(0xd8,0x20);
	write_cmos_sensor(0xda,0x03);
	write_cmos_sensor(0xdb,0x4e);
	write_cmos_sensor(0xdc,0xb3);
	write_cmos_sensor(0xde,0x40);
	write_cmos_sensor(0xe1,0x1a); 
	write_cmos_sensor(0xe2,0x00);	
	write_cmos_sensor(0xe3,0x71);
	write_cmos_sensor(0xe4,0x78);	
	write_cmos_sensor(0xe5,0x44);
	write_cmos_sensor(0xe6,0xdf);
	write_cmos_sensor(0xe8,0x02);
	write_cmos_sensor(0xe9,0x01);
	write_cmos_sensor(0xea,0x01);
	write_cmos_sensor(0xeb,0x02);
	write_cmos_sensor(0xec,0x02);
	write_cmos_sensor(0xed,0x01);
	write_cmos_sensor(0xee,0x01);
	write_cmos_sensor(0xef,0x02);

	/*ISP*/
	write_cmos_sensor(0x80,0x50);
	write_cmos_sensor(0x88,0x03);
	write_cmos_sensor(0x89,0x03);
	
	/*Crop Window*/
	write_cmos_sensor(0x90,0x01);
	write_cmos_sensor(0x92,PreStartY);
	write_cmos_sensor(0x94,PreStartX);
	write_cmos_sensor(0x95,0x04);
	write_cmos_sensor(0x96,0xc8);
	write_cmos_sensor(0x97,0x06);
	write_cmos_sensor(0x98,0x60);
	
	/*Gain*/
	write_cmos_sensor(0xb0,0x70);
	write_cmos_sensor(0xb1,0x01);
	write_cmos_sensor(0xb2,0x00);
	write_cmos_sensor(0xb6,0x00);
	write_cmos_sensor(0xfe,0x01);	
	write_cmos_sensor(0x50,0x00);
	write_cmos_sensor(0x51,0x08);
	write_cmos_sensor(0x52,0x10);
	write_cmos_sensor(0x53,0x18);
	write_cmos_sensor(0x54,0x19);
	write_cmos_sensor(0x55,0x1a);
	write_cmos_sensor(0x56,0x1b);
	write_cmos_sensor(0x57,0x1c);
	write_cmos_sensor(0x58,0x3c);
	write_cmos_sensor(0x59,0x5c);
	write_cmos_sensor(0x5a,0x7c);
	write_cmos_sensor(0xfe,0x00);	
	
	/*BLK*/
	write_cmos_sensor(0x40,0x22);
	write_cmos_sensor(0x41,0x20);
	write_cmos_sensor(0x42,0x10);
	write_cmos_sensor(0x4e,0x00);
	write_cmos_sensor(0x4f,0x3c);
	write_cmos_sensor(0x60,0x00);
	write_cmos_sensor(0x61,0x80);
	write_cmos_sensor(0x66,0x3c);
	write_cmos_sensor(0x69,0x03);
	write_cmos_sensor(0x6c,0x00);
	write_cmos_sensor(0x6d,0x0f);
	
	/*Dark Offset*/
	write_cmos_sensor(0x35,0x30);
	write_cmos_sensor(0x36,0x00);	
	
	/*Dark Sun*/
	write_cmos_sensor(0x37,0x70);
	write_cmos_sensor(0x38,0x80);	
	write_cmos_sensor(0x3b,0xf0);
	write_cmos_sensor(0x3d,0x00);	
	
	/*DD*/
	write_cmos_sensor(0xfe,0x01);
	write_cmos_sensor(0xc2,0x03);
	write_cmos_sensor(0xc3,0x00);	
	write_cmos_sensor(0xc4,0xd8);
	write_cmos_sensor(0xc5,0x00);
	
	/*MIPI*/
#ifdef GC8024MIPI_4LANE
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0x00);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf8);
	write_cmos_sensor(0x13,0x07);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x19,0x30);
	write_cmos_sensor(0x1a,0x03);
	write_cmos_sensor(0x21,0x10);
	write_cmos_sensor(0x22,0x01);
	write_cmos_sensor(0x23,0x06);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x10);
	write_cmos_sensor(0x26,0x03);
	write_cmos_sensor(0x29,0x01);
	write_cmos_sensor(0x2a,0x0a);
	write_cmos_sensor(0x2b,0x03);
	write_cmos_sensor(0xfe,0x00);
#else	
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0x00);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf8);
	write_cmos_sensor(0x13,0x07);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x18,0x01);
	write_cmos_sensor(0x19,0x00);
	write_cmos_sensor(0x1a,0x00);
	write_cmos_sensor(0x21,0x10);
	write_cmos_sensor(0x22,0x02);
	write_cmos_sensor(0x23,0x10);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x12);
	write_cmos_sensor(0x26,0x04);
	write_cmos_sensor(0x29,0x02);
	write_cmos_sensor(0x2a,0x0a);
	write_cmos_sensor(0x2b,0x04);	
	write_cmos_sensor(0xfe,0x00);
#endif		
}    /*    sensor_init  */


static void preview_setting(void)
{
	/*System*/
#ifdef GC8024MIPI_4LANE
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x9d);
	write_cmos_sensor(0xf8,0x08);
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x85);
	write_cmos_sensor(0xfc,0xca);
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x18,0x07);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfc,0xce);
#else	
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x95); 
	write_cmos_sensor(0xf8,0x08); 
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x84);
	write_cmos_sensor(0xfc,0xce); 
#endif	
	/*Analog&CISCTL*/
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0x03,0x08);
	write_cmos_sensor(0x04,0xca);
	write_cmos_sensor(0x05,0x02);
	write_cmos_sensor(0x06,0x1c);
	write_cmos_sensor(0x07,0x00);
	write_cmos_sensor(0x08,0x18);
	write_cmos_sensor(0x09,0x00);
	write_cmos_sensor(0x0a,0x14); 
	write_cmos_sensor(0x0b,0x00);
	write_cmos_sensor(0x0c,0x10); 
	write_cmos_sensor(0x0d,0x09);
	write_cmos_sensor(0x0e,0x9c); 
	write_cmos_sensor(0x0f,0x0c);
	write_cmos_sensor(0x10,0xd0); 
	write_cmos_sensor(0x17,MIRROR); 	
	write_cmos_sensor(0x18,0x02); 	
	write_cmos_sensor(0x19,0x0b);
	write_cmos_sensor(0x1a,0x19);	
	write_cmos_sensor(0x1c,0x0c);
	write_cmos_sensor(0x1d,0x11);//20160510 add
	write_cmos_sensor(0x21,0x12);
	write_cmos_sensor(0x23,0xb0);
	write_cmos_sensor(0x28,0xdf);
	
	write_cmos_sensor(0x29,0xd4); 
	write_cmos_sensor(0x2f,0x4c); 
	write_cmos_sensor(0x30,0xf8);
	write_cmos_sensor(0xcd,0x9a);
	write_cmos_sensor(0xce,0xfd);
	write_cmos_sensor(0xd0,0xd2);
	write_cmos_sensor(0xd1,0xa8);
	write_cmos_sensor(0xd3,0x35);
	write_cmos_sensor(0xd8,0x20);
	write_cmos_sensor(0xda,0x03);
	write_cmos_sensor(0xdb,0x4e);
	write_cmos_sensor(0xdc,0xb3);
	write_cmos_sensor(0xde,0x40);
	write_cmos_sensor(0xe1,0x1a); 
	write_cmos_sensor(0xe2,0x00);	
	write_cmos_sensor(0xe3,0x71);
	write_cmos_sensor(0xe4,0x78);	
	write_cmos_sensor(0xe5,0x44);
	write_cmos_sensor(0xe6,0xdf);
	write_cmos_sensor(0xe8,0x02);
	write_cmos_sensor(0xe9,0x01);
	write_cmos_sensor(0xea,0x01);
	write_cmos_sensor(0xeb,0x02);
	write_cmos_sensor(0xec,0x02);
	write_cmos_sensor(0xed,0x01);
	write_cmos_sensor(0xee,0x01);
	write_cmos_sensor(0xef,0x02);

	/*ISP*/
	write_cmos_sensor(0x80,0x50);
	write_cmos_sensor(0x88,0x03);
	write_cmos_sensor(0x89,0x03);
	
	/*Crop Window*/
	write_cmos_sensor(0x90,0x01);
	write_cmos_sensor(0x92,PreStartY);
	write_cmos_sensor(0x94,PreStartX);
	write_cmos_sensor(0x95,0x04);
	write_cmos_sensor(0x96,0xc8);
	write_cmos_sensor(0x97,0x06);
	write_cmos_sensor(0x98,0x60);
	
	/*Gain*/
	write_cmos_sensor(0xb0,0x70);
	write_cmos_sensor(0xb1,0x01);
	write_cmos_sensor(0xb2,0x00);
	write_cmos_sensor(0xb6,0x00);
	write_cmos_sensor(0xfe,0x01);	
	write_cmos_sensor(0x50,0x00);
	write_cmos_sensor(0x51,0x08);
	write_cmos_sensor(0x52,0x10);
	write_cmos_sensor(0x53,0x18);
	write_cmos_sensor(0x54,0x19);
	write_cmos_sensor(0x55,0x1a);
	write_cmos_sensor(0x56,0x1b);
	write_cmos_sensor(0x57,0x1c);
	write_cmos_sensor(0x58,0x3c);
	write_cmos_sensor(0x59,0x5c);
	write_cmos_sensor(0x5a,0x7c);
	write_cmos_sensor(0xfe,0x00);	
	
	/*BLK*/
	write_cmos_sensor(0x40,0x22);
	write_cmos_sensor(0x41,0x20);
	write_cmos_sensor(0x42,0x10);
	write_cmos_sensor(0x4e,0x00);
	write_cmos_sensor(0x4f,0x3c);
	write_cmos_sensor(0x60,0x00);
	write_cmos_sensor(0x61,0x80);
	write_cmos_sensor(0x66,0x3c);
	write_cmos_sensor(0x69,0x03);
	write_cmos_sensor(0x6c,0x00);
	write_cmos_sensor(0x6d,0x0f);
	
	/*Dark Offset*/
	write_cmos_sensor(0x35,0x30);
	write_cmos_sensor(0x36,0x00);	
	
	/*Dark Sun*/
	write_cmos_sensor(0x37,0x70);
	write_cmos_sensor(0x38,0x80);	
	write_cmos_sensor(0x3b,0xf0);
	write_cmos_sensor(0x3d,0x00);	
	
	/*DD*/
	write_cmos_sensor(0xfe,0x01);
	write_cmos_sensor(0xc2,0x03);
	write_cmos_sensor(0xc3,0x00);	
	write_cmos_sensor(0xc4,0xd8);
	write_cmos_sensor(0xc5,0x00);
	
	/*MIPI*/
#ifdef GC8024MIPI_4LANE
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0xd0);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf8);
	write_cmos_sensor(0x13,0x07);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x19,0x30);
	write_cmos_sensor(0x1a,0x03);
	write_cmos_sensor(0x21,0x10);
	write_cmos_sensor(0x22,0x01);
	write_cmos_sensor(0x23,0x06);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x10);
	write_cmos_sensor(0x26,0x03);
	write_cmos_sensor(0x29,0x01);
	write_cmos_sensor(0x2a,0x0a);
	write_cmos_sensor(0x2b,0x03);
	write_cmos_sensor(0xfe,0x00);
#else	
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0x91);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf8);
	write_cmos_sensor(0x13,0x07);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x18,0x01);
	write_cmos_sensor(0x19,0x00);
	write_cmos_sensor(0x1a,0x00);
	write_cmos_sensor(0x21,0x10);
	write_cmos_sensor(0x22,0x02);
	write_cmos_sensor(0x23,0x10);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x12);
	write_cmos_sensor(0x26,0x04);
	write_cmos_sensor(0x29,0x02);
	write_cmos_sensor(0x2a,0x0a);
	write_cmos_sensor(0x2b,0x04);	
	write_cmos_sensor(0xfe,0x00);
#endif	
	TxLowTh = 150;	
}    /*    preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	/*System*/
#ifdef GC8024MIPI_4LANE	
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x95);
	write_cmos_sensor(0xf8,0x08);
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x05);
	write_cmos_sensor(0xfc,0xca);
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x18,0x07);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfc,0xce);
#else
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0xf7,0x95);
	write_cmos_sensor(0xf8,0x08);
	write_cmos_sensor(0xf9,0x00);
	write_cmos_sensor(0xfa,0x09);
	write_cmos_sensor(0xfc,0xce);
#endif
	/*Analog&CISCTL*/
	write_cmos_sensor(0xfe,0x00);
	write_cmos_sensor(0x03,0x08);
	write_cmos_sensor(0x04,0xca);
	write_cmos_sensor(0x05,0x02);
	write_cmos_sensor(0x06,0x1c);
	write_cmos_sensor(0x07,0x00);
	write_cmos_sensor(0x08,0x18);
	write_cmos_sensor(0x09,0x00);
	write_cmos_sensor(0x0a,0x14);
	write_cmos_sensor(0x0b,0x00);
	write_cmos_sensor(0x0c,0x10);
	write_cmos_sensor(0x0d,0x09);
	write_cmos_sensor(0x0e,0x9c);
	write_cmos_sensor(0x0f,0x0c);
	write_cmos_sensor(0x10,0xd0);

	write_cmos_sensor(0x17, MIRROR);
	write_cmos_sensor(0x18,0x02);
	write_cmos_sensor(0x19,0x0b);//9d
	write_cmos_sensor(0x1a,0x19);
	write_cmos_sensor(0x1c,0x0c);//0c
	write_cmos_sensor(0x1d,0x11);//20160510 add
	write_cmos_sensor(0x21,0x12);
	write_cmos_sensor(0x23,0xb0);
	write_cmos_sensor(0x28,0x5f);

	write_cmos_sensor(0x29,0xd4);
	write_cmos_sensor(0x2f,0x4c);
	write_cmos_sensor(0x30,0xf8);
	write_cmos_sensor(0xcd,0x9a);
	write_cmos_sensor(0xce,0xfd);
	write_cmos_sensor(0xd0,0xd2);
	write_cmos_sensor(0xd1,0xa8);
	write_cmos_sensor(0xd3,0x35);
	write_cmos_sensor(0xd8,0x20);
	write_cmos_sensor(0xda,0x03);
	write_cmos_sensor(0xdb,0x4e);
	write_cmos_sensor(0xdc,0xb3);
	write_cmos_sensor(0xde,0x40);
	write_cmos_sensor(0xe1,0x1a);
	write_cmos_sensor(0xe2,0x00);
	write_cmos_sensor(0xe3,0x71);
	write_cmos_sensor(0xe4,0x78);
	write_cmos_sensor(0xe5,0x44);
	write_cmos_sensor(0xe6,0xdf);
	write_cmos_sensor(0xe8,0x02);
	write_cmos_sensor(0xe9,0x01);
	write_cmos_sensor(0xea,0x01);
	write_cmos_sensor(0xeb,0x02);
	write_cmos_sensor(0xec,0x02);
	write_cmos_sensor(0xed,0x01);
	write_cmos_sensor(0xee,0x01);
	write_cmos_sensor(0xef,0x02);

	/*ISP*/
	write_cmos_sensor(0x80,0x50);
	write_cmos_sensor(0x88,0x03);
	write_cmos_sensor(0x89,0x03);
	//write_cmos_sensor(0x8d,0x03);
	
	/*Crop Window*/
	write_cmos_sensor(0x90,0x01);
	write_cmos_sensor(0x92, CapStartY); //crop y
	write_cmos_sensor(0x94, CapStartX); //crop x
	write_cmos_sensor(0x95,0x09);
	write_cmos_sensor(0x96,0x90);
	write_cmos_sensor(0x97,0x0c);
	write_cmos_sensor(0x98,0xc0);

	/*Gain*/
	write_cmos_sensor(0xb0,0x70);
	write_cmos_sensor(0xb1,0x01);
	write_cmos_sensor(0xb2,0x00);
	write_cmos_sensor(0xb6,0x00);

	write_cmos_sensor(0xfe,0x01);
	write_cmos_sensor(0x50,0x00);
	write_cmos_sensor(0x51,0x08);
	write_cmos_sensor(0x52,0x10);
	write_cmos_sensor(0x53,0x18);
	write_cmos_sensor(0x54,0x19);
	write_cmos_sensor(0x55,0x1a);
	write_cmos_sensor(0x56,0x1b);
	write_cmos_sensor(0x57,0x1c);
	write_cmos_sensor(0x58,0x3c);
	write_cmos_sensor(0x59,0x5c);
	write_cmos_sensor(0x5a,0x7c);
	
	write_cmos_sensor(0xfe,0x00);

	/*BLK*/
	write_cmos_sensor(0x40,0x22);
	write_cmos_sensor(0x41,0x20);
	write_cmos_sensor(0x42,0x10);
	write_cmos_sensor(0x4e,0x3c);
	write_cmos_sensor(0x4f,0x00);
	write_cmos_sensor(0x60,0x00);
	write_cmos_sensor(0x61,0x80);
	write_cmos_sensor(0x66,0x3c);
	write_cmos_sensor(0x69,0x03);
	write_cmos_sensor(0x6c,0x00);
	write_cmos_sensor(0x6d,0x0f);

	/*Dark Offset*/
	write_cmos_sensor(0x35,0x30);
	write_cmos_sensor(0x36,0x00);

	/*Dark Sun*/
	write_cmos_sensor(0x37,0x70);
	write_cmos_sensor(0x38,0x80);
	write_cmos_sensor(0x3b,0xf0);
	write_cmos_sensor(0x3d,0x00);
	
	/*DD*/
	write_cmos_sensor(0xfe,0x01);
	write_cmos_sensor(0xc2,0x03);
	write_cmos_sensor(0xc3,0x00);
	write_cmos_sensor(0xc4,0xd8);
	write_cmos_sensor(0xc5,0x00);

	/*MIPI*/
#ifdef GC8024MIPI_4LANE	
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0xd0);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf0);
	write_cmos_sensor(0x13,0x0f);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x19,0x30);
	write_cmos_sensor(0x1a,0x03);
	write_cmos_sensor(0x21,0x04);
	write_cmos_sensor(0x22,0x02);
	write_cmos_sensor(0x23,0x10);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x14);
	write_cmos_sensor(0x26,0x04);
	write_cmos_sensor(0x29,0x02);
	write_cmos_sensor(0x2a,0x0a);
	write_cmos_sensor(0x2b,0x05);
	write_cmos_sensor(0xfe,0x00);
#else
	write_cmos_sensor(0xfe,0x03);
	write_cmos_sensor(0x01,0x07);
	write_cmos_sensor(0x02,0x34);
	write_cmos_sensor(0x03,0x13);
	write_cmos_sensor(0x04,0xf0);
	write_cmos_sensor(0x06,0x80);
	write_cmos_sensor(0x10,0x91);
	write_cmos_sensor(0x11,0x2b);
	write_cmos_sensor(0x12,0xf0); 
	write_cmos_sensor(0x13,0x0f);
	write_cmos_sensor(0x15,0x00);
	write_cmos_sensor(0x16,0x09);
	write_cmos_sensor(0x18,0x01);
	write_cmos_sensor(0x19,0x00);
	write_cmos_sensor(0x1a,0x00);
	write_cmos_sensor(0x21,0x10);
	write_cmos_sensor(0x22,0x05);
	write_cmos_sensor(0x23,0x30);
	write_cmos_sensor(0x24,0x02);
	write_cmos_sensor(0x25,0x15);	
	write_cmos_sensor(0x26,0x08);
	write_cmos_sensor(0x29,0x06);
	write_cmos_sensor(0x2a,0x04);
	write_cmos_sensor(0x2b,0x08);	
	write_cmos_sensor(0xfe,0x00);
#endif	
	TxLowTh = 300;
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);	
	preview_setting();	
}

static void hs_video_setting(void)
{
   	LOG_INF("E\n");
	preview_setting();	
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();	
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("enable: %d\n", enable);

    if (enable) {
        write_cmos_sensor(0xfe, 0x00);
        write_cmos_sensor(0x8c, 0x11);		
    } else {
        write_cmos_sensor(0xfe, 0x00);
        write_cmos_sensor(0x8c, 0x10);		
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
extern char front_cam_name[64];
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
                memset(front_cam_name, 0x00, sizeof(front_cam_name));
                memcpy(front_cam_name, "1_gc8024", 64); 
                pr_err("gc8024mipi_Sensor.c[%s](%d)    match  ok    i2c write id: 0x%x,      read sensor id: 0x%x    need id: 0x%x \n", 
                __FUNCTION__,__LINE__, imgsensor.i2c_write_id,  *sensor_id, imgsensor_info.sensor_id);
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

#ifdef GC8024_MIPI_USE_OTP
	gc8024_gcore_identify_otp();
#endif

    /* initail sequence write in  */
    sensor_init();

#ifdef GC8024_MIPI_USE_OTP
	gc8024_gcore_update_otp();
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
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    spin_unlock(&imgsensor_drv_lock);
    GC8024DuringTestPattern = KAL_FALSE; 

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
	PreorCap = 0;
    preview_setting();
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
	PreorCap = 1;
    capture_setting(imgsensor.current_fps);
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
	PreorCap = 0;
    normal_video_setting(imgsensor.current_fps);	
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
	PreorCap = 0;
    hs_video_setting();	
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
	PreorCap = 0;
    slim_video_setting();
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
			LOG_INF("adb_i2c_read 0x%x = 0x%x\n",sensor_reg_data->RegAddr,sensor_reg_data->RegData);//travis add		
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

UINT32 GC8024_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    GC8024MIPI_RAW_SensorInit    */

