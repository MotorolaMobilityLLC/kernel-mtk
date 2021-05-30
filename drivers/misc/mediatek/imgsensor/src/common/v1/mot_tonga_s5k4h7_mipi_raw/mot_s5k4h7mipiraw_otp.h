
/*
 * Copyright (C) 2021 lucas (guoqiang8@lenovo.com).
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

#ifndef _S5K4H7MIPI_OTP_H
#define _S5K4H7MIPI_OTP_H

#define DATA_PAGE_NUM 3
#define OTP_PAGE_START_ADDR 0x0A04
#define OTP_PAGE_SIZE 64

#define EEPROM_DATA_PATH "/data/vendor/camera_dump/s5k4h7_eeprom_data.bin"
#define SERIAL_DATA_PATH "/data/vendor/camera_dump/serial_number_wide.bin"
#define DUMP_WIDE_SERIAL_NUMBER_SIZE 8
#define S5K4H7_EEPROM_MAX_SIZE 2185

typedef enum {
	PAGE_INVAIL,
	PAGE_VAIL,
        PAGE_VAIL_BIG
} CHECK_PAGE;

typedef struct {
	unsigned short page_num;
	unsigned short page_sart_size;
	unsigned short page_end_size;
} PAGE_OFFSETINFO;

typedef struct {
	unsigned short page[DATA_PAGE_NUM];
	unsigned short page_vail;
        PAGE_OFFSETINFO page_offset[DATA_PAGE_NUM];;
        unsigned int   page_addr[DATA_PAGE_NUM];
	CHECK_PAGE     flag;

} PAGE_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} MNF_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AWB_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AF_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} AF_SYNC_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} LSC_INFO;

typedef struct {
	PAGE_INFO page_info;
	unsigned int size;
	unsigned char *data;
} COMMON_PAGE_INFO;

typedef struct {
	MNF_INFO  mnf_info;
	AWB_INFO  awb_info;
	AF_INFO   af_info;
	LSC_INFO  lsc_info;
	COMMON_PAGE_INFO  light_source_info;
	COMMON_PAGE_INFO  optical_center_info;
	COMMON_PAGE_INFO  sfr_distance_1_info;
	COMMON_PAGE_INFO  sfr_distance_2_info;
	COMMON_PAGE_INFO  sfr_distance_3_info;
//	COMMON_PAGE_INFO  af_sync_info;
//	COMMON_PAGE_INFO  mtk_module_info;
	unsigned char data[S5K4H7_EEPROM_MAX_SIZE];
} OTP_INFO;

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

#define AWB_R_MIN 200*64
#define AWB_R_MAX 880*64
#define AWB_GR_MIN 760*64
#define AWB_GR_MAX 880*64
#define AWB_GB_MIN 760*64
#define AWB_GB_MAX 880*64
#define AWB_B_MIN 200*64
#define AWB_B_MAX 880*64

/*
0.97 * 100* 16384
1.03 * 100* 16384
*/
#define AWB_GR_GB_MIN 1589248
#define AWB_GR_GB_MAX 1687552

typedef struct mot_s5k4h7_otp_map {
	//manufacture info, 33 bytes.
	UINT8 program_flag[1];
	UINT8 table_revision[1];
	UINT8 cal_hw_revision[1];
	UINT8 cal_sw_revision[1];
	UINT8 mot_part_number[8];
	UINT8 actuator_id[1];
	UINT8 lens_id[1];
	UINT8 manufacturer_id[2];
	UINT8 factory_id[2];
	UINT8 manufacture_line[1];
	UINT8 manufacture_date[3];
	UINT8 serial_number[8];
	UINT8 reserve_1[1];
	UINT8 mnf_info_crc[2];
	//light source, 23 bytes
	UINT8 light_source_flag[1];
	UINT8 wb_limit_range_rg_lower[1];
	UINT8 wb_limit_range_rg_upper[1];
	UINT8 wb_limit_range_bg_lower[1];
	UINT8 wb_limit_range_bg_upper[1];
	UINT8 reserve_2[4];
	UINT8 cie_ev_src1[2];
	UINT8 cie_u_src1[2];
	UINT8 cie_v_src1[2];
	UINT8 reserve_3[6];
	UINT8 light_source_crc[2];
	//AWB, 59 bytes.
	UINT8 wb_flag[1];
	UINT8 wb_golden_r[2];
	UINT8 wb_golden_gr[2];
	UINT8 wb_golden_gb[2];
	UINT8 wb_golden_b[2];
	UINT8 wb_golden_r_g_ratio[2];
	UINT8 wb_golden_b_g_ratio[2];
	UINT8 wb_golden_gr_gb_ratio[2];
	UINT8 reserve_4[14];
	UINT8 wb_unit_r[2];
	UINT8 wb_unit_gr[2];
	UINT8 wb_unit_gb[2];
	UINT8 wb_unit_b[2];
	UINT8 wb_unit_r_g_ratio[2];
	UINT8 wb_unit_b_g_ratio[2];
	UINT8 wb_unit_gr_gb_ratio[2];
	UINT8 reserve_5[14];
	UINT8 wb_crc[2];
	//optical center, 21 bytes
	UINT8 oc_flag[1];
	UINT8 oc_data[16];
	UINT8 reserve_6[2];
	UINT8 oc_crc[2];
	//SFR distance 1 = 50cm, 34 bytes
	UINT8 sfr_dist1_flag[1];
	UINT8 sfr_dist1_data[39];
	UINT8 sfr_dist1_crc[2];
	//SFR distance 2 = 2.5cm, 34 bytes
	UINT8 sfr_dist2_flag[1];
	UINT8 sfr_dist2_data[39];
	UINT8 sfr_dist2_crc[2];
	//SFR distance 3 = 1000cm, 34 bytes
	UINT8 sfr_dist3_flag[1];
	UINT8 sfr_dist3_data[39];
	UINT8 sfr_dist3_crc[2];
	// MTK LSC, 1871 bytes.
	UINT8 lsc_flag[1];
	UINT8 lsc_data[1868];
	UINT8 lsc_crc[2];
	// MTK necessary info, 22 bytes
	UINT8 mtk_info_flag[1];
	UINT8 module_flag[1];
	UINT8 otp_cal_type[4];
	UINT8 sensor_type[1];
	UINT8 awb_af_cal_info[1];
	UINT8 table_size[2];
	UINT8 af_inf_posture_cal[2];
	UINT8 af_macro_posture_cal[2];
	UINT8 format_version[1];
	UINT8 format_flag[3];
	UINT8 reserve_info[2];
	UINT8 mtk_info_crc[2];
} mot_s5k4h7_otp_t;

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing);
extern int iWriteRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData,
	u16 i2cId, u16 timing);

extern void kdSetI2CSpeed(u16 i2cSpeed);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId,
				u16 transfer_length, u16 timing);
int s5k4h7_otp_data(void);
#endif
