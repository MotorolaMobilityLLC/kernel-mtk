/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     s5k2l7_setting_mode2.h
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     CMOS sensor setting file
 *
 ****************************************************************************/
#ifndef _s5k2l7MIPI_SETTING_MODE2_H_
#define _s5k2l7MIPI_SETTING_MODE2_H_

#define _S5K2L7_MODE2_SENSOR_INFO_ static imgsensor_info_struct _imgsensor_info_m2 =                           \
{                                                                                                              \
    .sensor_id = S5K2L7_SENSOR_ID,   /* record sensor id defined in Kd_imgsensor.h */                          \
                                                                                                               \
    .checksum_value = 0xafb5098f,    /* checksum value for Camera Auto Test  */                                \
                                                                                                               \
    .pre = {                                                                                                   \
                                                                                                               \
        .pclk = 960000000,               /* record different mode's pclk */                                    \
        .linelength = 10160,             /* record different mode's linelength */                              \
        .framelength =3149,              /* record different mode's framelength */                             \
        .startx = 0,                     /* record different mode's startx of grabwindow */                    \
        .starty = 0,                     /* record different mode's starty of grabwindow */                    \
        .grabwindow_width = 2016,        /* Dual PD: need to tg grab width / 2, p1 drv will * 2 itself */      \
        .grabwindow_height = 1512,       /* record different mode's height of grabwindow */                    \
        .mipi_data_lp2hs_settle_dc = 85, /* unit , ns */                                                       \
        .max_framerate = 300                                                                                   \
                                                                                                               \
    },                                                                                                         \
    .cap = {                                                                                                   \
        .pclk = 960000000,               /* record different mode's pclk */                                    \
        .linelength = 10208,             /* record different mode's linelength */                              \
        .framelength =3134,              /* record different mode's framelength */                             \
        .startx = 0,                     /* record different mode's startx of grabwindow */                    \
        .starty = 0,                     /* record different mode's starty of grabwindow */                    \
        .grabwindow_width = 4032,        /* Dual PD: need to tg grab width / 2, p1 drv will * 2 itself */      \
        .grabwindow_height = 3024,       /* record different mode's height of grabwindow */                    \
        .mipi_data_lp2hs_settle_dc = 85, /* unit , ns */                                                       \
        .max_framerate = 300                                                                                   \
                                                                                                               \
    },                                                                                                         \
    .cap1 = {                                                                                                  \
        .pclk = 960000000,               /* record different mode's pclk  */                                   \
        .linelength = 10208,             /* record different mode's linelength  */                             \
        .framelength =3134,              /* record different mode's framelength */                             \
        .startx = 0,                     /* record different mode's startx of grabwindow */                    \
        .starty = 0,                     /* record different mode's starty of grabwindow */                    \
        .grabwindow_width = 4032,        /* Dual PD: need to tg grab width / 2, p1 drv will * 2 itself */      \
        .grabwindow_height = 3024,       /* record different mode's height of grabwindow */                    \
        .mipi_data_lp2hs_settle_dc = 85, /* unit , ns */                                                       \
        .max_framerate = 300                                                                                   \
    },                                                                                                         \
                                                                                                               \
    .normal_video = {                                                                                          \
                                                                                                               \
        .pclk = 960000000,                /* record different mode's pclk  */                                  \
        .linelength = 10208,              /* record different mode's linelength */                             \
        .framelength =3134,               /* record different mode's framelength */                            \
        .startx = 0,                      /* record different mode's startx of grabwindow */                   \
        .starty = 0,                      /* record different mode's starty of grabwindow */                   \
        .grabwindow_width = 4032,         /* Dual PD: need to tg grab width / 2, p1 drv will * 2 itself */     \
        .grabwindow_height = 3024,        /* record different mode's height of grabwindow */                   \
        .mipi_data_lp2hs_settle_dc = 85,  /* unit , ns */                                                      \
        .max_framerate = 300                                                                                   \
                                                                                                               \
    },                                                                                                         \
                                                                                                               \
    .hs_video = {                                                                                              \
        .pclk = 960000000,                                                                                     \
        .linelength = 10160,                                                                                   \
        .framelength = 1049,                                                                                   \
        .startx = 0,                                                                                           \
        .starty = 0,                                                                                           \
        .grabwindow_width = 1344,        /* record different mode's width of grabwindow */                     \
        .grabwindow_height = 756,        /* record different mode's height of grabwindow */                    \
        .mipi_data_lp2hs_settle_dc = 85, /* unit , ns */                                                       \
        .max_framerate = 900,                                                                                  \
    },                                                                                                         \
                                                                                                               \
    .slim_video = {                                                                                            \
        .pclk = 960000000,                                                                                     \
        .linelength = 10160,                                                                                   \
        .framelength = 3149,                                                                                   \
        .startx = 0,                                                                                           \
        .starty = 0,                                                                                           \
        .grabwindow_width = 1344,        /* record different mode's width of grabwindow */                     \
        .grabwindow_height = 756,        /* record different mode's height of grabwindow */                    \
        .mipi_data_lp2hs_settle_dc = 85, /* unit , ns */                                                       \
        .max_framerate = 300,                                                                                  \
                                                                                                               \
    },                                                                                                         \
    .margin = 16,                                                                                              \
    .min_shutter = 1,                                                                                          \
    .max_frame_length = 0xffff,                                                                                \
    .ae_shut_delay_frame = 0,                                                                                  \
    .ae_sensor_gain_delay_frame = 0,                                                                           \
    .ae_ispGain_delay_frame = 2,                                                                               \
    .ihdr_support = 0,       /* 1, support; 0,not support */                                                   \
    .ihdr_le_firstline = 0,  /* 1,le first ; 0, se first */                                                    \
    .sensor_mode_num = 5,    /* support sensor mode num */                                                     \
    .cap_delay_frame = 3,                                                                                      \
    .pre_delay_frame = 3,                                                                                      \
    .video_delay_frame = 3,                                                                                    \
    .hs_video_delay_frame = 3,                                                                                 \
    .slim_video_delay_frame = 3,                                                                               \
    .isp_driving_current = ISP_DRIVING_8MA,                                                                    \
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,                                                       \
    .mipi_sensor_type = MIPI_OPHY_NCSI2,             /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */                \
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO, /* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */ \
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,                                                   \
    .mclk = 24,                                                                                                \
    .mipi_lane_num = SENSOR_MIPI_4_LANE,                                                                       \
    .i2c_addr_table = { 0x5A, 0x20, 0xFF},                                                                     \
    .i2c_speed = 300,                                                                                          \
};

