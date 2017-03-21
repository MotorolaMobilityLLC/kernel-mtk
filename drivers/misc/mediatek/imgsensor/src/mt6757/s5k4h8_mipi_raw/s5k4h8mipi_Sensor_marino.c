/*****************************************************************************
 *
 * Filename:
 * ---------
 *   S5K4H8mipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description: 
 * [201512] 
 * ------------
 *	 Source code of Sensor driver
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

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k4h8mipi_Sensor.h"

#define PFX "S5K4H8_camera_sensor"
#define LOG_1 LOG_INF("S5K4H8,MIPI 4LANE\n")
#define LOG_2 LOG_INF("preview 2096*1552@30fps,1260Mbps/lane; video 4192*3104@30fps,1260Mbps/lane; capture 13M@30fps,1260Mbps/lane\n")
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
#define LOG_INF(fmt, args...)   pr_err(PFX "[%s] " fmt, __FUNCTION__, ##args)
#define LOGE(fmt, args...)   pr_err(PFX "[%s] " fmt, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);
static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = S5K4H8_SENSOR_ID,
	.checksum_value = 0x31f55cce,
	.pre = {
		.pclk = 280000000,				//record different mode's pclk
		.linelength = 3744,				//record different mode's linelength
		.framelength =1246, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1632,		//record different mode's width of grabwindow
		.grabwindow_height = 1224,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 280000000,
		.linelength =3744,
		.framelength = 2498,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,//5334,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
		},
	.cap1 = {
		.pclk = 140000000,        // 140000000
		.linelength = 3744,
		.framelength = 2492,      // 3870
		.startx = 0,
		.starty = 0,
		.grabwindow_width =3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 150,	
	},
	.normal_video = {
		.pclk = 280000000,
		.linelength = 3744,
		.framelength = 2492,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1632,//5334,
		.grabwindow_height = 1224,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 280000000,
		.linelength = 3744,
		.framelength = 640,
		.startx = 0,
		.starty = 0,
		.grabwindow_width =816, //1920,
		.grabwindow_height =612,// 1080,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 280000000,
		.linelength = 3744,
		.framelength = 2492,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,//1280,
		.grabwindow_height =720,// 720,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},
    .custom1 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
    .custom2 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
    .custom3 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
    .custom4 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
    .custom5 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.margin = 4,
	.min_shutter = 2,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 1,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	
	.cap_delay_frame = 2, 
	.pre_delay_frame = 2,  
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,
    .custom1_delay_frame = 2,
    .custom2_delay_frame = 2, 
    .custom3_delay_frame = 2, 
    .custom4_delay_frame = 2, 
    .custom5_delay_frame = 2,
	
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x20, 0xff},
    .i2c_speed = 300, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x5A,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] =	 
{
 { 3264, 2448,	  0,  0, 3264, 2448, 1632,  1224, 0000, 0000, 1632, 1224, 0,	0, 1632,  1224}, // Preview 
 { 3264, 2448,	  0,  0, 3264, 2448, 3264,  2448, 0000, 0000, 3264, 2448, 0,	0, 3264,  2448}, // capture 
 { 3264, 2448,	  0,  0, 3264, 2448, 1632,  1224, 0000, 0000, 1632, 1224, 0,	0, 1632,  1224}, // video 
 { 3264, 2448,	  0,  0, 3264, 2448,  816,   612, 0000, 0000,  816,  612, 0,	0,  816,   612},// hight video 120
 { 3264, 2448,	  0,  306, 3264, 1836, 1280,   720, 0000, 0000, 1280,  720, 0,	0, 1280,   720},// slim video 
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552}, // Custom1 (defaultuse preview) 
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552}, // Custom2 
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552}, // Custom3 
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552}, // Custom4 
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552}, // Custom5 
 };// slim video  
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}


static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
  
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
}


#define AWB_DATA_LENGTH  (12)   //all awb and module information length
#define S5K4H8_OTP_FUNCTION  (0)
#define AWB_VALID_DATA_SIZE   (12)
#define LSC_VALID_DATA_SIZE   (401)
#undef PAGE_SIZE
#define PAGE_SIZE (64)
#define PAGE_START_ADDR  (0x0A04)
#define AWB_PAGE     (15)
#define AWB_DATA_LEN        (12)
#define CHESUM_ADDR_LEN   (1)
#define AWB_FLAG_GROUP       (0x0A26)
#define MID_FLAG_GROUP1       (0x0A05)
#define MID_FLAG_GROUP2       (0x0A0F)
#define AWB_FLAG_ADDR_1   (0x0A27)
#define AWB_FLAG_ADDR_2   (0x0A34)
#define FLAG_GROUP_1          (0x40)           
#define FLAG_GROUP_2          (0x10)    
#define  PAGE1_SIZE               (7)
#define  PAGE2_SIZE                 (6)
#define CHECKSUM_DIVISOR     (0xFF)
#define GOLD_RG_VALUE          (574)
#define GOLD_BG_VALUE          (670)
typedef struct Lsc_Group
{
	kal_uint16 addr_start;
	kal_uint16 addr_end;
}Lsc_Group;
typedef struct Awb_otp
{
    kal_uint16 r_gain;
    kal_uint16 b_gain;
    kal_uint16 g_gain;
}Awb_otp;
typedef struct otp_Avail
{
	BOOL lsc_avail;
	BOOL awb_avail;
}otp_Avail;
static Awb_otp  s_Awb_otp = {0,0};
static otp_Avail s_otp_Avail = {FALSE,FALSE};
static kal_uint16 flag_otp_group = 0;
static kal_uint16 flag_mid_group1 = 0;
static kal_uint16 flag_mid_group2 = 0;
static BOOL read_otp(kal_uint16 Page, kal_uint16 address, kal_uint16 *iBuffer, int length)
{
    int i = 0;
    kal_uint16 tempValue;
    kal_uint16 bank = Page;
    kal_uint16 addr = address;
	while (i<length)
    	{
    	   LOG_INF("s5k4h8 OTP::awb   bank = %x\n",bank);
		write_cmos_sensor_8(0x0a02,bank);  //set register page
		write_cmos_sensor(0x0a00,0x0100);  //set register page
	        while(addr<PAGE_START_ADDR+PAGE_SIZE)
	        {
	            tempValue = read_cmos_sensor_8(addr);  
	   LOG_INF("s5k4h8 OTP::awb  addr = 0x%x, value = %x\n",addr,tempValue);
	            *(iBuffer+i) =tempValue;
	            addr ++;
	            i++;
	            if (i>=length) 
	            {
	                break;
	            }
	        }
		write_cmos_sensor(0x0a00,0x0000);  
	        addr  = PAGE_START_ADDR;	
	        bank ++;
    	}
    	return TRUE;
}
static BOOL read_lsc_otp(kal_uint16 flag_otp_page, kal_uint16 *iBuffer, kal_uint16 checksum )
{
	kal_uint16 tempValue;
	kal_uint16 addr ,bank;
	kal_uint32 lsc_sum = 0;
	int i = 0;
	Lsc_Group * m_address = NULL;
	unsigned char bank_num = 0;
	kal_uint16 *group_num = NULL;
	kal_uint16  group_1[PAGE1_SIZE] = {0x3,0x4,0x5,0x6,0x7,0x8,0x9};//lsc group 1 page num
	kal_uint16  group_2[PAGE2_SIZE] = {0x9,0xA,0xB,0xC,0xD,0xE};	 //lsc group 2 page num
	Lsc_Group  Lsc_Group_1[PAGE1_SIZE] = {{0x0A33,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A10}};
	Lsc_Group  Lsc_Group_2[PAGE2_SIZE] = {{0x0A1C,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43},{0x0A04,0x0A43}};
	 mdelay(10);
	 if(flag_otp_page== FLAG_GROUP_1)
	{
		m_address = Lsc_Group_1 ;
		bank_num = PAGE1_SIZE;
		group_num = group_1;
	}
	else  if(flag_otp_page== FLAG_GROUP_2)
	{
		m_address = Lsc_Group_2 ;
		bank_num = PAGE2_SIZE;
		group_num = group_2;
	}
	while (bank_num > 0)
    	{
    		bank = * group_num;
		LOG_INF("s5k4h8 OTP::lsc   bank = %x\n",bank);
		write_cmos_sensor_8(0x0a02,bank);  //set register page
		write_cmos_sensor(0x0a00,0x0100);  //set register page
		addr = m_address->addr_start;
	        while(addr <= m_address->addr_end)
	        {
			tempValue = read_cmos_sensor_8(addr);  
			*(iBuffer+i) =tempValue;
			  LOG_INF("s5k4h8 OTP::lsc  addr = 0x%X, value = %x\n",addr,tempValue);
			lsc_sum += *(iBuffer+i);
			addr++;
			i++;
	        }
		write_cmos_sensor(0x0a00,0x0000);  	
		m_address++;
		bank_num--;
		group_num++;
    	}
	if((lsc_sum%CHECKSUM_DIVISOR+1)!=checksum)
	{
		LOG_INF("s5k4h8 OTP::lsc check sum error:cal data = 0x%02x,otp_checksum = 0x%x\n",lsc_sum%CHECKSUM_DIVISOR+1,checksum);
		s_otp_Avail.lsc_avail = TRUE;
	//	return FALSE;
	}
	else
	{
		LOG_INF("s5k4h8 OTP::lsc check sum pass:cal data = 0x%02x,otp_checksum = 0x%x\n",lsc_sum%CHECKSUM_DIVISOR+1,checksum);
		s_otp_Avail.lsc_avail = TRUE;
	}
    return TRUE;
}
#if 1
static BOOL writer_awb_otp(kal_uint16 r_gain,kal_uint16 b_gain,kal_uint16 g_gain)
{
	write_cmos_sensor(0x6028,0x4000);  
	write_cmos_sensor(0x602A,0x3058);  
	write_cmos_sensor_8(0x6F12,0x01);  
	write_cmos_sensor(0x020e,g_gain);  
	write_cmos_sensor(0x0210,r_gain);  
	write_cmos_sensor(0x0212,b_gain);  
	write_cmos_sensor(0x0214,g_gain);  
	LOG_INF("s5k4h8 otp:writer_awb_otp success!\n");
	return TRUE;
}
#endif
static BOOL calculate_awb_data(kal_uint16 rg,kal_uint16 bg,kal_uint16 golden_rg, kal_uint16 golden_bg)
{
 	kal_uint16 r_ratio, b_ratio;
	kal_uint16 r_gain , g_gain, b_gain;
	kal_uint16 gr_gain, gb_gain;
	static kal_uint16 GAIN_DEFAULT = 0x0100;
	if((rg == 0)||(bg == 0))
	{
		LOG_INF("[%s] err: rg and bg can not be zero !\n", __func__);
		return FALSE;
	}
	r_ratio  = 512*golden_rg/rg ;
	b_ratio  = 512*golden_bg/bg ;
	if((!r_ratio)||(!b_ratio))
	{
		LOG_INF("[%s] err: r_ratio  and b_ratio can not be zero !\n", __func__);
		return FALSE;
	}
	if(r_ratio >= 512)
	{
		if(b_ratio >= 512)
		{
			r_gain = (kal_uint16)(GAIN_DEFAULT * r_ratio / 512);
			g_gain  = GAIN_DEFAULT ;
			b_gain = (kal_uint16)(GAIN_DEFAULT * b_ratio / 512);
		}
		else
		{
			r_gain = (kal_uint16)(GAIN_DEFAULT * r_ratio / b_ratio);
			g_gain  = (kal_uint16) (GAIN_DEFAULT *512 /b_ratio) ;
			b_gain = GAIN_DEFAULT ;
		}
	}
	else
	{
		if(b_ratio >= 512)
		{
			r_gain = GAIN_DEFAULT ;
			g_gain  = (kal_uint16) (GAIN_DEFAULT *512 /r_ratio) ; 
			b_gain = (kal_uint16)(GAIN_DEFAULT * b_ratio / r_ratio);
		}
		else
		{
			gr_gain =  (kal_uint16) (GAIN_DEFAULT *512 /r_ratio) ; 
			gb_gain = (kal_uint16)(GAIN_DEFAULT * 512 / b_ratio);
			if(gr_gain > gb_gain)
			{
				r_gain = GAIN_DEFAULT ;
				g_gain  = (kal_uint16) (GAIN_DEFAULT *512 /r_ratio) ;
				b_gain =  (kal_uint16)(GAIN_DEFAULT * b_ratio / r_ratio) ;
			}
			else
			{
				r_gain =  (kal_uint16)(GAIN_DEFAULT * r_ratio / b_ratio) ;
				g_gain = (kal_uint16)(GAIN_DEFAULT * 512 / b_ratio);
				b_gain = GAIN_DEFAULT ;
			}
		}
	}
	s_Awb_otp.r_gain = r_gain; //save awb data 
	s_Awb_otp.g_gain = g_gain; //save awb data 
	s_Awb_otp.b_gain = b_gain; //save awb data 
	LOG_INF("[%s] s5k4h5:r_gain =[0x%x],b_gain =[0x%x],g_gain =[0x%x] \n",__func__,r_gain,b_gain,g_gain);
	return TRUE;
}
static BOOL read_data_from_otp(void)
{
	kal_uint16 pTemp[AWB_VALID_DATA_SIZE+1]={0,};  
	kal_uint16 pTemp_lsc[LSC_VALID_DATA_SIZE+1]={0,};  
	kal_uint16 Length;
	kal_uint16 sum = 0;
	kal_uint16 awb_sum = 0;
	kal_uint16 lsc_sum = 0;
	kal_uint16 rg_gian, bg_gian,grgb_gain;
	kal_uint16 rg_gian_golden, bg_gian_golden,grgb_gain_golden;
	kal_uint16 MID = 0x07;//s5k4h8 MID
	kal_uint16 addr_start = 0xffff;
	int j = 0;
	 Length = 1;
	 LOG_INF("[%s]  read_s5k4h5_otp \n",__func__);

	if(!read_otp(AWB_PAGE, MID_FLAG_GROUP1,&flag_mid_group1,Length))
	{
		LOG_INF("[%s]  read s5k4h5 otp mid info0 flag err!\n",__func__);
	 	return FALSE;
	} 
	if(!read_otp(AWB_PAGE, MID_FLAG_GROUP2,&flag_mid_group2,Length))
	{
		LOG_INF("[%s]  read s5k4h5 otp mid info1  flag err!\n",__func__);
	 	return FALSE;
	}
	LOG_INF("[%s]  read s5k4h5 flag_mid_group1 = %d ,flag_mid_group2 = %d\n",__func__,flag_mid_group1,flag_mid_group2);
	if (flag_mid_group1 == MID  ) {
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0B00, 0x0180); //enable LSC 
	} else if (flag_mid_group2 == MID) {
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0B00, 0x0180); //enable LSC 
	} else {
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0B00, 0x0080); //disable LSC 
	}
	write_cmos_sensor(0x6028,0x4000);
	write_cmos_sensor(0x602A,0x0100);
	write_cmos_sensor_8(0x6F12,0x01);  //streamm on	
	mdelay(10);
	if(!read_otp(AWB_PAGE, MID_FLAG_GROUP1,&flag_mid_group1,Length))
	{
		LOG_INF("[%s]  read s5k4h5 otp mid info0 flag err!\n",__func__);
	 	return FALSE;
	} 
	if(!read_otp(AWB_PAGE, MID_FLAG_GROUP2,&flag_mid_group2,Length))
	{
		LOG_INF("[%s]  read s5k4h5 otp mid info1  flag err!\n",__func__);
	 	return FALSE;
	}
	LOG_INF("[%s] shen  read s5k4h5 flag_mid_group1 = %d ,flag_mid_group2 = %d\n",__func__,flag_mid_group1,flag_mid_group2);
	if(!read_otp(AWB_PAGE, AWB_FLAG_GROUP,&flag_otp_group,Length))
	{
		LOG_INF("[%s]  read s5k4h5 otp flag err!\n",__func__);
	 	return FALSE;
	}
	else
	{
		 if((flag_otp_group& 0xc0) == FLAG_GROUP_1)
		 {
			addr_start = AWB_FLAG_ADDR_1 ;
		 }
		 else  if((flag_otp_group& 0x30) == FLAG_GROUP_2)
		 {
			addr_start = AWB_FLAG_ADDR_2 ;
		 }
		 Length = AWB_DATA_LEN+CHESUM_ADDR_LEN;
		if(!read_otp(AWB_PAGE, addr_start,pTemp,Length))
		{
			LOG_INF("[%s]  read s5k4h5 otp err ! flag_otp_group(0x%x)\n",__func__, flag_otp_group);
			return FALSE;
		}
		 for (j=0;j<AWB_DATA_LEN;j++)
		{	
			 sum +=pTemp[j];
		}
		 awb_sum = pTemp[16];
		// lsc_sum = pTemp[9];
		if((sum%CHECKSUM_DIVISOR+1)!=awb_sum)
		{
			LOG_INF("s5k4h8 OTP::AWB check sum error:cal data = 0x%02x,otp_checksum = 0x%x\n",sum%CHECKSUM_DIVISOR+1,awb_sum);
			s_otp_Avail.awb_avail = TRUE; 
			//return FALSE;
		}
		else
		{
			s_otp_Avail.awb_avail = TRUE; 
		}
		if(pTemp[0]!=MID)
		{
			LOG_INF("s5k4h8 OTP::module information error:MID = %d,pTemp[2] = %d\n",MID,pTemp[1]);
		//	return FALSE;
		}
	}
	rg_gian =(pTemp[0] <<8 ) | (pTemp[1]&0xff) ;
	bg_gian = (pTemp[2] <<8 ) | (pTemp[3] &0xff);
	grgb_gain = (pTemp[4] <<8 ) | (pTemp[5]&0xff) ;
	LOG_INF("s5k4h8 OTP::rg_gian = %d, bg_gian = %d,grgb_gain = %d\n", rg_gian,bg_gian,grgb_gain);
	rg_gian_golden=(pTemp[6] <<8 ) | (pTemp[7]&0xff) ;
	bg_gian_golden = (pTemp[8] <<8 ) | (pTemp[9] &0xff);
	grgb_gain_golden = (pTemp[10] <<8 ) | (pTemp[11]&0xff) ;
	LOG_INF("s5k4h8 OTP::rg_gian_golden = %d, bg_gian_golden = %d,grgb_gain_golden = %d\n", rg_gian_golden,bg_gian_golden,grgb_gain_golden);
	calculate_awb_data(rg_gian,  bg_gian , rg_gian_golden, bg_gian_golden) ; 
      	read_lsc_otp(flag_otp_group, pTemp_lsc,lsc_sum);
    	return TRUE;
}
static void set_dummy(void)
{
	 LOG_INF("dummyline = %d, dummypixels = %d ", imgsensor.dummy_line, imgsensor.dummy_pixel);
    //write_cmos_sensor_8(0x0104, 0x01); 
    write_cmos_sensor(0x0340, imgsensor.frame_length);
    write_cmos_sensor(0x0342, imgsensor.line_length);
   // write_cmos_sensor_8(0x0104, 0x00); 
  
}	/*	set_dummy  */
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
//	kal_int16 dummy_line;
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
}	/*	set_max_framerate  */


