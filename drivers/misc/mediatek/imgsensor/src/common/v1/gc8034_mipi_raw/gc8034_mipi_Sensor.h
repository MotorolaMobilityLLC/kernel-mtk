/*****************************************************************************
 *
 * Filename:
 * ---------
 *     gc8034mipi_Sensor.h
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
#ifndef __GC8034MIPI_SENSOR_H__
#define __GC8034MIPI_SENSOR_H__

/* SENSOR MIRROR FLIP INFO */
#define GC8034_MIRROR_FLIP_ENABLE        1
#if GC8034_MIRROR_FLIP_ENABLE
#define GC8034_MIRROR		  0xc3
#define GC8034_BIN_STARTY	  0x04
#define GC8034_BIN_STARTX	  0x05
#define GC8034_FULL_STARTY	  0x08
#define GC8034_FULL_STARTX	  0x0b
#else
#define GC8034_MIRROR		  0xc0
#define GC8034_BIN_STARTY	  0x04
#define GC8034_BIN_STARTX	  0x05
#define GC8034_FULL_STARTY	  0x08
#define GC8034_FULL_STARTX	  0x09
#endif

#define SENSOR_BASE_GAIN       0x40
#define SENSOR_MAX_GAIN        (16 * SENSOR_BASE_GAIN)
#define MAX_AG_INDEX           9
#define MEAG_INDEX             7
#define AGC_REG_NUM            14

/* SENSOR OTP INFO */
#define GC8034OTP_FOR_CUSTOMER       0
/* Please do not enable the following micro definition, if you are not debuging otp function*/
#define GC8034OTP_DEBUG              0

#define DD_WIDTH                     3284
#define DD_HEIGHT                    2464
#define GC8034_OTP_ID_SIZE           9
#define GC8034_OTP_ID_DATA_OFFSET    0x0010

#if GC8034OTP_FOR_CUSTOMER
#define RG_TYPICAL          0x0400
#define BG_TYPICAL          0x0400
#define AF_ROM_START        0x3b
#define AF_WIDTH            0x04
#define INFO_ROM_START      0x70
#define INFO_WIDTH          0x08
#define WB_ROM_START        0x5f
#define WB_WIDTH            0x04
#define GOLDEN_ROM_START    0x67
#define GOLDEN_WIDTH        0x04
#define LSC_NUM             99		/* (7+2)*(9+2) */
#endif

struct gc8034_dd_t {
	kal_uint16 x;
	kal_uint16 y;
	kal_uint16 t;
};

struct gc8034_otp_t {
	kal_uint8 otp_id[GC8034_OTP_ID_SIZE];
	kal_uint8  dd_cnt;
	kal_uint8  dd_flag;
	struct gc8034_dd_t dd_param[160];
	kal_uint8  reg_flag;
	kal_uint8  reg_num;
	kal_uint8  reg_page[10];
	kal_uint8  reg_addr[10];
	kal_uint8  reg_value[10];
#if GC8034OTP_FOR_CUSTOMER
	kal_uint8  module_id;
	kal_uint8  lens_id;
	kal_uint8  vcm_id;
	kal_uint8  vcm_driver_id;
	kal_uint8  year;
	kal_uint8  month;
	kal_uint8  day;
	kal_uint8  af_flag;
	kal_uint16 af_infinity;
	kal_uint16 af_macro;
	kal_uint8  wb_flag;
	kal_uint16 rg_gain;
	kal_uint16 bg_gain;
	kal_uint8  golden_flag;
	kal_uint16 golden_rg;
	kal_uint16 golden_bg;
	kal_uint8  lsc_flag;		/* 0:Empty 1:Success 2:Invalid */
	kal_uint8  lsc_param[396];
#endif
};

enum{
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HIGH_SPEED_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
};

struct imgsensor_mode_struct {
	kal_uint32 pclk;
	kal_uint32 linelength;
	kal_uint32 framelength;
	kal_uint8 startx;
	kal_uint8 starty;
	kal_uint16 grabwindow_width;
	kal_uint16 grabwindow_height;
	kal_uint32 mipi_pixel_rate;
	kal_uint8 mipi_data_lp2hs_settle_dc;
	kal_uint16 max_framerate;
};

/* SENSOR PRIVATE STRUCT FOR VARIABLES */
struct imgsensor_struct {
	kal_uint8 mirror;
	kal_uint8 sensor_mode;
	kal_uint32 shutter;
	kal_uint16 gain;
	kal_uint32 pclk;
	kal_uint32 frame_length;
	kal_uint32 line_length;
	kal_uint32 min_frame_length;
	kal_uint16 dummy_pixel;
	kal_uint16 dummy_line;
	kal_uint16 current_fps;
	kal_bool   autoflicker_en;
	kal_bool   test_pattern;
	enum MSDK_SCENARIO_ID_ENUM current_scenario_id;
	kal_uint8  ihdr_en;
	kal_uint8 i2c_write_id;
};

/* SENSOR PRIVATE STRUCT FOR CONSTANT */
struct imgsensor_info_struct {
	kal_uint32 sensor_id;
	kal_uint32 checksum_value;
	struct imgsensor_mode_struct pre;
	struct imgsensor_mode_struct cap;
	struct imgsensor_mode_struct cap1;
	struct imgsensor_mode_struct cap2;
	struct imgsensor_mode_struct normal_video;
	struct imgsensor_mode_struct hs_video;
	struct imgsensor_mode_struct slim_video;
	kal_uint8  ae_shut_delay_frame;
	kal_uint8  ae_sensor_gain_delay_frame;
	kal_uint8  ae_ispGain_delay_frame;
	kal_uint8  ihdr_support;
	kal_uint8  ihdr_le_firstline;
	kal_uint8  sensor_mode_num;
	kal_uint8  cap_delay_frame;
	kal_uint8  pre_delay_frame;
	kal_uint8  video_delay_frame;
	kal_uint8  hs_video_delay_frame;
	kal_uint8  slim_video_delay_frame;
	kal_uint8  margin;
	kal_uint32 min_shutter;
	kal_uint32 max_frame_length;
	kal_uint8  isp_driving_current;
	kal_uint8  sensor_interface_type;
	kal_uint8  mipi_sensor_type;
	kal_uint8  mipi_settle_delay_mode;
	kal_uint8  sensor_output_dataformat;
	kal_uint8  mclk;
	kal_uint8  mipi_lane_num;
	kal_uint8  i2c_addr_table[5];
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int ontim_get_otp_data(u32  sensorid, u8 * p_buf, u32 Length);

#endif
