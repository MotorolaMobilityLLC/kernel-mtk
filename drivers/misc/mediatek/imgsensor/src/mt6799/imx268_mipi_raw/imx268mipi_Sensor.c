/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     IMX268mipi_Sensor.c
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
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx268mipi_Sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "IMX268_camera_sensor"
#define LOG_1 LOG_INF("IMX268,MIPI 4LANE\n")
#define LOG_2 LOG_INF("preview 2672*2008@30fps; video 5344*4016@30fps; capture 21M@24fps\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#if 0
/*******************************************************************************
* Proifling
********************************************************************************/
#define PROFILE 0
#if PROFILE
static struct timeval tv1, tv2;
static DEFINE_SPINLOCK(kdsensor_drv_lock);
/*******************************************************************************
*
********************************************************************************/
inline void KD_SENSOR_PROFILE_INIT(void)
{
    do_gettimeofday(&tv1);
}

/*******************************************************************************
*
********************************************************************************/
inline void KD_SENSOR_PROFILE(char *tag)
{
    unsigned long TimeIntervalUS;

    spin_lock(&kdsensor_drv_lock);

    do_gettimeofday(&tv2);
    TimeIntervalUS = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    tv1 = tv2;

    spin_unlock(&kdsensor_drv_lock);
    LOG_INF("[%s]Profile = %lu us\n", tag, TimeIntervalUS);
}
#else
inline void KD_SENSOR_PROFILE_INIT(void) {}
inline void KD_SENSOR_PROFILE(char *tag) {}
#endif
#endif

#define BYTE               unsigned char

/*static BOOL read_spc_flag = FALSE;*/

/*support ZHDR*/
//#define IMX268_ZHDR

static DEFINE_SPINLOCK(imgsensor_drv_lock);

/*static BYTE imx268_SPC_data[352]={0};*/

//extern void read_imx268_SPC( BYTE* data );
//extern void read_imx268_DCC( kal_uint16 addr,BYTE* data, kal_uint32 size);



