/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __HELIO_DVFSRC_H
#define __HELIO_DVFSRC_H

#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/io.h>

#if defined(CONFIG_MACH_MT6765)
#include <helio-dvfsrc-mt6765.h>
#else
#include <helio-dvfsrc-mt67xx.h>
#endif

#include "helio-dvfsrc-opp.h"

struct reg_config {
	u32 offset;
	u32 val;
};

struct helio_dvfsrc {
	struct devfreq		*devfreq;

	bool enabled;

	void __iomem		*regs;
	void __iomem		*sram_regs;

	struct notifier_block	pm_qos_memory_bw_nb;
	struct notifier_block	pm_qos_cpu_memory_bw_nb;
	struct notifier_block	pm_qos_gpu_memory_bw_nb;
	struct notifier_block	pm_qos_mm_memory_bw_nb;
	struct notifier_block	pm_qos_other_memory_bw_nb;
	struct notifier_block	pm_qos_ddr_opp_nb;
	struct notifier_block	pm_qos_vcore_opp_nb;
	struct notifier_block	pm_qos_vcore_dvfs_force_opp_nb;

	struct reg_config	*init_config;

	struct opp_profile	opp_table[VCORE_DVFS_OPP_NUM];
};

#define DVFSRC_TIMEOUT		1000

#define QOS_TOTAL_BW_BUF_SIZE	8

#define QOS_TOTAL_BW_BUF(idx)	(idx * 4)
#define QOS_TOTAL_BW		(QOS_TOTAL_BW_BUF_SIZE * 4)
#define QOS_CPU_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x4)
#define QOS_GPU_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x8)
#define QOS_MM_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0xC)
#define QOS_OTHER_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x10)

/* PMIC */
#define vcore_pmic_to_uv(pmic)	\
	(((pmic) * VCORE_STEP_UV) + VCORE_BASE_UV)
#define vcore_uv_to_pmic(uv)	/* pmic >= uv */	\
	((((uv) - VCORE_BASE_UV) + (VCORE_STEP_UV - 1)) / VCORE_STEP_UV)

#define wait_for_completion(condition, timeout)			\
({								\
	int ret = 0;						\
	while (!(condition)) {					\
		if (ret++ >= timeout)				\
			ret = -EBUSY;				\
		udelay(1);					\
	}							\
	ret;							\
})

enum {
	QOS_TOTAL = 0,
	QOS_CPU,
	QOS_GPU,
	QOS_MM,
	QOS_OTHER,
	QOS_TOTAL_AVE
};

extern int is_dvfsrc_enabled(void);
extern int dvfsrc_get_bw(int type);
extern int get_vcore_dvfs_level(void);
extern void mtk_spmfw_init(void);
extern struct reg_config *dvfsrc_get_init_conf(void);
extern void helio_dvfsrc_enable(int dvfsrc_en);
extern char *dvfsrc_dump_reg(char *ptr);
extern void dvfsrc_write(u32 offset, u32 val);
extern u32 dvfsrc_read(u32 offset);
extern void dvfsrc_rmw(u32 offset, u32 val, u32 mask, u32 shift);

extern int helio_dvfsrc_add_interface(struct device *dev);
extern void helio_dvfsrc_remove_interface(struct device *dev);
extern void vcore_volt_init(void);
extern void helio_dvfsrc_sspm_ipi_init(int dvfsrc_en);
extern void get_opp_info(char *p);
extern void get_dvfsrc_reg(char *p);
extern void get_spm_reg(char *p);

#endif /* __HELIO_DVFSRC_H */

