/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k3l6qtmipiraw_Sensor.h
 *
 ****************************************************************************/
#ifndef _S5K3L6QTMIPI_SENSOR_H
#define _S5K3L6QTMIPI_SENSOR_H
#include <linux/types.h>
#include "kd_camera_typedef.h"



//表示sensor的几种工作模式状态：init preview capture video hvideo svideo
typedef enum{
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HIGH_SPEED_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
} IMGSENSOR_MODE;

//表示几种（不同工作模式状态下）的sensor参数信息
typedef struct imgsensor_mode_struct {
	kal_uint32 pclk;				//record different mode's pclk
	kal_uint32 linelength;			//record different mode's linelength
	kal_uint32 framelength;			//record different mode's framelength

	kal_uint8 startx;				//record different mode's startx of grabwindow
	kal_uint8 starty;				//record different mode's startx of grabwindow

	kal_uint16 grabwindow_width;	//record different mode's width of grabwindow
	kal_uint16 grabwindow_height;	//record different mode's height of grabwindow

	/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
	kal_uint8 mipi_data_lp2hs_settle_dc;

	/*	 following for GetDefaultFramerateByScenario()	*/
	kal_uint16 max_framerate;

} imgsensor_mode_struct;

//表示（当前状态工作模式）下的sensor参数信息
/* SENSOR PRIVATE STRUCT FOR VARIABLES*/
typedef struct imgsensor_struct {
	kal_uint8 mirror;				//mirrorflip information

	kal_uint8 sensor_mode;			//record IMGSENSOR_MODE enum value

	kal_uint32 shutter;				//current shutter
	kal_uint16 gain;				//current gain

	kal_uint32 pclk;				//current pclk

	kal_uint32 frame_length;		//current framelength
	kal_uint32 line_length;			//current linelength

	kal_uint32 min_frame_length;	//current min  framelength to max framerate
	kal_uint16 dummy_pixel;			//current dummypixel
	kal_uint16 dummy_line;			//current dummline

	kal_uint16 current_fps;			//current max fps
	kal_bool   autoflicker_en;		//record autoflicker enable or disable
	kal_bool test_pattern;			//record test pattern mode or not
	MSDK_SCENARIO_ID_ENUM current_scenario_id;//current scenario id
	kal_bool  ihdr_en;				//ihdr enable or disable

	kal_uint8 i2c_write_id;			//record current sensor's i2c write id
} imgsensor_struct;

//sensor基本信息，datasheet上的信息
/* SENSOR PRIVATE STRUCT FOR CONSTANT*/
typedef struct imgsensor_info_struct {
	kal_uint16 sensor_id;			//record sensor id defined in Kd_imgsensor.h
	kal_uint32 checksum_value;		//checksum value for Camera Auto Test
	imgsensor_mode_struct pre;		//preview scenario relative information
	imgsensor_mode_struct cap;		//capture scenario relative information
	imgsensor_mode_struct normal_video;//normal video  scenario relative information
	imgsensor_mode_struct hs_video; //high speed video scenario relative information
	imgsensor_mode_struct slim_video;	//slim video for VT scenario relative information

	kal_uint8  ae_shut_delay_frame;	//shutter delay frame for AE cycle
	kal_uint8  ae_sensor_gain_delay_frame;	//sensor gain delay frame for AE cycle
	kal_uint8  ae_ispGain_delay_frame;	//isp gain delay frame for AE cycle
	kal_uint8  frame_time_delay_frame;	// The delay frame of setting frame length
	kal_uint8  ihdr_support;		//1, support; 0,not support
	kal_uint8  ihdr_le_firstline;	//1,le first ; 0, se first
	kal_uint8  sensor_mode_num;		//support sensor mode num

	kal_uint8  cap_delay_frame;		//enter capture delay frame num
	kal_uint8  pre_delay_frame;		//enter preview delay frame num
	kal_uint8  video_delay_frame;	//enter video delay frame num
	kal_uint8  hs_video_delay_frame;        //enter high speed video  delay frame num
	kal_uint8  slim_video_delay_frame;	//enter slim video delay frame num

	kal_uint8  margin;				//sensor framelength & shutter margin
	kal_uint32 min_shutter;			//min shutter
	kal_uint32 max_frame_length;	//max framelength by sensor register's limitation

	kal_uint8  isp_driving_current;	//mclk driving current
	kal_uint8  sensor_interface_type;//sensor_interface_type
	kal_uint8  mipi_sensor_type; //0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2, default is NCSI2, don't modify this para
	kal_uint8  mipi_settle_delay_mode; //0, high speed signal auto detect; 1, use settle delay,unit is ns, default is auto detect, don't modify this para
	kal_uint8  sensor_output_dataformat;//sensor output first pixel color
	kal_uint8  mclk;				//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz

	kal_uint8  mipi_lane_num;		//mipi lane num
	kal_uint8  i2c_addr_table[5];	//record sensor support all write id addr, only supprt 4must end with 0xff
	kal_uint32  i2c_speed;     //i2c speed
} imgsensor_info_struct;

