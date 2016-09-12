/*
 * Copyright (C) 2015-2016 MediaTek Inc.
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

unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	return 0;
}

void mt_gpufreq_enable_by_ptpod(void)
{
}

void mt_gpufreq_disable_by_ptpod(void)
{
}

void mt_gpufreq_restore_default_volt(void)
{
}

unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[],
				    unsigned int array_size)
{
	return 0;
}

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	return 0;
}

int mt_gpufreq_state_set(int enabled)
{
	return 0;
}

unsigned int mt_gpufreq_target(unsigned int idx)
{
	return 0;
}

void mt_gpufreq_thermal_protect(unsigned int limited_power)
{
}

unsigned int mt_gpufreq_get_max_power(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_min_power(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_thermal_limit_index(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_thermal_limit_freq(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_cur_freq_index(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_cur_freq(void)
{
	return 0;
}

unsigned int mt_gpufreq_get_cur_volt(void)
{
	return 0;
}

void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power)
{
}

unsigned int mt_gpufreq_get_leakage_mw(void)
{
	return 0;
}

typedef void (*gpufreq_input_boost_notify)(unsigned int);
void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB)
{
}

typedef void (*gpufreq_power_limit_notify)(unsigned int);
void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB)
{
}

typedef void (*sampler_func)(unsigned int);
void mt_gpufreq_setfreq_registerCB(sampler_func pCB)
{
}

void mt_gpufreq_setvolt_registerCB(sampler_func pCB)
{
}

