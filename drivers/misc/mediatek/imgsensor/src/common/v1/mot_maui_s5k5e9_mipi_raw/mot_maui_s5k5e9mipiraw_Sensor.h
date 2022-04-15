/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K5E9mipi_Sensor.h
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _S5K5E9MIPI_SENSOR_H
#define _S5K5E9MIPI_SENSOR_H


typedef enum{
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HS_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
} IMGSENSOR_MODE;

typedef struct imgsensor_mode_struct {
	kal_uint32 pclk;				/* record different mode's pclk */
	kal_uint32 linelength;			/* record different mode's linelength */
	kal_uint32 framelength;			/* record different mode's framelength */

	kal_uint8 startx;				/* record different mode's startx of grabwindow */
	kal_uint8 starty;				/* record different mode's startx of grabwindow */

	kal_uint16 grabwindow_width;	/* record different mode's width of grabwindow */
	kal_uint16 grabwindow_height;	/* record different mode's height of grabwindow */

	/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
	kal_uint8 mipi_data_lp2hs_settle_dc;

	/*	 following for GetDefaultFramerateByScenario()	*/
	kal_uint16 max_framerate;
	kal_uint32 mipi_pixel_rate;

} imgsensor_mode_struct;

/* SENSOR PRIVATE STRUCT FOR VARIABLES*/
typedef struct imgsensor_struct {
	kal_uint8 mirror;				/* mirrorflip information */

	kal_uint8 sensor_mode;			/* record IMGSENSOR_MODE enum value */

	kal_uint32 shutter;				/* current shutter */
	kal_uint16 gain;				/* current gain */

	kal_uint32 pclk;				/* current pclk */

	kal_uint32 frame_length;		/* current framelength */
	kal_uint32 line_length;			/* current linelength */

	kal_uint32 min_frame_length;	/* current min  framelength to max framerate */
	kal_uint16 dummy_pixel;			/* current dummypixel */
	kal_uint16 dummy_line;			/* current dummline */

	kal_uint16 current_fps;			/* current max fps */
	kal_bool   autoflicker_en;		/* record autoflicker enable or disable */
	kal_bool test_pattern;			/* record test pattern mode or not */
	enum MSDK_SCENARIO_ID_ENUM current_scenario_id;/* current scenario id */
	kal_uint8  ihdr_en;				/* ihdr enable or disable */

	kal_uint8 i2c_write_id;			/* record current sensor's i2c write id */
	struct IMGSENSOR_AE_FRM_MODE ae_frm_mode;
	kal_uint32 current_ae_effective_frame;
    kal_uint8 update_sensor_otp_awb;          /* Update sensor awb from otp or not */
    kal_uint8 update_sensor_otp_lsc;          /* Update sensor lsc from otp or not */
} imgsensor_struct;

/* SENSOR PRIVATE STRUCT FOR CONSTANT*/
typedef struct imgsensor_info_struct {
	kal_uint32 sensor_id;			/* record sensor id defined in Kd_imgsensor.h */
	kal_uint32 checksum_value;		/* checksum value for Camera Auto Test */
	imgsensor_mode_struct pre;		/* preview scenario relative information */
	imgsensor_mode_struct cap;		/* capture scenario relative information */
	imgsensor_mode_struct normal_video;/* normal video  scenario relative information */
	imgsensor_mode_struct hs_video;
	imgsensor_mode_struct slim_video;	/* slim video for VT scenario relative information */


	kal_uint8  ae_shut_delay_frame;	/* shutter delay frame for AE cycle */
	kal_uint8  ae_sensor_gain_delay_frame;	/* sensor gain delay frame for AE cycle */
	kal_uint8  ae_ispGain_delay_frame;	/* isp gain delay frame for AE cycle */
	kal_uint8  ihdr_support;		/* 1, support; 0,not support */
	kal_uint8  ihdr_le_firstline;	/* 1,le first ; 0, se first */
	kal_uint8  sensor_mode_num;		/* support sensor mode num */

	kal_uint8  cap_delay_frame;		/* enter capture delay frame num */
	kal_uint8  pre_delay_frame;		/* enter preview delay frame num */
	kal_uint8  video_delay_frame;	/* enter video delay frame num */
	kal_uint8  hs_video_delay_frame;	/* enter high speed video  delay frame num */
	kal_uint8  slim_video_delay_frame;	/* enter slim video delay frame num */


	kal_uint8  margin;				/* sensor framelength & shutter margin */
	kal_uint32 min_shutter;			/* min shutter */
	kal_uint32 max_frame_length;	/* max framelength by sensor register's limitation */

	kal_uint8  isp_driving_current;	/* mclk driving current */
	kal_uint8  sensor_interface_type;/* sensor_interface_type */
	kal_uint8  mipi_sensor_type; /* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2, default is NCSI2, don't modify this para */
	kal_uint8  mipi_settle_delay_mode; /* 0, high speed signal auto detect; 1, use settle delay,unit is ns, default is auto detect, don't modify this para */
	kal_uint8  sensor_output_dataformat;/* sensor output first pixel color */
	kal_uint8  mclk;				/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */

	kal_uint8  mipi_lane_num;		/* mipi lane num */
	kal_uint8  i2c_addr_table[5];	/* record sensor support all write id addr, only supprt 4must end with 0xff */
	kal_uint32 i2c_speed;
	kal_uint32 min_gain;
	kal_uint32 max_gain;
	kal_uint32 min_gain_iso;
	kal_uint32 gain_step;
	kal_uint32 gain_type;

} imgsensor_info_struct;

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

