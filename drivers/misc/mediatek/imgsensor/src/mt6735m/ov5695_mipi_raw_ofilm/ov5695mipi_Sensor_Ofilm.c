/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 OV5695mipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
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

//add by liuzhen
#include <linux/slab.h>
//add end
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov5695mipi_Sensor_Ofilm.h"

#define PFX "ov5695_Ofilm_camera_sensor"
//#define LOG_WRN(format, args...) xlog_printk(ANDROID_LOG_WARN ,PFX, "[%S] " format, __FUNCTION__, ##args)
//#defineLOG_INF(format, args...) xlog_printk(ANDROID_LOG_INFO ,PFX, "[%s] " format, __FUNCTION__, ##args)
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)

#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

//add devinfo by liuzhen 20160104
#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT 
#include  <linux/dev_info.h>
static struct devinfo_struct *s_DEVINFO_ccm;  
#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = OV5695_OFILM_SENSOR_ID,		//record sensor id defined in Kd_imgsensor.h
	
	.checksum_value = 0xc8106c60,		//checksum value for Camera Auto Test
	
	.pre = {
		.pclk = 45000000,				//record different mode's pclk
		.linelength = 672,				//record different mode's linelength
		.framelength = 2232,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1296,		//record different mode's width of grabwindow
		.grabwindow_height = 972,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14, //85,//85 unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 45000000,
		.linelength = 740,
		.framelength = 2024,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns modify from 85 by liuzhen
		.max_framerate = 300,
	},
	.cap1 = {							//capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 45000000,
		.linelength = 740,
		.framelength = 4048,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns 
		.max_framerate = 150,	//less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps  
	},
	.normal_video = {
		.pclk = 45000000,
		.linelength = 672,
		.framelength = 2232,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1296,
		.grabwindow_height = 972,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 45000000,
		.linelength = 672,
		.framelength = 558,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 45000000,
		.linelength = 672,
		.framelength = 2232,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1296,
		.grabwindow_height = 972,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns
		.max_framerate = 300,
	},
	.margin = 4,			//sensor framelength & shutter margin
	.min_shutter = 4,		//min shutter
	.max_frame_length = 0x7fff,//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	
	.cap_delay_frame = 2,		//enter capture delay frame num
	.pre_delay_frame = 2, 		//enter preview delay frame num
	.video_delay_frame = 2,		//enter video delay frame num
	.hs_video_delay_frame = 2,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,//enter slim video delay frame num
	
	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
	.i2c_addr_table = {0x20,0x6c, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_H_MIRROR,//IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x4C00,					//current shutter
	.gain = 0x0200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{{ 2624, 1980,	  0,	0, 2624, 1980, 1312,  990, 0000, 0000, 1312,   990,	  8,	6, 1296,  972}, // Preview 
 { 2624, 1980,	  0,	4, 2624, 1972, 2624, 1972, 0000, 0000, 2624,  1972,	 16,	8, 2592, 1944}, // capture 
 { 2624, 1980,	  0,	0, 2624, 1980, 1312,  990, 0000, 0000, 1312,   990,	  8,	6, 1296,  972}, // video 
 { 2624, 1980,	  0,	8, 2624, 1972,  656,  493, 0000, 0000,  656,   480,	  8,	4, 	640,  480}, //hight speed video 
 { 2624, 1980,	  0,	0, 2624, 1980, 1312,  990, 0000, 0000, 1312,   990,	  8,	6, 1296,  972}};// slim video 


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

//add otp code
#define OTP_SLAVE_ID2 0x20
static kal_uint16 read_otp_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, OTP_SLAVE_ID2);

	return get_byte;
}

static void write_otp_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, OTP_SLAVE_ID2);
}