static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;
   // kal_uint32 frame_length = 0;
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)       
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
 #if 1   
    if (imgsensor.autoflicker_en) { 
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
    } else {
        // Extend frame length
        //write_cmos_sensor_8(0x0104,0x01);
        write_cmos_sensor(0x0340, imgsensor.frame_length);
        //write_cmos_sensor_8(0x0104,0x00);
    }
#endif
    // Update Shutter
    //write_cmos_sensor_8(0x0104,0x01);
	write_cmos_sensor(0x0340, imgsensor.frame_length);
    write_cmos_sensor(0x0202, shutter);
    //write_cmos_sensor_8(0x0104,0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
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
#if 0
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	write_shutter(shutter);
}	/*	set_shutter */
 #endif


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;
    
    reg_gain = gain/2;
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
	  kal_uint16 reg_gain;

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
   // LOG_INF("gain = %d , reg_gain = 0x%x ", gain, reg_gain);

    //write_cmos_sensor_8(0x0104, 0x01);
    write_cmos_sensor_8(0x0204,(reg_gain>>8));
    write_cmos_sensor_8(0x0205,(reg_gain&0xff));
    //write_cmos_sensor_8(0x0104, 0x00);
    
    return gain;
}	/*	set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
   // kal_uint8 iRation;
  //  kal_uint8 iReg;

    LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
	se = le / 4;   // \B3\A4\C6ع\E2ʱ\BC\E4\CAǶ\CC\C6ع\E2ʱ\BC\E48\B1\B6
#if 1    
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

		LOG_INF("frame length:0x%x, le:0x%x, se:0x%x, gain:0x%x\n",imgsensor.frame_length,le,se,gain);

        // Extend frame length

		write_cmos_sensor_8(0x0104,0x01);
        
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		
        write_cmos_sensor(0x0202, le);
        write_cmos_sensor_8(0x0218, 0x04);
        write_cmos_sensor_8(0x021A, 0x01);

        set_gain(gain);
        
		write_cmos_sensor_8(0x0104,0x00);
    }
#endif

}

static void ihdr_write_shutter(kal_uint16 le, kal_uint16 se)
{
}



static void set_mirror_flip(kal_uint8 image_mirror)
{
#if 0
	LOG_INF("image_mirror = %d", image_mirror);

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
            write_cmos_sensor_8(0x0101,0x00);   // Gr
            break;
        case IMAGE_H_MIRROR:
            write_cmos_sensor_8(0x0101,0x01);
            break;
        case IMAGE_V_MIRROR:
            write_cmos_sensor_8(0x0101,0x02);
            break;
        case IMAGE_HV_MIRROR:
            write_cmos_sensor_8(0x0101,0x03);//Gb
            break;
        default:
			LOG_INF("Error image_mirror setting\n");
    }
#endif
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
static void sensor_init(void)
{
	kal_uint16 chip_id = 0;
	chip_id = read_cmos_sensor(0x0002);	
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// ***********************************************************************************
		// ATTENTION: DO NOT DELETE or CHANGE without prior notice to CHQ(or HQ) or local FAE.
		//            WE make NO WARRANTY on any use without prior consultation.
		// ***********************************************************************************
		//$MIPI[width:3264,height:2448,lane:4,format:raw10,datarate:700]
		////$MV1[MCLK:24,Width:3264,Height:2448,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:700,pvi_pclk_inverse:0]

		// Clock Gen
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7970);	// open all clocks
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);	// open all clocks

		// Start T&P part
		// DO NOT DELETE T&P SECTION COMMENTS! They are required to debug T&P related issues.
		// 2014/12/11 16:58:59
		// SVN Rev: WC
		// ROM Rev: 4H8YX_EVT0_Release
		// Signature:
		// md5 e6cc14a46965f19756062365fcd7a8ca .btp
		// md5 f2c378c99b1ff75ae4ccba6897d6fd29 .htp
		// md5 70b457628a3bff6cdd7d94fe8cb1bc73 .RegsMap.h
		// md5 2736958b1171f6f61187ee33237e1823 .RegsMap.bin
		//
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x1F30);
		write_cmos_sensor(0x6F12, 0x0448);
		write_cmos_sensor(0x6F12, 0x0349);
		write_cmos_sensor(0x6F12, 0x0160);
		write_cmos_sensor(0x6F12, 0xC26A);
		write_cmos_sensor(0x6F12, 0x511A);
		write_cmos_sensor(0x6F12, 0x8180);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x29BE);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2DE4);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1310);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x70B5);
		write_cmos_sensor(0x6F12, 0x0546);
		write_cmos_sensor(0x6F12, 0x0246);
		write_cmos_sensor(0x6F12, 0x47F2);
		write_cmos_sensor(0x6F12, 0xB604);
		write_cmos_sensor(0x6F12, 0x0821);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x87FE);
		write_cmos_sensor(0x6F12, 0xFE48);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x06D0);
		write_cmos_sensor(0x6F12, 0x2A46);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0x7040);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7CBE);
		write_cmos_sensor(0x6F12, 0x70BD);
		write_cmos_sensor(0x6F12, 0x70B5);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0xF848);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x4168);
		write_cmos_sensor(0x6F12, 0x0D0C);
		write_cmos_sensor(0x6F12, 0x8EB2);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x70FE);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x72FE);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x68FE);
		write_cmos_sensor(0x6F12, 0xF04D);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x2878);
		write_cmos_sensor(0x6F12, 0x10B1);
		write_cmos_sensor(0x6F12, 0x2078);
		write_cmos_sensor(0x6F12, 0x00B1);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xEE48);
		write_cmos_sensor(0x6F12, 0xEE4C);
		write_cmos_sensor(0x6F12, 0x4078);
		write_cmos_sensor(0x6F12, 0x48B1);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x2E00);
		write_cmos_sensor(0x6F12, 0x30B9);
		write_cmos_sensor(0x6F12, 0x6888);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x60FE);
		write_cmos_sensor(0x6F12, 0x6880);
		write_cmos_sensor(0x6F12, 0xA4F8);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0xE448);
		write_cmos_sensor(0x6F12, 0x0188);
		write_cmos_sensor(0x6F12, 0xE848);
		write_cmos_sensor(0x6F12, 0x01B1);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0xA4F8);
		write_cmos_sensor(0x6F12, 0x0611);
		write_cmos_sensor(0x6F12, 0xA0F8);
		write_cmos_sensor(0x6F12, 0xF810);
		write_cmos_sensor(0x6F12, 0x70BD);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF34F);
		write_cmos_sensor(0x6F12, 0xE04D);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0x95B0);
		write_cmos_sensor(0x6F12, 0xA87F);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x7ED0);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x6100);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0xFF31);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x0B90);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x58B0);
		write_cmos_sensor(0x6F12, 0x20F0);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x1190);
		write_cmos_sensor(0x6F12, 0x207E);
		write_cmos_sensor(0x6F12, 0x617E);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x010A);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x6310);
		write_cmos_sensor(0x6F12, 0xA07E);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x3BFE);
		write_cmos_sensor(0x6F12, 0x1290);
		write_cmos_sensor(0x6F12, 0xA07E);
		write_cmos_sensor(0x6F12, 0xE17E);
		write_cmos_sensor(0x6F12, 0x0844);
		write_cmos_sensor(0x6F12, 0x0090);
		write_cmos_sensor(0x6F12, 0x0790);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x6500);
		write_cmos_sensor(0x6F12, 0x00B1);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x0C90);
		write_cmos_sensor(0x6F12, 0xB4F8);
		write_cmos_sensor(0x6F12, 0x5020);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0009);
		write_cmos_sensor(0x6F12, 0xB2FB);
		write_cmos_sensor(0x6F12, 0xFAF0);
		write_cmos_sensor(0x6F12, 0x0AFB);
		write_cmos_sensor(0x6F12, 0x1021);
		write_cmos_sensor(0x6F12, 0xB4F8);
		write_cmos_sensor(0x6F12, 0x6E20);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x5200);
		write_cmos_sensor(0x6F12, 0x0D90);
		write_cmos_sensor(0x6F12, 0xCA48);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x5E00);
		write_cmos_sensor(0x6F12, 0x401A);
		write_cmos_sensor(0x6F12, 0x1390);
		write_cmos_sensor(0x6F12, 0x0C98);
		write_cmos_sensor(0x6F12, 0x30B1);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0xFF30);
		write_cmos_sensor(0x6F12, 0x0A90);
		write_cmos_sensor(0x6F12, 0xD5E9);
		write_cmos_sensor(0x6F12, 0x0406);
		write_cmos_sensor(0x6F12, 0x001F);
		write_cmos_sensor(0x6F12, 0x04E0);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x0A90);
		write_cmos_sensor(0x6F12, 0xD5E9);
		write_cmos_sensor(0x6F12, 0x0460);
		write_cmos_sensor(0x6F12, 0x001D);
		write_cmos_sensor(0x6F12, 0xBD49);
		write_cmos_sensor(0x6F12, 0x0990);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x1C80);
		write_cmos_sensor(0x6F12, 0x8868);
		write_cmos_sensor(0x6F12, 0x0390);
		write_cmos_sensor(0x6F12, 0xC968);
		write_cmos_sensor(0x6F12, 0x0491);
		write_cmos_sensor(0x6F12, 0xCDE9);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x6088);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x6210);
		write_cmos_sensor(0x6F12, 0xA38D);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x01FB);
		write_cmos_sensor(0x6F12, 0x0300);
		write_cmos_sensor(0x6F12, 0x0E90);
		write_cmos_sensor(0x6F12, 0xA088);
		write_cmos_sensor(0x6F12, 0x2188);
		write_cmos_sensor(0x6F12, 0x411A);
		write_cmos_sensor(0x6F12, 0xB1FB);
		write_cmos_sensor(0x6F12, 0xFAF0);
		write_cmos_sensor(0x6F12, 0x401E);
		write_cmos_sensor(0x6F12, 0x1490);
		write_cmos_sensor(0x6F12, 0x2DE1);
		write_cmos_sensor(0x6F12, 0x3068);
		write_cmos_sensor(0x6F12, 0x0A99);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x0043);
		write_cmos_sensor(0x6F12, 0x0693);
		write_cmos_sensor(0x6F12, 0x06EB);
		write_cmos_sensor(0x6F12, 0x8106);
		write_cmos_sensor(0x6F12, 0x0E9B);
		write_cmos_sensor(0x6F12, 0xC0F3);
		write_cmos_sensor(0x6F12, 0x0E41);
		write_cmos_sensor(0x6F12, 0x9942);
		write_cmos_sensor(0x6F12, 0xF2D3);
		write_cmos_sensor(0x6F12, 0x0C9B);
		write_cmos_sensor(0x6F12, 0x13B1);
		write_cmos_sensor(0x6F12, 0xE388);
		write_cmos_sensor(0x6F12, 0x5D1A);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x0E9B);
		write_cmos_sensor(0x6F12, 0xCD1A);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x64C0);
		write_cmos_sensor(0x6F12, 0x81B2);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x02D0);
		write_cmos_sensor(0x6F12, 0xA088);
		write_cmos_sensor(0x6F12, 0xA0EB);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0B98);
		write_cmos_sensor(0x6F12, 0x21EA);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x01D1);
		write_cmos_sensor(0x6F12, 0x1398);
		write_cmos_sensor(0x6F12, 0x0344);
		write_cmos_sensor(0x6F12, 0xB3FB);
		write_cmos_sensor(0x6F12, 0xFAF0);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0xE2E1);
		write_cmos_sensor(0x6F12, 0x0AFB);
		write_cmos_sensor(0x6F12, 0x1033);
		write_cmos_sensor(0x6F12, 0x23B1);
		write_cmos_sensor(0x6F12, 0x119F);
		write_cmos_sensor(0x6F12, 0x1F44);
		write_cmos_sensor(0x6F12, 0x5745);
		write_cmos_sensor(0x6F12, 0xD1D3);
		write_cmos_sensor(0x6F12, 0x401C);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x03D0);
		write_cmos_sensor(0x6F12, 0x149B);
		write_cmos_sensor(0x6F12, 0x9842);
		write_cmos_sensor(0x6F12, 0xCAD8);
		write_cmos_sensor(0x6F12, 0x03E0);
		write_cmos_sensor(0x6F12, 0x0D9B);
		write_cmos_sensor(0x6F12, 0x9842);
		write_cmos_sensor(0x6F12, 0xC6D3);
		write_cmos_sensor(0x6F12, 0xC01A);
		write_cmos_sensor(0x6F12, 0x60F3);
		write_cmos_sensor(0x6F12, 0x5F01);
		write_cmos_sensor(0x6F12, 0x0591);
		write_cmos_sensor(0x6F12, 0x4545);
		write_cmos_sensor(0x6F12, 0x01D3);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x7DD0);
		write_cmos_sensor(0x6F12, 0x0198);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x0C80);
		write_cmos_sensor(0x6F12, 0x0F90);
		write_cmos_sensor(0x6F12, 0x5F46);
		write_cmos_sensor(0x6F12, 0xA0EB);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x3CB2);
		write_cmos_sensor(0x6F12, 0x8010);
		write_cmos_sensor(0x6F12, 0x0890);
		write_cmos_sensor(0x6F12, 0xBBF8);
		write_cmos_sensor(0x6F12, 0xE200);
		write_cmos_sensor(0x6F12, 0x1090);
		write_cmos_sensor(0x6F12, 0x8C48);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x2C12);
		write_cmos_sensor(0x6F12, 0x0898);
		write_cmos_sensor(0x6F12, 0x8142);
		write_cmos_sensor(0x6F12, 0x04D2);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x1941);
		write_cmos_sensor(0x6F12, 0x3620);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9FFD);
		write_cmos_sensor(0x6F12, 0x8648);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xDE10);
		write_cmos_sensor(0x6F12, 0x0898);
		write_cmos_sensor(0x6F12, 0x09EB);
		write_cmos_sensor(0x6F12, 0x014B);
		write_cmos_sensor(0x6F12, 0x0128);
		write_cmos_sensor(0x6F12, 0x0CD0);
		write_cmos_sensor(0x6F12, 0x3AD9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x6FF0);
		write_cmos_sensor(0x6F12, 0x0042);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x95FD);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0xE081);
		write_cmos_sensor(0x6F12, 0x7F4A);
		write_cmos_sensor(0x6F12, 0x7F4B);
		write_cmos_sensor(0x6F12, 0x2BE0);
		write_cmos_sensor(0x6F12, 0xD8F8);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x1098);
		write_cmos_sensor(0x6F12, 0x0844);
		write_cmos_sensor(0x6F12, 0x7349);
		write_cmos_sensor(0x6F12, 0x40EA);
		write_cmos_sensor(0x6F12, 0x0B00);
		write_cmos_sensor(0x6F12, 0x0988);
		write_cmos_sensor(0x6F12, 0x51B1);
		write_cmos_sensor(0x6F12, 0x7A4A);
		write_cmos_sensor(0x6F12, 0x9742);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x5831);
		write_cmos_sensor(0x6F12, 0x3944);
		write_cmos_sensor(0x6F12, 0xC1F3);
		write_cmos_sensor(0x6F12, 0x8F01);
		write_cmos_sensor(0x6F12, 0x8029);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1746);
		write_cmos_sensor(0x6F12, 0x01C7);
		write_cmos_sensor(0x6F12, 0x18E0);
		write_cmos_sensor(0x6F12, 0x50F8);
		write_cmos_sensor(0x6F12, 0x04CB);
		write_cmos_sensor(0x6F12, 0x1099);
		write_cmos_sensor(0x6F12, 0x6144);
		write_cmos_sensor(0x6F12, 0xB8F8);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x6F12, 0x41EA);
		write_cmos_sensor(0x6F12, 0x0B01);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x09D0);
		write_cmos_sensor(0x6F12, 0x9F42);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x020C);
		write_cmos_sensor(0x6F12, 0xCCF3);
		write_cmos_sensor(0x6F12, 0x8F0C);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x800F);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1F46);
		write_cmos_sensor(0x6F12, 0x02C7);
		write_cmos_sensor(0x6F12, 0x0F99);
		write_cmos_sensor(0x6F12, 0x8842);
		write_cmos_sensor(0x6F12, 0xE6D1);
		write_cmos_sensor(0x6F12, 0x0298);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x1080);
		write_cmos_sensor(0x6F12, 0x0F90);
		write_cmos_sensor(0x6F12, 0xA0EB);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0x8010);
		write_cmos_sensor(0x6F12, 0x0890);
		write_cmos_sensor(0x6F12, 0x6048);
		write_cmos_sensor(0x6F12, 0x49F4);
		write_cmos_sensor(0x6F12, 0x8039);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xE2B0);
		write_cmos_sensor(0x6F12, 0x5E48);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x2C12);
		write_cmos_sensor(0x6F12, 0x0898);
		write_cmos_sensor(0x6F12, 0x8142);
		write_cmos_sensor(0x6F12, 0x04D2);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x1941);
		write_cmos_sensor(0x6F12, 0x3620);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x43FD);
		write_cmos_sensor(0x6F12, 0x5848);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xDE10);
		write_cmos_sensor(0x6F12, 0x0898);
		write_cmos_sensor(0x6F12, 0x09EB);
		write_cmos_sensor(0x6F12, 0x0149);
		write_cmos_sensor(0x6F12, 0x0128);
		write_cmos_sensor(0x6F12, 0x0ED0);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x4EE0);
		write_cmos_sensor(0x6F12, 0x3AD9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x6FF0);
		write_cmos_sensor(0x6F12, 0x0042);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x37FD);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x2481);
		write_cmos_sensor(0x6F12, 0x504A);
		write_cmos_sensor(0x6F12, 0x504B);
		write_cmos_sensor(0x6F12, 0x2BE0);
		write_cmos_sensor(0x6F12, 0xD8F8);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x0B00);
		write_cmos_sensor(0x6F12, 0x4449);
		write_cmos_sensor(0x6F12, 0x40EA);
		write_cmos_sensor(0x6F12, 0x0900);
		write_cmos_sensor(0x6F12, 0x0988);
		write_cmos_sensor(0x6F12, 0x51B1);
		write_cmos_sensor(0x6F12, 0x4B4A);
		write_cmos_sensor(0x6F12, 0x9742);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x5831);
		write_cmos_sensor(0x6F12, 0x3944);
		write_cmos_sensor(0x6F12, 0xC1F3);
		write_cmos_sensor(0x6F12, 0x8F01);
		write_cmos_sensor(0x6F12, 0x8029);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1746);
		write_cmos_sensor(0x6F12, 0x01C7);
		write_cmos_sensor(0x6F12, 0x18E0);
		write_cmos_sensor(0x6F12, 0x50F8);
		write_cmos_sensor(0x6F12, 0x04CB);
		write_cmos_sensor(0x6F12, 0x0CEB);
		write_cmos_sensor(0x6F12, 0x0B01);
		write_cmos_sensor(0x6F12, 0xB8F8);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x6F12, 0x41EA);
		write_cmos_sensor(0x6F12, 0x0901);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x09D0);
		write_cmos_sensor(0x6F12, 0x9F42);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x020C);
		write_cmos_sensor(0x6F12, 0xCCF3);
		write_cmos_sensor(0x6F12, 0x8F0C);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x800F);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1F46);
		write_cmos_sensor(0x6F12, 0x02C7);
		write_cmos_sensor(0x6F12, 0x0F99);
		write_cmos_sensor(0x6F12, 0x8842);
		write_cmos_sensor(0x6F12, 0xE6D1);
		write_cmos_sensor(0x6F12, 0x0398);
		write_cmos_sensor(0x6F12, 0x0190);
		write_cmos_sensor(0x6F12, 0x0498);
		write_cmos_sensor(0x6F12, 0x0290);
		write_cmos_sensor(0x6F12, 0x0098);
		write_cmos_sensor(0x6F12, 0x0099);
		write_cmos_sensor(0x6F12, 0xB5FB);
		write_cmos_sensor(0x6F12, 0xF0F0);
		write_cmos_sensor(0x6F12, 0x079A);
		write_cmos_sensor(0x6F12, 0x079B);
		write_cmos_sensor(0x6F12, 0x01FB);
		write_cmos_sensor(0x6F12, 0x1051);
		write_cmos_sensor(0x6F12, 0x4243);
		write_cmos_sensor(0x6F12, 0xBB46);
		write_cmos_sensor(0x6F12, 0x02EB);
		write_cmos_sensor(0x6F12, 0x0308);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x4049);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0xA91A);
		write_cmos_sensor(0x6F12, 0x1298);
		write_cmos_sensor(0x6F12, 0xC840);
		write_cmos_sensor(0x6F12, 0xC007);
		write_cmos_sensor(0x6F12, 0x17D0);
		write_cmos_sensor(0x6F12, 0x05F0);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0DF1);
		write_cmos_sensor(0x6F12, 0x040C);
		write_cmos_sensor(0x6F12, 0x03AB);
		write_cmos_sensor(0x6F12, 0x5CF8);
		write_cmos_sensor(0x6F12, 0x2100);
		write_cmos_sensor(0x6F12, 0x53F8);
		write_cmos_sensor(0x6F12, 0x2130);
		write_cmos_sensor(0x6F12, 0x9842);
		write_cmos_sensor(0x6F12, 0x06D0);
		write_cmos_sensor(0x6F12, 0xDDE9);
		write_cmos_sensor(0x6F12, 0x0535);
		write_cmos_sensor(0x6F12, 0x50F8);
		write_cmos_sensor(0x6F12, 0x047C);
		write_cmos_sensor(0x6F12, 0x2B43);
		write_cmos_sensor(0x6F12, 0x9F42);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0xDDE9);
		write_cmos_sensor(0x6F12, 0x0535);
		write_cmos_sensor(0x6F12, 0x2B43);
		write_cmos_sensor(0x6F12, 0x08C0);
		write_cmos_sensor(0x6F12, 0x4CF8);
		write_cmos_sensor(0x6F12, 0x2100);
		write_cmos_sensor(0x6F12, 0x0998);
		write_cmos_sensor(0x6F12, 0x8642);
		write_cmos_sensor(0x6F12, 0x7FF4);
		write_cmos_sensor(0x6F12, 0xCEAE);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x0480);
		write_cmos_sensor(0x6F12, 0x039D);
		write_cmos_sensor(0x6F12, 0x5C46);
		write_cmos_sensor(0x6F12, 0xA8EB);
		write_cmos_sensor(0x6F12, 0x0500);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0xA00A);
		write_cmos_sensor(0x6F12, 0x1848);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x58B0);
		write_cmos_sensor(0x6F12, 0x4F46);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x2C12);
		write_cmos_sensor(0x6F12, 0xBBF8);
		write_cmos_sensor(0x6F12, 0xE260);
		write_cmos_sensor(0x6F12, 0x5145);
		write_cmos_sensor(0x6F12, 0x03D2);
		write_cmos_sensor(0x6F12, 0x390C);
		write_cmos_sensor(0x6F12, 0x3620);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB3FC);
		write_cmos_sensor(0x6F12, 0xBBF8);
		write_cmos_sensor(0x6F12, 0xDE00);
		write_cmos_sensor(0x6F12, 0xBAF1);
		write_cmos_sensor(0x6F12, 0x010F);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x0047);
		write_cmos_sensor(0x6F12, 0x5046);
		write_cmos_sensor(0x6F12, 0x20D0);
		write_cmos_sensor(0x6F12, 0x43D9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x6FF0);
		write_cmos_sensor(0x6F12, 0x0042);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xA9FC);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x08C0);
		write_cmos_sensor(0x6F12, 0x0949);
		write_cmos_sensor(0x6F12, 0x0A4A);
		write_cmos_sensor(0x6F12, 0x36E0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3B00);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2DA0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1120);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x05C0);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xD000);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x7000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1340);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0C20);
		write_cmos_sensor(0x6F12, 0xDFFF);
		write_cmos_sensor(0x6F12, 0x6000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0xB000);
		write_cmos_sensor(0x6F12, 0xFF49);
		write_cmos_sensor(0x6F12, 0x2868);
		write_cmos_sensor(0x6F12, 0x3044);
		write_cmos_sensor(0x6F12, 0x0988);
		write_cmos_sensor(0x6F12, 0x3843);
		write_cmos_sensor(0x6F12, 0x51B1);
		write_cmos_sensor(0x6F12, 0xFD49);
		write_cmos_sensor(0x6F12, 0x8C42);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x5832);
		write_cmos_sensor(0x6F12, 0x2244);
		write_cmos_sensor(0x6F12, 0xC2F3);
		write_cmos_sensor(0x6F12, 0x8F02);
		write_cmos_sensor(0x6F12, 0x802A);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x0C46);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x10E0);
		write_cmos_sensor(0x6F12, 0x01CD);
		write_cmos_sensor(0x6F12, 0xBCF8);
		write_cmos_sensor(0x6F12, 0x0030);
		write_cmos_sensor(0x6F12, 0x3044);
		write_cmos_sensor(0x6F12, 0x3843);
		write_cmos_sensor(0x6F12, 0x3BB1);
		write_cmos_sensor(0x6F12, 0x9442);
		write_cmos_sensor(0x6F12, 0x05D2);
		write_cmos_sensor(0x6F12, 0x6318);
		write_cmos_sensor(0x6F12, 0xC3F3);
		write_cmos_sensor(0x6F12, 0x8F03);
		write_cmos_sensor(0x6F12, 0x802B);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1446);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x4545);
		write_cmos_sensor(0x6F12, 0xEED1);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x08A0);
		write_cmos_sensor(0x6F12, 0x049D);
		write_cmos_sensor(0x6F12, 0x49F4);
		write_cmos_sensor(0x6F12, 0x8036);
		write_cmos_sensor(0x6F12, 0xAAEB);
		write_cmos_sensor(0x6F12, 0x0500);
		write_cmos_sensor(0x6F12, 0x8710);
		write_cmos_sensor(0x6F12, 0xEB48);
		write_cmos_sensor(0x6F12, 0xBBF8);
		write_cmos_sensor(0x6F12, 0xE280);
		write_cmos_sensor(0x6F12, 0xD946);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x2C02);
		write_cmos_sensor(0x6F12, 0xB842);
		write_cmos_sensor(0x6F12, 0x03D2);
		write_cmos_sensor(0x6F12, 0x310C);
		write_cmos_sensor(0x6F12, 0x3620);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x52FC);
		write_cmos_sensor(0x6F12, 0xB9F8);
		write_cmos_sensor(0x6F12, 0xDE00);
		write_cmos_sensor(0x6F12, 0x012F);
		write_cmos_sensor(0x6F12, 0x06EB);
		write_cmos_sensor(0x6F12, 0x0046);
		write_cmos_sensor(0x6F12, 0x0AD0);
		write_cmos_sensor(0x6F12, 0x2ED9);
		write_cmos_sensor(0x6F12, 0x6FF0);
		write_cmos_sensor(0x6F12, 0x0042);
		write_cmos_sensor(0x6F12, 0x3946);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x4AFC);
		write_cmos_sensor(0x6F12, 0xDD4B);
		write_cmos_sensor(0x6F12, 0xDF49);
		write_cmos_sensor(0x6F12, 0xDD4A);
		write_cmos_sensor(0x6F12, 0x22E0);
		write_cmos_sensor(0x6F12, 0x2968);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0xD949);
		write_cmos_sensor(0x6F12, 0x3043);
		write_cmos_sensor(0x6F12, 0x0988);
		write_cmos_sensor(0x6F12, 0x51B1);
		write_cmos_sensor(0x6F12, 0xD849);
		write_cmos_sensor(0x6F12, 0x8C42);
		write_cmos_sensor(0x6F12, 0x07D2);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x5832);
		write_cmos_sensor(0x6F12, 0x2244);
		write_cmos_sensor(0x6F12, 0xC2F3);
		write_cmos_sensor(0x6F12, 0x8F02);
		write_cmos_sensor(0x6F12, 0x802A);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x0C46);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x10E0);
		write_cmos_sensor(0x6F12, 0x80CD);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0x1F88);
		write_cmos_sensor(0x6F12, 0x3043);
		write_cmos_sensor(0x6F12, 0x3FB1);
		write_cmos_sensor(0x6F12, 0x9442);
		write_cmos_sensor(0x6F12, 0x05D2);
		write_cmos_sensor(0x6F12, 0x6718);
		write_cmos_sensor(0x6F12, 0xC7F3);
		write_cmos_sensor(0x6F12, 0x8F07);
		write_cmos_sensor(0x6F12, 0x802F);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1446);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x5545);
		write_cmos_sensor(0x6F12, 0xEED1);
		write_cmos_sensor(0x6F12, 0x1698);
		write_cmos_sensor(0x6F12, 0x8442);
		write_cmos_sensor(0x6F12, 0x01D0);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0xC948);
		write_cmos_sensor(0x6F12, 0xC54A);
		write_cmos_sensor(0x6F12, 0x0170);
		write_cmos_sensor(0x6F12, 0x1288);
		write_cmos_sensor(0x6F12, 0x8021);
		write_cmos_sensor(0x6F12, 0x9AB1);
		write_cmos_sensor(0x6F12, 0xC34A);
		write_cmos_sensor(0x6F12, 0x9442);
		write_cmos_sensor(0x6F12, 0x05D9);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x5431);
		write_cmos_sensor(0x6F12, 0x2144);
		write_cmos_sensor(0x6F12, 0x8910);
		write_cmos_sensor(0x6F12, 0x8031);
		write_cmos_sensor(0x6F12, 0x02E0);
		write_cmos_sensor(0x6F12, 0x1699);
		write_cmos_sensor(0x6F12, 0x611A);
		write_cmos_sensor(0x6F12, 0x8910);
		write_cmos_sensor(0x6F12, 0x0184);
		write_cmos_sensor(0x6F12, 0x89B2);
		write_cmos_sensor(0x6F12, 0x8029);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x1446);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8071);
		write_cmos_sensor(0x6F12, 0x03E0);
		write_cmos_sensor(0x6F12, 0x169A);
		write_cmos_sensor(0x6F12, 0xA21A);
		write_cmos_sensor(0x6F12, 0x9210);
		write_cmos_sensor(0x6F12, 0x0284);
		write_cmos_sensor(0x6F12, 0x008C);
		write_cmos_sensor(0x6F12, 0x8842);
		write_cmos_sensor(0x6F12, 0x02D2);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0xFF30);
		write_cmos_sensor(0x6F12, 0x2060);
		write_cmos_sensor(0x6F12, 0x17B0);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF08F);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0xB64F);
		write_cmos_sensor(0x6F12, 0xB64D);
		write_cmos_sensor(0x6F12, 0x0E46);
		write_cmos_sensor(0x6F12, 0xF988);
		write_cmos_sensor(0x6F12, 0xA987);
		write_cmos_sensor(0x6F12, 0xB7F8);
		write_cmos_sensor(0x6F12, 0x1280);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0xB8F1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x1AD0);
		write_cmos_sensor(0x6F12, 0xA08A);
		write_cmos_sensor(0x6F12, 0x97F8);
		write_cmos_sensor(0x6F12, 0x9630);
		write_cmos_sensor(0x6F12, 0x1BB1);
		write_cmos_sensor(0x6F12, 0xAF4B);
		write_cmos_sensor(0x6F12, 0x1B6E);
		write_cmos_sensor(0x6F12, 0x4343);
		write_cmos_sensor(0x6F12, 0x180B);
		write_cmos_sensor(0x6F12, 0x3B8A);
		write_cmos_sensor(0x6F12, 0xB0EB);
		write_cmos_sensor(0x6F12, 0xC30F);
		write_cmos_sensor(0x6F12, 0x04D8);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x48C0);
		write_cmos_sensor(0x6F12, 0x87F8);
		write_cmos_sensor(0x6F12, 0x04C0);
		write_cmos_sensor(0x6F12, 0x0BE0);
		write_cmos_sensor(0x6F12, 0xB0EB);
		write_cmos_sensor(0x6F12, 0xC80F);
		write_cmos_sensor(0x6F12, 0x02D3);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x4820);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x48C0);
		write_cmos_sensor(0x6F12, 0x3A71);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x4820);
		write_cmos_sensor(0x6F12, 0xA34B);
		write_cmos_sensor(0x6F12, 0x3879);
		write_cmos_sensor(0x6F12, 0x1880);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x4A80);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x4830);
		write_cmos_sensor(0x6F12, 0x3BB1);
		write_cmos_sensor(0x6F12, 0xA048);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x2C01);
		write_cmos_sensor(0x6F12, 0x18B1);
		write_cmos_sensor(0x6F12, 0xC806);
		write_cmos_sensor(0x6F12, 0x01D5);
		write_cmos_sensor(0x6F12, 0xC807);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x21F0);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0xA887);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x4A20);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x4AC0);
		write_cmos_sensor(0x6F12, 0xC1F3);
		write_cmos_sensor(0x6F12, 0x4010);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x03B1);
		write_cmos_sensor(0x6F12, 0xEEB3);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x8AB2);
		write_cmos_sensor(0x6F12, 0xA88F);
		write_cmos_sensor(0x6F12, 0x2021);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x95FB);
		write_cmos_sensor(0x6F12, 0x9249);
		write_cmos_sensor(0x6F12, 0xA887);
		write_cmos_sensor(0x6F12, 0x891F);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x4A10);
		write_cmos_sensor(0x6F12, 0x4145);
		write_cmos_sensor(0x6F12, 0x04D0);
		write_cmos_sensor(0x6F12, 0x97F8);
		write_cmos_sensor(0x6F12, 0x9700);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9CFB);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9EFB);
		write_cmos_sensor(0x6F12, 0x207C);
		write_cmos_sensor(0x6F12, 0x30B3);
		write_cmos_sensor(0x6F12, 0x7889);
		write_cmos_sensor(0x6F12, 0xA5F8);
		write_cmos_sensor(0x6F12, 0x4400);
		write_cmos_sensor(0x6F12, 0xF889);
		write_cmos_sensor(0x6F12, 0xA5F8);
		write_cmos_sensor(0x6F12, 0x4600);
		write_cmos_sensor(0x6F12, 0xB6B3);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x35F8);
		write_cmos_sensor(0x6F12, 0x440F);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x76FB);
		write_cmos_sensor(0x6F12, 0x8249);
		write_cmos_sensor(0x6F12, 0x2880);
		write_cmos_sensor(0x6F12, 0x091F);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x6888);
		write_cmos_sensor(0x6F12, 0x01F1);
		write_cmos_sensor(0x6F12, 0x0201);
		write_cmos_sensor(0x6F12, 0xA5F1);
		write_cmos_sensor(0x6F12, 0x4405);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x7F48);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x8C02);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7AFB);
		write_cmos_sensor(0x6F12, 0x387D);
		write_cmos_sensor(0x6F12, 0x18B1);
		write_cmos_sensor(0x6F12, 0xA18A);
		write_cmos_sensor(0x6F12, 0x2068);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7EFB);
		write_cmos_sensor(0x6F12, 0x97F8);
		write_cmos_sensor(0x6F12, 0x3200);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x17E0);
		write_cmos_sensor(0x6F12, 0x19E0);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7F01);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x5B00);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x8842);
		write_cmos_sensor(0x6F12, 0x00D3);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x8021);
		write_cmos_sensor(0x6F12, 0x49F2);
		write_cmos_sensor(0x6F12, 0x3260);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x44FB);
		write_cmos_sensor(0x6F12, 0x95F8);
		write_cmos_sensor(0x6F12, 0x4A00);
		write_cmos_sensor(0x6F12, 0xB7F8);
		write_cmos_sensor(0x6F12, 0x9A10);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x0ED0);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x6F12, 0x0DE0);
		write_cmos_sensor(0x6F12, 0x07E0);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0xA6E7);
		write_cmos_sensor(0x6F12, 0x3889);
		write_cmos_sensor(0x6F12, 0xA5F8);
		write_cmos_sensor(0x6F12, 0x4400);
		write_cmos_sensor(0x6F12, 0xB889);
		write_cmos_sensor(0x6F12, 0xBCE7);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0102);
		write_cmos_sensor(0x6F12, 0xBDE7);
		write_cmos_sensor(0x6F12, 0xB7F9);
		write_cmos_sensor(0x6F12, 0x2A20);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x0200);
		write_cmos_sensor(0x6F12, 0x01D0);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x634A);
		write_cmos_sensor(0x6F12, 0x1080);
		write_cmos_sensor(0x6F12, 0x6348);
		write_cmos_sensor(0x6F12, 0x0180);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF081);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0x1046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x4BFB);
		write_cmos_sensor(0x6F12, 0x5848);
		write_cmos_sensor(0x6F12, 0x6049);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x9A00);
		write_cmos_sensor(0x6F12, 0x4042);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x524B);
		write_cmos_sensor(0x6F12, 0x891C);
		write_cmos_sensor(0x6F12, 0xB3F8);
		write_cmos_sensor(0x6F12, 0x1C02);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x93F8);
		write_cmos_sensor(0x6F12, 0x2302);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x17D0);
		write_cmos_sensor(0x6F12, 0x524A);
		write_cmos_sensor(0x6F12, 0xB2F8);
		write_cmos_sensor(0x6F12, 0x1C11);
		write_cmos_sensor(0x6F12, 0x491C);
		write_cmos_sensor(0x6F12, 0x89B2);
		write_cmos_sensor(0x6F12, 0xA2F8);
		write_cmos_sensor(0x6F12, 0x1C11);
		write_cmos_sensor(0x6F12, 0xB1F5);
		write_cmos_sensor(0x6F12, 0x804F);
		write_cmos_sensor(0x6F12, 0x02D3);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0xA2F8);
		write_cmos_sensor(0x6F12, 0x1C11);
		write_cmos_sensor(0x6F12, 0x5249);
		write_cmos_sensor(0x6F12, 0x1831);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x891C);
		write_cmos_sensor(0x6F12, 0xB2F8);
		write_cmos_sensor(0x6F12, 0x1C01);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x091F);
		write_cmos_sensor(0x6F12, 0x93F8);
		write_cmos_sensor(0x6F12, 0x2402);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xFE4F);
		write_cmos_sensor(0x6F12, 0x424F);
		write_cmos_sensor(0x6F12, 0x8246);
		write_cmos_sensor(0x6F12, 0x7868);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x76D1);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x2891);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0xF480);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0xA9F8);
		write_cmos_sensor(0x6F12, 0x2410);
		write_cmos_sensor(0x6F12, 0x98F8);
		write_cmos_sensor(0x6F12, 0x2602);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x6BD0);
		write_cmos_sensor(0x6F12, 0x454D);
		write_cmos_sensor(0x6F12, 0x2868);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x7840);
		write_cmos_sensor(0x6F12, 0x002C);
		write_cmos_sensor(0x6F12, 0x65D0);
		write_cmos_sensor(0x6F12, 0x0422);
		write_cmos_sensor(0x6F12, 0x02A9);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x0BFB);
		write_cmos_sensor(0x6F12, 0x0298);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x5DD0);
		write_cmos_sensor(0x6F12, 0x2868);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x7C00);
		write_cmos_sensor(0x6F12, 0x3883);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x07FB);
		write_cmos_sensor(0x6F12, 0x80B2);
		write_cmos_sensor(0x6F12, 0xB883);
		write_cmos_sensor(0x6F12, 0x398B);
		write_cmos_sensor(0x6F12, 0x8142);
		write_cmos_sensor(0x6F12, 0x12D9);
		write_cmos_sensor(0x6F12, 0x2868);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x7A00);
		write_cmos_sensor(0x6F12, 0x60B1);
		write_cmos_sensor(0x6F12, 0x2820);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x00FB);
		write_cmos_sensor(0x6F12, 0x2868);
		write_cmos_sensor(0x6F12, 0x98F8);
		write_cmos_sensor(0x6F12, 0x2612);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x7A00);
		write_cmos_sensor(0x6F12, 0x18B1);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x6F12, 0xFCD1);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0xB88B);
		write_cmos_sensor(0x6F12, 0x3883);
		write_cmos_sensor(0x6F12, 0x388B);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xF5FA);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xF8FA);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x7860);
		write_cmos_sensor(0x6F12, 0x98F8);
		write_cmos_sensor(0x6F12, 0x2602);
		write_cmos_sensor(0x6F12, 0x18B1);
		write_cmos_sensor(0x6F12, 0x388B);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0xB877);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x65D0);
		write_cmos_sensor(0x6F12, 0x0026);
		write_cmos_sensor(0x6F12, 0x3961);
		write_cmos_sensor(0x6F12, 0x3546);
		write_cmos_sensor(0x6F12, 0x0091);
		write_cmos_sensor(0x6F12, 0x14E0);
		write_cmos_sensor(0x6F12, 0x98F8);
		write_cmos_sensor(0x6F12, 0x2702);
		write_cmos_sensor(0x6F12, 0x28B1);
		write_cmos_sensor(0x6F12, 0x0422);
		write_cmos_sensor(0x6F12, 0x01A9);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCAFA);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x2068);
		write_cmos_sensor(0x6F12, 0x0190);
		write_cmos_sensor(0x6F12, 0x0199);
		write_cmos_sensor(0x6F12, 0x241D);
		write_cmos_sensor(0x6F12, 0x49B1);
		write_cmos_sensor(0x6F12, 0x5246);
		write_cmos_sensor(0x6F12, 0x6846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD9FA);
		write_cmos_sensor(0x6F12, 0x761C);
		write_cmos_sensor(0x6F12, 0xB6B2);
		write_cmos_sensor(0x6F12, 0x6D1C);
		write_cmos_sensor(0x6F12, 0x388B);
		write_cmos_sensor(0x6F12, 0xA842);
		write_cmos_sensor(0x6F12, 0xE7D8);
		write_cmos_sensor(0x6F12, 0x0099);
		write_cmos_sensor(0x6F12, 0x6FF0);
		write_cmos_sensor(0x6F12, 0x0042);
		write_cmos_sensor(0x6F12, 0x081F);
		write_cmos_sensor(0x6F12, 0x7861);
		write_cmos_sensor(0x6F12, 0x3869);
		write_cmos_sensor(0x6F12, 0x091A);
		write_cmos_sensor(0x6F12, 0x8C08);
		write_cmos_sensor(0x6F12, 0x2146);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x96FA);
		write_cmos_sensor(0x6F12, 0x388B);
		write_cmos_sensor(0x6F12, 0xC4EB);
		write_cmos_sensor(0x6F12, 0x8000);
		write_cmos_sensor(0x6F12, 0x8000);
		write_cmos_sensor(0x6F12, 0x21E0);
		write_cmos_sensor(0x6F12, 0x37E0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3B00);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0xB000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0C20);
		write_cmos_sensor(0x6F12, 0xDFFF);
		write_cmos_sensor(0x6F12, 0x6000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1120);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0B60);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1840);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1990);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x9606);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1340);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1550);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x9626);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x965C);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x9B06);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1310);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0560);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xA4FA);
		write_cmos_sensor(0x6F12, 0x7868);
		write_cmos_sensor(0x6F12, 0x08F5);
		write_cmos_sensor(0x6F12, 0x0A75);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x8400);
		write_cmos_sensor(0x6F12, 0xB860);
		write_cmos_sensor(0x6F12, 0xA988);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x8100);
		write_cmos_sensor(0x6F12, 0xF860);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7FFA);
		write_cmos_sensor(0x6F12, 0x7883);
		write_cmos_sensor(0x6F12, 0x2888);
		write_cmos_sensor(0x6F12, 0x6988);
		write_cmos_sensor(0x6F12, 0x04FB);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x8008);
		write_cmos_sensor(0x6F12, 0xA9F8);
		write_cmos_sensor(0x6F12, 0x2400);
		write_cmos_sensor(0x6F12, 0x3E83);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xFE8F);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF05F);
		write_cmos_sensor(0x6F12, 0x8146);
		write_cmos_sensor(0x6F12, 0xFE48);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xC068);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x104B);
		write_cmos_sensor(0x6F12, 0x80B2);
		write_cmos_sensor(0x6F12, 0x8246);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x5846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x2FFA);
		write_cmos_sensor(0x6F12, 0x4846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x81FA);
		write_cmos_sensor(0x6F12, 0xF84F);
		write_cmos_sensor(0x6F12, 0x7888);
		write_cmos_sensor(0x6F12, 0xA8B3);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x0058);
		write_cmos_sensor(0x6F12, 0x09EB);
		write_cmos_sensor(0x6F12, 0x8003);
		write_cmos_sensor(0x6F12, 0x09EB);
		write_cmos_sensor(0x6F12, 0x0004);
		write_cmos_sensor(0x6F12, 0x9A6B);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x4850);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0x4C60);
		write_cmos_sensor(0x6F12, 0x996A);
		write_cmos_sensor(0x6F12, 0x4545);
		write_cmos_sensor(0x6F12, 0x23D0);
		write_cmos_sensor(0x6F12, 0x4645);
		write_cmos_sensor(0x6F12, 0x21D0);
		write_cmos_sensor(0x6F12, 0x4145);
		write_cmos_sensor(0x6F12, 0x1FD0);
		write_cmos_sensor(0x6F12, 0x4245);
		write_cmos_sensor(0x6F12, 0x1DD0);
		write_cmos_sensor(0x6F12, 0x4145);
		write_cmos_sensor(0x6F12, 0x04D2);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x4005);
		write_cmos_sensor(0x6F12, 0xB5F8);
		write_cmos_sensor(0x6F12, 0x04C0);
		write_cmos_sensor(0x6F12, 0x04E0);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x400C);
		write_cmos_sensor(0x6F12, 0xBCF8);
		write_cmos_sensor(0x6F12, 0x04C0);
		write_cmos_sensor(0x6F12, 0xAC44);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x4005);
		write_cmos_sensor(0x6F12, 0x4245);
		write_cmos_sensor(0x6F12, 0xAD89);
		write_cmos_sensor(0x6F12, 0x00D3);
		write_cmos_sensor(0x6F12, 0x3544);
		write_cmos_sensor(0x6F12, 0x84F8);
		write_cmos_sensor(0x6F12, 0x48C0);
		write_cmos_sensor(0x6F12, 0x84F8);
		write_cmos_sensor(0x6F12, 0x4C50);
		write_cmos_sensor(0x6F12, 0x07EB);
		write_cmos_sensor(0x6F12, 0x4004);
		write_cmos_sensor(0x6F12, 0xA588);
		write_cmos_sensor(0x6F12, 0xE940);
		write_cmos_sensor(0x6F12, 0x9962);
		write_cmos_sensor(0x6F12, 0xA189);
		write_cmos_sensor(0x6F12, 0xCA40);
		write_cmos_sensor(0x6F12, 0x9A63);
		write_cmos_sensor(0x6F12, 0x401C);
		write_cmos_sensor(0x6F12, 0x0428);
		write_cmos_sensor(0x6F12, 0xCCDB);
		write_cmos_sensor(0x6F12, 0x5146);
		write_cmos_sensor(0x6F12, 0x5846);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF05F);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xECB9);
		write_cmos_sensor(0x6F12, 0x38B5);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0x00F5);
		write_cmos_sensor(0x6F12, 0x9970);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x16FA);
		write_cmos_sensor(0x6F12, 0x9DF8);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xFF28);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x04F2);
		write_cmos_sensor(0x6F12, 0x3314);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x38BD);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0xF8E7);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xFE43);
		write_cmos_sensor(0x6F12, 0x0024);
		write_cmos_sensor(0x6F12, 0x8046);
		write_cmos_sensor(0x6F12, 0x0E46);
		write_cmos_sensor(0x6F12, 0x2546);
		write_cmos_sensor(0x6F12, 0x6F46);
		write_cmos_sensor(0x6F12, 0x05EB);
		write_cmos_sensor(0x6F12, 0xC501);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0x0922);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xFBF9);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0xB85C);
		write_cmos_sensor(0x6F12, 0xC1F1);
		write_cmos_sensor(0x6F12, 0x0703);
		write_cmos_sensor(0x6F12, 0xD840);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x0103);
		write_cmos_sensor(0x6F12, 0x0819);
		write_cmos_sensor(0x6F12, 0x012B);
		write_cmos_sensor(0x6F12, 0x3354);
		write_cmos_sensor(0x6F12, 0x08D1);
		write_cmos_sensor(0x6F12, 0x38B1);
		write_cmos_sensor(0x6F12, 0x2428);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x4828);
		write_cmos_sensor(0x6F12, 0x03D0);
		write_cmos_sensor(0x6F12, 0x6C28);
		write_cmos_sensor(0x6F12, 0x01D0);
		write_cmos_sensor(0x6F12, 0x0F23);
		write_cmos_sensor(0x6F12, 0x3354);
		write_cmos_sensor(0x6F12, 0x491C);
		write_cmos_sensor(0x6F12, 0x0829);
		write_cmos_sensor(0x6F12, 0xE9DB);
		write_cmos_sensor(0x6F12, 0x0834);
		write_cmos_sensor(0x6F12, 0x521C);
		write_cmos_sensor(0x6F12, 0x092A);
		write_cmos_sensor(0x6F12, 0xE4DB);
		write_cmos_sensor(0x6F12, 0x6D1C);
		write_cmos_sensor(0x6F12, 0x022D);
		write_cmos_sensor(0x6F12, 0xD8DB);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xFE83);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0xB64F);
		write_cmos_sensor(0x6F12, 0x8846);
		write_cmos_sensor(0x6F12, 0x07F1);
		write_cmos_sensor(0x6F12, 0x1106);
		write_cmos_sensor(0x6F12, 0xA237);
		write_cmos_sensor(0x6F12, 0xB0B1);
		write_cmos_sensor(0x6F12, 0xB348);
		write_cmos_sensor(0x6F12, 0x0478);
		write_cmos_sensor(0x6F12, 0x9CB1);
		write_cmos_sensor(0x6F12, 0xB348);
		write_cmos_sensor(0x6F12, 0x0068);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x6A00);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xABFF);
		write_cmos_sensor(0x6F12, 0x85B2);
		write_cmos_sensor(0x6F12, 0xE007);
		write_cmos_sensor(0x6F12, 0x0AD0);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xB8FF);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x9072);
		write_cmos_sensor(0x6F12, 0x3946);
		write_cmos_sensor(0x6F12, 0x05F1);
		write_cmos_sensor(0x6F12, 0x1200);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xBBF9);
		write_cmos_sensor(0x6F12, 0xB8F1);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x3946);
		write_cmos_sensor(0x6F12, 0x3046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xDAB9);
		write_cmos_sensor(0x6F12, 0x5AE6);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0xB0FA);
		write_cmos_sensor(0x6F12, 0x80F0);
		write_cmos_sensor(0x6F12, 0xC0F1);
		write_cmos_sensor(0x6F12, 0x1F00);
		write_cmos_sensor(0x6F12, 0x0521);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD5F9);
		write_cmos_sensor(0x6F12, 0x401F);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x8140);
		write_cmos_sensor(0x6F12, 0x491E);
		write_cmos_sensor(0x6F12, 0x8C43);
		write_cmos_sensor(0x6F12, 0xA0B2);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x7082);
		write_cmos_sensor(0x6F12, 0x0646);
		write_cmos_sensor(0x6F12, 0x0F46);
		write_cmos_sensor(0x6F12, 0xB8F8);
		write_cmos_sensor(0x6F12, 0x2A00);
		write_cmos_sensor(0x6F12, 0x317F);
		write_cmos_sensor(0x6F12, 0xC308);
		write_cmos_sensor(0x6F12, 0x7088);
		write_cmos_sensor(0x6F12, 0x1546);
		write_cmos_sensor(0x6F12, 0xC840);
		write_cmos_sensor(0x6F12, 0x9842);
		write_cmos_sensor(0x6F12, 0x00D2);
		write_cmos_sensor(0x6F12, 0x0346);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0x48C2);
		write_cmos_sensor(0x6F12, 0x1C46);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x01EB);
		write_cmos_sensor(0x6F12, 0x4100);
		write_cmos_sensor(0x6F12, 0x0CEB);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x30F8);
		write_cmos_sensor(0x6F12, 0x142F);
		write_cmos_sensor(0x6F12, 0x6AB1);
		write_cmos_sensor(0x6F12, 0xA242);
		write_cmos_sensor(0x6F12, 0x08D8);
		write_cmos_sensor(0x6F12, 0x4288);
		write_cmos_sensor(0x6F12, 0xA242);
		write_cmos_sensor(0x6F12, 0x05D3);
		write_cmos_sensor(0x6F12, 0x0179);
		write_cmos_sensor(0x6F12, 0x09B1);
		write_cmos_sensor(0x6F12, 0x93B2);
		write_cmos_sensor(0x6F12, 0x04E0);
		write_cmos_sensor(0x6F12, 0x0388);
		write_cmos_sensor(0x6F12, 0x02E0);
		write_cmos_sensor(0x6F12, 0x491C);
		write_cmos_sensor(0x6F12, 0x0529);
		write_cmos_sensor(0x6F12, 0xEADB);
		write_cmos_sensor(0x6F12, 0x1846);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xC4FF);
		write_cmos_sensor(0x6F12, 0xF881);
		write_cmos_sensor(0x6F12, 0xC004);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0xA882);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xBDFF);
		write_cmos_sensor(0x6F12, 0xC004);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x98F8);
		write_cmos_sensor(0x6F12, 0x1E00);
		write_cmos_sensor(0x6F12, 0x40B1);
		write_cmos_sensor(0x6F12, 0x327F);
		write_cmos_sensor(0x6F12, 0x7088);
		write_cmos_sensor(0x6F12, 0xC2F1);
		write_cmos_sensor(0x6F12, 0x0F02);
		write_cmos_sensor(0x6F12, 0x9040);
		write_cmos_sensor(0x6F12, 0xAA8A);
		write_cmos_sensor(0x6F12, 0x90FB);
		write_cmos_sensor(0x6F12, 0xF2F0);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8050);
		write_cmos_sensor(0x6F12, 0xA861);
		write_cmos_sensor(0x6F12, 0x05F1);
		write_cmos_sensor(0x6F12, 0x1800);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x8AF9);
		write_cmos_sensor(0x6F12, 0xF889);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x8CF9);
		write_cmos_sensor(0x6F12, 0x3882);
		write_cmos_sensor(0x6F12, 0x2877);
		write_cmos_sensor(0x6F12, 0xFBE5);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0546);
		write_cmos_sensor(0x6F12, 0x0079);
		write_cmos_sensor(0x6F12, 0x0C46);
		write_cmos_sensor(0x6F12, 0x40F3);
		write_cmos_sensor(0x6F12, 0x0016);
		write_cmos_sensor(0x6F12, 0x761C);
		write_cmos_sensor(0x6F12, 0x4FF0);
		write_cmos_sensor(0x6F12, 0x0107);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0xE988);
		write_cmos_sensor(0x6F12, 0xA182);
		write_cmos_sensor(0x6F12, 0x6979);
		write_cmos_sensor(0x6F12, 0xA175);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x02E0);
		write_cmos_sensor(0x6F12, 0xA775);
		write_cmos_sensor(0x6F12, 0xA782);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x8021);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x13F9);
		write_cmos_sensor(0x6F12, 0x6749);
		write_cmos_sensor(0x6F12, 0x498E);
		write_cmos_sensor(0x6F12, 0xE175);
		write_cmos_sensor(0x6F12, 0x6989);
		write_cmos_sensor(0x6F12, 0x21B1);
		write_cmos_sensor(0x6F12, 0x2A7A);
		write_cmos_sensor(0x6F12, 0x12B1);
		write_cmos_sensor(0x6F12, 0x0EB1);
		write_cmos_sensor(0x6F12, 0x0126);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x0026);
		write_cmos_sensor(0x6F12, 0x8EB1);
		write_cmos_sensor(0x6F12, 0x2183);
		write_cmos_sensor(0x6F12, 0x297A);
		write_cmos_sensor(0x6F12, 0xA176);
		write_cmos_sensor(0x6F12, 0x297B);
		write_cmos_sensor(0x6F12, 0x01F0);
		write_cmos_sensor(0x6F12, 0x0701);
		write_cmos_sensor(0x6F12, 0xE176);
		write_cmos_sensor(0x6F12, 0xA18A);
		write_cmos_sensor(0x6F12, 0x09B1);
		write_cmos_sensor(0x6F12, 0xA17D);
		write_cmos_sensor(0x6F12, 0x41B9);
		write_cmos_sensor(0x6F12, 0xA169);
		write_cmos_sensor(0x6F12, 0x6161);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x8021);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xF6F8);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0x6169);
		write_cmos_sensor(0x6F12, 0xA161);
		write_cmos_sensor(0x6F12, 0x3246);
		write_cmos_sensor(0x6F12, 0x4021);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xEFF8);
		write_cmos_sensor(0x6F12, 0x574E);
		write_cmos_sensor(0x6F12, 0x2077);
		write_cmos_sensor(0x6F12, 0x3068);
		write_cmos_sensor(0x6F12, 0xC078);
		write_cmos_sensor(0x6F12, 0xC0B1);
		write_cmos_sensor(0x6F12, 0x687B);
		write_cmos_sensor(0x6F12, 0x6077);
		write_cmos_sensor(0x6F12, 0xA87B);
		write_cmos_sensor(0x6F12, 0xA077);
		write_cmos_sensor(0x6F12, 0x04F1);
		write_cmos_sensor(0x6F12, 0x1401);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x45F9);
		write_cmos_sensor(0x6F12, 0x617F);
		write_cmos_sensor(0x6F12, 0x514B);
		write_cmos_sensor(0x6F12, 0x90FB);
		write_cmos_sensor(0x6F12, 0xF1F1);
		write_cmos_sensor(0x6F12, 0x514A);
		write_cmos_sensor(0x6F12, 0xD962);
		write_cmos_sensor(0x6F12, 0x5089);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x4005);
		write_cmos_sensor(0x6F12, 0xC5EB);
		write_cmos_sensor(0x6F12, 0xC010);
		write_cmos_sensor(0x6F12, 0xB1EB);
		write_cmos_sensor(0x6F12, 0xC00F);
		write_cmos_sensor(0x6F12, 0x21D8);
		write_cmos_sensor(0x6F12, 0x9089);
		write_cmos_sensor(0x6F12, 0x20E0);
		write_cmos_sensor(0x6F12, 0xA87B);
		write_cmos_sensor(0x6F12, 0x697B);
		write_cmos_sensor(0x6F12, 0x6777);
		write_cmos_sensor(0x6F12, 0x4843);
		write_cmos_sensor(0x6F12, 0x4500);
		write_cmos_sensor(0x6F12, 0xA577);
		write_cmos_sensor(0x6F12, 0x0BE0);
		write_cmos_sensor(0x6F12, 0x801C);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xFE00);
		write_cmos_sensor(0x6F12, 0x6077);
		write_cmos_sensor(0x6F12, 0xB5FB);
		write_cmos_sensor(0x6F12, 0xF0F0);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x16F9);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x28F9);
		write_cmos_sensor(0x6F12, 0xA077);
		write_cmos_sensor(0x6F12, 0xA07F);
		write_cmos_sensor(0x6F12, 0x0C28);
		write_cmos_sensor(0x6F12, 0x03D8);
		write_cmos_sensor(0x6F12, 0x617F);
		write_cmos_sensor(0x6F12, 0x4143);
		write_cmos_sensor(0x6F12, 0xA942);
		write_cmos_sensor(0x6F12, 0xD0D0);
		write_cmos_sensor(0x6F12, 0x0128);
		write_cmos_sensor(0x6F12, 0xCED9);
		write_cmos_sensor(0x6F12, 0x607F);
		write_cmos_sensor(0x6F12, 0x0828);
		write_cmos_sensor(0x6F12, 0xE7D3);
		write_cmos_sensor(0x6F12, 0xCAE7);
		write_cmos_sensor(0x6F12, 0xD089);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0x4C00);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x00BF);
		write_cmos_sensor(0x6F12, 0xA17F);
		write_cmos_sensor(0x6F12, 0x8140);
		write_cmos_sensor(0x6F12, 0x3068);
		write_cmos_sensor(0x6F12, 0x8279);
		write_cmos_sensor(0x6F12, 0x4079);
		write_cmos_sensor(0x6F12, 0x4A43);
		write_cmos_sensor(0x6F12, 0x5100);
		write_cmos_sensor(0x6F12, 0xB1FB);
		write_cmos_sensor(0x6F12, 0xF0F0);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x0EF9);
		write_cmos_sensor(0x6F12, 0x0228);
		write_cmos_sensor(0x6F12, 0x00D8);
		write_cmos_sensor(0x6F12, 0x0220);
		write_cmos_sensor(0x6F12, 0x84F8);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x6BE5);
		write_cmos_sensor(0x6F12, 0x70B5);
		write_cmos_sensor(0x6F12, 0x0646);
		write_cmos_sensor(0x6F12, 0x2848);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x0169);
		write_cmos_sensor(0x6F12, 0x0C0C);
		write_cmos_sensor(0x6F12, 0x8DB2);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x85F8);
		write_cmos_sensor(0x6F12, 0x3046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xFFF8);
		write_cmos_sensor(0x6F12, 0x0646);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x7CF8);
		write_cmos_sensor(0x6F12, 0x4EB1);
		write_cmos_sensor(0x6F12, 0x2048);
		write_cmos_sensor(0x6F12, 0x0C21);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x3600);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xF7F8);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x00D0);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x70BD);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x70BD);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x7721);
		write_cmos_sensor(0x6F12, 0x1E48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xEFF8);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xAFF6);
		write_cmos_sensor(0x6F12, 0x2941);
		write_cmos_sensor(0x6F12, 0x1C48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE9F8);
		write_cmos_sensor(0x6F12, 0x124C);
		write_cmos_sensor(0x6F12, 0x6060);
		write_cmos_sensor(0x6F12, 0xAFF6);
		write_cmos_sensor(0x6F12, 0xD530);
		write_cmos_sensor(0x6F12, 0x1949);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x0864);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x0171);
		write_cmos_sensor(0x6F12, 0x1848);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x9B51);
		write_cmos_sensor(0x6F12, 0x1648);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD7F8);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xAFF6);
		write_cmos_sensor(0x6F12, 0x8341);
		write_cmos_sensor(0x6F12, 0x1448);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD1F8);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x5B51);
		write_cmos_sensor(0x6F12, 0x2060);
		write_cmos_sensor(0x6F12, 0x1148);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCAF8);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0xF931);
		write_cmos_sensor(0x6F12, 0xA060);
		write_cmos_sensor(0x6F12, 0x0F48);
		write_cmos_sensor(0x6F12, 0x1EE0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2DA0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3B00);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0C20);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0560);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0630);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x1340);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0FF0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x136B);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x1E13);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0570);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0C99);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x149B);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x7885);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x1C0D);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x504F);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0xA921);
		write_cmos_sensor(0x6F12, 0xE060);
		write_cmos_sensor(0x6F12, 0x0848);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9CF8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x1B21);
		write_cmos_sensor(0x6F12, 0x0648);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x96F8);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x0711);
		write_cmos_sensor(0x6F12, 0x0448);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x2061);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x72DD);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x8157);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x7937);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0xFB6C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F6);
		write_cmos_sensor(0x6F12, 0x136C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0xF16C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x855C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0xA10C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x015C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F6);
		write_cmos_sensor(0x6F12, 0xA91C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F6);
		write_cmos_sensor(0x6F12, 0x6B4C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F6);
		write_cmos_sensor(0x6F12, 0xA73C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x3B4C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x48F6);
		write_cmos_sensor(0x6F12, 0xBD0C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0xAD5C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x45F2);
		write_cmos_sensor(0x6F12, 0x532C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0xCB5C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x42F2);
		write_cmos_sensor(0x6F12, 0x575C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x976C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x42F2);
		write_cmos_sensor(0x6F12, 0x815C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x45F2);
		write_cmos_sensor(0x6F12, 0x4F0C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x050C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0x5B6C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F2);
		write_cmos_sensor(0x6F12, 0x932C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x46F6);
		write_cmos_sensor(0x6F12, 0xCD7C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F6);
		write_cmos_sensor(0x6F12, 0xDF2C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F6);
		write_cmos_sensor(0x6F12, 0x013C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F6);
		write_cmos_sensor(0x6F12, 0x0B3C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F6);
		write_cmos_sensor(0x6F12, 0x371C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x47F6);
		write_cmos_sensor(0x6F12, 0xE50C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x48F6);
		write_cmos_sensor(0x6F12, 0xE56C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x4088);
		write_cmos_sensor(0x6F12, 0x014F);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x11FF);
		write_cmos_sensor(0x602A, 0x3B00);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0080);
		write_cmos_sensor(0x602A, 0x3B1A);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);	
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3B20);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x00FE);
		write_cmos_sensor(0x602A, 0x3B26);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);	
		write_cmos_sensor(0x6F12, 0x00F7);
		write_cmos_sensor(0x602A, 0x3B2C);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);	
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x602A, 0x3B32);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3B36);
		write_cmos_sensor(0x6F12, 0x00B2);
		write_cmos_sensor(0x602A, 0x0E44);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3666);
		write_cmos_sensor(0x6F12, 0x0200);
		write_cmos_sensor(0x6F12, 0x0200);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x0201);
		write_cmos_sensor(0x6F12, 0x0800);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x0801);
		write_cmos_sensor(0x6F12, 0x0900);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x0901);
		write_cmos_sensor(0x6F12, 0x0A00);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x0F80);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x6F12, 0x1000);
		write_cmos_sensor(0x602A, 0x31FA);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x000F);
		write_cmos_sensor(0x6F12, 0x00A9);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x00A7);
		write_cmos_sensor(0x6F12, 0x00D3);
		write_cmos_sensor(0x602A, 0x339A);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0x005E);
		write_cmos_sensor(0x6F12, 0x0052);
		write_cmos_sensor(0x6F12, 0x0012);
		write_cmos_sensor(0x6F12, 0x0086);
		write_cmos_sensor(0x6F12, 0x0060);
		write_cmos_sensor(0x6F12, 0x0050);
		write_cmos_sensor(0x6F12, 0x0014);
		write_cmos_sensor(0x6F12, 0x0051);
		write_cmos_sensor(0x6F12, 0x0012);
		write_cmos_sensor(0x6F12, 0x0086);
		write_cmos_sensor(0x6F12, 0x0060);
		write_cmos_sensor(0x6F12, 0x0050);
		write_cmos_sensor(0x6F12, 0x0014);
		write_cmos_sensor(0x6F12, 0x0014);
		write_cmos_sensor(0x602A, 0x320E);
		write_cmos_sensor(0x6F12, 0x00A5);
		write_cmos_sensor(0x6F12, 0x01C8);
		write_cmos_sensor(0x6F12, 0x00A9);
		write_cmos_sensor(0x6F12, 0x01C6);
		write_cmos_sensor(0x6F12, 0x00A5);
		write_cmos_sensor(0x6F12, 0x00E5);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0049);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0044);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0046);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x00A5);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x00DC);
		write_cmos_sensor(0x6F12, 0x0003);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x6F12, 0x00D1);
		write_cmos_sensor(0x6F12, 0x00D2);
		write_cmos_sensor(0x6F12, 0x01C5);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x007C);
		write_cmos_sensor(0x6F12, 0x00A5);
		write_cmos_sensor(0x6F12, 0x0135);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00C1);
		write_cmos_sensor(0x6F12, 0x00B1);
		write_cmos_sensor(0x6F12, 0x00C9);
		write_cmos_sensor(0x6F12, 0x00B9);
		write_cmos_sensor(0x6F12, 0x00C9);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00AC);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x00B1);
		write_cmos_sensor(0x6F12, 0x00C9);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00AC);
		write_cmos_sensor(0x6F12, 0x007C);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0x0098);
		write_cmos_sensor(0x6F12, 0x00A5);
		write_cmos_sensor(0x6F12, 0x0135);
		write_cmos_sensor(0x6F12, 0x0174);
		write_cmos_sensor(0x6F12, 0x0184);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x008A);
		write_cmos_sensor(0x6F12, 0x008F);
		write_cmos_sensor(0x6F12, 0x00AD);
		write_cmos_sensor(0x6F12, 0x00B3);
		write_cmos_sensor(0x6F12, 0x00BE);
		write_cmos_sensor(0x6F12, 0x00C4);
		write_cmos_sensor(0x6F12, 0x0176);
		write_cmos_sensor(0x6F12, 0x017B);
		write_cmos_sensor(0x6F12, 0x01C8);
		write_cmos_sensor(0x6F12, 0x01CD);
		write_cmos_sensor(0x6F12, 0x008C);
		write_cmos_sensor(0x6F12, 0x0090);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x6F12, 0x00B4);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x6F12, 0x00C5);
		write_cmos_sensor(0x6F12, 0x0178);
		write_cmos_sensor(0x6F12, 0x017C);
		write_cmos_sensor(0x6F12, 0x01CA);
		write_cmos_sensor(0x6F12, 0x01CE);
		write_cmos_sensor(0x6F12, 0x008D);
		write_cmos_sensor(0x6F12, 0x0090);
		write_cmos_sensor(0x6F12, 0x00B0);
		write_cmos_sensor(0x6F12, 0x00B4);
		write_cmos_sensor(0x6F12, 0x00C1);
		write_cmos_sensor(0x6F12, 0x00C5);
		write_cmos_sensor(0x6F12, 0x0179);
		write_cmos_sensor(0x6F12, 0x017C);
		write_cmos_sensor(0x6F12, 0x01CB);
		write_cmos_sensor(0x6F12, 0x01CE);
		write_cmos_sensor(0x6F12, 0x008B);
		write_cmos_sensor(0x6F12, 0x008D);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00AC);
		write_cmos_sensor(0x6F12, 0x00BB);
		write_cmos_sensor(0x6F12, 0x00BD);
		write_cmos_sensor(0x6F12, 0x0177);
		write_cmos_sensor(0x6F12, 0x0179);
		write_cmos_sensor(0x6F12, 0x01C6);
		write_cmos_sensor(0x6F12, 0x01C8);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x008F);
		write_cmos_sensor(0x6F12, 0x0092);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x6F12, 0x00B4);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x6F12, 0x00C5);
		write_cmos_sensor(0x6F12, 0x017B);
		write_cmos_sensor(0x6F12, 0x017E);
		write_cmos_sensor(0x6F12, 0x01CA);
		write_cmos_sensor(0x6F12, 0x01CE);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x008B);
		write_cmos_sensor(0x6F12, 0x008D);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00AC);
		write_cmos_sensor(0x6F12, 0x00BB);
		write_cmos_sensor(0x6F12, 0x00BD);
		write_cmos_sensor(0x6F12, 0x0177);
		write_cmos_sensor(0x6F12, 0x0179);
		write_cmos_sensor(0x6F12, 0x01C6);
		write_cmos_sensor(0x6F12, 0x01C8);
		write_cmos_sensor(0x6F12, 0x008F);
		write_cmos_sensor(0x602A, 0x32FA);
		write_cmos_sensor(0x6F12, 0x00AD);
		write_cmos_sensor(0x6F12, 0x017B);
		write_cmos_sensor(0x6F12, 0x01C8);
		write_cmos_sensor(0x6F12, 0x007B);
		write_cmos_sensor(0x6F12, 0x00A7);
		write_cmos_sensor(0x6F12, 0x0134);
		write_cmos_sensor(0x6F12, 0x01C5);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x0009);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00C4);
		write_cmos_sensor(0x6F12, 0x0009);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x00AC);
		write_cmos_sensor(0x6F12, 0x0037);
		write_cmos_sensor(0x6F12, 0x0005);
		write_cmos_sensor(0x6F12, 0x0058);
		write_cmos_sensor(0x6F12, 0x01CA);
		write_cmos_sensor(0x6F12, 0x0059);
		write_cmos_sensor(0x6F12, 0x005B);
		write_cmos_sensor(0x6F12, 0x00A7);
		write_cmos_sensor(0x6F12, 0x00AA);
		write_cmos_sensor(0x6F12, 0x01C6);
		write_cmos_sensor(0x6F12, 0x01C9);
		write_cmos_sensor(0x6F12, 0x00A7);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x01CE);
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
		write_cmos_sensor(0x6F12, 0x000A);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x0038);
		write_cmos_sensor(0x6F12, 0x0040);
		write_cmos_sensor(0x6F12, 0x0050);
		write_cmos_sensor(0x6F12, 0x0058);
		write_cmos_sensor(0x6F12, 0x0068);
		write_cmos_sensor(0x6F12, 0x0070);
		write_cmos_sensor(0x6F12, 0x0080);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0x0017);
		write_cmos_sensor(0x6F12, 0x002F);
		write_cmos_sensor(0x6F12, 0x0047);
		write_cmos_sensor(0x6F12, 0x005F);
		write_cmos_sensor(0x6F12, 0x0077);
		write_cmos_sensor(0x6F12, 0x008F);
		write_cmos_sensor(0x602A, 0x34D4);
		write_cmos_sensor(0x6F12, 0x0505);
		write_cmos_sensor(0x602A, 0x34CE);
		write_cmos_sensor(0x6F12, 0x0D0D);
		write_cmos_sensor(0x602A, 0x34FA);
		write_cmos_sensor(0x6F12, 0x3ADC);
		write_cmos_sensor(0x602A, 0x34D6);
		write_cmos_sensor(0x6F12, 0x0808);
		write_cmos_sensor(0x602A, 0x34D8);
		write_cmos_sensor(0x6F12, 0x0404);
		write_cmos_sensor(0x602A, 0x3508);
		write_cmos_sensor(0x6F12, 0x000D);
		write_cmos_sensor(0x6F12, 0x000D);
		write_cmos_sensor(0x6F12, 0x000D);
		write_cmos_sensor(0x602A, 0x3500);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);
		write_cmos_sensor(0x602A, 0x34C0);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor(0x6F12, 0x0104);
		write_cmos_sensor(0x602A, 0x0110);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x3082);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);
		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x317A);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x602A, 0x317C);
		write_cmos_sensor(0x6F12, 0x0096);
		write_cmos_sensor(0x602A, 0x36B2);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x009B);
		write_cmos_sensor(0x602A, 0xF490);
		write_cmos_sensor(0x6F12, 0x0030);
		write_cmos_sensor(0x602A, 0xF47A);
		write_cmos_sensor(0x6F12, 0x0012);
		write_cmos_sensor(0x602A, 0xF428);
		write_cmos_sensor(0x6F12, 0x0200);
		write_cmos_sensor(0x602A, 0xF48E);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x602A, 0xF45C);
		write_cmos_sensor(0x6F12, 0x0004);
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);
		write_cmos_sensor(0x6F12, 0x0900);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x021C, 0x0618);	
	}
	else if(chip_id == 0xc001)
	{
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x1FD0);
		write_cmos_sensor(0x6F12, 0x0448);
		write_cmos_sensor(0x6F12, 0x0349);
		write_cmos_sensor(0x6F12, 0x0160);
		write_cmos_sensor(0x6F12, 0xC26A);
		write_cmos_sensor(0x6F12, 0x511A);
		write_cmos_sensor(0x6F12, 0x8180);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x60B8);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x20E8);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x13A0);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x38B5);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x0446);
		write_cmos_sensor(0x6F12, 0x8DF8);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x00F5);
		write_cmos_sensor(0x6F12, 0xB470);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x59F8);
		write_cmos_sensor(0x6F12, 0x9DF8);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xFF28);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x08B1);
		write_cmos_sensor(0x6F12, 0x04F2);
		write_cmos_sensor(0x6F12, 0x6914);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x38BD);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0xF8E7);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0x92B0);
		write_cmos_sensor(0x6F12, 0x0C46);
		write_cmos_sensor(0x6F12, 0x4822);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x46F8);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0x04EB);
		write_cmos_sensor(0x6F12, 0x4003);
		write_cmos_sensor(0x6F12, 0x0A5C);
		write_cmos_sensor(0x6F12, 0x02F0);
		write_cmos_sensor(0x6F12, 0x0F02);
		write_cmos_sensor(0x6F12, 0x04F8);
		write_cmos_sensor(0x6F12, 0x1020);
		write_cmos_sensor(0x6F12, 0x0A5C);
		write_cmos_sensor(0x6F12, 0x401C);
		write_cmos_sensor(0x6F12, 0x1209);
		write_cmos_sensor(0x6F12, 0x5A70);
		write_cmos_sensor(0x6F12, 0x4828);
		write_cmos_sensor(0x6F12, 0xF2D3);
		write_cmos_sensor(0x6F12, 0x12B0);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x164E);
		write_cmos_sensor(0x6F12, 0x0F46);
		write_cmos_sensor(0x6F12, 0x06F1);
		write_cmos_sensor(0x6F12, 0x1105);
		write_cmos_sensor(0x6F12, 0xA236);
		write_cmos_sensor(0x6F12, 0xB0B1);
		write_cmos_sensor(0x6F12, 0x1449);
		write_cmos_sensor(0x6F12, 0x1248);
		write_cmos_sensor(0x6F12, 0x0968);
		write_cmos_sensor(0x6F12, 0x0078);
		write_cmos_sensor(0x6F12, 0xB1F8);
		write_cmos_sensor(0x6F12, 0x6A10);
		write_cmos_sensor(0x6F12, 0xC007);
		write_cmos_sensor(0x6F12, 0x0ED0);
		write_cmos_sensor(0x6F12, 0x0846);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xBEFF);
		write_cmos_sensor(0x6F12, 0x84B2);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xFFF7);
		write_cmos_sensor(0x6F12, 0xD0FF);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x9072);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x04F1);
		write_cmos_sensor(0x6F12, 0x4800);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x16F8);
		write_cmos_sensor(0x6F12, 0x002F);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x13B8);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF081);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x5501);
		write_cmos_sensor(0x6F12, 0x0348);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x10B8);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0C40);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0560);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x152D);
		write_cmos_sensor(0x6F12, 0x48F6);
		write_cmos_sensor(0x6F12, 0x296C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x41F2);
		write_cmos_sensor(0x6F12, 0x950C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x49F2);
		write_cmos_sensor(0x6F12, 0x514C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x4088);
		write_cmos_sensor(0x6F12, 0x0166);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x5360, 0x0004);
		write_cmos_sensor(0x3078, 0x0059);
		write_cmos_sensor(0x307C, 0x0025);
		write_cmos_sensor(0x36D0, 0x00DD);
		write_cmos_sensor(0x36D2, 0x0100);
		write_cmos_sensor(0x306A, 0x00EF);
	//	write_cmos_sensor(0x6028, 0x4000);
	//	write_cmos_sensor(0x0B00, 0x0180); //enable LSC 
		//write_cmos_sensor(0x0100, 0x0000);	 // stream off
		LOG_INF("-- sensor_init shen enter\n");
		// evb2 don't need init setting
              //  write_cmos_sensor(0x0b00, 0x0180);	 // stream lsc on
	}
	else
	{
		
	}
}	/*	sensor_init  */
/**********************************************************************************************************************/
//$MIPI[Width:2096,Height:1552,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1260,useEmbData:0]
//$MV1[MCLK:24,Width:2096,Height:1552,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:1260,pvi_pclk_inverse:0]
//=====================================================
// 4H8XXM
// 2X2 Binning Normal Mode
// X_output size : 2096
// Y_output size : 1552
// Frame_rate : 30.06 fps
// Output_format RAW 10
// Output_lanes 4
// Output_clock_mhz : 1272 Mhz
// System_clock_mhz : 440 Mhz
// Input_clock_mhz : 24 Mhz
// TnP R651
//=====================================================
/**********************************************************************************************************************/
static void preview_setting(void)
{
	kal_uint16 chip_id = 0;
	int retry = 0;
	chip_id = read_cmos_sensor(0x0002);	
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// Size setting
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor(0x6F12, 0x0000);

		write_cmos_sensor(0x602A, 0x0344);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x0348);
		write_cmos_sensor(0x6F12, 0x0CC7);
		write_cmos_sensor(0x602A, 0x0346);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x034A);
		write_cmos_sensor(0x6F12, 0x0997);
		write_cmos_sensor(0x6F12, 0x0660);	//1632
		write_cmos_sensor(0x6F12, 0x04C8);	//1224
		write_cmos_sensor(0x602A, 0x0342);
		write_cmos_sensor(0x6F12, 0x0EA0);
		write_cmos_sensor(0x602A, 0x0340);
		write_cmos_sensor(0x6F12, 0x09BC);

		// Analog binning
		write_cmos_sensor(0x602A, 0x0900);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0901);
		write_cmos_sensor_8(0x6F12, 0x12);
		write_cmos_sensor(0x602A, 0x0380);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0003);

		// Digital binning
		write_cmos_sensor(0x602A, 0x0400);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0404);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E42);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor_8(0x6F12, 0x00);

		//Integration time & Gain setting
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);	//2014.12.15
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);




		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);



		write_cmos_sensor(0x602A, 0x34C1);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0111);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0114);
		write_cmos_sensor_8(0x6F12, 0x03);
		write_cmos_sensor(0x602A, 0x3083);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x602A, 0x0B05);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);


		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor_8(0x6F12, 0x01);

		write_cmos_sensor(0x602A, 0x0302);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x0306);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602a, 0x030E);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602A, 0x3008);
		write_cmos_sensor(0x6F12, 0x0000);

		// Stream On
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor_8(0x6F12, 0x01);
		if(imgsensor.ihdr_en)
		{
			write_cmos_sensor(0x0216, 0x0201);	 // LNHDR mode 2  
		}
		else
		{
			write_cmos_sensor(0x0216, 0x0000);
		}

		
	}	
	else if(chip_id == 0xc001)
	{
		write_cmos_sensor(0x0100, 0x0000);	 // stream off
		//mdelay(50);
	LOG_INF("-- sensor_init shen preview\n");
		while(retry<20)
		{
			if(read_cmos_sensor_8(0x0005)!=0xff)
			{
				msleep(5);
				retry++;
				LOGE("Sensor has output %x\n",read_cmos_sensor_8(0x0005));			  
			}
		 	else
			{
				retry=0;
				LOG_INF("Sensor has not output stream %x\n",read_cmos_sensor(0x0100));
				break;
			}
		}
		
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7971);
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0EC6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0xF490, 0x0030);
		write_cmos_sensor(0xF47A, 0x0012);
		write_cmos_sensor(0xF428, 0x0200);
		write_cmos_sensor(0xF48E, 0x0010);
		write_cmos_sensor(0xF45C, 0x0004);
		write_cmos_sensor(0x0B04, 0x0101);
		//write_cmos_sensor(0x0B00, 0x0180);//enable LSC
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C40);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0x0200, 0x0618);
		write_cmos_sensor(0x0202, 0x0904);
		write_cmos_sensor(0x31AA, 0x0004);
		write_cmos_sensor(0x1006, 0x0006);
		write_cmos_sensor(0x31FA, 0x0000);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0348, 0x0CC7);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x034A, 0x0997);
		write_cmos_sensor(0x034C, 0x0660);
		write_cmos_sensor(0x034E, 0x04C8);
		write_cmos_sensor(0x0342, 0x0EA0);
		write_cmos_sensor(0x0340, 0x04DE);
		write_cmos_sensor(0x0900, 0x0212);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0003);
		write_cmos_sensor(0x0400, 0x0002);
		write_cmos_sensor(0x0404, 0x0020);
		write_cmos_sensor(0x0114, 0x0330);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0005);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x00AF);
		write_cmos_sensor(0x030C, 0x0006);
		write_cmos_sensor(0x030E, 0x00AF);
		write_cmos_sensor(0x3008, 0x0000);
		write_cmos_sensor(0x0100, 0x0100);
	}

}



	/*	preview_setting  */
