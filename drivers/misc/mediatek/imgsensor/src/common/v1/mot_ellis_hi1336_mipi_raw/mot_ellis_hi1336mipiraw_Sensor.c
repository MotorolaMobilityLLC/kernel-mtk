/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "mot_ellis_hi1336mipiraw_Sensor.h"
#include "mot_ellis_hi1336mipiraw_Setting.h"
#define MAX_TIME1 655706 // 6004 * 0xfff7 / PCLK   us  (PCLK @ 600MHz)
#define MAX_TIME2 20000000  // 6004 * 0x1E8480 / PCLK us  (PCLK @ 600MHz)

#define PFX "mot_ellis_hi1336_camera_sensor"
static int m_mot_camera_debug = 0;
#define LOG_INF_D(format, args...)        do { if (m_mot_camera_debug   ) { pr_info(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_DEBUG_D(format, args...)        do { if (m_mot_camera_debug   ) { pr_debug(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)

#define LOG_INF(format, args...)    \
    pr_err(PFX "[%s] " format, __func__, ##args)

#define LOG_ERR(format, args...)    \
    pr_err(PFX "[%s] " format, __func__, ##args)

//PDAF
#define ENABLE_PDAF 1
//+bug449738 maliping.wt, add, 2019/06/25,video pdaf flow
//#define __ENABLE_VIDEO_CUSTOM_PDAF__    //wt.maliping add
//-bug449738 maliping.wt, add, 2019/06/25,video pdaf flow

#define e2prom 0

#define per_frame 1
extern bool read_mot_ellis_hi1336_eeprom( kal_uint16 addr, BYTE *data, kal_uint32 size);
extern bool read_eeprom( kal_uint16 addr, BYTE * data, kal_uint32 size);
#define HI1336_SERIAL_NUM_SIZE 16
#define MAIN_SERIAL_NUM_DATA_PATH "/data/vendor/camera_dump/serial_number_main.bin"

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
    .sensor_id = MOT_ELLIS_HI1336_SENSOR_ID,

    .checksum_value = 0x4f1b1d5e,       //0x6d01485c // Auto Test Mode
    .pre = {
        .pclk = 600000000,                //record different mode's pclk
        .linelength =  6004,             //record different mode's
        .framelength = 3316,             //record different mode's
        .startx = 0,                    //record different mode's
        .starty = 0,                    //record different mode's
        .grabwindow_width = 4208,         //record different mode's
        .grabwindow_height = 3120,        //record different mode's
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount
by different scenario */
        .mipi_data_lp2hs_settle_dc = 85,
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .cap = {
        .pclk = 600000000,
        .linelength = 6004,
        .framelength = 3316,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4208,
        .grabwindow_height = 3120,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
    },
    // need to setting
    .cap1 = {
        .pclk = 600000000,
        .linelength = 12000,
        .framelength = 3328,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4208,
        .grabwindow_height = 3120,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 150,

    },
    .normal_video = {
        .pclk = 600000000,
        .linelength = 6004,
        .framelength = 3316,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4208,
        .grabwindow_height = 3120,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,

    },
    .hs_video = {
        .pclk = 600000000,
        .linelength = 6004,
        .framelength = 832,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 640 ,
        .grabwindow_height = 480 ,
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        .max_framerate = 1200,

    },
    .slim_video = {
        .pclk = 600000000,
        .linelength = 6004,
        .framelength = 832,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 1280,
        .grabwindow_height = 720,
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        .max_framerate = 1200,
    },
    .custom1 = {//24.005
        .pclk = 600000000,
        .linelength = 6004,
        .framelength = 4163,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4208,
        .grabwindow_height = 3120,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 240,
    },
    .margin = 7,
    .min_shutter = 7,
    .max_frame_length = 0x30D400,      //32s
#if per_frame
    .ae_shut_delay_frame = 0,
    .ae_sensor_gain_delay_frame = 0,
    .ae_ispGain_delay_frame = 2,
#else
    .ae_shut_delay_frame = 0,
    .ae_sensor_gain_delay_frame = 1,
    .ae_ispGain_delay_frame = 2,
#endif

    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 6,      //support sensor mode num
    .frame_time_delay_frame = 1,//The delay frame of setting frame length
    .cap_delay_frame = 1,
    .pre_delay_frame = 1,
    .video_delay_frame = 1,
    .hs_video_delay_frame = 3,
    .slim_video_delay_frame = 3,
    .custom1_delay_frame = 2,
    .isp_driving_current = ISP_DRIVING_2MA,
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_NCSI2,
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
    .mclk = 24,
    .mipi_lane_num = SENSOR_MIPI_4_LANE,
    .i2c_addr_table = {0x40, 0xff},
    .i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
    .mirror = IMAGE_NORMAL,
    .sensor_mode = IMGSENSOR_MODE_INIT,
    .shutter = 0x0100,
    .gain = 0xe0,
    .dummy_pixel = 0,
    .dummy_line = 0,
//full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .current_fps = 300,
    .autoflicker_en = KAL_FALSE,
    .test_pattern = KAL_FALSE,
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
    .ihdr_en = 0,
    .i2c_write_id = 0x40,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
    { 4224, 3136,   0,   6, 4224, 3124,  4224, 3124,  8,  2, 4208, 3120, 0, 0, 4208, 3120},        // preview (4208 x 3120)
    { 4224, 3136,   0,   6, 4224, 3124,  4224, 3124,  8,  2, 4208, 3120, 0, 0, 4208, 3120},        // capture (4208 x 3120)
    { 4224, 3136,   0,   6, 4224, 3124,  4224, 3124,  8,  2, 4208, 3120, 0, 0, 4208, 3120},        // VIDEO (4208 x 3120)
    { 4224, 3136,   0, 116, 4224, 2904,   704,  484,  32, 2, 640, 480, 0, 0,  640,  480},        // hight speed video (640 x 480)
    { 4224, 3136,   0, 482, 4224, 2172,  1408,  724,  64, 2, 1280, 720, 0, 0, 1280,  720},       // slim video (1280 x 720)
    { 4224, 3136,   0,   6, 4224, 3124,  4224, 3124,  8,  2, 4208, 3120, 0, 0, 4208, 3120}        // custom1
};

#if ENABLE_PDAF
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{
	/* Preview mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x0838, 0x0618,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x0140, 0x0300,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??

	/* Capture mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x1070, 0x0C30,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  //0x01, 0x30, 0x0000, 0x0000,   // VC2 PDAF
	  0x01, 0x30, 0x0140, 0x0300,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??

	/* Video mode setting */
	{0x02, //VC_Num
	 0x0a, //VC_PixelNum
	 0x00, //ModeSelect    /* 0:auto 1:direct */
	 0x00, //EXPO_Ratio    /* 1/1, 1/2, 1/4, 1/8 */
	 0x00, //0DValue	   /* 0D Value */
	 0x00, //RG_STATSMODE  /* STATS divistion mode 0:16x16	1:8x8  2:4x4  3:1x1 */
	 0x00, 0x2B, 0x1070, 0x0C30,   // VC0 Maybe image data?
	 0x00, 0x00, 0x0000, 0x0000,   // VC1 MVHDR
	 //0x01, 0x30, 0x0000, 0x0000,	 // VC2 PDAF
	 0x01, 0x30, 0x0140, 0x0300,   // VC2 PDAF
	 0x00, 0x00, 0x0000, 0x0000},  // VC3 ??
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
    .i4OffsetX = 56,
    .i4OffsetY = 24,
    .i4PitchX = 32,
    .i4PitchY = 32,
    .i4PairNum =8,
    .i4SubBlkW =16,
    .i4SubBlkH =8,
    .i4PosL = {{61,28},{61,44},{69,32},{69,48},{77,28},{77,44},{85,32},{85,48}},
    .i4PosR = {{61,24},{61,40},{69,36},{69,52},{77,24},{77,40},{85,36},{85,52}},
    .i4BlockNumX = 128,
    .i4BlockNumY = 96,
    /*
0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR */
    .iMirrorFlip = 0,
};
//+bug449738 maliping.wt, add, 2019/06/25,video pdaf flow
#ifdef __ENABLE_VIDEO_CUSTOM_PDAF__
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_4208_2368 =
{
#if 0
    .i4OffsetX = 56,
    .i4OffsetY = 0,
    .i4PitchX = 32,
    .i4PitchY = 32,
    .i4PairNum =8,
    .i4SubBlkW =16,
    .i4SubBlkH =8,
    .i4PosL = {{61,4},{61,20},{69, 8},{69,24},{77,4},{77,20},{85, 8},{85,24}},
    .i4PosR = {{61,0},{61,16},{69,12},{69,28},{77,0},{77,16},{85,12},{85,28}},
    .i4BlockNumX = 128,
    .i4BlockNumY = 74,
    /*
0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR */
    .iMirrorFlip = 0,
    #else
/*
     .i4OffsetX = 56,
    .i4OffsetY = 0,
    .i4PitchX = 32,
    .i4PitchY = 32,
    .i4PairNum =8,
    .i4SubBlkW =16,
    .i4SubBlkH =8,
    .i4PosL = {{61,6},{61,22},{69, 10},{69,26},{77,6},{77,22},{85, 10},{85,26}},
    .i4PosR = {{61,2},{61,18},{69,14},{69,30},{77,2},{77,18},{85,14},{85,30}},
    .i4BlockNumX = 128,
    .i4BlockNumY = 74,
    .iMirrorFlip = 0,
*/
    .i4OffsetX = 56,
    .i4OffsetY = 24,
    .i4PitchX  = 32,
    .i4PitchY  = 32,
    .i4PairNum = 8,
    .i4SubBlkW = 16,
    .i4SubBlkH = 8,
    .i4PosL    ={{61,28},{61,44},{69,32},{69,48},{77,28},{77,44},{85,32},{85,48}},
    .i4PosR    ={{61,24},{61,40},{69,36},{69,52},{77,24},{77,40},{85,36},{85,52}},
    .i4BlockNumX = 128,
    .i4BlockNumY = 73,
    .iMirrorFlip = 0,
    .i4Crop = { {0, 0}, {0, 0}, {0, 374}, {0,0}, {0, 0},{0, 0}, {0, 0}, {0, 0},{0, 0}, {0, 0} },
    #endif
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_3264_2448 =
{
#if 0
    .i4OffsetX = 0,
    .i4OffsetY = 0,
    .i4PitchX = 16,
    .i4PitchY = 16,
    .i4PairNum =2,
    .i4SubBlkW =16,
    .i4SubBlkH =8,
    .i4PosL = {{13,0},{5,12}},
    .i4PosR = {{13,4},{5,8}},
    .i4BlockNumX = 204,
    .i4BlockNumY = 153,
    /*
0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR */
    .iMirrorFlip = 0,
#else
    .i4OffsetX = 56,
    .i4OffsetY = 24,
    .i4PitchX = 32,
    .i4PitchY = 32,
    .i4PairNum =8,
    .i4SubBlkW =16,
    .i4SubBlkH =8,
    .i4PosL = {{61,28},{61,44},{69,32},{69,48},{77,28},{77,44},{85,32},{85,48}},
    .i4PosR = {{61,24},{61,40},{69,36},{69,52},{77,24},{77,40},{85,36},{85,52}},
    .i4BlockNumX = 102,
    .i4BlockNumY = 76,
    .i4Crop = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {472,336},{0, 0}, {0, 0}, {0, 0}, {0, 0} },
    .iMirrorFlip = 0,
#endif
};
#endif
//-bug449738 maliping.wt, add, 2019/06/25,video pdaf flow

#endif



#define MULTI_WRITE 1

#if MULTI_WRITE
#define I2C_BUFFER_LEN 255

static kal_uint16 mot_ellis_hi1336_table_write_cmos_sensor(
                    kal_uint16 *para, kal_uint32 len)
{
    char puSendCmd[I2C_BUFFER_LEN];
    kal_uint32 tosend, IDX;
    kal_uint16 addr = 0, addr_last = 0, data;

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
        }

        if ((I2C_BUFFER_LEN - tosend) < 4 ||
            len == IDX ||
            addr != addr_last) {
            iBurstWriteReg_multi(puSendCmd, tosend,
                imgsensor.i2c_write_id,
                4, imgsensor_info.i2c_speed);

            tosend = 0;
        }
    }
    return 0;
}
#endif

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte = 0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

    iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

    return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8),
        (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};

    iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8),
        (char)(addr & 0xFF), (char)(para & 0xFF)};

    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
    LOG_INF("dummyline = %d, dummypixels = %d\n",
        imgsensor.dummy_line, imgsensor.dummy_pixel);
    write_cmos_sensor(0x020e, imgsensor.frame_length & 0xFFFF);
    write_cmos_sensor(0x0206, imgsensor.line_length/4);

}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor(0x0716) << 8) | read_cmos_sensor(0x0717));

}


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
    kal_uint32 frame_length = imgsensor.frame_length;

    frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
    spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ?
            frame_length : imgsensor.min_frame_length;
    imgsensor.dummy_line = imgsensor.frame_length -
        imgsensor.min_frame_length;

    if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
        imgsensor.frame_length = imgsensor_info.max_frame_length;
        imgsensor.dummy_line = imgsensor.frame_length -
            imgsensor.min_frame_length;
    }
    if (min_framelength_en)
        imgsensor.min_frame_length = imgsensor.frame_length;

    spin_unlock(&imgsensor_drv_lock);
    set_dummy();
}    /*    set_max_framerate  */

static void write_shutter(kal_uint32 shutter)
{
    kal_uint16 realtime_fps = 0;

    spin_lock(&imgsensor_drv_lock);

    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);

    LOG_INF_D("shutter = %d, imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n",
        shutter, imgsensor.frame_length, imgsensor.min_frame_length);


    shutter = (shutter < imgsensor_info.min_shutter) ?
        imgsensor_info.min_shutter : shutter;
    shutter = (shutter >
        (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
        (imgsensor_info.max_frame_length - imgsensor_info.margin) :
        shutter;
    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk * 10 /
            (imgsensor.line_length * imgsensor.frame_length);
        if (realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296, 0);
        else if (realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146, 0);
        else
            write_cmos_sensor(0x020e, imgsensor.frame_length);

    } else{
            write_cmos_sensor(0x020e, imgsensor.frame_length);
    }

    write_cmos_sensor_8(0x020D, (shutter & 0xFF0000) >> 16 );
    write_cmos_sensor(0x020A, (shutter & 0xFFFF));

    LOG_INF_D("frame_length = %d , shutter = %d \n", imgsensor.frame_length, shutter);
}

