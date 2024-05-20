/* SPDX-License-Identifier: GPL-2.0
 * aw883xx_monitor.h   aw883xx codec driver
 *
 * Copyright (c) 2020 AWINIC Technology CO., LTD
 *
 *  Author: Bruce zhao <zhaolei@awinic.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __AW883XX_MONITOR_H__
#define __AW883XX_MONITOR_H__

/*#define AW_DEBUG*/
/*#define AW_SYS_BATTERY_ST*/
#include "aw883xx_data_type.h"

struct aw_table;

#define AW_TABLE_SIZE	sizeof(struct aw_table)
#define AW_MONITOR_DEFAULT_FLAG (0)

#define IPEAK_NONE	(0xFF)
#define GAIN_NONE	(0xFF)
#define VMAX_NONE	(0xFFFFFFFF)


#define AW_GET_32_DATA(w, x, y, z) \
		((uint32_t)((((uint8_t)w) << 24) | (((uint8_t)x) << 16) | (((uint8_t)y) << 8) | ((uint8_t)z)))
#define AW_GET_16_DATA(x, y) \
		((uint16_t)((((uint8_t)x) << 8) | (uint8_t)y))

typedef enum {
	AW_GET_INIT_VMAX = 0,
	AW_SET_INIT_VMAX,
	AW_HW_MONITOR_ST,
	AW_DSP_MONITOR_ST,
} monitor_update_t;

enum {
	AW_MONITOR_DISABLE = 0,
	AW_MONITOR_ENABLE,
};

enum {
	AW_MON_LOGIC_OR = 0,
	AW_MON_LOGIC_AND = 1,
};

enum {
	AW_FIRST_ENTRY = 0,
	AW_NOT_FIRST_ENTRY = 1,
};

enum aw_monitor_hdr_ver {
	AW_MONITOR_HDR_VER_0_1_1 = 0x00010100,
};

enum aw_monitor_init {
	AW_MON_CFG_ST = 0,
	AW_MON_CFG_OK = 1,
};

#define MONITOR_EN_MASK  0x01

enum {
	MONITOR_EN_BIT = 0,
	MONITOR_LOGIC_BIT = 1,
	MONITOR_IPEAK_EN_BIT = 2,
	MONITOR_GAIN_EN_BIT = 3,
	MONITOR_VMAX_EN_BIT = 4,
	MONITOR_TEMP_EN_BIT = 5,
	MONITOR_VOL_EN_BIT = 6,
};

enum {
	AW_INTERNAL_TEMP = 0,
	AW_EXTERNAL_TEMP = 1,
};

struct aw_monitor_hdr_v_0_1_1 {
	uint32_t check_sum;
	uint32_t monitor_ver;
	char chip_type[16];
	uint32_t ui_ver;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t enable_flag;
		/* [bit 31:7]*/
		/* [bit 6: vol en]*/
		/* [bit 5: temp en]*/
		/* [bit 4: vmax en]*/
		/* [bit 3: gain en]*/
		/* [bit 2: ipeak en]*/
		/* [bit 1: & or | flag]*/
		/* [bit 0: monitor en]*/
	uint32_t temp_aplha;
	uint32_t temp_num;
	uint32_t single_temp_size;
	uint32_t temp_offset;
	uint32_t vol_aplha;
	uint32_t vol_num;
	uint32_t single_vol_size;
	uint32_t vol_offset;
	uint32_t reserver[3];
};

struct aw_table {
	int16_t min_val;
	int16_t max_val;
	uint16_t ipeak;
	uint16_t gain;
	uint32_t vmax;
};

struct aw_table_info {
	uint8_t table_num;
	struct aw_table *aw_table;
};

struct aw_monitor_cfg {
	uint8_t monitor_status;
	uint32_t monitor_switch;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t logic_switch;
	uint32_t temp_switch;
	uint32_t temp_aplha;
	uint32_t vol_switch;
	uint32_t vol_aplha;
	uint32_t ipeak_switch;
	uint32_t gain_switch;
	uint32_t vmax_switch;
	struct aw_table_info temp_info;
	struct aw_table_info vol_info;
};

struct aw_monitor_trace {
	int32_t pre_val;
	int32_t sum_val;
	struct aw_table aw_table;
};

struct aw_voltage_desc {
	unsigned int reg;
	unsigned int vbat_range;
	unsigned int int_bit;
};

struct aw_temperature_desc {
	unsigned int reg;
	unsigned int sign_mask;
	unsigned int neg_mask;
};

struct aw_ipeak_desc {
	unsigned int reg;
	unsigned int mask;
};

struct aw_vmax_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int init_vmax;
};


struct aw_dsp_monitor_desc {
	unsigned int dsp_temp_reg;
	unsigned int dsp_temp_reg_type;
	unsigned int dsp_temp_en_mask;

	unsigned int dsp_volt_reg;
	unsigned int dsp_volt_reg_type;
	unsigned int dsp_volt_en_mask;

	unsigned int extn_temp_reg;
	unsigned char extn_temp_reg_type;

	unsigned int extn_temp_en_reg;
	unsigned int extn_temp_en_reg_type;
	unsigned int extn_temp_en_mask;
};

struct aw_hw_monitor_desc {
	unsigned int reg;
	unsigned int bop_mask;
	unsigned int vol_mask;
	unsigned int ipeak_mask;
};

/******************************************************************
 * struct monitor
 *******************************************************************/
struct aw_monitor_desc {
	struct delayed_work delay_work;
	struct delayed_work dsp_monitor_work;
	struct aw_monitor_cfg monitor_cfg;

	uint8_t hw_monitor_en;
	uint8_t dsp_monitor_en;
	uint8_t dsp_temp_flag;
	uint32_t dsp_monitor_delay;

	uint8_t first_entry;
	uint8_t samp_count;
	uint32_t pre_vmax;

	struct aw_monitor_trace temp_trace;
	struct aw_monitor_trace vol_trace;

#ifdef AW_DEBUG
	uint16_t test_vol;
	int16_t test_temp;
#endif

	struct aw_hw_monitor_desc hw_monitor_desc;
	struct aw_dsp_monitor_desc dsp_monitor_desc;

	struct aw_voltage_desc voltage_desc;
	struct aw_temperature_desc temp_desc;
	struct aw_vmax_desc vmax_desc;
	struct aw_ipeak_desc ipeak_desc;
};

/******************************************************************
 * aw882xx monitor functions
 *******************************************************************/
void aw883xx_monitor_start(struct aw_monitor_desc *monitor_desc);
int aw883xx_monitor_stop(struct aw_monitor_desc *monitor_desc);
void aw883xx_monitor_init(struct aw_monitor_desc *monitor_desc);
void aw883xx_monitor_deinit(struct aw_monitor_desc *monitor_desc);
int aw883xx_monitor_parse_fw(struct aw_monitor_desc *monitor_desc,
				uint8_t *data, uint32_t data_len);
void aw883xx_monitor_update_st(struct aw_monitor_desc *monitor_desc, monitor_update_t type);

#endif