static imgsensor_info_struct imgsensor_info = {
    .sensor_id = IMX268_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0x6c259b92,        //checksum value for Camera Auto Test

    .pre = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .cap = {/*data rate 1499.20 Mbps/lane*/
        .pclk = 279600000,
        .linelength = 4200,
        .framelength = 2218,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 3872,
        .grabwindow_height =  2192,
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        .max_framerate = 300,
    },
	.cap1 = {/*data rate 1499.20 Mbps/lane*/
        .pclk = 279600000,
        .linelength = 4200,
        .framelength = 2218,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 3872,
        .grabwindow_height =  2192,
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        .max_framerate = 300,
	},

    .normal_video = {/*data rate 1499.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .hs_video = {/*data rate 600 Mbps/lane*/
	    .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .slim_video = {/*data rate 792 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
	.custom1 = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
	},

	.custom2 = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
	},
	.custom3 = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
	},
	.custom4 = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,

	},
	.custom5 = {/*data rate 1099.20 Mbps/lane*/
        .pclk = 135600000,                //record different mode's pclk
        .linelength = 4008,                //record different mode's linelength
        .framelength = 1122,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1932,        //record different mode's width of grabwindow
        .grabwindow_height = 1096,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
	},
    .margin = 10,            //sensor framelength & shutter margin
    .min_shutter = 1,        //min shutter
    .max_frame_length = 0xffff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 10,      //support sensor mode num

    .cap_delay_frame = 1,        //enter capture delay frame num
    .pre_delay_frame = 2,         //enter preview delay frame num
    .video_delay_frame = 1,        //enter video delay frame num
    .hs_video_delay_frame = 3,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 3,//enter slim video delay frame num

	.isp_driving_current = ISP_DRIVING_6MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
    .i2c_addr_table = {0x34, 0x20, 0x6c, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
   	.i2c_speed = 400, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_NORMAL,                //mirrorflip information
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3D0,                    //current shutter
    .gain = 0x100,                        //current gain
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .hdr_mode = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x6c,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] =
{{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096}, // Preview
{ 3872, 2192,    0,    0, 3872, 2192, 3872, 2192, 0000, 0000, 3872, 2192,      0,    0, 3872, 2192}, // capture
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096},
{ 3872, 2192,    0,    0, 3872, 2192, 1936, 1096,    2, 0000, 1932, 1096,      0,    0, 1932, 1096}};

#if 0

 /*VC1 for HDR(DT=0X35) , VC2 for PDAF(DT=0X36), unit : 10bit*/
 static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
 {/* Preview mode setting */
 {0x03, 0x0a,   0x00,   0x08, 0x40, 0x00,
  0x00, 0x2b, 0x0A70, 0x07D8, 0x00, 0x35, 0x0280, 0x0001,
  0x00, 0x36, 0x0C48, 0x0001, 0x03, 0x00, 0x0000, 0x0000},
  /* Capture mode setting */
  {0x03, 0x0a,	 0x00,	 0x08, 0x40, 0x00,
   0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x35, 0x0280, 0x0001,
  0x00, 0x36, 0x1a18, 0x0001, 0x03, 0x00, 0x0000, 0x0000},
   /* Video mode setting */
  {0x02, 0x0a,	 0x00,	 0x08, 0x40, 0x00,
   0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x35, 0x0280, 0x0001,
   0x02, 0x00, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};


typedef struct
{
    MUINT16 DarkLimit_H;
    MUINT16 DarkLimit_L;
    MUINT16 OverExp_Min_H;
    MUINT16 OverExp_Min_L;
    MUINT16 OverExp_Max_H;
    MUINT16 OverExp_Max_L;
}SENSOR_ATR_INFO, *pSENSOR_ATR_INFO;
#endif
#if 0
static SENSOR_ATR_INFO sensorATR_Info[4]=
{/* Strength Range Min */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* Strength Range Std */
    {0x00, 0x32, 0x00, 0x3c, 0x03, 0xff},
    /* Strength Range Max */
    {0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff},
    /* Strength Range Custom */
    {0x3F, 0xFF, 0x00, 0x0, 0x3F, 0xFF}};
#endif

#define IMX268MIPI_MaxGainIndex (115)
kal_uint16 IMX268MIPI_sensorGainMapping[IMX268MIPI_MaxGainIndex][2] ={
	{64 ,0	},
	{65 ,8	},
	{66 ,16 },
	{67 ,25 },
	{68 ,30 },
	{69 ,37 },
	{70 ,45 },
	{71 ,51 },
	{72 ,57 },
	{73 ,63 },
	{74 ,67 },
	{75 ,75 },
	{76 ,81 },
	{77 ,85 },
	{78 ,92 },
	{79 ,96 },
	{80 ,103},
	{81 ,107},
	{82 ,112},
	{83 ,118},
	{84 ,122},
	{86 ,133},
	{88 ,140},
	{89 ,144},
	{90 ,148},
	{93 ,159},
	{96 ,171},
	{97 ,175},
	{99 ,182},
	{101,188},
	{102,192},
	{104,197},
	{106,202},
	{107,206},
	{109,211},
	{112,220},
	{113,222},
	{115,228},
	{118,235},
	{120,239},
	{125,250},
	{126,252},
	{128,256},
	{129,258},
	{130,260},
	{132,264},
	{133,266},
	{135,269},
	{136,271},
	{138,274},
	{139,276},
	{141,279},
	{142,282},
	{144,285},
	{145,286},
	{147,290},
	{149,292},
	{150,294},
	{155,300},
	{157,303},
	{158,305},
	{161,309},
	{163,311},
	{170,319},
	{172,322},
	{174,324},
	{176,326},
	{179,329},
	{181,331},
	{185,335},
	{189,339},
	{193,342},
	{195,344},
	{196,345},
	{200,348},
	{202,350},
	{205,352},
	{207,354},
	{210,356},
	{211,357},
	{214,359},
	{217,361},
	{218,362},
	{221,364},
	{224,366},
	{231,370},
	{237,374},
	{246,379},
	{250,381},
	{252,382},
	{256,384},
	{260,386},
	{262,387},
	{273,392},
	{275,393},
	{280,395},
	{290,399},
	{306,405},
	{312,407},
	{321,410},
	{331,413},
	{345,417},
	{352,419},
	{360,421},
	{364,422},
	{372,424},
	{386,427},
	{400,430},
	{410,432},
	{420,434},
	{431,436},
	{437,437},
	{449,439},
	{468,442},
	{512,448},
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
#if 0
static kal_uint32 imx268_ATR(UINT16 DarkLimit, UINT16 OverExp)
{
/*
    write_cmos_sensor(0x6e50,sensorATR_Info[DarkLimit].DarkLimit_H);
    write_cmos_sensor(0x6e51,sensorATR_Info[DarkLimit].DarkLimit_L);
    write_cmos_sensor(0x9340,sensorATR_Info[OverExp].OverExp_Min_H);
    write_cmos_sensor(0x9341,sensorATR_Info[OverExp].OverExp_Min_L);
    write_cmos_sensor(0x9342,sensorATR_Info[OverExp].OverExp_Max_H);
    write_cmos_sensor(0x9343,sensorATR_Info[OverExp].OverExp_Max_L);
    write_cmos_sensor(0x9706,0x10);
    write_cmos_sensor(0x9707,0x03);
    write_cmos_sensor(0x9708,0x03);
    write_cmos_sensor(0x9e24,0x00);
    write_cmos_sensor(0x9e25,0x8c);
    write_cmos_sensor(0x9e26,0x00);
    write_cmos_sensor(0x9e27,0x94);
    write_cmos_sensor(0x9e28,0x00);
    write_cmos_sensor(0x9e29,0x96);
    LOG_INF("DarkLimit 0x6e50(0x%x), 0x6e51(0x%x)\n",sensorATR_Info[DarkLimit].DarkLimit_H,
                                                     sensorATR_Info[DarkLimit].DarkLimit_L);
    LOG_INF("OverExpMin 0x9340(0x%x), 0x9341(0x%x)\n",sensorATR_Info[OverExp].OverExp_Min_H,
                                                     sensorATR_Info[OverExp].OverExp_Min_L);
    LOG_INF("OverExpMin 0x9342(0x%x), 0x9343(0x%x)\n",sensorATR_Info[OverExp].OverExp_Max_H,
                                                     sensorATR_Info[OverExp].OverExp_Max_L);
                                                     */
    return ERROR_NONE;
}
#endif
#if 0
static MUINT32 cur_startpos = 0;
static MUINT32 cur_size = 0;
static void imx268_set_pd_focus_area(MUINT32 startpos, MUINT32 size)
{
	UINT16 start_x_pos, start_y_pos, end_x_pos, end_y_pos;
	UINT16 focus_width, focus_height;

	if((cur_startpos == startpos) && (cur_size == size))
	{
		LOG_INF("Not to need update focus area!\n");
		return;
	}
	else
	{
		cur_startpos = startpos;
		cur_size = size;
	}

	start_x_pos = (startpos >> 16) & 0xFFFF;
	start_y_pos = startpos & 0xFFFF;
	focus_width = (size >> 16) & 0xFFFF;
	focus_height = size & 0xFFFF;

	end_x_pos = start_x_pos + focus_width;
	end_y_pos = start_y_pos + focus_height;

	if(imgsensor.pdaf_mode == 1)
	{
		LOG_INF("GC pre PDAF\n");
		/*PDAF*/
		/*PD_CAL_ENALBE*/
		write_cmos_sensor(0x3121,0x01);
		/*AREA MODE*/
		write_cmos_sensor(0x31B0,0x02);// 8x6 output
		write_cmos_sensor(0x31B4,0x01);// 8x6 output
		/*PD_OUT_EN=1*/
		write_cmos_sensor(0x3123,0x01);

		/*Fixed area mode*/

		write_cmos_sensor(0x3158,(start_x_pos >> 8) & 0xFF);
		write_cmos_sensor(0x3159,start_x_pos & 0xFF);// X start
		write_cmos_sensor(0x315a,(start_y_pos >> 8) & 0xFF);
		write_cmos_sensor(0x315b,start_y_pos & 0xFF);// Y start
		write_cmos_sensor(0x315c,(end_x_pos >> 8) & 0xFF);
		write_cmos_sensor(0x315d,end_x_pos & 0xFF);//X end
		write_cmos_sensor(0x315e,(end_y_pos >> 8) & 0xFF);
		write_cmos_sensor(0x315f,end_y_pos & 0xFF);// Y end


	}


	LOG_INF("start_x_pos:%d, start_y_pos:%d, focus_width:%d, focus_height:%d, end_x_pos:%d, end_y_pos:%d\n", \
			start_x_pos, start_y_pos, focus_width, focus_height, end_x_pos, end_y_pos);

	return;
}

static void imx268_apply_SPC(void)
{
	unsigned int start_reg = 0x7c00;
	int i;

	if(read_spc_flag == FALSE)
	{
		//read_imx268_SPC(imx268_SPC_data);
		read_spc_flag = TRUE;
		return;
	}

	for(i=0;i<352;i++)
	{
		write_cmos_sensor(start_reg, imx268_SPC_data[i]);
	//	LOG_INF("SPC[%d]= %x \n", i , imx268_SPC_data[i]);

		start_reg++;
	}

}
#endif
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
	return ((read_cmos_sensor(0x0016) << 8) | read_cmos_sensor(0x0017));
}

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    LOG_INF("framerate = %d, min framelength should enable %d \n", framerate,min_framelength_en);

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
#define MAX_CIT_LSHIFT 7
static void set_shutter(kal_uint32 shutter)
{
    unsigned long flags;
    kal_uint16 realtime_fps = 0;
    kal_uint16 l_shift = 1;
	LOG_INF("Enter! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
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
	if(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)){//long expsoure
		for( l_shift = 1; l_shift<MAX_CIT_LSHIFT; l_shift++){
			if((shutter >> l_shift)<(imgsensor_info.max_frame_length - imgsensor_info.margin))
				{
					break;
				}
		}
		if( l_shift > MAX_CIT_LSHIFT ){
			LOG_INF("Unable to set such a long exposure %d, set to max\n", shutter);
			  l_shift = MAX_CIT_LSHIFT;
		}
		shutter = shutter >> l_shift;
		imgsensor.frame_length = shutter + imgsensor_info.margin;//imgsensor_info.max_frame_length;
		//LOG_INF("0x3028 0x%x l_shift %d l_shift&0x3 %d\n", read_cmos_sensor(0x3028),l_shift,l_shift&0x7);
		write_cmos_sensor(0x3028,read_cmos_sensor(0x3028)|(l_shift&0x7));
		//LOG_INF("0x3028 0x%x\n", read_cmos_sensor(0x3028));

	} else {
		write_cmos_sensor(0x3028,read_cmos_sensor(0x3028)&0xc);
	}

	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
	else if (realtime_fps >= 237 && realtime_fps <= 243 )
		set_max_framerate(236,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        // Extend frame length
 		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
        }
    } else {
        // Extend frame length
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
    }

    // Update Shutter
	write_cmos_sensor(0x0104, 0x01);
    write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    write_cmos_sensor(0x0203, shutter  & 0xFF);
    write_cmos_sensor(0x0104, 0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}    /*    set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint8 iI;
    LOG_INF("[IMX268MIPI]enter IMX268MIPIGain2Reg function\n");
    for (iI = 0; iI < IMX268MIPI_MaxGainIndex; iI++)
	{
		if (gain <= IMX268MIPI_sensorGainMapping[iI][0])
			return IMX268MIPI_sensorGainMapping[iI][1];


    }
	LOG_INF("exit IMX268MIPIGain2Reg function\n");
    return IMX268MIPI_sensorGainMapping[iI-1][1];
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
    /* Global analog Gain for Long expo*/
    write_cmos_sensor(0x0204, (reg_gain>>8)& 0xFF);
    write_cmos_sensor(0x0205, reg_gain & 0xFF);
    /* Global analog Gain for Short expo*/
    write_cmos_sensor(0x0216, (reg_gain>>8)& 0xFF);
    write_cmos_sensor(0x0217, reg_gain & 0xFF);
    write_cmos_sensor(0x0104, 0x00);


    return gain;
}    /*    set_gain  */
/*************************************************************************
* FUNCTION
*    set_dual_gain
*
* DESCRIPTION
*    This function is to set dual gain to sensor.
*
* PARAMETERS
*    iGain : sensor dual gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
#if 0
static kal_uint16 set_dual_gain(kal_uint16 gain1, kal_uint16 gain2)
{
	kal_uint16 reg_gain1, reg_gain2;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X    */
    /* [4:9] = M meams M X         */
    /* Total gain = M + N /16 X   */


	if (gain1 < BASEGAIN || gain1 > 8 * BASEGAIN) {
		LOG_INF("Error gain1 setting");

		if (gain1 < BASEGAIN)
			gain1 = BASEGAIN;
		else if (gain1 > 8 * BASEGAIN)
			gain1 = 8 * BASEGAIN;
	}

	if (gain2 < BASEGAIN || gain2 > 8 * BASEGAIN) {
		LOG_INF("Error gain2 setting");

		if (gain2 < BASEGAIN)
			gain2 = BASEGAIN;
		else if (gain2 > 8 * BASEGAIN)
			gain2 = 8 * BASEGAIN;
	}

	reg_gain1 = gain2reg(gain1);
	reg_gain2 = gain2reg(gain2);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain1;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain1 = %d , reg_gain1 = 0x%x, gain2 = %d , reg_gain2 = 0x%x\n ", gain1, reg_gain1, gain2, reg_gain2);

	write_cmos_sensor(0x0104, 0x01);
	/* Global analog Gain for Long expo*/
	write_cmos_sensor(0x0204, (reg_gain1>>8) & 0xFF);
	write_cmos_sensor(0x0205, reg_gain1 & 0xFF);
	/* Global analog Gain for Short expo*/
	write_cmos_sensor(0x0216, (reg_gain2>>8) & 0xFF);
	write_cmos_sensor(0x0217, reg_gain2 & 0xFF);
	write_cmos_sensor(0x0104, 0x00);

	return gain1;

}    /*    set_dual_gain  */
#endif
static void hdr_write_shutter(kal_uint16 le, kal_uint16 se)
{
    kal_uint16 realtime_fps = 0;
    kal_uint16 ratio;
    LOG_INF("le:0x%x, se:0x%x\n",le,se);
    spin_lock(&imgsensor_drv_lock);
    if (le > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = le + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);
    if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        write_cmos_sensor(0x0104, 0x01);
        write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
        write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
        write_cmos_sensor(0x0104, 0x00);
        }
    } else {
        write_cmos_sensor(0x0104, 0x01);
        write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
        write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
        write_cmos_sensor(0x0104, 0x00);
    }
    write_cmos_sensor(0x0104, 0x01);

    /* Long exposure */
    write_cmos_sensor(0x0202, (le >> 8) & 0xFF);
    write_cmos_sensor(0x0203, le  & 0xFF);
    /* Short exposure */
    write_cmos_sensor(0x0224, (se >> 8) & 0xFF);
    write_cmos_sensor(0x0225, se  & 0xFF);
    write_cmos_sensor(0x0104, 0x00);

    /* Ratio */
    if(se == 0)
        ratio = 2;
    else {
        ratio = (le+ (se >> 1)) / se;
        if(ratio > 16)
            ratio = 2;
    }

    LOG_INF("le:%d, se:%d, ratio:%d\n",le,se, ratio);
    write_cmos_sensor(0x0222,ratio);

}