static void set_shutter_frame_length(kal_uint32 shutter, kal_uint16 frame_length){

    unsigned long flags;
    unsigned short realtime_fps = 0;
    static unsigned short fps_fine = 0;
    LOG_INF("set_shutter_frame_length");
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

    spin_lock(&imgsensor_drv_lock);
    /*Change frame time */
    if (frame_length > 1)
        imgsensor.frame_length = frame_length;
    if (shutter > imgsensor.frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;

    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);

    LOG_INF("shutter = %d, imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n",
        shutter, imgsensor.frame_length, imgsensor.min_frame_length);

    shutter = (shutter < imgsensor_info.min_shutter) ?
        imgsensor_info.min_shutter : shutter;
    shutter = (shutter >
        (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
        (imgsensor_info.max_frame_length - imgsensor_info.margin) :
        shutter;

    //shutter = (shutter >> 1) << 1;

    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk * 10 /
            (imgsensor.line_length * imgsensor.frame_length);
        if (realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296, 0);
        else if (realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146, 0);
        else
            write_cmos_sensor(0x020e, imgsensor.frame_length);

    } else{
            write_cmos_sensor(0x020e, imgsensor.frame_length & 0xFFFF);
    }
    if(imgsensor.frame_length == 7005){
        fps_fine++;
        if (fps_fine != 3){
            imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
            imgsensor.frame_length -= 1;
        }
        else{
            imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
            fps_fine = 0;
        }
    }
    else imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

    set_dummy();
    write_cmos_sensor_8(0x020D, (shutter & 0xFF0000) >> 16);
    write_cmos_sensor(0x020A, shutter);

    LOG_INF("frame_length = %d , shutter = %d \n", imgsensor.frame_length, shutter);

}

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
static void set_shutter(kal_uint32 shutter)
{
    unsigned long flags;

    LOG_INF_D("set_shutter");
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

    write_shutter(shutter);
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
static kal_uint16 gain2reg(kal_uint16 gain)
{
    kal_uint16 reg_gain = 0x0000;
    reg_gain = gain / 4 - 16;

    return (kal_uint16)reg_gain;

}


static kal_uint16 set_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X    */
    /* [4:9] = M meams M X         */
    /* Total gain = M + N /16 X   */

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
    LOG_INF_D("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

    write_cmos_sensor_8(0x0213,reg_gain);
    return gain;

}

#if 0
static void ihdr_write_shutter_gain(kal_uint16 le,
                kal_uint16 se, kal_uint16 gain)
{
    LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
    if (imgsensor.ihdr_en) {
        spin_lock(&imgsensor_drv_lock);
        if (le > imgsensor.min_frame_length - imgsensor_info.margin)
            imgsensor.frame_length = le + imgsensor_info.margin;
        else
            imgsensor.frame_length = imgsensor.min_frame_length;
        if (imgsensor.frame_length > imgsensor_info.max_frame_length)
            imgsensor.frame_length =
                imgsensor_info.max_frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (le < imgsensor_info.min_shutter)
            le = imgsensor_info.min_shutter;
        if (se < imgsensor_info.min_shutter)
            se = imgsensor_info.min_shutter;
        // Extend frame length first
        write_cmos_sensor(0x020e, imgsensor.frame_length);
        write_cmos_sensor(0x3502, (le << 4) & 0xFF);
        write_cmos_sensor(0x3501, (le >> 4) & 0xFF);
        write_cmos_sensor(0x3500, (le >> 12) & 0x0F);
        write_cmos_sensor(0x3508, (se << 4) & 0xFF);
        write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
        write_cmos_sensor(0x3506, (se >> 12) & 0x0F);
        set_gain(gain);
    }
}
#endif


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
    LOG_INF("image_mirror = %d", image_mirror);

    switch (image_mirror) {
    case IMAGE_NORMAL:
        write_cmos_sensor(0x0202, 0x0000);
        break;
    case IMAGE_H_MIRROR:
        write_cmos_sensor(0x0202, 0x0100);

        break;
    case IMAGE_V_MIRROR:
        write_cmos_sensor(0x0202, 0x0200);

        break;
    case IMAGE_HV_MIRROR:
        write_cmos_sensor(0x0202, 0x0300);

        break;
    default:
        LOG_INF("Error image_mirror setting");
        break;
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



static void sensor_init(void)
{
#if MULTI_WRITE
    LOG_INF("sensor_init multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_init_mot_ellis_hi1336,
        sizeof(addr_data_pair_init_mot_ellis_hi1336) /
        sizeof(kal_uint16));
    LOG_INF("sensor_init multi write end\n");
#else
    LOG_INF("sensor_init normal write\n");
    write_cmos_sensor(0x0b00, 0x0000);
    write_cmos_sensor(0x2000, 0x0021);
    write_cmos_sensor(0x2002, 0x04a5);
    write_cmos_sensor(0x2004, 0xb124);
    write_cmos_sensor(0x2006, 0xc09c);
    write_cmos_sensor(0x2008, 0x0064);
    write_cmos_sensor(0x200a, 0x088e);
    write_cmos_sensor(0x200c, 0x01c2);
    write_cmos_sensor(0x200e, 0x00b4);
    write_cmos_sensor(0x2010, 0x4020);
    write_cmos_sensor(0x2012, 0x4292);
    write_cmos_sensor(0x2014, 0xf00a);
    write_cmos_sensor(0x2016, 0x0310);
    write_cmos_sensor(0x2018, 0x12b0);
    write_cmos_sensor(0x201a, 0xc3f2);
    write_cmos_sensor(0x201c, 0x425f);
    write_cmos_sensor(0x201e, 0x0282);
    write_cmos_sensor(0x2020, 0xf35f);
    write_cmos_sensor(0x2022, 0xf37f);
    write_cmos_sensor(0x2024, 0x5f0f);
    write_cmos_sensor(0x2026, 0x4f92);
    write_cmos_sensor(0x2028, 0xf692);
    write_cmos_sensor(0x202a, 0x0402);
    write_cmos_sensor(0x202c, 0x93c2);
    write_cmos_sensor(0x202e, 0x82cc);
    write_cmos_sensor(0x2030, 0x2403);
    write_cmos_sensor(0x2032, 0xf0f2);
    write_cmos_sensor(0x2034, 0xffe7);
    write_cmos_sensor(0x2036, 0x0254);
    write_cmos_sensor(0x2038, 0x4130);
    write_cmos_sensor(0x203a, 0x120b);
    write_cmos_sensor(0x203c, 0x120a);
    write_cmos_sensor(0x203e, 0x1209);
    write_cmos_sensor(0x2040, 0x425f);
    write_cmos_sensor(0x2042, 0x0600);
    write_cmos_sensor(0x2044, 0xf35f);
    write_cmos_sensor(0x2046, 0x4f4b);
    write_cmos_sensor(0x2048, 0x12b0);
    write_cmos_sensor(0x204a, 0xc6ae);
    write_cmos_sensor(0x204c, 0x403d);
    write_cmos_sensor(0x204e, 0x0100);
    write_cmos_sensor(0x2050, 0x403e);
    write_cmos_sensor(0x2052, 0x2bfc);
    write_cmos_sensor(0x2054, 0x403f);
    write_cmos_sensor(0x2056, 0x8020);
    write_cmos_sensor(0x2058, 0x12b0);
    write_cmos_sensor(0x205a, 0xc476);
    write_cmos_sensor(0x205c, 0x930b);
    write_cmos_sensor(0x205e, 0x2009);
    write_cmos_sensor(0x2060, 0x93c2);
    write_cmos_sensor(0x2062, 0x0c0a);
    write_cmos_sensor(0x2064, 0x2403);
    write_cmos_sensor(0x2066, 0x43d2);
    write_cmos_sensor(0x2068, 0x0e1f);
    write_cmos_sensor(0x206a, 0x3c13);
    write_cmos_sensor(0x206c, 0x43c2);
    write_cmos_sensor(0x206e, 0x0e1f);
    write_cmos_sensor(0x2070, 0x3c10);
    write_cmos_sensor(0x2072, 0x4039);
    write_cmos_sensor(0x2074, 0x0e08);
    write_cmos_sensor(0x2076, 0x492a);
    write_cmos_sensor(0x2078, 0x421c);
    write_cmos_sensor(0x207a, 0xf010);
    write_cmos_sensor(0x207c, 0x430b);
    write_cmos_sensor(0x207e, 0x430d);
    write_cmos_sensor(0x2080, 0x12b0);
    write_cmos_sensor(0x2082, 0xdfb6);
    write_cmos_sensor(0x2084, 0x403d);
    write_cmos_sensor(0x2086, 0x000e);
    write_cmos_sensor(0x2088, 0x12b0);
    write_cmos_sensor(0x208a, 0xc62c);
    write_cmos_sensor(0x208c, 0x4e89);
    write_cmos_sensor(0x208e, 0x0000);
    write_cmos_sensor(0x2090, 0x3fe7);
    write_cmos_sensor(0x2092, 0x4139);
    write_cmos_sensor(0x2094, 0x413a);
    write_cmos_sensor(0x2096, 0x413b);
    write_cmos_sensor(0x2098, 0x4130);
    write_cmos_sensor(0x209a, 0xb0b2);
    write_cmos_sensor(0x209c, 0x0020);
    write_cmos_sensor(0x209e, 0xf002);
    write_cmos_sensor(0x20a0, 0x2429);
    write_cmos_sensor(0x20a2, 0x421e);
    write_cmos_sensor(0x20a4, 0x0256);
    write_cmos_sensor(0x20a6, 0x532e);
    write_cmos_sensor(0x20a8, 0x421f);
    write_cmos_sensor(0x20aa, 0xf008);
    write_cmos_sensor(0x20ac, 0x9e0f);
    write_cmos_sensor(0x20ae, 0x2c01);
    write_cmos_sensor(0x20b0, 0x4e0f);
    write_cmos_sensor(0x20b2, 0x4f0c);
    write_cmos_sensor(0x20b4, 0x430d);
    write_cmos_sensor(0x20b6, 0x421e);
    write_cmos_sensor(0x20b8, 0x7300);
    write_cmos_sensor(0x20ba, 0x421f);
    write_cmos_sensor(0x20bc, 0x7302);
    write_cmos_sensor(0x20be, 0x5c0e);
    write_cmos_sensor(0x20c0, 0x6d0f);
    write_cmos_sensor(0x20c2, 0x821e);
    write_cmos_sensor(0x20c4, 0x830c);
    write_cmos_sensor(0x20c6, 0x721f);
    write_cmos_sensor(0x20c8, 0x830e);
    write_cmos_sensor(0x20ca, 0x2c0d);
    write_cmos_sensor(0x20cc, 0x0900);
    write_cmos_sensor(0x20ce, 0x7312);
    write_cmos_sensor(0x20d0, 0x421e);
    write_cmos_sensor(0x20d2, 0x7300);
    write_cmos_sensor(0x20d4, 0x421f);
    write_cmos_sensor(0x20d6, 0x7302);
    write_cmos_sensor(0x20d8, 0x5c0e);
    write_cmos_sensor(0x20da, 0x6d0f);
    write_cmos_sensor(0x20dc, 0x821e);
    write_cmos_sensor(0x20de, 0x830c);
    write_cmos_sensor(0x20e0, 0x721f);
    write_cmos_sensor(0x20e2, 0x830e);
    write_cmos_sensor(0x20e4, 0x2bf3);
    write_cmos_sensor(0x20e6, 0x4292);
    write_cmos_sensor(0x20e8, 0x8248);
    write_cmos_sensor(0x20ea, 0x0a08);
    write_cmos_sensor(0x20ec, 0x0c10);
    write_cmos_sensor(0x20ee, 0x4292);
    write_cmos_sensor(0x20f0, 0x8252);
    write_cmos_sensor(0x20f2, 0x0a12);
    write_cmos_sensor(0x20f4, 0x12b0);
    write_cmos_sensor(0x20f6, 0xdc9c);
    write_cmos_sensor(0x20f8, 0xd0f2);
    write_cmos_sensor(0x20fa, 0x0018);
    write_cmos_sensor(0x20fc, 0x0254);
    write_cmos_sensor(0x20fe, 0x4130);
    write_cmos_sensor(0x2100, 0x120b);
    write_cmos_sensor(0x2102, 0x12b0);
    write_cmos_sensor(0x2104, 0xcfc8);
    write_cmos_sensor(0x2106, 0x4f4b);
    write_cmos_sensor(0x2108, 0x12b0);
    write_cmos_sensor(0x210a, 0xcfc8);
    write_cmos_sensor(0x210c, 0xf37f);
    write_cmos_sensor(0x210e, 0x108f);
    write_cmos_sensor(0x2110, 0xdb0f);
    write_cmos_sensor(0x2112, 0x413b);
    write_cmos_sensor(0x2114, 0x4130);
    write_cmos_sensor(0x2116, 0x120b);
    write_cmos_sensor(0x2118, 0x12b0);
    write_cmos_sensor(0x211a, 0xcfc8);
    write_cmos_sensor(0x211c, 0x4f4b);
    write_cmos_sensor(0x211e, 0x108b);
    write_cmos_sensor(0x2120, 0x12b0);
    write_cmos_sensor(0x2122, 0xcfc8);
    write_cmos_sensor(0x2124, 0xf37f);
    write_cmos_sensor(0x2126, 0xdb0f);
    write_cmos_sensor(0x2128, 0x413b);
    write_cmos_sensor(0x212a, 0x4130);
    write_cmos_sensor(0x212c, 0x120b);
    write_cmos_sensor(0x212e, 0x120a);
    write_cmos_sensor(0x2130, 0x1209);
    write_cmos_sensor(0x2132, 0x1208);
    write_cmos_sensor(0x2134, 0x4338);
    write_cmos_sensor(0x2136, 0x40b2);
    write_cmos_sensor(0x2138, 0x17fb);
    write_cmos_sensor(0x213a, 0x83be);
    write_cmos_sensor(0x213c, 0x12b0);
    write_cmos_sensor(0x213e, 0xcfc8);
    write_cmos_sensor(0x2140, 0xf37f);
    write_cmos_sensor(0x2142, 0x903f);
    write_cmos_sensor(0x2144, 0x0013);
    write_cmos_sensor(0x2146, 0x244c);
    write_cmos_sensor(0x2148, 0x12b0);
    write_cmos_sensor(0x214a, 0xf100);
    write_cmos_sensor(0x214c, 0x4f82);
    write_cmos_sensor(0x214e, 0x82a4);
    write_cmos_sensor(0x2150, 0xb3e2);
    write_cmos_sensor(0x2152, 0x0282);
    write_cmos_sensor(0x2154, 0x240a);
    write_cmos_sensor(0x2156, 0x5f0f);
    write_cmos_sensor(0x2158, 0x5f0f);
    write_cmos_sensor(0x215a, 0x521f);
    write_cmos_sensor(0x215c, 0x83be);
    write_cmos_sensor(0x215e, 0x533f);
    write_cmos_sensor(0x2160, 0x4f82);
    write_cmos_sensor(0x2162, 0x83be);
    write_cmos_sensor(0x2164, 0x43f2);
    write_cmos_sensor(0x2166, 0x83c0);
    write_cmos_sensor(0x2168, 0x4308);
    write_cmos_sensor(0x216a, 0x4309);
    write_cmos_sensor(0x216c, 0x9219);
    write_cmos_sensor(0x216e, 0x82a4);
    write_cmos_sensor(0x2170, 0x2c34);
    write_cmos_sensor(0x2172, 0xb3e2);
    write_cmos_sensor(0x2174, 0x0282);
    write_cmos_sensor(0x2176, 0x242a);
    write_cmos_sensor(0x2178, 0x12b0);
    write_cmos_sensor(0x217a, 0xf116);
    write_cmos_sensor(0x217c, 0x4f0b);
    write_cmos_sensor(0x217e, 0x12b0);
    write_cmos_sensor(0x2180, 0xf116);
    write_cmos_sensor(0x2182, 0x4f0a);
    write_cmos_sensor(0x2184, 0x490f);
    write_cmos_sensor(0x2186, 0x5f0f);
    write_cmos_sensor(0x2188, 0x5f0f);
    write_cmos_sensor(0x218a, 0x4b8f);
    write_cmos_sensor(0x218c, 0x2bfc);
    write_cmos_sensor(0x218e, 0x4a8f);
    write_cmos_sensor(0x2190, 0x2bfe);
    write_cmos_sensor(0x2192, 0x5319);
    write_cmos_sensor(0x2194, 0x9039);
    write_cmos_sensor(0x2196, 0x0100);
    write_cmos_sensor(0x2198, 0x2be9);
    write_cmos_sensor(0x219a, 0x43d2);
    write_cmos_sensor(0x219c, 0x83c0);
    write_cmos_sensor(0x219e, 0x421e);
    write_cmos_sensor(0x21a0, 0x82a4);
    write_cmos_sensor(0x21a2, 0x903e);
    write_cmos_sensor(0x21a4, 0x0080);
    write_cmos_sensor(0x21a6, 0x2810);
    write_cmos_sensor(0x21a8, 0x421f);
    write_cmos_sensor(0x21aa, 0x2d28);
    write_cmos_sensor(0x21ac, 0x503f);
    write_cmos_sensor(0x21ae, 0x0014);
    write_cmos_sensor(0x21b0, 0x4f82);
    write_cmos_sensor(0x21b2, 0x82a0);
    write_cmos_sensor(0x21b4, 0x903e);
    write_cmos_sensor(0x21b6, 0x00c0);
    write_cmos_sensor(0x21b8, 0x2805);
    write_cmos_sensor(0x21ba, 0x421f);
    write_cmos_sensor(0x21bc, 0x2e28);
    write_cmos_sensor(0x21be, 0x503f);
    write_cmos_sensor(0x21c0, 0x0014);
    write_cmos_sensor(0x21c2, 0x3c12);
    write_cmos_sensor(0x21c4, 0x480f);
    write_cmos_sensor(0x21c6, 0x3c10);
    write_cmos_sensor(0x21c8, 0x480f);
    write_cmos_sensor(0x21ca, 0x3ff2);
    write_cmos_sensor(0x21cc, 0x12b0);
    write_cmos_sensor(0x21ce, 0xf100);
    write_cmos_sensor(0x21d0, 0x4f0a);
    write_cmos_sensor(0x21d2, 0x12b0);
    write_cmos_sensor(0x21d4, 0xf100);
    write_cmos_sensor(0x21d6, 0x4f0b);
    write_cmos_sensor(0x21d8, 0x3fd5);
    write_cmos_sensor(0x21da, 0x430a);
    write_cmos_sensor(0x21dc, 0x430b);
    write_cmos_sensor(0x21de, 0x3fd2);
    write_cmos_sensor(0x21e0, 0x40b2);
    write_cmos_sensor(0x21e2, 0x1bfe);
    write_cmos_sensor(0x21e4, 0x83be);
    write_cmos_sensor(0x21e6, 0x3fb0);
    write_cmos_sensor(0x21e8, 0x4f82);
    write_cmos_sensor(0x21ea, 0x82a2);
    write_cmos_sensor(0x21ec, 0x4138);
    write_cmos_sensor(0x21ee, 0x4139);
    write_cmos_sensor(0x21f0, 0x413a);
    write_cmos_sensor(0x21f2, 0x413b);
    write_cmos_sensor(0x21f4, 0x4130);
    write_cmos_sensor(0x21f6, 0x43d2);
    write_cmos_sensor(0x21f8, 0x0300);
    write_cmos_sensor(0x21fa, 0x12b0);
    write_cmos_sensor(0x21fc, 0xcf6a);
    write_cmos_sensor(0x21fe, 0x12b0);
    write_cmos_sensor(0x2200, 0xcf0a);
    write_cmos_sensor(0x2202, 0xb3d2);
    write_cmos_sensor(0x2204, 0x0267);
    write_cmos_sensor(0x2206, 0x2404);
    write_cmos_sensor(0x2208, 0x12b0);
    write_cmos_sensor(0x220a, 0xf12c);
    write_cmos_sensor(0x220c, 0xc3d2);
    write_cmos_sensor(0x220e, 0x0267);
    write_cmos_sensor(0x2210, 0x12b0);
    write_cmos_sensor(0x2212, 0xd0d4);
    write_cmos_sensor(0x2214, 0x0261);
    write_cmos_sensor(0x2216, 0x0000);
    write_cmos_sensor(0x2218, 0x43c2);
    write_cmos_sensor(0x221a, 0x0300);
    write_cmos_sensor(0x221c, 0x4392);
    write_cmos_sensor(0x221e, 0x732a);
    write_cmos_sensor(0x2220, 0x4130);
    write_cmos_sensor(0x2222, 0x90f2);
    write_cmos_sensor(0x2224, 0x0010);
    write_cmos_sensor(0x2226, 0x0260);
    write_cmos_sensor(0x2228, 0x2002);
    write_cmos_sensor(0x222a, 0x12b0);
    write_cmos_sensor(0x222c, 0xd4aa);
    write_cmos_sensor(0x222e, 0x12b0);
    write_cmos_sensor(0x2230, 0xd5fa);
    write_cmos_sensor(0x2232, 0x4392);
    write_cmos_sensor(0x2234, 0x732a);
    write_cmos_sensor(0x2236, 0x12b0);
    write_cmos_sensor(0x2238, 0xf1f6);
    write_cmos_sensor(0x223a, 0x4130);
    write_cmos_sensor(0x223c, 0x120b);
    write_cmos_sensor(0x223e, 0x120a);
    write_cmos_sensor(0x2240, 0x1209);
    write_cmos_sensor(0x2242, 0x1208);
    write_cmos_sensor(0x2244, 0x1207);
    write_cmos_sensor(0x2246, 0x1206);
    write_cmos_sensor(0x2248, 0x1205);
    write_cmos_sensor(0x224a, 0x1204);
    write_cmos_sensor(0x224c, 0x8031);
    write_cmos_sensor(0x224e, 0x000a);
    write_cmos_sensor(0x2250, 0x4291);
    write_cmos_sensor(0x2252, 0x82d8);
    write_cmos_sensor(0x2254, 0x0004);
    write_cmos_sensor(0x2256, 0x411f);
    write_cmos_sensor(0x2258, 0x0004);
    write_cmos_sensor(0x225a, 0x4fa1);
    write_cmos_sensor(0x225c, 0x0006);
    write_cmos_sensor(0x225e, 0x4257);
    write_cmos_sensor(0x2260, 0x82e5);
    write_cmos_sensor(0x2262, 0x4708);
    write_cmos_sensor(0x2264, 0xd038);
    write_cmos_sensor(0x2266, 0xff00);
    write_cmos_sensor(0x2268, 0x4349);
    write_cmos_sensor(0x226a, 0x4346);
    write_cmos_sensor(0x226c, 0x90b2);
    write_cmos_sensor(0x226e, 0x07d1);
    write_cmos_sensor(0x2270, 0x0b94);
    write_cmos_sensor(0x2272, 0x2806);
    write_cmos_sensor(0x2274, 0x40b2);
    write_cmos_sensor(0x2276, 0x0246);
    write_cmos_sensor(0x2278, 0x0228);
    write_cmos_sensor(0x227a, 0x40b2);
    write_cmos_sensor(0x227c, 0x09fb);
    write_cmos_sensor(0x227e, 0x0232);
    write_cmos_sensor(0x2280, 0x4291);
    write_cmos_sensor(0x2282, 0x0422);
    write_cmos_sensor(0x2284, 0x0000);
    write_cmos_sensor(0x2286, 0x421f);
    write_cmos_sensor(0x2288, 0x0424);
    write_cmos_sensor(0x228a, 0x812f);
    write_cmos_sensor(0x228c, 0x4f81);
    write_cmos_sensor(0x228e, 0x0002);
    write_cmos_sensor(0x2290, 0x4291);
    write_cmos_sensor(0x2292, 0x8248);
    write_cmos_sensor(0x2294, 0x0008);
    write_cmos_sensor(0x2296, 0x4214);
    write_cmos_sensor(0x2298, 0x0310);
    write_cmos_sensor(0x229a, 0x421a);
    write_cmos_sensor(0x229c, 0x82a0);
    write_cmos_sensor(0x229e, 0xf80a);
    write_cmos_sensor(0x22a0, 0x421b);
    write_cmos_sensor(0x22a2, 0x82a2);
    write_cmos_sensor(0x22a4, 0xf80b);
    write_cmos_sensor(0x22a6, 0x4382);
    write_cmos_sensor(0x22a8, 0x7334);
    write_cmos_sensor(0x22aa, 0x0f00);
    write_cmos_sensor(0x22ac, 0x7304);
    write_cmos_sensor(0x22ae, 0x4192);
    write_cmos_sensor(0x22b0, 0x0008);
    write_cmos_sensor(0x22b2, 0x0a08);
    write_cmos_sensor(0x22b4, 0x4382);
    write_cmos_sensor(0x22b6, 0x040c);
    write_cmos_sensor(0x22b8, 0x4305);
    write_cmos_sensor(0x22ba, 0x9382);
    write_cmos_sensor(0x22bc, 0x7112);
    write_cmos_sensor(0x22be, 0x2001);
    write_cmos_sensor(0x22c0, 0x4315);
    write_cmos_sensor(0x22c2, 0x421e);
    write_cmos_sensor(0x22c4, 0x7100);
    write_cmos_sensor(0x22c6, 0xb2f2);
    write_cmos_sensor(0x22c8, 0x0261);
    write_cmos_sensor(0x22ca, 0x2406);
    write_cmos_sensor(0x22cc, 0xb3d2);
    write_cmos_sensor(0x22ce, 0x0b02);
    write_cmos_sensor(0x22d0, 0x2403);
    write_cmos_sensor(0x22d2, 0x42d2);
    write_cmos_sensor(0x22d4, 0x0809);
    write_cmos_sensor(0x22d6, 0x0b00);
    write_cmos_sensor(0x22d8, 0x40b2);
    write_cmos_sensor(0x22da, 0x00b6);
    write_cmos_sensor(0x22dc, 0x7334);
    write_cmos_sensor(0x22de, 0x0f00);
    write_cmos_sensor(0x22e0, 0x7304);
    write_cmos_sensor(0x22e2, 0x4482);
    write_cmos_sensor(0x22e4, 0x0a08);
    write_cmos_sensor(0x22e6, 0xb2e2);
    write_cmos_sensor(0x22e8, 0x0b05);
    write_cmos_sensor(0x22ea, 0x2404);
    write_cmos_sensor(0x22ec, 0x4392);
    write_cmos_sensor(0x22ee, 0x7a0e);
    write_cmos_sensor(0x22f0, 0x0800);
    write_cmos_sensor(0x22f2, 0x7a10);
    write_cmos_sensor(0x22f4, 0xf80e);
    write_cmos_sensor(0x22f6, 0x93c2);
    write_cmos_sensor(0x22f8, 0x82de);
    write_cmos_sensor(0x22fa, 0x2468);
    write_cmos_sensor(0x22fc, 0x9e0a);
    write_cmos_sensor(0x22fe, 0x2803);
    write_cmos_sensor(0x2300, 0x9349);
    write_cmos_sensor(0x2302, 0x2001);
    write_cmos_sensor(0x2304, 0x4359);
    write_cmos_sensor(0x2306, 0x9e0b);
    write_cmos_sensor(0x2308, 0x2802);
    write_cmos_sensor(0x230a, 0x9369);
    write_cmos_sensor(0x230c, 0x245c);
    write_cmos_sensor(0x230e, 0x421f);
    write_cmos_sensor(0x2310, 0x731a);
    write_cmos_sensor(0x2312, 0xc312);
    write_cmos_sensor(0x2314, 0x100f);
    write_cmos_sensor(0x2316, 0x4f82);
    write_cmos_sensor(0x2318, 0x7334);
    write_cmos_sensor(0x231a, 0x0f00);
    write_cmos_sensor(0x231c, 0x7304);
    write_cmos_sensor(0x231e, 0x4192);
    write_cmos_sensor(0x2320, 0x0008);
    write_cmos_sensor(0x2322, 0x0a08);
    write_cmos_sensor(0x2324, 0x421e);
    write_cmos_sensor(0x2326, 0x7100);
    write_cmos_sensor(0x2328, 0x812e);
    write_cmos_sensor(0x232a, 0x425c);
    write_cmos_sensor(0x232c, 0x0419);
    write_cmos_sensor(0x232e, 0x537c);
    write_cmos_sensor(0x2330, 0xfe4c);
    write_cmos_sensor(0x2332, 0x9305);
    write_cmos_sensor(0x2334, 0x2003);
    write_cmos_sensor(0x2336, 0x40b2);
    write_cmos_sensor(0x2338, 0x0c78);
    write_cmos_sensor(0x233a, 0x7100);
    write_cmos_sensor(0x233c, 0x421f);
    write_cmos_sensor(0x233e, 0x731a);
    write_cmos_sensor(0x2340, 0xc312);
    write_cmos_sensor(0x2342, 0x100f);
    write_cmos_sensor(0x2344, 0x503f);
    write_cmos_sensor(0x2346, 0x00b6);
    write_cmos_sensor(0x2348, 0x4f82);
    write_cmos_sensor(0x234a, 0x7334);
    write_cmos_sensor(0x234c, 0x0f00);
    write_cmos_sensor(0x234e, 0x7304);
    write_cmos_sensor(0x2350, 0x4482);
    write_cmos_sensor(0x2352, 0x0a08);
    write_cmos_sensor(0x2354, 0x9e81);
    write_cmos_sensor(0x2356, 0x0002);
    write_cmos_sensor(0x2358, 0x2814);
    write_cmos_sensor(0x235a, 0xf74c);
    write_cmos_sensor(0x235c, 0x434d);
    write_cmos_sensor(0x235e, 0x411f);
    write_cmos_sensor(0x2360, 0x0004);
    write_cmos_sensor(0x2362, 0x4f1e);
    write_cmos_sensor(0x2364, 0x0002);
    write_cmos_sensor(0x2366, 0x9381);
    write_cmos_sensor(0x2368, 0x0006);
    write_cmos_sensor(0x236a, 0x240b);
    write_cmos_sensor(0x236c, 0x4e6f);
    write_cmos_sensor(0x236e, 0xf74f);
    write_cmos_sensor(0x2370, 0x9c4f);
    write_cmos_sensor(0x2372, 0x2423);
    write_cmos_sensor(0x2374, 0x535d);
    write_cmos_sensor(0x2376, 0x503e);
    write_cmos_sensor(0x2378, 0x0006);
    write_cmos_sensor(0x237a, 0x4d4f);
    write_cmos_sensor(0x237c, 0x911f);
    write_cmos_sensor(0x237e, 0x0006);
    write_cmos_sensor(0x2380, 0x2bf5);
    write_cmos_sensor(0x2382, 0x9359);
    write_cmos_sensor(0x2384, 0x2403);
    write_cmos_sensor(0x2386, 0x9079);
    write_cmos_sensor(0x2388, 0x0003);
    write_cmos_sensor(0x238a, 0x2028);
    write_cmos_sensor(0x238c, 0x434d);
    write_cmos_sensor(0x238e, 0x464f);
    write_cmos_sensor(0x2390, 0x5f0f);
    write_cmos_sensor(0x2392, 0x5f0f);
    write_cmos_sensor(0x2394, 0x4f9f);
    write_cmos_sensor(0x2396, 0x2dfc);
    write_cmos_sensor(0x2398, 0x8020);
    write_cmos_sensor(0x239a, 0x4f9f);
    write_cmos_sensor(0x239c, 0x2dfe);
    write_cmos_sensor(0x239e, 0x8022);
    write_cmos_sensor(0x23a0, 0x5356);
    write_cmos_sensor(0x23a2, 0x9076);
    write_cmos_sensor(0x23a4, 0x0040);
    write_cmos_sensor(0x23a6, 0x2407);
    write_cmos_sensor(0x23a8, 0x9076);
    write_cmos_sensor(0x23aa, 0xff80);
    write_cmos_sensor(0x23ac, 0x2404);
    write_cmos_sensor(0x23ae, 0x535d);
    write_cmos_sensor(0x23b0, 0x926d);
    write_cmos_sensor(0x23b2, 0x2bed);
    write_cmos_sensor(0x23b4, 0x3c13);
    write_cmos_sensor(0x23b6, 0x5359);
    write_cmos_sensor(0x23b8, 0x3c11);
    write_cmos_sensor(0x23ba, 0x4ea2);
    write_cmos_sensor(0x23bc, 0x040c);
    write_cmos_sensor(0x23be, 0x4e92);
    write_cmos_sensor(0x23c0, 0x0002);
    write_cmos_sensor(0x23c2, 0x040e);
    write_cmos_sensor(0x23c4, 0x3fde);
    write_cmos_sensor(0x23c6, 0x4079);
    write_cmos_sensor(0x23c8, 0x0003);
    write_cmos_sensor(0x23ca, 0x3fa1);
    write_cmos_sensor(0x23cc, 0x9a0e);
    write_cmos_sensor(0x23ce, 0x2803);
    write_cmos_sensor(0x23d0, 0x9349);
    write_cmos_sensor(0x23d2, 0x2001);
    write_cmos_sensor(0x23d4, 0x4359);
    write_cmos_sensor(0x23d6, 0x9b0e);
    write_cmos_sensor(0x23d8, 0x2b9a);
    write_cmos_sensor(0x23da, 0x3f97);
    write_cmos_sensor(0x23dc, 0x9305);
    write_cmos_sensor(0x23de, 0x2363);
    write_cmos_sensor(0x23e0, 0x5031);
    write_cmos_sensor(0x23e2, 0x000a);
    write_cmos_sensor(0x23e4, 0x4134);
    write_cmos_sensor(0x23e6, 0x4135);
    write_cmos_sensor(0x23e8, 0x4136);
    write_cmos_sensor(0x23ea, 0x4137);
    write_cmos_sensor(0x23ec, 0x4138);
    write_cmos_sensor(0x23ee, 0x4139);
    write_cmos_sensor(0x23f0, 0x413a);
    write_cmos_sensor(0x23f2, 0x413b);
    write_cmos_sensor(0x23f4, 0x4130);
    write_cmos_sensor(0x23f6, 0x120b);
    write_cmos_sensor(0x23f8, 0x120a);
    write_cmos_sensor(0x23fa, 0x1209);
    write_cmos_sensor(0x23fc, 0x1208);
    write_cmos_sensor(0x23fe, 0x1207);
    write_cmos_sensor(0x2400, 0x1206);
    write_cmos_sensor(0x2402, 0x1205);
    write_cmos_sensor(0x2404, 0x1204);
    write_cmos_sensor(0x2406, 0x8221);
    write_cmos_sensor(0x2408, 0x425f);
    write_cmos_sensor(0x240a, 0x0600);
    write_cmos_sensor(0x240c, 0xf35f);
    write_cmos_sensor(0x240e, 0x4fc1);
    write_cmos_sensor(0x2410, 0x0002);
    write_cmos_sensor(0x2412, 0x43c1);
    write_cmos_sensor(0x2414, 0x0003);
    write_cmos_sensor(0x2416, 0x403f);
    write_cmos_sensor(0x2418, 0x0603);
    write_cmos_sensor(0x241a, 0x4fe1);
    write_cmos_sensor(0x241c, 0x0000);
    write_cmos_sensor(0x241e, 0xb3ef);
    write_cmos_sensor(0x2420, 0x0000);
    write_cmos_sensor(0x2422, 0x2431);
    write_cmos_sensor(0x2424, 0x4344);
    write_cmos_sensor(0x2426, 0x4445);
    write_cmos_sensor(0x2428, 0x450f);
    write_cmos_sensor(0x242a, 0x5f0f);
    write_cmos_sensor(0x242c, 0x5f0f);
    write_cmos_sensor(0x242e, 0x403d);
    write_cmos_sensor(0x2430, 0x000e);
    write_cmos_sensor(0x2432, 0x4f1e);
    write_cmos_sensor(0x2434, 0x0632);
    write_cmos_sensor(0x2436, 0x4f1f);
    write_cmos_sensor(0x2438, 0x0634);
    write_cmos_sensor(0x243a, 0x12b0);
    write_cmos_sensor(0x243c, 0xc62c);
    write_cmos_sensor(0x243e, 0x4e08);
    write_cmos_sensor(0x2440, 0x4f09);
    write_cmos_sensor(0x2442, 0x421e);
    write_cmos_sensor(0x2444, 0xf00c);
    write_cmos_sensor(0x2446, 0x430f);
    write_cmos_sensor(0x2448, 0x480a);
    write_cmos_sensor(0x244a, 0x490b);
    write_cmos_sensor(0x244c, 0x4e0c);
    write_cmos_sensor(0x244e, 0x4f0d);
    write_cmos_sensor(0x2450, 0x12b0);
    write_cmos_sensor(0x2452, 0xdf96);
    write_cmos_sensor(0x2454, 0x421a);
    write_cmos_sensor(0x2456, 0xf00e);
    write_cmos_sensor(0x2458, 0x430b);
    write_cmos_sensor(0x245a, 0x403d);
    write_cmos_sensor(0x245c, 0x0009);
    write_cmos_sensor(0x245e, 0x12b0);
    write_cmos_sensor(0x2460, 0xc62c);
    write_cmos_sensor(0x2462, 0x4e06);
    write_cmos_sensor(0x2464, 0x4f07);
    write_cmos_sensor(0x2466, 0x5a06);
    write_cmos_sensor(0x2468, 0x6b07);
    write_cmos_sensor(0x246a, 0x425f);
    write_cmos_sensor(0x246c, 0x0668);
    write_cmos_sensor(0x246e, 0xf37f);
    write_cmos_sensor(0x2470, 0x9f08);
    write_cmos_sensor(0x2472, 0x2c6b);
    write_cmos_sensor(0x2474, 0x4216);
    write_cmos_sensor(0x2476, 0x06ca);
    write_cmos_sensor(0x2478, 0x4307);
    write_cmos_sensor(0x247a, 0x5505);
    write_cmos_sensor(0x247c, 0x4685);
    write_cmos_sensor(0x247e, 0x065e);
    write_cmos_sensor(0x2480, 0x5354);
    write_cmos_sensor(0x2482, 0x9264);
    write_cmos_sensor(0x2484, 0x2bd0);
    write_cmos_sensor(0x2486, 0x403b);
    write_cmos_sensor(0x2488, 0x0603);
    write_cmos_sensor(0x248a, 0x416f);
    write_cmos_sensor(0x248c, 0xc36f);
    write_cmos_sensor(0x248e, 0x4fcb);
    write_cmos_sensor(0x2490, 0x0000);
    write_cmos_sensor(0x2492, 0x12b0);
    write_cmos_sensor(0x2494, 0xcd42);
    write_cmos_sensor(0x2496, 0x41eb);
    write_cmos_sensor(0x2498, 0x0000);
    write_cmos_sensor(0x249a, 0x421f);
    write_cmos_sensor(0x249c, 0x0256);
    write_cmos_sensor(0x249e, 0x522f);
    write_cmos_sensor(0x24a0, 0x421b);
    write_cmos_sensor(0x24a2, 0xf008);
    write_cmos_sensor(0x24a4, 0x532b);
    write_cmos_sensor(0x24a6, 0x9f0b);
    write_cmos_sensor(0x24a8, 0x2c01);
    write_cmos_sensor(0x24aa, 0x4f0b);
    write_cmos_sensor(0x24ac, 0x9381);
    write_cmos_sensor(0x24ae, 0x0002);
    write_cmos_sensor(0x24b0, 0x2409);
    write_cmos_sensor(0x24b2, 0x430a);
    write_cmos_sensor(0x24b4, 0x421e);
    write_cmos_sensor(0x24b6, 0x0614);
    write_cmos_sensor(0x24b8, 0x503e);
    write_cmos_sensor(0x24ba, 0x000a);
    write_cmos_sensor(0x24bc, 0x421f);
    write_cmos_sensor(0x24be, 0x0680);
    write_cmos_sensor(0x24c0, 0x9f0e);
    write_cmos_sensor(0x24c2, 0x2801);
    write_cmos_sensor(0x24c4, 0x431a);
    write_cmos_sensor(0x24c6, 0xb0b2);
    write_cmos_sensor(0x24c8, 0x0020);
    write_cmos_sensor(0x24ca, 0xf002);
    write_cmos_sensor(0x24cc, 0x241f);
    write_cmos_sensor(0x24ce, 0x93c2);
    write_cmos_sensor(0x24d0, 0x82cc);
    write_cmos_sensor(0x24d2, 0x201c);
    write_cmos_sensor(0x24d4, 0x4b0e);
    write_cmos_sensor(0x24d6, 0x430f);
    write_cmos_sensor(0x24d8, 0x521e);
    write_cmos_sensor(0x24da, 0x7300);
    write_cmos_sensor(0x24dc, 0x621f);
    write_cmos_sensor(0x24de, 0x7302);
    write_cmos_sensor(0x24e0, 0x421c);
    write_cmos_sensor(0x24e2, 0x7316);
    write_cmos_sensor(0x24e4, 0x421d);
    write_cmos_sensor(0x24e6, 0x7318);
    write_cmos_sensor(0x24e8, 0x8c0e);
    write_cmos_sensor(0x24ea, 0x7d0f);
    write_cmos_sensor(0x24ec, 0x2c0f);
    write_cmos_sensor(0x24ee, 0x930a);
    write_cmos_sensor(0x24f0, 0x240d);
    write_cmos_sensor(0x24f2, 0x421f);
    write_cmos_sensor(0x24f4, 0x8248);
    write_cmos_sensor(0x24f6, 0xf03f);
    write_cmos_sensor(0x24f8, 0xf7ff);
    write_cmos_sensor(0x24fa, 0x4f82);
    write_cmos_sensor(0x24fc, 0x0a08);
    write_cmos_sensor(0x24fe, 0x0c10);
    write_cmos_sensor(0x2500, 0x421f);
    write_cmos_sensor(0x2502, 0x8252);
    write_cmos_sensor(0x2504, 0xd03f);
    write_cmos_sensor(0x2506, 0x00c0);
    write_cmos_sensor(0x2508, 0x4f82);
    write_cmos_sensor(0x250a, 0x0a12);
    write_cmos_sensor(0x250c, 0x4b0a);
    write_cmos_sensor(0x250e, 0x430b);
    write_cmos_sensor(0x2510, 0x421e);
    write_cmos_sensor(0x2512, 0x7300);
    write_cmos_sensor(0x2514, 0x421f);
    write_cmos_sensor(0x2516, 0x7302);
    write_cmos_sensor(0x2518, 0x5a0e);
    write_cmos_sensor(0x251a, 0x6b0f);
    write_cmos_sensor(0x251c, 0x421c);
    write_cmos_sensor(0x251e, 0x7316);
    write_cmos_sensor(0x2520, 0x421d);
    write_cmos_sensor(0x2522, 0x7318);
    write_cmos_sensor(0x2524, 0x8c0e);
    write_cmos_sensor(0x2526, 0x7d0f);
    write_cmos_sensor(0x2528, 0x2c1a);
    write_cmos_sensor(0x252a, 0x0900);
    write_cmos_sensor(0x252c, 0x7312);
    write_cmos_sensor(0x252e, 0x421e);
    write_cmos_sensor(0x2530, 0x7300);
    write_cmos_sensor(0x2532, 0x421f);
    write_cmos_sensor(0x2534, 0x7302);
    write_cmos_sensor(0x2536, 0x5a0e);
    write_cmos_sensor(0x2538, 0x6b0f);
    write_cmos_sensor(0x253a, 0x421c);
    write_cmos_sensor(0x253c, 0x7316);
    write_cmos_sensor(0x253e, 0x421d);
    write_cmos_sensor(0x2540, 0x7318);
    write_cmos_sensor(0x2542, 0x8c0e);
    write_cmos_sensor(0x2544, 0x7d0f);
    write_cmos_sensor(0x2546, 0x2bf1);
    write_cmos_sensor(0x2548, 0x3c0a);
    write_cmos_sensor(0x254a, 0x460e);
    write_cmos_sensor(0x254c, 0x470f);
    write_cmos_sensor(0x254e, 0x803e);
    write_cmos_sensor(0x2550, 0x0800);
    write_cmos_sensor(0x2552, 0x730f);
    write_cmos_sensor(0x2554, 0x2b92);
    write_cmos_sensor(0x2556, 0x4036);
    write_cmos_sensor(0x2558, 0x07ff);
    write_cmos_sensor(0x255a, 0x4307);
    write_cmos_sensor(0x255c, 0x3f8e);
    write_cmos_sensor(0x255e, 0x5221);
    write_cmos_sensor(0x2560, 0x4134);
    write_cmos_sensor(0x2562, 0x4135);
    write_cmos_sensor(0x2564, 0x4136);
    write_cmos_sensor(0x2566, 0x4137);
    write_cmos_sensor(0x2568, 0x4138);
    write_cmos_sensor(0x256a, 0x4139);
    write_cmos_sensor(0x256c, 0x413a);
    write_cmos_sensor(0x256e, 0x413b);
    write_cmos_sensor(0x2570, 0x4130);
    write_cmos_sensor(0x2572, 0x7400);
    write_cmos_sensor(0x2574, 0x2003);
    write_cmos_sensor(0x2576, 0x72a1);
    write_cmos_sensor(0x2578, 0x2f00);
    write_cmos_sensor(0x257a, 0x7020);
    write_cmos_sensor(0x257c, 0x2f21);
    write_cmos_sensor(0x257e, 0x7800);
    write_cmos_sensor(0x2580, 0x0040);
    write_cmos_sensor(0x2582, 0x7400);
    write_cmos_sensor(0x2584, 0x2005);
    write_cmos_sensor(0x2586, 0x72a1);
    write_cmos_sensor(0x2588, 0x2f00);
    write_cmos_sensor(0x258a, 0x7020);
    write_cmos_sensor(0x258c, 0x2f22);
    write_cmos_sensor(0x258e, 0x7800);
    write_cmos_sensor(0x2590, 0x7400);
    write_cmos_sensor(0x2592, 0x2011);
    write_cmos_sensor(0x2594, 0x72a1);
    write_cmos_sensor(0x2596, 0x2f00);
    write_cmos_sensor(0x2598, 0x7020);
    write_cmos_sensor(0x259a, 0x2f21);
    write_cmos_sensor(0x259c, 0x7800);
    write_cmos_sensor(0x259e, 0x7400);
    write_cmos_sensor(0x25a0, 0x2009);
    write_cmos_sensor(0x25a2, 0x72a1);
    write_cmos_sensor(0x25a4, 0x2f1f);
    write_cmos_sensor(0x25a6, 0x7021);
    write_cmos_sensor(0x25a8, 0x3f40);
    write_cmos_sensor(0x25aa, 0x7800);
    write_cmos_sensor(0x25ac, 0x7400);
    write_cmos_sensor(0x25ae, 0x2005);
    write_cmos_sensor(0x25b0, 0x72a1);
    write_cmos_sensor(0x25b2, 0x2f1f);
    write_cmos_sensor(0x25b4, 0x7021);
    write_cmos_sensor(0x25b6, 0x3f40);
    write_cmos_sensor(0x25b8, 0x7800);
    write_cmos_sensor(0x25ba, 0x7400);
    write_cmos_sensor(0x25bc, 0x2009);
    write_cmos_sensor(0x25be, 0x72a1);
    write_cmos_sensor(0x25c0, 0x2f00);
    write_cmos_sensor(0x25c2, 0x7020);
    write_cmos_sensor(0x25c4, 0x2f22);
    write_cmos_sensor(0x25c6, 0x7800);
    write_cmos_sensor(0x25c8, 0x0009);
    write_cmos_sensor(0x25ca, 0xf572);
    write_cmos_sensor(0x25cc, 0x0009);
    write_cmos_sensor(0x25ce, 0xf582);
    write_cmos_sensor(0x25d0, 0x0009);
    write_cmos_sensor(0x25d2, 0xf590);
    write_cmos_sensor(0x25d4, 0x0009);
    write_cmos_sensor(0x25d6, 0xf59e);
    write_cmos_sensor(0x25d8, 0xf580);
    write_cmos_sensor(0x25da, 0x0004);
    write_cmos_sensor(0x25dc, 0x0009);
    write_cmos_sensor(0x25de, 0xf590);
    write_cmos_sensor(0x25e0, 0x0009);
    write_cmos_sensor(0x25e2, 0xf5ba);
    write_cmos_sensor(0x25e4, 0x0009);
    write_cmos_sensor(0x25e6, 0xf572);
    write_cmos_sensor(0x25e8, 0x0009);
    write_cmos_sensor(0x25ea, 0xf5ac);
    write_cmos_sensor(0x25ec, 0xf580);
    write_cmos_sensor(0x25ee, 0x0004);
    write_cmos_sensor(0x25f0, 0x0009);
    write_cmos_sensor(0x25f2, 0xf572);
    write_cmos_sensor(0x25f4, 0x0009);
    write_cmos_sensor(0x25f6, 0xf5ac);
    write_cmos_sensor(0x25f8, 0x0009);
    write_cmos_sensor(0x25fa, 0xf590);
    write_cmos_sensor(0x25fc, 0x0009);
    write_cmos_sensor(0x25fe, 0xf59e);
    write_cmos_sensor(0x2600, 0xf580);
    write_cmos_sensor(0x2602, 0x0004);
    write_cmos_sensor(0x2604, 0x0009);
    write_cmos_sensor(0x2606, 0xf590);
    write_cmos_sensor(0x2608, 0x0009);
    write_cmos_sensor(0x260a, 0xf59e);
    write_cmos_sensor(0x260c, 0x0009);
    write_cmos_sensor(0x260e, 0xf572);
    write_cmos_sensor(0x2610, 0x0009);
    write_cmos_sensor(0x2612, 0xf5ac);
    write_cmos_sensor(0x2614, 0xf580);
    write_cmos_sensor(0x2616, 0x0004);
    write_cmos_sensor(0x2618, 0x0212);
    write_cmos_sensor(0x261a, 0x0217);
    write_cmos_sensor(0x261c, 0x041f);
    write_cmos_sensor(0x261e, 0x1017);
    write_cmos_sensor(0x2620, 0x0413);
    write_cmos_sensor(0x2622, 0x0103);
    write_cmos_sensor(0x2624, 0x010b);
    write_cmos_sensor(0x2626, 0x1c0a);
    write_cmos_sensor(0x2628, 0x0202);
    write_cmos_sensor(0x262a, 0x0407);
    write_cmos_sensor(0x262c, 0x0205);
    write_cmos_sensor(0x262e, 0x0204);
    write_cmos_sensor(0x2630, 0x0114);
    write_cmos_sensor(0x2632, 0x0110);
    write_cmos_sensor(0x2634, 0xffff);
    write_cmos_sensor(0x2636, 0x0048);
    write_cmos_sensor(0x2638, 0x0090);
    write_cmos_sensor(0x263a, 0x0000);
    write_cmos_sensor(0x263c, 0x0000);
    write_cmos_sensor(0x263e, 0xf618);
    write_cmos_sensor(0x2640, 0x0000);
    write_cmos_sensor(0x2642, 0x0000);
    write_cmos_sensor(0x2644, 0x0060);
    write_cmos_sensor(0x2646, 0x0078);
    write_cmos_sensor(0x2648, 0x0060);
    write_cmos_sensor(0x264a, 0x0078);
    write_cmos_sensor(0x264c, 0x004f);
    write_cmos_sensor(0x264e, 0x0037);
    write_cmos_sensor(0x2650, 0x0048);
    write_cmos_sensor(0x2652, 0x0090);
    write_cmos_sensor(0x2654, 0x0000);
    write_cmos_sensor(0x2656, 0x0000);
    write_cmos_sensor(0x2658, 0xf618);
    write_cmos_sensor(0x265a, 0x0000);
    write_cmos_sensor(0x265c, 0x0000);
    write_cmos_sensor(0x265e, 0x0180);
    write_cmos_sensor(0x2660, 0x0780);
    write_cmos_sensor(0x2662, 0x0180);
    write_cmos_sensor(0x2664, 0x0780);
    write_cmos_sensor(0x2666, 0x04cf);
    write_cmos_sensor(0x2668, 0x0337);
    write_cmos_sensor(0x266a, 0xf636);
    write_cmos_sensor(0x266c, 0xf650);
    write_cmos_sensor(0x266e, 0xf5c8);
    write_cmos_sensor(0x2670, 0xf5dc);
    write_cmos_sensor(0x2672, 0xf5f0);
    write_cmos_sensor(0x2674, 0xf604);
    write_cmos_sensor(0x2676, 0x0100);
    write_cmos_sensor(0x2678, 0xff8a);
    write_cmos_sensor(0x267a, 0xffff);
    write_cmos_sensor(0x267c, 0x0104);
    write_cmos_sensor(0x267e, 0xff0a);
    write_cmos_sensor(0x2680, 0xffff);
    write_cmos_sensor(0x2682, 0x0108);
    write_cmos_sensor(0x2684, 0xff02);
    write_cmos_sensor(0x2686, 0xffff);
    write_cmos_sensor(0x2688, 0x010c);
    write_cmos_sensor(0x268a, 0xff82);
    write_cmos_sensor(0x268c, 0xffff);
    write_cmos_sensor(0x268e, 0x0004);
    write_cmos_sensor(0x2690, 0xf676);
    write_cmos_sensor(0x2692, 0xe4e4);
    write_cmos_sensor(0x2694, 0x4e4e);
    write_cmos_sensor(0x2ffe, 0xc114);
    write_cmos_sensor(0x3224, 0xf222);
    write_cmos_sensor(0x322a, 0xf23c);
    write_cmos_sensor(0x3230, 0xf03a);
    write_cmos_sensor(0x3238, 0xf09a);
    write_cmos_sensor(0x323a, 0xf012);
    write_cmos_sensor(0x323e, 0xf3f6);
    write_cmos_sensor(0x32a0, 0x0000);
    write_cmos_sensor(0x32a2, 0x0000);
    write_cmos_sensor(0x32a4, 0x0000);
    write_cmos_sensor(0x32b0, 0x0000);
    write_cmos_sensor(0x32c0, 0xf66a);
    write_cmos_sensor(0x32c2, 0xf66e);
    write_cmos_sensor(0x32c4, 0x0000);
    write_cmos_sensor(0x32c6, 0xf66e);
    write_cmos_sensor(0x32c8, 0x0000);
    write_cmos_sensor(0x32ca, 0xf68e);
    write_cmos_sensor(0x0a7e, 0x219c);
    write_cmos_sensor(0x3244, 0x8400);
    write_cmos_sensor(0x3246, 0xe400);
    write_cmos_sensor(0x3248, 0xc88e);
    write_cmos_sensor(0x324e, 0xfcd8);
    write_cmos_sensor(0x3250, 0xa060);
    write_cmos_sensor(0x325a, 0x7a37);
    write_cmos_sensor(0x0734, 0x4b0b);
    write_cmos_sensor(0x0736, 0xd8b0);
    write_cmos_sensor(0x0600, 0x1190);
    write_cmos_sensor(0x0602, 0x0052);
    write_cmos_sensor(0x0604, 0x1008);
    write_cmos_sensor(0x0606, 0x0200);
    write_cmos_sensor(0x0616, 0x0040);
    write_cmos_sensor(0x0614, 0x0040);
    write_cmos_sensor(0x0612, 0x0040);
    write_cmos_sensor(0x0610, 0x0040);
    write_cmos_sensor(0x06b2, 0x0500);
    write_cmos_sensor(0x06b4, 0x3ff0);
    write_cmos_sensor(0x0618, 0x0a80);
    write_cmos_sensor(0x0668, 0x4303);
    write_cmos_sensor(0x06ca, 0x02cc);
    write_cmos_sensor(0x066e, 0x0050);
    write_cmos_sensor(0x0670, 0x0050);
    write_cmos_sensor(0x113c, 0x0001);
    write_cmos_sensor(0x11c4, 0x1080);
    write_cmos_sensor(0x11c6, 0x0c34);
    write_cmos_sensor(0x1104, 0x0160);
    write_cmos_sensor(0x1106, 0x0138);
    write_cmos_sensor(0x110a, 0x010e);
    write_cmos_sensor(0x110c, 0x021d);
    write_cmos_sensor(0x110e, 0xba2e);
    write_cmos_sensor(0x1110, 0x0056);
    write_cmos_sensor(0x1112, 0x00ac);
    write_cmos_sensor(0x1114, 0x6907);
    write_cmos_sensor(0x1122, 0x0011);
    write_cmos_sensor(0x1124, 0x0022);
    write_cmos_sensor(0x1126, 0x2e8c);
    write_cmos_sensor(0x1128, 0x0016);
    write_cmos_sensor(0x112a, 0x002b);
    write_cmos_sensor(0x112c, 0x3483);
    write_cmos_sensor(0x1130, 0x0200);
    write_cmos_sensor(0x1132, 0x0200);
    write_cmos_sensor(0x1102, 0x0028);
    write_cmos_sensor(0x113e, 0x0200);
    write_cmos_sensor(0x0d00, 0x4000);
    write_cmos_sensor(0x0d02, 0x8004); //Digital Gain GR - x1
    write_cmos_sensor(0x120a, 0x0a00); //Digital Gain GB - x1
    write_cmos_sensor(0x0214, 0x0200); //Digital Gain R  - x1
    write_cmos_sensor(0x0216, 0x0200); //Digital Gain B  - x1
    write_cmos_sensor(0x0218, 0x0200);
    write_cmos_sensor(0x021a, 0x0200);
    write_cmos_sensor(0x1000, 0x0300);
    write_cmos_sensor(0x1002, 0xc319);
    write_cmos_sensor(0x105a, 0x0091);
    write_cmos_sensor(0x105c, 0x0f08);
    write_cmos_sensor(0x105e, 0x0000);
    write_cmos_sensor(0x1060, 0x3008);
    write_cmos_sensor(0x1062, 0x0000);
    write_cmos_sensor(0x0202, 0x0200);
    write_cmos_sensor(0x0b10, 0x400c);
    write_cmos_sensor(0x0212, 0x0000);
    write_cmos_sensor(0x035e, 0x0701);
    write_cmos_sensor(0x040a, 0x0000);
    write_cmos_sensor(0x0420, 0x0003);
    write_cmos_sensor(0x0424, 0x0c47);
    write_cmos_sensor(0x0418, 0x1010);
    write_cmos_sensor(0x0740, 0x004f);
    write_cmos_sensor(0x0354, 0x1000);
    write_cmos_sensor(0x035c, 0x0303);
    write_cmos_sensor(0x050e, 0x0000);
    write_cmos_sensor(0x0510, 0x0058);
    write_cmos_sensor(0x0512, 0x0058);
    write_cmos_sensor(0x0514, 0x0050);
    write_cmos_sensor(0x0516, 0x0050);
    write_cmos_sensor(0x0260, 0x0003);
    write_cmos_sensor(0x0262, 0x0700);
    write_cmos_sensor(0x0266, 0x0007);
    write_cmos_sensor(0x0250, 0x0000);
    write_cmos_sensor(0x0258, 0x0002);
    write_cmos_sensor(0x025c, 0x0002);
    write_cmos_sensor(0x025a, 0x03e8);
    write_cmos_sensor(0x0256, 0x0100);
    write_cmos_sensor(0x0254, 0x0001);
    write_cmos_sensor(0x0440, 0x000c);
    write_cmos_sensor(0x0908, 0x0003);
    write_cmos_sensor(0x0708, 0x2f00);
    write_cmos_sensor(0x027e, 0x0100);
	
#endif
}



static void preview_setting(void)
{
#if MULTI_WRITE
    LOG_INF("preview_setting multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_preview_mot_ellis_hi1336,
        sizeof(addr_data_pair_preview_mot_ellis_hi1336) /
        sizeof(kal_uint16));
    LOG_INF("preview_setting multi write end\n");
#else
    write_cmos_sensor(0x3250, 0xa060);
    write_cmos_sensor(0x0730, 0x760f);
    write_cmos_sensor(0x0732, 0xe0b0);
    write_cmos_sensor(0x1118, 0x0006);
    write_cmos_sensor(0x1200, 0x0d1f);
    write_cmos_sensor(0x1204, 0x1c01);
    write_cmos_sensor(0x1240, 0x0100);
    write_cmos_sensor(0x0b20, 0x8100);
    write_cmos_sensor(0x0f00, 0x0000);
    write_cmos_sensor(0x1002, 0xc319);
    write_cmos_sensor(0x103e, 0x0000);
    write_cmos_sensor(0x1020, 0xc10b);
    write_cmos_sensor(0x1022, 0x0a31);
    write_cmos_sensor(0x1024, 0x030b);
    write_cmos_sensor(0x1026, 0x0d0f);
    write_cmos_sensor(0x1028, 0x1a0e);
    write_cmos_sensor(0x102a, 0x1311);
    write_cmos_sensor(0x102c, 0x2400);
    write_cmos_sensor(0x1010, 0x07d0);
    write_cmos_sensor(0x1012, 0x016e);
    write_cmos_sensor(0x1014, 0x0063);
    write_cmos_sensor(0x1016, 0x0063);
    write_cmos_sensor(0x101a, 0x0063);
    write_cmos_sensor(0x0404, 0x0008);
    write_cmos_sensor(0x0406, 0x1087);
    write_cmos_sensor(0x0220, 0x0008);
    write_cmos_sensor(0x022a, 0x0017);
    write_cmos_sensor(0x0222, 0x0c80);
    write_cmos_sensor(0x022c, 0x0c89);
    write_cmos_sensor(0x0224, 0x002e);
    write_cmos_sensor(0x022e, 0x0c61);
    write_cmos_sensor(0x0f04, 0x0008);
    write_cmos_sensor(0x0f06, 0x0000);
    write_cmos_sensor(0x023a, 0x1111);
    write_cmos_sensor(0x0234, 0x1111);
    write_cmos_sensor(0x0238, 0x1111);
    write_cmos_sensor(0x0246, 0x0020);
    write_cmos_sensor(0x020a, 0x0bb6);
    write_cmos_sensor(0x021c, 0x0008);
    write_cmos_sensor(0x0206, 0x05dd);
    write_cmos_sensor(0x020e, 0x0cf4);
    write_cmos_sensor(0x0b12, 0x1070);
    write_cmos_sensor(0x0b14, 0x0c30); 
    write_cmos_sensor(0x0204, 0x0000); 
    write_cmos_sensor(0x041c, 0x0048); 
    write_cmos_sensor(0x041e, 0x1047); 
    write_cmos_sensor(0x0b04, 0x037e); 

#endif
}




static void capture_setting(kal_uint16 currefps)
{
    LOG_INF("E! %s currefps:%d\n",__func__,currefps);
#if MULTI_WRITE
    LOG_INF("capture_setting multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_capture_30fps_mot_ellis_hi1336,
        sizeof(addr_data_pair_capture_30fps_mot_ellis_hi1336) /
        sizeof(kal_uint16));
    LOG_INF("capture_setting multi write end\n");
#else
    LOG_INF("capture_setting normal write\n");
    if( currefps > 150) {
        write_cmos_sensor(0x3250, 0xa060);
        write_cmos_sensor(0x0730, 0x760f);
        write_cmos_sensor(0x0732, 0xe0b0);
        write_cmos_sensor(0x1118, 0x0006);
        write_cmos_sensor(0x1200, 0x0d1f);
        write_cmos_sensor(0x1204, 0x1c01);
        write_cmos_sensor(0x1240, 0x0100);
        write_cmos_sensor(0x0b20, 0x8100);
        write_cmos_sensor(0x0f00, 0x0000);
        write_cmos_sensor(0x1002, 0xc319);
        write_cmos_sensor(0x103e, 0x0000);
        write_cmos_sensor(0x1020, 0xc10b);
        write_cmos_sensor(0x1022, 0x0a31);
        write_cmos_sensor(0x1024, 0x030b);
        write_cmos_sensor(0x1026, 0x0d0f);
        write_cmos_sensor(0x1028, 0x1a0e);
        write_cmos_sensor(0x102a, 0x1311);
        write_cmos_sensor(0x102c, 0x2400);
        write_cmos_sensor(0x1010, 0x07d0);
        write_cmos_sensor(0x1012, 0x016e);
        write_cmos_sensor(0x1014, 0x0063);
        write_cmos_sensor(0x1016, 0x0063);
        write_cmos_sensor(0x101a, 0x0063);
        write_cmos_sensor(0x0404, 0x0008);
        write_cmos_sensor(0x0406, 0x1087);
        write_cmos_sensor(0x0220, 0x0008);
        write_cmos_sensor(0x022a, 0x0017);
        write_cmos_sensor(0x0222, 0x0c80);
        write_cmos_sensor(0x022c, 0x0c89);
        write_cmos_sensor(0x0224, 0x002e);
        write_cmos_sensor(0x022e, 0x0c61);
        write_cmos_sensor(0x0f04, 0x0008);
        write_cmos_sensor(0x0f06, 0x0000);
        write_cmos_sensor(0x023a, 0x1111);
        write_cmos_sensor(0x0234, 0x1111);
        write_cmos_sensor(0x0238, 0x1111);
        write_cmos_sensor(0x0246, 0x0020);
        write_cmos_sensor(0x020a, 0x0bb6);
        write_cmos_sensor(0x021c, 0x0008);
        write_cmos_sensor(0x0206, 0x05dd);
        write_cmos_sensor(0x020e, 0x0cf4);
        write_cmos_sensor(0x0b12, 0x1070);
        write_cmos_sensor(0x0b14, 0x0c30); 
        write_cmos_sensor(0x0204, 0x0000); 
        write_cmos_sensor(0x041c, 0x0048);
        write_cmos_sensor(0x041e, 0x1047);
        write_cmos_sensor(0x0b04, 0x037e);
		
		
        write_cmos_sensor(0x0206, 0x05dd); //line length pck
        write_cmos_sensor(0x020e, 0x0d00); //frame length lines
        write_cmos_sensor(0x0b12, 0x1070); //x output size
        write_cmos_sensor(0x0b14, 0x0c30); //y output size
        write_cmos_sensor(0x0204, 0x0000);
        write_cmos_sensor(0x041c, 0x0048);
        write_cmos_sensor(0x041e, 0x1047);
        write_cmos_sensor(0x0b04, 0x037e);
        write_cmos_sensor(0x027e, 0x0100);
        // PIP
    } else    {
        write_cmos_sensor(0x3250, 0xa060);
        write_cmos_sensor(0x0730, 0x760f);
        write_cmos_sensor(0x0732, 0xe0b0);
        write_cmos_sensor(0x1118, 0x0006);
        write_cmos_sensor(0x1200, 0x0d1f);
        write_cmos_sensor(0x1204, 0x1c01);
        write_cmos_sensor(0x1240, 0x0100);
        write_cmos_sensor(0x0b20, 0x8100);
        write_cmos_sensor(0x0f00, 0x0000);
        write_cmos_sensor(0x1002, 0xc319);
        write_cmos_sensor(0x103e, 0x0000);
        write_cmos_sensor(0x1020, 0xc10b);
        write_cmos_sensor(0x1022, 0x0a31);
        write_cmos_sensor(0x1024, 0x030b);
        write_cmos_sensor(0x1026, 0x0d0f);
        write_cmos_sensor(0x1028, 0x1a0e);
        write_cmos_sensor(0x102a, 0x1311);
        write_cmos_sensor(0x102c, 0x2400);
        write_cmos_sensor(0x1010, 0x07d0);
        write_cmos_sensor(0x1012, 0x016e);
        write_cmos_sensor(0x1014, 0x0063);
        write_cmos_sensor(0x1016, 0x0063);
        write_cmos_sensor(0x101a, 0x0063);
        write_cmos_sensor(0x0404, 0x0008);
        write_cmos_sensor(0x0406, 0x1087);
        write_cmos_sensor(0x0220, 0x0008);
        write_cmos_sensor(0x022a, 0x0017);
        write_cmos_sensor(0x0222, 0x0c80);
        write_cmos_sensor(0x022c, 0x0c89);
        write_cmos_sensor(0x0224, 0x002e);
        write_cmos_sensor(0x022e, 0x0c61);
        write_cmos_sensor(0x0f04, 0x0008);
        write_cmos_sensor(0x0f06, 0x0000);
        write_cmos_sensor(0x023a, 0x1111);
        write_cmos_sensor(0x0234, 0x1111);
        write_cmos_sensor(0x0238, 0x1111);
        write_cmos_sensor(0x0246, 0x0020);
        write_cmos_sensor(0x020a, 0x0bb6);
        write_cmos_sensor(0x021c, 0x0008);
        write_cmos_sensor(0x0206, 0x05dd);
        write_cmos_sensor(0x020e, 0x0cf4);
        write_cmos_sensor(0x0b12, 0x1070);
        write_cmos_sensor(0x0b14, 0x0c30);
        write_cmos_sensor(0x0204, 0x0000);
		write_cmos_sensor(0x041c, 0x0048); 
		write_cmos_sensor(0x041e, 0x1047); 
		write_cmos_sensor(0x0b04, 0x037e); 
		
        write_cmos_sensor(0x0206, 0x0bb8); //line length pck
        write_cmos_sensor(0x020e, 0x0d00); //frame length lines
        write_cmos_sensor(0x0b12, 0x1070); //x output size
        write_cmos_sensor(0x0b14, 0x0c30); //y output size
        write_cmos_sensor(0x0204, 0x0000);
        write_cmos_sensor(0x041c, 0x0048);
        write_cmos_sensor(0x041e, 0x1047);
        write_cmos_sensor(0x0b04, 0x037e);
        write_cmos_sensor(0x027e, 0x0100);
    }
#endif
}



static void video_setting(void)
{
#if MULTI_WRITE
    LOG_INF("video_setting multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_capture_30fps_mot_ellis_hi1336,
        sizeof(addr_data_pair_capture_30fps_mot_ellis_hi1336) /
        sizeof(kal_uint16));
#else
    LOG_INF("video_setting normal write\n");
    write_cmos_sensor(0x3250, 0xa060);
    write_cmos_sensor(0x0730, 0x760f);
    write_cmos_sensor(0x0732, 0xe0b0);
    write_cmos_sensor(0x1118, 0x018e);
    write_cmos_sensor(0x1200, 0x0d1f);
    write_cmos_sensor(0x1204, 0x1c01);
    write_cmos_sensor(0x1240, 0x0100);
    write_cmos_sensor(0x0b20, 0x8100);
    write_cmos_sensor(0x0f00, 0x0000);
    write_cmos_sensor(0x1002, 0xc319);
    write_cmos_sensor(0x103e, 0x0000);
    write_cmos_sensor(0x1020, 0xc10b);
    write_cmos_sensor(0x1022, 0x0a31);
    write_cmos_sensor(0x1024, 0x030b);
    write_cmos_sensor(0x1026, 0x0d0f);
    write_cmos_sensor(0x1028, 0x1a0e);
    write_cmos_sensor(0x102a, 0x1311);
    write_cmos_sensor(0x102c, 0x2400);
    write_cmos_sensor(0x1010, 0x07d0);
    write_cmos_sensor(0x1012, 0x016e);
    write_cmos_sensor(0x1014, 0x0063);
    write_cmos_sensor(0x1016, 0x0063);
    write_cmos_sensor(0x101a, 0x0063);
    write_cmos_sensor(0x0404, 0x0008);
    write_cmos_sensor(0x0406, 0x1087);
    write_cmos_sensor(0x0220, 0x0008);
    write_cmos_sensor(0x022a, 0x0017);
    write_cmos_sensor(0x0222, 0x0c80);
    write_cmos_sensor(0x022c, 0x0c89);
    write_cmos_sensor(0x0224, 0x01b6); 
    write_cmos_sensor(0x022e, 0x0ad9); 
    write_cmos_sensor(0x0f04, 0x0008);
    write_cmos_sensor(0x0f06, 0x0000);
    write_cmos_sensor(0x023a, 0x1111);
    write_cmos_sensor(0x0234, 0x1111);
    write_cmos_sensor(0x0238, 0x1111);
    write_cmos_sensor(0x0246, 0x0020);
    write_cmos_sensor(0x020a, 0x0bb6); 
    write_cmos_sensor(0x021c, 0x0008); 
    write_cmos_sensor(0x0206, 0x05dd); 
    write_cmos_sensor(0x020e, 0x0cf4); 
    write_cmos_sensor(0x0b12, 0x1070); 
    write_cmos_sensor(0x0b14, 0x0920); 
    write_cmos_sensor(0x0204, 0x0000);
    write_cmos_sensor(0x041c, 0x0048);
    write_cmos_sensor(0x041e, 0x1047);
    write_cmos_sensor(0x0b04, 0x037e); 

#endif
}



static void hs_video_setting(void)
{

#if MULTI_WRITE
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_hs_video_mot_ellis_hi1336,
        sizeof(addr_data_pair_hs_video_mot_ellis_hi1336) /
        sizeof(kal_uint16));
#else
    LOG_INF("hs_video_setting normal write\n");
    // setting ver5.0 None PD 640x480 120fps
    write_cmos_sensor(0x3250, 0xa470);
    write_cmos_sensor(0x0730, 0x770f);
    write_cmos_sensor(0x0732, 0xe4b0);
    write_cmos_sensor(0x1118, 0x0072);
    write_cmos_sensor(0x1200, 0x011f);
    write_cmos_sensor(0x1204, 0x1c01);
    write_cmos_sensor(0x1240, 0x0100);
    write_cmos_sensor(0x0b20, 0x8600);
    write_cmos_sensor(0x0f00, 0x1400);
    write_cmos_sensor(0x103e, 0x0500);
    write_cmos_sensor(0x1020, 0xc102);
    write_cmos_sensor(0x1022, 0x0209);
    write_cmos_sensor(0x1024, 0x0303);
    write_cmos_sensor(0x1026, 0x0305);
    write_cmos_sensor(0x1028, 0x0903);
    write_cmos_sensor(0x102a, 0x0404);
    write_cmos_sensor(0x102c, 0x0400);
    write_cmos_sensor(0x1010, 0x07d0);
    write_cmos_sensor(0x1012, 0x0040);
    write_cmos_sensor(0x1014, 0xffea);
    write_cmos_sensor(0x1016, 0xffea);
    write_cmos_sensor(0x101a, 0xffea);
    write_cmos_sensor(0x1038, 0x0000);
    write_cmos_sensor(0x1042, 0x0008);
    write_cmos_sensor(0x1004, 0x2b30);
    write_cmos_sensor(0x1048, 0x0080);
    write_cmos_sensor(0x1044, 0x0100);
    write_cmos_sensor(0x1046, 0x0004);
    write_cmos_sensor(0x0404, 0x0008);
    write_cmos_sensor(0x0406, 0x1087);
    write_cmos_sensor(0x0220, 0x000c);
    write_cmos_sensor(0x022a, 0x0013);
    write_cmos_sensor(0x0222, 0x0c80);
    write_cmos_sensor(0x022c, 0x0c89);
    write_cmos_sensor(0x0224, 0x009c);
    write_cmos_sensor(0x022e, 0x0bed);
    write_cmos_sensor(0x0f04, 0x0020);
    write_cmos_sensor(0x0f06, 0x0000);
    write_cmos_sensor(0x023a, 0x6666);
    write_cmos_sensor(0x0234, 0x7755);
    write_cmos_sensor(0x0238, 0x7755);
    write_cmos_sensor(0x0246, 0x0020);
    write_cmos_sensor(0x020a, 0x0339); //coarse integ time
    write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
    write_cmos_sensor(0x0206, 0x05dd); //line length pck
    write_cmos_sensor(0x020e, 0x0340); //frame length lines
    write_cmos_sensor(0x0b12, 0x0280); //x output size
    write_cmos_sensor(0x0b14, 0x01e0); //y output size
    write_cmos_sensor(0x0204, 0x0200);
    write_cmos_sensor(0x041c, 0x0048);
    write_cmos_sensor(0x041e, 0x1047);
    write_cmos_sensor(0x0b04, 0x037e);
    write_cmos_sensor(0x027e, 0x0100);
#endif
}




static void slim_video_setting(void)
{

#if MULTI_WRITE
    LOG_INF("slim_video_setting multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_slim_video_mot_ellis_hi1336,
        sizeof(addr_data_pair_slim_video_mot_ellis_hi1336) /
        sizeof(kal_uint16));
#else
    LOG_INF("slim_video_setting normal write\n");
    write_cmos_sensor(0x3250, 0xa060);
    write_cmos_sensor(0x0730, 0x770f);
    write_cmos_sensor(0x0732, 0xe2b0);
    write_cmos_sensor(0x1118, 0x01a8);
    write_cmos_sensor(0x1200, 0x011f);
    write_cmos_sensor(0x1204, 0x1c01);
    write_cmos_sensor(0x1240, 0x0100);
    write_cmos_sensor(0x0b20, 0x8300);
    write_cmos_sensor(0x0f00, 0x0800);
    write_cmos_sensor(0x103e, 0x0200);
    write_cmos_sensor(0x1020, 0xc104);
    write_cmos_sensor(0x1022, 0x0410);
    write_cmos_sensor(0x1024, 0x0304);
    write_cmos_sensor(0x1026, 0x0507);
    write_cmos_sensor(0x1028, 0x0d05);
    write_cmos_sensor(0x102a, 0x0704);
    write_cmos_sensor(0x102c, 0x1400);
    write_cmos_sensor(0x1010, 0x07d0);
    write_cmos_sensor(0x1012, 0x009c);
    write_cmos_sensor(0x1014, 0x0013);
    write_cmos_sensor(0x1016, 0x0013);
    write_cmos_sensor(0x101a, 0x0013);
    write_cmos_sensor(0x1038, 0x0000);
    write_cmos_sensor(0x1042, 0x0008);
    write_cmos_sensor(0x1004, 0x2b30);
    write_cmos_sensor(0x1048, 0x0080);
    write_cmos_sensor(0x1044, 0x0100);
    write_cmos_sensor(0x1046, 0x0004);
    write_cmos_sensor(0x0404, 0x0008);
    write_cmos_sensor(0x0406, 0x1087);
    write_cmos_sensor(0x0220, 0x0008);
    write_cmos_sensor(0x022a, 0x0017);
    write_cmos_sensor(0x0222, 0x0c80);
    write_cmos_sensor(0x022c, 0x0c89);
    write_cmos_sensor(0x0224, 0x020a);
    write_cmos_sensor(0x022e, 0x0a83);
    write_cmos_sensor(0x0f04, 0x0040);
    write_cmos_sensor(0x0f06, 0x0000);
    write_cmos_sensor(0x023a, 0x3333);
    write_cmos_sensor(0x0234, 0x3333);
    write_cmos_sensor(0x0238, 0x3333);
    write_cmos_sensor(0x0246, 0x0020);
    write_cmos_sensor(0x020a, 0x0339); //coarse integ time
    write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
    write_cmos_sensor(0x0206, 0x05dd); //line length pck
    write_cmos_sensor(0x020e, 0x0340); //frame length lines
    write_cmos_sensor(0x0b12, 0x0500); //x output size
    write_cmos_sensor(0x0b14, 0x02d0); //y output size
    write_cmos_sensor(0x0204, 0x0000);
    write_cmos_sensor(0x041c, 0x0048);
    write_cmos_sensor(0x041e, 0x1047);
    write_cmos_sensor(0x0b04, 0x037e);
    write_cmos_sensor(0x027e, 0x0100);


#endif
}



static void custom1_setting(void){
#if MULTI_WRITE
    LOG_INF("custom1_setting multi write\n");
    mot_ellis_hi1336_table_write_cmos_sensor(
        addr_data_pair_custom1_hi1336,
        sizeof(addr_data_pair_custom1_hi1336) /
        sizeof(kal_uint16));
#else
    LOG_INF("custom1_setting normal write\n");
    write_cmos_sensor(0x3250, 0xa060);
    write_cmos_sensor(0x0730, 0x600f);
    write_cmos_sensor(0x0732, 0xe0b0);
    write_cmos_sensor(0x1118, 0x0000);
    write_cmos_sensor(0x1200, 0x0d1f);
    write_cmos_sensor(0x1204, 0x1c01);
    write_cmos_sensor(0x1240, 0x0100);
    write_cmos_sensor(0x0b20, 0x8100);
    write_cmos_sensor(0x0f00, 0x0000);
    write_cmos_sensor(0x103e, 0x0000);
    write_cmos_sensor(0x1020, 0xc10b);
    write_cmos_sensor(0x1022, 0x0a31);
    write_cmos_sensor(0x1024, 0x030b);
    write_cmos_sensor(0x1026, 0x0d0f);
    write_cmos_sensor(0x1028, 0x1a0e);
    write_cmos_sensor(0x102a, 0x1311);
    write_cmos_sensor(0x102c, 0x2400);
    write_cmos_sensor(0x1010, 0x2720);
    write_cmos_sensor(0x1012, 0x0140);
    write_cmos_sensor(0x1014, 0x00fe);
    write_cmos_sensor(0x1016, 0x00fe);
    write_cmos_sensor(0x101a, 0x00fe);
    write_cmos_sensor(0x0404, 0x0008);
    write_cmos_sensor(0x0406, 0x1087);
    write_cmos_sensor(0x0220, 0x0008);
    write_cmos_sensor(0x022a, 0x0017);
    write_cmos_sensor(0x0222, 0x0c80);
    write_cmos_sensor(0x022c, 0x0c89);
    write_cmos_sensor(0x0224, 0x017e);
    write_cmos_sensor(0x022e, 0x0b11);
    write_cmos_sensor(0x0f04, 0x01e0);
    write_cmos_sensor(0x0f06, 0x0000);
    write_cmos_sensor(0x023a, 0x1111);
    write_cmos_sensor(0x0234, 0x1111);
    write_cmos_sensor(0x0238, 0x1111);
    write_cmos_sensor(0x0246, 0x0020);
    write_cmos_sensor(0x020a, 0x1036);
    write_cmos_sensor(0x021c, 0x0008);
    write_cmos_sensor(0x0206, 0x05dd);
    write_cmos_sensor(0x020e, 0x1043);
    write_cmos_sensor(0x0b12, 0x0cc0);
    write_cmos_sensor(0x0b14, 0x0990);
    write_cmos_sensor(0x0204, 0x0000);
    write_cmos_sensor(0x041c, 0x0048);
    write_cmos_sensor(0x041e, 0x1047);
    write_cmos_sensor(0x0b04, 0x037e);
    write_cmos_sensor(0x027e, 0x0100);
#endif
}
#if 0
#include "cam_cal_define.h"
#include <linux/slab.h>
#define EEPROM_ID 0xA0

static kal_uint16 read_eeprom(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, EEPROM_ID);
	return get_byte;
}

static struct stCAM_CAL_DATAINFO_STRUCT mot_ellis_hi1336_eeprom_data ={
    .sensorID= ELLIS_QTECH_HI1336_SENSOR_ID,
    .deviceID = 0x01,
    .dataLength = 0x0CE9,
    .sensorVendorid = 0x19000101,
    .vendorByte = {1,2,3,4},
    .dataBuffer = NULL,
};
static struct stCAM_CAL_CHECKSUM_STRUCT mot_ellis_hi1336Checksum[8] =
{
    {MODULE_ITEM,0x0000,0x0000,0x0017,0x0018,0x55},
    {AWB_ITEM,0x0019,0x0019,0x0029,0x002a,0x55},
    {AF_ITEM,0x002b,0x002b,0x0030,0x0031,0x55},
    {LSC_ITEM,0x0032,0x0032,0x077e,0x077f,0x55},
    {PDAF_ITEM,0x0780,0x0780,0x0ce6,0x0ce7,0x55},
    {TOTAL_ITEM,0x0000,0x0000,0x0ce7,0x0ce8,0x55},
    {MAX_ITEM,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x55},  // this line must haved
};
extern int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData,
    struct stCAM_CAL_CHECKSUM_STRUCT* checkData);
