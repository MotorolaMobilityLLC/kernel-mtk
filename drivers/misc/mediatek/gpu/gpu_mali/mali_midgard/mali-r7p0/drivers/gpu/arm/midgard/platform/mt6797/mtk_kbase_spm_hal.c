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

#include "mtk_kbase_spm.h"

#include <mt_gpufreq.h>
#include <mtk_mali_config.h>

#ifdef MTK_GPU_SPM

static unsigned int max_level = 7;
// highest freq => idx 0, level num - 1
//  lowest freq => idx num - 1, level 0
#define TRANS2IDX(level) (max_level - (level))

static struct {
	int idx_fix;
	unsigned int idx_floor;
	unsigned int idx_floor_cust;
	unsigned int idx_ceiling;
	unsigned int idx_ceiling_thermal;

	unsigned int loading;
	unsigned int block;
	unsigned int idle;
} glo = {
	.idx_floor = 0,
	.idx_floor_cust = 0,
	.idx_ceiling = 0,
	.idx_ceiling_thermal = 0,

	.loading = 0,
	.block = 0,
	.idle = 0
};

extern struct mtk_config *g_config;

#define MTKCLK_prepare_enable(clk) \
	if (config->clk) { if (clk_prepare_enable(config->clk)) \
		pr_alert("MALI: clk_prepare_enable failed when enabling " #clk ); }

#define MTKCLK_disable_unprepare(clk) \
	if (config->clk) {  clk_disable_unprepare(config->clk); }

static void spm_update_fc(void)
{
	struct mtk_config *config = g_config;

	if (config)
	{
		unsigned int fv, ff, cv, cf;
		unsigned int fidx, cidx;

		if (glo.idx_fix != -1)
		{
			fidx = cidx = glo.idx_fix;
		}
		else
		{
			fidx = TRANS2IDX(0);
			cidx = TRANS2IDX(max_level);

			fidx = min(fidx, glo.idx_floor);
			fidx = min(fidx, glo.idx_floor_cust);

			cidx = max(cidx, glo.idx_ceiling);
			cidx = max(cidx, glo.idx_ceiling_thermal);
		}

		fv = mt_gpufreq_get_volt_by_idx(fidx) / 100; // div 100 to mA
		ff = mt_gpufreq_get_freq_by_idx(fidx);

		cv = mt_gpufreq_get_volt_by_idx(cidx) / 100; // div 100 to mA
		cf = mt_gpufreq_get_freq_by_idx(cidx);

		MTKCLK_prepare_enable(clk_dvfs_gpu);
		mtk_kbase_spm_set_vol_freq_cf(cv, cf, fv, ff);
		MTKCLK_disable_unprepare(clk_dvfs_gpu);
	}
}

static void spm_boost(unsigned int idx)
{
	struct mtk_config *config = g_config;

	if (config)
	{
		MTKCLK_prepare_enable(clk_dvfs_gpu);
		mtk_kbase_spm_boost(idx, 40);
		MTKCLK_disable_unprepare(clk_dvfs_gpu);
	}
}

static void spm_mtk_gpu_power_limit_CB(unsigned int index)
{
	glo.idx_ceiling_thermal = index;
	spm_update_fc();
}
static void spm_mtk_gpu_input_boost_CB(unsigned int index)
{
	spm_boost(TRANS2IDX(max_level));
}

extern void (*mtk_boost_gpu_freq_fp)(void);
static void ssspm_mtk_boost_gpu_freq(void)
{
	spm_boost(TRANS2IDX(max_level));
}

void mtk_gpu_spm_fix_freq(unsigned int idx)
{
	glo.idx_floor = idx;
	glo.idx_ceiling = idx;
	spm_update_fc();
}
EXPORT_SYMBOL(mtk_gpu_spm_fix_freq);

extern unsigned int (*mtk_custom_get_gpu_freq_level_count_fp)(void);
static unsigned int ssspm_mtk_custom_get_gpu_freq_level_count(void)
{
	return max_level + 1;
}

extern void (*mtk_custom_boost_gpu_freq_fp)(unsigned int level);
static void ssspm_mtk_custom_boost_gpu_freq(unsigned int level)
{
	glo.idx_floor_cust = TRANS2IDX(level);
	spm_update_fc();
}

extern unsigned int (*mtk_get_custom_boost_gpu_freq_fp)(void);
static unsigned int ssspm_mtk_get_custom_boost_gpu_freq(void)
{
	return TRANS2IDX(glo.idx_floor_cust);
}

extern void (*mtk_custom_upbound_gpu_freq_fp)(unsigned int level);
static void ssspm_mtk_custom_upbound_gpu_freq(unsigned int level)
{
	glo.idx_ceiling = TRANS2IDX(level);
	spm_update_fc();
}

extern unsigned int (*mtk_get_custom_upbound_gpu_freq_fp)(void);
static unsigned int ssspm_mtk_get_custom_upbound_gpu_freq(void)
{
	return TRANS2IDX(glo.idx_ceiling);
}

static unsigned long long ssspm_get_time(void)
{
	unsigned long long temp;

	preempt_disable();
	temp = cpu_clock(smp_processor_id());
	preempt_enable();

	return temp;
}

void MTKCalGpuUtilization(unsigned int* pui32Loading, unsigned int* pui32Block, unsigned int* pui32Idle);
static void ssspm_update_loading(void)
{
	static unsigned long long lasttime = 0;
	unsigned long long now, diff;
	unsigned long rem;

	now = ssspm_get_time();

	diff = now - lasttime;
	rem = do_div(diff, 1000000); // ms

	// update loading if > n ms
	if (diff >= 16)
	{
		MTKCalGpuUtilization(&glo.loading, &glo.block, &glo.idle);
		lasttime = now;
	}
}
extern unsigned int (*mtk_get_gpu_loading_fp)(void);
static unsigned int ssspm_mtk_get_gpu_loading(void)
{
	ssspm_update_loading();
	return glo.loading;
}

extern unsigned int (*mtk_get_gpu_block_fp)(void);
static unsigned int ssspm_mtk_get_gpu_block(void)
{
	ssspm_update_loading();
	return glo.block;
}

extern unsigned int (*mtk_get_gpu_idle_fp)(void);
static unsigned int ssspm_mtk_get_gpu_idle(void)
{
	ssspm_update_loading();
	return glo.idle;
}

static struct workqueue_struct     *g_update_volt_workqueue = NULL;
static struct work_struct          g_update_volt_work;

static void update_volt_Handle(struct work_struct *_psWork)
{
	struct mtk_config *config = g_config;

	if (config)
	{
		MTKCLK_prepare_enable(clk_dvfs_gpu);
		mtk_kbase_spm_update_table();
		MTKCLK_disable_unprepare(clk_dvfs_gpu);
	}
}
static void spm_mtk_gpu_ptpod_update_notify(void)
{
	if (g_update_volt_workqueue)
	{
		queue_work(g_update_volt_workqueue, &g_update_volt_work);
	}
}

void mtk_kbase_spm_hal_init(void)
{
	g_update_volt_workqueue = alloc_ordered_workqueue("mali_wp_update_volt", WQ_FREEZABLE | WQ_MEM_RECLAIM);
	INIT_WORK(&g_update_volt_work, update_volt_Handle);

	max_level = mt_gpufreq_get_dvfs_table_num() - 1;

	glo.idx_fix = -1;

	glo.idx_floor   	= TRANS2IDX(0);
	glo.idx_floor_cust  = TRANS2IDX(0);
	glo.idx_ceiling 	= TRANS2IDX(max_level);
	glo.idx_ceiling_thermal = TRANS2IDX(max_level);

	// hal
	mtk_boost_gpu_freq_fp = ssspm_mtk_boost_gpu_freq;
	mtk_custom_get_gpu_freq_level_count_fp = ssspm_mtk_custom_get_gpu_freq_level_count;
	mtk_custom_boost_gpu_freq_fp = ssspm_mtk_custom_boost_gpu_freq;
	mtk_get_custom_boost_gpu_freq_fp = ssspm_mtk_get_custom_boost_gpu_freq;
	mtk_custom_upbound_gpu_freq_fp = ssspm_mtk_custom_upbound_gpu_freq;
	mtk_get_custom_upbound_gpu_freq_fp = ssspm_mtk_get_custom_upbound_gpu_freq;
	mtk_get_gpu_loading_fp = ssspm_mtk_get_gpu_loading;
	mtk_get_gpu_block_fp = ssspm_mtk_get_gpu_block;
	mtk_get_gpu_idle_fp = ssspm_mtk_get_gpu_idle;

	// thermal
	mt_gpufreq_power_limit_notify_registerCB(spm_mtk_gpu_power_limit_CB);
	// touch boost
	mt_gpufreq_input_boost_notify_registerCB(spm_mtk_gpu_input_boost_CB);
	// voltage
  	mt_gpufreq_update_volt_registerCB(spm_mtk_gpu_ptpod_update_notify);
}

void mtk_gpu_spm_resume_hal(void)
{
	spm_update_fc();
}

void mtk_gpu_spm_fix_by_idx(unsigned int idx)
{
	glo.idx_fix = idx;
	spm_update_fc();
}
void mtk_gpu_spm_reset_fix(void)
{
	glo.idx_fix = -1;
	spm_update_fc();
}

void mtk_gpu_spm_pause(void)
{
	/* do nothing */
	MTK_err("spm_pause");
}
void mtk_gpu_spm_resume(void)
{
	/* do nothing */
	MTK_err("spm_resume");
}

#endif