static kal_uint8 read_Ofilm_otp_mid(void)
{
	int otp_flag, addr, i;
	int useful_group = 0;
	kal_uint8 otp_mid = 0;
	kal_uint8 lens_id = 0;
	
	// read OTP into buffer
	write_otp_cmos_sensor(0x3d84, 0xC0);
	write_otp_cmos_sensor(0x3d88, 0x70); // OTP start address
	write_otp_cmos_sensor(0x3d89, 0x0C);
	write_otp_cmos_sensor(0x3d8A, 0x70); // OTP end address
	write_otp_cmos_sensor(0x3d8B, 0x1B);
	write_otp_cmos_sensor(0x3d81, 0x01); // load otp into buffer
	mDELAY(5);
	// OTP base information and WB calibration data
	otp_flag = read_otp_cmos_sensor(0x700C);
	printk("Ofilm:OV5695_otp:otp_flag =0x%x\n",otp_flag);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) 
	{
		printk("Ofilm:OV5695_otp:Group1 valid\n");
		useful_group = 0x1;
		addr = 0x700D; // base address of info group 1
	}
	else {
		if((otp_flag & 0x30) == 0x10) 
		{
			printk("Ofilm:OV5695_otp:Group2 valid\n");
			useful_group = 0x2;
			addr = 0x7012; // base address of info group 2
		}
		else if((otp_flag & 0x0C) == 0x04) 
		{
			printk("Ofilm:OV5695_otp:Group3 valid\n");
			useful_group = 0x3;
			addr = 0x7017; // base address of info group 3
		}
	}
	if(addr != 0) {
		otp_mid = read_cmos_sensor(addr);
		lens_id = read_cmos_sensor(addr + 1);
		printk("Ofilm:OV5695_otp:valid Group=%d,mid=%d\n",useful_group,otp_mid);
	}else {
		printk("Ofilm:OV5695_otp: Invalid otp data\n");
	}
	
	for(i=0x700C;i<=0x701B;i++) 
	{
		write_otp_cmos_sensor(i,0); // clear OTP buffer, recommended use continuous write to accelarate
	}
	
	return otp_mid;
}
#define USE_OTP_CALI_OFILM
#ifdef USE_OTP_CALI_OFILM
static int read_otp(struct otp_struct *otp_ptr)
{
	int otp_flag, addr, temp, i;
	int useful_group = 0;
	int ret = 0;
	
	// read OTP into buffer
	write_otp_cmos_sensor(0x3d84, 0xC0);
	write_otp_cmos_sensor(0x3d88, 0x70); // OTP start address
	write_otp_cmos_sensor(0x3d89, 0x0C);
	write_otp_cmos_sensor(0x3d8A, 0x70); // OTP end address
	write_otp_cmos_sensor(0x3d8B, 0x1B);
	write_otp_cmos_sensor(0x3d81, 0x01); // load otp into buffer
	mDELAY(5);
	// OTP base information and WB calibration data
	otp_flag = read_otp_cmos_sensor(0x700C);
	printk("Ofilm:OV5695_otp:otp_flag =0x%x\n",otp_flag);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) 
	{
		printk("OV5695_otp:Group1 valid\n");
		useful_group = 0x1;
		addr = 0x700D; // base address of info group 1
	}
	else {
		if((otp_flag & 0x30) == 0x10) 
		{
			printk("OV5695_otp:Group2 valid\n");
			useful_group = 0x2;
			addr = 0x7012; // base address of info group 2
		}
		else if((otp_flag & 0x0C) == 0x04) 
		{
			printk("OV5695_otp:Group3 valid\n");
			useful_group = 0x3;
			addr = 0x7017; // base address of info group 3
		}
	}
	if(addr != 0) 
	{
		(*otp_ptr).flag |= 0x80; // valid info and AWB in OTP
		(*otp_ptr).module_integrator_id = read_cmos_sensor(addr);
		(*otp_ptr).lens_id = read_cmos_sensor( addr + 1);
		temp = read_otp_cmos_sensor(addr + 4);
		(*otp_ptr).rg_ratio = (read_cmos_sensor(addr + 2)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (read_cmos_sensor(addr + 3)<<2) + ((temp>>4) & 0x03);
		printk("OV5695_otp:valid Group=%d,Rg=0x%x,Bg=0x%x\n",useful_group,(*otp_ptr).rg_ratio,(*otp_ptr).bg_ratio);
	}
	else 
	{
		(*otp_ptr).flag = 0x00; // not info in OTP
		(*otp_ptr).module_integrator_id = 0;
		(*otp_ptr).lens_id = 0;
		(*otp_ptr).rg_ratio =0;
		(*otp_ptr).bg_ratio = 0;
		ret = -1;
		printk("OV5695_otp: Invalid otp data\n");
	}
	
	for(i=0x700C;i<=0x701B;i++) 
	{
		write_otp_cmos_sensor(i,0); // clear OTP buffer, recommended use continuous write to accelarate
	}
	
	return ret;
}

