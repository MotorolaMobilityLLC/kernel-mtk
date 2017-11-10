/*******************************Modified by paul 2016-2-3**********************************************
 *
 * Filename:
 * ---------
 *	 hi553mipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6735
 *	20150820: mirror flip not setting,so preview fuction can can use it ,to resolv it.
  *	20150914: 送测完了fae 要求修改setting，重新合setting.
  -------------------------------
  @DateTime:    20151021163817
  modify the winsize info,last time foroget to modity it.
 * Description:
 -------------------------------
 @DateTime:    20150925173104 move to mt6735
 modify winsizeinfo,add pip setting ,add high speed video setting-------------------------------
 @DateTime:    20151119155951
 add group hold fuction at shutter gain setting, for ae peak
 * ------------
 *	 Source code of Sensor driver
 *
 *	PengtaoFan

 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/kernel.h>
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

#include "hi553mipiraw_Sensor.h"
#include <linux/hardware_info.h> //sunhushan


/*===FEATURE SWITH===*/
 // #define FPTPDAFSUPPORT   //for pdaf switch
 // #define FANPENGTAO   //for debug log
 #define LOG_INF LOG_INF_NEW
 //#define NONCONTINUEMODE
/*===FEATURE SWITH===*/

/****************************Modify Following Strings for Debug****************************/
#define PFX "hi553"
#define LOG_INF_NEW(format, args...)    pr_err(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF_LOD(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)
#define LOG_1 LOG_INF("hi553,MIPI 2LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/
#if 0
extern int get_device_info(char* buf);
extern int get_camera_info(char* buf);
#endif
static int hi553_info_limit = 0;
static char hi553_factory_module_id[50] = {0};

#define HI_553_MIPI_USE_OTP

static DEFINE_SPINLOCK(imgsensor_drv_lock);
#define Hi553_I2C_BURST // 20151228 paul add 

static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = HI553_SENSOR_ID,		//Sensor ID Value: 0x30C8//record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0x4f06178b,   //checksum value for Camera Auto Test
	//bug 252202 - yewei.wt modify 2017.03.21 modify ATA test color pattern checksum_value
	.pre = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,				//record different mode's linelength/************2816/176000000*1000000000=16000*************************/
		.framelength = 2083,			//record different mode's framelength    /*  176000000/2816/2049=30.5fps  */
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 960,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//5808, 			//record different mode's linelength/************2816/176000000*1000000000=16000*************************/
		.framelength = 2083,			//record different mode's framelength  /*  176000000/2816/2049=30.5fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2560,		//record different mode's width of grabwindow
		.grabwindow_height = 1920,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
		
	.cap1 = {							//capture for PIP 15ps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//,				//record different mode's linelength/************2816/176000000*1000000000=16000*************************/
		.framelength = 4166,			//record different mode's framelength      /*  176000000/2816/2049=30.5fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2560,		//record different mode's width of grabwindow
		.grabwindow_height = 1920,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 150,	
	},
	.normal_video = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//5808,				//record different mode's linelength/************2816/176000000*1000000000=16000*************************/
		.framelength = 2083,			//record different mode's framelength    /*  176000000/2816/2049=30.5fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 960,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
	.hs_video = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,				//record different mode's linelength/************2816/176000000*1000000000=16000*************************/
		.framelength = 520,			//record different mode's framelength    /*  176000000/2816/520=120.2fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 640,		//record different mode's width of grabwindow
		.grabwindow_height = 480,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 19,
		.max_framerate = 1200,	
	},
	.slim_video = {
		.pclk = 176000000,				//record different mode's pclk/************2816/176000000*1000000000=16000*************************/
		.linelength  = 2816,				//record different mode's linelength
		.framelength = 2083,			//record different mode's framelength    /*  176000000/2816/2049=30.5fps  */
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 960,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
  .custom1 = {
		.pclk = 176000000,				//record different mode's pclk/************4592/176000000*1000000000=26091*************************/
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength    /*  176000000/2816/2049=12.0fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,	
	},
  .custom2 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength/************4592/176000000*1000000000=26091*************************/
		.framelength =3188, //3168,			//record different mode's framelength   /*  176000000/2816/2049=12.0fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,	
	},
  .custom3 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength/************4592/176000000*1000000000=26091*************************/
		.framelength =3188, //3168,			//record different mode's framelength      /*  176000000/2816/2049=12.0fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,	
	},
  .custom4 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength/************4592/176000000*1000000000=26091*************************/
		.framelength =3188, //3168,			//record different mode's framelength      /*  176000000/2816/2049=12.0fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,	
	},
  .custom5 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength/************4592/176000000*1000000000=26091*************************/
		.framelength =3188, //3168,			//record different mode's framelength    /*  176000000/2816/2049=12.0fps  */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,	
	},

	.margin = 6,			//sensor framelength & shutter margin
	.min_shutter = 6,		//min shutter
	.max_frame_length = 0x7FFF,//REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation
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
    .custom1_delay_frame = 2,
    .custom2_delay_frame = 2, 
    .custom3_delay_frame = 2, 
    .custom4_delay_frame = 2, 
    .custom5_delay_frame = 2,
	
	.isp_driving_current = ISP_DRIVING_4MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
	.i2c_addr_table = {0x40, /*0x20,*/0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 400, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0,//record current sensor's i2c write id
};


/* Sensor output window information*/

static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{
 //{ 2592, 1944,	  0,  	0, 2592, 1944, 1296,  972,   0,	0, 1296,  972, 	 0, 0, 1296,  972}, // Preview 
 { 2560, 1920,	  0,  	0, 2560, 1920, 1280,  960,   0,	0, 1280,  960, 	 0, 0, 1280,  960}, // Preview 
 { 2560, 1920,	  0,  	0, 2560, 1920, 2560, 1920,   0,	0, 2560, 1920, 	 0, 0, 2560, 1920}, // capture 
 { 2560, 1920,	  0,	0, 2560, 1920, 1280,  960,	 0, 0, 1280,  960,	 0, 0, 1280,  960}, // VIDEO 
 { 2560, 1920,	  0,  	0, 2560, 1920, 1280,  960,   0,	0,  640,  480, 	 0, 0,  640,  480}, // hight speed video
 { 2560, 1920,	  0,  	0, 2560, 1920, 1280,  960,   0,	0, 1280,  960, 	 0, 0, 1280,  960}, // slim
};

#ifdef Hi553_I2C_BURST// 20151228 paul add 
struct Hynix_Sensor_reg { 
	u16 addr;
	u16 val;
};
#define HYNIX_TABLE_END 65535
#define HYNIX_SIZEOF_I2C_BUF 254
char hi553_i2c_buf[HYNIX_SIZEOF_I2C_BUF];
static int Hi842_write_burst_mode(struct Hynix_Sensor_reg table[])
{
    int err;
	const struct Hynix_Sensor_reg *curr;
	const struct Hynix_Sensor_reg *next;
	u16 buf_count = 0;
 
	LOG_INF("start \n");

	for (curr = table; curr->addr != HYNIX_TABLE_END; curr++) 
    {
		if (!buf_count) { 
			
			hi553_i2c_buf[buf_count] = curr->addr >> 8; buf_count++;
            
			hi553_i2c_buf[buf_count] = curr->addr & 0xFF; buf_count++;

		}
        
		hi553_i2c_buf[buf_count] = curr->val >> 8; buf_count++;

		hi553_i2c_buf[buf_count] = curr->val & 0xFF; buf_count++;

		next = curr + 1;
		if (next->addr == curr->addr + 2 &&
			buf_count < HYNIX_SIZEOF_I2C_BUF &&
			next->addr != HYNIX_TABLE_END)
        {  
            continue;
        }
        kdSetI2CSpeed(400);// I2C Set . 400KHz Speed 
        err = iBurstWriteReg(hi553_i2c_buf , buf_count, imgsensor.i2c_write_id);
        
		if (err)
		{	
			LOG_INF("err \n");

		    return err;
        }

		buf_count = 0;
	}

	LOG_INF("end \n");

	return 0;
}
#endif
#if 1
static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}
static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}
#endif

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    return ((get_byte<<8)&0xFF00)|((get_byte>>8)&0x00FF);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}

#ifdef HI_553_MIPI_USE_OTP

#define ADDR_FLAG_SEG1 0x501
#define ADDR_FLAG_SEG2 0x535

typedef struct otp_hi553_info{
	kal_uint16 module_id;
	kal_uint16 year;
	kal_uint16 month;
	kal_uint16 day;
	kal_uint16 lens_id;
	kal_uint16 vcm_id;
	kal_uint16 checksum_info;
}hi553_otp_info;

typedef struct otp_hi553_data{
	int R_Gr_Ratio;
	int B_Gb_Ratio;
	int Gb_Gr_Ratio;
	int checksun_data;
}hi553_otp_data;

int check_otp_info(void)
{
	unsigned int flag;
	int group;

/*************enter otp w/r mode **********************/
		write_cmos_sensor_byte(0x0a02,0x01);
		write_cmos_sensor_byte(0x0a00,0x00);
		mdelay(100);
		write_cmos_sensor_byte(0x0f02,0x00);
		write_cmos_sensor_byte(0x011a,0x01);
		write_cmos_sensor_byte(0x011b,0x09);
		write_cmos_sensor_byte(0x0d04,0x01);
		write_cmos_sensor_byte(0x0d00,0x07);
		write_cmos_sensor_byte(0x003f,0x10);
		write_cmos_sensor_byte(0x0a00,0x01);
/*************enter otp w/r mode **********************/

		write_cmos_sensor_byte(0x010a,(ADDR_FLAG_SEG1>>8)&0xff);
		write_cmos_sensor_byte(0x010b,ADDR_FLAG_SEG1&0xff);
		write_cmos_sensor_byte(0x0102,0x01);
		flag = read_cmos_sensor_byte(0x0108);
		printk("yw, seg1 0x501 flag:%x\n",flag);

		if(0x37 == flag) 
			group = 3;
		else if(0x13 == flag) 
			group = 2;
		else if(0x01 == flag) 
			group = 1;
		else
		{
			group = 0;
		}
		return group;
}

// read otp info of seg1, seg1 addr form 0x502-0x512(group1), 0x513-0x51A(group2),0x524-0x534(group3)
static int read_otp_info(int group, hi553_otp_info *otp_info)
{
	unsigned int start_addr_info=0;
	unsigned int end_addr_info,checksum_addr_info;
	int i,temp=0;
	int data[17]={ 0 };
	if(group == 1)
	{
		start_addr_info = 0x502;	//Group1
        end_addr_info = 0x511;
        checksum_addr_info = 0x512;
	}
	else if(group == 2)
	{
		start_addr_info = 0x513;	//Group2
        end_addr_info = 0x522;
        checksum_addr_info = 0x523;
	}
	else if(group == 3)
	{
		start_addr_info = 0x524;	//Group3
        end_addr_info = 0x533;
        checksum_addr_info = 0x534;
	}

	/*  read out the group data form sensor seg1  */
	for(i = 0; i <= checksum_addr_info-start_addr_info;i++)
	{
		write_cmos_sensor_byte(0x010a,(((start_addr_info+i)>>8)&0xff));
		write_cmos_sensor_byte(0x010b,((start_addr_info+i)&0xff));
		write_cmos_sensor_byte(0x0102,0x01);
		data[i] = read_cmos_sensor_byte(0x0108);
		printk("yw,seg1 data[%d]:%x\n",i,data[i]);
	}

		(*otp_info).module_id =  data[0];
		(*otp_info).year =  data[2];
		(*otp_info).month =  data[3];
		(*otp_info).day =  data[4];
		(*otp_info).lens_id =  data[6];
		(*otp_info).vcm_id =  data[7];

		(*otp_info).checksum_info =  data[16];
		
		//  checksum the data 
		for(i = 0; i <= end_addr_info - start_addr_info; i++)
			{
				temp += data[i];
			}
		if((*otp_info).checksum_info == (temp%0xFF + 1))
			{
				printk("yw,checksum_info otp_info success\n");
			}
		else
			{
				printk("yw,checksum_info otp_info fail,checksun_info %x,temp %x\n",(*otp_info).checksum_info,((temp%0xFF) + 1));
				return -1;			
			}
		return 1;
}

static void update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	// Digital_gain_Gr
	write_cmos_sensor_byte(0x0126,(G_gain>>8));
	write_cmos_sensor_byte(0x0127,(G_gain>>0)&0xff);
	// Digital_gain_Gb
	write_cmos_sensor_byte(0x0128,(G_gain>>8));
	write_cmos_sensor_byte(0x0129,(G_gain>>0)&0xff);
	//Digital_gain_R
	write_cmos_sensor_byte(0x012a,(R_gain>>8));
	write_cmos_sensor_byte(0x012b,(R_gain>>0)&0xff);
	//Digital_gain_B
	write_cmos_sensor_byte(0x012c,(B_gain>>8));
	write_cmos_sensor_byte(0x012d,(B_gain>>0)&0xff);
}

static int read_otp_awb(unsigned int group,hi553_otp_data* otp_data)
{
	unsigned int start_addr=0x0,end_addr=0x0,checksum_addr=0x0;
	int i,temp=0;
	int data[30]={0};
	if(group == 1)
	{
		start_addr = 0x536;	//Group1
        end_addr = 0x552;
        checksum_addr = 0x553;
	}
	else if(group == 2)
	{
		start_addr = 0x554;	//Group2
        end_addr = 0x570;
        checksum_addr = 0x571;
	}
	else if(group == 3)
	{
		start_addr = 0x572;	//Group3
        end_addr = 0x58E;
        checksum_addr = 0x58F;
	}
	printk("yw,read_otp_awb,group:%d,start_addr %x,end_addr %x,checksum_addr %x\n",group,start_addr,end_addr,checksum_addr);

		/*  read out the group data form sensor seg2  */
		for(i = 0; i <= checksum_addr -start_addr; i++)
		{
			write_cmos_sensor_byte(0x010a,((start_addr+i)>>8)&0xff);
			write_cmos_sensor_byte(0x010b,(start_addr+i)&0xff);
			write_cmos_sensor_byte(0x0102,0x01);
			data[i] = read_cmos_sensor_byte(0x0108);
			printk("yw,seg2 data[%d]:%x\n",i,data[i]);
		}
		
		otp_data->R_Gr_Ratio =  ( (data[0] << 8 )  | (data[1] & 0x03ff));
		otp_data->B_Gb_Ratio =  ( (data[2] << 8 )  | (data[3] & 0x03ff));
		otp_data->Gb_Gr_Ratio =  ( (data[4] << 8 ) | (data[5] & 0x03ff));
		otp_data->checksun_data = data[29];

/*************exit otp w/r mode **********************/
		write_cmos_sensor_byte(0x0a00,0x00);
		mdelay(100);
		write_cmos_sensor_byte(0x003f,0x00);
		write_cmos_sensor_byte(0x0a00,0x01);
/*************exit otp w/r mode **********************/

		printk("yw,otp_data: otp_data.R_Gr_Ratio:%x,otp_data.B_Gb_Ratio:%x,otp_data.Gb_Gr_Ratio:%x,checksum:%x\n",otp_data->R_Gr_Ratio,otp_data->B_Gb_Ratio,otp_data->Gb_Gr_Ratio,data[29]);

		// checksum the data
		for(i=0;i<=end_addr-start_addr;i++)
			{
				temp += data[i];
			}
		if(otp_data->checksun_data == ((temp%0xFF) + 1))
			{
				printk("yw,checksum_data otp_awb success\n");
			}
		else{
			printk("yw,checksum_data otp_awb fail,checksun_data %x,temp %x\n",otp_data->checksun_data,((temp%0xFF) + 1));
			return -1;			
			}
	return 0;
}

// check otp flag ,seg2 addr 0x535 is 0x01(group1),0x13(group2),0x37(group3)
int check_otp_awb(void)
{
	unsigned int flag = 0;
	int group;

	write_cmos_sensor_byte(0x010a,(ADDR_FLAG_SEG2>>8)&0xff);
	write_cmos_sensor_byte(0x010b,ADDR_FLAG_SEG2&0xff);
	write_cmos_sensor_byte(0x0102,0x01);
	flag = read_cmos_sensor_byte(0x0108);
	printk("yw, seg2 0x535 flag:%x\n",flag);

	if(0x37 == flag) 
		group = 3;
	else if(0x13 == flag) 
		group = 2;
	else if(0x01 == flag) 
		group = 1;
	else
	{
		group = 0;
	}
	return group;
}

static int hi553_gcore_update_awb(void)
{
	int group=0;
	unsigned int i;
	int ret;
	int nR_gain = 1,nG_gain = 1,nB_gain = 1;
	int R_gain,B_gain,G_gain;
	int RG_golden = 322;//= 0.5 * 512 + 0.5;
	int BG_golden = 304;//= 0.6 * 512 + 0.5;
	hi553_otp_data otp_data;

	for(i=1;i<=3;i++)
	{
		group = check_otp_awb();
		if(group)
		{
			printk("yw,check_otp_awb,i:%d,group:%d\n",i,group);
			break;
		}
		printk("yw,check_otp_awb,i %d, fail, again \n",i);
	}
	if(i>3 || group == 0)
	{
		printk("yw,[hi553-OTP]no valid OTP awb data\n");
		return -1;
	}
	ret = read_otp_awb(group,&otp_data);
	if(ret){
		printk("hi553 read_otp_awb faid\n");
		return -1;	
	}

	nR_gain = otp_data.R_Gr_Ratio;
	nB_gain = otp_data.B_Gb_Ratio;
	nG_gain = otp_data.Gb_Gr_Ratio;

	printk("yw,hi553_gcore_update_awb-> nR_gain:%x,nB_gain:%x,nG_gain;%x\n",nR_gain,nB_gain,nG_gain);
	/*** SK-hynix  check awb data ***/
	R_gain = (0x100 * 1000 * RG_golden / nR_gain / 1000);
	B_gain = (0x100 * 1000 * BG_golden / nB_gain / 1000);
    G_gain = 0x100;

	if (R_gain < B_gain) {
		if(R_gain < 0x100) {
			B_gain =0x100 *  B_gain / R_gain;
			G_gain =0x100 *  G_gain / R_gain;
			R_gain = 0x100;
		}
	} else {
		if (B_gain < 0x100) {
			R_gain = 0x100 * R_gain / B_gain;
			G_gain = 0x100 * G_gain / B_gain;
			B_gain = 0x100;
		}
	}
	printk("yw,hi553_gcore_update_awb-> R_gain:%x,B_gain:%x,G_gain;%x\n",R_gain,B_gain,G_gain);
	update_awb_gain(R_gain, G_gain, B_gain);
	return 0;
}

static void hi553_gcore_update_otp(void)
{
	int ret;
	ret = hi553_gcore_update_awb();
	if(ret)
	{
		printk("yw,hi553_gcore_update_awb fail\n");
	}
}

// read out the otp info module id  from seg1 addr 0x502(group1),0x513(group2),0x524(group3)
static int otp_module_id(void)
{
    hi553_otp_info otp_info;
    int i,ret,group=0;
    for(i=1;i<=3;i++)
    {
		group = check_otp_info();
        if(group)
		{
			printk("yw,otp_module_id,i:%d,group:%d\n",i,group);
			break;
		}
		printk("yw,check_otp_info,i %d, fail, again \n",i);
    }
    if (i>3 || group == 0)
    {
        LOG_INF("yw,[hi553-OTP]no valid module_id OTP data\n");
        return -1;
    }
	ret = read_otp_info(group, &otp_info);
	if(!ret)
	{
		printk("yw,otp_module_id, read_otp_info fail\n");
		return -1;
	}
    return otp_info.module_id;
	
}
#endif

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);	  
	write_cmos_sensor(0x0008, imgsensor.line_length & 0xFFFF);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

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

#if 0
static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	   
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
		write_cmos_sensor(0X0006, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0x0004, (shutter) & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

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
		write_cmos_sensor(0x0046, 0x0100); //group para hold on
		write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		write_cmos_sensor(0x0046, 0x0100); //group para hold on
		// Extend frame length
		write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0X0004, shutter & 0xFFFF);
	write_cmos_sensor(0x0046, 0x0000); //group para hold off
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	//gain = 64 = 1x real gain.
	reg_gain = gain / 4 - 16;
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
	write_cmos_sensor(0x0046, 0x0100); //group para hold o
	write_cmos_sensor(0x003A, (reg_gain&0xFFFF)); 
	write_cmos_sensor(0x0046, 0x0000); //group para hold off   
	return gain;
}	/*	set_gain  */

//[TODO]ihdr_write_shutter_gain not support for hi553
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


