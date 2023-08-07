/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 mot_ov50a2q_Sensor_setting.h
 *
 * Project:
 * --------
 * Description:
 * ------------
 *	 CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _MOT_OV50A2Q_SENSOR_SETTING_H
#define _MOT_OV50A2Q_SENSOR_SETTING_H

#define CPHY_3TRIO
#define SEAMLESS_SWITCH_V2
#define ROTATION_180

kal_uint16 addr_data_pair_xtalk_mot_ov50a2q[] = {

};

kal_uint16 addr_data_pair_init_mot_ov50a2q[] = {
#include "setting/OV50A40_Initial_CPHY_3Trio_1924Msps_20230605_remove_xtalk_data.h"
};

kal_uint16 addr_data_pair_init_mot_ov50a_20230307[] = {
#include "setting/OV50A40_Initial_CPHY_3Trio_1924Msps_20230605.h"
};

kal_uint16 addr_data_pair_preview_mot_ov50a2q[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_capture_mot_ov50a2q[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_video_mot_ov50a2q[] = {
#include"setting/OV50A40_4096x2304_4C2SCG_10bit_30fps_AG255_PDVC1_4096x576_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_hs_video_mot_ov50a2q[] = {
#include"setting/OV50A40_2048x1152_4C1SCG_10bit_120fps_AG255_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_slim_video_mot_ov50a2q[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_custom1[] = {
#include"setting/OV50A40_8192x6144_10bit_30fps_AG16_2600Msps_20230531.h"
};

kal_uint16 addr_data_pair_custom2[] = {
#include"setting/OV50A40_4096x2304_4C2SCG__10bit_60fps_AG64_PDVC2_4096x576_1924Msps_20230807.h"
};
kal_uint16 addr_data_pair_custom3[] = {
#include"setting/OV50A40_1920x1080_4C1SCG_10bit_240fps_AG64_1924Msps_20230531.h"
};
kal_uint16 addr_data_pair_custom4[] = {
#include"setting/OV50A40_4096x2304_4C2SCG_STG2_10bit_30fps_AG64_PDVC2_4096x576_1924Msps_20230531.h"
}; //sHDR
kal_uint16 addr_data_pair_custom5[] = {
#include"setting/OV50A40_4096x3072_Cropping_10bit_30fps_AG64_PDVC1_2048x768LR_1924Msps_20230531.h"
};

kal_uint16 addr_data_pair_custom6[] = {
#include"setting/OV50A40_2048x1536_4C2SCG_10bit_30fps_AG255_PDVC1_2048x768_1924Msps_20230531.h"
}; //3rd video call


kal_uint16 addr_data_pair_custom7[] = {
#include"setting/OV50A40_4096x3072_4C2SCG_10bit_30fps_AG255_PDVC1_4096x768_1924Msps_20230531_DPCoff.h"
}; //disbale dpc

#ifdef SEAMLESS_SWITCH_V2
kal_uint16 addr_data_pair_seamless_switch_step1_mot_ov50a2q[] = {
	//group 0
	0x3208, 0x00,
	0x3016, 0xf1,
	//new mode settings
};

kal_uint16 addr_data_pair_seamless_switch_step2_mot_ov50a2q[] = {
	0x3016, 0xf0,
	0x3208, 0x10,
	//group 1
	0x3208, 0x01,
	//0x3016, 0xf1,
	//new gain for 2nd frame
};
kal_uint16 addr_data_pair_seamless_switch_step3_mot_ov50a2q[] = {
	//0x3016, 0xf0,
	0x3208, 0x11,

	//group 2 - intend to be blank
	0x3208, 0x02,
	0x3208, 0x12,

	//group 3 - intend to be blank
	0x3208, 0x03,
	0x3208, 0x13,

	//switch
	0x3689, 0x37,
	0x320d, 0x07,
	0x3209, 0x01,
	0x320a, 0x01,
	0x320b, 0x01,
	0x320c, 0x01,
	0x320e, 0xa0,
};
#else
kal_uint16 addr_data_pair_seamless_switch_step1_mot_ov50a2q[] = {
	//group 0 intend to be blank
	0x3208, 0x00,
	0x3208, 0x10,
	//group 1
	0x3208, 0x01,
	0x3016, 0xf1,
	//new mode settings
};

kal_uint16 addr_data_pair_seamless_switch_step2_mot_ov50a2q[] = {
	0x3016, 0xf0,
	0x3208, 0x11,
	0x3208, 0x02,
	//0x3016, 0xf1,
	//new gain for 2nd frame
};
kal_uint16 addr_data_pair_seamless_switch_step3_mot_ov50a2q[] = {
	//0x3016, 0xf0,
	0x3208, 0x12,

	//group 3 - intend to be blank
	0x3208, 0x03,
	0x3208, 0x13,

	//switch
	0x3689, 0x37,
	0x320d, 0x07,
	0x3209, 0x01,
	0x320a, 0x01,
	0x320b, 0x01,
	0x320c, 0x01,
	0x320e, 0xa0,
};
#endif
#endif
