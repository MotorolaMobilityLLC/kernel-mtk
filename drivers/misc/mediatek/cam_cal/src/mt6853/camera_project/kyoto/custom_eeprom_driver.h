/*
 * Copyright (C) 2019 MediaTek Inc.
 * Copyright (C) 2021 lucas <guoqiang8@lenovo.com>
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

#ifndef __EEPROM_DRIVER_OTP_H
#define __EEPROM_DRIVER_OTP_H
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/unistd.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "eeprom_driver.h"
#include "eeprom_i2c_common_driver.h"
#include "cam_cal_list.h"
#include "kd_camera_feature.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"

#define DEV_NAME_FMT_DEV "/dev/camera_eeprom%u"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s] " format, __func__, ##args)


#define BLACK_LEVEL_SAMSUNG_10B_64 0x40

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

typedef struct {
	uint8_t cie_rsv[6];
	uint8_t awb_r_g_golden_min_limit[1];
	uint8_t awb_r_g_golden_max_limit[1];
	uint8_t awb_b_g_golden_min_limit[1];
	uint8_t awb_b_g_golden_max_limit[1];
	// golden data
	uint8_t awb_src_1_golden_r[2];
	uint8_t awb_src_1_golden_gr[2];
	uint8_t awb_src_1_golden_gb[2];
	uint8_t awb_src_1_golden_b[2];
	uint8_t awb_src_1_golden_rg_ratio[2];
	uint8_t awb_src_1_golden_bg_ratio[2];
	uint8_t awb_src_1_golden_gr_gb_ratio[2];
	// unit data
	uint8_t awb_src_1_r[2];
	uint8_t awb_src_1_gr[2];
	uint8_t awb_src_1_gb[2];
	uint8_t awb_src_1_b[2];
	uint8_t awb_src_1_rg_ratio[2];
	uint8_t awb_src_1_bg_ratio[2];
	uint8_t awb_src_1_gr_gb_ratio[2];
	uint8_t awb_reserve[5];
} camcal_awb;

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

typedef enum {
	EEPROM_CRC_MANUFACTURING,
	EEPROM_CRC_AF_CAL,
	EEPROM_CRC_AWB_CAL,
	EEPROM_CRC_LSC,
	EEPROM_CRC_PDAF_OUTPUT,
	EEPROM_CRC_LIST
}EEPROM_CRC_TYPE_ENUM;

typedef struct {
	// Common manufacture info
	u8 table_revision[1];
	u8 cal_hw_revision[1];
	u8 cal_sw_revision[1];
	u8 mot_part_number[8];
	u8 actuator_id[1];
	u8 lens_id[1];
	u8 manufacturer_id[2];
	u8 factory_id[2];
	u8 manufacture_line[1];
	u8 manufacture_date[3];
	u8 serial_number[16];
} manufacture_info;

typedef struct {
    UINT16 Include;
    UINT32 StartAddr;
    UINT32 BlockSize;
    void (*doCalDataCheck)(u8 *pCamCaldata, UINT32 start_addr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
} CALIBRATION_ITEM_STRUCT;

typedef struct {
	struct stCAM_CAL_DATAINFO_STRUCT camCalData;
	const CALIBRATION_ITEM_STRUCT CalItemTbl[EEPROM_CRC_LIST];
}MOT_EEPROM_CAL;

static void mot_check_mnf_data(u8 *data, UINT32 StartAddr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
static void mot_check_af_data(u8 *data, UINT32 StartAddr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
static void mot_check_awb_data(u8 *data, UINT32 StartAddr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
static void mot_check_lsc_data(u8 *data, UINT32 StartAddr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
static void mot_check_pdaf_data(u8 *data, UINT32 StartAddr, UINT32 BlockSize, mot_calibration_info_t *mot_cal_info);
#endif