#if 0
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
			write_cmos_sensor(0x0101, 0x00);
			write_cmos_sensor(0x3A27, 0x00);
			write_cmos_sensor(0x3A28, 0x00);
			write_cmos_sensor(0x3A29, 0x01);
			write_cmos_sensor(0x3A2A, 0x00);
			write_cmos_sensor(0x3A2B, 0x00);
			write_cmos_sensor(0x3A2C, 0x00);
			write_cmos_sensor(0x3A2D, 0x01);
			write_cmos_sensor(0x3A2E, 0x01);
			break;
        case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101, 0x01);
			write_cmos_sensor(0x3A27, 0x01);
			write_cmos_sensor(0x3A28, 0x01);
			write_cmos_sensor(0x3A29, 0x00);
			write_cmos_sensor(0x3A2A, 0x00);
			write_cmos_sensor(0x3A2B, 0x01);
			write_cmos_sensor(0x3A2C, 0x00);
			write_cmos_sensor(0x3A2D, 0x00);
			write_cmos_sensor(0x3A2E, 0x01);
            break;
        case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101, 0x02);
			write_cmos_sensor(0x3A27, 0x10);
			write_cmos_sensor(0x3A28, 0x10);
			write_cmos_sensor(0x3A29, 0x01);
			write_cmos_sensor(0x3A2A, 0x01);
			write_cmos_sensor(0x3A2B, 0x00);
			write_cmos_sensor(0x3A2C, 0x01);
			write_cmos_sensor(0x3A2D, 0x01);
			write_cmos_sensor(0x3A2E, 0x00);
            break;
        case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101, 0x03);
			write_cmos_sensor(0x3A27, 0x11);
			write_cmos_sensor(0x3A28, 0x11);
			write_cmos_sensor(0x3A29, 0x00);
			write_cmos_sensor(0x3A2A, 0x01);
			write_cmos_sensor(0x3A2B, 0x01);
			write_cmos_sensor(0x3A2C, 0x01);
			write_cmos_sensor(0x3A2D, 0x00);
			write_cmos_sensor(0x3A2E, 0x00);
            break;
        default:
            LOG_INF("Error image_mirror setting\n");
    }

}
#endif
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