/**********************************************************************************************************************/
//$MIPI[Width:4192,Height:3104,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1260,useEmbData:0]
//$MV1[MCLK:24,Width:4192,Height:3104,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:1260,pvi_pclk_inverse:0]

//=====================================================
// 4H8XXM
// Full Resolution normal Mode
// X_output size : 4192
// Y_output size : 3104
// Frame_rate : 30 fps
// Output_format RAW 10
// Output_lanes 4
// Output_clock_mhz : 1272 Mhz
// System_clock_mhz : 440 Mhz , VT_PIX_clk : 88Mhz
// Input_clock_mhz : 24 Mhz
// TnP R651
//=====================================================
//200
/**********************************************************************************************************************/
static void normal_capture_setting(void)
{
	kal_uint16 chip_id = 0;
	int retry = 0;
	chip_id = read_cmos_sensor(0x0002); 
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// Size setting
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor(0x6F12, 0x0000);

		write_cmos_sensor(0x602A, 0x0344);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x0348);
		write_cmos_sensor(0x6F12, 0x0CC7);
		write_cmos_sensor(0x602A, 0x0346);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x034A);
		write_cmos_sensor(0x6F12, 0x0997);
		write_cmos_sensor(0x6F12, 0x0CC0);	 //3264
		write_cmos_sensor(0x6F12, 0x0990);	 //2448
		write_cmos_sensor(0x602A, 0x0342);
		write_cmos_sensor(0x6F12, 0x0EA0);
		write_cmos_sensor(0x602A, 0x0340);
		write_cmos_sensor(0x6F12, 0x09BC);


		// Analog binning
		write_cmos_sensor(0x602A, 0x0900);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x0901);
		write_cmos_sensor_8(0x6F12, 0x11);
		write_cmos_sensor(0x602A, 0x0380);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);

		// Digital binning
		write_cmos_sensor(0x602A, 0x0400);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0404);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E42);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);

		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor_8(0x6F12, 0x00);

		//Integration time & Gain setting
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);




		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);



		write_cmos_sensor(0x602A, 0x34C1);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0111);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0114);
		write_cmos_sensor_8(0x6F12, 0x03);
		write_cmos_sensor(0x602A, 0x3083);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x602A, 0x0B05);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);


		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor_8(0x6F12, 0x01);

		write_cmos_sensor(0x602A, 0x0302);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x0306);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602a, 0x030E);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602A, 0x3008);
		write_cmos_sensor(0x6F12, 0x0000);


		// Stream On
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor_8(0x6F12, 0x01);

		if(imgsensor.ihdr_en)
		{
			write_cmos_sensor(0x0216, 0x0201);   // LNHDR mode 2  
		}
		else
		{
			write_cmos_sensor(0x0216, 0x0000);
		}		

	}
	else if(chip_id == 0xc001)
	{
	LOG_INF("-- capture...  shen enter\n");
		write_cmos_sensor(0x0100, 0x0000);	 // stream off
		//mdelay(50);
		while(retry<20)
		{
			if(read_cmos_sensor_8(0x0005)!=0xff)
			{
				msleep(5);
				retry++;
				LOGE("Sensor has output %x\n",read_cmos_sensor_8(0x0005));			  
			}
		 	else
			{
				retry=0;
				LOG_INF("Sensor has not output stream %x\n",read_cmos_sensor(0x0100));
				break;
			}
		}		
		
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7971);
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0EC6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0xF490, 0x0030);
		write_cmos_sensor(0xF47A, 0x0012);
		write_cmos_sensor(0xF428, 0x0200);
		write_cmos_sensor(0xF48E, 0x0010);
		write_cmos_sensor(0xF45C, 0x0004);
		write_cmos_sensor(0x0B04, 0x0101);
		//write_cmos_sensor(0x0B00, 0x0180); //enable LSC 
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C40);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0x0200, 0x0618);
		write_cmos_sensor(0x0202, 0x0904);
		write_cmos_sensor(0x31AA, 0x0004);
		write_cmos_sensor(0x1006, 0x0006);
		write_cmos_sensor(0x31FA, 0x0000);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0348, 0x0CC7);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x034A, 0x0997);
		write_cmos_sensor(0x034C, 0x0CC0);
		write_cmos_sensor(0x034E, 0x0990);
		write_cmos_sensor(0x0342, 0x0EA0);
		write_cmos_sensor(0x0340, 0x09C2);
		write_cmos_sensor(0x0900, 0x0111);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0001);
		write_cmos_sensor(0x0400, 0x0002);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0114, 0x0330);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0005);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x00AF);
		write_cmos_sensor(0x030C, 0x0006);
		write_cmos_sensor(0x030E, 0x00AF);
		write_cmos_sensor(0x3008, 0x0000);
		write_cmos_sensor(0x0100, 0x0100);
		
	}

}




