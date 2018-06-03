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
#include "mtk_cpufreq_internal.h"
#include "mtk_cpufreq_hybrid.h"

#if 0
/* #ifdef CONFIG_HYBRID_CPU_DVFS */
enum mt_cpu_dvfs_id group(unsigned int cpu)
{
	enum mt_cpu_dvfs_id id;

	id = (cpu > 7) ? MT_CPU_DVFS_B :
		(cpu > 4) ? MT_CPU_DVFS_L : MT_CPU_DVFS_LL;

	return id;
}
#endif

#if 0
int mt_cpufreq_set_by_schedule_load(unsigned int cpu, enum cpu_dvfs_sched_type state, unsigned int freq)
{
#ifdef CONFIG_HYBRID_CPU_DVFS
	enum mt_cpu_dvfs_id id = group(cpu);

	if (freq < mt_cpufreq_get_freq_by_idx(id, 15))
		freq = mt_cpufreq_get_freq_by_idx(id, 15);

	if (freq > mt_cpufreq_get_freq_by_idx(id, 0))
		freq = mt_cpufreq_get_freq_by_idx(id, 0);

	if (state < NUM_SCHE_TYPE)
		cpuhvfs_set_cpu_load_freq(cpu, state, freq);
#endif

	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_set_by_schedule_load);
#else
int mt_cpufreq_set_by_schedule_load_cluster(unsigned int cluster_id, unsigned int freq)
{
#ifdef CONFIG_HYBRID_CPU_DVFS
	enum mt_cpu_dvfs_id id = (enum mt_cpu_dvfs_id) cluster_id;

	if (freq < mt_cpufreq_get_freq_by_idx(id, 15))
		freq = mt_cpufreq_get_freq_by_idx(id, 15);

	if (freq > mt_cpufreq_get_freq_by_idx(id, 0))
		freq = mt_cpufreq_get_freq_by_idx(id, 0);

	cpuhvfs_set_cluster_load_freq(id, freq);
#endif

	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_set_by_schedule_load_cluster);
#endif

int mt_cpufreq_set_iccs_frequency_by_cluster(int en, unsigned int cluster_id, unsigned int freq)
{
#ifdef CONFIG_HYBRID_CPU_DVFS
	enum mt_cpu_dvfs_id id = (enum mt_cpu_dvfs_id) cluster_id;

	if (!en)
		freq = mt_cpufreq_get_freq_by_idx(id, 15);

	if (freq < mt_cpufreq_get_freq_by_idx(id, 15))
		freq = mt_cpufreq_get_freq_by_idx(id, 15);

	if (freq > mt_cpufreq_get_freq_by_idx(id, 0))
		freq = mt_cpufreq_get_freq_by_idx(id, 0);

	cpuhvfs_set_iccs_freq(id, freq);
#endif

	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_set_iccs_frequency_by_cluster);

int is_in_suspend(void)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);

	return p->dvfs_disable_by_suspend;
}
EXPORT_SYMBOL(is_in_suspend);

int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl, int nr_volt_tbl)
{
#ifdef EEM_AP2SSPM
	FUNC_ENTER(FUNC_LV_API);

	cpuhvfs_update_volt((unsigned int)id, volt_tbl, nr_volt_tbl);
#else
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	_mt_cpufreq_dvfs_request_wrapper(p, p->idx_opp_tbl, MT_CPU_DVFS_EEM_UPDATE,
		(void *)&volt_tbl);
#endif

	FUNC_EXIT(FUNC_LV_API);

	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_update_volt);

cpuVoltsampler_func g_pCpuVoltSampler_met;
cpuVoltsampler_func g_pCpuVoltSampler_ocp;
void notify_cpu_volt_sampler(enum mt_cpu_dvfs_id id, unsigned int volt, int up, int event)
{
	unsigned int mv = volt / 100;

	if (g_pCpuVoltSampler_met)
		g_pCpuVoltSampler_met(id, mv, up, event);

	if (g_pCpuVoltSampler_ocp)
		g_pCpuVoltSampler_ocp(id, mv, up, event);
}

/* cpu voltage sampler */
void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB)
{
	g_pCpuVoltSampler_met = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_setvolt_registerCB);

/* ocp cpu voltage sampler */
void mt_cpufreq_setvolt_ocp_registerCB(cpuVoltsampler_func pCB)
{
	g_pCpuVoltSampler_ocp = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_setvolt_ocp_registerCB);

