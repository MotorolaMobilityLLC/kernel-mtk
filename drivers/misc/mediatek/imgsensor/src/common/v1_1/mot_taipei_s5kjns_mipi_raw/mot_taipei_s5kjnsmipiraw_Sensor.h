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
 *     mot_taipei_s5kjnsmipi_Sensor.h
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _MOT_TAIPEI_S5KJNS_SENSOR_H
#define _MOT_TAIPEI_S5KJNS_SENSOR_H
#include "imgsensor_sensor.h"

enum IMGSENSOR_MODE {
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HIGH_SPEED_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
	IMGSENSOR_MODE_CUSTOM1,
	IMGSENSOR_MODE_CUSTOM2,
	IMGSENSOR_MODE_CUSTOM3,
	IMGSENSOR_MODE_CUSTOM4,
};

enum {
	OTP_QSC_NONE = 0x0,
	OTP_QSC_INTERNAL,
	OTP_QSC_CUSTOM,
};

struct imgsensor_mode_struct {
	kal_uint32 pclk; /* record different mode's pclk */
	kal_uint32 linelength; /* record different mode's linelength */
	kal_uint32 framelength; /* record different mode's framelength */

	kal_uint8 startx; /* record different mode's startx of grabwindow */
	kal_uint8 starty; /* record different mode's startx of grabwindow */

	kal_uint16 grabwindow_width;
	kal_uint16 grabwindow_height;

/* for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario*/
	kal_uint8 mipi_data_lp2hs_settle_dc;

 /*     following for GetDefaultFramerateByScenario()    */
	kal_uint16 max_framerate;
	kal_uint32 mipi_pixel_rate;
};

/* SENSOR PRIVATE STRUCT FOR VARIABLES*/
struct imgsensor_struct {
	kal_uint8 mirror; /* mirrorflip information */

	kal_uint8 sensor_mode; /* record IMGSENSOR_MODE enum value */

	kal_uint32 shutter; /* current shutter */

	kal_uint16 gain; /* current gain */

	kal_uint32 pclk; /* current pclk */

	kal_uint32 frame_length; /* current framelength */
	kal_uint32 line_length; /* current linelength */

	kal_uint32 min_frame_length;
	kal_uint16 dummy_pixel; /* current dummypixel */
	kal_uint16 dummy_line; /* current dummline */

	kal_uint16 current_fps; /* current max fps */
	kal_bool autoflicker_en; /* record autoflicker enable or disable */
	kal_bool test_pattern; /* record test pattern mode or not */
	enum MSDK_SCENARIO_ID_ENUM current_scenario_id;
	kal_bool ihdr_en; /* ihdr enable or disable */
	kal_uint8 ihdr_mode; /* ihdr enable or disable */
	kal_uint8 pdaf_mode; /* ihdr enable or disable */
	struct IMGSENSOR_AE_FRM_MODE ae_frm_mode;
	kal_uint8 current_ae_effective_frame;
	kal_uint8 i2c_write_id; /* record current sensor's i2c write id */
	struct SENSOR_FUNCTION_STRUCT *psensor_func;
};

/* SENSOR PRIVATE STRUCT FOR CONSTANT*/
struct imgsensor_info_struct {
	kal_uint16 sensor_id;			//record sensor id defined in Kd_imgsensor.h
	kal_uint32 checksum_value; /* checksum value for Camera Auto Test */
	struct imgsensor_mode_struct pre;
	struct imgsensor_mode_struct cap;
	struct imgsensor_mode_struct normal_video;
	struct imgsensor_mode_struct hs_video;
	struct imgsensor_mode_struct slim_video;
	struct imgsensor_mode_struct custom1;
	struct imgsensor_mode_struct custom2;
	struct imgsensor_mode_struct custom3;
	struct imgsensor_mode_struct custom4;

	kal_uint8 ae_shut_delay_frame; /* shutter delay frame for AE cycle */
	kal_uint8 ae_sensor_gain_delay_frame;
	kal_uint8 ae_ispGain_delay_frame;
	kal_uint8 ihdr_support; /* 1, support; 0,not support */
	kal_uint8 ihdr_le_firstline; /* 1,le first ; 0, se first */
	kal_uint8 temperature_support;	/* 1, support; 0,not support */
	kal_uint8 sensor_mode_num; /* support sensor mode num */

	kal_uint8 cap_delay_frame; /* enter capture delay frame num */
	kal_uint8 pre_delay_frame; /* enter preview delay frame num */
	kal_uint8 video_delay_frame; /* enter video delay frame num */
	kal_uint8 hs_video_delay_frame;
	kal_uint8 slim_video_delay_frame; /* enter slim video delay frame num */
	kal_uint8 custom1_delay_frame; /* enter custom1 delay frame num */
	kal_uint8 custom2_delay_frame; /* enter custom2 delay frame num */
	kal_uint8 custom3_delay_frame; /* enter custom3 delay frame num */
	kal_uint8 custom4_delay_frame; /* enter custom4 delay frame num */
	kal_uint8  frame_time_delay_frame;
	kal_uint8 margin; /* sensor framelength & shutter margin */
	kal_uint32 min_shutter; /* min shutter */
	kal_uint32 max_frame_length;
	kal_uint32 min_gain;
	kal_uint32 max_gain;
	kal_uint32 max_gain_slim;
	kal_uint32 max_gain_60fps;
	kal_uint32 max_gain_120fps;
	kal_uint32 min_gain_iso;
	kal_uint32 gain_step;
	kal_uint32 exp_step;
	kal_uint32 gain_type;
	kal_uint8 isp_driving_current; /* mclk driving current */
	kal_uint8 sensor_interface_type; /* sensor_interface_type */
	kal_uint8 mipi_sensor_type;
	/* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2, default is NCSI2,
	 * don't modify this para
	 */
	kal_uint8 mipi_settle_delay_mode;
	/* 0, high speed signal auto detect;
	 * 1, use settle delay,unit is ns,
	 * default is auto detect, don't modify this para
	 */
	kal_uint8 sensor_output_dataformat;
	kal_uint8 mclk;	 /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	kal_uint32 i2c_speed; /* i2c speed */
	kal_uint8 mipi_lane_num; /* mipi lane num */
	kal_uint8 i2c_addr_table[5];
};