extern int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData);
#endif
#define ELLIS_Hi1336_EEPROM_SLAVE_ADDR 0xA0
#define ELLIS_Hi1336_SENSOR_IIC_SLAVE_ADDR 0x40
#define ELLIS_Hi1336_EEPROM_SIZE  0x1D1B
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/hi1336_eeprom_data.bin"
#define ELLIS_Hi1336_EEPROM_CRC_AF_CAL_SIZE 24
#define ELLIS_Hi1336_EEPROM_CRC_AWB_CAL_SIZE 43
#define ELLIS_Hi1336_EEPROM_CRC_LSC_SIZE 1868
#define ELLIS_Hi1336_EEPROM_CRC_PDAF_OUTPUT1_SIZE 496
#define ELLIS_Hi1336_EEPROM_CRC_PDAF_OUTPUT2_SIZE 1004
#define ELLIS_Hi1336_EEPROM_CRC_MANUFACTURING_SIZE 37
#define ELLIS_Hi1336_MANUFACTURE_PART_NUMBER "28D14866"
#define ELLIS_Hi1336_MPN_LENGTH 8

static uint8_t ELLIS_Hi1336_eeprom[ELLIS_Hi1336_EEPROM_SIZE] = {0};
static calibration_status_t mnf_status = CRC_FAILURE;
static calibration_status_t af_status = CRC_FAILURE;
static calibration_status_t awb_status = CRC_FAILURE;
static calibration_status_t lsc_status = CRC_FAILURE;
static calibration_status_t pdaf_status = CRC_FAILURE;
static calibration_status_t dual_status = CRC_FAILURE;

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static uint16_t to_uint16_swap(uint8_t *data)
{
	uint16_t converted;
	memcpy(&converted, data, sizeof(uint16_t));
	return ntohs(converted);
}

