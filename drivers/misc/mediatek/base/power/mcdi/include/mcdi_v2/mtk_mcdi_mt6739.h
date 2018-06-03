/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_MCDI_MT6739_H__
#define __MTK_MCDI_MT6739_H__

/* #define MCDI_TRACE */
/* #define MCDI_MBOX*/

#define NF_CPU                  4
#define NF_CLUSTER              1

#define MCDI_SYSRAM_SIZE    0x400
#define MCDI_SYSRAM_NF_WORD (MCDI_SYSRAM_SIZE / 4)
#define SYSRAM_DUMP_RANGE   50

#define SYSRAM_TO_MCUPM_BASE_REG   (mcdi_sysram_base + 0xC08)
#define SYSRAM_PROF_RATIO_REG      (mcdi_sysram_base + 0xC20)
#define SYSRAM_PROF_BASE_REG       (mcdi_sysram_base + 0xC48)
#define SYSRAM_PROF_DATA_REG       (mcdi_sysram_base + 0xC70)
#define SYSRAM_LATENCY_BASE_REG    (mcdi_sysram_base + 0xCA0)
#define SYSRAM_PROF_RARIO_DUR      (mcdi_sysram_base + 0xC9C)
#define SYSRAM_DISTRIBUTE_BASE_REG (mcdi_sysram_base + 0xCE0)

#define SYSRAM_PROF_REG(ofs)          (SYSRAM_PROF_BASE_REG + ofs)
#define CPU_OFF_LATENCY_REG(ofs)      (SYSRAM_LATENCY_BASE_REG + ofs)
#define CPU_ON_LATENCY_REG(ofs)       (SYSRAM_LATENCY_BASE_REG + 0x10 + ofs)
#define Cluster_OFF_LATENCY_REG(ofs)  (SYSRAM_LATENCY_BASE_REG + 0x20 + ofs)
#define Cluster_ON_LATENCY_REG(ofs)   (SYSRAM_LATENCY_BASE_REG + 0x30 + ofs)

#define LATENCY_DISTRIBUTE_REG(ofs) (SYSRAM_DISTRIBUTE_BASE_REG + ofs)
#define PROF_OFF_CNT_REG(idx)       (LATENCY_DISTRIBUTE_REG(idx * 4))
#define PROF_ON_CNT_REG(idx)        (LATENCY_DISTRIBUTE_REG((idx + 4) * 4))

enum {
	ALL_CPU_IN_CLUSTER = 0,
	CPU_CLUSTER,
	CPU_IN_OTHER_CLUSTER,

	NF_PWR_STAT_MAP_TYPE
};

extern void __iomem *mcdi_sysram_base;
extern void __iomem *mcdi_mcupm_base;
extern void __iomem *mcdi_mcupm_sram_base;
extern unsigned int cpu_cluster_pwr_stat_map[NF_PWR_STAT_MAP_TYPE][NF_CPU];
#endif /* __MTK_MCDI_MT6739_H__ */
