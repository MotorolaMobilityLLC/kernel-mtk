/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 mot_lyriq_ov50a_Sensor_setting.h
 *
 * Project:
 * --------
 * Description:
 * ------------
 *	 CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _MOT_LYRIQ_OV50A_SENSOR_SETTING_H
#define _MOT_LYRIQ_OV50A_SENSOR_SETTING_H

kal_uint16 addr_data_pair_init_mot_lyriq_ov50a[] = {
#include "setting/OV50A40_Initial_CPHY_3Trio_1924Msps_20220714.h"
};

/* Binning 4096*3072@30fps*/
kal_uint16 addr_data_pair_preview_mot_lyriq_ov50a[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20220705.h"
};

kal_uint16 addr_data_pair_capture_mot_lyriq_ov50a[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20220705.h"
};

kal_uint16 addr_data_pair_video_mot_lyriq_ov50a[] = {
#include"setting/OV50A40_4096x2304_4C2SCG_10bit_30fps_AG255_PDVC1_4096x576_1924Msps_20220727.h"
};

kal_uint16 addr_data_pair_slim_video_mot_lyriq_ov50a[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20220705.h"
};

/* full 8192*6144@20fps*/
kal_uint16 addr_data_pair_fullsize_ov50a[] = {
#include"setting/OV50A40_8192x6144_10bit_30fps_AG16_2600Msps_20220711.h"
};

//hs_video
kal_uint16 addr_data_pair_60fps_ov50a_20220728[] = {
#include"setting/OV50A40_4096x3072_4C2FastSpeed_10bit_60fps_AG16_PDVC1_4096x768_1924Msps_20220705.h"
};
kal_uint16 addr_data_pair_60fps_ov50a_20221103[] = {
#include"setting/OV50A40_2048x1536_4C1SCG_10bit_60fps_AG255_1924Msps_20221103.h"
};
kal_uint16 addr_data_pair_60fps_ov50a[] = {
#include"setting/OV50A40_2048x1536_4C2SCG_10bit_60fps_AG255_PDVC1_2048x768_1924Msps_20221208.h"
};

//custom2
kal_uint16 addr_data_pair_120fpsov50a[] = {
#include"setting/OV50A40_2048x1536_4C1SCG_10bit_120fps_AG255_1924Msps_20220711.h"
};

//custom3
kal_uint16 addr_data_pair_custom3[] = {
#include"setting/OV50A40_1920x1080_4C1SCG_10bit_240fps_AG64_1924Msps_20220705.h"
};

//custom4
kal_uint16 addr_data_pair_custom4[] = {
#include"setting/OV50A40_4096x2304_4C2SCG_STG2_10bit_30fps_AG64_PDVC2_4096x576_1924Msps_20220705.h"
};

//custom5
kal_uint16 addr_data_pair_custom5[] = {
#include"setting/OV50A40_4096x3072_Cropping_10bit_30fps_AG64_1924Msps_20221121.h"
};

kal_uint16 addr_data_pair_seamless_switch_group1_start[] = {
//group 1
0x3208,0x01,
0x3016,0xf3,
0x3017,0xf2,
0x301f,0x9b,
0x382e,0x49,
};

kal_uint16 addr_data_pair_seamless_switch_group1_end[] = {
//group 1
0x383f,0x08,
0x382a,0x80,
0x301f,0x98,
0x3017,0xf0,
0x3016,0xf0,
0x3208,0x11,
0x382e,0x40,
0x383f,0x00,
0x3208,0xa1,
};

kal_uint16 addr_data_pair_seamless_switch_group2_start[] = {
//group 2
0x3208,0x02,
0x3016,0xf3,
0x3017,0xf2,
0x301f,0x9b,
0x382e,0x49,
};

kal_uint16 addr_data_pair_seamless_switch_group2_end[] = {
//group 2
0x383f,0x08,
0x382a,0x80,
0x301f,0x98,
0x3017,0xf0,
0x3016,0xf0,
0x3208,0x12,
0x3208,0xa2,
};

kal_uint16 addr_data_pair_seamless_switch_group3_start[] = {
//group 3
0x3208,0x01,
0x3016,0xf3,
0x3017,0xf2,
0x301f,0x9b,
};

kal_uint16 addr_data_pair_seamless_switch_group3_end[] = {
//group 3
//0x383f,0x08,
//0x382a,0x80,
0x301f,0x98,
0x3017,0xf0,
0x3016,0xf0,
0x3208,0x11,
0x3208,0xa1,
};

#endif