#define TEST_WB_DATA
static int apply_otp(struct otp_struct *otp_ptr)
{
	int rg, bg, R_gain, G_gain, B_gain, Base_gain;
	int ret = 0;
#ifdef TEST_WB_DATA
	kal_uint16 temH,temL;
	kal_uint16 final_R_Gain,final_G_Gain,final_B_Gain;
#endif
	// apply OTP WB Calibration
	if ((*otp_ptr).flag & 0x80) 
	{
		rg = (*otp_ptr).rg_ratio;
		bg = (*otp_ptr).bg_ratio;
		printk("OV5695_otp:Read from Module:rg=0x%x,bg=0x%x\n",rg,bg);
		printk("OV5695_otp:RG_Ratio_Typical_Ofilm=0x%x,BG_Ratio_Typical_Ofilm=0x%x\n",RG_Ratio_Typical_Ofilm,BG_Ratio_Typical_Ofilm);
		//calculate G gain
		R_gain = (RG_Ratio_Typical_Ofilm*1000) / rg;
		B_gain = (BG_Ratio_Typical_Ofilm*1000) / bg;
		G_gain = 1000;
		if (R_gain < 1000 || B_gain < 1000)
		{
			if (R_gain < B_gain)
		 		Base_gain = R_gain;
			else
				Base_gain = B_gain;
		}
		else
		{
			Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);
		printk("OV5695_otp:After calc:R_gain=0x%x,R_gain=0x%x,G_gain=0x%x\n",R_gain,B_gain,G_gain);
		// update sensor WB gain
		if (R_gain>0x400) 
		{
			write_otp_cmos_sensor(0x5019, R_gain>>8);
			write_otp_cmos_sensor(0x501A, R_gain & 0x00ff);
		}
		if(G_gain>0x400) 
		{
			write_otp_cmos_sensor(0x501B, G_gain>>8);
			write_otp_cmos_sensor(0x501C, G_gain & 0x00ff);
		}
		if(B_gain>0x400) 
		{
			write_otp_cmos_sensor(0x501D, B_gain>>8);
			write_otp_cmos_sensor(0x501E, B_gain & 0x00ff);
		}

#ifdef TEST_WB_DATA
	temH = read_otp_cmos_sensor(0x5019);
	temL = read_otp_cmos_sensor(0x501A);
	final_R_Gain = ( ((temH&0x00ff) << 8) | temL);
	temH = read_otp_cmos_sensor(0x501B);
	temL = read_otp_cmos_sensor(0x501C);
	final_G_Gain = ( ((temH&0x00ff) << 8) | temL);
	temH = read_otp_cmos_sensor(0x501D);
	temL = read_otp_cmos_sensor(0x501E);
	final_B_Gain = ( ((temH&0x00ff) << 8) | temL);
	printk("OV5695_otp:Fianl Read from Sensor,final_R_Gain=0x%x,final_G_Gain=0x%x,final_B_gain=0x%x\n",final_R_Gain,final_G_Gain,final_B_Gain);
#endif
	}else{
		printk("OV5695_otp no otp data\n");
		ret = -1;
	}

	return ret;	
}
//add end
#endif

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
	write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);	  
	write_cmos_sensor(0x380c, (imgsensor.line_length >> 8) & 0xFF);
	write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
  
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable=%d \n", framerate,min_framelength_en);
   
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
    unsigned long flags;
	kal_uint16 realtime_fps = 0;
	printk("sensorDebug:enter! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

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
	imgsensor.frame_length = (imgsensor.frame_length>>1)<<1;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
	shutter = (shutter>>1)<<1;
	
	if (imgsensor.autoflicker_en) { 
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
        else {
        // Extend frame length
        write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
        write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
        }
	} else {
		// Extend frame length
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8)& 0xFF);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	write_cmos_sensor(0x3502, (shutter << 4) & 0xF0);
	write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);	  
	write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);	
	printk("sensorDebug:Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);
	
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
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	write_shutter(shutter);
}	/*	set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;
	//kal_uint16 iGain=gain;
	
	iReg = gain*16/BASEGAIN;
		if(iReg < 0x10)
		{
			iReg = 0x10;
		}
		if(iReg > 0xf8)
		{
			iReg = 0xf8;
		}
	return iReg;
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
 
		reg_gain = gain2reg(gain);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.gain = reg_gain; 
		spin_unlock(&imgsensor_drv_lock);
		printk("sensorDebug:gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);
	
		write_cmos_sensor(0x3508, reg_gain >> 8);
		write_cmos_sensor(0x3509, reg_gain & 0xFF);    
		
		//return gain;
	
	return gain;
}	/*	set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);

	write_cmos_sensor(0x3820, 0x81);   //enable ihdr
 	
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
				write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8)& 0xFF);
				write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);	 
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);
		
		write_cmos_sensor(0x3508, (se << 4) & 0xFF); 
		write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
		write_cmos_sensor(0x3506, (se >> 12) & 0x0F); 

		set_gain(gain);
	}

}


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

	/********************************************************
	   *
	   *   0x3820[3]=1  hmirror
	   *   0x3820[5:4]=11  Vertical flip
	   *
	   ********************************************************/
	
	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) & 0xF7)));//mirror off
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) & 0xcf)));//flip off
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) | 0x08)));//mirror on
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) & 0xcf)));//flip off
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) & 0xF7)));//mirror off
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) | 0x30)));//flip on
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) | 0x08)));//mirror on
			write_cmos_sensor(0x3820,((read_cmos_sensor(0x3820) | 0x30)));//flip on
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}

}
#endif

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
	LOG_INF("OV5695_Sensor_Init_2lane E\n");
	//OV5695R1A_AM07
	//@@Init
