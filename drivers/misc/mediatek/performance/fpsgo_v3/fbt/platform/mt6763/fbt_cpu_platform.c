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
#include <mtk_vcorefs_governor.h>
#include <mtk_vcorefs_manager.h>
#include <mach/mtk_ppm_api.h>
#include <mtk_dramc.h>

int ultra_req;
int cm_req;
int cm_req_opp;
/* thres for 6763
 * ddr3
 * ll0 = 1.6G(67)
 * ll1 = NA
 * l0  = 1.5G(70)
 * l1  = NA

 * ddr4
 * ll0 = 0.806G(33)
 * ll1 = 1.6G(67)
 * l0  = 1G(41)
 * l1  = 1.5G(70)
 *
 * ddr3 opp0: 101 (never)
 * ddr3 opp1: 67  (ll 1.6G)
 * ddr4 opp1: 33  (ll 0.8G)
 * ddr4 opp0: 67  (ll 1.6G)
 *
 */
static int cpi_thres = 250;
static unsigned int cpi_uclamp_thres_opp1_ddr3 = 67;
static unsigned int cpi_uclamp_thres_opp1_ddr4 = 33;
static unsigned int cpi_uclamp_thres_opp0_ddr4 = 67;


void fbt_notify_CM_limit(int reach_limit)
{
}

void fbt_reg_dram_request(int reg)
{
}

void fbt_dram_arbitration(void)
{
	int ret = -1;

	if (ultra_req)
		ret = vcorefs_request_dvfs_opp(KIR_FBT, 0);
	else if (cm_req)
		ret = vcorefs_request_dvfs_opp(KIR_FBT, cm_req_opp);
	else
		ret = vcorefs_request_dvfs_opp(KIR_FBT, -1);

	if (ret < 0)
		fpsgo_systrace_c_fbt_gm(-100, ret, "fbt_dram_arbitration_ret");

	fpsgo_systrace_c_fbt_gm(-100, cm_req, "cm_req");
	fpsgo_systrace_c_fbt_gm(-100, cm_req_opp, "cm_req_opp");
	fpsgo_systrace_c_fbt_gm(-100, ultra_req, "ultra_req");
}

void fbt_boost_dram(int boost)
{

	if (boost == ultra_req)
		return;

	ultra_req = boost;
	fbt_dram_arbitration();

}

void fbt_set_boost_value(unsigned int base_blc)
{
	int cpi = 0;
	unsigned int cpi_base_blc = clamp(base_blc, 1U, 100U);

	base_blc = (base_blc << 10) / 100U;
	base_blc = clamp(base_blc, 1U, 1024U);
#ifdef CONFIG_CGROUP_SCHEDTUNE
	capacity_min_write_for_perf_idx(CGROUP_TA, (int)base_blc);
#endif
	fpsgo_systrace_c_fbt_gm(-100, base_blc, "TA_cap");


	/* dual single cluster for mt6739 */
	cpi = max(ppm_get_cluster_cpi(0), ppm_get_cluster_cpi(1));
	fpsgo_systrace_c_fbt_gm(-100, cpi, "cpi");
	if (get_ddr_type() == TYPE_LPDDR3) {
		if (cpi > cpi_thres &&
				cpi_base_blc > cpi_uclamp_thres_opp1_ddr3) {
			cm_req = 1;
			cm_req_opp = 1;
		} else {
			cm_req = 0;
			cm_req_opp = -1;
		}
	} else {
		if (cpi > cpi_thres &&
				cpi_base_blc > cpi_uclamp_thres_opp0_ddr4) {
			cm_req = 1;
			cm_req_opp = 0;
		} else if (cpi > cpi_thres &&
				cpi_base_blc > cpi_uclamp_thres_opp1_ddr4){
			cm_req = 1;
			cm_req_opp = 1;
		} else {
			cm_req = 0;
			cm_req_opp = -1;
		}
	}

	fbt_dram_arbitration();
}

void fbt_clear_boost_value(void)
{
#ifdef CONFIG_CGROUP_SCHEDTUNE
	capacity_min_write_for_perf_idx(CGROUP_TA, 0);
#endif
	fpsgo_systrace_c_fbt_gm(-100, 0, "TA_cap");

	cm_req = 0;
	cm_req_opp = -1;
	ultra_req = 0;
	fbt_notify_CM_limit(0);
	fbt_dram_arbitration();
}

/*
 * mt6763 use boost_ta to support CM optimize.
 * per-task uclamp doesn't support CM optimize.
 */
void fbt_set_per_task_min_cap(int pid, unsigned int base_blc)
{
	int ret = -1;

	if (!pid)
		return;

#ifdef CONFIG_UCLAMP_TASK
	ret = set_task_uclamp(pid, base_blc);
#endif
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

int fbt_get_L_min_ceiling(void)
{
	int freq = 0;

	return freq;
}

int fbt_get_default_boost_ta(void)
{
	return 1;
}