//[TODO]
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
			write_cmos_sensor(0x0000,0X0000); //GR
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0000,0X0100); //R
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0000,0X0200); //B	
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0000,0X0300); //GB
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
#if 0
static void sensor_init(void)
{
	LOG_INF("E\n");
//Sensor Information////////////////////////////
//Sensor	  : Hi-553
//Date		  : 2015-09-05
//Customer        : LGE
//Image size	  : 2592x1944
//MCLK		  : 24MHz
//MIPI speed(Mbps): 880Mbps x 2Lane
//Frame Length	  : 2049
//Line Length 	  : 2816
//Max Fps	  : 30.5fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offset	  : 64code
////////////////////////////////////////////////
write_cmos_sensor(0x0a00, 0x0000); //stream off
	write_cmos_sensor(0x0e00, 0x0102);
	write_cmos_sensor(0x0e02, 0x0102);
	write_cmos_sensor(0x0e0c, 0x0100);
	write_cmos_sensor(0x2000, 0x7400);
	write_cmos_sensor(0x2002, 0x1303);
	write_cmos_sensor(0x2004, 0x7006);
	write_cmos_sensor(0x2006, 0x1303);
	write_cmos_sensor(0x2008, 0x0bd2);
	write_cmos_sensor(0x200a, 0x5001);
	write_cmos_sensor(0x200c, 0x7000);
	write_cmos_sensor(0x200e, 0x16c6);
	write_cmos_sensor(0x2010, 0x0044);
	write_cmos_sensor(0x2012, 0x0307);
	write_cmos_sensor(0x2014, 0x0008);
	write_cmos_sensor(0x2016, 0x00c4);
	write_cmos_sensor(0x2018, 0x0009);
	write_cmos_sensor(0x201a, 0x207e);
	write_cmos_sensor(0x201c, 0x7001);
	write_cmos_sensor(0x201e, 0x0fd0);
	write_cmos_sensor(0x2020, 0x00d1);
	write_cmos_sensor(0x2022, 0x00a1);
	write_cmos_sensor(0x2024, 0x00c9);
	write_cmos_sensor(0x2026, 0x030b);
	write_cmos_sensor(0x2028, 0x7008);
	write_cmos_sensor(0x202a, 0x81c0);
	write_cmos_sensor(0x202c, 0x01d7);
	write_cmos_sensor(0x202e, 0x0022);
	write_cmos_sensor(0x2030, 0x0016);
	write_cmos_sensor(0x2032, 0x7006);
	write_cmos_sensor(0x2034, 0x0fe2);
	write_cmos_sensor(0x2036, 0x0016);
	write_cmos_sensor(0x2038, 0x20d0);
	write_cmos_sensor(0x203a, 0x15c8);
	write_cmos_sensor(0x203c, 0x0007);
	write_cmos_sensor(0x203e, 0x0000);
	write_cmos_sensor(0x2040, 0x0540);
	write_cmos_sensor(0x2042, 0x0007);
	write_cmos_sensor(0x2044, 0x0008);
	write_cmos_sensor(0x2046, 0x0ec5);
	write_cmos_sensor(0x2048, 0x00cc);
	write_cmos_sensor(0x204a, 0x00c5);
	write_cmos_sensor(0x204c, 0x00cc);
	write_cmos_sensor(0x204e, 0x04d7);
	write_cmos_sensor(0x2050, 0x18e2);
	write_cmos_sensor(0x2052, 0x0016);
	write_cmos_sensor(0x2054, 0x70e1);
	write_cmos_sensor(0x2056, 0x18e2);
	write_cmos_sensor(0x2058, 0x16d6);
	write_cmos_sensor(0x205a, 0x0022);
	write_cmos_sensor(0x205c, 0x8251);
	write_cmos_sensor(0x205e, 0x2128);
	write_cmos_sensor(0x2060, 0x2100);
	write_cmos_sensor(0x2062, 0x702f);
	write_cmos_sensor(0x2064, 0x2f06);
	write_cmos_sensor(0x2066, 0x2300);
	write_cmos_sensor(0x2068, 0x7800);
	write_cmos_sensor(0x206a, 0x7400);
	write_cmos_sensor(0x206c, 0x1303);
	write_cmos_sensor(0x206e, 0x7006);
	write_cmos_sensor(0x2070, 0x1303);
	write_cmos_sensor(0x2072, 0x0bd2);
	write_cmos_sensor(0x2074, 0x5061);
	write_cmos_sensor(0x2076, 0x7000);
	write_cmos_sensor(0x2078, 0x16c6);
	write_cmos_sensor(0x207a, 0x0044);
	write_cmos_sensor(0x207c, 0x0307);
	write_cmos_sensor(0x207e, 0x0008);
	write_cmos_sensor(0x2080, 0x00c4);
	write_cmos_sensor(0x2082, 0x0009);
	write_cmos_sensor(0x2084, 0x207e);
	write_cmos_sensor(0x2086, 0x7001);
	write_cmos_sensor(0x2088, 0x0fd0);
	write_cmos_sensor(0x208a, 0x00d1);
	write_cmos_sensor(0x208c, 0x00a1);
	write_cmos_sensor(0x208e, 0x00c9);
	write_cmos_sensor(0x2090, 0x030b);
	write_cmos_sensor(0x2092, 0x7008);
	write_cmos_sensor(0x2094, 0x84c0);
	write_cmos_sensor(0x2096, 0x01d7);
	write_cmos_sensor(0x2098, 0x0022);
	write_cmos_sensor(0x209a, 0x0016);
	write_cmos_sensor(0x209c, 0x7006);
	write_cmos_sensor(0x209e, 0x0fe2);
	write_cmos_sensor(0x20a0, 0x0016);
	write_cmos_sensor(0x20a2, 0x20d0);
	write_cmos_sensor(0x20a4, 0x15c8);
	write_cmos_sensor(0x20a6, 0x0007);
	write_cmos_sensor(0x20a8, 0x0000);
	write_cmos_sensor(0x20aa, 0x0540);
	write_cmos_sensor(0x20ac, 0x0007);
	write_cmos_sensor(0x20ae, 0x0008);
	write_cmos_sensor(0x20b0, 0x0ec5);
	write_cmos_sensor(0x20b2, 0x00cc);
	write_cmos_sensor(0x20b4, 0x00c5);
	write_cmos_sensor(0x20b6, 0x00cc);
	write_cmos_sensor(0x20b8, 0x04d7);
	write_cmos_sensor(0x20ba, 0x18e2);
	write_cmos_sensor(0x20bc, 0x0016);
	write_cmos_sensor(0x20be, 0x70e1);
	write_cmos_sensor(0x20c0, 0x18e2);
	write_cmos_sensor(0x20c2, 0x16d6);
	write_cmos_sensor(0x20c4, 0x0022);
	write_cmos_sensor(0x20c6, 0x8551);
	write_cmos_sensor(0x20c8, 0x2128);
	write_cmos_sensor(0x20ca, 0x2100);
	write_cmos_sensor(0x20cc, 0x2006);
	write_cmos_sensor(0x20ce, 0x2300);
	write_cmos_sensor(0x20d0, 0x7800);
	write_cmos_sensor(0x20d2, 0x7400);
	write_cmos_sensor(0x20d4, 0x0005);
	write_cmos_sensor(0x20d6, 0x7002);
	write_cmos_sensor(0x20d8, 0x1303);
	write_cmos_sensor(0x20da, 0x00c5);
	write_cmos_sensor(0x20dc, 0x7002);
	write_cmos_sensor(0x20de, 0x1303);
	write_cmos_sensor(0x20e0, 0x0bd2);
	write_cmos_sensor(0x20e2, 0x50c1);
	write_cmos_sensor(0x20e4, 0x7000);
	write_cmos_sensor(0x20e6, 0x16c6);
	write_cmos_sensor(0x20e8, 0x0044);
	write_cmos_sensor(0x20ea, 0x0307);
	write_cmos_sensor(0x20ec, 0x0008);
	write_cmos_sensor(0x20ee, 0x00c4);
	write_cmos_sensor(0x20f0, 0x0009);
	write_cmos_sensor(0x20f2, 0x207e);
	write_cmos_sensor(0x20f4, 0x7001);
	write_cmos_sensor(0x20f6, 0x0fd0);
	write_cmos_sensor(0x20f8, 0x00d1);
	write_cmos_sensor(0x20fa, 0x00a1);
	write_cmos_sensor(0x20fc, 0x00c9);
	write_cmos_sensor(0x20fe, 0x030b);
	write_cmos_sensor(0x2100, 0x7008);
	write_cmos_sensor(0x2102, 0x87c0);
	write_cmos_sensor(0x2104, 0x01d7);
	write_cmos_sensor(0x2106, 0x0022);
	write_cmos_sensor(0x2108, 0x0016);
	write_cmos_sensor(0x210a, 0x08a2);
	write_cmos_sensor(0x210c, 0x0016);
	write_cmos_sensor(0x210e, 0x20d0);
	write_cmos_sensor(0x2110, 0x15c8);
	write_cmos_sensor(0x2112, 0x0007);
	write_cmos_sensor(0x2114, 0x0000);
	write_cmos_sensor(0x2116, 0x0540);
	write_cmos_sensor(0x2118, 0x0007);
	write_cmos_sensor(0x211a, 0x0008);
	write_cmos_sensor(0x211c, 0x0d45);
	write_cmos_sensor(0x211e, 0x00cc);
	write_cmos_sensor(0x2120, 0x00c5);
	write_cmos_sensor(0x2122, 0x00cc);
	write_cmos_sensor(0x2124, 0x04d7);
	write_cmos_sensor(0x2126, 0x18e2);
	write_cmos_sensor(0x2128, 0x0016);
	write_cmos_sensor(0x212a, 0x706f);
	write_cmos_sensor(0x212c, 0x18e2);
	write_cmos_sensor(0x212e, 0x16d6);
	write_cmos_sensor(0x2130, 0x0022);
	write_cmos_sensor(0x2132, 0x8857);
	write_cmos_sensor(0x2134, 0x2128);
	write_cmos_sensor(0x2136, 0x2100);
	write_cmos_sensor(0x2138, 0x7800);
	write_cmos_sensor(0x213a, 0x3100);
	write_cmos_sensor(0x213c, 0x01c6);
	write_cmos_sensor(0x213e, 0x01c4);
	write_cmos_sensor(0x2140, 0x01c0);
	write_cmos_sensor(0x2142, 0x01c6);
	write_cmos_sensor(0x2144, 0x2700);
	write_cmos_sensor(0x2146, 0x7007);
	write_cmos_sensor(0x2148, 0x3f00);
	write_cmos_sensor(0x214a, 0x7800);
	write_cmos_sensor(0x214c, 0x4031);
	write_cmos_sensor(0x214e, 0x83ce);
write_cmos_sensor(0x2150, 0x4347);
	write_cmos_sensor(0x2152, 0x40f2);
	write_cmos_sensor(0x2154, 0x0006);
	write_cmos_sensor(0x2156, 0x81fe);
	write_cmos_sensor(0x2158, 0x40f2);
	write_cmos_sensor(0x215a, 0x0042);
	write_cmos_sensor(0x215c, 0x81ff);
	write_cmos_sensor(0x215e, 0x40f2);
	write_cmos_sensor(0x2160, 0xff84);
	write_cmos_sensor(0x2162, 0x8200);
	write_cmos_sensor(0x2164, 0x43c2);
	write_cmos_sensor(0x2166, 0x8201);
	write_cmos_sensor(0x2168, 0x40b2);
	write_cmos_sensor(0x216a, 0x0230);
	write_cmos_sensor(0x216c, 0x8030);
	write_cmos_sensor(0x216e, 0x40b2);
	write_cmos_sensor(0x2170, 0x0032);
	write_cmos_sensor(0x2172, 0x8032);
	write_cmos_sensor(0x2174, 0x40b2);
	write_cmos_sensor(0x2176, 0x01e0);
	write_cmos_sensor(0x2178, 0x81d2);
	write_cmos_sensor(0x217a, 0x40b2);
	write_cmos_sensor(0x217c, 0x03c0);
	write_cmos_sensor(0x217e, 0x81d4);
	write_cmos_sensor(0x2180, 0x40b2);
	write_cmos_sensor(0x2182, 0x05a0);
	write_cmos_sensor(0x2184, 0x81d6);
	write_cmos_sensor(0x2186, 0x40b2);
	write_cmos_sensor(0x2188, 0x0281);
	write_cmos_sensor(0x218a, 0x81d8);
	write_cmos_sensor(0x218c, 0x40b2);
	write_cmos_sensor(0x218e, 0x02c0);
	write_cmos_sensor(0x2190, 0x81da);
	write_cmos_sensor(0x2192, 0x40b2);
	write_cmos_sensor(0x2194, 0x0064);
	write_cmos_sensor(0x2196, 0x81e6);
	write_cmos_sensor(0x2198, 0x93d2);
	write_cmos_sensor(0x219a, 0x003d);
	write_cmos_sensor(0x219c, 0x2002);
	write_cmos_sensor(0x219e, 0x4030);
write_cmos_sensor(0x21a0, 0xf956);
	write_cmos_sensor(0x21a2, 0x0900);
	write_cmos_sensor(0x21a4, 0x7312);
	write_cmos_sensor(0x21a6, 0x43d2);
	write_cmos_sensor(0x21a8, 0x003d);
	write_cmos_sensor(0x21aa, 0x40b2);
	write_cmos_sensor(0x21ac, 0x9887);
	write_cmos_sensor(0x21ae, 0x0b82);
	write_cmos_sensor(0x21b0, 0x40b2);
	write_cmos_sensor(0x21b2, 0xc540);
	write_cmos_sensor(0x21b4, 0x0b84);
	write_cmos_sensor(0x21b6, 0x40b2);
	write_cmos_sensor(0x21b8, 0xb540);
	write_cmos_sensor(0x21ba, 0x0b86);
	write_cmos_sensor(0x21bc, 0x40b2);
	write_cmos_sensor(0x21be, 0x0085);
	write_cmos_sensor(0x21c0, 0x0b88);
	write_cmos_sensor(0x21c2, 0x40b2);
	write_cmos_sensor(0x21c4, 0xd304);
	write_cmos_sensor(0x21c6, 0x0b8a);
	write_cmos_sensor(0x21c8, 0x40b2);
	write_cmos_sensor(0x21ca, 0x0420);
	write_cmos_sensor(0x21cc, 0x0b8c);
	write_cmos_sensor(0x21ce, 0x40b2);
	write_cmos_sensor(0x21d0, 0xc200);
	write_cmos_sensor(0x21d2, 0x0b8e);
	write_cmos_sensor(0x21d4, 0x4392);
	write_cmos_sensor(0x21d6, 0x0ba6);
	write_cmos_sensor(0x21d8, 0x43d2);
	write_cmos_sensor(0x21da, 0x01a0);
	write_cmos_sensor(0x21dc, 0x40f2);
	write_cmos_sensor(0x21de, 0x0003);
	write_cmos_sensor(0x21e0, 0x01a2);
	write_cmos_sensor(0x21e2, 0x43d2);
	write_cmos_sensor(0x21e4, 0x019d);
	write_cmos_sensor(0x21e6, 0x43d2);
	write_cmos_sensor(0x21e8, 0x0f82);
	write_cmos_sensor(0x21ea, 0x0cff);
	write_cmos_sensor(0x21ec, 0x0cff);
	write_cmos_sensor(0x21ee, 0x0cff);
	write_cmos_sensor(0x21f0, 0x0cff);
	write_cmos_sensor(0x21f2, 0x0cff);
	write_cmos_sensor(0x21f4, 0x0cff);
	write_cmos_sensor(0x21f6, 0x0c32);
	write_cmos_sensor(0x21f8, 0x40f2);
	write_cmos_sensor(0x21fa, 0x000e);
	write_cmos_sensor(0x21fc, 0x0f90);
	write_cmos_sensor(0x21fe, 0x4392);
	write_cmos_sensor(0x2200, 0x7326);
	write_cmos_sensor(0x2202, 0x90f2);
	write_cmos_sensor(0x2204, 0x0010);
	write_cmos_sensor(0x2206, 0x00bf);
	write_cmos_sensor(0x2208, 0x2002);
	write_cmos_sensor(0x220a, 0x4030);
write_cmos_sensor(0x220c, 0xf7aa);
	write_cmos_sensor(0x220e, 0x403b);
	write_cmos_sensor(0x2210, 0x7f10);
	write_cmos_sensor(0x2212, 0x439b);
	write_cmos_sensor(0x2214, 0x0000);
	write_cmos_sensor(0x2216, 0x403f);
	write_cmos_sensor(0x2218, 0xf13a);
	write_cmos_sensor(0x221a, 0x12b0);
	write_cmos_sensor(0x221c, 0xe12a);
	write_cmos_sensor(0x221e, 0x438b);
	write_cmos_sensor(0x2220, 0x0000);
	write_cmos_sensor(0x2222, 0x4392);
	write_cmos_sensor(0x2224, 0x81de);
	write_cmos_sensor(0x2226, 0x40b2);
	write_cmos_sensor(0x2228, 0x02bc);
	write_cmos_sensor(0x222a, 0x731e);
	write_cmos_sensor(0x222c, 0x43a2);
	write_cmos_sensor(0x222e, 0x81dc);
	write_cmos_sensor(0x2230, 0xb3e2);
	write_cmos_sensor(0x2232, 0x00b4);
	write_cmos_sensor(0x2234, 0x2402);
	write_cmos_sensor(0x2236, 0x4392);
	write_cmos_sensor(0x2238, 0x81dc);
write_cmos_sensor(0x223a, 0x4326);
	write_cmos_sensor(0x223c, 0xb3d2);
	write_cmos_sensor(0x223e, 0x00b4);
	write_cmos_sensor(0x2240, 0x2002);
	write_cmos_sensor(0x2242, 0x4030);
write_cmos_sensor(0x2244, 0xf79a);
write_cmos_sensor(0x2246, 0x4306);
	write_cmos_sensor(0x2248, 0x12b0);
	write_cmos_sensor(0x224a, 0xe16a);
	write_cmos_sensor(0x224c, 0x40b2);
	write_cmos_sensor(0x224e, 0x012c);
	write_cmos_sensor(0x2250, 0x8036);
	write_cmos_sensor(0x2252, 0x40b2);
	write_cmos_sensor(0x2254, 0x0030);
	write_cmos_sensor(0x2256, 0x8034);
	write_cmos_sensor(0x2258, 0x12b0);
	write_cmos_sensor(0x225a, 0xe238);
	write_cmos_sensor(0x225c, 0x4382);
	write_cmos_sensor(0x225e, 0x81e4);
write_cmos_sensor(0x2260, 0x43a2);
write_cmos_sensor(0x2262, 0x7320);
write_cmos_sensor(0x2264, 0x4382);
write_cmos_sensor(0x2266, 0x7322);
write_cmos_sensor(0x2268, 0x4392);
write_cmos_sensor(0x226a, 0x7326);
write_cmos_sensor(0x226c, 0x425f);
write_cmos_sensor(0x226e, 0x00be);
write_cmos_sensor(0x2270, 0xf07f);
write_cmos_sensor(0x2272, 0x0010);
write_cmos_sensor(0x2274, 0x4f0e);
write_cmos_sensor(0x2276, 0xf03e);
write_cmos_sensor(0x2278, 0x0010);
write_cmos_sensor(0x227a, 0x4e82);
write_cmos_sensor(0x227c, 0x81e2);
write_cmos_sensor(0x227e, 0x425f);
write_cmos_sensor(0x2280, 0x008c);
write_cmos_sensor(0x2282, 0x4fc2);
write_cmos_sensor(0x2284, 0x81e0);
write_cmos_sensor(0x2286, 0x43c2);
write_cmos_sensor(0x2288, 0x81e1);
write_cmos_sensor(0x228a, 0x425f);
write_cmos_sensor(0x228c, 0x00cb);
write_cmos_sensor(0x228e, 0x4f49);
write_cmos_sensor(0x2290, 0x425f);
write_cmos_sensor(0x2292, 0x009e);
write_cmos_sensor(0x2294, 0xf07f);
write_cmos_sensor(0x2296, 0x000f);
write_cmos_sensor(0x2298, 0x4f4e);
write_cmos_sensor(0x229a, 0x425f);
write_cmos_sensor(0x229c, 0x009f);
write_cmos_sensor(0x229e, 0xf07f);
write_cmos_sensor(0x22a0, 0x000f);
write_cmos_sensor(0x22a2, 0xf37f);
write_cmos_sensor(0x22a4, 0x5f0e);
write_cmos_sensor(0x22a6, 0x110e);
write_cmos_sensor(0x22a8, 0x425f);
write_cmos_sensor(0x22aa, 0x00b2);
write_cmos_sensor(0x22ac, 0xf07f);
write_cmos_sensor(0x22ae, 0x000f);
write_cmos_sensor(0x22b0, 0x4f48);
write_cmos_sensor(0x22b2, 0x425f);
write_cmos_sensor(0x22b4, 0x00b3);
write_cmos_sensor(0x22b6, 0xf07f);
write_cmos_sensor(0x22b8, 0x000f);
write_cmos_sensor(0x22ba, 0xf37f);
write_cmos_sensor(0x22bc, 0x5f08);
write_cmos_sensor(0x22be, 0x1108);
write_cmos_sensor(0x22c0, 0x421f);
write_cmos_sensor(0x22c2, 0x0098);
write_cmos_sensor(0x22c4, 0x821f);
write_cmos_sensor(0x22c6, 0x0092);
write_cmos_sensor(0x22c8, 0x531f);
write_cmos_sensor(0x22ca, 0x4f0c);
write_cmos_sensor(0x22cc, 0x4e0a);
write_cmos_sensor(0x22ce, 0x12b0);
write_cmos_sensor(0x22d0, 0xfba8);
write_cmos_sensor(0x22d2, 0x4c82);
write_cmos_sensor(0x22d4, 0x0a86);
write_cmos_sensor(0x22d6, 0x9309);
write_cmos_sensor(0x22d8, 0x2002);
write_cmos_sensor(0x22da, 0x4030);
write_cmos_sensor(0x22dc, 0xf78c);
write_cmos_sensor(0x22de, 0x9318);
write_cmos_sensor(0x22e0, 0x2002);
write_cmos_sensor(0x22e2, 0x4030);
write_cmos_sensor(0x22e4, 0xf78c);
write_cmos_sensor(0x22e6, 0x421f);
write_cmos_sensor(0x22e8, 0x00ac);
write_cmos_sensor(0x22ea, 0x821f);
write_cmos_sensor(0x22ec, 0x00a6);
write_cmos_sensor(0x22ee, 0x531f);
write_cmos_sensor(0x22f0, 0x5338);
	write_cmos_sensor(0x22f2, 0x4f0c);
write_cmos_sensor(0x22f4, 0x480a);
	write_cmos_sensor(0x22f6, 0x12b0);
write_cmos_sensor(0x22f8, 0xfba8);
	write_cmos_sensor(0x22fa, 0x4c82);
	write_cmos_sensor(0x22fc, 0x0a88);
write_cmos_sensor(0x22fe, 0x40b2);
write_cmos_sensor(0x2300, 0x0034);
write_cmos_sensor(0x2302, 0x7900);
write_cmos_sensor(0x2304, 0x4292);
write_cmos_sensor(0x2306, 0x0092);
write_cmos_sensor(0x2308, 0x0a8c);
write_cmos_sensor(0x230a, 0x4292);
write_cmos_sensor(0x230c, 0x0098);
write_cmos_sensor(0x230e, 0x0a9e);
write_cmos_sensor(0x2310, 0x40b2);
write_cmos_sensor(0x2312, 0x0c01);
write_cmos_sensor(0x2314, 0x7500);
write_cmos_sensor(0x2316, 0x40b2);
write_cmos_sensor(0x2318, 0x0c01);
write_cmos_sensor(0x231a, 0x8270);
write_cmos_sensor(0x231c, 0x40b2);
write_cmos_sensor(0x231e, 0x0803);
write_cmos_sensor(0x2320, 0x7502);
write_cmos_sensor(0x2322, 0x40b2);
write_cmos_sensor(0x2324, 0x0807);
write_cmos_sensor(0x2326, 0x7504);
write_cmos_sensor(0x2328, 0x40b2);
write_cmos_sensor(0x232a, 0x5803);
write_cmos_sensor(0x232c, 0x7506);
write_cmos_sensor(0x232e, 0x40b2);
write_cmos_sensor(0x2330, 0x0801);
write_cmos_sensor(0x2332, 0x7508);
write_cmos_sensor(0x2334, 0x40b2);
write_cmos_sensor(0x2336, 0x0805);
write_cmos_sensor(0x2338, 0x750a);
write_cmos_sensor(0x233a, 0x40b2);
write_cmos_sensor(0x233c, 0x5801);
write_cmos_sensor(0x233e, 0x750c);
write_cmos_sensor(0x2340, 0x40b2);
write_cmos_sensor(0x2342, 0x0803);
write_cmos_sensor(0x2344, 0x750e);
write_cmos_sensor(0x2346, 0x40b2);
write_cmos_sensor(0x2348, 0x0802);
write_cmos_sensor(0x234a, 0x7510);
write_cmos_sensor(0x234c, 0x40b2);
write_cmos_sensor(0x234e, 0x0800);
write_cmos_sensor(0x2350, 0x7512);
write_cmos_sensor(0x2352, 0x403f);
write_cmos_sensor(0x2354, 0x8270);
write_cmos_sensor(0x2356, 0x12b0);
write_cmos_sensor(0x2358, 0xe2dc);
write_cmos_sensor(0x235a, 0x4392);
write_cmos_sensor(0x235c, 0x7f06);
write_cmos_sensor(0x235e, 0x43a2);
write_cmos_sensor(0x2360, 0x7f0a);
write_cmos_sensor(0x2362, 0x9382);
write_cmos_sensor(0x2364, 0x81e0);
write_cmos_sensor(0x2366, 0x2002);
write_cmos_sensor(0x2368, 0x4030);
write_cmos_sensor(0x236a, 0xf75a);
write_cmos_sensor(0x236c, 0x42a2);
write_cmos_sensor(0x236e, 0x7f0a);
write_cmos_sensor(0x2370, 0x40b2);
write_cmos_sensor(0x2372, 0x000a);
write_cmos_sensor(0x2374, 0x8264);
write_cmos_sensor(0x2376, 0x40b2);
write_cmos_sensor(0x2378, 0x01ea);
write_cmos_sensor(0x237a, 0x8266);
write_cmos_sensor(0x237c, 0x40b2);
write_cmos_sensor(0x237e, 0x03ca);
write_cmos_sensor(0x2380, 0x8268);
write_cmos_sensor(0x2382, 0x40b2);
write_cmos_sensor(0x2384, 0x05aa);
write_cmos_sensor(0x2386, 0x826a);
write_cmos_sensor(0x2388, 0x40b2);
write_cmos_sensor(0x238a, 0xf0d2);
write_cmos_sensor(0x238c, 0x8272);
write_cmos_sensor(0x238e, 0x4382);
write_cmos_sensor(0x2390, 0x81d0);
write_cmos_sensor(0x2392, 0x9382);
write_cmos_sensor(0x2394, 0x7f0a);
write_cmos_sensor(0x2396, 0x2413);
write_cmos_sensor(0x2398, 0x430e);
write_cmos_sensor(0x239a, 0x421d);
write_cmos_sensor(0x239c, 0x8272);
write_cmos_sensor(0x239e, 0x0800);
write_cmos_sensor(0x23a0, 0x7f08);
write_cmos_sensor(0x23a2, 0x4e0f);
write_cmos_sensor(0x23a4, 0x5f0f);
write_cmos_sensor(0x23a6, 0x4f92);
write_cmos_sensor(0x23a8, 0x8264);
write_cmos_sensor(0x23aa, 0x7f00);
write_cmos_sensor(0x23ac, 0x4d82);
write_cmos_sensor(0x23ae, 0x7f02);
write_cmos_sensor(0x23b0, 0x531e);
write_cmos_sensor(0x23b2, 0x421f);
	write_cmos_sensor(0x23b4, 0x7f0a);
write_cmos_sensor(0x23b6, 0x9f0e);
write_cmos_sensor(0x23b8, 0x2bf2);
write_cmos_sensor(0x23ba, 0x4e82);
write_cmos_sensor(0x23bc, 0x81d0);
write_cmos_sensor(0x23be, 0x407b);
write_cmos_sensor(0x23c0, 0xff8b);
write_cmos_sensor(0x23c2, 0x4392);
write_cmos_sensor(0x23c4, 0x731c);
write_cmos_sensor(0x23c6, 0x40b2);
write_cmos_sensor(0x23c8, 0x8038);
write_cmos_sensor(0x23ca, 0x7a00);
write_cmos_sensor(0x23cc, 0x40b2);
write_cmos_sensor(0x23ce, 0x0064);
write_cmos_sensor(0x23d0, 0x7a02);
write_cmos_sensor(0x23d2, 0x40b2);
write_cmos_sensor(0x23d4, 0x0304);
write_cmos_sensor(0x23d6, 0x7a08);
write_cmos_sensor(0x23d8, 0xb3e2);
write_cmos_sensor(0x23da, 0x00ce);
write_cmos_sensor(0x23dc, 0x242d);
write_cmos_sensor(0x23de, 0x4292);
write_cmos_sensor(0x23e0, 0x0126);
write_cmos_sensor(0x23e2, 0x0580);
write_cmos_sensor(0x23e4, 0x4292);
write_cmos_sensor(0x23e6, 0x01a8);
write_cmos_sensor(0x23e8, 0x0582);
write_cmos_sensor(0x23ea, 0x4292);
write_cmos_sensor(0x23ec, 0x01aa);
write_cmos_sensor(0x23ee, 0x0584);
write_cmos_sensor(0x23f0, 0x4292);
write_cmos_sensor(0x23f2, 0x01ac);
write_cmos_sensor(0x23f4, 0x0586);
write_cmos_sensor(0x23f6, 0x9382);
write_cmos_sensor(0x23f8, 0x003a);
write_cmos_sensor(0x23fa, 0x2599);
write_cmos_sensor(0x23fc, 0x90b2);
write_cmos_sensor(0x23fe, 0x0011);
write_cmos_sensor(0x2400, 0x003a);
write_cmos_sensor(0x2402, 0x2d95);
write_cmos_sensor(0x2404, 0x403e);
write_cmos_sensor(0x2406, 0x012e);
write_cmos_sensor(0x2408, 0x4e6f);
write_cmos_sensor(0x240a, 0xf37f);
write_cmos_sensor(0x240c, 0x521f);
write_cmos_sensor(0x240e, 0x0126);
write_cmos_sensor(0x2410, 0x4f82);
write_cmos_sensor(0x2412, 0x0580);
write_cmos_sensor(0x2414, 0x4e6f);
write_cmos_sensor(0x2416, 0xf37f);
write_cmos_sensor(0x2418, 0x521f);
write_cmos_sensor(0x241a, 0x01a8);
write_cmos_sensor(0x241c, 0x4f82);
write_cmos_sensor(0x241e, 0x0582);
write_cmos_sensor(0x2420, 0x4e6f);
write_cmos_sensor(0x2422, 0xf37f);
write_cmos_sensor(0x2424, 0x521f);
write_cmos_sensor(0x2426, 0x01aa);
write_cmos_sensor(0x2428, 0x4f82);
write_cmos_sensor(0x242a, 0x0584);
write_cmos_sensor(0x242c, 0x4e6f);
write_cmos_sensor(0x242e, 0xf37f);
write_cmos_sensor(0x2430, 0x521f);
write_cmos_sensor(0x2432, 0x01ac);
write_cmos_sensor(0x2434, 0x4f82);
write_cmos_sensor(0x2436, 0x0586);
write_cmos_sensor(0x2438, 0x9382);
write_cmos_sensor(0x243a, 0x81de);
write_cmos_sensor(0x243c, 0x2411);
write_cmos_sensor(0x243e, 0x4382);
	write_cmos_sensor(0x2440, 0x81d0);
write_cmos_sensor(0x2442, 0x421d);
write_cmos_sensor(0x2444, 0x81d0);
write_cmos_sensor(0x2446, 0x4d0e);
write_cmos_sensor(0x2448, 0x5e0e);
write_cmos_sensor(0x244a, 0x4e0f);
write_cmos_sensor(0x244c, 0x510f);
write_cmos_sensor(0x244e, 0x4e9f);
write_cmos_sensor(0x2450, 0x0b00);
write_cmos_sensor(0x2452, 0x0000);
write_cmos_sensor(0x2454, 0x531d);
write_cmos_sensor(0x2456, 0x4d82);
write_cmos_sensor(0x2458, 0x81d0);
write_cmos_sensor(0x245a, 0x903d);
write_cmos_sensor(0x245c, 0x0016);
write_cmos_sensor(0x245e, 0x2bf1);
write_cmos_sensor(0x2460, 0xb3d2);
write_cmos_sensor(0x2462, 0x00ce);
write_cmos_sensor(0x2464, 0x2412);
write_cmos_sensor(0x2466, 0x4382);
write_cmos_sensor(0x2468, 0x81d0);
write_cmos_sensor(0x246a, 0x421e);
write_cmos_sensor(0x246c, 0x81d0);
write_cmos_sensor(0x246e, 0x903e);
write_cmos_sensor(0x2470, 0x0009);
write_cmos_sensor(0x2472, 0x2405);
write_cmos_sensor(0x2474, 0x5e0e);
write_cmos_sensor(0x2476, 0x4e0f);
write_cmos_sensor(0x2478, 0x510f);
write_cmos_sensor(0x247a, 0x4fae);
write_cmos_sensor(0x247c, 0x0b80);
write_cmos_sensor(0x247e, 0x5392);
write_cmos_sensor(0x2480, 0x81d0);
write_cmos_sensor(0x2482, 0x90b2);
write_cmos_sensor(0x2484, 0x0016);
write_cmos_sensor(0x2486, 0x81d0);
write_cmos_sensor(0x2488, 0x2bf0);
write_cmos_sensor(0x248a, 0x9382);
write_cmos_sensor(0x248c, 0x81de);
write_cmos_sensor(0x248e, 0x200b);
write_cmos_sensor(0x2490, 0x0b00);
write_cmos_sensor(0x2492, 0x7302);
write_cmos_sensor(0x2494, 0x0258);
write_cmos_sensor(0x2496, 0x4382);
write_cmos_sensor(0x2498, 0x7904);
write_cmos_sensor(0x249a, 0x0900);
write_cmos_sensor(0x249c, 0x7308);
write_cmos_sensor(0x249e, 0x403f);
write_cmos_sensor(0x24a0, 0x8270);
write_cmos_sensor(0x24a2, 0x12b0);
write_cmos_sensor(0x24a4, 0xe2dc);
write_cmos_sensor(0x24a6, 0xb2e2);
write_cmos_sensor(0x24a8, 0x00ce);
write_cmos_sensor(0x24aa, 0x2425);
write_cmos_sensor(0x24ac, 0x4392);
write_cmos_sensor(0x24ae, 0x7326);
write_cmos_sensor(0x24b0, 0x9382);
write_cmos_sensor(0x24b2, 0x81de);
write_cmos_sensor(0x24b4, 0x200f);
write_cmos_sensor(0x24b6, 0x4292);
write_cmos_sensor(0x24b8, 0x8260);
write_cmos_sensor(0x24ba, 0x0b92);
write_cmos_sensor(0x24bc, 0x4292);
write_cmos_sensor(0x24be, 0x826c);
write_cmos_sensor(0x24c0, 0x0580);
write_cmos_sensor(0x24c2, 0x4292);
write_cmos_sensor(0x24c4, 0x8262);
write_cmos_sensor(0x24c6, 0x0582);
write_cmos_sensor(0x24c8, 0x4292);
write_cmos_sensor(0x24ca, 0x826e);
write_cmos_sensor(0x24cc, 0x0584);
write_cmos_sensor(0x24ce, 0x4292);
write_cmos_sensor(0x24d0, 0x8274);
write_cmos_sensor(0x24d2, 0x0586);
write_cmos_sensor(0x24d4, 0x4292);
write_cmos_sensor(0x24d6, 0x00ba);
write_cmos_sensor(0x24d8, 0x8260);
write_cmos_sensor(0x24da, 0x4292);
write_cmos_sensor(0x24dc, 0x0580);
write_cmos_sensor(0x24de, 0x826c);
write_cmos_sensor(0x24e0, 0x4292);
write_cmos_sensor(0x24e2, 0x0582);
write_cmos_sensor(0x24e4, 0x8262);
write_cmos_sensor(0x24e6, 0x4292);
write_cmos_sensor(0x24e8, 0x0584);
write_cmos_sensor(0x24ea, 0x826e);
write_cmos_sensor(0x24ec, 0x4292);
write_cmos_sensor(0x24ee, 0x0586);
write_cmos_sensor(0x24f0, 0x8274);
write_cmos_sensor(0x24f2, 0x4392);
write_cmos_sensor(0x24f4, 0x7326);
write_cmos_sensor(0x24f6, 0x434e);
write_cmos_sensor(0x24f8, 0x9382);
write_cmos_sensor(0x24fa, 0x81de);
write_cmos_sensor(0x24fc, 0x2007);
write_cmos_sensor(0x24fe, 0x421f);
write_cmos_sensor(0x2500, 0x732a);
write_cmos_sensor(0x2502, 0xf35f);
write_cmos_sensor(0x2504, 0x2003);
write_cmos_sensor(0x2506, 0xb3a2);
write_cmos_sensor(0x2508, 0x732a);
write_cmos_sensor(0x250a, 0x2401);
write_cmos_sensor(0x250c, 0x435e);
write_cmos_sensor(0x250e, 0x4ec2);
write_cmos_sensor(0x2510, 0x81e8);
write_cmos_sensor(0x2512, 0x42a2);
write_cmos_sensor(0x2514, 0x81ea);
write_cmos_sensor(0x2516, 0x40b2);
write_cmos_sensor(0x2518, 0x0262);
write_cmos_sensor(0x251a, 0x81ec);
write_cmos_sensor(0x251c, 0x40b2);
write_cmos_sensor(0x251e, 0x02c4);
write_cmos_sensor(0x2520, 0x81ee);
write_cmos_sensor(0x2522, 0x40b2);
write_cmos_sensor(0x2524, 0x0532);
write_cmos_sensor(0x2526, 0x81f0);
write_cmos_sensor(0x2528, 0x40b2);
write_cmos_sensor(0x252a, 0x03b6);
write_cmos_sensor(0x252c, 0x81f2);
write_cmos_sensor(0x252e, 0x40b2);
write_cmos_sensor(0x2530, 0x03c4);
write_cmos_sensor(0x2532, 0x81f4);
write_cmos_sensor(0x2534, 0x40b2);
write_cmos_sensor(0x2536, 0x03de);
write_cmos_sensor(0x2538, 0x81f6);
write_cmos_sensor(0x253a, 0x40b2);
write_cmos_sensor(0x253c, 0x0596);
write_cmos_sensor(0x253e, 0x81f8);
write_cmos_sensor(0x2540, 0x40b2);
write_cmos_sensor(0x2542, 0x05c4);
write_cmos_sensor(0x2544, 0x81fa);
write_cmos_sensor(0x2546, 0x40b2);
write_cmos_sensor(0x2548, 0x0776);
write_cmos_sensor(0x254a, 0x81fc);
write_cmos_sensor(0x254c, 0x9382);
write_cmos_sensor(0x254e, 0x81e2);
write_cmos_sensor(0x2550, 0x24e1);
write_cmos_sensor(0x2552, 0x40b2);
write_cmos_sensor(0x2554, 0x0289);
write_cmos_sensor(0x2556, 0x81ee);
write_cmos_sensor(0x2558, 0x40b2);
write_cmos_sensor(0x255a, 0x04e7);
write_cmos_sensor(0x255c, 0x81f0);
write_cmos_sensor(0x255e, 0x12b0);
write_cmos_sensor(0x2560, 0xea88);
write_cmos_sensor(0x2562, 0x0900);
write_cmos_sensor(0x2564, 0x7328);
write_cmos_sensor(0x2566, 0x4682);
write_cmos_sensor(0x2568, 0x7114);
write_cmos_sensor(0x256a, 0x421f);
write_cmos_sensor(0x256c, 0x7316);
write_cmos_sensor(0x256e, 0xc312);
write_cmos_sensor(0x2570, 0x100f);
write_cmos_sensor(0x2572, 0x503f);
write_cmos_sensor(0x2574, 0xff9c);
write_cmos_sensor(0x2576, 0x4f82);
write_cmos_sensor(0x2578, 0x7334);
write_cmos_sensor(0x257a, 0x0f00);
write_cmos_sensor(0x257c, 0x7302);
write_cmos_sensor(0x257e, 0x4392);
write_cmos_sensor(0x2580, 0x7f0c);
write_cmos_sensor(0x2582, 0x4392);
write_cmos_sensor(0x2584, 0x7f10);
write_cmos_sensor(0x2586, 0x4392);
write_cmos_sensor(0x2588, 0x770a);
write_cmos_sensor(0x258a, 0x4392);
write_cmos_sensor(0x258c, 0x770e);
write_cmos_sensor(0x258e, 0x9392);
write_cmos_sensor(0x2590, 0x7114);
write_cmos_sensor(0x2592, 0x2073);
write_cmos_sensor(0x2594, 0x0b00);
write_cmos_sensor(0x2596, 0x7302);
write_cmos_sensor(0x2598, 0x0258);
write_cmos_sensor(0x259a, 0x4382);
write_cmos_sensor(0x259c, 0x7904);
write_cmos_sensor(0x259e, 0x0800);
write_cmos_sensor(0x25a0, 0x7118);
write_cmos_sensor(0x25a2, 0x1247);
write_cmos_sensor(0x25a4, 0x1230);
write_cmos_sensor(0x25a6, 0x0cce);
write_cmos_sensor(0x25a8, 0x1230);
write_cmos_sensor(0x25aa, 0x0cf0);
write_cmos_sensor(0x25ac, 0x410f);
write_cmos_sensor(0x25ae, 0x503f);
write_cmos_sensor(0x25b0, 0x0030);
write_cmos_sensor(0x25b2, 0x120f);
write_cmos_sensor(0x25b4, 0x421c);
write_cmos_sensor(0x25b6, 0x0ca0);
write_cmos_sensor(0x25b8, 0x421d);
write_cmos_sensor(0x25ba, 0x0caa);
write_cmos_sensor(0x25bc, 0x421e);
write_cmos_sensor(0x25be, 0x0cb4);
write_cmos_sensor(0x25c0, 0x421f);
write_cmos_sensor(0x25c2, 0x0cb2);
write_cmos_sensor(0x25c4, 0x12b0);
write_cmos_sensor(0x25c6, 0xf9a4);
write_cmos_sensor(0x25c8, 0x1247);
write_cmos_sensor(0x25ca, 0x1230);
write_cmos_sensor(0x25cc, 0x0cd0);
write_cmos_sensor(0x25ce, 0x1230);
write_cmos_sensor(0x25d0, 0x0cf2);
write_cmos_sensor(0x25d2, 0x410f);
write_cmos_sensor(0x25d4, 0x503f);
write_cmos_sensor(0x25d6, 0x003a);
write_cmos_sensor(0x25d8, 0x120f);
write_cmos_sensor(0x25da, 0x421c);
write_cmos_sensor(0x25dc, 0x0ca2);
write_cmos_sensor(0x25de, 0x421d);
write_cmos_sensor(0x25e0, 0x0cac);
write_cmos_sensor(0x25e2, 0x421e);
write_cmos_sensor(0x25e4, 0x0cb8);
write_cmos_sensor(0x25e6, 0x421f);
write_cmos_sensor(0x25e8, 0x0cb6);
write_cmos_sensor(0x25ea, 0x12b0);
write_cmos_sensor(0x25ec, 0xf9a4);
write_cmos_sensor(0x25ee, 0x1247);
write_cmos_sensor(0x25f0, 0x1230);
write_cmos_sensor(0x25f2, 0x0cd2);
write_cmos_sensor(0x25f4, 0x1230);
write_cmos_sensor(0x25f6, 0x0cf4);
write_cmos_sensor(0x25f8, 0x410f);
write_cmos_sensor(0x25fa, 0x503f);
write_cmos_sensor(0x25fc, 0x0044);
write_cmos_sensor(0x25fe, 0x120f);
write_cmos_sensor(0x2600, 0x421c);
write_cmos_sensor(0x2602, 0x0ca4);
write_cmos_sensor(0x2604, 0x421d);
write_cmos_sensor(0x2606, 0x0cae);
write_cmos_sensor(0x2608, 0x421e);
write_cmos_sensor(0x260a, 0x0cbc);
write_cmos_sensor(0x260c, 0x421f);
write_cmos_sensor(0x260e, 0x0cba);
write_cmos_sensor(0x2610, 0x12b0);
write_cmos_sensor(0x2612, 0xf9a4);
write_cmos_sensor(0x2614, 0x1247);
write_cmos_sensor(0x2616, 0x1230);
write_cmos_sensor(0x2618, 0x0cd4);
write_cmos_sensor(0x261a, 0x1230);
write_cmos_sensor(0x261c, 0x0cf6);
write_cmos_sensor(0x261e, 0x410f);
write_cmos_sensor(0x2620, 0x503f);
write_cmos_sensor(0x2622, 0x004e);
write_cmos_sensor(0x2624, 0x120f);
write_cmos_sensor(0x2626, 0x421c);
write_cmos_sensor(0x2628, 0x0ca6);
write_cmos_sensor(0x262a, 0x421d);
write_cmos_sensor(0x262c, 0x0cb0);
write_cmos_sensor(0x262e, 0x421e);
write_cmos_sensor(0x2630, 0x0cc0);
write_cmos_sensor(0x2632, 0x421f);
write_cmos_sensor(0x2634, 0x0cbe);
write_cmos_sensor(0x2636, 0x12b0);
write_cmos_sensor(0x2638, 0xf9a4);
write_cmos_sensor(0x263a, 0x425f);
write_cmos_sensor(0x263c, 0x0c80);
write_cmos_sensor(0x263e, 0xf35f);
write_cmos_sensor(0x2640, 0x5031);
write_cmos_sensor(0x2642, 0x0020);
write_cmos_sensor(0x2644, 0x934f);
write_cmos_sensor(0x2646, 0x2008);
write_cmos_sensor(0x2648, 0x4382);
write_cmos_sensor(0x264a, 0x0cce);
write_cmos_sensor(0x264c, 0x4382);
write_cmos_sensor(0x264e, 0x0cd0);
write_cmos_sensor(0x2650, 0x4382);
write_cmos_sensor(0x2652, 0x0cd2);
write_cmos_sensor(0x2654, 0x4382);
write_cmos_sensor(0x2656, 0x0cd4);
write_cmos_sensor(0x2658, 0x425f);
write_cmos_sensor(0x265a, 0x81e8);
write_cmos_sensor(0x265c, 0xdf47);
write_cmos_sensor(0x265e, 0x474e);
write_cmos_sensor(0x2660, 0x934f);
write_cmos_sensor(0x2662, 0x2001);
write_cmos_sensor(0x2664, 0x5e0e);
write_cmos_sensor(0x2666, 0x4e47);
write_cmos_sensor(0x2668, 0x0900);
write_cmos_sensor(0x266a, 0x7112);
write_cmos_sensor(0x266c, 0x4b4f);
write_cmos_sensor(0x266e, 0x12b0);
write_cmos_sensor(0x2670, 0xfaa0);
write_cmos_sensor(0x2672, 0x0b00);
write_cmos_sensor(0x2674, 0x7302);
write_cmos_sensor(0x2676, 0x0036);
write_cmos_sensor(0x2678, 0x3f8a);
write_cmos_sensor(0x267a, 0x0b00);
write_cmos_sensor(0x267c, 0x7302);
write_cmos_sensor(0x267e, 0x0036);
write_cmos_sensor(0x2680, 0x4392);
write_cmos_sensor(0x2682, 0x7904);
write_cmos_sensor(0x2684, 0x421e);
write_cmos_sensor(0x2686, 0x7100);
write_cmos_sensor(0x2688, 0x9382);
write_cmos_sensor(0x268a, 0x7114);
write_cmos_sensor(0x268c, 0x2009);
write_cmos_sensor(0x268e, 0x421f);
write_cmos_sensor(0x2690, 0x00a2);
write_cmos_sensor(0x2692, 0x9f0e);
write_cmos_sensor(0x2694, 0x2803);
write_cmos_sensor(0x2696, 0x9e82);
write_cmos_sensor(0x2698, 0x00a8);
write_cmos_sensor(0x269a, 0x2c02);
write_cmos_sensor(0x269c, 0x4382);
write_cmos_sensor(0x269e, 0x7904);
write_cmos_sensor(0x26a0, 0x93a2);
write_cmos_sensor(0x26a2, 0x7114);
write_cmos_sensor(0x26a4, 0x2424);
write_cmos_sensor(0x26a6, 0x903e);
write_cmos_sensor(0x26a8, 0x0038);
write_cmos_sensor(0x26aa, 0x2815);
write_cmos_sensor(0x26ac, 0x503e);
write_cmos_sensor(0x26ae, 0xffd8);
write_cmos_sensor(0x26b0, 0x4e82);
write_cmos_sensor(0x26b2, 0x7a04);
write_cmos_sensor(0x26b4, 0x43b2);
write_cmos_sensor(0x26b6, 0x7a06);
write_cmos_sensor(0x26b8, 0x9382);
write_cmos_sensor(0x26ba, 0x81e0);
write_cmos_sensor(0x26bc, 0x2408);
write_cmos_sensor(0x26be, 0x9382);
write_cmos_sensor(0x26c0, 0x81e4);
write_cmos_sensor(0x26c2, 0x2411);
write_cmos_sensor(0x26c4, 0x421f);
write_cmos_sensor(0x26c6, 0x7a04);
write_cmos_sensor(0x26c8, 0x832f);
write_cmos_sensor(0x26ca, 0x4f82);
write_cmos_sensor(0x26cc, 0x7a06);
write_cmos_sensor(0x26ce, 0x4392);
write_cmos_sensor(0x26d0, 0x7a0a);
write_cmos_sensor(0x26d2, 0x0800);
write_cmos_sensor(0x26d4, 0x7a0a);
write_cmos_sensor(0x26d6, 0x4b4f);
write_cmos_sensor(0x26d8, 0x12b0);
write_cmos_sensor(0x26da, 0xfaa0);
write_cmos_sensor(0x26dc, 0x930f);
write_cmos_sensor(0x26de, 0x2757);
write_cmos_sensor(0x26e0, 0x4382);
write_cmos_sensor(0x26e2, 0x81de);
write_cmos_sensor(0x26e4, 0x3e79);
write_cmos_sensor(0x26e6, 0x421f);
write_cmos_sensor(0x26e8, 0x7a04);
write_cmos_sensor(0x26ea, 0x532f);
write_cmos_sensor(0x26ec, 0x3fee);
write_cmos_sensor(0x26ee, 0x421f);
write_cmos_sensor(0x26f0, 0x00a6);
write_cmos_sensor(0x26f2, 0x9f0e);
write_cmos_sensor(0x26f4, 0x2803);
write_cmos_sensor(0x26f6, 0x9e82);
write_cmos_sensor(0x26f8, 0x00ac);
write_cmos_sensor(0x26fa, 0x2c02);
write_cmos_sensor(0x26fc, 0x4382);
write_cmos_sensor(0x26fe, 0x7904);
write_cmos_sensor(0x2700, 0x4b4f);
write_cmos_sensor(0x2702, 0xc312);
write_cmos_sensor(0x2704, 0x104f);
write_cmos_sensor(0x2706, 0xf35b);
write_cmos_sensor(0x2708, 0xe37b);
write_cmos_sensor(0x270a, 0x535b);
write_cmos_sensor(0x270c, 0xf07b);
write_cmos_sensor(0x270e, 0xffb8);
write_cmos_sensor(0x2710, 0xef4b);
write_cmos_sensor(0x2712, 0x3fc9);
write_cmos_sensor(0x2714, 0x9382);
write_cmos_sensor(0x2716, 0x81e0);
write_cmos_sensor(0x2718, 0x2722);
write_cmos_sensor(0x271a, 0x40b2);
write_cmos_sensor(0x271c, 0x001e);
write_cmos_sensor(0x271e, 0x81ec);
write_cmos_sensor(0x2720, 0x40b2);
write_cmos_sensor(0x2722, 0x01d6);
write_cmos_sensor(0x2724, 0x81ee);
write_cmos_sensor(0x2726, 0x40b2);
write_cmos_sensor(0x2728, 0x0205);
write_cmos_sensor(0x272a, 0x81f0);
write_cmos_sensor(0x272c, 0x3f18);
write_cmos_sensor(0x272e, 0x90b2);
write_cmos_sensor(0x2730, 0x0011);
write_cmos_sensor(0x2732, 0x003a);
write_cmos_sensor(0x2734, 0x2807);
write_cmos_sensor(0x2736, 0x90b2);
write_cmos_sensor(0x2738, 0x0080);
write_cmos_sensor(0x273a, 0x003a);
write_cmos_sensor(0x273c, 0x2c03);
write_cmos_sensor(0x273e, 0x403e);
write_cmos_sensor(0x2740, 0x012f);
write_cmos_sensor(0x2742, 0x3e62);
write_cmos_sensor(0x2744, 0x90b2);
write_cmos_sensor(0x2746, 0x0080);
write_cmos_sensor(0x2748, 0x003a);
write_cmos_sensor(0x274a, 0x2a76);
write_cmos_sensor(0x274c, 0x90b2);
write_cmos_sensor(0x274e, 0x0100);
write_cmos_sensor(0x2750, 0x003a);
write_cmos_sensor(0x2752, 0x2e72);
write_cmos_sensor(0x2754, 0x403e);
write_cmos_sensor(0x2756, 0x0130);
write_cmos_sensor(0x2758, 0x3e57);
write_cmos_sensor(0x275a, 0x9382);
write_cmos_sensor(0x275c, 0x81e2);
write_cmos_sensor(0x275e, 0x240b);
write_cmos_sensor(0x2760, 0x40b2);
write_cmos_sensor(0x2762, 0x000e);
write_cmos_sensor(0x2764, 0x8264);
write_cmos_sensor(0x2766, 0x40b2);
write_cmos_sensor(0x2768, 0x028f);
write_cmos_sensor(0x276a, 0x8266);
write_cmos_sensor(0x276c, 0x40b2);
write_cmos_sensor(0x276e, 0xf06a);
write_cmos_sensor(0x2770, 0x8272);
write_cmos_sensor(0x2772, 0x4030);
write_cmos_sensor(0x2774, 0xf38e);
write_cmos_sensor(0x2776, 0x40b2);
write_cmos_sensor(0x2778, 0x000e);
write_cmos_sensor(0x277a, 0x8264);
write_cmos_sensor(0x277c, 0x40b2);
write_cmos_sensor(0x277e, 0x02ce);
write_cmos_sensor(0x2780, 0x8266);
write_cmos_sensor(0x2782, 0x40b2);
write_cmos_sensor(0x2784, 0xf000);
write_cmos_sensor(0x2786, 0x8272);
write_cmos_sensor(0x2788, 0x4030);
write_cmos_sensor(0x278a, 0xf38e);
write_cmos_sensor(0x278c, 0x421f);
write_cmos_sensor(0x278e, 0x00ac);
write_cmos_sensor(0x2790, 0x821f);
write_cmos_sensor(0x2792, 0x00a6);
write_cmos_sensor(0x2794, 0x531f);
write_cmos_sensor(0x2796, 0x4030);
write_cmos_sensor(0x2798, 0xf2f2);
write_cmos_sensor(0x279a, 0xb3e2);
write_cmos_sensor(0x279c, 0x00b4);
write_cmos_sensor(0x279e, 0x2002);
write_cmos_sensor(0x27a0, 0x4030);
write_cmos_sensor(0x27a2, 0xf248);
write_cmos_sensor(0x27a4, 0x4316);
write_cmos_sensor(0x27a6, 0x4030);
write_cmos_sensor(0x27a8, 0xf248);
write_cmos_sensor(0x27aa, 0x43d2);
write_cmos_sensor(0x27ac, 0x0180);
write_cmos_sensor(0x27ae, 0x4392);
write_cmos_sensor(0x27b0, 0x760e);
write_cmos_sensor(0x27b2, 0x9382);
write_cmos_sensor(0x27b4, 0x760c);
write_cmos_sensor(0x27b6, 0x2002);
write_cmos_sensor(0x27b8, 0x0c64);
write_cmos_sensor(0x27ba, 0x3ffb);
write_cmos_sensor(0x27bc, 0x421f);
write_cmos_sensor(0x27be, 0x760a);
write_cmos_sensor(0x27c0, 0x932f);
write_cmos_sensor(0x27c2, 0x2012);
write_cmos_sensor(0x27c4, 0x421b);
write_cmos_sensor(0x27c6, 0x018a);
write_cmos_sensor(0x27c8, 0x4b82);
write_cmos_sensor(0x27ca, 0x7600);
write_cmos_sensor(0x27cc, 0x12b0);
write_cmos_sensor(0x27ce, 0xfb18);
write_cmos_sensor(0x27d0, 0x903b);
write_cmos_sensor(0x27d2, 0x1000);
write_cmos_sensor(0x27d4, 0x2806);
write_cmos_sensor(0x27d6, 0x403f);
write_cmos_sensor(0x27d8, 0x8028);
write_cmos_sensor(0x27da, 0x12b0);
write_cmos_sensor(0x27dc, 0xfb2c);
write_cmos_sensor(0x27de, 0x4b0d);
write_cmos_sensor(0x27e0, 0x3fe6);
write_cmos_sensor(0x27e2, 0x403f);
write_cmos_sensor(0x27e4, 0x0028);
write_cmos_sensor(0x27e6, 0x3ff9);
write_cmos_sensor(0x27e8, 0x903f);
write_cmos_sensor(0x27ea, 0x0003);
write_cmos_sensor(0x27ec, 0x28af);
write_cmos_sensor(0x27ee, 0x903f);
write_cmos_sensor(0x27f0, 0x0102);
write_cmos_sensor(0x27f2, 0x208a);
write_cmos_sensor(0x27f4, 0x43c2);
write_cmos_sensor(0x27f6, 0x018c);
write_cmos_sensor(0x27f8, 0x425f);
write_cmos_sensor(0x27fa, 0x0186);
write_cmos_sensor(0x27fc, 0x4f49);
write_cmos_sensor(0x27fe, 0x93d2);
write_cmos_sensor(0x2800, 0x018f);
write_cmos_sensor(0x2802, 0x2480);
write_cmos_sensor(0x2804, 0x425f);
write_cmos_sensor(0x2806, 0x018f);
write_cmos_sensor(0x2808, 0x4f4a);
write_cmos_sensor(0x280a, 0x4b0e);
write_cmos_sensor(0x280c, 0x108e);
write_cmos_sensor(0x280e, 0xf37e);
write_cmos_sensor(0x2810, 0xc312);
write_cmos_sensor(0x2812, 0x100e);
write_cmos_sensor(0x2814, 0x110e);
write_cmos_sensor(0x2816, 0x110e);
write_cmos_sensor(0x2818, 0x110e);
write_cmos_sensor(0x281a, 0x4d0f);
write_cmos_sensor(0x281c, 0x108f);
write_cmos_sensor(0x281e, 0xf37f);
write_cmos_sensor(0x2820, 0xc312);
write_cmos_sensor(0x2822, 0x100f);
write_cmos_sensor(0x2824, 0x110f);
write_cmos_sensor(0x2826, 0x110f);
write_cmos_sensor(0x2828, 0x110f);
write_cmos_sensor(0x282a, 0x9f0e);
write_cmos_sensor(0x282c, 0x240d);
write_cmos_sensor(0x282e, 0x0261);
write_cmos_sensor(0x2830, 0x0000);
	write_cmos_sensor(0x2832, 0x4b82);
	write_cmos_sensor(0x2834, 0x7600);
write_cmos_sensor(0x2836, 0x12b0);
write_cmos_sensor(0x2838, 0xfb18);
write_cmos_sensor(0x283a, 0x903b);
write_cmos_sensor(0x283c, 0x1000);
write_cmos_sensor(0x283e, 0x285f);
write_cmos_sensor(0x2840, 0x403f);
write_cmos_sensor(0x2842, 0x8028);
write_cmos_sensor(0x2844, 0x12b0);
write_cmos_sensor(0x2846, 0xfb2c);
write_cmos_sensor(0x2848, 0x4382);
write_cmos_sensor(0x284a, 0x81d0);
write_cmos_sensor(0x284c, 0x430c);
write_cmos_sensor(0x284e, 0x4c0d);
write_cmos_sensor(0x2850, 0x431f);
write_cmos_sensor(0x2852, 0x4c0e);
write_cmos_sensor(0x2854, 0x930e);
write_cmos_sensor(0x2856, 0x2403);
write_cmos_sensor(0x2858, 0x5f0f);
write_cmos_sensor(0x285a, 0x831e);
write_cmos_sensor(0x285c, 0x23fd);
write_cmos_sensor(0x285e, 0xf90f);
write_cmos_sensor(0x2860, 0x2444);
write_cmos_sensor(0x2862, 0x430f);
write_cmos_sensor(0x2864, 0x9a0f);
write_cmos_sensor(0x2866, 0x2c2c);
write_cmos_sensor(0x2868, 0x4c0e);
write_cmos_sensor(0x286a, 0x4b82);
write_cmos_sensor(0x286c, 0x7600);
write_cmos_sensor(0x286e, 0x4e82);
write_cmos_sensor(0x2870, 0x7602);
write_cmos_sensor(0x2872, 0x4982);
write_cmos_sensor(0x2874, 0x7604);
write_cmos_sensor(0x2876, 0x0264);
	write_cmos_sensor(0x2878, 0x0000);
write_cmos_sensor(0x287a, 0x0224);
write_cmos_sensor(0x287c, 0x0000);
write_cmos_sensor(0x287e, 0x0264);
write_cmos_sensor(0x2880, 0x0000);
write_cmos_sensor(0x2882, 0x0260);
write_cmos_sensor(0x2884, 0x0000);
write_cmos_sensor(0x2886, 0x0268);
write_cmos_sensor(0x2888, 0x0000);
write_cmos_sensor(0x288a, 0x0c5a);
write_cmos_sensor(0x288c, 0x02e8);
write_cmos_sensor(0x288e, 0x0000);
write_cmos_sensor(0x2890, 0x0cb5);
write_cmos_sensor(0x2892, 0x02a8);
write_cmos_sensor(0x2894, 0x0000);
write_cmos_sensor(0x2896, 0x0cb5);
write_cmos_sensor(0x2898, 0x0cb5);
write_cmos_sensor(0x289a, 0x0cb5);
write_cmos_sensor(0x289c, 0x0cb5);
write_cmos_sensor(0x289e, 0x0cb5);
write_cmos_sensor(0x28a0, 0x0cb5);
write_cmos_sensor(0x28a2, 0x0cb5);
write_cmos_sensor(0x28a4, 0x0cb5);
write_cmos_sensor(0x28a6, 0x0c00);
write_cmos_sensor(0x28a8, 0x02e8);
write_cmos_sensor(0x28aa, 0x0000);
write_cmos_sensor(0x28ac, 0x0cb5);
write_cmos_sensor(0x28ae, 0x0268);
write_cmos_sensor(0x28b0, 0x0000);
write_cmos_sensor(0x28b2, 0x0c64);
write_cmos_sensor(0x28b4, 0x0260);
write_cmos_sensor(0x28b6, 0x0000);
write_cmos_sensor(0x28b8, 0x0c64);
write_cmos_sensor(0x28ba, 0x531f);
write_cmos_sensor(0x28bc, 0x9a0f);
write_cmos_sensor(0x28be, 0x2bd5);
write_cmos_sensor(0x28c0, 0x4c82);
write_cmos_sensor(0x28c2, 0x7602);
write_cmos_sensor(0x28c4, 0x4b82);
write_cmos_sensor(0x28c6, 0x7600);
write_cmos_sensor(0x28c8, 0x0270);
write_cmos_sensor(0x28ca, 0x0000);
write_cmos_sensor(0x28cc, 0x0c1c);
write_cmos_sensor(0x28ce, 0x0270);
write_cmos_sensor(0x28d0, 0x0001);
write_cmos_sensor(0x28d2, 0x421f);
write_cmos_sensor(0x28d4, 0x7606);
write_cmos_sensor(0x28d6, 0x4f4e);
write_cmos_sensor(0x28d8, 0x431f);
write_cmos_sensor(0x28da, 0x930d);
write_cmos_sensor(0x28dc, 0x2403);
write_cmos_sensor(0x28de, 0x5f0f);
write_cmos_sensor(0x28e0, 0x831d);
write_cmos_sensor(0x28e2, 0x23fd);
write_cmos_sensor(0x28e4, 0xff4e);
write_cmos_sensor(0x28e6, 0xdec2);
write_cmos_sensor(0x28e8, 0x018c);
write_cmos_sensor(0x28ea, 0x531c);
write_cmos_sensor(0x28ec, 0x923c);
write_cmos_sensor(0x28ee, 0x2baf);
write_cmos_sensor(0x28f0, 0x4c82);
write_cmos_sensor(0x28f2, 0x81d0);
write_cmos_sensor(0x28f4, 0x0260);
write_cmos_sensor(0x28f6, 0x0000);
write_cmos_sensor(0x28f8, 0x4b0d);
write_cmos_sensor(0x28fa, 0x531b);
write_cmos_sensor(0x28fc, 0x3f58);
write_cmos_sensor(0x28fe, 0x403f);
write_cmos_sensor(0x2900, 0x0028);
write_cmos_sensor(0x2902, 0x3fa0);
write_cmos_sensor(0x2904, 0x432a);
write_cmos_sensor(0x2906, 0x3f81);
write_cmos_sensor(0x2908, 0x903f);
write_cmos_sensor(0x290a, 0x0201);
write_cmos_sensor(0x290c, 0x2350);
write_cmos_sensor(0x290e, 0x531b);
write_cmos_sensor(0x2910, 0x4b0e);
write_cmos_sensor(0x2912, 0x108e);
write_cmos_sensor(0x2914, 0xf37e);
write_cmos_sensor(0x2916, 0xc312);
write_cmos_sensor(0x2918, 0x100e);
write_cmos_sensor(0x291a, 0x110e);
write_cmos_sensor(0x291c, 0x110e);
write_cmos_sensor(0x291e, 0x110e);
write_cmos_sensor(0x2920, 0x4d0f);
write_cmos_sensor(0x2922, 0x108f);
write_cmos_sensor(0x2924, 0xf37f);
write_cmos_sensor(0x2926, 0xc312);
write_cmos_sensor(0x2928, 0x100f);
write_cmos_sensor(0x292a, 0x110f);
write_cmos_sensor(0x292c, 0x110f);
write_cmos_sensor(0x292e, 0x110f);
write_cmos_sensor(0x2930, 0x9f0e);
write_cmos_sensor(0x2932, 0x2406);
write_cmos_sensor(0x2934, 0x0261);
write_cmos_sensor(0x2936, 0x0000);
write_cmos_sensor(0x2938, 0x4b82);
write_cmos_sensor(0x293a, 0x7600);
write_cmos_sensor(0x293c, 0x12b0);
write_cmos_sensor(0x293e, 0xfb18);
write_cmos_sensor(0x2940, 0x4b0f);
write_cmos_sensor(0x2942, 0x12b0);
write_cmos_sensor(0x2944, 0xfb56);
write_cmos_sensor(0x2946, 0x4fc2);
write_cmos_sensor(0x2948, 0x0188);
write_cmos_sensor(0x294a, 0x3f49);
write_cmos_sensor(0x294c, 0x931f);
write_cmos_sensor(0x294e, 0x232f);
write_cmos_sensor(0x2950, 0x421b);
write_cmos_sensor(0x2952, 0x018a);
write_cmos_sensor(0x2954, 0x3ff1);
write_cmos_sensor(0x2956, 0x40b2);
write_cmos_sensor(0x2958, 0x1807);
write_cmos_sensor(0x295a, 0x0b82);
write_cmos_sensor(0x295c, 0x40b2);
write_cmos_sensor(0x295e, 0x3540);
write_cmos_sensor(0x2960, 0x0b84);
write_cmos_sensor(0x2962, 0x40b2);
write_cmos_sensor(0x2964, 0x3540);
write_cmos_sensor(0x2966, 0x0b86);
write_cmos_sensor(0x2968, 0x4382);
write_cmos_sensor(0x296a, 0x0b88);
write_cmos_sensor(0x296c, 0x4382);
write_cmos_sensor(0x296e, 0x0b8a);
write_cmos_sensor(0x2970, 0x4382);
write_cmos_sensor(0x2972, 0x0b8c);
write_cmos_sensor(0x2974, 0x40b2);
write_cmos_sensor(0x2976, 0x8200);
write_cmos_sensor(0x2978, 0x0b8e);
write_cmos_sensor(0x297a, 0x4382);
write_cmos_sensor(0x297c, 0x0ba6);
write_cmos_sensor(0x297e, 0x43c2);
write_cmos_sensor(0x2980, 0x01a0);
write_cmos_sensor(0x2982, 0x43c2);
write_cmos_sensor(0x2984, 0x01a2);
write_cmos_sensor(0x2986, 0x43c2);
write_cmos_sensor(0x2988, 0x019d);
write_cmos_sensor(0x298a, 0x40f2);
write_cmos_sensor(0x298c, 0x000a);
write_cmos_sensor(0x298e, 0x0f90);
write_cmos_sensor(0x2990, 0x43c2);
write_cmos_sensor(0x2992, 0x0f82);
write_cmos_sensor(0x2994, 0x43c2);
write_cmos_sensor(0x2996, 0x003d);
write_cmos_sensor(0x2998, 0x4030);
write_cmos_sensor(0x299a, 0xf1a2);
write_cmos_sensor(0x299c, 0x5031);
write_cmos_sensor(0x299e, 0x0032);
write_cmos_sensor(0x29a0, 0x4030);
write_cmos_sensor(0x29a2, 0xfb6a);
write_cmos_sensor(0x29a4, 0x120b);
write_cmos_sensor(0x29a6, 0x120a);
write_cmos_sensor(0x29a8, 0x1209);
write_cmos_sensor(0x29aa, 0x1208);
write_cmos_sensor(0x29ac, 0x1207);
write_cmos_sensor(0x29ae, 0x1206);
write_cmos_sensor(0x29b0, 0x1205);
write_cmos_sensor(0x29b2, 0x1204);
write_cmos_sensor(0x29b4, 0x8321);
write_cmos_sensor(0x29b6, 0x4039);
write_cmos_sensor(0x29b8, 0x0014);
write_cmos_sensor(0x29ba, 0x5109);
write_cmos_sensor(0x29bc, 0x4d07);
write_cmos_sensor(0x29be, 0x4c0d);
write_cmos_sensor(0x29c0, 0x4926);
write_cmos_sensor(0x29c2, 0x4991);
write_cmos_sensor(0x29c4, 0x0002);
write_cmos_sensor(0x29c6, 0x0000);
write_cmos_sensor(0x29c8, 0x4915);
write_cmos_sensor(0x29ca, 0x0004);
write_cmos_sensor(0x29cc, 0x4954);
write_cmos_sensor(0x29ce, 0x0006);
write_cmos_sensor(0x29d0, 0x4f0b);
write_cmos_sensor(0x29d2, 0x430a);
write_cmos_sensor(0x29d4, 0x4e08);
write_cmos_sensor(0x29d6, 0x4309);
write_cmos_sensor(0x29d8, 0xda08);
write_cmos_sensor(0x29da, 0xdb09);
write_cmos_sensor(0x29dc, 0x570d);
write_cmos_sensor(0x29de, 0x432e);
write_cmos_sensor(0x29e0, 0x421f);
write_cmos_sensor(0x29e2, 0x0a86);
write_cmos_sensor(0x29e4, 0x821e);
write_cmos_sensor(0x29e6, 0x81e0);
write_cmos_sensor(0x29e8, 0x930e);
write_cmos_sensor(0x29ea, 0x2403);
write_cmos_sensor(0x29ec, 0x5f0f);
write_cmos_sensor(0x29ee, 0x831e);
write_cmos_sensor(0x29f0, 0x23fd);
write_cmos_sensor(0x29f2, 0x8f0d);
write_cmos_sensor(0x29f4, 0x425f);
write_cmos_sensor(0x29f6, 0x0ce1);
write_cmos_sensor(0x29f8, 0xf37f);
write_cmos_sensor(0x29fa, 0x421e);
write_cmos_sensor(0x29fc, 0x00ba);
write_cmos_sensor(0x29fe, 0x4f0a);
write_cmos_sensor(0x2a00, 0x4e0c);
write_cmos_sensor(0x2a02, 0x12b0);
write_cmos_sensor(0x2a04, 0xfb6e);
write_cmos_sensor(0x2a06, 0x4e0f);
write_cmos_sensor(0x2a08, 0x108f);
write_cmos_sensor(0x2a0a, 0x4f47);
write_cmos_sensor(0x2a0c, 0xc312);
write_cmos_sensor(0x2a0e, 0x1007);
write_cmos_sensor(0x2a10, 0x5808);
write_cmos_sensor(0x2a12, 0x6909);
write_cmos_sensor(0x2a14, 0x5808);
write_cmos_sensor(0x2a16, 0x6909);
write_cmos_sensor(0x2a18, 0x5808);
write_cmos_sensor(0x2a1a, 0x6909);
write_cmos_sensor(0x2a1c, 0x5808);
write_cmos_sensor(0x2a1e, 0x6909);
write_cmos_sensor(0x2a20, 0x5808);
write_cmos_sensor(0x2a22, 0x6909);
write_cmos_sensor(0x2a24, 0x5808);
write_cmos_sensor(0x2a26, 0x6909);
write_cmos_sensor(0x2a28, 0x4d0e);
write_cmos_sensor(0x2a2a, 0x430f);
write_cmos_sensor(0x2a2c, 0x480c);
write_cmos_sensor(0x2a2e, 0x490d);
write_cmos_sensor(0x2a30, 0x4e0a);
write_cmos_sensor(0x2a32, 0x4f0b);
write_cmos_sensor(0x2a34, 0x12b0);
write_cmos_sensor(0x2a36, 0xfbc4);
write_cmos_sensor(0x2a38, 0x5c07);
write_cmos_sensor(0x2a3a, 0x474e);
write_cmos_sensor(0x2a3c, 0xf07e);
write_cmos_sensor(0x2a3e, 0x003f);
write_cmos_sensor(0x2a40, 0x93c2);
write_cmos_sensor(0x2a42, 0x81e8);
write_cmos_sensor(0x2a44, 0x200f);
write_cmos_sensor(0x2a46, 0x9344);
write_cmos_sensor(0x2a48, 0x200d);
write_cmos_sensor(0x2a4a, 0x470c);
write_cmos_sensor(0x2a4c, 0x430d);
write_cmos_sensor(0x2a4e, 0x462e);
write_cmos_sensor(0x2a50, 0x430f);
write_cmos_sensor(0x2a52, 0x5e0c);
write_cmos_sensor(0x2a54, 0x6f0d);
write_cmos_sensor(0x2a56, 0xc312);
write_cmos_sensor(0x2a58, 0x100d);
write_cmos_sensor(0x2a5a, 0x100c);
write_cmos_sensor(0x2a5c, 0x4c07);
write_cmos_sensor(0x2a5e, 0x4c4e);
write_cmos_sensor(0x2a60, 0xf07e);
write_cmos_sensor(0x2a62, 0x003f);
write_cmos_sensor(0x2a64, 0x4786);
write_cmos_sensor(0x2a66, 0x0000);
write_cmos_sensor(0x2a68, 0xb0f2);
write_cmos_sensor(0x2a6a, 0x0010);
write_cmos_sensor(0x2a6c, 0x0c83);
write_cmos_sensor(0x2a6e, 0x2409);
write_cmos_sensor(0x2a70, 0x4e4f);
write_cmos_sensor(0x2a72, 0x5f0f);
write_cmos_sensor(0x2a74, 0x5f0f);
write_cmos_sensor(0x2a76, 0x5f0f);
write_cmos_sensor(0x2a78, 0x5f0f);
write_cmos_sensor(0x2a7a, 0x5f0f);
write_cmos_sensor(0x2a7c, 0x4f85);
write_cmos_sensor(0x2a7e, 0x0000);
write_cmos_sensor(0x2a80, 0x3c02);
write_cmos_sensor(0x2a82, 0x4385);
write_cmos_sensor(0x2a84, 0x0000);
write_cmos_sensor(0x2a86, 0x412f);
write_cmos_sensor(0x2a88, 0x478f);
write_cmos_sensor(0x2a8a, 0x0000);
write_cmos_sensor(0x2a8c, 0x5321);
write_cmos_sensor(0x2a8e, 0x4134);
write_cmos_sensor(0x2a90, 0x4135);
write_cmos_sensor(0x2a92, 0x4136);
write_cmos_sensor(0x2a94, 0x4137);
write_cmos_sensor(0x2a96, 0x4138);
write_cmos_sensor(0x2a98, 0x4139);
write_cmos_sensor(0x2a9a, 0x413a);
write_cmos_sensor(0x2a9c, 0x413b);
write_cmos_sensor(0x2a9e, 0x4130);
write_cmos_sensor(0x2aa0, 0x4f4e);
write_cmos_sensor(0x2aa2, 0x421f);
write_cmos_sensor(0x2aa4, 0x7316);
write_cmos_sensor(0x2aa6, 0xc312);
write_cmos_sensor(0x2aa8, 0x100f);
write_cmos_sensor(0x2aaa, 0x821f);
write_cmos_sensor(0x2aac, 0x81e6);
write_cmos_sensor(0x2aae, 0x4f82);
write_cmos_sensor(0x2ab0, 0x7334);
write_cmos_sensor(0x2ab2, 0x0f00);
write_cmos_sensor(0x2ab4, 0x7302);
write_cmos_sensor(0x2ab6, 0xb0b2);
write_cmos_sensor(0x2ab8, 0x000f);
write_cmos_sensor(0x2aba, 0x7300);
write_cmos_sensor(0x2abc, 0x200e);
write_cmos_sensor(0x2abe, 0x403f);
write_cmos_sensor(0x2ac0, 0x0cd8);
write_cmos_sensor(0x2ac2, 0x43df);
write_cmos_sensor(0x2ac4, 0x0000);
write_cmos_sensor(0x2ac6, 0x43cf);
write_cmos_sensor(0x2ac8, 0x0000);
write_cmos_sensor(0x2aca, 0x4ec2);
write_cmos_sensor(0x2acc, 0x0c5a);
write_cmos_sensor(0x2ace, 0x4ec2);
write_cmos_sensor(0x2ad0, 0x0c5c);
write_cmos_sensor(0x2ad2, 0x4ec2);
write_cmos_sensor(0x2ad4, 0x0c5e);
write_cmos_sensor(0x2ad6, 0x4ec2);
write_cmos_sensor(0x2ad8, 0x0c60);
write_cmos_sensor(0x2ada, 0x421f);
write_cmos_sensor(0x2adc, 0x7112);
write_cmos_sensor(0x2ade, 0x93a2);
write_cmos_sensor(0x2ae0, 0x7114);
write_cmos_sensor(0x2ae2, 0x2408);
write_cmos_sensor(0x2ae4, 0x9382);
write_cmos_sensor(0x2ae6, 0x7112);
write_cmos_sensor(0x2ae8, 0x2403);
write_cmos_sensor(0x2aea, 0x5292);
write_cmos_sensor(0x2aec, 0x81dc);
write_cmos_sensor(0x2aee, 0x7114);
write_cmos_sensor(0x2af0, 0x430f);
write_cmos_sensor(0x2af2, 0x4130);
write_cmos_sensor(0x2af4, 0xf31f);
write_cmos_sensor(0x2af6, 0x27f6);
write_cmos_sensor(0x2af8, 0x4382);
write_cmos_sensor(0x2afa, 0x7f10);
write_cmos_sensor(0x2afc, 0x40b2);
write_cmos_sensor(0x2afe, 0x0003);
write_cmos_sensor(0x2b00, 0x7114);
write_cmos_sensor(0x2b02, 0x40b2);
write_cmos_sensor(0x2b04, 0x000a);
write_cmos_sensor(0x2b06, 0x7334);
write_cmos_sensor(0x2b08, 0x0f00);
write_cmos_sensor(0x2b0a, 0x7302);
write_cmos_sensor(0x2b0c, 0x4392);
write_cmos_sensor(0x2b0e, 0x7708);
write_cmos_sensor(0x2b10, 0x4382);
write_cmos_sensor(0x2b12, 0x770e);
write_cmos_sensor(0x2b14, 0x431f);
write_cmos_sensor(0x2b16, 0x4130);
write_cmos_sensor(0x2b18, 0x0260);
	write_cmos_sensor(0x2b1a, 0x0000);
write_cmos_sensor(0x2b1c, 0x0c64);
write_cmos_sensor(0x2b1e, 0x0c64);
write_cmos_sensor(0x2b20, 0x0240);
	write_cmos_sensor(0x2b22, 0x0000);
	write_cmos_sensor(0x2b24, 0x0260);
	write_cmos_sensor(0x2b26, 0x0000);
write_cmos_sensor(0x2b28, 0x0c1e);
write_cmos_sensor(0x2b2a, 0x4130);
write_cmos_sensor(0x2b2c, 0x4f0e);
write_cmos_sensor(0x2b2e, 0xc312);
write_cmos_sensor(0x2b30, 0x100f);
write_cmos_sensor(0x2b32, 0x110f);
write_cmos_sensor(0x2b34, 0xc312);
write_cmos_sensor(0x2b36, 0x100f);
write_cmos_sensor(0x2b38, 0x4f82);
write_cmos_sensor(0x2b3a, 0x7600);
write_cmos_sensor(0x2b3c, 0xf03e);
write_cmos_sensor(0x2b3e, 0x0007);
write_cmos_sensor(0x2b40, 0x4e82);
write_cmos_sensor(0x2b42, 0x7602);
write_cmos_sensor(0x2b44, 0x0262);
write_cmos_sensor(0x2b46, 0x0000);
write_cmos_sensor(0x2b48, 0x0222);
write_cmos_sensor(0x2b4a, 0x0000);
write_cmos_sensor(0x2b4c, 0x0262);
write_cmos_sensor(0x2b4e, 0x0000);
write_cmos_sensor(0x2b50, 0x0260);
write_cmos_sensor(0x2b52, 0x0000);
write_cmos_sensor(0x2b54, 0x4130);
write_cmos_sensor(0x2b56, 0x4f82);
write_cmos_sensor(0x2b58, 0x7600);
write_cmos_sensor(0x2b5a, 0x0270);
write_cmos_sensor(0x2b5c, 0x0000);
write_cmos_sensor(0x2b5e, 0x0c1c);
write_cmos_sensor(0x2b60, 0x0270);
write_cmos_sensor(0x2b62, 0x0001);
write_cmos_sensor(0x2b64, 0x421f);
write_cmos_sensor(0x2b66, 0x7606);
write_cmos_sensor(0x2b68, 0x4130);
write_cmos_sensor(0x2b6a, 0xdf02);
write_cmos_sensor(0x2b6c, 0x3ffe);
write_cmos_sensor(0x2b6e, 0x430e);
write_cmos_sensor(0x2b70, 0x930a);
write_cmos_sensor(0x2b72, 0x2407);
write_cmos_sensor(0x2b74, 0xc312);
write_cmos_sensor(0x2b76, 0x100c);
write_cmos_sensor(0x2b78, 0x2801);
write_cmos_sensor(0x2b7a, 0x5a0e);
write_cmos_sensor(0x2b7c, 0x5a0a);
write_cmos_sensor(0x2b7e, 0x930c);
write_cmos_sensor(0x2b80, 0x23f7);
write_cmos_sensor(0x2b82, 0x4130);
write_cmos_sensor(0x2b84, 0x430e);
write_cmos_sensor(0x2b86, 0x430f);
write_cmos_sensor(0x2b88, 0x3c08);
write_cmos_sensor(0x2b8a, 0xc312);
write_cmos_sensor(0x2b8c, 0x100d);
write_cmos_sensor(0x2b8e, 0x100c);
write_cmos_sensor(0x2b90, 0x2802);
write_cmos_sensor(0x2b92, 0x5a0e);
write_cmos_sensor(0x2b94, 0x6b0f);
write_cmos_sensor(0x2b96, 0x5a0a);
write_cmos_sensor(0x2b98, 0x6b0b);
write_cmos_sensor(0x2b9a, 0x930c);
write_cmos_sensor(0x2b9c, 0x23f6);
write_cmos_sensor(0x2b9e, 0x930d);
write_cmos_sensor(0x2ba0, 0x23f4);
write_cmos_sensor(0x2ba2, 0x4130);
write_cmos_sensor(0x2ba4, 0x4030);
write_cmos_sensor(0x2ba6, 0xfb84);
write_cmos_sensor(0x2ba8, 0xee0e);
write_cmos_sensor(0x2baa, 0x403b);
write_cmos_sensor(0x2bac, 0x0011);
write_cmos_sensor(0x2bae, 0x3c05);
write_cmos_sensor(0x2bb0, 0x100d);
write_cmos_sensor(0x2bb2, 0x6e0e);
write_cmos_sensor(0x2bb4, 0x9a0e);
write_cmos_sensor(0x2bb6, 0x2801);
write_cmos_sensor(0x2bb8, 0x8a0e);
write_cmos_sensor(0x2bba, 0x6c0c);
write_cmos_sensor(0x2bbc, 0x6d0d);
write_cmos_sensor(0x2bbe, 0x831b);
write_cmos_sensor(0x2bc0, 0x23f7);
	write_cmos_sensor(0x2bc2, 0x4130);
write_cmos_sensor(0x2bc4, 0xef0f);
write_cmos_sensor(0x2bc6, 0xee0e);
write_cmos_sensor(0x2bc8, 0x4039);
write_cmos_sensor(0x2bca, 0x0021);
write_cmos_sensor(0x2bcc, 0x3c0a);
write_cmos_sensor(0x2bce, 0x1008);
write_cmos_sensor(0x2bd0, 0x6e0e);
write_cmos_sensor(0x2bd2, 0x6f0f);
write_cmos_sensor(0x2bd4, 0x9b0f);
write_cmos_sensor(0x2bd6, 0x2805);
write_cmos_sensor(0x2bd8, 0x2002);
write_cmos_sensor(0x2bda, 0x9a0e);
write_cmos_sensor(0x2bdc, 0x2802);
write_cmos_sensor(0x2bde, 0x8a0e);
write_cmos_sensor(0x2be0, 0x7b0f);
write_cmos_sensor(0x2be2, 0x6c0c);
write_cmos_sensor(0x2be4, 0x6d0d);
write_cmos_sensor(0x2be6, 0x6808);
write_cmos_sensor(0x2be8, 0x8319);
write_cmos_sensor(0x2bea, 0x23f1);
write_cmos_sensor(0x2bec, 0x4130);
write_cmos_sensor(0x2bee, 0x4130);
	write_cmos_sensor(0x2ffe, 0xf14c);
	write_cmos_sensor(0x3000, 0x01fc);
	write_cmos_sensor(0x3002, 0x320f);
	write_cmos_sensor(0x3004, 0x0001);
	write_cmos_sensor(0x3006, 0x01fc);
	write_cmos_sensor(0x3008, 0x320f);
	write_cmos_sensor(0x300a, 0x0001);
	write_cmos_sensor(0x300c, 0x01fc);
	write_cmos_sensor(0x300e, 0x320f);
	write_cmos_sensor(0x3010, 0x0001);
	write_cmos_sensor(0x4000, 0x2500);
	write_cmos_sensor(0x4002, 0xfc2c);
	write_cmos_sensor(0x4004, 0xc004);
	write_cmos_sensor(0x4006, 0x2500);
	write_cmos_sensor(0x4008, 0xfc2c);
	write_cmos_sensor(0x400a, 0xc004);
	write_cmos_sensor(0x400c, 0x2500);
	write_cmos_sensor(0x400e, 0xfc2c);
	write_cmos_sensor(0x4010, 0xc004);
	write_cmos_sensor(0x0a00, 0x0000); //stream off
	write_cmos_sensor(0x0b00, 0x0000); 
	write_cmos_sensor(0x0b02, 0x9887); 
	write_cmos_sensor(0x0b04, 0xc540); 
	write_cmos_sensor(0x0b06, 0xb540); 
	write_cmos_sensor(0x0b08, 0x0085); 
	write_cmos_sensor(0x0b0a, 0xd304); 
	write_cmos_sensor(0x0b0c, 0x0420); 
	write_cmos_sensor(0x0b0e, 0xc200); 
	write_cmos_sensor(0x0b10, 0xac20); 
	write_cmos_sensor(0x0b12, 0x0000); 
write_cmos_sensor(0x0b14, 0x402c);
	write_cmos_sensor(0x0b16, 0x6e0b); 
	write_cmos_sensor(0x0b18, 0xf20b); 
	write_cmos_sensor(0x0b1a, 0x0000); 
	write_cmos_sensor(0x0b1c, 0x0000); 
	write_cmos_sensor(0x0b1e, 0x0081); 
	write_cmos_sensor(0x0b20, 0x0000); 
write_cmos_sensor(0x0b22, 0xeb80);
write_cmos_sensor(0x0b24, 0x4500);
	write_cmos_sensor(0x0b26, 0x0001); 
	write_cmos_sensor(0x0b28, 0x0807); 
	write_cmos_sensor(0x0c00, 0x1190); //blc enable
	write_cmos_sensor(0x0c02, 0x0011); 
	write_cmos_sensor(0x0c04, 0x0000); 
	write_cmos_sensor(0x0c06, 0x0200); 
	write_cmos_sensor(0x0c10, 0x0040);
	write_cmos_sensor(0x0c12, 0x0040);
	write_cmos_sensor(0x0c14, 0x0040);
	write_cmos_sensor(0x0c16, 0x0040);
	write_cmos_sensor(0x0a10, 0x4000);
	write_cmos_sensor(0x0c18, 0x80ff); 
	write_cmos_sensor(0x0c60, 0x0600); 
	write_cmos_sensor(0x0000, 0x0100); //image orient
	write_cmos_sensor(0x0f0a, 0x0000);
	write_cmos_sensor(0x0e0a, 0x0001);
	write_cmos_sensor(0x004a, 0x0100);
	write_cmos_sensor(0x000c, 0x0022);
	write_cmos_sensor(0x0008, 0x0b00); //line length pck
	write_cmos_sensor(0x000a, 0x0f00);
	write_cmos_sensor(0x005a, 0x0202);
write_cmos_sensor(0x0012, 0x001e);
write_cmos_sensor(0x0018, 0x0a21);
	write_cmos_sensor(0x0034, 0x0700);
	write_cmos_sensor(0x0022, 0x0008);
	write_cmos_sensor(0x0028, 0x0017);
write_cmos_sensor(0x0024, 0x0044);
write_cmos_sensor(0x002a, 0x0049);
write_cmos_sensor(0x0026, 0x004c);
write_cmos_sensor(0x002c, 0x07cb);
	write_cmos_sensor(0x002e, 0x1111);
	write_cmos_sensor(0x0030, 0x1111);
	write_cmos_sensor(0x0032, 0x1111);
	write_cmos_sensor(0x001a, 0x1111);
	write_cmos_sensor(0x001c, 0x1111);
	write_cmos_sensor(0x001e, 0x1111);
write_cmos_sensor(0x0006, 0x0823); //frame length lines
	write_cmos_sensor(0x0a22, 0x0000);
write_cmos_sensor(0x0a12, 0x0a00); //x output size
write_cmos_sensor(0x0a14, 0x0780); //y output size
	write_cmos_sensor(0x003e, 0x0000);
write_cmos_sensor(0x0004, 0x081d); //coarse int. time
	write_cmos_sensor(0x0052, 0x019c);
	write_cmos_sensor(0x0002, 0x0000);
	write_cmos_sensor(0x0a02, 0x0100);
	write_cmos_sensor(0x0a04, 0x014a); //isp_en
	write_cmos_sensor(0x0508, 0x0100);
	write_cmos_sensor(0x0046, 0x0000);
	write_cmos_sensor(0x003a, 0x0000); //analog gain 1x
	write_cmos_sensor(0x0036, 0x0070);
	write_cmos_sensor(0x0038, 0x7000);
	write_cmos_sensor(0x004c, 0x7070);
	write_cmos_sensor(0x0122, 0x0301);
	write_cmos_sensor(0x0804, 0x0002);
	write_cmos_sensor(0x004e, 0x0300);
	write_cmos_sensor(0x0a1a, 0x0800);
	write_cmos_sensor(0x0126, 0x0100); //Dgain_gr
	write_cmos_sensor(0x0128, 0x0100); //Dgain_gb
	write_cmos_sensor(0x012a, 0x0100); //Dgain_r
	write_cmos_sensor(0x012c, 0x0100); //Dgain_b
	write_cmos_sensor(0x012e, 0x0203);
	write_cmos_sensor(0x0130, 0x0600);
write_cmos_sensor(0x090c, 0x0fdc);
write_cmos_sensor(0x090e, 0x0041);
//===============================================
//             mipi 2 lane 880Mbps             
//===============================================
	write_cmos_sensor(0x0902, 0x431a);
write_cmos_sensor(0x0914, 0xc10c);
write_cmos_sensor(0x0916, 0x061d);
	write_cmos_sensor(0x091c, 0x0e09);
	write_cmos_sensor(0x0918, 0x0307);
	write_cmos_sensor(0x091a, 0x0c0c);
	write_cmos_sensor(0x091e, 0x0a00);
	write_cmos_sensor(0x0a00, 0x0100); //stream on

}	/*	sensor_init  */
#endif