#define MULTI_WRITE 1

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765 /* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 3

#endif
static kal_uint16 imx268_table_write_cmos_sensor(kal_uint16* para, kal_uint32 len)
{



   char puSendCmd[I2C_BUFFER_LEN];
   kal_uint32 tosend, IDX;
   kal_uint16 addr = 0, addr_last = 0, data;

   tosend = 0;
   IDX = 0;
//   kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor

   while(IDX < len)
   {
      addr = para[IDX];

       {
           puSendCmd[tosend++] = (char)(addr >> 8);
           puSendCmd[tosend++] = (char)(addr & 0xFF);
           data = para[IDX+1];
           puSendCmd[tosend++] = (char)(data & 0xFF);
           IDX += 2;
           addr_last = addr;

       }
#if MULTI_WRITE

	   if (tosend >= I2C_BUFFER_LEN || IDX == len || addr != addr_last)
       {
           iBurstWriteReg_multi(puSendCmd , tosend, imgsensor.i2c_write_id, 3, imgsensor_info.i2c_speed);
           tosend = 0;
       }
#else
	   iWriteRegI2C(puSendCmd, 3, imgsensor.i2c_write_id);
	   tosend = 0;

#endif
   }
   return 0;
}
#if 0
static kal_uint16 zvhdr_setting(void){

	LOG_INF("zhdr(mode:%d)\n",imgsensor.hdr_mode);

	if(imgsensor.hdr_mode == 9)
	{
		write_cmos_sensor(0x30b1,0x00);
	    write_cmos_sensor(0x30c6,0x00);
	    write_cmos_sensor(0x30b2,0x00);
	    write_cmos_sensor(0x30b3,0x00);
	    write_cmos_sensor(0x30c7,0x00);

		write_cmos_sensor(0x30b4,0x01);
	    write_cmos_sensor(0x30b5,0x01);
	    write_cmos_sensor(0x30b6,0x01);
	    write_cmos_sensor(0x30b7,0x01);
	    write_cmos_sensor(0x30b8,0x01);
	    write_cmos_sensor(0x30b9,0x01);
	    write_cmos_sensor(0x30ba,0x01);
	    write_cmos_sensor(0x30bb,0x01);
		write_cmos_sensor(0x30bc,0x01);
	}
	else
	{
		write_cmos_sensor(0x30b4,0x00);
	    write_cmos_sensor(0x30b5,0x00);
	    write_cmos_sensor(0x30b6,0x00);
	    write_cmos_sensor(0x30b7,0x00);
	    write_cmos_sensor(0x30b8,0x00);
	    write_cmos_sensor(0x30b9,0x00);
	    write_cmos_sensor(0x30ba,0x00);
	    write_cmos_sensor(0x30bb,0x00);
		write_cmos_sensor(0x30bc,0x00);
	}
	return 0;

}
#endif
kal_uint16 addr_data_pair_init_imx268[] =
{
/* External Clock Setting */
	0x0136, 0x18,
	0x0137, 0x00,
	0x4E21,	0x04,
	0x6B01,	0xB0,
	0x6B02,	0xED,
	0x6B05,	0x66,
	0x6B06,	0xFB,
	0x6B18,	0x3F,
	0x6B19,	0xFF,
	0x6541,	0x01,
};