#define AWB_R_MIN 200
#define AWB_R_MAX 880
#define AWB_GR_MIN 760
#define AWB_GR_MAX 880
#define AWB_GB_MIN 760
#define AWB_GB_MAX 880
#define AWB_B_MIN 200
#define AWB_B_MAX 880

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

struct TAIPEI_S5KJNS_eeprom_t{
	uint8_t eeprom_table_version[1];
	uint8_t cal_hw_ver[1];
	uint8_t cal_sw_ver[1];
	uint8_t mpn[8];
	uint8_t actuator_id[1];
	uint8_t lens_id[1];
	uint8_t manufacturer_id[2];
	uint8_t factory_id[2];
	uint8_t manufacture_line[1];
	uint8_t manufacture_date[3];
	uint8_t serial_number[16];
	uint8_t manufacture_crc16[2];
	//AFC
	uint8_t af_data[24];
	uint8_t af_crc16[2];
	//WBC
	uint8_t cie_src_1_ev[2];
	uint8_t cie_src_1_u[2];
	uint8_t cie_src_1_v[2];
	uint8_t awb_r_g_golden_min_limit[1];
	uint8_t awb_r_g_golden_max_limit[1];
	uint8_t awb_b_g_golden_min_limit[1];
	uint8_t awb_b_g_golden_max_limit[1];
	uint8_t awb_src_1_golden_r[2];
	uint8_t awb_src_1_golden_gr[2];
	uint8_t awb_src_1_golden_gb[2];
	uint8_t awb_src_1_golden_b[2];
	uint8_t awb_src_1_golden_rg_ratio[2];
	uint8_t awb_src_1_golden_bg_ratio[2];
	uint8_t awb_src_1_golden_gr_gb_ratio[2];
	uint8_t awb_src_1_r[2];
	uint8_t awb_src_1_gr[2];
	uint8_t awb_src_1_gb[2];
	uint8_t awb_src_1_b[2];
	uint8_t awb_src_1_rg_ratio[2];
	uint8_t awb_src_1_bg_ratio[2];
	uint8_t awb_src_1_gr_gb_ratio[2];
	uint8_t awb_reserve[5];
	uint8_t awb_crc16[2];
	//OC
	uint8_t rgb_optical_center_src_1_X_r[2];
	uint8_t rgb_optical_center_src_1_Y_r[2];
	uint8_t rgb_optical_center_src_1_X_gr[2];
	uint8_t rgb_optical_center_src_1_Y_gr[2];
	uint8_t rgb_optical_center_src_1_X_gb[2];
	uint8_t rgb_optical_center_src_1_Y_gb[2];
	uint8_t rgb_optical_center_src_1_X_b[2];
	uint8_t rgb_optical_center_src_1_Y_b[2];
	uint8_t rgb_oc_crc16[2];
	//SFR
	uint8_t sfr_data[117];
	uint8_t sfr_crc16[2];
	//QCOM LSC
	uint8_t lsc_data_qcom[1768];
	uint8_t lsc_crc16_qcom[2];
	//Qcom PDAF LRC(Gain map)
	uint8_t dcc_cal[3];
	uint8_t gain_map[890];
	uint8_t pdaf_lrc_crc[2];
	//QCOM PDAF DCC
	uint8_t pdaf_dcc[102];
	uint8_t dcc_crc[2];
	//QCOM PDAF OFFSET
	uint8_t pdaf_offset[198];
	uint8_t pdaf_offset_crc[2];
	//MTK LSC
	uint8_t lsc_data_mtk[1868];
	uint8_t lsc_crc16_mtk[2];
	//MTK PDAF
	uint8_t pdaf_out1_data_mtk[496];
	uint8_t pdaf_out2_data_mtk[1004];
	uint8_t pdaf_out1_crc16_mtk[2];
	uint8_t pdaf_out2_crc16_mtk[2];
	//AF Sync
	uint8_t af_sync_data[24];
	uint8_t af_sync_crc[2];
	//AF Posture
	uint8_t necessary_data[19];
	uint8_t necessary_crc[2];
	//HW_GCC
	uint8_t hw_ggc_data[346];
	uint8_t xtc_data_crc[2];
};

typedef struct {
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
	uint16_t r_g;
	uint16_t b_g;
	uint16_t gr_gb;
} awb_t;

typedef struct {
	uint8_t r_g_golden_min;
	uint8_t r_g_golden_max;
	uint8_t b_g_golden_min;
	uint8_t b_g_golden_max;
} awb_limit_t;

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);

int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId,
	u16 transfer_length, u16 timing);

extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
#endif