/**********************************************************************************************************************/
//$MIPI[Width:4192,Height:3104,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1260,useEmbData:0]
//$MV1[MCLK:24,Width:4192,Height:3104,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:1260,pvi_pclk_inverse:0]
//=====================================================
// 4H8XXM
// Full Resolution normal Mode
// X_output size : 4192
// Y_output size : 3104
// Frame_rate : 24.07 fps
// Output_format RAW 10
// Output_lanes 4
// Output_clock_mhz : 1272 Mhz
// System_clock_mhz : 352 Mhz , VT_PIX_clk : 88Mhz
// Input_clock_mhz : 24 Mhz
// TnP R651
//=====================================================
/**********************************************************************************************************************/
static void pip_capture_setting(void)
{
	kal_uint16 chip_id = 0;
	chip_id = read_cmos_sensor(0x0002); 
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// Size setting
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor(0x6F12, 0x0000);

		write_cmos_sensor(0x602A, 0x0344);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x0348);
		write_cmos_sensor(0x6F12, 0x0CC7);
		write_cmos_sensor(0x602A, 0x0346);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x034A);
		write_cmos_sensor(0x6F12, 0x0997);
		write_cmos_sensor(0x6F12, 0x0CC0);	 //3264
		write_cmos_sensor(0x6F12, 0x0990);	 //2448
		write_cmos_sensor(0x602A, 0x0342);
		write_cmos_sensor(0x6F12, 0x0EA0);
		write_cmos_sensor(0x602A, 0x0340);
		write_cmos_sensor(0x6F12, 0x09BC);

		// Analog binning
		write_cmos_sensor(0x602A, 0x0900);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x0901);
		write_cmos_sensor_8(0x6F12, 0x11);
		write_cmos_sensor(0x602A, 0x0380);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);

		// Digital binning
		write_cmos_sensor(0x602A, 0x0400);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0404);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E42);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);

		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor_8(0x6F12, 0x00);

		//Integration time & Gain setting
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);


		
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);



		write_cmos_sensor(0x602A, 0x34C1);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0111);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0114);
		write_cmos_sensor_8(0x6F12, 0x03);
		write_cmos_sensor(0x602A, 0x3083);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x602A, 0x0B05);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);


		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor_8(0x6F12, 0x01);

		write_cmos_sensor(0x602A, 0x0302);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0306);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602a, 0x030E);
		write_cmos_sensor(0x6F12, 0x00AF);	
		write_cmos_sensor(0x602A, 0x3008);
		write_cmos_sensor(0x6F12, 0x0001);

		// Stream On
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor_8(0x6F12, 0x01);
	}
	else if(chip_id == 0xc001)
	{
		write_cmos_sensor(0x0100, 0x0000);	 // stream off
	
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7971);
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0EC6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0xF490, 0x0030);
		write_cmos_sensor(0xF47A, 0x0012);
		write_cmos_sensor(0xF428, 0x0200);
		write_cmos_sensor(0xF48E, 0x0010);
		write_cmos_sensor(0xF45C, 0x0004);
		write_cmos_sensor(0x0B04, 0x0101);
      //write_cmos_sensor(0x0B00, 0x0080); //lsc on
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C40);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0x0200, 0x0618);
		write_cmos_sensor(0x0202, 0x0465);
		write_cmos_sensor(0x31AA, 0x0004);
		write_cmos_sensor(0x1006, 0x0006);
		write_cmos_sensor(0x31FA, 0x0000);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0348, 0x0CC7);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x034A, 0x0997);
		write_cmos_sensor(0x034C, 0x0CC0);
		write_cmos_sensor(0x034E, 0x0990);
		write_cmos_sensor(0x0342, 0x0EA0);
		write_cmos_sensor(0x0340, 0x09BC);
		write_cmos_sensor(0x0900, 0x0111);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0001);
		write_cmos_sensor(0x0400, 0x0002);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0114, 0x0330);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0005);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x008C);
		write_cmos_sensor(0x030C, 0x0006);
		write_cmos_sensor(0x030E, 0x008C);
		write_cmos_sensor(0x3008, 0x0000);
		write_cmos_sensor(0x317A, 0x0100);
		write_cmos_sensor(0x0100, 0x0100);
	}

}


