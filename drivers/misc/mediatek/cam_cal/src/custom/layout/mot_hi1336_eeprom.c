// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "cam_cal_config.h"

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000003, 0x32443832, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000025, do_part_number},
		{0x00000001, 0x000000F7, 0x0000074C, mot_do_single_lsc},
		{0x00000001, 0x00000027, 0x00000045, mot_do_2a_gain},
		{0x00000001, 0x00000845, 0x000005DC, mot_do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000001, 0x00000000, 0x00000E25, mot_do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id},
		{0x00000001, 0x00000000, 0x00000025, mot_do_manufacture_info}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_hi1336_eeprom = {
	.name = "mot_hi1336_eeprom",
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.layout = &cal_layout_table,
	.sensor_id = MOT_DUBAI_HI1336_SENSOR_ID,
	.i2c_write_id = 0xA0,
	.max_size = 0x8000,
	.enable_preload = 1,
	.preload_size = 0x0E25,
};