static void sensor_init(void)
{
    LOG_INF("E\n");
	imx268_table_write_cmos_sensor(addr_data_pair_init_imx268, sizeof(addr_data_pair_init_imx268)/sizeof(kal_uint16));
}    /*    sensor_init  */


kal_uint16 addr_data_pair_preview_imx268[] =
{
	0x0100, 0x00,

	0x0112,	0x0A,
	0x0113,	0x0A,
	0x0114,	0x01,

	0x0301,	0x05,
	0x0303,	0x02,
	0x0305,	0x04,
	0x0306,	0x00,
	0x0307,	0x71,
	0x0309,	0x0A,
	0x030B,	0x01,
	0x030D,	0x00,
	0x030E,	0x00,
	0x030F,	0x00,
	0x0310,	0x00,
	0x0820,	0x05,
	0x0821,	0x4C,
	0x0822,	0x00,
	0x0823,	0x00,

	0x0342,	0x0F,
	0x0343,	0xA8,

	0x0340,	0x04,
	0x0341,	0x62,

	0x0344,	0x00,
	0x0345,	0x00,
	0x0346,	0x00,
	0x0347,	0x00,
	0x0348,	0x0F,
	0x0349,	0x1F,
	0x034A,	0x08,
	0x034B,	0x8F,

	0x0381,	0x01,
	0x0383,	0x01,
	0x0385,	0x01,
	0x0387,	0x01,
	0x0900,	0x01,
	0x0901,	0x12,

	0x0401,	0x01,
	0x0404,	0x00,
	0x0405,	0x20,
	0x0408,	0x00,
	0x0409,	0x02,
	0x040A,	0x00,
	0x040B,	0x00,
	0x040C,	0x0F,
	0x040D,	0x1A,
	0x040E,	0x04,
	0x040F,	0x48,
	0x300D,	0x00,

	0x034C,	0x07,
	0x034D,	0x8C,
	0x034E,	0x04,
	0x034F,	0x48,

	0x0202,	0x04,
	0x0203,	0x58,

	0x0204,	0x00,
	0x0205,	0x00,
	0x020E,	0x01,
	0x020F,	0x00,
	0x0210,	0x01,
	0x0211,	0x00,
	0x0212,	0x01,
	0x0213,	0x00,
	0x0214,	0x01,
	0x0215,	0x00,

	0x6568,	0x00,
	0x6B11,	0x8F,
	0x6B12,	0xAB,
	0x6B13,	0xAA,

	0x0220,	0x00,

	0x0B00,	0x00,
};