/* for PTP-OD */
static mt_cpufreq_set_ptbl_funcPTP mt_cpufreq_update_private_tbl;

void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB)
{
	mt_cpufreq_update_private_tbl = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_set_ptbl_registerCB);


unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id)
{
#if 0
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	struct buck_ctrl_t *vproc_p = id_to_buck_ctrl(p->Vproc_buck_id);

	return vproc_p->buck_ops->get_cur_volt(vproc_p);
#else
#ifdef CONFIG_HYBRID_CPU_DVFS
	return cpuhvfs_get_cur_volt(id);
#else
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	struct buck_ctrl_t *vproc_p = id_to_buck_ctrl(p->Vproc_buck_id);

	return vproc_p->cur_volt;
#endif
#endif
}
EXPORT_SYMBOL(mt_cpufreq_get_cur_volt);

unsigned int mt_cpufreq_get_cur_freq(enum mt_cpu_dvfs_id id)
{
#ifdef CONFIG_HYBRID_CPU_DVFS
	int freq_idx = cpuhvfs_get_cur_dvfs_freq_idx(id);

	if (freq_idx < 0)
		freq_idx = 0;

	if (freq_idx > 15)
		freq_idx = 15;

	return mt_cpufreq_get_freq_by_idx(id, freq_idx);
#else
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	return cpu_dvfs_get_cur_freq(p);
#endif
}
EXPORT_SYMBOL(mt_cpufreq_get_cur_freq);

unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	FUNC_EXIT(FUNC_LV_API);

	return cpu_dvfs_get_freq_by_idx(p, idx);
}
EXPORT_SYMBOL(mt_cpufreq_get_freq_by_idx);

unsigned int mt_cpufreq_get_volt_by_idx(enum mt_cpu_dvfs_id id, int idx)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	FUNC_EXIT(FUNC_LV_API);

	return cpu_dvfs_get_volt_by_idx(p, idx);
}
EXPORT_SYMBOL(mt_cpufreq_get_volt_by_idx);

unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned int freq = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	freq = cpu_dvfs_get_cur_freq(p);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return freq;
}
EXPORT_SYMBOL(mt_cpufreq_get_cur_phy_freq_no_lock);

/* for PBM */
unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id)
{
# if 0
#ifndef DISABLE_PBM_FEATURE
	struct mt_cpu_dvfs *p;
	int temp;
	int mw = 0;
	int i;

	if (id == 0) {
		for_each_cpu_dvfs_only(i, p) {
			if (cpu_dvfs_is(p, MT_CPU_DVFS_LL) && p->armpll_is_available) {
				temp = tscpu_get_temp_by_bank(THERMAL_BANK4) / 1000;
				mw += mt_spower_get_leakage(MT_SPOWER_CPULL, cpu_dvfs_get_cur_volt(p) / 100, temp);
			} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L) && p->armpll_is_available) {
				temp = tscpu_get_temp_by_bank(THERMAL_BANK3) / 1000;
				mw += mt_spower_get_leakage(MT_SPOWER_CPUL, cpu_dvfs_get_cur_volt(p) / 100, temp);
			} else if (cpu_dvfs_is(p, MT_CPU_DVFS_B) && p->armpll_is_available) {
				temp = tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000;
				mw += mt_spower_get_leakage(MT_SPOWER_CPUBIG, cpu_dvfs_get_cur_volt(p) / 100, temp);
			}
		}
	} else if (id > 0) {
		id = id - 1;
		p = id_to_cpu_dvfs(id);
		if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			temp = tscpu_get_temp_by_bank(THERMAL_BANK4) / 1000;
			mw = mt_spower_get_leakage(MT_SPOWER_CPULL, cpu_dvfs_get_cur_volt(p) / 100, temp);
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L)) {
			temp = tscpu_get_temp_by_bank(THERMAL_BANK3) / 1000;
			mw = mt_spower_get_leakage(MT_SPOWER_CPUL, cpu_dvfs_get_cur_volt(p) / 100, temp);
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_B)) {
			temp = tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000;
			mw = mt_spower_get_leakage(MT_SPOWER_CPUBIG, cpu_dvfs_get_cur_volt(p) / 100, temp);
		}
	}
	return mw;
#else
	return 0;
#endif
#endif
	return 0;
}

int mt_cpufreq_get_ppb_state(void)
{
	return dvfs_power_mode;
}