static void preview_setting(void)
{
	LOG_INF("E\n");




//Sensor Information////////////////////////////
//Sensor	  : Hi-553
//Date		  : 2015-09-05
//Customer        : LGE
//Image size	  : 1296x972
//MCLK		  : 24MHz
//MIPI speed(Mbps): 440Mbps x 2Lane
//Frame Length	  : 2049
//Line Length 	  : 2816
//Max Fps 	  : 30.5fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offset	  : 64code
////////////////////////////////////////////////

write_cmos_sensor(0x0a00, 0x0000); //stream on
write_cmos_sensor(0x0b14, 0x402c);
write_cmos_sensor(0x0b16, 0x6e0b);
write_cmos_sensor(0x0b18, 0xf24b);
write_cmos_sensor(0x004a, 0x0100);
write_cmos_sensor(0x000c, 0x0022);
write_cmos_sensor(0x0008, 0x0b00);
write_cmos_sensor(0x005a, 0x0204);
write_cmos_sensor(0x0012, 0x001e);
write_cmos_sensor(0x0018, 0x0a21);
write_cmos_sensor(0x0024, 0x0044);
write_cmos_sensor(0x002a, 0x0049);
write_cmos_sensor(0x0026, 0x004c);
write_cmos_sensor(0x002c, 0x07cb);
write_cmos_sensor(0x002e, 0x1111);
write_cmos_sensor(0x0030, 0x1111);
write_cmos_sensor(0x0032, 0x3311);
write_cmos_sensor(0x001a, 0x1111);
write_cmos_sensor(0x001c, 0x1111);
write_cmos_sensor(0x001e, 0x1111);
write_cmos_sensor(0x0006, 0x0823); //frame length lines
write_cmos_sensor(0x0a22, 0x0000);
write_cmos_sensor(0x0a12, 0x0500); //x output size
write_cmos_sensor(0x0a14, 0x03c0); //y output size
write_cmos_sensor(0x003e, 0x0000);
write_cmos_sensor(0x0a04, 0x016a); //isp_en
write_cmos_sensor(0x0a1a, 0x0800);
write_cmos_sensor(0x090c, 0x0514);
write_cmos_sensor(0x090e, 0x0010);
//===============================================
//             mipi 2 lane 440Mbps
//===============================================
write_cmos_sensor(0x0902, 0x431a);
write_cmos_sensor(0x0914, 0xc106);
write_cmos_sensor(0x091c, 0x0e05);
write_cmos_sensor(0x0916, 0x040e);
write_cmos_sensor(0x0918, 0x0304);
write_cmos_sensor(0x091a, 0x0708);
write_cmos_sensor(0x091e, 0x0300);
write_cmos_sensor(0x0a00, 0x0100); //stream on



}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	if (currefps == 300) {
//Sensor Information////////////////////////////
//Sensor	  : Hi-553
//Date		  : 2015-09-05
//Customer        : LGE
//Image size	  : 2592x1944
//MCLK		  : 24MHz
//MIPI speed(Mbps): 880Mbps x 2Lane
//Frame Length	  : 2049
//Line Length 	  : 2816
//Max Fps 	  : 30.5fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offset	  : 64code
////////////////////////////////////////////////

write_cmos_sensor(0x0a00, 0x0000); //stream on         
write_cmos_sensor(0x0b14, 0x402c);
write_cmos_sensor(0x0b16, 0x6e0b);                     
write_cmos_sensor(0x0b18, 0xf20b);                     
write_cmos_sensor(0x004a, 0x0100);                     
write_cmos_sensor(0x000c, 0x0022);                     
write_cmos_sensor(0x0008, 0x0b00);                     
write_cmos_sensor(0x005a, 0x0202);                     
write_cmos_sensor(0x0012, 0x001e);
write_cmos_sensor(0x0018, 0x0a21);
write_cmos_sensor(0x0024, 0x0044);
write_cmos_sensor(0x002a, 0x0049);
write_cmos_sensor(0x0026, 0x004c);
write_cmos_sensor(0x002c, 0x07cb);
write_cmos_sensor(0x002e, 0x1111);                     
write_cmos_sensor(0x0030, 0x1111);                     
write_cmos_sensor(0x0032, 0x1111);                     
write_cmos_sensor(0x001a, 0x1111);                     
write_cmos_sensor(0x001c, 0x1111);                     
write_cmos_sensor(0x001e, 0x1111);                     
write_cmos_sensor(0x0006, 0x0823); //frame length lines
write_cmos_sensor(0x0a22, 0x0000);                     
write_cmos_sensor(0x0a12, 0x0a00); //x output size
write_cmos_sensor(0x0a14, 0x0780); //y output size
write_cmos_sensor(0x003e, 0x0000);                     
write_cmos_sensor(0x0a04, 0x014a); //isp_en            
write_cmos_sensor(0x0a1a, 0x0800);                     
write_cmos_sensor(0x090c, 0x0fdc);
write_cmos_sensor(0x090e, 0x0041);
//===============================================
//             mipi 2 lane 880Mbps             
//===============================================
write_cmos_sensor(0x0902, 0x431a);
write_cmos_sensor(0x0914, 0xc10c);
write_cmos_sensor(0x0916, 0x061d);
write_cmos_sensor(0x091c, 0x0e09);
write_cmos_sensor(0x0918, 0x0307);
write_cmos_sensor(0x091a, 0x0c0c);
write_cmos_sensor(0x091e, 0x0a00);
write_cmos_sensor(0x0a00, 0x0100); //stream on
	}
	//else if (currefps == 240) {}
	else if (currefps == 150) {
		write_cmos_sensor(0x0a00, 0x0000); //stream on
		write_cmos_sensor(0x0b14, 0x402c);
		write_cmos_sensor(0x0b16, 0x6e0b);
		write_cmos_sensor(0x0b18, 0xf20b);
		write_cmos_sensor(0x004a, 0x0100);
		write_cmos_sensor(0x000c, 0x0022);
		write_cmos_sensor(0x0008, 0x0b00);
		write_cmos_sensor(0x005a, 0x0202);
		write_cmos_sensor(0x0012, 0x001e);
		write_cmos_sensor(0x0018, 0x0a21);
		write_cmos_sensor(0x0024, 0x0044);
		write_cmos_sensor(0x002a, 0x0049);
		write_cmos_sensor(0x0026, 0x004c);
		write_cmos_sensor(0x002c, 0x07cb);
		write_cmos_sensor(0x002e, 0x1111);
		write_cmos_sensor(0x0030, 0x1111);
		write_cmos_sensor(0x0032, 0x1111);
		write_cmos_sensor(0x001a, 0x1111);
		write_cmos_sensor(0x001c, 0x1111);
		write_cmos_sensor(0x001e, 0x1111);
		write_cmos_sensor(0x0006, 0x1046); //frame length lines
		write_cmos_sensor(0x0a22, 0x0000);
		write_cmos_sensor(0x0a12, 0x0a00); //x output size
		write_cmos_sensor(0x0a14, 0x0780); //y output size
		write_cmos_sensor(0x003e, 0x0000);
		write_cmos_sensor(0x0a04, 0x014a); //isp_en
		write_cmos_sensor(0x0a1a, 0x0800);
		write_cmos_sensor(0x090c, 0x0fdc);
		write_cmos_sensor(0x090e, 0x0041);
		//===============================================
		//			   mipi 2 lane 880Mbps
		//===============================================
		write_cmos_sensor(0x0902, 0x431a);
		write_cmos_sensor(0x0914, 0xc10c);
		write_cmos_sensor(0x0916, 0x061d);
		write_cmos_sensor(0x091c, 0x0e09);
		write_cmos_sensor(0x0918, 0x0307);
		write_cmos_sensor(0x091a, 0x0c0c);
		write_cmos_sensor(0x091e, 0x0a00);
		write_cmos_sensor(0x0a00, 0x0100); //stream on
		}

	else {
//Sensor Information////////////////////////////
//Sensor	  : Hi-553
//Date		  : 2015-09-05
//Customer        : LGE
//Image size	  : 2592x1944
//MCLK		  : 24MHz
//MIPI speed(Mbps): 880Mbps x 2Lane
//Frame Length	  : 2049
//Line Length 	  : 2816
//Max Fps 	  : 30.5fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offset	  : 64code
////////////////////////////////////////////////

write_cmos_sensor(0x0a00, 0x0000); //stream on         
		write_cmos_sensor(0x0b14, 0x402c);
write_cmos_sensor(0x0b16, 0x6e0b);                     
write_cmos_sensor(0x0b18, 0xf20b);                     
write_cmos_sensor(0x004a, 0x0100);                     
write_cmos_sensor(0x000c, 0x0022);                     
write_cmos_sensor(0x0008, 0x0b00);                     
write_cmos_sensor(0x005a, 0x0202);                     
		write_cmos_sensor(0x0012, 0x001e);
		write_cmos_sensor(0x0018, 0x0a21);
		write_cmos_sensor(0x0024, 0x0044);
		write_cmos_sensor(0x002a, 0x0049);
		write_cmos_sensor(0x0026, 0x004c);
		write_cmos_sensor(0x002c, 0x07cb);
write_cmos_sensor(0x002e, 0x1111);                     
write_cmos_sensor(0x0030, 0x1111);                     
write_cmos_sensor(0x0032, 0x1111);                     
write_cmos_sensor(0x001a, 0x1111);                     
write_cmos_sensor(0x001c, 0x1111);                     
write_cmos_sensor(0x001e, 0x1111);                     
		write_cmos_sensor(0x0006, 0x0823); //frame length lines
write_cmos_sensor(0x0a22, 0x0000);                     
		write_cmos_sensor(0x0a12, 0x0a00); //x output size
		write_cmos_sensor(0x0a14, 0x0780); //y output size
write_cmos_sensor(0x003e, 0x0000);                     
write_cmos_sensor(0x0a04, 0x014a); //isp_en            
write_cmos_sensor(0x0a1a, 0x0800);                     
		write_cmos_sensor(0x090c, 0x0fdc);
		write_cmos_sensor(0x090e, 0x0041);
//===============================================
//             mipi 2 lane 880Mbps             
//===============================================
write_cmos_sensor(0x0902, 0x431a);
		write_cmos_sensor(0x0914, 0xc10c);
		write_cmos_sensor(0x0916, 0x061d);
write_cmos_sensor(0x091c, 0x0e09);
write_cmos_sensor(0x0918, 0x0307);
write_cmos_sensor(0x091a, 0x0c0c);
write_cmos_sensor(0x091e, 0x0a00);
write_cmos_sensor(0x0a00, 0x0100); //stream on
	}
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
 // capture_setting(currefps);
  	preview_setting();
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
////////////////////////////////////////////////

		write_cmos_sensor(0x0a00, 0x0000); //stream on
		write_cmos_sensor(0x0b14, 0x404c);
		write_cmos_sensor(0x0b16, 0x6e0b);
		write_cmos_sensor(0x0b18, 0xf28b);
		write_cmos_sensor(0x004a, 0x0100);
		write_cmos_sensor(0x000c, 0x0022);
		write_cmos_sensor(0x0008, 0x0b00);
		write_cmos_sensor(0x005a, 0x0208);
		write_cmos_sensor(0x0012, 0x001E);
		write_cmos_sensor(0x0018, 0x0a21);
		write_cmos_sensor(0x0024, 0x0044);
		write_cmos_sensor(0x002a, 0x0049);
		write_cmos_sensor(0x0026, 0x004c);
		write_cmos_sensor(0x002c, 0x07cb);
		write_cmos_sensor(0x002e, 0x1111);
		write_cmos_sensor(0x0030, 0x1111);
		write_cmos_sensor(0x0032, 0x7711);
		write_cmos_sensor(0x001a, 0x1111);
		write_cmos_sensor(0x001c, 0x1111);
		write_cmos_sensor(0x001e, 0x1111);
		write_cmos_sensor(0x0006, 0x0208); //frame length lines
		write_cmos_sensor(0x0a22, 0x0100);
		write_cmos_sensor(0x0a12, 0x0280); //x output size
		write_cmos_sensor(0x0a14, 0x01e0); //y output size
		write_cmos_sensor(0x003e, 0x0000);
		write_cmos_sensor(0x0004, 0x0202); //coarse int. time
		write_cmos_sensor(0x0a04, 0x016a); //isp_en
		write_cmos_sensor(0x0a1A, 0x0800);
		write_cmos_sensor(0x090c, 0x0270);
		write_cmos_sensor(0x090e, 0x0010);
	//===============================================
	//			   mipi 2 lane 220Mbps
	//===============================================
		write_cmos_sensor(0x0902, 0x431a);
		write_cmos_sensor(0x0914, 0xc103);
		write_cmos_sensor(0x0916, 0x0207);
		write_cmos_sensor(0x091c, 0x0903);
		write_cmos_sensor(0x0918, 0x0302);
		write_cmos_sensor(0x091a, 0x0406);
		write_cmos_sensor(0x091e, 0x0300);
		write_cmos_sensor(0x0a00, 0x0100); //stream on

}


