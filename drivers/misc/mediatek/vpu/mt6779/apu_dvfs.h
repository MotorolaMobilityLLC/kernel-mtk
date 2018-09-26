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

#ifndef __APU_DVFS_H
#define __APU_DVFS_H

#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/io.h>
#include <linux/interrupt.h>


#define VVPU_DVFS_VOLT0	 (82500)	/* mV x 100 */
#define VVPU_DVFS_VOLT1	 (72500)	/* mV x 100 */
#define VVPU_DVFS_VOLT2	 (65000)	/* mV x 100 */
#define VVPU_PTPOD_FIX_VOLT	 (80000)	/* mV x 100 */


#define VMDLA_DVFS_VOLT0	 (82500)	/* mV x 100 */
#define VMDLA_DVFS_VOLT1	 (72500)	/* mV x 100 */
#define VMDLA_DVFS_VOLT2	 (65000)	/* mV x 100 */
#define VMDLA_PTPOD_FIX_VOLT	 (80000)	/* mV x 100 */

struct vpu_opp_table_info {
	unsigned int vpufreq_khz;
	unsigned int vpufreq_volt;
	unsigned int vpufreq_idx;
};
struct mdla_opp_table_info {
	unsigned int mdlafreq_khz;
	unsigned int mdlafreq_volt;
	unsigned int mdlafreq_idx;
};


struct apu_dvfs {
	struct devfreq		*devfreq;

	bool qos_enabled;
	bool dvfs_enabled;

	void __iomem		*regs;
	void __iomem		*sram_regs;

	struct notifier_block	pm_qos_vvpu_opp_nb;
	struct notifier_block	pm_qos_vmdla_opp_nb;

	struct reg_config	*init_config;
	struct device_node *dvfs_node;
	unsigned int dvfs_irq;

	bool opp_forced;
	char			force_start[20];
	char			force_end[20];
};

enum vvpu_opp {
	VVPU_OPP_0 = 0,
	VVPU_OPP_1,
	VVPU_OPP_2,
	VVPU_OPP_NUM,
	VVPU_OPP_UNREQ = PM_QOS_VVPU_OPP_DEFAULT_VALUE,
};
enum vmdla_opp {
	VMDLA_OPP_0 = 0,
	VMDLA_OPP_1,
	VMDLA_OPP_2,
	VMDLA_OPP_NUM,
	VMDLA_OPP_UNREQ = PM_QOS_VMDLA_OPP_DEFAULT_VALUE,
};



extern char *apu_dvfs_dump_reg(char *ptr);
extern void apu_dvfs_reg_config(struct reg_config *config);
extern int apu_dvfs_add_interface(struct device *dev);
extern void apu_dvfs_remove_interface(struct device *dev);
extern int apu_dvfs_platform_init(struct apu_dvfs *dvfs);

extern unsigned int vvpu_get_cur_volt(void);
extern unsigned int vvpu_update_volt(unsigned int pmic_volt[]
	, unsigned int array_size);
extern void vvpu_restore_default_volt(void);
extern unsigned int vpu_get_freq_by_idx(unsigned int idx);
extern unsigned int vpu_get_volt_by_idx(unsigned int idx);
extern void vpu_disable_by_ptpod(void);
extern void vpu_enable_by_ptpod(void);

#endif /* __APU_DVFS_H */

