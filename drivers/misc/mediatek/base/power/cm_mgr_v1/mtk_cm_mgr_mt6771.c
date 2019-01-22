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

/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/sched/rt.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/suspend.h>
#include <linux/topology.h>
#include <linux/math64.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/aee.h>
#include <trace/events/mtk_events.h>
#include <mt-plat/met_drv.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <mtk_cm_mgr.h>
#include <mtk_spm_reg.h>

void __iomem *mcucfg_mp0_counter_base;
void __iomem *mcucfg_mp2_counter_base;
void __iomem *spm_sleep_base;

#define diff_value_overflow(diff, a, b) do {\
	if ((a) >= (b)) \
	diff = (a) - (b);\
	else \
	diff = 0xffffffff - (b) + (a); \
} while (0) \

#define CM_MGR_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define USE_TIME_NS
/* #define USE_DEBUG_LOG */

struct stall_s {
	unsigned int clustor[CM_MGR_CPU_CLUSTER];
	unsigned long long stall_val[CM_MGR_CPU_COUNT];
	unsigned long long stall_val_diff[CM_MGR_CPU_COUNT];
	unsigned long long time_ns[CM_MGR_CPU_COUNT];
	unsigned long long time_ns_diff[CM_MGR_CPU_COUNT];
	unsigned long long ratio[CM_MGR_CPU_COUNT];
	unsigned int ratio_max[CM_MGR_CPU_COUNT];
	unsigned int cpu;
	unsigned int cpu_count[CM_MGR_CPU_CLUSTER];
};

static struct stall_s stall_all;
static struct stall_s *pstall_all = &stall_all;
static int cm_mgr_idx = -1;

#ifdef USE_DEBUG_LOG
void debug_stall(int cpu)
{
	pr_debug("%s: cpu number %d ################\n", __func__, cpu);
	pr_debug("%s: clustor[%d] 0x%08x\n", __func__, cpu / 4, pstall_all->clustor[cpu / 4]);
	pr_debug("%s: stall_val[%d] 0x%016llx\n", __func__, cpu, pstall_all->stall_val[cpu]);
	pr_debug("%s: stall_val_diff[%d] 0x%016llx\n", __func__, cpu, pstall_all->stall_val_diff[cpu]);
	pr_debug("%s: time_ns[%d] 0x%016llx\n", __func__, cpu, pstall_all->time_ns[cpu]);
	pr_debug("%s: time_ns_diff[%d] 0x%016llx\n", __func__, cpu, pstall_all->time_ns_diff[cpu]);
	pr_debug("%s: ratio[%d] 0x%016llx\n", __func__, cpu, pstall_all->ratio[cpu]);
	pr_debug("%s: ratio_max[%d] 0x%08x\n", __func__, cpu / 4, pstall_all->ratio_max[cpu / 4]);
	pr_debug("%s: cpu 0x%08x\n", __func__, pstall_all->cpu);
	pr_debug("%s: cpu_count[%d] 0x%08x\n", __func__, cpu / 4, pstall_all->cpu_count[cpu / 4]);
}

void debug_stall_all(void)
{
	int i;

	for (i = 0; i < CM_MGR_CPU_COUNT; i++)
		debug_stall(i);
}
#endif

static int cm_mgr_check_dram_type(void)
{
#ifdef CONFIG_MTK_DRAMC
	int ddr_type = get_ddr_type();
	int ddr_hz = dram_steps_freq(0);

	if (ddr_type == TYPE_LPDDR4X && ddr_hz == 3600)
		cm_mgr_idx = CM_MGR_LP4X_2CH_3600;
	else if (ddr_type == TYPE_LPDDR4X && ddr_hz == 3200)
		cm_mgr_idx = CM_MGR_LP4X_2CH_3200;
	else if (ddr_type == TYPE_LPDDR3 && ddr_hz == 1866)
		cm_mgr_idx = CM_MGR_LP3_1CH_1866;
	pr_info("#@# %s(%d) cm_mgr_idx 0x%x\n", __func__, __LINE__, cm_mgr_idx);
#else
	cm_mgr_idx = 0;
	pr_info("#@# %s(%d) !!!! NO CONFIG_MTK_DRAMC !!! set cm_mgr_idx to 0x%x\n", __func__, __LINE__, cm_mgr_idx);
#endif
	return cm_mgr_idx;
};

int cm_mgr_get_idx(void)
{
	if (cm_mgr_idx < 0)
		return cm_mgr_check_dram_type();
	else
		return cm_mgr_idx;
};

int cm_mgr_get_stall_ratio(int cpu)
{
	return pstall_all->ratio[cpu];
}

int cm_mgr_get_max_stall_ratio(int cluster)
{
	return pstall_all->ratio_max[cluster];
}