static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! huangsh 4currefps:%d\n",currefps);
	if(currefps==300)
		normal_capture_setting();
	else if(currefps==240) // PIP
		pip_capture_setting();
	else
		pip_capture_setting();
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);	
	preview_setting();	
}
/**********************************************************************************************************************/
//$MIPI[Width:688,Height:512,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1260,useEmbData:0]
//$MV1[MCLK:24,Width:688,Height:512,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:1260,pvi_pclk_inverse:0]

//=====================================================
// 4H8XXM
// 6X6 Binning Normal Mode
// X_output size : 688
// Y_output size : 512
// Frame_rate : 120.07 fps
// Output_format RAW 10
// Output_lanes 4
// Output_clock_mhz : 1272 Mhz
// System_clock_mhz : 440 Mhz
// Input_clock_mhz : 24 Mhz
// TnP R651
//=====================================================
/**********************************************************************************************************************/
static void hs_video_setting(void)
{
	kal_uint16 chip_id = 0;
	chip_id = read_cmos_sensor(0x0002); 
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// Size setting
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor(0x6F12, 0x0000);

		write_cmos_sensor(0x602A, 0x0344);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x602A, 0x0348);
		write_cmos_sensor(0x6F12, 0x0CC7);
		write_cmos_sensor(0x602A, 0x0346);
		write_cmos_sensor(0x6F12, 0x0009);
		write_cmos_sensor(0x602A, 0x034A);
		write_cmos_sensor(0x6F12, 0x0996);
		write_cmos_sensor(0x6F12, 0x0330);	//816
		write_cmos_sensor(0x6F12, 0x0264);	//612
		write_cmos_sensor(0x602A, 0x0342);
		write_cmos_sensor(0x6F12, 0x0EA0);
		write_cmos_sensor(0x602A, 0x0340);
		write_cmos_sensor(0x6F12, 0x0280);

		// Analog binning
		write_cmos_sensor(0x602A, 0x0900);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0901);
		write_cmos_sensor_8(0x6F12, 0x14);
		write_cmos_sensor(0x602A, 0x0380);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0007);

		// Digital binning
		write_cmos_sensor(0x602A, 0x0400);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0404);
		write_cmos_sensor(0x6F12, 0x0040);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E42);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor_8(0x6F12, 0x00);

		//Integration time & Gain setting
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);




		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);



		write_cmos_sensor(0x602A, 0x34C1);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0111);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0114);
		write_cmos_sensor_8(0x6F12, 0x03);
		write_cmos_sensor(0x602A, 0x3083);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x602A, 0x0B05);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);


		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0302);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x0306);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602a, 0x030E);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602A, 0x3008);
		write_cmos_sensor(0x6F12, 0x0000);

		// Stream On
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor_8(0x6F12, 0x01);
	}
	else if(chip_id == 0xc001)
	{

		write_cmos_sensor(0x0100, 0x0000);	// stream off
		
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7971);
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0EC6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0xF490, 0x0030);
		write_cmos_sensor(0xF47A, 0x0012);
		write_cmos_sensor(0xF428, 0x0200);
		write_cmos_sensor(0xF48E, 0x0010);
		write_cmos_sensor(0xF45C, 0x0004);
		write_cmos_sensor(0x0B04, 0x0101);
         //       write_cmos_sensor(0x0B00, 0x0180); //lsc on
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C40);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0x0200, 0x0618);
		write_cmos_sensor(0x0202, 0x0465);
		write_cmos_sensor(0x31AA, 0x0004);
		write_cmos_sensor(0x1006, 0x0006);
		write_cmos_sensor(0x31FA, 0x0000);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0348, 0x0CC7);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x034A, 0x0997);
		write_cmos_sensor(0x034C, 0x0330);
		write_cmos_sensor(0x034E, 0x0264);
		write_cmos_sensor(0x0342, 0x0EA0);
		write_cmos_sensor(0x0340, 0x0280);
		write_cmos_sensor(0x0900, 0x0214);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0007);
		write_cmos_sensor(0x0400, 0x0002);
		write_cmos_sensor(0x0404, 0x0040);
		write_cmos_sensor(0x0114, 0x0330);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0005);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x00AF);
		write_cmos_sensor(0x030C, 0x0006);
		write_cmos_sensor(0x030E, 0x00AF);
		write_cmos_sensor(0x3008, 0x0000);
		write_cmos_sensor(0x317A, 0x0100);
		write_cmos_sensor(0x0100, 0x0100);
	}
}