static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}


static kal_uint32 return_sensor_id(void)
{
    return read_cmos_sensor(0x0f16);
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
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
#ifdef CONFIG_MTK_CAM_CAL
		    //read_imx135_otp_mtk_fmt();
#endif
			hardwareinfo_set_prop(HARDWARE_BACK_CAM_ID, "jinkang");
		    LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);
		    return ERROR_NONE;
	    }
	    LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);	
	    retry--;
	} while(retry > 0);
        i++;
        retry = 1;
    }
	
sprintf(hi553_factory_module_id, "main_camera:5M-Camera hi553-ZhengQiao\n");
    if (*sensor_id != imgsensor_info.sensor_id) {
        // if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
        *sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    } else if(hi553_info_limit < 1){
#if 0	
		get_device_info(hi553_factory_module_id);
		get_camera_info(hi553_factory_module_id);
#endif
		
	}
	hi553_info_limit++;
	
    return ERROR_NONE;
}

#ifdef Hi553_I2C_BURST   //20151228 paul add 
static struct Hynix_Sensor_reg hi842_init_setting[] = 
{
//Sensor Information////////////////////////////
//Sensor	  : Hi-553
//Image size	  : 2560x1920
//MCLK		  : 24MHz
//MIPI speed(Mbps): 880Mbps x 2Lane
//Frame Length	  : 2083
//Line Length 	  : 2816
//Max Fps	  : 30fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offset	  : 64code
////////////////////////////////////////////////
	{0x0e00, 0x0102},
	{0x0e02, 0x0102},
	{0x0e0c, 0x0100},
	{0x2000, 0x7400},
	{0x2002, 0x1303},
	{0x2004, 0x7006},
	{0x2006, 0x1303},
	{0x2008, 0x0bd2},
	{0x200a, 0x5001},
	{0x200c, 0x7000},
	{0x200e, 0x16c6},
	{0x2010, 0x0044},
	{0x2012, 0x0307},
	{0x2014, 0x0008},
	{0x2016, 0x00c4},
	{0x2018, 0x0009},
	{0x201a, 0x207e},
	{0x201c, 0x7001},
	{0x201e, 0x0fd0},
	{0x2020, 0x00d1},
	{0x2022, 0x00a1},
	{0x2024, 0x00c9},
	{0x2026, 0x030b},
	{0x2028, 0x7008},
	{0x202a, 0x81c0},
	{0x202c, 0x01d7},
	{0x202e, 0x0022},
	{0x2030, 0x0016},
	{0x2032, 0x7006},
	{0x2034, 0x0fe2},
	{0x2036, 0x0016},
	{0x2038, 0x20d0},
	{0x203a, 0x15c8},
	{0x203c, 0x0007},
	{0x203e, 0x0000},
	{0x2040, 0x0540},
	{0x2042, 0x0007},
	{0x2044, 0x0008},
	{0x2046, 0x0ec5},
	{0x2048, 0x00cc},
	{0x204a, 0x00c5},
	{0x204c, 0x00cc},
	{0x204e, 0x04d7},
	{0x2050, 0x18e2},
	{0x2052, 0x0016},
	{0x2054, 0x70e1},
	{0x2056, 0x18e2},
	{0x2058, 0x16d6},
	{0x205a, 0x0022},
	{0x205c, 0x8251},
	{0x205e, 0x2128},
	{0x2060, 0x2100},
	{0x2062, 0x702f},
	{0x2064, 0x2f06},
	{0x2066, 0x2300},
	{0x2068, 0x7800},
	{0x206a, 0x7400},
	{0x206c, 0x1303},
	{0x206e, 0x7006},
	{0x2070, 0x1303},
	{0x2072, 0x0bd2},
	{0x2074, 0x5061},
	{0x2076, 0x7000},
	{0x2078, 0x16c6},
	{0x207a, 0x0044},
	{0x207c, 0x0307},
	{0x207e, 0x0008},
	{0x2080, 0x00c4},
	{0x2082, 0x0009},
	{0x2084, 0x207e},
	{0x2086, 0x7001},
	{0x2088, 0x0fd0},
	{0x208a, 0x00d1},
	{0x208c, 0x00a1},
	{0x208e, 0x00c9},
	{0x2090, 0x030b},
	{0x2092, 0x7008},
	{0x2094, 0x84c0},
	{0x2096, 0x01d7},
	{0x2098, 0x0022},
	{0x209a, 0x0016},
	{0x209c, 0x7006},
	{0x209e, 0x0fe2},
	{0x20a0, 0x0016},
	{0x20a2, 0x20d0},
	{0x20a4, 0x15c8},
	{0x20a6, 0x0007},
	{0x20a8, 0x0000},
	{0x20aa, 0x0540},
	{0x20ac, 0x0007},
	{0x20ae, 0x0008},
	{0x20b0, 0x0ec5},
	{0x20b2, 0x00cc},
	{0x20b4, 0x00c5},
	{0x20b6, 0x00cc},
	{0x20b8, 0x04d7},
	{0x20ba, 0x18e2},
	{0x20bc, 0x0016},
	{0x20be, 0x70e1},
	{0x20c0, 0x18e2},
	{0x20c2, 0x16d6},
	{0x20c4, 0x0022},
	{0x20c6, 0x8551},
	{0x20c8, 0x2128},
	{0x20ca, 0x2100},
	{0x20cc, 0x2006},
	{0x20ce, 0x2300},
	{0x20d0, 0x7800},
	{0x20d2, 0x7400},
	{0x20d4, 0x0005},
	{0x20d6, 0x7002},
	{0x20d8, 0x1303},
	{0x20da, 0x00c5},
	{0x20dc, 0x7002},
	{0x20de, 0x1303},
	{0x20e0, 0x0bd2},
	{0x20e2, 0x50c1},
	{0x20e4, 0x7000},
	{0x20e6, 0x16c6},
	{0x20e8, 0x0044},
	{0x20ea, 0x0307},
	{0x20ec, 0x0008},
	{0x20ee, 0x00c4},
	{0x20f0, 0x0009},
	{0x20f2, 0x207e},
	{0x20f4, 0x7001},
	{0x20f6, 0x0fd0},
	{0x20f8, 0x00d1},
	{0x20fa, 0x00a1},
	{0x20fc, 0x00c9},
	{0x20fe, 0x030b},
	{0x2100, 0x7008},
	{0x2102, 0x87c0},
	{0x2104, 0x01d7},
	{0x2106, 0x0022},
	{0x2108, 0x0016},
	{0x210a, 0x08a2},
	{0x210c, 0x0016},
	{0x210e, 0x20d0},
	{0x2110, 0x15c8},
	{0x2112, 0x0007},
	{0x2114, 0x0000},
	{0x2116, 0x0540},
	{0x2118, 0x0007},
	{0x211a, 0x0008},
	{0x211c, 0x0d45},
	{0x211e, 0x00cc},
	{0x2120, 0x00c5},
	{0x2122, 0x00cc},
	{0x2124, 0x04d7},
	{0x2126, 0x18e2},
	{0x2128, 0x0016},
	{0x212a, 0x706f},
	{0x212c, 0x18e2},
	{0x212e, 0x16d6},
	{0x2130, 0x0022},
	{0x2132, 0x8857},
	{0x2134, 0x2128},
	{0x2136, 0x2100},
	{0x2138, 0x7800},
	{0x213a, 0x3100},
	{0x213c, 0x01c6},
	{0x213e, 0x01c4},
	{0x2140, 0x01c0},
	{0x2142, 0x01c6},
	{0x2144, 0x2700},
	{0x2146, 0x7007},
	{0x2148, 0x3f00},
	{0x214a, 0x7800},
	{0x214c, 0x4031},
	{0x214e, 0x83ce},
	{0x2150, 0x4347},
	{0x2152, 0x40f2},
	{0x2154, 0x0006},
	{0x2156, 0x81fe},
	{0x2158, 0x40f2},
	{0x215a, 0x0042},
	{0x215c, 0x81ff},
	{0x215e, 0x40f2},
	{0x2160, 0xff84},
	{0x2162, 0x8200},
	{0x2164, 0x43c2},
	{0x2166, 0x8201},
	{0x2168, 0x40b2},
	{0x216a, 0x0230},
	{0x216c, 0x8030},
	{0x216e, 0x40b2},
	{0x2170, 0x0032},
	{0x2172, 0x8032},
	{0x2174, 0x40b2},
	{0x2176, 0x01e0},
	{0x2178, 0x81d2},
	{0x217a, 0x40b2},
	{0x217c, 0x03c0},
	{0x217e, 0x81d4},
	{0x2180, 0x40b2},
	{0x2182, 0x05a0},
	{0x2184, 0x81d6},
	{0x2186, 0x40b2},
	{0x2188, 0x0281},
	{0x218a, 0x81d8},
	{0x218c, 0x40b2},
	{0x218e, 0x02c0},
	{0x2190, 0x81da},
	{0x2192, 0x40b2},
	{0x2194, 0x0064},
	{0x2196, 0x81e6},
	{0x2198, 0x93d2},
	{0x219a, 0x003d},
	{0x219c, 0x2002},
	{0x219e, 0x4030},
	{0x21a0, 0xf956},
	{0x21a2, 0x0900},
	{0x21a4, 0x7312},
	{0x21a6, 0x43d2},
	{0x21a8, 0x003d},
	{0x21aa, 0x40b2},
	{0x21ac, 0x9887},
	{0x21ae, 0x0b82},
	{0x21b0, 0x40b2},
	{0x21b2, 0xc540},
	{0x21b4, 0x0b84},
	{0x21b6, 0x40b2},
	{0x21b8, 0xb540},
	{0x21ba, 0x0b86},
	{0x21bc, 0x40b2},
	{0x21be, 0x0085},
	{0x21c0, 0x0b88},
	{0x21c2, 0x40b2},
	{0x21c4, 0xd304},
	{0x21c6, 0x0b8a},
	{0x21c8, 0x40b2},
	{0x21ca, 0x0420},
	{0x21cc, 0x0b8c},
	{0x21ce, 0x40b2},
	{0x21d0, 0xc200},
	{0x21d2, 0x0b8e},
	{0x21d4, 0x4392},
	{0x21d6, 0x0ba6},
	{0x21d8, 0x43d2},
	{0x21da, 0x01a0},
	{0x21dc, 0x40f2},
	{0x21de, 0x0003},
	{0x21e0, 0x01a2},
	{0x21e2, 0x43d2},
	{0x21e4, 0x019d},
	{0x21e6, 0x43d2},
	{0x21e8, 0x0f82},
	{0x21ea, 0x0cff},
	{0x21ec, 0x0cff},
	{0x21ee, 0x0cff},
	{0x21f0, 0x0cff},
	{0x21f2, 0x0cff},
	{0x21f4, 0x0cff},
	{0x21f6, 0x0c32},
	{0x21f8, 0x40f2},
	{0x21fa, 0x000e},
	{0x21fc, 0x0f90},
	{0x21fe, 0x4392},
	{0x2200, 0x7326},
	{0x2202, 0x90f2},
	{0x2204, 0x0010},
	{0x2206, 0x00bf},
	{0x2208, 0x2002},
	{0x220a, 0x4030},
	{0x220c, 0xf7aa},
	{0x220e, 0x403b},
	{0x2210, 0x7f10},
	{0x2212, 0x439b},
	{0x2214, 0x0000},
	{0x2216, 0x403f},
	{0x2218, 0xf13a},
	{0x221a, 0x12b0},
	{0x221c, 0xe12a},
	{0x221e, 0x438b},
	{0x2220, 0x0000},
	{0x2222, 0x4392},
	{0x2224, 0x81de},
	{0x2226, 0x40b2},
	{0x2228, 0x02bc},
	{0x222a, 0x731e},
	{0x222c, 0x43a2},
	{0x222e, 0x81dc},
	{0x2230, 0xb3e2},
	{0x2232, 0x00b4},
	{0x2234, 0x2402},
	{0x2236, 0x4392},
	{0x2238, 0x81dc},
	{0x223a, 0x4326},
	{0x223c, 0xb3d2},
	{0x223e, 0x00b4},
	{0x2240, 0x2002},
	{0x2242, 0x4030},
	{0x2244, 0xf79a},
	{0x2246, 0x4306},
	{0x2248, 0x12b0},
	{0x224a, 0xe16a},
	{0x224c, 0x40b2},
	{0x224e, 0x012c},
	{0x2250, 0x8036},
	{0x2252, 0x40b2},
	{0x2254, 0x0030},
	{0x2256, 0x8034},
	{0x2258, 0x12b0},
	{0x225a, 0xe238},
	{0x225c, 0x4382},
	{0x225e, 0x81e4},
	{0x2260, 0x43a2},
	{0x2262, 0x7320},
	{0x2264, 0x4382},
	{0x2266, 0x7322},
	{0x2268, 0x4392},
	{0x226a, 0x7326},
	{0x226c, 0x425f},
	{0x226e, 0x00be},
	{0x2270, 0xf07f},
	{0x2272, 0x0010},
	{0x2274, 0x4f0e},
	{0x2276, 0xf03e},
	{0x2278, 0x0010},
	{0x227a, 0x4e82},
	{0x227c, 0x81e2},
	{0x227e, 0x425f},
	{0x2280, 0x008c},
	{0x2282, 0x4fc2},
	{0x2284, 0x81e0},
	{0x2286, 0x43c2},
	{0x2288, 0x81e1},
	{0x228a, 0x425f},
	{0x228c, 0x00cb},
	{0x228e, 0x4f49},
	{0x2290, 0x425f},
	{0x2292, 0x009e},
	{0x2294, 0xf07f},
	{0x2296, 0x000f},
	{0x2298, 0x4f4e},
	{0x229a, 0x425f},
	{0x229c, 0x009f},
	{0x229e, 0xf07f},
	{0x22a0, 0x000f},
	{0x22a2, 0xf37f},
	{0x22a4, 0x5f0e},
	{0x22a6, 0x110e},
	{0x22a8, 0x425f},
	{0x22aa, 0x00b2},
	{0x22ac, 0xf07f},
	{0x22ae, 0x000f},
	{0x22b0, 0x4f48},
	{0x22b2, 0x425f},
	{0x22b4, 0x00b3},
	{0x22b6, 0xf07f},
	{0x22b8, 0x000f},
	{0x22ba, 0xf37f},
	{0x22bc, 0x5f08},
	{0x22be, 0x1108},
	{0x22c0, 0x421f},
	{0x22c2, 0x0098},
	{0x22c4, 0x821f},
	{0x22c6, 0x0092},
	{0x22c8, 0x531f},
	{0x22ca, 0x4f0c},
	{0x22cc, 0x4e0a},
	{0x22ce, 0x12b0},
	{0x22d0, 0xfba8},
	{0x22d2, 0x4c82},
	{0x22d4, 0x0a86},
	{0x22d6, 0x9309},
	{0x22d8, 0x2002},
	{0x22da, 0x4030},
	{0x22dc, 0xf78c},
	{0x22de, 0x9318},
	{0x22e0, 0x2002},
	{0x22e2, 0x4030},
	{0x22e4, 0xf78c},
	{0x22e6, 0x421f},
	{0x22e8, 0x00ac},
	{0x22ea, 0x821f},
	{0x22ec, 0x00a6},
	{0x22ee, 0x531f},
	{0x22f0, 0x5338},
	{0x22f2, 0x4f0c},
	{0x22f4, 0x480a},
	{0x22f6, 0x12b0},
	{0x22f8, 0xfba8},
	{0x22fa, 0x4c82},
	{0x22fc, 0x0a88},
	{0x22fe, 0x40b2},
	{0x2300, 0x0034},
	{0x2302, 0x7900},
	{0x2304, 0x4292},
	{0x2306, 0x0092},
	{0x2308, 0x0a8c},
	{0x230a, 0x4292},
	{0x230c, 0x0098},
	{0x230e, 0x0a9e},
	{0x2310, 0x40b2},
	{0x2312, 0x0c01},
	{0x2314, 0x7500},
	{0x2316, 0x40b2},
	{0x2318, 0x0c01},
	{0x231a, 0x8270},
	{0x231c, 0x40b2},
	{0x231e, 0x0803},
	{0x2320, 0x7502},
	{0x2322, 0x40b2},
	{0x2324, 0x0807},
	{0x2326, 0x7504},
	{0x2328, 0x40b2},
	{0x232a, 0x5803},
	{0x232c, 0x7506},
	{0x232e, 0x40b2},
	{0x2330, 0x0801},
	{0x2332, 0x7508},
	{0x2334, 0x40b2},
	{0x2336, 0x0805},
	{0x2338, 0x750a},
	{0x233a, 0x40b2},
	{0x233c, 0x5801},
	{0x233e, 0x750c},
	{0x2340, 0x40b2},
	{0x2342, 0x0803},
	{0x2344, 0x750e},
	{0x2346, 0x40b2},
	{0x2348, 0x0802},
	{0x234a, 0x7510},
	{0x234c, 0x40b2},
	{0x234e, 0x0800},
	{0x2350, 0x7512},
	{0x2352, 0x403f},
	{0x2354, 0x8270},
	{0x2356, 0x12b0},
	{0x2358, 0xe2dc},
	{0x235a, 0x4392},
	{0x235c, 0x7f06},
	{0x235e, 0x43a2},
	{0x2360, 0x7f0a},
	{0x2362, 0x9382},
	{0x2364, 0x81e0},
	{0x2366, 0x2002},
	{0x2368, 0x4030},
	{0x236a, 0xf75a},
	{0x236c, 0x42a2},
	{0x236e, 0x7f0a},
	{0x2370, 0x40b2},
	{0x2372, 0x000a},
	{0x2374, 0x8264},
	{0x2376, 0x40b2},
	{0x2378, 0x01ea},
	{0x237a, 0x8266},
	{0x237c, 0x40b2},
	{0x237e, 0x03ca},
	{0x2380, 0x8268},
	{0x2382, 0x40b2},
	{0x2384, 0x05aa},
	{0x2386, 0x826a},
	{0x2388, 0x40b2},
	{0x238a, 0xf0d2},
	{0x238c, 0x8272},
	{0x238e, 0x4382},
	{0x2390, 0x81d0},
	{0x2392, 0x9382},
	{0x2394, 0x7f0a},
	{0x2396, 0x2413},
	{0x2398, 0x430e},
	{0x239a, 0x421d},
	{0x239c, 0x8272},
	{0x239e, 0x0800},
	{0x23a0, 0x7f08},
	{0x23a2, 0x4e0f},
	{0x23a4, 0x5f0f},
	{0x23a6, 0x4f92},
	{0x23a8, 0x8264},
	{0x23aa, 0x7f00},
	{0x23ac, 0x4d82},
	{0x23ae, 0x7f02},
	{0x23b0, 0x531e},
	{0x23b2, 0x421f},
	{0x23b4, 0x7f0a},
	{0x23b6, 0x9f0e},
	{0x23b8, 0x2bf2},
	{0x23ba, 0x4e82},
	{0x23bc, 0x81d0},
	{0x23be, 0x407b},
	{0x23c0, 0xff8b},
	{0x23c2, 0x4392},
	{0x23c4, 0x731c},
	{0x23c6, 0x40b2},
	{0x23c8, 0x8038},
	{0x23ca, 0x7a00},
	{0x23cc, 0x40b2},
	{0x23ce, 0x0064},
	{0x23d0, 0x7a02},
	{0x23d2, 0x40b2},
	{0x23d4, 0x0304},
	{0x23d6, 0x7a08},
	{0x23d8, 0xb3e2},
	{0x23da, 0x00ce},
	{0x23dc, 0x242d},
	{0x23de, 0x4292},
	{0x23e0, 0x0126},
	{0x23e2, 0x0580},
	{0x23e4, 0x4292},
	{0x23e6, 0x01a8},
	{0x23e8, 0x0582},
	{0x23ea, 0x4292},
	{0x23ec, 0x01aa},
	{0x23ee, 0x0584},
	{0x23f0, 0x4292},
	{0x23f2, 0x01ac},
	{0x23f4, 0x0586},
	{0x23f6, 0x9382},
	{0x23f8, 0x003a},
	{0x23fa, 0x2599},
	{0x23fc, 0x90b2},
	{0x23fe, 0x0011},
	{0x2400, 0x003a},
	{0x2402, 0x2d95},
	{0x2404, 0x403e},
	{0x2406, 0x012e},
	{0x2408, 0x4e6f},
	{0x240a, 0xf37f},
	{0x240c, 0x521f},
	{0x240e, 0x0126},
	{0x2410, 0x4f82},
	{0x2412, 0x0580},
	{0x2414, 0x4e6f},
	{0x2416, 0xf37f},
	{0x2418, 0x521f},
	{0x241a, 0x01a8},
	{0x241c, 0x4f82},
	{0x241e, 0x0582},
	{0x2420, 0x4e6f},
	{0x2422, 0xf37f},
	{0x2424, 0x521f},
	{0x2426, 0x01aa},
	{0x2428, 0x4f82},
	{0x242a, 0x0584},
	{0x242c, 0x4e6f},
	{0x242e, 0xf37f},
	{0x2430, 0x521f},
	{0x2432, 0x01ac},
	{0x2434, 0x4f82},
	{0x2436, 0x0586},
	{0x2438, 0x9382},
	{0x243a, 0x81de},
	{0x243c, 0x2411},
	{0x243e, 0x4382},
	{0x2440, 0x81d0},
	{0x2442, 0x421d},
	{0x2444, 0x81d0},
	{0x2446, 0x4d0e},
	{0x2448, 0x5e0e},
	{0x244a, 0x4e0f},
	{0x244c, 0x510f},
	{0x244e, 0x4e9f},
	{0x2450, 0x0b00},
	{0x2452, 0x0000},
	{0x2454, 0x531d},
	{0x2456, 0x4d82},
	{0x2458, 0x81d0},
	{0x245a, 0x903d},
	{0x245c, 0x0016},
	{0x245e, 0x2bf1},
	{0x2460, 0xb3d2},
	{0x2462, 0x00ce},
	{0x2464, 0x2412},
	{0x2466, 0x4382},
	{0x2468, 0x81d0},
	{0x246a, 0x421e},
	{0x246c, 0x81d0},
	{0x246e, 0x903e},
	{0x2470, 0x0009},
	{0x2472, 0x2405},
	{0x2474, 0x5e0e},
	{0x2476, 0x4e0f},
	{0x2478, 0x510f},
	{0x247a, 0x4fae},
	{0x247c, 0x0b80},
	{0x247e, 0x5392},
	{0x2480, 0x81d0},
	{0x2482, 0x90b2},
	{0x2484, 0x0016},
	{0x2486, 0x81d0},
	{0x2488, 0x2bf0},
	{0x248a, 0x9382},
	{0x248c, 0x81de},
	{0x248e, 0x200b},
	{0x2490, 0x0b00},
	{0x2492, 0x7302},
	{0x2494, 0x0258},
	{0x2496, 0x4382},
	{0x2498, 0x7904},
	{0x249a, 0x0900},
	{0x249c, 0x7308},
	{0x249e, 0x403f},
	{0x24a0, 0x8270},
	{0x24a2, 0x12b0},
	{0x24a4, 0xe2dc},
	{0x24a6, 0xb2e2},
	{0x24a8, 0x00ce},
	{0x24aa, 0x2425},
	{0x24ac, 0x4392},
	{0x24ae, 0x7326},
	{0x24b0, 0x9382},
	{0x24b2, 0x81de},
	{0x24b4, 0x200f},
	{0x24b6, 0x4292},
	{0x24b8, 0x8260},
	{0x24ba, 0x0b92},
	{0x24bc, 0x4292},
	{0x24be, 0x826c},
	{0x24c0, 0x0580},
	{0x24c2, 0x4292},
	{0x24c4, 0x8262},
	{0x24c6, 0x0582},
	{0x24c8, 0x4292},
	{0x24ca, 0x826e},
	{0x24cc, 0x0584},
	{0x24ce, 0x4292},
	{0x24d0, 0x8274},
	{0x24d2, 0x0586},
	{0x24d4, 0x4292},
	{0x24d6, 0x00ba},
	{0x24d8, 0x8260},
	{0x24da, 0x4292},
	{0x24dc, 0x0580},
	{0x24de, 0x826c},
	{0x24e0, 0x4292},
	{0x24e2, 0x0582},
	{0x24e4, 0x8262},
	{0x24e6, 0x4292},
	{0x24e8, 0x0584},
	{0x24ea, 0x826e},
	{0x24ec, 0x4292},
	{0x24ee, 0x0586},
	{0x24f0, 0x8274},
	{0x24f2, 0x4392},
	{0x24f4, 0x7326},
	{0x24f6, 0x434e},
	{0x24f8, 0x9382},
	{0x24fa, 0x81de},
	{0x24fc, 0x2007},
	{0x24fe, 0x421f},
	{0x2500, 0x732a},
	{0x2502, 0xf35f},
	{0x2504, 0x2003},
	{0x2506, 0xb3a2},
	{0x2508, 0x732a},
	{0x250a, 0x2401},
	{0x250c, 0x435e},
	{0x250e, 0x4ec2},
	{0x2510, 0x81e8},
	{0x2512, 0x42a2},
	{0x2514, 0x81ea},
	{0x2516, 0x40b2},
	{0x2518, 0x0262},
	{0x251a, 0x81ec},
	{0x251c, 0x40b2},
	{0x251e, 0x02c4},
	{0x2520, 0x81ee},
	{0x2522, 0x40b2},
	{0x2524, 0x0532},
	{0x2526, 0x81f0},
	{0x2528, 0x40b2},
	{0x252a, 0x03b6},
	{0x252c, 0x81f2},
	{0x252e, 0x40b2},
	{0x2530, 0x03c4},
	{0x2532, 0x81f4},
	{0x2534, 0x40b2},
	{0x2536, 0x03de},
	{0x2538, 0x81f6},
	{0x253a, 0x40b2},
	{0x253c, 0x0596},
	{0x253e, 0x81f8},
	{0x2540, 0x40b2},
	{0x2542, 0x05c4},
	{0x2544, 0x81fa},
	{0x2546, 0x40b2},
	{0x2548, 0x0776},
	{0x254a, 0x81fc},
	{0x254c, 0x9382},
	{0x254e, 0x81e2},
	{0x2550, 0x24e1},
	{0x2552, 0x40b2},
	{0x2554, 0x0289},
	{0x2556, 0x81ee},
	{0x2558, 0x40b2},
	{0x255a, 0x04e7},
	{0x255c, 0x81f0},
	{0x255e, 0x12b0},
	{0x2560, 0xea88},
	{0x2562, 0x0900},
	{0x2564, 0x7328},
	{0x2566, 0x4682},
	{0x2568, 0x7114},
	{0x256a, 0x421f},
	{0x256c, 0x7316},
	{0x256e, 0xc312},
	{0x2570, 0x100f},
	{0x2572, 0x503f},
	{0x2574, 0xff9c},
	{0x2576, 0x4f82},
	{0x2578, 0x7334},
	{0x257a, 0x0f00},
	{0x257c, 0x7302},
	{0x257e, 0x4392},
	{0x2580, 0x7f0c},
	{0x2582, 0x4392},
	{0x2584, 0x7f10},
	{0x2586, 0x4392},
	{0x2588, 0x770a},
	{0x258a, 0x4392},
	{0x258c, 0x770e},
	{0x258e, 0x9392},
	{0x2590, 0x7114},
	{0x2592, 0x2073},
	{0x2594, 0x0b00},
	{0x2596, 0x7302},
	{0x2598, 0x0258},
	{0x259a, 0x4382},
	{0x259c, 0x7904},
	{0x259e, 0x0800},
	{0x25a0, 0x7118},
	{0x25a2, 0x1247},
	{0x25a4, 0x1230},
	{0x25a6, 0x0cce},
	{0x25a8, 0x1230},
	{0x25aa, 0x0cf0},
	{0x25ac, 0x410f},
	{0x25ae, 0x503f},
	{0x25b0, 0x0030},
	{0x25b2, 0x120f},
	{0x25b4, 0x421c},
	{0x25b6, 0x0ca0},
	{0x25b8, 0x421d},
	{0x25ba, 0x0caa},
	{0x25bc, 0x421e},
	{0x25be, 0x0cb4},
	{0x25c0, 0x421f},
	{0x25c2, 0x0cb2},
	{0x25c4, 0x12b0},
	{0x25c6, 0xf9a4},
	{0x25c8, 0x1247},
	{0x25ca, 0x1230},
	{0x25cc, 0x0cd0},
	{0x25ce, 0x1230},
	{0x25d0, 0x0cf2},
	{0x25d2, 0x410f},
	{0x25d4, 0x503f},
	{0x25d6, 0x003a},
	{0x25d8, 0x120f},
	{0x25da, 0x421c},
	{0x25dc, 0x0ca2},
	{0x25de, 0x421d},
	{0x25e0, 0x0cac},
	{0x25e2, 0x421e},
	{0x25e4, 0x0cb8},
	{0x25e6, 0x421f},
	{0x25e8, 0x0cb6},
	{0x25ea, 0x12b0},
	{0x25ec, 0xf9a4},
	{0x25ee, 0x1247},
	{0x25f0, 0x1230},
	{0x25f2, 0x0cd2},
	{0x25f4, 0x1230},
	{0x25f6, 0x0cf4},
	{0x25f8, 0x410f},
	{0x25fa, 0x503f},
	{0x25fc, 0x0044},
	{0x25fe, 0x120f},
	{0x2600, 0x421c},
	{0x2602, 0x0ca4},
	{0x2604, 0x421d},
	{0x2606, 0x0cae},
	{0x2608, 0x421e},
	{0x260a, 0x0cbc},
	{0x260c, 0x421f},
	{0x260e, 0x0cba},
	{0x2610, 0x12b0},
	{0x2612, 0xf9a4},
	{0x2614, 0x1247},
	{0x2616, 0x1230},
	{0x2618, 0x0cd4},
	{0x261a, 0x1230},
	{0x261c, 0x0cf6},
	{0x261e, 0x410f},
	{0x2620, 0x503f},
	{0x2622, 0x004e},
	{0x2624, 0x120f},
	{0x2626, 0x421c},
	{0x2628, 0x0ca6},
	{0x262a, 0x421d},
	{0x262c, 0x0cb0},
	{0x262e, 0x421e},
	{0x2630, 0x0cc0},
	{0x2632, 0x421f},
	{0x2634, 0x0cbe},
	{0x2636, 0x12b0},
	{0x2638, 0xf9a4},
	{0x263a, 0x425f},
	{0x263c, 0x0c80},
	{0x263e, 0xf35f},
	{0x2640, 0x5031},
	{0x2642, 0x0020},
	{0x2644, 0x934f},
	{0x2646, 0x2008},
	{0x2648, 0x4382},
	{0x264a, 0x0cce},
	{0x264c, 0x4382},
	{0x264e, 0x0cd0},
	{0x2650, 0x4382},
	{0x2652, 0x0cd2},
	{0x2654, 0x4382},
	{0x2656, 0x0cd4},
	{0x2658, 0x425f},
	{0x265a, 0x81e8},
	{0x265c, 0xdf47},
	{0x265e, 0x474e},
	{0x2660, 0x934f},
	{0x2662, 0x2001},
	{0x2664, 0x5e0e},
	{0x2666, 0x4e47},
	{0x2668, 0x0900},
	{0x266a, 0x7112},
	{0x266c, 0x4b4f},
	{0x266e, 0x12b0},
	{0x2670, 0xfaa0},
	{0x2672, 0x0b00},
	{0x2674, 0x7302},
	{0x2676, 0x0036},
	{0x2678, 0x3f8a},
	{0x267a, 0x0b00},
	{0x267c, 0x7302},
	{0x267e, 0x0036},
	{0x2680, 0x4392},
	{0x2682, 0x7904},
	{0x2684, 0x421e},
	{0x2686, 0x7100},
	{0x2688, 0x9382},
	{0x268a, 0x7114},
	{0x268c, 0x2009},
	{0x268e, 0x421f},
	{0x2690, 0x00a2},
	{0x2692, 0x9f0e},
	{0x2694, 0x2803},
	{0x2696, 0x9e82},
	{0x2698, 0x00a8},
	{0x269a, 0x2c02},
	{0x269c, 0x4382},
	{0x269e, 0x7904},
	{0x26a0, 0x93a2},
	{0x26a2, 0x7114},
	{0x26a4, 0x2424},
	{0x26a6, 0x903e},
	{0x26a8, 0x0038},
	{0x26aa, 0x2815},
	{0x26ac, 0x503e},
	{0x26ae, 0xffd8},
	{0x26b0, 0x4e82},
	{0x26b2, 0x7a04},
	{0x26b4, 0x43b2},
	{0x26b6, 0x7a06},
	{0x26b8, 0x9382},
	{0x26ba, 0x81e0},
	{0x26bc, 0x2408},
	{0x26be, 0x9382},
	{0x26c0, 0x81e4},
	{0x26c2, 0x2411},
	{0x26c4, 0x421f},
	{0x26c6, 0x7a04},
	{0x26c8, 0x832f},
	{0x26ca, 0x4f82},
	{0x26cc, 0x7a06},
	{0x26ce, 0x4392},
	{0x26d0, 0x7a0a},
	{0x26d2, 0x0800},
	{0x26d4, 0x7a0a},
	{0x26d6, 0x4b4f},
	{0x26d8, 0x12b0},
	{0x26da, 0xfaa0},
	{0x26dc, 0x930f},
	{0x26de, 0x2757},
	{0x26e0, 0x4382},
	{0x26e2, 0x81de},
	{0x26e4, 0x3e79},
	{0x26e6, 0x421f},
	{0x26e8, 0x7a04},
	{0x26ea, 0x532f},
	{0x26ec, 0x3fee},
	{0x26ee, 0x421f},
	{0x26f0, 0x00a6},
	{0x26f2, 0x9f0e},
	{0x26f4, 0x2803},
	{0x26f6, 0x9e82},
	{0x26f8, 0x00ac},
	{0x26fa, 0x2c02},
	{0x26fc, 0x4382},
	{0x26fe, 0x7904},
	{0x2700, 0x4b4f},
	{0x2702, 0xc312},
	{0x2704, 0x104f},
	{0x2706, 0xf35b},
	{0x2708, 0xe37b},
	{0x270a, 0x535b},
	{0x270c, 0xf07b},
	{0x270e, 0xffb8},
	{0x2710, 0xef4b},
	{0x2712, 0x3fc9},
	{0x2714, 0x9382},
	{0x2716, 0x81e0},
	{0x2718, 0x2722},
	{0x271a, 0x40b2},
	{0x271c, 0x001e},
	{0x271e, 0x81ec},
	{0x2720, 0x40b2},
	{0x2722, 0x01d6},
	{0x2724, 0x81ee},
	{0x2726, 0x40b2},
	{0x2728, 0x0205},
	{0x272a, 0x81f0},
	{0x272c, 0x3f18},
	{0x272e, 0x90b2},
	{0x2730, 0x0011},
	{0x2732, 0x003a},
	{0x2734, 0x2807},
	{0x2736, 0x90b2},
	{0x2738, 0x0080},
	{0x273a, 0x003a},
	{0x273c, 0x2c03},
	{0x273e, 0x403e},
	{0x2740, 0x012f},
	{0x2742, 0x3e62},
	{0x2744, 0x90b2},
	{0x2746, 0x0080},
	{0x2748, 0x003a},
	{0x274a, 0x2a76},
	{0x274c, 0x90b2},
	{0x274e, 0x0100},
	{0x2750, 0x003a},
	{0x2752, 0x2e72},
	{0x2754, 0x403e},
	{0x2756, 0x0130},
	{0x2758, 0x3e57},
	{0x275a, 0x9382},
	{0x275c, 0x81e2},
	{0x275e, 0x240b},
	{0x2760, 0x40b2},
	{0x2762, 0x000e},
	{0x2764, 0x8264},
	{0x2766, 0x40b2},
	{0x2768, 0x028f},
	{0x276a, 0x8266},
	{0x276c, 0x40b2},
	{0x276e, 0xf06a},
	{0x2770, 0x8272},
	{0x2772, 0x4030},
	{0x2774, 0xf38e},
	{0x2776, 0x40b2},
	{0x2778, 0x000e},
	{0x277a, 0x8264},
	{0x277c, 0x40b2},
	{0x277e, 0x02ce},
	{0x2780, 0x8266},
	{0x2782, 0x40b2},
	{0x2784, 0xf000},
	{0x2786, 0x8272},
	{0x2788, 0x4030},
	{0x278a, 0xf38e},
	{0x278c, 0x421f},
	{0x278e, 0x00ac},
	{0x2790, 0x821f},
	{0x2792, 0x00a6},
	{0x2794, 0x531f},
	{0x2796, 0x4030},
	{0x2798, 0xf2f2},
	{0x279a, 0xb3e2},
	{0x279c, 0x00b4},
	{0x279e, 0x2002},
	{0x27a0, 0x4030},
	{0x27a2, 0xf248},
	{0x27a4, 0x4316},
	{0x27a6, 0x4030},
	{0x27a8, 0xf248},
	{0x27aa, 0x43d2},
	{0x27ac, 0x0180},
	{0x27ae, 0x4392},
	{0x27b0, 0x760e},
	{0x27b2, 0x9382},
	{0x27b4, 0x760c},
	{0x27b6, 0x2002},
	{0x27b8, 0x0c64},
	{0x27ba, 0x3ffb},
	{0x27bc, 0x421f},
	{0x27be, 0x760a},
	{0x27c0, 0x932f},
	{0x27c2, 0x2012},
	{0x27c4, 0x421b},
	{0x27c6, 0x018a},
	{0x27c8, 0x4b82},
	{0x27ca, 0x7600},
	{0x27cc, 0x12b0},
	{0x27ce, 0xfb18},
	{0x27d0, 0x903b},
	{0x27d2, 0x1000},
	{0x27d4, 0x2806},
	{0x27d6, 0x403f},
	{0x27d8, 0x8028},
	{0x27da, 0x12b0},
	{0x27dc, 0xfb2c},
	{0x27de, 0x4b0d},
	{0x27e0, 0x3fe6},
	{0x27e2, 0x403f},
	{0x27e4, 0x0028},
	{0x27e6, 0x3ff9},
	{0x27e8, 0x903f},
	{0x27ea, 0x0003},
	{0x27ec, 0x28af},
	{0x27ee, 0x903f},
	{0x27f0, 0x0102},
	{0x27f2, 0x208a},
	{0x27f4, 0x43c2},
	{0x27f6, 0x018c},
	{0x27f8, 0x425f},
	{0x27fa, 0x0186},
	{0x27fc, 0x4f49},
	{0x27fe, 0x93d2},
	{0x2800, 0x018f},
	{0x2802, 0x2480},
	{0x2804, 0x425f},
	{0x2806, 0x018f},
	{0x2808, 0x4f4a},
	{0x280a, 0x4b0e},
	{0x280c, 0x108e},
	{0x280e, 0xf37e},
	{0x2810, 0xc312},
	{0x2812, 0x100e},
	{0x2814, 0x110e},
	{0x2816, 0x110e},
	{0x2818, 0x110e},
	{0x281a, 0x4d0f},
	{0x281c, 0x108f},
	{0x281e, 0xf37f},
	{0x2820, 0xc312},
	{0x2822, 0x100f},
	{0x2824, 0x110f},
	{0x2826, 0x110f},
	{0x2828, 0x110f},
	{0x282a, 0x9f0e},
	{0x282c, 0x240d},
	{0x282e, 0x0261},
	{0x2830, 0x0000},
	{0x2832, 0x4b82},
	{0x2834, 0x7600},
	{0x2836, 0x12b0},
	{0x2838, 0xfb18},
	{0x283a, 0x903b},
	{0x283c, 0x1000},
	{0x283e, 0x285f},
	{0x2840, 0x403f},
	{0x2842, 0x8028},
	{0x2844, 0x12b0},
	{0x2846, 0xfb2c},
	{0x2848, 0x4382},
	{0x284a, 0x81d0},
	{0x284c, 0x430c},
	{0x284e, 0x4c0d},
	{0x2850, 0x431f},
	{0x2852, 0x4c0e},
	{0x2854, 0x930e},
	{0x2856, 0x2403},
	{0x2858, 0x5f0f},
	{0x285a, 0x831e},
	{0x285c, 0x23fd},
	{0x285e, 0xf90f},
	{0x2860, 0x2444},
	{0x2862, 0x430f},
	{0x2864, 0x9a0f},
	{0x2866, 0x2c2c},
	{0x2868, 0x4c0e},
	{0x286a, 0x4b82},
	{0x286c, 0x7600},
	{0x286e, 0x4e82},
	{0x2870, 0x7602},
	{0x2872, 0x4982},
	{0x2874, 0x7604},
	{0x2876, 0x0264},
	{0x2878, 0x0000},
	{0x287a, 0x0224},
	{0x287c, 0x0000},
	{0x287e, 0x0264},
	{0x2880, 0x0000},
	{0x2882, 0x0260},
	{0x2884, 0x0000},
	{0x2886, 0x0268},
	{0x2888, 0x0000},
	{0x288a, 0x0c5a},
	{0x288c, 0x02e8},
	{0x288e, 0x0000},
	{0x2890, 0x0cb5},
	{0x2892, 0x02a8},
	{0x2894, 0x0000},
	{0x2896, 0x0cb5},
	{0x2898, 0x0cb5},
	{0x289a, 0x0cb5},
	{0x289c, 0x0cb5},
	{0x289e, 0x0cb5},
	{0x28a0, 0x0cb5},
	{0x28a2, 0x0cb5},
	{0x28a4, 0x0cb5},
	{0x28a6, 0x0c00},
	{0x28a8, 0x02e8},
	{0x28aa, 0x0000},
	{0x28ac, 0x0cb5},
	{0x28ae, 0x0268},
	{0x28b0, 0x0000},
	{0x28b2, 0x0c64},
	{0x28b4, 0x0260},
	{0x28b6, 0x0000},
	{0x28b8, 0x0c64},
	{0x28ba, 0x531f},
	{0x28bc, 0x9a0f},
	{0x28be, 0x2bd5},
	{0x28c0, 0x4c82},
	{0x28c2, 0x7602},
	{0x28c4, 0x4b82},
	{0x28c6, 0x7600},
	{0x28c8, 0x0270},
	{0x28ca, 0x0000},
	{0x28cc, 0x0c1c},
	{0x28ce, 0x0270},
	{0x28d0, 0x0001},
	{0x28d2, 0x421f},
	{0x28d4, 0x7606},
	{0x28d6, 0x4f4e},
	{0x28d8, 0x431f},
	{0x28da, 0x930d},
	{0x28dc, 0x2403},
	{0x28de, 0x5f0f},
	{0x28e0, 0x831d},
	{0x28e2, 0x23fd},
	{0x28e4, 0xff4e},
	{0x28e6, 0xdec2},
	{0x28e8, 0x018c},
	{0x28ea, 0x531c},
	{0x28ec, 0x923c},
	{0x28ee, 0x2baf},
	{0x28f0, 0x4c82},
	{0x28f2, 0x81d0},
	{0x28f4, 0x0260},
	{0x28f6, 0x0000},
	{0x28f8, 0x4b0d},
	{0x28fa, 0x531b},
	{0x28fc, 0x3f58},
	{0x28fe, 0x403f},
	{0x2900, 0x0028},
	{0x2902, 0x3fa0},
	{0x2904, 0x432a},
	{0x2906, 0x3f81},
	{0x2908, 0x903f},
	{0x290a, 0x0201},
	{0x290c, 0x2350},
	{0x290e, 0x531b},
	{0x2910, 0x4b0e},
	{0x2912, 0x108e},
	{0x2914, 0xf37e},
	{0x2916, 0xc312},
	{0x2918, 0x100e},
	{0x291a, 0x110e},
	{0x291c, 0x110e},
	{0x291e, 0x110e},
	{0x2920, 0x4d0f},
	{0x2922, 0x108f},
	{0x2924, 0xf37f},
	{0x2926, 0xc312},
	{0x2928, 0x100f},
	{0x292a, 0x110f},
	{0x292c, 0x110f},
	{0x292e, 0x110f},
	{0x2930, 0x9f0e},
	{0x2932, 0x2406},
	{0x2934, 0x0261},
	{0x2936, 0x0000},
	{0x2938, 0x4b82},
	{0x293a, 0x7600},
	{0x293c, 0x12b0},
	{0x293e, 0xfb18},
	{0x2940, 0x4b0f},
	{0x2942, 0x12b0},
	{0x2944, 0xfb56},
	{0x2946, 0x4fc2},
	{0x2948, 0x0188},
	{0x294a, 0x3f49},
	{0x294c, 0x931f},
	{0x294e, 0x232f},
	{0x2950, 0x421b},
	{0x2952, 0x018a},
	{0x2954, 0x3ff1},
	{0x2956, 0x40b2},
	{0x2958, 0x1807},
	{0x295a, 0x0b82},
	{0x295c, 0x40b2},
	{0x295e, 0x3540},
	{0x2960, 0x0b84},
	{0x2962, 0x40b2},
	{0x2964, 0x3540},
	{0x2966, 0x0b86},
	{0x2968, 0x4382},
	{0x296a, 0x0b88},
	{0x296c, 0x4382},
	{0x296e, 0x0b8a},
	{0x2970, 0x4382},
	{0x2972, 0x0b8c},
	{0x2974, 0x40b2},
	{0x2976, 0x8200},
	{0x2978, 0x0b8e},
	{0x297a, 0x4382},
	{0x297c, 0x0ba6},
	{0x297e, 0x43c2},
	{0x2980, 0x01a0},
	{0x2982, 0x43c2},
	{0x2984, 0x01a2},
	{0x2986, 0x43c2},
	{0x2988, 0x019d},
	{0x298a, 0x40f2},
	{0x298c, 0x000a},
	{0x298e, 0x0f90},
	{0x2990, 0x43c2},
	{0x2992, 0x0f82},
	{0x2994, 0x43c2},
	{0x2996, 0x003d},
	{0x2998, 0x4030},
	{0x299a, 0xf1a2},
	{0x299c, 0x5031},
	{0x299e, 0x0032},
	{0x29a0, 0x4030},
	{0x29a2, 0xfb6a},
	{0x29a4, 0x120b},
	{0x29a6, 0x120a},
	{0x29a8, 0x1209},
	{0x29aa, 0x1208},
	{0x29ac, 0x1207},
	{0x29ae, 0x1206},
	{0x29b0, 0x1205},
	{0x29b2, 0x1204},
	{0x29b4, 0x8321},
	{0x29b6, 0x4039},
	{0x29b8, 0x0014},
	{0x29ba, 0x5109},
	{0x29bc, 0x4d07},
	{0x29be, 0x4c0d},
	{0x29c0, 0x4926},
	{0x29c2, 0x4991},
	{0x29c4, 0x0002},
	{0x29c6, 0x0000},
	{0x29c8, 0x4915},
	{0x29ca, 0x0004},
	{0x29cc, 0x4954},
	{0x29ce, 0x0006},
	{0x29d0, 0x4f0b},
	{0x29d2, 0x430a},
	{0x29d4, 0x4e08},
	{0x29d6, 0x4309},
	{0x29d8, 0xda08},
	{0x29da, 0xdb09},
	{0x29dc, 0x570d},
	{0x29de, 0x432e},
	{0x29e0, 0x421f},
	{0x29e2, 0x0a86},
	{0x29e4, 0x821e},
	{0x29e6, 0x81e0},
	{0x29e8, 0x930e},
	{0x29ea, 0x2403},
	{0x29ec, 0x5f0f},
	{0x29ee, 0x831e},
	{0x29f0, 0x23fd},
	{0x29f2, 0x8f0d},
	{0x29f4, 0x425f},
	{0x29f6, 0x0ce1},
	{0x29f8, 0xf37f},
	{0x29fa, 0x421e},
	{0x29fc, 0x00ba},
	{0x29fe, 0x4f0a},
	{0x2a00, 0x4e0c},
	{0x2a02, 0x12b0},
	{0x2a04, 0xfb6e},
	{0x2a06, 0x4e0f},
	{0x2a08, 0x108f},
	{0x2a0a, 0x4f47},
	{0x2a0c, 0xc312},
	{0x2a0e, 0x1007},
	{0x2a10, 0x5808},
	{0x2a12, 0x6909},
	{0x2a14, 0x5808},
	{0x2a16, 0x6909},
	{0x2a18, 0x5808},
	{0x2a1a, 0x6909},
	{0x2a1c, 0x5808},
	{0x2a1e, 0x6909},
	{0x2a20, 0x5808},
	{0x2a22, 0x6909},
	{0x2a24, 0x5808},
	{0x2a26, 0x6909},
	{0x2a28, 0x4d0e},
	{0x2a2a, 0x430f},
	{0x2a2c, 0x480c},
	{0x2a2e, 0x490d},
	{0x2a30, 0x4e0a},
	{0x2a32, 0x4f0b},
	{0x2a34, 0x12b0},
	{0x2a36, 0xfbc4},
	{0x2a38, 0x5c07},
	{0x2a3a, 0x474e},
	{0x2a3c, 0xf07e},
	{0x2a3e, 0x003f},
	{0x2a40, 0x93c2},
	{0x2a42, 0x81e8},
	{0x2a44, 0x200f},
	{0x2a46, 0x9344},
	{0x2a48, 0x200d},
	{0x2a4a, 0x470c},
	{0x2a4c, 0x430d},
	{0x2a4e, 0x462e},
	{0x2a50, 0x430f},
	{0x2a52, 0x5e0c},
	{0x2a54, 0x6f0d},
	{0x2a56, 0xc312},
	{0x2a58, 0x100d},
	{0x2a5a, 0x100c},
	{0x2a5c, 0x4c07},
	{0x2a5e, 0x4c4e},
	{0x2a60, 0xf07e},
	{0x2a62, 0x003f},
	{0x2a64, 0x4786},
	{0x2a66, 0x0000},
	{0x2a68, 0xb0f2},
	{0x2a6a, 0x0010},
	{0x2a6c, 0x0c83},
	{0x2a6e, 0x2409},
	{0x2a70, 0x4e4f},
	{0x2a72, 0x5f0f},
	{0x2a74, 0x5f0f},
	{0x2a76, 0x5f0f},
	{0x2a78, 0x5f0f},
	{0x2a7a, 0x5f0f},
	{0x2a7c, 0x4f85},
	{0x2a7e, 0x0000},
	{0x2a80, 0x3c02},
	{0x2a82, 0x4385},
	{0x2a84, 0x0000},
	{0x2a86, 0x412f},
	{0x2a88, 0x478f},
	{0x2a8a, 0x0000},
	{0x2a8c, 0x5321},
	{0x2a8e, 0x4134},
	{0x2a90, 0x4135},
	{0x2a92, 0x4136},
	{0x2a94, 0x4137},
	{0x2a96, 0x4138},
	{0x2a98, 0x4139},
	{0x2a9a, 0x413a},
	{0x2a9c, 0x413b},
	{0x2a9e, 0x4130},
	{0x2aa0, 0x4f4e},
	{0x2aa2, 0x421f},
	{0x2aa4, 0x7316},
	{0x2aa6, 0xc312},
	{0x2aa8, 0x100f},
	{0x2aaa, 0x821f},
	{0x2aac, 0x81e6},
	{0x2aae, 0x4f82},
	{0x2ab0, 0x7334},
	{0x2ab2, 0x0f00},
	{0x2ab4, 0x7302},
	{0x2ab6, 0xb0b2},
	{0x2ab8, 0x000f},
	{0x2aba, 0x7300},
	{0x2abc, 0x200e},
	{0x2abe, 0x403f},
	{0x2ac0, 0x0cd8},
	{0x2ac2, 0x43df},
	{0x2ac4, 0x0000},
	{0x2ac6, 0x43cf},
	{0x2ac8, 0x0000},
	{0x2aca, 0x4ec2},
	{0x2acc, 0x0c5a},
	{0x2ace, 0x4ec2},
	{0x2ad0, 0x0c5c},
	{0x2ad2, 0x4ec2},
	{0x2ad4, 0x0c5e},
	{0x2ad6, 0x4ec2},
	{0x2ad8, 0x0c60},
	{0x2ada, 0x421f},
	{0x2adc, 0x7112},
	{0x2ade, 0x93a2},
	{0x2ae0, 0x7114},
	{0x2ae2, 0x2408},
	{0x2ae4, 0x9382},
	{0x2ae6, 0x7112},
	{0x2ae8, 0x2403},
	{0x2aea, 0x5292},
	{0x2aec, 0x81dc},
	{0x2aee, 0x7114},
	{0x2af0, 0x430f},
	{0x2af2, 0x4130},
	{0x2af4, 0xf31f},
	{0x2af6, 0x27f6},
	{0x2af8, 0x4382},
	{0x2afa, 0x7f10},
	{0x2afc, 0x40b2},
	{0x2afe, 0x0003},
	{0x2b00, 0x7114},
	{0x2b02, 0x40b2},
	{0x2b04, 0x000a},
	{0x2b06, 0x7334},
	{0x2b08, 0x0f00},
	{0x2b0a, 0x7302},
	{0x2b0c, 0x4392},
	{0x2b0e, 0x7708},
	{0x2b10, 0x4382},
	{0x2b12, 0x770e},
	{0x2b14, 0x431f},
	{0x2b16, 0x4130},
	{0x2b18, 0x0260},
	{0x2b1a, 0x0000},
	{0x2b1c, 0x0c64},
	{0x2b1e, 0x0c64},
	{0x2b20, 0x0240},
	{0x2b22, 0x0000},
	{0x2b24, 0x0260},
	{0x2b26, 0x0000},
	{0x2b28, 0x0c1e},
	{0x2b2a, 0x4130},
	{0x2b2c, 0x4f0e},
	{0x2b2e, 0xc312},
	{0x2b30, 0x100f},
	{0x2b32, 0x110f},
	{0x2b34, 0xc312},
	{0x2b36, 0x100f},
	{0x2b38, 0x4f82},
	{0x2b3a, 0x7600},
	{0x2b3c, 0xf03e},
	{0x2b3e, 0x0007},
	{0x2b40, 0x4e82},
	{0x2b42, 0x7602},
	{0x2b44, 0x0262},
	{0x2b46, 0x0000},
	{0x2b48, 0x0222},
	{0x2b4a, 0x0000},
	{0x2b4c, 0x0262},
	{0x2b4e, 0x0000},
	{0x2b50, 0x0260},
	{0x2b52, 0x0000},
	{0x2b54, 0x4130},
	{0x2b56, 0x4f82},
	{0x2b58, 0x7600},
	{0x2b5a, 0x0270},
	{0x2b5c, 0x0000},
	{0x2b5e, 0x0c1c},
	{0x2b60, 0x0270},
	{0x2b62, 0x0001},
	{0x2b64, 0x421f},
	{0x2b66, 0x7606},
	{0x2b68, 0x4130},
	{0x2b6a, 0xdf02},
	{0x2b6c, 0x3ffe},
	{0x2b6e, 0x430e},
	{0x2b70, 0x930a},
	{0x2b72, 0x2407},
	{0x2b74, 0xc312},
	{0x2b76, 0x100c},
	{0x2b78, 0x2801},
	{0x2b7a, 0x5a0e},
	{0x2b7c, 0x5a0a},
	{0x2b7e, 0x930c},
	{0x2b80, 0x23f7},
	{0x2b82, 0x4130},
	{0x2b84, 0x430e},
	{0x2b86, 0x430f},
	{0x2b88, 0x3c08},
	{0x2b8a, 0xc312},
	{0x2b8c, 0x100d},
	{0x2b8e, 0x100c},
	{0x2b90, 0x2802},
	{0x2b92, 0x5a0e},
	{0x2b94, 0x6b0f},
	{0x2b96, 0x5a0a},
	{0x2b98, 0x6b0b},
	{0x2b9a, 0x930c},
	{0x2b9c, 0x23f6},
	{0x2b9e, 0x930d},
	{0x2ba0, 0x23f4},
	{0x2ba2, 0x4130},
	{0x2ba4, 0x4030},
	{0x2ba6, 0xfb84},
	{0x2ba8, 0xee0e},
	{0x2baa, 0x403b},
	{0x2bac, 0x0011},
	{0x2bae, 0x3c05},
	{0x2bb0, 0x100d},
	{0x2bb2, 0x6e0e},
	{0x2bb4, 0x9a0e},
	{0x2bb6, 0x2801},
	{0x2bb8, 0x8a0e},
	{0x2bba, 0x6c0c},
	{0x2bbc, 0x6d0d},
	{0x2bbe, 0x831b},
	{0x2bc0, 0x23f7},
	{0x2bc2, 0x4130},
	{0x2bc4, 0xef0f},
	{0x2bc6, 0xee0e},
	{0x2bc8, 0x4039},
	{0x2bca, 0x0021},
	{0x2bcc, 0x3c0a},
	{0x2bce, 0x1008},
	{0x2bd0, 0x6e0e},
	{0x2bd2, 0x6f0f},
	{0x2bd4, 0x9b0f},
	{0x2bd6, 0x2805},
	{0x2bd8, 0x2002},
	{0x2bda, 0x9a0e},
	{0x2bdc, 0x2802},
	{0x2bde, 0x8a0e},
	{0x2be0, 0x7b0f},
	{0x2be2, 0x6c0c},
	{0x2be4, 0x6d0d},
	{0x2be6, 0x6808},
	{0x2be8, 0x8319},
	{0x2bea, 0x23f1},
	{0x2bec, 0x4130},
	{0x2bee, 0x4130},
	{0x2ffe, 0xf14c},
	{0x3000, 0x01fc},
	{0x3002, 0x320f},
	{0x3004, 0x0001},
	{0x3006, 0x01fc},
	{0x3008, 0x320f},
	{0x300a, 0x0001},
	{0x300c, 0x01fc},
	{0x300e, 0x320f},
	{0x3010, 0x0001},
	{0x4000, 0x2500},
	{0x4002, 0xfc2c},
	{0x4004, 0xc004},
	{0x4006, 0x2500},
	{0x4008, 0xfc2c},
	{0x400a, 0xc004},
	{0x400c, 0x2500},
	{0x400e, 0xfc2c},
	{0x4010, 0xc004},
	{0x0a00, 0x0000}, //stream off
	{0x0b00, 0x0000}, 
	{0x0b02, 0x9887}, 
	{0x0b04, 0xc540}, 
	{0x0b06, 0xb540}, 
	{0x0b08, 0x0085}, 
	{0x0b0a, 0xd304}, 
	{0x0b0c, 0x0420}, 
	{0x0b0e, 0xc200}, 
	{0x0b10, 0xac20}, 
	{0x0b12, 0x0000}, 
	{0x0b14, 0x402c},
	{0x0b16, 0x6e0b}, 
	{0x0b18, 0xf20b}, 
	{0x0b1a, 0x0000}, 
	{0x0b1c, 0x0000}, 
	{0x0b1e, 0x0081}, 
	{0x0b20, 0x0000}, 
	{0x0b22, 0xeb80},
	{0x0b24, 0x4500},
	{0x0b26, 0x0001}, 
	{0x0b28, 0x0807}, 
	{0x0c00, 0x1190}, //blc enable
	{0x0c02, 0x0011}, 
	{0x0c04, 0x0000}, 
	{0x0c06, 0x0200}, 
	{0x0c10, 0x0040},
	{0x0c12, 0x0040},
	{0x0c14, 0x0040},
	{0x0c16, 0x0040},
	{0x0a10, 0x4000},
	{0x0c18, 0x80ff}, 
	{0x0c60, 0x0600}, 
	{0x0000, 0x0100}, //image orient
	{0x0f0a, 0x0000},
	{0x0e0a, 0x0001},
	{0x004a, 0x0100},
	{0x000c, 0x0022},
	{0x0008, 0x0b00}, //line length pck
	{0x000a, 0x0f00},
	{0x005a, 0x0202},
	{0x0012, 0x001e},
	{0x0018, 0x0a21},
	{0x0034, 0x0700},
	{0x0022, 0x0008},
	{0x0028, 0x0017},
	{0x0024, 0x0044},
	{0x002a, 0x0049},
	{0x0026, 0x004c},
	{0x002c, 0x07cb},
	{0x002e, 0x1111},
	{0x0030, 0x1111},
	{0x0032, 0x1111},
	{0x001a, 0x1111},
	{0x001c, 0x1111},
	{0x001e, 0x1111},
	{0x0006, 0x0823}, //frame length lines
	{0x0a22, 0x0000},
	{0x0a12, 0x0a00}, //x output size
	{0x0a14, 0x0780}, //y output size
	{0x003e, 0x0000},
	{0x0004, 0x081d}, //coarse int. time
	{0x0052, 0x019c},
	{0x0002, 0x0000},
	{0x0a02, 0x0100},
	{0x0a04, 0x014a}, //isp_en
	{0x0508, 0x0100},
	{0x0046, 0x0000},
	{0x003a, 0x0000}, //analog gain 1x
	{0x0036, 0x0070},
	{0x0038, 0x7000},
	{0x004c, 0x7070},
	{0x0122, 0x0301},
	{0x0804, 0x0002},
	{0x004e, 0x0300},
	{0x0a1a, 0x0800},
	{0x0126, 0x0100}, //Dgain_gr
	{0x0128, 0x0100}, //Dgain_gb
	{0x012a, 0x0100}, //Dgain_r
	{0x012c, 0x0100}, //Dgain_b
	{0x012e, 0x0203},
	{0x0130, 0x0600},
	{0x090c, 0x0fdc},
	{0x090e, 0x0041},
//===============================================
//             mipi 2 lane 880Mbps             
//===============================================
	{0x0902, 0x431a},
	{0x0914, 0xc10c},
	{0x0916, 0x061d},
	{0x091c, 0x0e09},
	{0x0918, 0x0307},
	{0x091a, 0x0c0c},
	{0x091e, 0x0a00},
	//{0x0a00, 0x0100}, //stream on

