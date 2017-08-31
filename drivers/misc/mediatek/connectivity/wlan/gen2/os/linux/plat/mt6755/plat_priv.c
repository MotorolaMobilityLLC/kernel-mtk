/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include "mach/mt_ppm_api.h"
#include "mt_idle.h"

#define MAX_CPU_CORE_NUM 8
#define MAX_CPU_FREQ     1500000000

static unsigned int g_force_core_num;
static unsigned int g_force_core_freq;

int kalBoostCpu(unsigned int core_num)
{
	unsigned long freq = MAX_CPU_FREQ;

	if (core_num >= 2)
		idle_lock_by_conn(1);/* Disable deepidle/SODI when throughput > 40Mbps */
	else
		idle_lock_by_conn(0);/* Enable deepidle/SODI */

	if (g_force_core_num || g_force_core_freq) {
		core_num = g_force_core_num;
		freq = g_force_core_freq;
	} else {
		if (core_num == 1)
			core_num++;
		freq = core_num == 0 ? 0 : freq;
	}

	if (core_num > MAX_CPU_CORE_NUM)
		core_num = MAX_CPU_CORE_NUM;
	pr_warn("enter kalBoostCpu, core_num:%d, freq:%ld\n", core_num, freq);

	mt_ppm_sysboost_core(BOOST_BY_WIFI, core_num);
	mt_ppm_sysboost_freq(BOOST_BY_WIFI, freq);

	return 0;
}

int kalSetCpuNumFreq(unsigned int core_num, unsigned int freq)
{
	pr_warn("enter kalSetCpuNumFreq, core_num:%d, freq:%d\n", core_num, freq);

	g_force_core_num = core_num;
	g_force_core_freq = freq;

	kalBoostCpu(g_force_core_num);

	return 0;
}