/**********************************************************************************************************************/
//$MIPI[Width:1280,Height:720,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1260,useEmbData:0]
//$MV1[MCLK:24,Width:1280,Height:720,Format:MIPI_RAW10,mipi_lane:4,mipi_datarate:1260,pvi_pclk_inverse:0]
//=====================================================
// 4H8XX EVT 2.0
// 3X3 Binning Normal Mode
// X_output size : 1280
// Y_output size : 720
// Frame_rate : 30.06 fps
// Output_format RAW 10
// Output_lanes 4
// Output_clock_mhz : 1260 Mhz
// System_clock_mhz : 440 Mhz
// Input_clock_mhz : 24 Mhz
// TnP R532
//=====================================================
/**********************************************************************************************************************/
static void slim_video_setting(void)
{
	kal_uint16 chip_id = 0;
	chip_id = read_cmos_sensor(0x0002); 
	LOG_INF("-- sensor_init, chip id = 0x%x\n", chip_id);
	if (chip_id == 0xb001) 
	{

		// Size setting
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor(0x6F12, 0x0000);

		write_cmos_sensor(0x602A, 0x0344);
		write_cmos_sensor(0x6F12, 0x0168);
		write_cmos_sensor(0x602A, 0x0348);
		write_cmos_sensor(0x6F12, 0x0B67);
		write_cmos_sensor(0x602A, 0x0346);
		write_cmos_sensor(0x6F12, 0x0200);
		write_cmos_sensor(0x602A, 0x034A);
		write_cmos_sensor(0x6F12, 0x079F);
		write_cmos_sensor(0x6F12, 0x0500);	//1280
		write_cmos_sensor(0x6F12, 0x02D0);	//720
		write_cmos_sensor(0x602A, 0x0342);
		write_cmos_sensor(0x6F12, 0x0EA0);
		write_cmos_sensor(0x602A, 0x0340);
		write_cmos_sensor(0x6F12, 0x09BC);

		// Analog binning
		write_cmos_sensor(0x602A, 0x0900);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0901);
		write_cmos_sensor_8(0x6F12, 0x12);
		write_cmos_sensor(0x602A, 0x0380);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0003);

		// Digital binning
		write_cmos_sensor(0x602A, 0x0400);
		write_cmos_sensor(0x6F12, 0x0002);
		write_cmos_sensor(0x602A, 0x0404);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E42);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x36C6);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x36C4);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x602A, 0x36EE);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0100);

		write_cmos_sensor(0x602A, 0x36B0);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor_8(0x6F12, 0x00);

		//Integration time & Gain setting);
		write_cmos_sensor(0x602A, 0x0200);
		write_cmos_sensor(0x6F12, 0x0618);	//2014.12.15
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x602A, 0x020E);
		write_cmos_sensor(0x6F12, 0x0100);




		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C20);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x306A);
		write_cmos_sensor(0x6F12, 0x015B);



		write_cmos_sensor(0x602A, 0x34C1);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x34C2);
		write_cmos_sensor_8(0x6F12, 0x01);


		write_cmos_sensor(0x602A, 0x0111);
		write_cmos_sensor_8(0x6F12, 0x02);
		write_cmos_sensor(0x602A, 0x0114);
		write_cmos_sensor_8(0x6F12, 0x03);
		write_cmos_sensor(0x602A, 0x3083);
		write_cmos_sensor_8(0x6F12, 0x00);


		write_cmos_sensor(0x602A, 0x0B05);
		write_cmos_sensor_8(0x6F12, 0x01);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0E5C);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x3078);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x602A, 0x307C);
		write_cmos_sensor(0x6F12, 0x0030);


		write_cmos_sensor(0x602A, 0x34F4);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x3766);
		write_cmos_sensor(0x6F12, 0x0610);
		write_cmos_sensor(0x602A, 0x375A);
		write_cmos_sensor(0x6F12, 0x0045);
		write_cmos_sensor(0x602A, 0x31AE);
		write_cmos_sensor_8(0x6F12, 0x01);

		write_cmos_sensor(0x602A, 0x0302);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x0306);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602a, 0x030E);
		write_cmos_sensor(0x6F12, 0x00AF);
		write_cmos_sensor(0x602A, 0x3008);
		write_cmos_sensor(0x6F12, 0x0000);


		// Stream On
		write_cmos_sensor(0x602A, 0x0100);
		write_cmos_sensor_8(0x6F12, 0x01);
	}
	else if(chip_id == 0xc001)
	{
	
		write_cmos_sensor(0x0100, 0x0000);	// stream off

		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x602A, 0x6214);
		write_cmos_sensor(0x6F12, 0x7971);
		write_cmos_sensor(0x602A, 0x6218);
		write_cmos_sensor(0x6F12, 0x7150);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0EC6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0xF490, 0x0030);
		write_cmos_sensor(0xF47A, 0x0012);
		write_cmos_sensor(0xF428, 0x0200);
		write_cmos_sensor(0xF48E, 0x0010);
		write_cmos_sensor(0xF45C, 0x0004);
		write_cmos_sensor(0x0B04, 0x0101);
         //       write_cmos_sensor(0x0B00, 0x0180); //lsc on
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x0C40);
		write_cmos_sensor(0x6F12, 0x0140);
		write_cmos_sensor(0xFCFC, 0x4000);
		write_cmos_sensor(0x0200, 0x0618);
		write_cmos_sensor(0x0202, 0x0465);
		write_cmos_sensor(0x31AA, 0x0004);
		write_cmos_sensor(0x1006, 0x0006);
		write_cmos_sensor(0x31FA, 0x0000);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0344, 0x0168);
		write_cmos_sensor(0x0348, 0x0B67);
		write_cmos_sensor(0x0346, 0x0200);
		write_cmos_sensor(0x034A, 0x079F);
		write_cmos_sensor(0x034C, 0x0500);
		write_cmos_sensor(0x034E, 0x02D0);
		write_cmos_sensor(0x0342, 0x0EA0);
		write_cmos_sensor(0x0340, 0x09BC);
		write_cmos_sensor(0x0900, 0x0212);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0003);
		write_cmos_sensor(0x0400, 0x0002);
		write_cmos_sensor(0x0404, 0x0020);
		write_cmos_sensor(0x0114, 0x0330);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0300, 0x0005);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x00AF);
		write_cmos_sensor(0x030C, 0x0006);
		write_cmos_sensor(0x030E, 0x00AF);
		write_cmos_sensor(0x3008, 0x0000);
		write_cmos_sensor(0x317A, 0x0100);
		write_cmos_sensor(0x0100, 0x0100);
	}
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
    kal_uint8 retry = 1;
    /*sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address*/
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
			write_cmos_sensor(0x602C,0x4000);
			write_cmos_sensor(0x602E,0x0000);
			*sensor_id = read_cmos_sensor(0x6F12);
			//*sensor_id = imgsensor_info.sensor_id;
            if (*sensor_id == imgsensor_info.sensor_id) {               
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);      
                return ERROR_NONE;
            }   
            LOG_INF("Read sensor id fail, write id: 0x%x, sensor id = 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        retry = 1;
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
        static kal_uint16 otp_have_read = 0;
       // int a1 = 0,a2 = 0,a3 = 0,a4 = 0;
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0; 
	LOG_1;
	LOG_2;
	//sensor have two i2c address 0x5a 0x5b & 0x21 0x20, we should detect the module used i2c address
	    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
			write_cmos_sensor(0x602C,0x4000);
			write_cmos_sensor(0x602E,0x0000);
            sensor_id =  read_cmos_sensor(0x6F12);
			//sensor_id = imgsensor_info.sensor_id;
            if (sensor_id == imgsensor_info.sensor_id) {                
                LOG_INF("i2c huangsh 123  write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);   
                break;
            }   
            LOG_INF("Read sensor id fail write_id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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

    #ifdef  S5K4H8_OTP_FUNCTION
	if(otp_have_read <= 1)
	{
		otp_have_read ++;
		read_data_from_otp();
		writer_awb_otp(s_Awb_otp.r_gain,s_Awb_otp.b_gain,s_Awb_otp.g_gain);
		LOG_INF("s5k4h8 (%s):  first time read otp\n",__func__);
	}
	else
	{
		if(s_otp_Avail.awb_avail)
		{
			writer_awb_otp(s_Awb_otp.r_gain,s_Awb_otp.b_gain,s_Awb_otp.g_gain);
			LOG_INF(" s5k4h8 (%s):otp_rbg_gain =[%d,%d,%d] \n",__func__,s_Awb_otp.r_gain,s_Awb_otp.b_gain,s_Awb_otp.g_gain);
		}
		else 
		{
			LOG_INF("s5k4h8 otp err !!!\n");
		}
	}
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
	imgsensor.ihdr_en = KAL_FALSE;
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
	set_mirror_flip(IMAGE_NORMAL);
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
	LOG_INF("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

    if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) // 30fps
    {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
    {
		if (imgsensor.current_fps != imgsensor_info.cap1.max_framerate)
	LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} 

	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("Caputre fps:%d\n",imgsensor.current_fps);
	capture_setting(imgsensor.current_fps); 
    set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");
	
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
	LOG_INF("E");
	
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
	set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/*	hs_video   */


static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");
	
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
	LOG_INF("E");
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
	LOG_INF("scenario_id = %d", scenario_id);

	
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
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) 	  
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
        case MSDK_SCENARIO_ID_CUSTOM1:
            frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();            
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
           // set_dummy();            
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
            //set_dummy();            
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
            //set_dummy();            
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
        write_cmos_sensor(0x0600, 0x0002);
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
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
	
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;	
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
    //unsigned long long *feature_return_para=(unsigned long long *) feature_para;	
 
	LOG_INF("feature_id = %d", feature_id);
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
			write_shutter(*feature_data);
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
			if((sensor_reg_data->RegData>>8)>0)
			   write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			else
				write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
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
			get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
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
		case SENSOR_FEATURE_SET_HDR:
            LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.ihdr_en = (BOOL)*feature_data;
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
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			break;
		case SENSOR_FEATURE_SET_HDR_SHUTTER:
			LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1));
			ihdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
			break;

		default:
			break;
	}
	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K4H8_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	s5k4h8_MIPI_RAW_SensorInit	*/
