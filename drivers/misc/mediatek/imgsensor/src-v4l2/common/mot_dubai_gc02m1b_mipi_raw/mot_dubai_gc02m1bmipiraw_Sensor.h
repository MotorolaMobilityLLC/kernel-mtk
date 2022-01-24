/*****************************************************************************
 *
 * Filename:
 * ---------
 *     gc02m1bmipi_Sensor.h
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
#ifndef _GC02M1BMIPI_SENSOR_H
#define _GC02M1BMIPI_SENSOR_H


/* SENSOR MIRROR FLIP INFO */
#define GC02M1B_MIRROR_NORMAL    0
#define GC02M1B_MIRROR_H         0
#define GC02M1B_MIRROR_V         0
#define GC02M1B_MIRROR_HV        1

#if GC02M1B_MIRROR_NORMAL
#define GC02M1B_MIRROR	        0x80
#elif GC02M1B_MIRROR_H
#define GC02M1B_MIRROR	        0x81
#elif GC02M1B_MIRROR_V
#define GC02M1B_MIRROR	        0x82
#elif GC02M1B_MIRROR_HV
#define GC02M1B_MIRROR	        0x83
#else
#define GC02M1B_MIRROR	        0x80
#endif

/* SENSOR PRIVATE INFO FOR GAIN SETTING */
#define GC02M1B_SENSOR_GAIN_BASE             0x400
#define GC02M1B_SENSOR_GAIN_MAX              (12 * GC02M1B_SENSOR_GAIN_BASE)
#define GC02M1B_SENSOR_GAIN_MAX_VALID_INDEX  16
#define GC02M1B_SENSOR_GAIN_MAP_SIZE         16
#define GC02M1B_SENSOR_DGAIN_BASE            0x400

enum IMGSENSOR_MODE {
	IMGSENSOR_MODE_INIT,
	IMGSENSOR_MODE_PREVIEW,
	IMGSENSOR_MODE_CAPTURE,
	IMGSENSOR_MODE_VIDEO,
	IMGSENSOR_MODE_HIGH_SPEED_VIDEO,
	IMGSENSOR_MODE_SLIM_VIDEO,
};

struct imgsensor_mode_struct {
	kal_uint32 pclk;	/* record different mode's pclk */
	kal_uint32 linelength;	/* record different mode's linelength */
	kal_uint32 framelength;	/* record different mode's framelength */

	kal_uint8 startx; /* record different mode's startx of grabwindow */
	kal_uint8 starty; /* record different mode's startx of grabwindow */

	/* record different mode's width of grabwindow */
	kal_uint16 grabwindow_width;

	/* record different mode's height of grabwindow */
	kal_uint16 grabwindow_height;

	/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
	 * by different scenario
	 */
	kal_uint8 mipi_data_lp2hs_settle_dc;

	/*   following for GetDefaultFramerateByScenario()  */
	kal_uint16 max_framerate;
	kal_uint32 mipi_pixel_rate;

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

/* SENSOR PRIVATE STRUCT FOR CONSTANT*/
struct imgsensor_info_struct {
	kal_uint32 sensor_id;	/* record sensor id defined in Kd_imgsensor.h */
	kal_uint32 checksum_value; /* checksum value for Camera Auto Test */

	/* preview scenario relative information */
	struct imgsensor_mode_struct pre;

	/* capture scenario relative information */
	struct imgsensor_mode_struct cap;

	/* normal video  scenario relative information */
	struct imgsensor_mode_struct normal_video;

	/* high speed video scenario relative information */
	struct imgsensor_mode_struct hs_video;

	/* slim video for VT scenario relative information */
	struct imgsensor_mode_struct slim_video;

	kal_uint8 ae_shut_delay_frame;	/* shutter delay frame for AE cycle */
	kal_uint8 ae_sensor_gain_delay_frame;/* sensor gain delay frame for AE cycle */
	kal_uint8 ae_ispGain_delay_frame;/* isp gain delay frame for AE cycle */
    kal_uint8 frame_time_delay_frame;	/* The delay frame of setting frame length  */

	kal_uint8 ihdr_support;	/* 1, support; 0,not support */
	kal_uint8 ihdr_le_firstline;	/* 1,le first ; 0, se first */
	kal_uint8 sensor_mode_num;	/* support sensor mode num */

	kal_uint8 cap_delay_frame;	/* enter capture delay frame num */
	kal_uint8 pre_delay_frame;	/* enter preview delay frame num */
	kal_uint8 video_delay_frame;	/* enter video delay frame num */
	kal_uint8 hs_video_delay_frame;/* enter high speed video  delay frame num */
	kal_uint8 slim_video_delay_frame; /* enter slim video delay frame num */

	kal_uint8 margin;	/* sensor framelength & shutter margin */
	kal_uint32 min_shutter;	/* min shutter */
	kal_uint32 min_gain;
	kal_uint32 max_gain;
	kal_uint32 min_gain_iso;
	kal_uint32 gain_step;
	kal_uint32 gain_type;

	/* max framelength by sensor register's limitation */
	kal_uint32 max_frame_length;

	kal_uint8 isp_driving_current;	/* mclk driving current */
	kal_uint8 sensor_interface_type;	/* sensor_interface_type */
	kal_uint8 temperature_support;
	kal_uint8 mipi_sensor_type;
	/* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2,
	 * default is NCSI2, don't modify this para
	 */

	kal_uint8 mipi_settle_delay_mode;
	/* 0, high speed signal auto detect; 1,
	 * use settle delay,unit is ns,
	 * default is auto detect, don't modify this para
	 */

	kal_uint8 sensor_output_dataformat;
	kal_uint8 mclk;	/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */

	kal_uint8 mipi_lane_num;	/* mipi lane num */

/* record sensor support all write id addr, only supprt 4must end with 0xff */
	kal_uint8 i2c_addr_table[5];
	kal_uint32 i2c_speed;	/* i2c speed */
};


typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;


#endif