static void ELLIS_Hi1336_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = slave;
	spin_unlock(&imgsensor_drv_lock);

	//read eeprom data
	for (i = 0; i < size; i ++) {
		ELLIS_Hi1336_eeprom[i] = read_cmos_sensor(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = ELLIS_Hi1336_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_drv_lock);
}

static void ELLIS_Hi1336_eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
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

static calibration_status_t ELLIS_Hi1336_check_awb_data(void *data)
{
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	if(!eeprom_util_check_crc16(eeprom->cie_src_1_ev,
		ELLIS_Hi1336_EEPROM_CRC_AWB_CAL_SIZE,
		convert_crc(eeprom->awb_crc16))) {
		LOG_INF("AWB CRC Fails!");
		return CRC_FAILURE;
	}

	unit.r = to_uint16_swap(eeprom->awb_src_1_r)/64;
	unit.gr = to_uint16_swap(eeprom->awb_src_1_gr)/64;
	unit.gb = to_uint16_swap(eeprom->awb_src_1_gb)/64;
	unit.b = to_uint16_swap(eeprom->awb_src_1_b)/64;
	unit.r_g = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(eeprom->awb_src_1_golden_r)/64;
	golden.gr = to_uint16_swap(eeprom->awb_src_1_golden_gr)/64;
	golden.gb = to_uint16_swap(eeprom->awb_src_1_golden_gb)/64;
	golden.b = to_uint16_swap(eeprom->awb_src_1_golden_b)/64;
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

static calibration_status_t ELLIS_Hi1336_check_lsc_data_mtk(void *data)
{
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->lsc_data_mtk, ELLIS_Hi1336_EEPROM_CRC_LSC_SIZE,
		convert_crc(eeprom->lsc_crc16_mtk))) {
		LOG_INF("LSC CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("LSC CRC Pass");
	return NO_ERRORS;
}

static void ELLIS_Hi1336_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;

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
	ELLIS_Hi1336_eeprom_dump_bin(MAIN_SERIAL_NUM_DATA_PATH,  HI1336_SERIAL_NUM_SIZE,  eeprom->serial_number);
	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}