static void preview_setting(void)
{
	imx268_table_write_cmos_sensor(addr_data_pair_preview_imx268, sizeof(addr_data_pair_preview_imx268)/sizeof(kal_uint16));
	write_cmos_sensor(0x0100,0x01);

}    /*    preview_setting  */

static void custom1_setting(void)
{
	preview_setting();

}
static void custom2_setting(void)
{
	preview_setting();

}
static void custom3_setting(void)
{
	preview_setting();

}
static void custom4_setting(void)
{
	preview_setting();

}
static void custom5_setting(void)
{
	preview_setting();
}

kal_uint16 addr_data_pair_capture_imx268[] =
{
	0x0100, 0x00,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x01,

	0x0301, 0x05,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x00,
	0x0307, 0xE9,
	0x0309, 0x0A,
	0x030B, 0x01,
	0x030D, 0x00,
	0x030E, 0x00,
	0x030F, 0x00,
	0x0310, 0x00,
	0x0820, 0x0A,
	0x0821, 0xEC,
	0x0822, 0x00,
	0x0823, 0x00,

	0x0342, 0x10,
	0x0343, 0x68,

	0x0340, 0x08,
	0x0341, 0xAA,

	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x0F,
	0x0349, 0x1F,
	0x034A, 0x08,
	0x034B, 0x8F,

	0x0381, 0x01,
	0x0383, 0x01,
	0x0385, 0x01,
	0x0387, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,

	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x0F,
	0x040D, 0x20,
	0x040E, 0x08,
	0x040F, 0x90,
	0x300D, 0x00,

	0x034C, 0x0F,
	0x034D, 0x20,
	0x034E, 0x08,
	0x034F, 0x90,

	0x0202, 0x08,
	0x0203, 0xA0,

	0x0204, 0x00,
	0x0205, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0210, 0x01,
	0x0211, 0x00,
	0x0212, 0x01,
	0x0213, 0x00,
	0x0214, 0x01,
	0x0215, 0x00,

	0x6568, 0x00,
	0x6B11, 0x8F,
	0x6B12, 0xAB,
	0x6B13, 0xAA,

	0x0220, 0x00,

	0x0B00, 0x00,


};