int cm_mgr_get_cpu_count(int cluster)
{
	return pstall_all->cpu_count[cluster];
}

int cm_mgr_read_stall(int cpu)
{
	int val = 0;

	if (cpu < 4) {
		if (cm_mgr_read(spm_sleep_base + 0x180) & (1 << 8))
			val = cm_mgr_read(MP0_CPU0_STALL_COUNTER + 4 * cpu);
	} else {
		if (cm_mgr_read(spm_sleep_base + 0x180) & (1 << 15))
			val = cm_mgr_read(CPU0_STALL_COUNTER + 4 * (cpu - 4));
	}

	return val;
}

int cm_mgr_check_stall_ratio(int mp0, int mp2)
{
	unsigned int i;
	unsigned int clustor;
	unsigned int stall_val_new;
#ifdef USE_TIME_NS
	unsigned long long time_ns_new;
#endif

	pstall_all->clustor[0] = mp0;
	pstall_all->clustor[1] = mp2;
	pstall_all->cpu = 0;
	for (i = 0; i < CM_MGR_CPU_CLUSTER; i++) {
		pstall_all->ratio_max[i] = 0;
		pstall_all->cpu_count[i] = 0;
	}

	for (i = 0; i < CM_MGR_CPU_COUNT; i++) {
		pstall_all->ratio[i] = 0;
		clustor = i / 4;

		stall_val_new = cm_mgr_read_stall(i);

		if (stall_val_new == 0 || stall_val_new == 0xdeadbeef) {
#ifdef USE_DEBUG_LOG
			pr_debug("%s: WARN!!! stall_val_new is 0x%08x\n", __func__, stall_val_new);
			debug_stall(i);
#endif
			continue;
		}

#ifdef USE_TIME_NS
		time_ns_new = sched_clock();
		pstall_all->time_ns_diff[i] = time_ns_new - pstall_all->time_ns[i];
		pstall_all->time_ns[i] = time_ns_new;
#endif

		diff_value_overflow(pstall_all->stall_val_diff[i], stall_val_new, pstall_all->stall_val[i]);
		pstall_all->stall_val[i] = stall_val_new;

		if (pstall_all->stall_val_diff[i] == 0) {
#ifdef USE_DEBUG_LOG
			pr_debug("%s: WARN!!! cpu:%d diff == 0\n", __func__, i);
			debug_stall(i);
#endif
			continue;
		}

#ifdef CONFIG_ARM64
		pstall_all->ratio[i] = pstall_all->stall_val_diff[i] * 100000 /
			pstall_all->time_ns_diff[i] / pstall_all->clustor[clustor];
#else
		pstall_all->ratio[i] = pstall_all->stall_val_diff[i] * 100000;
		do_div(pstall_all->ratio[i], pstall_all->time_ns_diff[i]);
		do_div(pstall_all->ratio[i], pstall_all->clustor[clustor]);
#endif
		if (pstall_all->ratio[i] > 100) {
#ifdef USE_DEBUG_LOG
			pr_debug("%s: WARN!!! cpu:%d ratio > 100\n", __func__, i);
			debug_stall(i);
#endif
			pstall_all->ratio[i] = 100;
			/* continue; */
		}

		pstall_all->cpu |= (1 << i);
		pstall_all->cpu_count[clustor]++;
		pstall_all->ratio_max[clustor] = CM_MGR_MAX(pstall_all->ratio[i], pstall_all->ratio_max[clustor]);
#ifdef USE_DEBUG_LOG
		debug_stall(i);
#endif
	}

#ifdef USE_DEBUG_LOG
	debug_stall_all();
#endif
	return 0;
}

int cm_mgr_register_init(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg_mp0_counter");
	if (!node)
		pr_info("find mp0_counter node failed\n");
	mcucfg_mp0_counter_base = of_iomap(node, 0);
	if (!mcucfg_mp0_counter_base) {
		pr_info("base mcucfg_mp0_counter_base failed\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg_mp2_counter");
	if (!node)
		pr_info("find mp2_counter node failed\n");
	mcucfg_mp2_counter_base = of_iomap(node, 0);
	if (!mcucfg_mp2_counter_base) {
		pr_info("base mcucfg_mp2_counter_base failed\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node)
		pr_info("find mp2_counter node failed\n");
	spm_sleep_base = of_iomap(node, 0);
	if (!spm_sleep_base) {
		pr_info("base spm_sleep_base failed\n");
		return -1;
	}

	return 0;
}

int cm_mgr_platform_init(void)
{
	int r;

	r = cm_mgr_register_init();
	if (r) {
		pr_info("FAILED TO CREATE REGISTER(%d)\n", r);
		return r;
	}

	mt_cpufreq_set_governor_freq_registerCB(check_cm_mgr_status);

	return r;
}
