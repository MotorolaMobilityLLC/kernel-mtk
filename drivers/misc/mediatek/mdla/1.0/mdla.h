/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __MDLA_H__
#define __MDLA_H__
#include <linux/types.h>

#ifdef CONFIG_MTK_MDLA_SUPPORT
unsigned int mdla_cfg_read(u32 offset);
unsigned int mdla_reg_read(u32 offset);

// TODO: clearify the usage of gsm_bitmap
extern unsigned long gsm_bitmap[];
extern void *apu_mdla_gsm_top;
extern void *apu_mdla_biu_top;
extern u32 mdla_timeout;
#else
static inline
unsigned int mdla_dump_reg(void)
{
	return 0;
}
static inline
unsigned int mdla_dump_reg(void)
{
	return 0;
}
#endif

#ifndef MTK_MDLA_FPGA_PORTING
int mdla_set_power_parameter(uint8_t param, int argc, int *args);
int mdla_dump_power(struct seq_file *s);
int mdla_dump_opp_table(struct seq_file *s);

#else
static inline int mdla_set_power_parameter(uint8_t param, int argc, int *args)
{
	return 0;
}
static inline int mdla_dump_power(struct seq_file *s)
{
	return 0;
}
static inline int mdla_dump_opp_table(struct seq_file *s)
{
	return 0;
}

#endif

#endif