/* full_w; full_h; x0_offset; y0_offset; w0_size; h0_size; scale_w; scale_h; x1_offset;  y1_offset;  w1_size;  h1_size;
 	 x2_tg_offset;	 y2_tg_offset;	w2_tg_size;  h2_tg_size;*/
#define _S5K2L7_MODE2_WINSIZE_INFO_ static SENSOR_WINSIZE_INFO_STRUCT _imgsensor_winsize_info_m2[5] =     \
{                                                                                                         \
    { 4032, 3024, 0,   0, 4032, 3024, 2016, 1512, 0, 0, 2016, 1512, 0, 0, 2016, 1512}, /* Preview */      \
    { 4032, 3024, 0,   0, 4032, 3024, 4032, 3024, 0, 0, 4032, 3024, 0, 0, 4032, 3024}, /* capture */      \
    { 4032, 3024, 0,   0, 4032, 3024, 4032, 3024, 0, 0, 4032, 3024, 0, 0, 4032, 3024}, /* normal_video */ \
    { 4032, 3024, 0, 348, 4032, 2328, 1344,  756, 0, 0, 1344,  756, 0, 0, 1344,  756}, /* hs_video */     \
    { 4032, 3024, 0, 348, 4032, 2328, 1344,  756, 0, 0, 1336,  756, 0, 0, 1344,  756}, /* slim_video */   \
};

#define _SET_MODE2_SENSOR_INFO_AND_WINSIZE_ do{ \
    memcpy((void *)&imgsensor_info, (void *)&_imgsensor_info_m2, sizeof(imgsensor_info_struct)); \
    memcpy((void *)&imgsensor_winsize_info, (void *)&_imgsensor_winsize_info_m2, sizeof(SENSOR_WINSIZE_INFO_STRUCT)*5); \
}while(0)


/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 2 initial setting
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_INIT_ do{ \
}while(0)


/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 2 preview setting
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_PREVIEW_ do{ \
}while(0)


/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 1 capture setting (M1_fullsize_setting)
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_CAPTURE_ do{ \
}while(0)


/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 2 high speed video setting
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_HS_VIDEO_ do{ \
}while(0)

/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 2 slim video setting
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_SLIM_VIDEO_ do{ \
}while(0)

/*****************************************************************************
 *
 * Description:
 * ------------
 *     mode 2 cpature with WDR setting
 *
 ****************************************************************************/
#define _S5K2L7_MODE2_CAPTURE_WDR_ do{ \
}while(0)

#endif