static calibration_status_t ELLIS_Hi1336_check_af_data(void *data)
{
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->af_data, ELLIS_Hi1336_EEPROM_CRC_AF_CAL_SIZE,
		convert_crc(eeprom->af_crc16))) {
		LOG_INF("Autofocus CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Autofocus CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t ELLIS_Hi1336_check_pdaf_data(void *data)
{
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->pdaf_out1_data_mtk, ELLIS_Hi1336_EEPROM_CRC_PDAF_OUTPUT1_SIZE,
		convert_crc(eeprom->pdaf_out1_crc16_mtk))) {
		LOG_INF("PDAF OUTPUT1 CRC Fails!");
		return CRC_FAILURE;
	}

	if (!eeprom_util_check_crc16(eeprom->pdaf_out2_data_mtk, ELLIS_Hi1336_EEPROM_CRC_PDAF_OUTPUT2_SIZE,
		convert_crc(eeprom->pdaf_out2_crc16_mtk))) {
		LOG_INF("PDAF OUTPUT2 CRC Fails!");
		return CRC_FAILURE;
	}

	LOG_INF("PDAF CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t ELLIS_Hi1336_check_manufacturing_data(void *data)
{
	struct ELLIS_Hi1336_eeprom_t *eeprom = (struct ELLIS_Hi1336_eeprom_t*)data;
LOG_INF("Manufacturing eeprom->mpn = %s !",eeprom->mpn);
#if 0
	if(strncmp(eeprom->mpn, ELLIS_Hi1336_MANUFACTURE_PART_NUMBER, ELLIS_Hi1336_MPN_LENGTH) != 0) {
		LOG_INF("Manufacturing part number (%s) check Fails!", eeprom->mpn);
		return CRC_FAILURE;
	}
#endif
	if (!eeprom_util_check_crc16(data, ELLIS_Hi1336_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static void ELLIS_Hi1336_eeprom_format_calibration_data(void *data)
{
	if (NULL == data) {
		LOG_INF("data is NULL");
		return;
	}

	mnf_status = ELLIS_Hi1336_check_manufacturing_data(data);
	af_status = ELLIS_Hi1336_check_af_data(data);
	awb_status = ELLIS_Hi1336_check_awb_data(data);;
	lsc_status = ELLIS_Hi1336_check_lsc_data_mtk(data);;
	pdaf_status = ELLIS_Hi1336_check_pdaf_data(data);
	dual_status = 0;

	LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, dual:%d",
		mnf_status, af_status, awb_status, lsc_status, pdaf_status, dual_status);
}
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    //kal_int32 size = 0;

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
                /*
                size = imgSensorReadEepromData(&mot_ellis_hi1336_eeprom_data,mot_ellis_hi1336Checksum);
                if(size != mot_ellis_hi1336_eeprom_data.dataLength) {
                    printk("get eeprom data failed\n");
                    LOG_INF("mot_ellis_hi1336 size:%d\n", size);
                    if(mot_ellis_hi1336_eeprom_data.dataBuffer != NULL) {
                        kfree(mot_ellis_hi1336_eeprom_data.dataBuffer);
                        mot_ellis_hi1336_eeprom_data.dataBuffer = NULL;
                    }
                    *sensor_id = 0xFFFFFFFF;
                    return ERROR_SENSOR_CONNECT_FAIL;
                } else {
                    LOG_INF("get eeprom data success\n");
                    imgSensorSetEepromData(&mot_ellis_hi1336_eeprom_data);
                }
                */
                ELLIS_Hi1336_read_data_from_eeprom(ELLIS_Hi1336_EEPROM_SLAVE_ADDR, 0x0d75, ELLIS_Hi1336_EEPROM_SIZE);
                ELLIS_Hi1336_eeprom_dump_bin(EEPROM_DATA_PATH, ELLIS_Hi1336_EEPROM_SIZE, (void *)ELLIS_Hi1336_eeprom);
                ELLIS_Hi1336_eeprom_format_calibration_data((void *)ELLIS_Hi1336_eeprom);
                LOG_ERR("mot_ellis_hi1336 get_imgsensor_id: mot_ellis_hi1336 :i2c write id : 0x%x, sensor id: 0x%x\n",
                imgsensor.i2c_write_id, *sensor_id);
                return ERROR_NONE;
            }
            retry--;
        } while (retry > 0);
        i++;
        retry = 2;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        LOG_ERR("mot_ellis_hi1336 get_imgsensor_id: Read id fail,sensor id: 0x%x\n", *sensor_id);
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
    kal_uint16 sensor_id = 0;

    LOG_INF("[open]: PLATFORM:MT6737,MIPI 24LANE\n");
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if (sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
                    imgsensor.i2c_write_id, sensor_id);
                break;
            }
            retry--;
        } while (retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id) {
        LOG_INF("open sensor id fail: 0x%x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    /* initail sequence write in  */
    sensor_init();

    spin_lock(&imgsensor_drv_lock);
    imgsensor.autoflicker_en = KAL_FALSE;
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
    //imgsensor.pdaf_mode = 1;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}    /*    open  */

static kal_uint32 close(void)
{
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
 *    *image_window : address pointer of pixel numbers in one period of
HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period
of VSYNC
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
    LOG_INF("E");
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
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

    if (imgsensor.current_fps == imgsensor_info.cap.max_framerate)    {
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
     //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }

    spin_unlock(&imgsensor_drv_lock);
    LOG_INF("Caputre fps:%d\n", imgsensor.current_fps);
    capture_setting(imgsensor.current_fps);

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
    imgsensor.current_fps = 300;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    video_setting();
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

    return ERROR_NONE;
}    /*    slim_video     */
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
    custom1_setting();
    return ERROR_NONE;
}   /*  Custom1    */

static kal_uint32 get_resolution(
        MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
    sensor_resolution->SensorFullWidth =
        imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight =
        imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth =
        imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight =
        imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth =
        imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight =
        imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth =
        imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight =
        imgsensor_info.hs_video.grabwindow_height;

    sensor_resolution->SensorSlimVideoWidth =
        imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight =
        imgsensor_info.slim_video.grabwindow_height;
    sensor_resolution->SensorCustom1Width  =  imgsensor_info.custom1.grabwindow_width;
    sensor_resolution->SensorCustom1Height =  imgsensor_info.custom1.grabwindow_height;
    return ERROR_NONE;
}    /*    get_resolution    */


static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
            MSDK_SENSOR_INFO_STRUCT *sensor_info,
            MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("scenario_id = %d\n", scenario_id);

    sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorInterruptDelayLines = 4; /* not use */
    sensor_info->SensorResetActiveHigh = FALSE; /* not use */
    sensor_info->SensorResetDelayCount = 5; /* not use */

    sensor_info->SensroInterfaceType =
    imgsensor_info.sensor_interface_type;
    sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
    sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
    sensor_info->SensorOutputDataFormat =
        imgsensor_info.sensor_output_dataformat;

    sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
    sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
    sensor_info->VideoDelayFrame =
        imgsensor_info.video_delay_frame;
    sensor_info->HighSpeedVideoDelayFrame =
        imgsensor_info.hs_video_delay_frame;
    sensor_info->SlimVideoDelayFrame =
        imgsensor_info.slim_video_delay_frame;
    sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;

    sensor_info->SensorMasterClockSwitch = 0; /* not use */
    sensor_info->SensorDrivingCurrent =
        imgsensor_info.isp_driving_current;
/* The frame of setting shutter default 0 for TG int */
    sensor_info->AEShutDelayFrame =
        imgsensor_info.ae_shut_delay_frame;
/* The frame of setting sensor gain */
    sensor_info->AESensorGainDelayFrame =
        imgsensor_info.ae_sensor_gain_delay_frame;
    sensor_info->AEISPGainDelayFrame =
        imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine =
        imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum =
        imgsensor_info.sensor_mode_num;

    sensor_info->SensorMIPILaneNumber =
        imgsensor_info.mipi_lane_num;
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

#if ENABLE_PDAF
    sensor_info->PDAF_Support = 2;
#endif
    sensor_info->calibration_status.mnf = mnf_status;
    sensor_info->calibration_status.af = af_status;
    sensor_info->calibration_status.awb = awb_status;
    sensor_info->calibration_status.lsc = lsc_status;
    sensor_info->calibration_status.pdaf = pdaf_status;
    sensor_info->calibration_status.dual = dual_status;
    ELLIS_Hi1336_eeprom_get_mnf_data((void *) ELLIS_Hi1336_eeprom, &sensor_info->mnf_calibration);
    switch (scenario_id) {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
                imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
    break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
            imgsensor_info.cap.mipi_data_lp2hs_settle_dc;
    break;
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
            imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;
    break;
    case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
        sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;
        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
            imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;
    break;
    case MSDK_SCENARIO_ID_SLIM_VIDEO:
        sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;
        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
            imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;
    break;
    case MSDK_SCENARIO_ID_CUSTOM1:
        sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
    break;
    default:
        sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
        sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

        sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
            imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
    break;
    }

    return ERROR_NONE;
}    /*    get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
            MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
            MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("scenario_id = %d\n", scenario_id);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.current_scenario_id = scenario_id;
    spin_unlock(&imgsensor_drv_lock);
    switch (scenario_id) {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        LOG_INF("preview\n");
        preview(image_window, sensor_config_data);
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        LOG_INF("capture\n");
    //case MSDK_SCENARIO_ID_CAMERA_ZSD:
        capture(image_window, sensor_config_data);
        break;
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        LOG_INF("video preview\n");
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
    default:
        LOG_INF("default mode\n");
        preview(image_window, sensor_config_data);
        return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
    LOG_INF("framerate = %d ", framerate);
    // SetVideoMode Function should fix framerate
    if (framerate == 0)
        // Dynamic frame rate
        return ERROR_NONE;
    spin_lock(&imgsensor_drv_lock);

    if ((framerate == 30) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 296;
    else if ((framerate == 15) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 146;
    else
        imgsensor.current_fps = 10 * framerate;
    spin_unlock(&imgsensor_drv_lock);
    set_max_framerate(imgsensor.current_fps, 1);
    set_dummy();
    return ERROR_NONE;
}


static kal_uint32 set_auto_flicker_mode(kal_bool enable,
            UINT16 framerate)
{
    LOG_INF("enable = %d, framerate = %d ", enable, framerate);
    spin_lock(&imgsensor_drv_lock);
    if (enable)
        imgsensor.autoflicker_en = KAL_TRUE;
    else //Cancel Auto flick
        imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
            enum MSDK_SCENARIO_ID_ENUM scenario_id,
            MUINT32 framerate)
{
    kal_uint32 frame_length;

    LOG_INF("scenario_id = %d, framerate = %d\n",
                scenario_id, framerate);

    switch (scenario_id) {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        frame_length = imgsensor_info.pre.pclk / framerate * 10 /
            imgsensor_info.pre.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.pre.framelength) ?
            (frame_length - imgsensor_info.pre.framelength) : 0;
        imgsensor.frame_length = imgsensor_info.pre.framelength +
            imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        if (framerate == 0)
            return ERROR_NONE;
        frame_length = imgsensor_info.normal_video.pclk /
            framerate * 10 / imgsensor_info.normal_video.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.normal_video.framelength) ?
        (frame_length - imgsensor_info.normal_video.framelength) : 0;
        imgsensor.frame_length = imgsensor_info.normal_video.framelength +
            imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        if (imgsensor.current_fps ==
                imgsensor_info.cap1.max_framerate) {
        frame_length = imgsensor_info.cap1.pclk / framerate * 10 /
                imgsensor_info.cap1.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.cap1.framelength) ?
            (frame_length - imgsensor_info.cap1.framelength) : 0;
        imgsensor.frame_length = imgsensor_info.cap1.framelength +
                imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        } else {
            if (imgsensor.current_fps !=
                imgsensor_info.cap.max_framerate)
            LOG_INF("fps %d fps not support,use cap: %d fps!\n",
            framerate, imgsensor_info.cap.max_framerate/10);
            frame_length = imgsensor_info.cap.pclk /
                framerate * 10 / imgsensor_info.cap.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length >
                imgsensor_info.cap.framelength) ?
            (frame_length - imgsensor_info.cap.framelength) : 0;
            imgsensor.frame_length =
                imgsensor_info.cap.framelength +
                imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
        }
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
        frame_length = imgsensor_info.hs_video.pclk /
            framerate * 10 / imgsensor_info.hs_video.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.hs_video.framelength) ? (frame_length -
            imgsensor_info.hs_video.framelength) : 0;
        imgsensor.frame_length = imgsensor_info.hs_video.framelength +
            imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    case MSDK_SCENARIO_ID_SLIM_VIDEO:
        frame_length = imgsensor_info.slim_video.pclk /
            framerate * 10 / imgsensor_info.slim_video.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.slim_video.framelength) ? (frame_length -
            imgsensor_info.slim_video.framelength) : 0;
        imgsensor.frame_length =
            imgsensor_info.slim_video.framelength +
            imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    case MSDK_SCENARIO_ID_CUSTOM1:
        frame_length = imgsensor_info.custom1.pclk /
            framerate * 10 / imgsensor_info.custom1.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.custom1.framelength) ? (frame_length -
            imgsensor_info.custom1.framelength) : 0;
        imgsensor.frame_length =
            imgsensor_info.custom1.framelength +
            imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
    break;
    default:  //coding with  preview scenario by default
        frame_length = imgsensor_info.pre.pclk / framerate * 10 /
                        imgsensor_info.pre.linelength;
        spin_lock(&imgsensor_drv_lock);
        imgsensor.dummy_line = (frame_length >
            imgsensor_info.pre.framelength) ?
            (frame_length - imgsensor_info.pre.framelength) : 0;
        imgsensor.frame_length = imgsensor_info.pre.framelength +
                imgsensor.dummy_line;
        imgsensor.min_frame_length = imgsensor.frame_length;
        spin_unlock(&imgsensor_drv_lock);
        if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
        LOG_INF("error scenario_id = %d, we use preview scenario\n",
                scenario_id);
    break;
    }
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
                enum MSDK_SCENARIO_ID_ENUM scenario_id,
                MUINT32 *framerate)
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
    default:
    break;
    }

    return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("set_test_pattern_mode enable: %d", enable);
    if (enable) {
        write_cmos_sensor(0x1038, 0x0000); //mipi_virtual_channel_ctrl
        write_cmos_sensor(0x1042, 0x0008); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
        write_cmos_sensor(0x0b04, 0x0141);
        write_cmos_sensor(0x0C0A, 0x0200);

    } else {
        write_cmos_sensor(0x1038, 0x4100); //mipi_virtual_channel_ctrl
        write_cmos_sensor(0x1042, 0x0108); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
        write_cmos_sensor(0x0b04, 0x0349);
        write_cmos_sensor(0x0C0A, 0x0000);

    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
    pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

    if (enable)
        write_cmos_sensor(0x0b00, 0x0100); // stream on
    else
        write_cmos_sensor(0x0b00, 0x0000); // stream off

    mdelay(10);
    return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
    INT32 temperature_convert = 25;
    return temperature_convert;
}

static kal_uint32 feature_control(
            MSDK_SENSOR_FEATURE_ENUM feature_id,
            UINT8 *feature_para, UINT32 *feature_para_len)
{
    UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
    UINT16 *feature_data_16 = (UINT16 *) feature_para;
    UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
    UINT32 *feature_data_32 = (UINT32 *) feature_para;
    INT32 *feature_return_para_i32 = (INT32 *) feature_para;

#if ENABLE_PDAF
    struct SET_PD_BLOCK_INFO_T *PDAFinfo;
    struct SENSOR_VC_INFO_STRUCT *pvcinfo;
#endif

    unsigned long long *feature_data =
        (unsigned long long *) feature_para;

    struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
        (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    LOG_INF_D("feature_id = %d\n", feature_id);
    switch (feature_id) {
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
		
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	
    case SENSOR_FEATURE_GET_PERIOD:
        *feature_return_para_16++ = imgsensor.line_length;
        *feature_return_para_16 = imgsensor.frame_length;
        *feature_para_len = 4;
    break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *feature_return_para_32 = imgsensor.pclk;
        *feature_para_len = 4;
    break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        set_shutter(*feature_data);
    break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        night_mode((BOOL) * feature_data);
    break;
    case SENSOR_FEATURE_SET_GAIN:
        set_gain((UINT16) *feature_data);
    break;
    case SENSOR_FEATURE_SET_FLASHLIGHT:
    break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
    break;
    case SENSOR_FEATURE_SET_REGISTER:
        write_cmos_sensor(sensor_reg_data->RegAddr,
                        sensor_reg_data->RegData);
    break;
    case SENSOR_FEATURE_GET_REGISTER:
        sensor_reg_data->RegData =
                read_cmos_sensor(sensor_reg_data->RegAddr);
    break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        *feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
        *feature_para_len = 4;
    break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:
        set_video_mode(*feature_data);
    break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
        get_imgsensor_id(feature_return_para_32);
    break;
    case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
        set_auto_flicker_mode((BOOL)*feature_data_16,
            *(feature_data_16+1));
    break;
    case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
        set_max_framerate_by_scenario(
            (enum MSDK_SCENARIO_ID_ENUM)*feature_data,
            *(feature_data+1));
    break;
    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
        get_default_framerate_by_scenario(
            (enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
            (MUINT32 *)(uintptr_t)(*(feature_data+1)));
    break;
    case SENSOR_FEATURE_SET_TEST_PATTERN:
        set_test_pattern_mode((BOOL)*feature_data);
    break;
    case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
        *feature_return_para_32 = imgsensor_info.checksum_value;
        *feature_para_len = 4;
    break;
    case SENSOR_FEATURE_SET_FRAMERATE:
        spin_lock(&imgsensor_drv_lock);
        imgsensor.current_fps = *feature_data_16;
        spin_unlock(&imgsensor_drv_lock);
            LOG_INF("current fps :%d\n", imgsensor.current_fps);
    break;
    case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
        set_shutter_frame_length(
            (UINT32) *feature_data, (UINT16) *(feature_data + 1));
        break;

    case SENSOR_FEATURE_SET_HDR:
        spin_lock(&imgsensor_drv_lock);
        imgsensor.ihdr_en = (BOOL)*feature_data_32;
        spin_unlock(&imgsensor_drv_lock);
    break;
    case SENSOR_FEATURE_GET_CROP_INFO:
        LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
                (UINT32)*feature_data);

        wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)
            (uintptr_t)(*(feature_data+1));

        switch (*feature_data_32) {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            memcpy((void *)wininfo,
                (void *)&imgsensor_winsize_info[1],
                sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            memcpy((void *)wininfo,
                (void *)&imgsensor_winsize_info[2],
                sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            memcpy((void *)wininfo,
                (void *)&imgsensor_winsize_info[3],
                sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            memcpy((void *)wininfo,
                (void *)&imgsensor_winsize_info[4],
                sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        case MSDK_SCENARIO_ID_CUSTOM1:
            memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[5],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        default:
            memcpy((void *)wininfo,
                (void *)&imgsensor_winsize_info[0],
                sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
        break;
        }
    break;
    case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
        /*LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
            (UINT16)*feature_data, (UINT16)*(feature_data+1),
            (UINT16)*(feature_data+2));*/
    #if 0
        ihdr_write_shutter_gain((UINT16)*feature_data,
            (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
    #endif
    break;
    case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
        *feature_return_para_i32 = get_sensor_temperature();
        *feature_para_len = 4;
        break;
    case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
        streaming_control(KAL_FALSE);
        break;
    case SENSOR_FEATURE_SET_STREAMING_RESUME:
        if (*feature_data != 0)
            set_shutter(*feature_data);
        streaming_control(KAL_TRUE);
        break;
#if ENABLE_PDAF
    case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
        LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
        //PDAF capacity enable or not, 2p8 only full size support PDAF
        switch (*feature_data) {
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; // type2 - VC enable
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                #ifdef __ENABLE_VIDEO_CUSTOM_PDAF__
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; // video & capture use same setting
                #else
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
                #endif
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
            case MSDK_SCENARIO_ID_CUSTOM1:
                #ifdef __ENABLE_VIDEO_CUSTOM_PDAF__
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
                #else
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
                #endif
                break;
            default:
                *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
                break;
        }
        break;
    case SENSOR_FEATURE_GET_VC_INFO:
        LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
        pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
        switch (*feature_data_32)
        {
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                LOG_INF("SENSOR_FEATURE_GET_VC_INFO CAPTURE_JPEG\n");
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(struct SENSOR_VC_INFO_STRUCT));
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                LOG_INF("SENSOR_FEATURE_GET_VC_INFO VIDEO PREVIEW\n");
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(struct SENSOR_VC_INFO_STRUCT));
                break;
            case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                LOG_INF("SENSOR_FEATURE_GET_VC_INFO DEFAULT_PREVIEW\n");
                memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(struct SENSOR_VC_INFO_STRUCT));
                break;
            default:
                break;
        }
        break;
    case SENSOR_FEATURE_GET_PDAF_INFO:
        pr_info("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n", *feature_data);
        PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

        switch (*feature_data) {
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
                break;
            case MSDK_SCENARIO_ID_CUSTOM1:
                #ifdef __ENABLE_VIDEO_CUSTOM_PDAF__
                memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info_3264_2448,sizeof(struct SET_PD_BLOCK_INFO_T));
                #endif
                break;
            case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            case MSDK_SCENARIO_ID_SLIM_VIDEO:
            case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
                break;
            default:
                break;
        }
        break;
    case SENSOR_FEATURE_GET_PDAF_DATA:
        pr_info("SENSOR_FEATURE_GET_PDAF_DATA\n");
        break;
    case SENSOR_FEATURE_SET_PDAF:
        LOG_ERR("[HI1336] PDAF mode : %d\n", *feature_data_16);
        imgsensor.pdaf_mode = *feature_data_16;
        break;
#endif

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

UINT32 MOT_ELLIS_HI1336_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc != NULL)
        *pfFunc =  &sensor_func;
    return ERROR_NONE;
}    /*    ELLIS_QTECH_HI1336_MIPI_RAW_SensorInit    */
