/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "eas_ctrl.h"
#include "fbt_cpu_platform.h"
#include <fpsgo_common.h>
#include <linux/pm_qos.h>

static struct pm_qos_request dram_req;

void fbt_notify_CM_limit(int reach_limit)
{
	cm_mgr_perf_set_status(reach_limit);
	fpsgo_systrace_c_fbt_gm(-100, reach_limit, "notify_cm");
}

void fbt_reg_dram_request(int reg)
{
	if (reg) {
		if (!pm_qos_request_active(&dram_req))
			pm_qos_add_request(&dram_req, PM_QOS_DDR_OPP,
					PM_QOS_DDR_OPP_DEFAULT_VALUE);
	} else {
		if (pm_qos_request_active(&dram_req))
			pm_qos_remove_request(&dram_req);
	}
}

void fbt_boost_dram(int boost)
{
	if (!pm_qos_request_active(&dram_req)) {
		fbt_reg_dram_request(1);
		if (!pm_qos_request_active(&dram_req)) {
			fpsgo_systrace_c_fbt_gm(-100, -1, "dram_boost");
			return;
		}
	}

	if (boost)
		pm_qos_update_request(&dram_req, 0);
	else
		pm_qos_update_request(&dram_req,
				PM_QOS_DDR_OPP_DEFAULT_VALUE);

	fpsgo_systrace_c_fbt_gm(-100, boost, "dram_boost");
}

void fbt_set_boost_value(unsigned int base_blc)
{
	base_blc = (base_blc << 10) / 100U;
	base_blc = clamp(base_blc, 1U, 1024U);
#ifdef CONFIG_CGROUP_SCHEDTUNE
	capacity_min_write_for_perf_idx(CGROUP_TA, (int)base_blc);
#endif
	fpsgo_systrace_c_fbt_gm(-100, base_blc, "TA_cap");
}

void fbt_clear_boost_value(void)
{
#ifdef CONFIG_CGROUP_SCHEDTUNE
	capacity_min_write_for_perf_idx(CGROUP_TA, 0);
#endif
	fpsgo_systrace_c_fbt_gm(-100, 0, "TA_cap");

	fbt_notify_CM_limit(0);
	fbt_boost_dram(0);
}

void fbt_set_per_task_min_cap(int pid, unsigned int base_blc)
{
	int ret;

	if (!pid)
		return;

	ret = set_task_uclamp(pid, base_blc);
	if (ret != 0) {
		fpsgo_systrace_c_fbt(pid, ret, "uclamp fail");
		fpsgo_systrace_c_fbt(pid, 0, "uclamp fail");
	}

	fpsgo_systrace_c_fbt_gm(pid, base_blc, "min_cap");
}

int fbt_get_L_cluster_num(void)
{
	return 0;
}