#define AWB_R_MIN 200
#define AWB_R_MAX 880
#define AWB_GR_MIN 760
#define AWB_GR_MAX 880
#define AWB_GB_MIN 760
#define AWB_GB_MAX 880
#define AWB_B_MIN 200
#define AWB_B_MAX 880

struct mot_s5k5e9_eeprom_t {
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

	uint8_t af_reserve[26];
	uint8_t cie_ev[2];
	uint8_t cie_u[2];
	uint8_t cie_v[2];
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

	uint8_t optical_center_x_1_r_channel[2];
	uint8_t optical_center_y_1_r_channel[2];
	uint8_t optical_center_x_1_gr_channel[2];
	uint8_t optical_center_y_1_gr_channel[2];
	uint8_t optical_center_x_1_gb_channel[2];
	uint8_t optical_center_y_1_gb_channel[2];
	uint8_t optical_center_x_1_b_channel[2];
	uint8_t optical_center_y_1_b_channel[2];
	uint8_t optical_center_crc16[2];

	uint8_t sfr_distance1_reserve[3];
	uint8_t sfr_ctr_distance1[2];
	uint8_t sfr_field1_upper_left_distance1[2];
	uint8_t sfr_field1_upper_right_distance1[2];
	uint8_t sfr_field1_lower_right_distance1[2];
	uint8_t sfr_field1_lower_left_distance1[2];
	uint8_t sfr_field2_upper_left_distance1[2];
	uint8_t sfr_field2_upper_right_distance1[2];
	uint8_t sfr_field2_lower_right_distance1[2];
	uint8_t sfr_field2_lower_left_distance1[2];
	uint8_t sfr_field3_upper_left_distance1[2];
	uint8_t sfr_field3_upper_right_distance1[2];
	uint8_t sfr_field3_lower_right_distance1[2];
	uint8_t sfr_field3_lower_left_distance1[2];
	uint8_t sfr_field__distance1_reserve[8];
	uint8_t sfr_field2_corner_to_corner_variation_distance1[2];
	uint8_t sfr_distance2_reserve[3];
	uint8_t sfr_ctr_distance2[2];
	uint8_t sfr_field1_upper_left_distance2[2];
	uint8_t sfr_field1_upper_right_distance2[2];
	uint8_t sfr_field1_lower_right_distance2[2];
	uint8_t sfr_field1_lower_left_distance2[2];
	uint8_t sfr_field2_upper_left_distance2[2];
	uint8_t sfr_field2_upper_right_distance2[2];
	uint8_t sfr_field2_lower_right_distance2[2];
	uint8_t sfr_field2_lower_left_distance2[2];
	uint8_t sfr_field3_upper_left_distance2[2];
	uint8_t sfr_field3_upper_right_distance2[2];
	uint8_t sfr_field3_lower_right_distance2[2];
	uint8_t sfr_field3_lower_left_distance2[2];
	uint8_t sfr_field_distance2_reserve[8];
	uint8_t sfr_field2_corner_to_corner_variation_distance2_reserve[2];
	uint8_t sfr_distance3_reserve[3];
	uint8_t sfr_ctr_distance3[2];
	uint8_t sfr_field1_upper_left_distance3[2];
	uint8_t sfr_field1_upper_right_distance3[2];
	uint8_t sfr_field1_lower_right_distance3[2];
	uint8_t sfr_field1_lower_left_distance3[2];
	uint8_t sfr_field2_upper_left_distance3[2];
	uint8_t sfr_field2_upper_right_distance3[2];
	uint8_t sfr_field2_lower_right_distance3[2];
	uint8_t sfr_field2_lower_left_distance3[2];
	uint8_t sfr_field3_upper_left_distance3[2];
	uint8_t sfr_field3_upper_right_distance3[2];
	uint8_t sfr_field3_lower_right_distance3[2];
	uint8_t sfr_field3_lower_left_distance3[2];
	uint8_t sfr_field_distance3_reserve[8];
	uint8_t sfr_field2_corner_to_corner_variation_distance3_reserve[2];
	uint8_t sfr_crc16[2];

	uint8_t lsc_calibration[1868];
	uint8_t lsc_crc16[2];

	uint8_t module_check_flag[1];
	uint8_t otp_calibration_type[4];
	uint8_t sensor_type[1];
	uint8_t awb_af_calibration_information[1];
	uint8_t one_table_size[2];
	uint8_t posture_af_infinite_reserve[2];
	uint8_t posture_af_macro_reserve[2];
	uint8_t new_mtk_format_version[1];
	uint8_t new_mtk_format_flag[3];
	uint8_t af_temperature_reserve[1];
	uint8_t mtk_necessary_crc16[2];
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

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
int s5k5e9_get_cal_info(mot_calibration_info_t * pOtpCalInfo);
#endif