static void capture_setting(kal_uint16 currefps)
{
	imx268_table_write_cmos_sensor(addr_data_pair_capture_imx268, sizeof(addr_data_pair_capture_imx268)/sizeof(kal_uint16));
	write_cmos_sensor(0x0100,0x01);
}


static void normal_video_setting(kal_uint16 currefps)
{
	preview_setting();
}


static void hs_video_setting(void)
{
	preview_setting();
}


static void slim_video_setting(void)
{
	preview_setting();
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("enable: %d\n", enable);

    if (enable) {
        write_cmos_sensor(0x0601, 0x02);
    } else {
        write_cmos_sensor(0x0601, 0x00);
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



    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.hdr_mode = 0;
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



	capture_setting(imgsensor.current_fps);/*Full mode*/



    return ERROR_NONE;
}    /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{


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

    return ERROR_NONE;
}    /*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{



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
    return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{



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

    return ERROR_NONE;
}    /*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{

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
}    /*    get_resolution    */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
                      MSDK_SENSOR_INFO_STRUCT *sensor_info,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("scenario_id = %d\n", scenario_id);


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
	sensor_info->PDAF_Support = 0; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
#if defined(IMX268_ZHDR)
	sensor_info->HDR_Support = 3; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
	/*0: no support, 1: G0,R0.B0, 2: G0,R0.B1, 3: G0,R1.B0, 4: G0,R1.B1*/
	/*5: G1,R0.B0, 6: G1,R0.B1, 7: G1,R1.B0, 8: G1,R1.B1*/
	sensor_info->ZHDR_Mode = 8;
#else
	sensor_info->HDR_Support = 2; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
#endif
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
}    /*    get_info  */

static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

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

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom2_setting();
    return ERROR_NONE;
}

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
    imgsensor.pclk = imgsensor_info.custom3.pclk;
    imgsensor.line_length = imgsensor_info.custom3.linelength;
    imgsensor.frame_length = imgsensor_info.custom3.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom3_setting();
    return ERROR_NONE;
}

static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
    imgsensor.pclk = imgsensor_info.custom4.pclk;
    imgsensor.line_length = imgsensor_info.custom4.linelength;
    imgsensor.frame_length = imgsensor_info.custom4.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom4_setting();
    return ERROR_NONE;
}   /*  Custom4   */


