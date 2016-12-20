/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef MTK_POWER_GS_H
#define MTK_POWER_GS_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define REMAP_SIZE_MASK     0xFFF

extern bool is_already_snap_shot;

typedef enum {
	MODE_NORMAL,
	MODE_COMPARE,
	MODE_APPLY,
	MODE_COLOR,
	MODE_DIFF,
} print_mode;

struct golden_setting {
	unsigned int addr;
	unsigned int mask;
	unsigned int golden_val;
};

struct snapshot {
	const char *func;
	unsigned int line;
	unsigned int reg_val[1];
};

struct golden {
	unsigned int is_golden_log;

	print_mode mode;

	char func[64];
	unsigned int line;

	unsigned int *buf;
	unsigned int buf_size;

	struct golden_setting *buf_golden_setting;
	unsigned int nr_golden_setting;
	unsigned int max_nr_golden_setting;

	struct snapshot *buf_snapshot;
	unsigned int max_nr_snapshot;
	unsigned int snapshot_head;
	unsigned int snapshot_tail;
#ifdef CONFIG_OF
	unsigned int phy_base;
	void __iomem *io_base;
#endif
};

unsigned int golden_read_reg(unsigned int addr);
int snapshot_golden_setting(const char *func, const unsigned int line);
void mt_power_gs_compare(char *scenario, char *pmic_name,
			 const unsigned int *pmic_gs, unsigned int pmic_gs_len);
unsigned int _golden_read_reg(unsigned int addr);
void _golden_write_reg(unsigned int addr, unsigned int mask, unsigned int reg_val);
int _snapshot_golden_setting(struct golden *g, const char *func, const unsigned int line);
void mt_power_gs_suspend_compare(void);
void mt_power_gs_dpidle_compare(void);
void mt_power_gs_sp_dump(void);

extern struct golden _golden;

#endif
