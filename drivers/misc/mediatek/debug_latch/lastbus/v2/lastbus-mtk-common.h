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

#ifndef __LASTBUS_MTK_COMMON_H__
#define __LASTBUS_MTK_COMMON_H__

#define get_bit_at(reg, pos) (((reg) >> (pos)) & 1)

struct lastbus_mcusys_offsets {
	unsigned int bus_mcu_m0;
	unsigned int bus_mcu_s1;
	unsigned int bus_mcu_m0_m;
	unsigned int bus_mcu_s1_m;
};

struct lastbus_perisys_offsets {
	unsigned int bus_peri_r0;
	unsigned int bus_peri_r1;
	unsigned int bus_peri_mon;
};

struct lastbus_plt_cfg {
	unsigned int num_master_port;
	unsigned int num_slave_port;
	unsigned int num_perisys_mon;
	unsigned int peri_enable_setting;
	unsigned int peri_timeout_setting;
	struct lastbus_mcusys_offsets mcusys_offsets;
	struct lastbus_perisys_offsets perisys_offsets;
};

extern const struct lastbus_plt_cfg plt_cfg;

#endif /* end of __LASTBUS_MTK_COMMON_H__ */