//;;1296X972_HBIN_VBIN_30FPS_MIPI_2_LANE


//;Xclk 24Mhz
//;Pclk clock frequency: 45Mhz
//;linelength = 672(0x2a0)
//;framelength = 2232(0x8b8)
//;grabwindow_width  = 1296
//;grabwindow_height = 972
//;max_framerate: 30fps	
//;mipi_datarate per lane: 840Mbps

		write_cmos_sensor(0x0103, 0x01);
		mDELAY(10);
		write_cmos_sensor(0x0100, 0x00);
		write_cmos_sensor(0x0300, 0x04);
		write_cmos_sensor(0x0301, 0x00);
		write_cmos_sensor(0x0302, 0x69);
		write_cmos_sensor(0x0303, 0x00);
		write_cmos_sensor(0x0304, 0x00);
		write_cmos_sensor(0x0305, 0x01);
		write_cmos_sensor(0x0307, 0x00);
		write_cmos_sensor(0x030b, 0x00);
		write_cmos_sensor(0x030c, 0x00);
		write_cmos_sensor(0x030d, 0x1e);
		write_cmos_sensor(0x030e, 0x04); //05 neil
		write_cmos_sensor(0x030f, 0x03);
		write_cmos_sensor(0x0312, 0x01);
		write_cmos_sensor(0x3000, 0x00);
		write_cmos_sensor(0x3002, 0x21);
		write_cmos_sensor(0x3022, 0x51);
		write_cmos_sensor(0x3106, 0x15);
		write_cmos_sensor(0x3107, 0x01);
		write_cmos_sensor(0x3108, 0x05);
		write_cmos_sensor(0x3500, 0x00);
		write_cmos_sensor(0x3501, 0x45);
		write_cmos_sensor(0x3502, 0x00);
		write_cmos_sensor(0x3503, 0x08);
		write_cmos_sensor(0x3504, 0x03);
		write_cmos_sensor(0x3505, 0x8c);
		write_cmos_sensor(0x3507, 0x03);
		write_cmos_sensor(0x3508, 0x00);
		write_cmos_sensor(0x3509, 0x10);
		write_cmos_sensor(0x350c, 0x00);
		write_cmos_sensor(0x350d, 0x80);
		write_cmos_sensor(0x3510, 0x00);
		write_cmos_sensor(0x3511, 0x02);
		write_cmos_sensor(0x3512, 0x00);
		write_cmos_sensor(0x3601, 0x55);
		write_cmos_sensor(0x3602, 0x58);
		write_cmos_sensor(0x3614, 0x30);
		write_cmos_sensor(0x3615, 0x77);
		write_cmos_sensor(0x3621, 0x08);
		write_cmos_sensor(0x3624, 0x40);
		write_cmos_sensor(0x3633, 0x0c);
		write_cmos_sensor(0x3634, 0x0c);
		write_cmos_sensor(0x3635, 0x0c);
		write_cmos_sensor(0x3636, 0x0c);
		write_cmos_sensor(0x3638, 0x00); //11 neil
		write_cmos_sensor(0x3639, 0x00); //15 neil
		write_cmos_sensor(0x363a, 0x00); //1f neil
		write_cmos_sensor(0x363b, 0x00); //1f neil
		write_cmos_sensor(0x363c, 0xff);
		write_cmos_sensor(0x363d, 0xfa);
		write_cmos_sensor(0x3650, 0x44);
		write_cmos_sensor(0x3651, 0x44);
		write_cmos_sensor(0x3652, 0x44);
		write_cmos_sensor(0x3653, 0x44);
		write_cmos_sensor(0x3654, 0x44);
		write_cmos_sensor(0x3655, 0x44);
		write_cmos_sensor(0x3656, 0x44);
		write_cmos_sensor(0x3657, 0x44);
		write_cmos_sensor(0x3660, 0x00);
		write_cmos_sensor(0x3661, 0x00);
		write_cmos_sensor(0x3662, 0x00);
		write_cmos_sensor(0x366a, 0x00);
		write_cmos_sensor(0x366e, 0x0c);
		write_cmos_sensor(0x3673, 0x04);
		write_cmos_sensor(0x3700, 0x14);
		write_cmos_sensor(0x3703, 0x0c);
		write_cmos_sensor(0x3715, 0x01);
		write_cmos_sensor(0x3733, 0x10);
		write_cmos_sensor(0x3734, 0x40);
		write_cmos_sensor(0x373f, 0xa0);
		write_cmos_sensor(0x3765, 0x20);
		write_cmos_sensor(0x37a1, 0x1d);
		write_cmos_sensor(0x37a8, 0x26);
		write_cmos_sensor(0x37ab, 0x14);
		write_cmos_sensor(0x37c2, 0x04);
		write_cmos_sensor(0x37cb, 0x09);
		write_cmos_sensor(0x37cc, 0x13);
		write_cmos_sensor(0x37cd, 0x1f);
		write_cmos_sensor(0x37ce, 0x1f);
		write_cmos_sensor(0x3800, 0x00);
		write_cmos_sensor(0x3801, 0x00);
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x00);
		write_cmos_sensor(0x3804, 0x0a);
		write_cmos_sensor(0x3805, 0x3f);
		write_cmos_sensor(0x3806, 0x07);
		write_cmos_sensor(0x3807, 0xaf);
		write_cmos_sensor(0x3808, 0x05);
		write_cmos_sensor(0x3809, 0x10);
		write_cmos_sensor(0x380a, 0x03);
		write_cmos_sensor(0x380b, 0xcc);
		write_cmos_sensor(0x380c, 0x02);
		write_cmos_sensor(0x380d, 0xa0);
		write_cmos_sensor(0x380e, 0x08);
		write_cmos_sensor(0x380f, 0xb8);
		write_cmos_sensor(0x3810, 0x00);
		write_cmos_sensor(0x3811, 0x06); //08 neil
		write_cmos_sensor(0x3812, 0x00);
		write_cmos_sensor(0x3813, 0x06);
		write_cmos_sensor(0x3814, 0x03);
		write_cmos_sensor(0x3815, 0x01);
		write_cmos_sensor(0x3816, 0x03);
		write_cmos_sensor(0x3817, 0x01);
		write_cmos_sensor(0x3818, 0x00);
		write_cmos_sensor(0x3819, 0x00);
		write_cmos_sensor(0x381a, 0x00);
		write_cmos_sensor(0x381b, 0x01);
		write_cmos_sensor(0x3820, 0x8b);
		write_cmos_sensor(0x3821, 0x01);
		write_cmos_sensor(0x3c80, 0x08);
		write_cmos_sensor(0x3c82, 0x00);
		write_cmos_sensor(0x3c83, 0x00);
		write_cmos_sensor(0x3c88, 0x00);
		write_cmos_sensor(0x3d85, 0x14);
		write_cmos_sensor(0x3f02, 0x08); //02 neil
		write_cmos_sensor(0x3f03, 0x10); //04 neil
		write_cmos_sensor(0x4008, 0x02);
		write_cmos_sensor(0x4009, 0x09);
		write_cmos_sensor(0x404e, 0x20);
		write_cmos_sensor(0x4501, 0x00);
		write_cmos_sensor(0x4502, 0x10);
		write_cmos_sensor(0x4800, 0x00);
		write_cmos_sensor(0x481f, 0x2a);
		write_cmos_sensor(0x4837, 0x13);
		write_cmos_sensor(0x5000, 0x17);
		write_cmos_sensor(0x5780, 0x3e);
		write_cmos_sensor(0x5781, 0x0f);
		write_cmos_sensor(0x5782, 0x44);
		write_cmos_sensor(0x5783, 0x02);
		write_cmos_sensor(0x5784, 0x01);
		write_cmos_sensor(0x5785, 0x01);
		write_cmos_sensor(0x5786, 0x00);
		write_cmos_sensor(0x5787, 0x04);
		write_cmos_sensor(0x5788, 0x02);
		write_cmos_sensor(0x5789, 0x0f);
		write_cmos_sensor(0x578a, 0xfd);
		write_cmos_sensor(0x578b, 0xf5);
		write_cmos_sensor(0x578c, 0xf5);
		write_cmos_sensor(0x578d, 0x03);
		write_cmos_sensor(0x578e, 0x08);
		write_cmos_sensor(0x578f, 0x0c);
		write_cmos_sensor(0x5790, 0x08);
		write_cmos_sensor(0x5791, 0x06);
		write_cmos_sensor(0x5792, 0x00);
		write_cmos_sensor(0x5793, 0x52);
		write_cmos_sensor(0x5794, 0xa3);
		write_cmos_sensor(0x5b00, 0x00);
		write_cmos_sensor(0x5b01, 0x1c);
		write_cmos_sensor(0x5b02, 0x00);
		write_cmos_sensor(0x5b03, 0x7f);
		write_cmos_sensor(0x5b05, 0x6c);
		write_cmos_sensor(0x5e10, 0xfc);
		write_cmos_sensor(0x4010, 0xf1);
		write_cmos_sensor(0x3503, 0x08);
		write_cmos_sensor(0x3505, 0x8c);
		write_cmos_sensor(0x3507, 0x03);
		write_cmos_sensor(0x3508, 0x00);
		write_cmos_sensor(0x3509, 0xf8);
		write_cmos_sensor(0x4002, 0x00); // add by Ron
		write_cmos_sensor(0x4003, 0x40); // add by Ron		
		write_cmos_sensor(0x0100, 0x01);	//open stream on by liuzhen

}	/*	sensor_init  */


