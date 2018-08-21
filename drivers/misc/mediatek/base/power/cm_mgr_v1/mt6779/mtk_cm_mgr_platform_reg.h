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

#ifndef __MTK_CM_MGR_PLATFORM_REG_H__
#define __MTK_CM_MGR_PLATFORM_REG_H__

#undef MCUSYS_MP0
#define MCUSYS_MP0 mcucfg_mp0_counter_base

#define STALL_INFO_CONF(x) (MCUSYS_MP0 + 0x800 * (x) + 0x238)

#define CPU_AVG_STALL_RATIO(x) (MCUSYS_MP0 + 0x800 * (x) + 0x700)
#define CPU_AVG_STALL_DEBUG_CTRL(x) (MCUSYS_MP0 + 0x800 * (x) + 0x704)
#define CPU_AVG_STALL_FMETER_STATUS(x) (MCUSYS_MP0 + 0x800 * (x) + 0x708)
#define CPU_AVG_STALL_RATIO_CTRL(x) (MCUSYS_MP0 + 0x800 * (x) + 0x710)
#define CPU_AVG_STALL_COUNTER(x) (MCUSYS_MP0 + 0x800 * (x) + 0x714)

#define MP0_CPU_NONWFX_CTRL(x) (MCUSYS_MP0 + 0x8 * (x) + 0x8500)
#define MP0_CPU_NONWFX_CNT(x) (MCUSYS_MP0 + 0x8 * (x) + 0x8504)

/* STALL_INFO_CONF */
#define STALL_INFO_GEN_CONF_REG (0x8 << 6)
/* CPU_AVG_STALL_RATIO */
#define RG_CPU_AVG_STALL_RATIO (1 << 0)
/* CPU_AVG_STALL_DEBUG_CTRL */
#define RG_FMETER_APB_FREQUENCY (1 << 0)
#define RG_STALL_INFO_SEL (1 << 12)
#define RG_SAMPLE_RATIO (1 << 16)
/* CPU_AVG_STALL_FMETER_STATUS */
#define RO_FMETER_OUTPUT (1 << 0)
#define RO_FMETER_LOW (1 << 12)
/* CPU_AVG_STALL_RATIO_CTRL */
#define RG_CPU_AVG_STALL_RATIO_EN (1 << 0)
#define RG_CPU_AVG_STALL_RATIO_RESTART (1 << 1)
#define RG_CPU_STALL_COUNTER_EN (1 << 4)
#define REMOVED_RG_CPU_NON_WFX_COUNTER_EN (1 << 5)
#define RG_CPU_STALL_COUNTER_RESET (1 << 6)
#define REMOVED_RG_CPU_NON_WFX_COUNTER_RESET (1 << 7)
#define RG_MP0_AVG_STALL_PERIOD (1 << 8)
#define RG_FMETER_MIN_FREQUENCY (1 << 12)
#define RG_FMETER_EN (1 << 24)
/* CPU_AVG_STALL_COUNTER */
#define RO_CPU_STALL_COUNTER (1 << 0)
/* MP0_CPU0_NONWFX_CTRL */
#define RG_CPU0_NON_WFX_COUNTER_EN (1 << 0)
#define RG_CPU0_NON_WFX_COUNTER_RESET (1 << 4)

#define _RG_MP0_AVG_STALL_PERIOD 8
#define RG_MP0_AVG_STALL_PERIOD_1MS (0x8 << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_2MS (0x9 << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_4MS (0xa << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_8MS (0xb << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_10MS (0xc << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_20MS (0xd << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_40MS (0xe << _RG_MP0_AVG_STALL_PERIOD)
#define RG_MP0_AVG_STALL_PERIOD_80MS (0xf << _RG_MP0_AVG_STALL_PERIOD)

#define cm_mgr_read(addr)	__raw_readl((void __force __iomem *)(addr))
#define cm_mgr_write(addr, val)	mt_reg_sync_writel(val, addr)

#endif /* __MTK_CM_MGR_PLATFORM_REG_H__*/