#define BLACK_LEVEL_SAMSUNG_10B_64 0x40

#define AWB_R_MIN 200
#define AWB_R_MAX 880
#define AWB_GR_MIN 760
#define AWB_GR_MAX 880
#define AWB_GB_MIN 760
#define AWB_GB_MAX 880
#define AWB_B_MIN 200
#define AWB_B_MAX 880

#define S5K3L6_EEPROM_CRC_MANUFACTURING_SIZE (37)
#define S5K3L6_EEPROM_CRC_AF_CAL_SIZE (24)

#define S5K3L6_EEPROM_CRC_AWB_CAL_SIZE (43)
#define S5K3L6_EEPROM_CRC_LSC_SIZE (1868)
#define S5K3L6_EEPROM_CRC_PDAF_OUTPUT1_SIZE (496)
#define S5K3L6_EEPROM_CRC_PDAF_OUTPUT2_SIZE (918)

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

struct s5k3l6_eeprom_t{
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
	uint8_t af_infinity_cal_dac[2];
	uint8_t af_macro_cal_dac[2];
	uint8_t af_infinity_sfr_center[2];
	uint8_t af_macro_sfr_center[2];
	uint8_t af_infinity_verify_dac[2];
	uint8_t af_macro_verify_dac[2];
	uint8_t af_infinity_verify_sfr_center[2];
	uint8_t af_macro_verify_sfr_center[2];
	uint8_t af_infinity_range[2];
	uint8_t af_macro_range[2];
	uint8_t af_sensor_temp_is_positive_infinity_cal[1];
	uint8_t af_sensor_temp_infinity_cal[1];
	uint8_t af_sensor_temp_is_positive_macro_cal[1];
	uint8_t af_sensor_temp_macro_cal[1];
	uint8_t af_cal_crc16[2];
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
	uint8_t rgb_optical_center_src_1_X_r[2];
	uint8_t rgb_optical_center_src_1_Y_r[2];
	uint8_t rgb_optical_center_src_1_X_gr[2];
	uint8_t rgb_optical_center_src_1_Y_gr[2];
	uint8_t rgb_optical_center_src_1_X_gb[2];
	uint8_t rgb_optical_center_src_1_Y_gb[2];
	uint8_t rgb_optical_center_src_1_X_b[2];
	uint8_t rgb_optical_center_src_1_Y_b[2];
	uint8_t rgb_oc_crc16[2];
	uint8_t sfr_data[453];
	uint8_t sfr_crc16[2];
	uint8_t lsc_data[1868];
	uint8_t lsc_crc16[2];
	uint8_t pdaf_output1_data[496];
	uint8_t pdaf_output2_data[918];
	uint8_t pdaf_output1_crc16[2];
	uint8_t pdaf_crc16[2];
};

typedef struct {
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
} awb_t;

typedef struct {
	float r_over_g;
	float b_over_g;
	float gr_over_gb;
} awb_factors_t;

typedef struct {
	uint8_t r_g_golden_min;
	uint8_t r_g_golden_max;
	uint8_t b_g_golden_min;
	uint8_t b_g_golden_max;
} awb_limit_t;

/* SENSOR READ/WRITE ID */
//#define IMGSENSOR_WRITE_ID_1 (0x6c)
//#define IMGSENSOR_READ_ID_1  (0x6d)
//#define IMGSENSOR_WRITE_ID_2 (0x20)
//#define IMGSENSOR_READ_ID_2  (0x21)

extern void kdSetI2CSpeed(u16 i2cSpeed);
extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern bool S5K3L6QT_read_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size);
#endif

/*
PREVIEW:尽量用binning mode， 并告知是Binning average还是Binning sum？AVERAGE
使用GP方式， shutter当前桢设置后，下下桢生效）
Static  DPC ON
slim video   不用到120fps吧。
get sensor id and Open()  has more code need to reused. need to modify the two place
20150513 第一次合入PDAF参数

*/