static void preview_setting(void)
{
	LOG_INF(" OV5695PreviewSetting_2lane enter\n");

//@@1296X972_30fps
//;;1296X972_HBIN_VBIN_30FPS_MIPI_2_LANE
//102 3601 1770	
//;Xclk 24Mhz
//;Pclk clock frequency: 45Mhz
//;linelength = 672(0x2a0)
//;framelength = 2232(0x8b8)
//;grabwindow_width  = 1296
//;grabwindow_height = 972
//;max_framerate: 30fps	
//;mipi_datarate per lane: 840Mbps				  
																			  
	write_cmos_sensor(0x0100, 0x00);  // 
    write_cmos_sensor(0x3501, 0x45);
    write_cmos_sensor(0x366e, 0x0c);
    write_cmos_sensor(0x3800, 0x00);
    write_cmos_sensor(0x3801, 0x00);
    write_cmos_sensor(0x3802, 0x00);
    write_cmos_sensor(0x3803, 0x00);
    write_cmos_sensor(0x3804, 0x0a);
    write_cmos_sensor(0x3805, 0x3f);
    write_cmos_sensor(0x3806, 0x07);
    write_cmos_sensor(0x3807, 0xaf);
    write_cmos_sensor(0x3808, 0x05);
    write_cmos_sensor(0x3809, 0x10);
    write_cmos_sensor(0x380a, 0x03);
    write_cmos_sensor(0x380b, 0xcc);
    write_cmos_sensor(0x380c, 0x02);
    write_cmos_sensor(0x380d, 0xa0);
    write_cmos_sensor(0x380e, 0x08);
    write_cmos_sensor(0x380f, 0xb8);
//
    write_cmos_sensor(0x3811, 0x06); //08
    write_cmos_sensor(0x3813, 0x06);
    write_cmos_sensor(0x3814, 0x03);
    write_cmos_sensor(0x3816, 0x03);
    write_cmos_sensor(0x3817, 0x01);
    write_cmos_sensor(0x3820, 0x8b);

	//read reg value

    write_cmos_sensor(0x3821, 0x01);
    write_cmos_sensor(0x4501, 0x00);
    write_cmos_sensor(0x4008, 0x02);
    write_cmos_sensor(0x4009, 0x09);
	write_cmos_sensor(0x0100, 0x01);  // 

	
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("OV5695CaptureSetting_2lane enter! currefps:%d\n",currefps);
	if (currefps == 150) { //15fps for PIP
		write_cmos_sensor(0x0100, 0x00); 
		write_cmos_sensor(0x3501, 0x7e);
		write_cmos_sensor(0x366e, 0x18);
		write_cmos_sensor(0x3800, 0x00);
		write_cmos_sensor(0x3801, 0x00);
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x04);
		write_cmos_sensor(0x3804, 0x0a);
		write_cmos_sensor(0x3805, 0x3f);
		write_cmos_sensor(0x3806, 0x07);
		write_cmos_sensor(0x3807, 0xab);
		write_cmos_sensor(0x3808, 0x0a);
		write_cmos_sensor(0x3809, 0x20);
		write_cmos_sensor(0x380a, 0x07);
		write_cmos_sensor(0x380b, 0x98);
		write_cmos_sensor(0x380c, 0x02);
		write_cmos_sensor(0x380d, 0xe4);
		write_cmos_sensor(0x380e, 0x0f);
		write_cmos_sensor(0x380f, 0xd0);
		write_cmos_sensor(0x3811, 0x06); //10
		write_cmos_sensor(0x3813, 0x08);
		write_cmos_sensor(0x3814, 0x01);
		write_cmos_sensor(0x3816, 0x01);
		write_cmos_sensor(0x3817, 0x01);
		write_cmos_sensor(0x3820, 0x88);
		write_cmos_sensor(0x3821, 0x00);
		write_cmos_sensor(0x4501, 0x00);
		write_cmos_sensor(0x4008, 0x04);
		write_cmos_sensor(0x4009, 0x13);
		write_cmos_sensor(0x0100, 0x01);
		
	} else{ // for 30fps need ti update
		write_cmos_sensor(0x0100, 0x00); 
		write_cmos_sensor(0x3501, 0x7e);
		write_cmos_sensor(0x366e, 0x18);
		write_cmos_sensor(0x3800, 0x00);
		write_cmos_sensor(0x3801, 0x00);
		write_cmos_sensor(0x3802, 0x00);
		write_cmos_sensor(0x3803, 0x04);
		write_cmos_sensor(0x3804, 0x0a);
		write_cmos_sensor(0x3805, 0x3f);
		write_cmos_sensor(0x3806, 0x07);
		write_cmos_sensor(0x3807, 0xab);
		write_cmos_sensor(0x3808, 0x0a);
		write_cmos_sensor(0x3809, 0x20);
		write_cmos_sensor(0x380a, 0x07);
		write_cmos_sensor(0x380b, 0x98);
		write_cmos_sensor(0x380c, 0x02);
		write_cmos_sensor(0x380d, 0xe4);
		write_cmos_sensor(0x380e, 0x07);
		write_cmos_sensor(0x380f, 0xe8);
		write_cmos_sensor(0x3811, 0x06); //10
		write_cmos_sensor(0x3813, 0x08);
		write_cmos_sensor(0x3814, 0x01);
		write_cmos_sensor(0x3816, 0x01);
		write_cmos_sensor(0x3817, 0x01);
		write_cmos_sensor(0x3820, 0x88);
		write_cmos_sensor(0x3821, 0x00);
		write_cmos_sensor(0x4501, 0x00);
		write_cmos_sensor(0x4008, 0x04);
		write_cmos_sensor(0x4009, 0x13);	
		write_cmos_sensor(0x0100, 0x01);
		
	} 
	
		
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("normal_video_setting Enter! currefps:%d\n",currefps);
	preview_setting();
}
static void hs_video_setting(void)
{ 
	LOG_INF("hs_video_setting enter!\n");

//VGA 120fps
	write_cmos_sensor(0x0100, 0x00);
	write_cmos_sensor(0x3501, 0x22);
	write_cmos_sensor(0x366e, 0x0c);
	write_cmos_sensor(0x3800, 0x00);
	write_cmos_sensor(0x3801, 0x00);
	write_cmos_sensor(0x3802, 0x00);
	write_cmos_sensor(0x3803, 0x08);
	write_cmos_sensor(0x3804, 0x0a);
	write_cmos_sensor(0x3805, 0x3f);
	write_cmos_sensor(0x3806, 0x07);
	write_cmos_sensor(0x3807, 0xa7);
	write_cmos_sensor(0x3808, 0x02);
	write_cmos_sensor(0x3809, 0x80);
	write_cmos_sensor(0x380a, 0x01);
	write_cmos_sensor(0x380b, 0xe0);
	write_cmos_sensor(0x380c, 0x02);
	write_cmos_sensor(0x380d, 0xa0);
	write_cmos_sensor(0x380e, 0x02);
	write_cmos_sensor(0x380f, 0x2e);
	write_cmos_sensor(0x3811, 0x06); //08
	write_cmos_sensor(0x3813, 0x04);
	write_cmos_sensor(0x3814, 0x07);
	write_cmos_sensor(0x3816, 0x05);
	write_cmos_sensor(0x3817, 0x03);
	write_cmos_sensor(0x3820, 0x8d);
	write_cmos_sensor(0x3821, 0x01);
	write_cmos_sensor(0x4501, 0x00);
	write_cmos_sensor(0x4008, 0x02);
	write_cmos_sensor(0x4009, 0x09);
	write_cmos_sensor(0x0100, 0x01);


}