	{HYNIX_TABLE_END, 0x0000},
};
#endif

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
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;
    /* initail sequence write in  */
#ifdef Hi553_I2C_BURST   //20151228 paul add 
	Hi842_write_burst_mode(hi842_init_setting); // I2C Burst Mode . Write Init Register 
#else
	sensor_init();
#endif

#ifdef HI_553_MIPI_USE_OTP

	if (0x02 == otp_module_id()) // module otp_module_id is 2
	{
		printk("yw read otp_module_id ok is 0x02\n");
		//return ERROR_NONE;
	}
	else 
	{
		printk("yw read otp_module_id fail\n");
	}

    hi553_gcore_update_otp();
#endif	

	set_mirror_flip(IMAGE_V_MIRROR);
    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
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
	//capture_setting();
	//set_mirror_flip(IMAGE_NORMAL);	
	mdelay(10);
	#ifdef FANPENGTAO
	int i=0;
	for(i=0; i<10; i++){
		LOG_INF("delay time = %d, the frame no = %d\n", i*10, read_cmos_sensor(0x0005));
		mdelay(10);
	}
	#endif
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
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} 
	else  
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
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps); 
	//set_mirror_flip(IMAGE_NORMAL);	
	mdelay(10);

#if 0
	if(imgsensor.test_pattern == KAL_TRUE)
	{
		 write_cmos_sensor(0x0a04, 0x0141);
		 write_cmos_sensor(0x0200, 0x0001);
		 write_cmos_sensor(0x0206, 0x000a);
		 write_cmos_sensor(0x0208, 0x0a0a);
		 write_cmos_sensor(0x020a, 0x000a);
		 write_cmos_sensor(0x020c, 0x0a0a);
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
	//set_mirror_flip(IMAGE_NORMAL);	
	
	
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
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
    imgsensor.pclk = imgsensor_info.custom1.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom1.linelength;
    imgsensor.frame_length = imgsensor_info.custom1.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom1   */

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom2   */

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
    imgsensor.pclk = imgsensor_info.custom3.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom3.linelength;
    imgsensor.frame_length = imgsensor_info.custom3.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom3   */

static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
    imgsensor.pclk = imgsensor_info.custom4.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom4.linelength;
    imgsensor.frame_length = imgsensor_info.custom4.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom4   */
static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
    imgsensor.pclk = imgsensor_info.custom5.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom5.linelength;
    imgsensor.frame_length = imgsensor_info.custom5.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom5   */
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
    sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
    sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;

    sensor_resolution->SensorCustom2Width  = imgsensor_info.custom2.grabwindow_width;
    sensor_resolution->SensorCustom2Height     = imgsensor_info.custom2.grabwindow_height;

    sensor_resolution->SensorCustom3Width  = imgsensor_info.custom3.grabwindow_width;
    sensor_resolution->SensorCustom3Height     = imgsensor_info.custom3.grabwindow_height;

    sensor_resolution->SensorCustom4Width  = imgsensor_info.custom4.grabwindow_width;
    sensor_resolution->SensorCustom4Height     = imgsensor_info.custom4.grabwindow_height;

    sensor_resolution->SensorCustom5Width  = imgsensor_info.custom5.grabwindow_width;
    sensor_resolution->SensorCustom5Height     = imgsensor_info.custom5.grabwindow_height;
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
    sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame; 
    sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame; 
    sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame; 
    sensor_info->Custom4DelayFrame = imgsensor_info.custom4_delay_frame; 
    sensor_info->Custom5DelayFrame = imgsensor_info.custom5_delay_frame; 

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
	#ifdef FPTPDAFSUPPORT
	sensor_info->PDAF_Support = 1;
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
        case MSDK_SCENARIO_ID_CUSTOM1:
            sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            sensor_info->SensorGrabStartX = imgsensor_info.custom4.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom4.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            sensor_info->SensorGrabStartX = imgsensor_info.custom5.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom5.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

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
        case MSDK_SCENARIO_ID_CUSTOM1:
            Custom1(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            Custom2(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            Custom3(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            Custom4(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            Custom5(image_window, sensor_config_data); // Custom1
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
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:	
			if(framerate==300)
			{
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			else
			{
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			break;	
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;	
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;
        case MSDK_SCENARIO_ID_CUSTOM1:
            frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM3:
            frame_length = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom3.framelength) ? (frame_length - imgsensor_info.custom3.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM4:
            frame_length = imgsensor_info.custom4.pclk / framerate * 10 / imgsensor_info.custom4.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom4.framelength) ? (frame_length - imgsensor_info.custom4.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom4.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM5:
            frame_length = imgsensor_info.custom5.pclk / framerate * 10 / imgsensor_info.custom5.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom5.framelength) ? (frame_length - imgsensor_info.custom5.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;	
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
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
        case MSDK_SCENARIO_ID_CUSTOM1:
            *framerate = imgsensor_info.custom1.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            *framerate = imgsensor_info.custom2.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            *framerate = imgsensor_info.custom3.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            *framerate = imgsensor_info.custom4.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            *framerate = imgsensor_info.custom5.max_framerate;
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
		 write_cmos_sensor(0x0a04, 0x014b);
		 write_cmos_sensor(0x0200, 0x0602);
		 write_cmos_sensor(0x0202, 0x0a40);
		 
		 LOG_INF(">>crc enable>> reg: %d\n", enable);
	} else {
		 write_cmos_sensor(0x0a04, 0x0140);
		 write_cmos_sensor(0x0200, 0x0000);
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
    //SET_PD_BLOCK_INFO_T *PDAFinfo;

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


UINT32 HI553_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	hi553_MIPI_RAW_SensorInit	*/



