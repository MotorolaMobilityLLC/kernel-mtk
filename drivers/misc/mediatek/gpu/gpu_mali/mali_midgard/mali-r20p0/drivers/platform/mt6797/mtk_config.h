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

#ifndef __MTK_KBASE_SPM_H__
#define __MTK_KBASE_SPM_H__

#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/debugfs.h>

struct mtk_config {
	struct clk *clk_mfg_async;
	struct clk *clk_mfg;
	struct clk *clk_mfg_core0;
	struct clk *clk_mfg_core1;
	struct clk *clk_mfg_core2;
	struct clk *clk_mfg_core3;
	struct clk *clk_mfg_main;
	struct clk *clk_mfg52m_vcg;

	struct clk *mux_mfg52m;
	struct clk *mux_mfg52m_52m;

	unsigned int max_volt;
	unsigned int max_freq;
	unsigned int min_volt;
	unsigned int min_freq;

	int32_t async_value;
};

#define MFG_DEBUG_SEL   (0x180)
#define MFG_DEBUG_A     (0x184)
#define MFG_DEBUG_IDEL  (1<<2)

#define CLK_MISC_CFG_0              0x104

#define base_write32(addr, value)     writel(value, addr)
#define base_read32(addr)             readl(addr)
#define MFG_write32(addr, value)      base_write32(g_MFG_base+addr, value)
#define MFG_read32(addr)              base_read32(g_MFG_base+addr)

unsigned int get_eem_status_for_gpu(void);

#define MFG_PROT_MASK                    ((0x1 << 21) | (0x1 << 23))
int spm_topaxi_protect(unsigned int mask_value, int en);

#define MTK_WDT_SWSYS_RST_MFG_RST (0x0004)
int mtk_wdt_swsysret_config(int bit, int set_value);

#endif