static void slim_video_setting(void)
{
	LOG_INF("slim_video_setting enter!\n");
	preview_setting();

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
	kal_uint8 otp_mid = 0;

#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT 
 	s_DEVINFO_ccm =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_ccm->device_type = "CCM-S";
	s_DEVINFO_ccm->device_module = "L5695F40";
	s_DEVINFO_ccm->device_vendor = "Ofilm";
	s_DEVINFO_ccm->device_ic = "OV5695";
	s_DEVINFO_ccm->device_version = "OmniVision";
	s_DEVINFO_ccm->device_info = "500W";
#endif

	printk("Ofilm:ov5695 get sensor id\n");
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor(0x300B) << 8) | read_cmos_sensor(0x300C));
			*sensor_id = *sensor_id +2;
			if (*sensor_id == imgsensor_info.sensor_id) {				
				printk("ov5695=>i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);	  
				break;
			}	
			printk("ov5695=>Read sensor id fail, I2c addr=0x%x,sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		if(*sensor_id == imgsensor_info.sensor_id) break;
		i++;
		retry = 2;
	}

	/* initail sequence write in  */
	sensor_init();
	otp_mid = read_Ofilm_otp_mid();
	printk("Ofilm:ov5695_otp:otp_mid=%d\n",otp_mid);
	if ((*sensor_id != imgsensor_info.sensor_id) || (otp_mid != 0x07)) {
	#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT 
		s_DEVINFO_ccm->device_used = DEVINFO_UNUSED;
		devinfo_check_add_device(s_DEVINFO_ccm);
	#endif
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF 
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT 
	s_DEVINFO_ccm->device_used = DEVINFO_USED;
	devinfo_check_add_device(s_DEVINFO_ccm);
#endif

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
	kal_uint16 sensor_id = 0; 
#ifdef USE_OTP_CALI_OFILM
	struct otp_struct *otp_ptr = NULL;
#endif

	LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");
	
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor(0x300B) << 8) | read_cmos_sensor(0x300C));
			sensor_id = sensor_id + 2;
			if (sensor_id == imgsensor_info.sensor_id) {				
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);	  
				break;
			}	
			LOG_INF("Read sensor id fail, i2c write id: 0x%x,sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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

#ifdef USE_OTP_CALI_OFILM
	mdelay(10);
	printk("%s,OV5695_otp: start\n",__func__);
	otp_ptr = (struct otp_struct *)kmalloc(sizeof(struct otp_struct), GFP_KERNEL);
	if(otp_ptr != NULL){
		if(0 == read_otp(otp_ptr))
			apply_otp(otp_ptr);
		else
			printk("%s,OV5695_otp:Otp cali fail.\n",__func__);
		kfree(otp_ptr);
	}else{
		printk("%s,OV5695_otp:allocate memory fail.\n",__func__);	
	}
	printk("%s,OV5695_otp: end\n",__func__);
#endif

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x4C00;
	imgsensor.gain = 0x0200;
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
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	
	return ERROR_NONE;
}	/*	hs_video   */

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
	
	return ERROR_NONE;
}	/*	slim_video	 */



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

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		write_cmos_sensor(0x4503,0x80);
		write_cmos_sensor(0x5001,0xc2);
	} else {
		write_cmos_sensor(0x4503,0x00);
		write_cmos_sensor(0x5001,0xca);
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
	unsigned long long *feature_data = (unsigned long long*)feature_para;
	//unsigned long long *feature_return_data = (unsigned long long*)feature_para;
	
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
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
            set_video_mode(*feature_data_16);
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
            LOG_INF("current fps :%d\n", (UINT32)*feature_data);
			spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
			spin_unlock(&imgsensor_drv_lock);		
			break;
		case SENSOR_FEATURE_SET_HDR:
            LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_en = *feature_data;
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
            ihdr_write_shutter_gain(*feature_data,*(feature_data+1),*(feature_data+2));
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

UINT32 OV5695_MIPI_RAW_SensorInit_Ofilm(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	OV5695_MIPI_RAW_SensorInit	*/