static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
    imgsensor.pclk = imgsensor_info.custom5.pclk;
    imgsensor.line_length = imgsensor_info.custom5.linelength;
    imgsensor.frame_length = imgsensor_info.custom5.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom5_setting();
    return ERROR_NONE;
}   /*  Custom5   */

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
            Custom1(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            Custom2(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            Custom3(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            Custom4(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            Custom5(image_window, sensor_config_data);
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
   // LOG_INF("scenario_id = %d\n", scenario_id);

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


static kal_uint32 imx268_awb_gain(SET_SENSOR_AWB_GAIN *pSetSensorAWB)
{
    UINT32 rgain_32, grgain_32, gbgain_32, bgain_32;
	  LOG_INF("imx268_awb_gain\n");

    grgain_32 = (pSetSensorAWB->ABS_GAIN_GR << 8) >> 9;
    rgain_32 = (pSetSensorAWB->ABS_GAIN_R << 8) >> 9;
    bgain_32 = (pSetSensorAWB->ABS_GAIN_B << 8) >> 9;
    gbgain_32 = (pSetSensorAWB->ABS_GAIN_GB << 8) >> 9;

    LOG_INF("[imx268_awb_gain] ABS_GAIN_GR:%d, grgain_32:%d\n", pSetSensorAWB->ABS_GAIN_GR, grgain_32);
    LOG_INF("[imx268_awb_gain] ABS_GAIN_R:%d, rgain_32:%d\n", pSetSensorAWB->ABS_GAIN_R, rgain_32);
    LOG_INF("[imx268_awb_gain] ABS_GAIN_B:%d, bgain_32:%d\n", pSetSensorAWB->ABS_GAIN_B, bgain_32);
    LOG_INF("[imx268_awb_gain] ABS_GAIN_GB:%d, gbgain_32:%d\n", pSetSensorAWB->ABS_GAIN_GB, gbgain_32);

    write_cmos_sensor(0x0b8e, (grgain_32 >> 8) & 0xFF);
    write_cmos_sensor(0x0b8f, grgain_32 & 0xFF);
    write_cmos_sensor(0x0b90, (rgain_32 >> 8) & 0xFF);
    write_cmos_sensor(0x0b91, rgain_32 & 0xFF);
    write_cmos_sensor(0x0b92, (bgain_32 >> 8) & 0xFF);
    write_cmos_sensor(0x0b93, bgain_32 & 0xFF);
    write_cmos_sensor(0x0b94, (gbgain_32 >> 8) & 0xFF);
    write_cmos_sensor(0x0b95, gbgain_32 & 0xFF);
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
//    SENSOR_VC_INFO_STRUCT *pvcinfo;
    SET_SENSOR_AWB_GAIN *pSetSensorAWB=(SET_SENSOR_AWB_GAIN *)feature_para;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    //LOG_INF("feature_id = %d\n", feature_id);
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
/*		case SENSOR_FEATURE_SET_DUAL_GAIN:
			set_dual_gain((UINT16) * feature_data, (UINT16) * (feature_data+1));
			break;
*/
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
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_GET_PDAF_DATA:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
			//read_imx268_DCC((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
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
		/*HDR CMD*/
		case SENSOR_FEATURE_SET_HDR:
            LOG_INF("hdr enable :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
			imgsensor.hdr_mode = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
		case SENSOR_FEATURE_SET_HDR_SHUTTER:
            LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1));
            hdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
            break;
#if 0
		case SENSOR_FEATURE_GET_VC_INFO:
            LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
            pvcinfo = (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
            switch (*feature_data_32) {
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(SENSOR_VC_INFO_STRUCT));
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(SENSOR_VC_INFO_STRUCT));
                break;
            case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            default:
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(SENSOR_VC_INFO_STRUCT));
                break;
            }
            break;
#endif
		case SENSOR_FEATURE_SET_AWB_GAIN:
            imx268_awb_gain(pSetSensorAWB);
            break;
		case SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY scenarioId:%llu\n", *feature_data);
	/*
	SENSOR_VHDR_MODE_NONE  = 0x0,
	SENSOR_VHDR_MODE_IVHDR = 0x01,
	SENSOR_VHDR_MODE_MVHDR = 0x02,
	SENSOR_VHDR_MODE_ZVHDR = 0x09

	*/
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x02;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x02;
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x0;
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x0;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x02;
					break;
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0x0;
					break;
			}
			break;

		/*END OF HDR CMD*/
		/*PDAF CMD*/
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
			//PDAF capacity enable or not, 2p8 only full size support PDAF
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
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
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
		case SENSOR_FEATURE_SET_PDAF:
			LOG_INF("PDAF mode :%d\n", *feature_data_16);
			imgsensor.pdaf_mode= *feature_data_16;
			break;

		case SENSOR_FEATURE_SET_PDFOCUS_AREA:
			LOG_INF("SENSOR_FEATURE_SET_IMX268_PDFOCUS_AREA not supported\n");/* Start Pos=%d, Size=%d\n",(UINT32)*feature_data,(UINT32)*(feature_data+1));*/
//			imx268_set_pd_focus_area(*feature_data,*(feature_data+1));
			break;
		/*End of PDAF*/
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

UINT32 IMX268_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    IMX230_MIPI_RAW_SensorInit    */
