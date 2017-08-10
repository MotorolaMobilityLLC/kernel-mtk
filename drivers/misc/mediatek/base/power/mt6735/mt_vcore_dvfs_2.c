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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <mach/mt_pmic_wrap.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_freqhopping.h>
#include <mach/upmu_hw.h>
#include <mt_dramc.h>

#include "mt_vcore_dvfs.h"
#include "mt_cpufreq.h"
#include "mt_spm.h"
#include "mt_ptp.h"

/**************************************
 * Macro and Inline
 **************************************/
#define TAG	"[VcoreFS] "
#define tag_pr_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define tag_pr_notice(fmt, args...)	pr_notice(TAG fmt, ##args)
#define tag_pr_info(fmt, args...)	pr_info(TAG fmt, ##args)
#define tag_pr_debug(fmt, args...)	pr_debug(TAG fmt, ##args)

/* log_mask[15:0]: show nothing, log_mask[31:16]: show only on MobileLog */
#define vcorefs_crit_mask(fmt, args...)				\
do {								\
	if (pwrctrl->log_mask & (1U << kicker))			\
		;						\
	else if ((pwrctrl->log_mask >> 16) & (1U << kicker))	\
		tag_pr_debug(fmt, ##args);			\
	else							\
		tag_pr_info(fmt, ##args);			\
} while (0)

#define DEFINE_ATTR_RO(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define DEFINE_ATTR_RW(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define __ATTR_OF(_name)	(&_name##_attr.attr)

#define cmd_cmp(cmd, str)	strncmp(cmd, str, sizeof(str) - 1)

/**************************************
 * Define and Declare
 **************************************/
struct vcorefs_profile {
	/* for framework */
	bool recover_en;
	bool vcore_dvs;
	bool freq_dfs;
	bool ddr_dfs;
	bool sdio_lock;
	int error_code;
	unsigned int log_mask;

	/* for Stay-LV */
	bool late_init_opp_done;
	bool init_opp_perf;
	unsigned int kr_req_mask;
	unsigned int late_init_opp;

	/* for Vcore control */
	unsigned int curr_vcore_uv;

	/* for Freq control */
	unsigned int curr_ddr_khz;
	unsigned int curr_venc_khz;
	unsigned int curr_axi_khz;
	unsigned int curr_qtrhalf_khz;
};

struct opp_profile {
	unsigned int vcore_uv;
	unsigned int ddr_khz;
	unsigned int venc_khz;
	unsigned int axi_khz;
	unsigned int qtrhalf_khz;
};

struct kicker_profile {
	int opp;
};

/**************************************
 *
 **************************************/
static DEFINE_MUTEX(vcorefs_mutex);

/**************************************
 * __nosavedata will not be restored after IPO-H boot
 **************************************/
static unsigned int trans[NUM_TRANS] __nosavedata;
static bool feature_en __nosavedata = 1;	/* if feature disable, then keep HPM */

static unsigned int vcorefs_curr_opp __nosavedata = OPPI_PERF;
static unsigned int vcorefs_prev_opp __nosavedata = OPPI_PERF;

static struct vcorefs_profile vcorefs_ctrl = {
	.vcore_dvs		= 1,
	.freq_dfs		= 1,
	.ddr_dfs		= 1,
	.log_mask		= 0xffff0000 | (1U << KIR_GPU),

	.late_init_opp_done	= 0,
	.init_opp_perf		= 0,
	.kr_req_mask		= 0,
	.late_init_opp		= OPPI_LOW_PWR,

	.curr_vcore_uv		= VCORE_1_P_15_UV,

	.curr_ddr_khz		= FDDR_S0_KHZ,
#ifndef VCOREFS_FVENC_NOCTRL
	.curr_venc_khz		= FVENC_S0_KHZ,
#endif
	.curr_axi_khz		= FAXI_S0_KHZ,
	.curr_qtrhalf_khz	= FQTRHALF_S0_KHZ,
};

static struct opp_profile opp_table[] __nosavedata = {
	[OPP_0] = {	/* ultra-performance mode */
		.vcore_uv	= VCORE_1_P_25_UV,
		.ddr_khz	= FDDR_S0_KHZ,
		.venc_khz	= FVENC_S0_KHZ,
		.axi_khz	= FAXI_S0_KHZ,
		.qtrhalf_khz	= FQTRHALF_S0_KHZ,
	},
	[OPP_1] = {	/* performance mode */
		.vcore_uv	= VCORE_1_P_15_UV,
		.ddr_khz	= FDDR_S0_KHZ,
		.venc_khz	= FVENC_S0_KHZ,
		.axi_khz	= FAXI_S0_KHZ,
		.qtrhalf_khz	= FQTRHALF_S0_KHZ,
	},
	[OPP_2] = {	/* low power mode */
		.vcore_uv	= VCORE_1_P_05_UV,
		.ddr_khz	= FDDR_S1_KHZ,
		.venc_khz	= FVENC_S1_KHZ,
		.axi_khz	= FAXI_S1_KHZ,
		.qtrhalf_khz	= FQTRHALF_S1_KHZ,
	}
};

static struct kicker_profile kicker_table[] = {
	[KIR_GPU] = {
		.opp	= OPP_OFF,
	},
	[KIR_MM] = {
		.opp	= OPP_OFF,
	},
	[KIR_EMIBW] = {
		.opp	= OPP_OFF,
	},
	[KIR_SDIO] = {
		.opp	= OPP_OFF,
	},
	[KIR_WIFI] = {
		.opp	= OPP_OFF,
	},
	[KIR_SYSFS] = {
		.opp	= OPP_OFF,
	}
};

/**************************************
 * Vcore Control Function
 **************************************/
static unsigned int get_vcore_uv(void)
{
	unsigned int vcore = VCORE_INVALID;

	pwrap_read(MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, &vcore);
	if (vcore >= VCORE_INVALID)	/* try again */
		pwrap_read(MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, &vcore);

	return vcore < VCORE_INVALID ? vcore_pmic_to_uv(vcore) : 0;
}

static void update_vcore_pwrap_cmd(struct opp_profile *opp_ctrl_table)
{
	unsigned int diff;

	diff = opp_ctrl_table[OPPI_PERF].vcore_uv - opp_ctrl_table[OPPI_LOW_PWR].vcore_uv;
	trans[TRANS1] = opp_ctrl_table[OPPI_LOW_PWR].vcore_uv + (diff / 3) * 1;
	trans[TRANS2] = opp_ctrl_table[OPPI_LOW_PWR].vcore_uv + (diff / 3) * 2;

	diff = opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv - opp_ctrl_table[OPPI_PERF].vcore_uv;
	trans[TRANS3] = opp_ctrl_table[OPPI_PERF].vcore_uv + (diff / 3) * 1;
	trans[TRANS4] = opp_ctrl_table[OPPI_PERF].vcore_uv + (diff / 3) * 2;

	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_UHPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS4, vcore_uv_to_pmic(trans[TRANS4]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS3, vcore_uv_to_pmic(trans[TRANS3]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_HPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS2, vcore_uv_to_pmic(trans[TRANS2]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS1, vcore_uv_to_pmic(trans[TRANS1]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_LPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_HPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_TRANS2, vcore_uv_to_pmic(trans[TRANS2]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_TRANS1, vcore_uv_to_pmic(trans[TRANS1]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_LPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_HPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_TRANS2, vcore_uv_to_pmic(trans[TRANS2]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_TRANS1, vcore_uv_to_pmic(trans[TRANS1]));
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_LPM,
				vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	tag_pr_info("UHPM  : %u (0x%x)\n", opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv,
						vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv));
	tag_pr_info("TRANS4: %u (0x%x)\n", trans[TRANS4], vcore_uv_to_pmic(trans[TRANS4]));
	tag_pr_info("TRANS3: %u (0x%x)\n", trans[TRANS3], vcore_uv_to_pmic(trans[TRANS3]));
	tag_pr_info("HPM   : %u (0x%x)\n", opp_ctrl_table[OPPI_PERF].vcore_uv,
						vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	tag_pr_info("TRANS2: %u (0x%x)\n", trans[TRANS2], vcore_uv_to_pmic(trans[TRANS2]));
	tag_pr_info("TRANS1: %u (0x%x)\n", trans[TRANS1], vcore_uv_to_pmic(trans[TRANS1]));
	tag_pr_info("LPM   : %u (0x%x)\n", opp_ctrl_table[OPPI_LOW_PWR].vcore_uv,
						vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));
}

/**************************************
 * External Control Function
 **************************************/
unsigned int vcorefs_get_curr_voltage(void)
{
	return get_vcore_uv();
}

unsigned int get_ddr_khz(void)
{
	int freq;

	freq = get_dram_data_rate();

	return freq * 1000;
}

unsigned int get_ddr_khz_by_steps(unsigned int step)
{
	int freq;

	freq = dram_fh_steps_freq(step);
	BUG_ON(freq < 0);

	return freq * 1000;
}

bool is_vcorefs_can_work(void)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	return pwrctrl->late_init_opp_done && feature_en;
}

/**************************************
 * Vcore DVS/DFS Function
 **************************************/
static int set_vcore_with_opp(enum dvfs_kicker kicker, unsigned int opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;
	int r;

	pwrctrl->curr_vcore_uv = get_vcore_uv();

	vcorefs_crit_mask("opp: %u, vcore: %u(%u) %s\n",
			  opp,
			  opp_ctrl_table[opp].vcore_uv, pwrctrl->curr_vcore_uv,
			  pwrctrl->vcore_dvs ? "" : "[X]");

	if (!pwrctrl->vcore_dvs)
		return 0;

	r = spm_set_vcore_dvs_voltage(opp);

	return r;
}

static int set_freq_with_opp(enum dvfs_kicker kicker, unsigned int opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;
	int r = 0;

	vcorefs_crit_mask("opp: %u, faxi: %u(%u)"
#ifndef VCOREFS_FVENC_NOCTRL
			  ", fvenc: %u(%u)"
#endif
			  " %s\n",
			  opp,
			  opp_ctrl_table[opp].axi_khz, pwrctrl->curr_axi_khz,
#ifndef VCOREFS_FVENC_NOCTRL
			  opp_ctrl_table[opp].venc_khz, pwrctrl->curr_venc_khz,
#endif
			  pwrctrl->freq_dfs ? "" : "[X]");

	if (!pwrctrl->freq_dfs)
		return 0;
	if (opp_ctrl_table[opp].axi_khz == pwrctrl->curr_axi_khz
#ifndef VCOREFS_FVENC_NOCTRL
	    && opp_ctrl_table[opp].venc_khz == pwrctrl->curr_venc_khz
#endif
	    )
		return 0;

	if (opp == OPPI_PERF_ULTRA || opp == OPPI_PERF) {
		clkmux_sel(MT_MUX_AXI, 2, "vcorefs");	/* 218MHz - SYSPLL_D5 */
		pwrctrl->curr_axi_khz = opp_ctrl_table[opp].axi_khz;
		pwrctrl->curr_qtrhalf_khz = opp_ctrl_table[opp].qtrhalf_khz;

#ifndef VCOREFS_FVENC_NOCTRL
		r = mt_dfs_vencpll(0x1713B1);	/* 300MHz */
		if (r)
			tag_pr_notice("*** FAILED: VENCPLL COULD NOT BE FREQ HOPPING ***\n");
		else
			pwrctrl->curr_venc_khz = opp_ctrl_table[opp].venc_khz;
#endif
	} else if (opp == OPPI_LOW_PWR) {
#ifndef VCOREFS_FVENC_NOCTRL
		r = mt_dfs_vencpll(0xE0000);	/* 182MHz */
		if (r)
			tag_pr_notice("*** FAILED: VENCPLL COULD NOT BE FREQ HOPPING ***\n");
		else
			pwrctrl->curr_venc_khz = opp_ctrl_table[opp].venc_khz;
#endif

		clkmux_sel(MT_MUX_AXI, 3, "vcorefs");	/* 136MHz - SYSPLL1_D4 */
		pwrctrl->curr_axi_khz = opp_ctrl_table[opp].axi_khz;
		pwrctrl->curr_qtrhalf_khz = opp_ctrl_table[opp].qtrhalf_khz;
	} else {
		return -EINVAL;
	}

	return r;
}

static int set_fddr_with_opp(enum dvfs_kicker kicker, unsigned int opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;
	int r;

	pwrctrl->curr_ddr_khz = get_ddr_khz();

	vcorefs_crit_mask("opp: %u, fddr: %u(%u) %s\n",
			  opp,
			  opp_ctrl_table[opp].ddr_khz, pwrctrl->curr_ddr_khz,
			  pwrctrl->ddr_dfs ? "" : "[X]");

	if (!pwrctrl->ddr_dfs)
		return 0;
	if (opp_ctrl_table[opp].ddr_khz == pwrctrl->curr_ddr_khz)
		return 0;

	r = dram_do_dfs_by_fh(opp_ctrl_table[opp].ddr_khz);

	if (r)
		tag_pr_notice("*** FAILED: DRAM COULD NOT BE FREQ HOPPING ***\n");
	else
		pwrctrl->curr_ddr_khz = opp_ctrl_table[opp].ddr_khz;

	return r;
}

/**************************************
 *
 **************************************/
static unsigned int find_min_opp(enum dvfs_kicker kicker)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_profile *kicker_ctrl_table = kicker_table;
	unsigned int min = UINT_MAX;
	int i;

	vcorefs_crit_mask("[%d, %d, %d, %d, %d, %d]\n",
				kicker_ctrl_table[KIR_GPU].opp,
				kicker_ctrl_table[KIR_MM].opp,
				kicker_ctrl_table[KIR_EMIBW].opp,
				kicker_ctrl_table[KIR_SDIO].opp,
				kicker_ctrl_table[KIR_WIFI].opp,
				kicker_ctrl_table[KIR_SYSFS].opp);

	/* find the min opp from kicker table */
	for (i = 0; i < NUM_KICKER; i++) {
		if (kicker_ctrl_table[i].opp < 0)
			continue;

		if ((unsigned)kicker_ctrl_table[i].opp < min)
			min = kicker_ctrl_table[i].opp;
	}

	/* if have no request, set to init OPP */
	if (min == UINT_MAX)
		min = pwrctrl->late_init_opp;

	return min;
}

static unsigned int find_opp_index_by_request(enum dvfs_kicker kicker, int new_opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_profile *kicker_ctrl_table = kicker_table;

	/* compare kicker table opp with new opp (except SYSFS) */
	if (new_opp == kicker_ctrl_table[kicker].opp && kicker != KIR_SYSFS) {
		if (vcorefs_curr_opp == vcorefs_prev_opp) {
			pwrctrl->error_code = 0;
			return NUM_OPP;
		}
	}

	if (pwrctrl->kr_req_mask & (1U << kicker)) {
		if (new_opp < 0)
			kicker_ctrl_table[kicker].opp = new_opp;
		pwrctrl->error_code = -ERR_NO_CHANGE;
		return NUM_OPP;
	}

	kicker_ctrl_table[kicker].opp = new_opp;

	return find_min_opp(kicker);
}

#if 0	/* NO USE */
static int is_need_recover_dvfs(enum dvfs_kicker kicker, int curr_opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	unsigned int recover_opp;
	int r = FAIL;

	if (pwrctrl->recover_en && kicker < NUM_KICKER && feature_en) {

		mutex_lock(&fuct_check_mutex);
		recover_opp = find_min_opp(kicker);
		mutex_unlock(&fuct_check_mutex);

		if (recover_opp > curr_opp) {
			if (get_ddr_khz() == FDDR_S1_KHZ) {

				vcorefs_crit_mask("Restore to LPM, do DVS\n");

				r = set_vcore_with_opp(kicker, recover_opp);
				if (r)
					return ERR_VCORE_DVS;

				vcorefs_curr_opp = recover_opp;

				return r;
			}
		}

		if (recover_opp < curr_opp) {
			if (get_vcore_uv() == VCORE_1_P_15_UV) {

				vcorefs_crit_mask("Restore to HPM, do DFS\n");

				r = set_freq_with_opp(kicker, recover_opp);
				if (r)
					return ERR_VENCPLL_FH;

				r = set_fddr_with_opp(kicker, recover_opp);
				if (r) {
					set_freq_with_opp(kicker, curr_opp);
					return ERR_DDR_DFS;
				}

				vcorefs_curr_opp = recover_opp;

				return r;
			}
		}
	}

	return r;
}
#endif

/**************************************
 *
 **************************************/
static int do_dvfs_for_performance(enum dvfs_kicker kicker, unsigned int opp)
{
	int r;

	/* for performance: scale UP voltage -> frequency */

	r = set_vcore_with_opp(kicker, opp);
	if (r)
		return -ERR_VCORE_DVS;

	r = set_freq_with_opp(kicker, opp);
	if (r)
		return ERR_VENCPLL_FH;

	r = set_fddr_with_opp(kicker, opp);
	if (r)
		return ERR_DDR_DFS;

	return 0;
}

static int do_dvfs_for_low_power(enum dvfs_kicker kicker, unsigned int opp)
{
	int r;

	/* for low power: scale DOWN frequency -> voltage */

	r = set_fddr_with_opp(kicker, opp);
	if (r)
		return -ERR_DDR_DFS;

	r = set_freq_with_opp(kicker, opp);
	if (r)
		return ERR_VENCPLL_FH;

	r = set_vcore_with_opp(kicker, opp);
	if (r)
		return ERR_VCORE_DVS;

	return 0;
}

/**************************************
 *
 **************************************/
static int kick_dvfs_by_opp_index(enum dvfs_kicker kicker, unsigned int opp)
{
	int r;
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	vcorefs_crit_mask("kicker: %u, opp: %u, curr_opp: %u(%u)\n",
			  kicker, opp, vcorefs_curr_opp, vcorefs_prev_opp);

	if (vcorefs_curr_opp == opp && vcorefs_curr_opp == vcorefs_prev_opp)
		return 0;

	if (!feature_en)
		return -ERR_FEATURE_DISABLE;

	/* try again since previous change is partial success */
	if (vcorefs_curr_opp == opp && vcorefs_curr_opp != vcorefs_prev_opp)
		vcorefs_curr_opp = vcorefs_prev_opp;

	/* 0 (performance) <= index <= X (low power) */
	if (opp < vcorefs_curr_opp)
		r = do_dvfs_for_performance(kicker, opp);
	else
		r = do_dvfs_for_low_power(kicker, opp);

	if (r == 0) {
		vcorefs_prev_opp = opp;
		vcorefs_curr_opp = opp;
	} else if (r > 0) {
		vcorefs_prev_opp = vcorefs_curr_opp;
		vcorefs_curr_opp = opp;
	}

	return r;
}

static int vcorefs_func_enable_check(enum dvfs_kicker kicker, enum dvfs_opp new_opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_profile *kicker_ctrl_table = kicker_table;

	vcorefs_crit_mask("feature_en: %u(%d), kicker: %u, new_opp: %d(%d)\n",
			  feature_en, pwrctrl->error_code, kicker, new_opp, kicker_ctrl_table[kicker].opp);

	if (kicker < KIR_GPU || kicker >= NUM_KICKER || kicker == KIR_SDIO)
		return -ERR_KICKER;
	if (new_opp < OPP_OFF || new_opp >= NUM_OPP)
		return -ERR_OPP;

	/* UHPM is only for GPU */
	if (new_opp == OPPI_PERF_ULTRA && kicker != KIR_GPU && kicker != KIR_SYSFS)
		return -ERR_OPP;

	if (!pwrctrl->late_init_opp_done)
		return -ERR_LATE_INIT_OPP;

	return 0;
}

/******************************************
 * kicker Idx   Condition
 * ----------------------------------------
 * [0][GPU]     GPU loading
 * [1][MM]      Video format (by loading)
 * [1][MM]      ISP state
 * [2][EMI]     EMI bandwidth
 * [5][SYSFS]   Command
 ******************************************/
int vcorefs_request_dvfs_opp(enum dvfs_kicker kicker, enum dvfs_opp new_opp)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	unsigned int request_opp;
	int r;

	mutex_lock(&vcorefs_mutex);
	r = vcorefs_func_enable_check(kicker, new_opp);
	if (r) {
		vcorefs_crit_mask("*** VCORE DVFS STOP (%d) ***\n", r);
		mutex_unlock(&vcorefs_mutex);
		return r;
	}

	request_opp = find_opp_index_by_request(kicker, new_opp);
	if (request_opp < NUM_OPP)
		pwrctrl->error_code = kick_dvfs_by_opp_index(kicker, request_opp);
	else
		vcorefs_crit_mask("*** VCORE DVFS OPP NO CHANGE (0x%x) ***\n", pwrctrl->kr_req_mask);

	r = pwrctrl->error_code;
	mutex_unlock(&vcorefs_mutex);

	return r;
}

/**************************************
 * SDIO AutoK related API
 **************************************/
#if 0	/* NO USE */
int vcorefs_sdio_lock_dvfs(bool is_online_tuning)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	if (is_online_tuning) {	/* avoid OT thread sleeping in vcorefs_mutex */
		tag_pr_info("sdio: lock in online-tuning\n");
		return PASS;
	}

	mutex_lock(&vcorefs_mutex);
	pwrctrl->sdio_lock = 1;
	tag_pr_info("sdio: set lock: %d, vcorefs_curr_opp: %d\n", pwrctrl->sdio_lock, vcorefs_curr_opp);
	mutex_unlock(&vcorefs_mutex);

	return PASS;
}

unsigned int vcorefs_sdio_get_vcore_nml(void)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	unsigned int voltage, ret;

	mutex_lock(&vcorefs_mutex);
	voltage = vcorefs_get_curr_voltage();
	tag_pr_info("sdio: get vcore dvfs voltage: %u\n", voltage);
	if (voltage == opp_ctrl_table[OPP_0].vcore_uv)
		ret = VCORE_1_P_15_UV;
	else
		ret = VCORE_1_P_05_UV;
	mutex_unlock(&vcorefs_mutex);

	return ret;
}

int vcorefs_sdio_set_vcore_nml(unsigned int autok_vol_uv)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	tag_pr_info("sdio: autok_vol_uv: %d, vcorefs_curr_opp: %d, sdio_lock: %d\n",
				autok_vol_uv, vcorefs_curr_opp, pwrctrl->sdio_lock);

	if (!pwrctrl->sdio_lock)
		return FAIL;

	mutex_lock(&vcorefs_mutex);
	if (autok_vol_uv == VCORE_1_P_15_UV)
		kick_dvfs_by_opp_index(KIR_SDIO_AUTOK, OPP_0);
	else
		kick_dvfs_by_opp_index(KIR_SDIO_AUTOK, OPP_1);
	mutex_unlock(&vcorefs_mutex);

	return PASS;
}

int vcorefs_sdio_unlock_dvfs(bool is_online_tuning)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	if (is_online_tuning) {	/* avoid OT thread sleeping in vcorefs_mutex */
		tag_pr_info("sdio: unlock in online-tuning\n");
		return PASS;
	}

	mutex_lock(&vcorefs_mutex);
	kick_dvfs_by_opp_index(KIR_SDIO_AUTOK, vcorefs_curr_opp);

	pwrctrl->sdio_lock = 0;
	tag_pr_info("sdio: set unlock: %d, vcorefs_curr_opp: %d\n",
					pwrctrl->sdio_lock, vcorefs_curr_opp);
	mutex_unlock(&vcorefs_mutex);

	return PASS;
}

bool vcorefs_sdio_need_multi_autok(void)
{
	/* true : do LPM+HPM AutoK */
	/* false: do HPM AutoK     */

	/* SDIO only work on HPM */
	return false;
}
#endif

/**************************************
 * init boot-up OPP from late init
 **************************************/
static void set_init_opp_index(struct vcorefs_profile *pwrctrl)
{
	if (feature_en) {
		if (pwrctrl->init_opp_perf || !dram_can_support_fh())
			pwrctrl->late_init_opp = OPPI_PERF;
		else
			pwrctrl->late_init_opp = OPPI_LOW_PWR;
	} else {
		pwrctrl->late_init_opp = vcorefs_curr_opp;
	}
}

static int late_init_to_lowpwr_opp(void)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;

	mutex_lock(&vcorefs_mutex);
	set_init_opp_index(pwrctrl);
	kick_dvfs_by_opp_index(KIR_LATE_INIT, pwrctrl->late_init_opp);

	pwrctrl->late_init_opp_done = 1;

	tag_pr_info("[%s] feature_en: %u, late_init_opp: %u\n", __func__, feature_en, pwrctrl->late_init_opp);
	mutex_unlock(&vcorefs_mutex);

	return 0;
}

/**************************************
 *
 **************************************/
static ssize_t opp_table_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	char *p = buf;
	int i;

	for (i = 0; i < NUM_OPP; i++) {
		p += sprintf(p, "[OPP_%d] vcore_uv:    %u (0x%x)\n", i, opp_ctrl_table[i].vcore_uv,
								     vcore_uv_to_pmic(opp_ctrl_table[i].vcore_uv));
		p += sprintf(p, "[OPP_%d] ddr_khz:     %u\n", i, opp_ctrl_table[i].ddr_khz);
#ifndef VCOREFS_FVENC_NOCTRL
		p += sprintf(p, "[OPP_%d] venc_khz:    %u\n", i, opp_ctrl_table[i].venc_khz);
#endif
		p += sprintf(p, "[OPP_%d] axi_khz:     %u\n", i, opp_ctrl_table[i].axi_khz);
		p += sprintf(p, "[OPP_%d] qtrhalf_khz: %u\n", i, opp_ctrl_table[i].qtrhalf_khz);
		p += sprintf(p, "\n");
	}

	for (i = 0; i < NUM_TRANS; i++)
		p += sprintf(p, "[TRANS%d] vcore_uv: %u (0x%x)\n", i + 1, trans[i], vcore_uv_to_pmic(trans[i]));

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t opp_table_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	unsigned int val, uv;
	char cmd[32];

	if (sscanf(buf, "%31s %u", cmd, &val) != 2)
		return -EPERM;

	tag_pr_info("opp_table: cmd = %s, val = %u (0x%x)\n", cmd, val, val);

	if (!cmd_cmp(cmd, "UHPM") && val < VCORE_INVALID) {
		uv = vcore_pmic_to_uv(val);
		mutex_lock(&vcorefs_mutex);
		if (uv >= opp_ctrl_table[OPPI_PERF].vcore_uv && !feature_en) {
			opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv = uv;
			update_vcore_pwrap_cmd(opp_ctrl_table);
		}
		mutex_unlock(&vcorefs_mutex);
	} else if (!cmd_cmp(cmd, "HPM") && val < VCORE_INVALID) {
		uv = vcore_pmic_to_uv(val);
		mutex_lock(&vcorefs_mutex);
		if (uv >= opp_ctrl_table[OPPI_LOW_PWR].vcore_uv &&
		    uv <= opp_ctrl_table[OPPI_PERF_ULTRA].vcore_uv && !feature_en) {
			opp_ctrl_table[OPPI_PERF].vcore_uv = uv;
			update_vcore_pwrap_cmd(opp_ctrl_table);
		}
		mutex_unlock(&vcorefs_mutex);
	} else if (!cmd_cmp(cmd, "LPM") && val < VCORE_INVALID) {
		uv = vcore_pmic_to_uv(val);
		mutex_lock(&vcorefs_mutex);
		if (uv <= opp_ctrl_table[OPPI_PERF].vcore_uv && !feature_en) {
			opp_ctrl_table[OPPI_LOW_PWR].vcore_uv = uv;
			update_vcore_pwrap_cmd(opp_ctrl_table);
		}
		mutex_unlock(&vcorefs_mutex);
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t error_table_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	char *p = buf;

	p += sprintf(p, "%u: PASS\n"               , PASS);
	p += sprintf(p, "%u: FAIL\n"               , FAIL);
	p += sprintf(p, "%u: ERR_FEATURE_DISABLE\n", ERR_FEATURE_DISABLE);
	p += sprintf(p, "%u: ERR_SDIO_AUTOK\n"     , ERR_SDIO_AUTOK);
	p += sprintf(p, "%u: ERR_OPP\n"            , ERR_OPP);
	p += sprintf(p, "%u: ERR_KICKER\n"         , ERR_KICKER);
	p += sprintf(p, "%u: ERR_NO_CHANGE\n"      , ERR_NO_CHANGE);
	p += sprintf(p, "%u: ERR_VCORE_DVS\n"      , ERR_VCORE_DVS);
	p += sprintf(p, "%u: ERR_DDR_DFS\n"        , ERR_DDR_DFS);
	p += sprintf(p, "%u: ERR_VENCPLL_FH\n"     , ERR_VENCPLL_FH);
	p += sprintf(p, "%u: ERR_LATE_INIT_OPP\n"  , ERR_LATE_INIT_OPP);

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t vcore_debug_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_profile *kicker_ctrl_table = kicker_table;
	char *p = buf;
	unsigned int uv = get_vcore_uv();

	p += sprintf(p, "[feature_en]: %u\n"  , feature_en);
	p += sprintf(p, "[recover_en]: %u\n"  , pwrctrl->recover_en);
	p += sprintf(p, "[vcore_dvs ]: %u\n"  , pwrctrl->vcore_dvs);
	p += sprintf(p, "[freq_dfs  ]: %u\n"  , pwrctrl->freq_dfs);
	p += sprintf(p, "[ddr_dfs   ]: %u\n"  , pwrctrl->ddr_dfs);
	p += sprintf(p, "[sdio_lock ]: %u\n"  , pwrctrl->sdio_lock);
	p += sprintf(p, "[ERROR_CODE]: %d\n"  , pwrctrl->error_code);
	p += sprintf(p, "[log_mask  ]: 0x%x\n", pwrctrl->log_mask);
	p += sprintf(p, "\n");

	p += sprintf(p, "[late_init_opp_done]: %u\n"  , pwrctrl->late_init_opp_done);
	p += sprintf(p, "[init_opp_perf     ]: %u\n"  , pwrctrl->init_opp_perf);
	p += sprintf(p, "[kr_req_mask       ]: 0x%x\n", pwrctrl->kr_req_mask);
	p += sprintf(p, "[late_init_opp     ]: %u\n"  , pwrctrl->late_init_opp);
	p += sprintf(p, "\n");

	p += sprintf(p, "vcorefs_curr_opp: %u\n", vcorefs_curr_opp);
	p += sprintf(p, "vcorefs_prev_opp: %u\n", vcorefs_prev_opp);
	p += sprintf(p, "\n");

	p += sprintf(p, "[voltage] uv : %u (0x%x)\n", uv, vcore_uv_to_pmic(uv));
	p += sprintf(p, "[ddr    ] khz: %u\n", get_ddr_khz());
#ifndef VCOREFS_FVENC_NOCTRL
	p += sprintf(p, "[venc   ] khz: %u\n", pwrctrl->curr_venc_khz);
#endif
	p += sprintf(p, "[axi    ] khz: %u\n", pwrctrl->curr_axi_khz);
	p += sprintf(p, "[qtrhalf] khz: %u\n", pwrctrl->curr_qtrhalf_khz);
	p += sprintf(p, "\n");

	p += sprintf(p, "[KIR_GPU  ] opp: %d\n", kicker_ctrl_table[KIR_GPU].opp);
	p += sprintf(p, "[KIR_MM   ] opp: %d\n", kicker_ctrl_table[KIR_MM].opp);
	p += sprintf(p, "[KIR_EMIBW] opp: %d\n", kicker_ctrl_table[KIR_EMIBW].opp);
	p += sprintf(p, "[KIR_SDIO ] opp: %d\n", kicker_ctrl_table[KIR_SDIO].opp);
	p += sprintf(p, "[KIR_WIFI ] opp: %d\n", kicker_ctrl_table[KIR_WIFI].opp);
	p += sprintf(p, "[KIR_SYSFS] opp: %d\n", kicker_ctrl_table[KIR_SYSFS].opp);
	p += sprintf(p, "\n");

	p = spm_dump_vcore_dvs_regs(p);

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t vcore_debug_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct kicker_profile *kicker_ctrl_table = kicker_table;
	int val, r;
	char cmd[32];
	unsigned int opp;

	if (sscanf(buf, "%31s %d", cmd, &val) != 2)
		return -EPERM;

	tag_pr_info("vcore_debug: cmd = %s, val = %d (0x%x)\n", cmd, val, val);

	if (!cmd_cmp(cmd, "feature_en")) {
		mutex_lock(&vcorefs_mutex);
		if (val && dram_can_support_fh() && !feature_en) {
			feature_en = 1;
			set_init_opp_index(pwrctrl);
		} else if (!val && kicker_ctrl_table[KIR_GPU].opp != OPPI_PERF_ULTRA && feature_en) {
			r = kick_dvfs_by_opp_index(KIR_SYSFS, OPPI_PERF);
			BUG_ON(r);
			feature_en = 0;
			set_init_opp_index(pwrctrl);
		}
		mutex_unlock(&vcorefs_mutex);
	} else if (!cmd_cmp(cmd, "vcore_dvs")) {
		pwrctrl->vcore_dvs = !!val;
	} else if (!cmd_cmp(cmd, "freq_dfs")) {
		pwrctrl->freq_dfs = !!val;
	} else if (!cmd_cmp(cmd, "ddr_dfs")) {
		pwrctrl->ddr_dfs = !!val;
	} else if (!cmd_cmp(cmd, "log_mask")) {
		pwrctrl->log_mask = val;
	} else if (!cmd_cmp(cmd, "init_opp_perf")) {
		pwrctrl->init_opp_perf = !!val;
	} else if (!cmd_cmp(cmd, "kr_req_mask")) {
		pwrctrl->kr_req_mask = val;
	} else if (!cmd_cmp(cmd, "KIR_GPU")) {
		vcorefs_request_dvfs_opp(KIR_GPU, val);
	} else if (!cmd_cmp(cmd, "KIR_MM")) {
		vcorefs_request_dvfs_opp(KIR_MM, val);
	} else if (!cmd_cmp(cmd, "KIR_EMIBW")) {
		vcorefs_request_dvfs_opp(KIR_EMIBW, val);
	} else if (!cmd_cmp(cmd, "KIR_SDIO")) {
		vcorefs_request_dvfs_opp(KIR_SDIO, val);
	} else if (!cmd_cmp(cmd, "KIR_WIFI")) {
		vcorefs_request_dvfs_opp(KIR_WIFI, val);
	} else if (!cmd_cmp(cmd, "KIR_SYSFS") && (val >= OPP_OFF && val < NUM_OPP)) {
		if (is_vcorefs_can_work()) {
			r = vcorefs_request_dvfs_opp(KIR_SYSFS, val);
			BUG_ON(r);
		}
	} else if (!cmd_cmp(cmd, "KIR_SYSFSX") && (val >= OPP_OFF && val < NUM_OPP)) {
		mutex_lock(&vcorefs_mutex);
		if (val != OPP_OFF) {
			pwrctrl->kr_req_mask = (1U << NUM_KICKER) - 1;
			kick_dvfs_by_opp_index(KIR_SYSFS, val);
		} else {
			opp = find_min_opp(KIR_SYSFS);
			kick_dvfs_by_opp_index(KIR_SYSFS, opp);
			pwrctrl->kr_req_mask = 0;
		}
		mutex_unlock(&vcorefs_mutex);
	} else {
		return -EINVAL;
	}

	return count;
}

DEFINE_ATTR_RW(opp_table);
DEFINE_ATTR_RO(error_table);
DEFINE_ATTR_RW(vcore_debug);

static struct attribute *vcorefs_attrs[] = {
	__ATTR_OF(opp_table),
	__ATTR_OF(error_table),
	__ATTR_OF(vcore_debug),
	NULL,
};

static struct attribute_group vcorefs_attr_group = {
	.name	= "vcorefs",
	.attrs	= vcorefs_attrs,
};

/**************************************
 * Init Function
 **************************************/
static int init_vcorefs_pwrctrl(void)
{
	int i;
	struct vcorefs_profile *pwrctrl = &vcorefs_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;

	mutex_lock(&vcorefs_mutex);
	pwrctrl->curr_vcore_uv = get_vcore_uv();
	BUG_ON(pwrctrl->curr_vcore_uv == 0);

	pwrctrl->curr_ddr_khz = get_ddr_khz();

	tag_pr_info("curr_vcore_uv: %u, curr_ddr_khz: %u\n", pwrctrl->curr_vcore_uv, pwrctrl->curr_ddr_khz);

	for (i = 0; i < NUM_OPP; i++) {
		if (i == OPPI_PERF_ULTRA) {
			opp_ctrl_table[i].vcore_uv = vcore_pmic_to_uv(get_vcore_ptp_volt(VCORE_1_P_25_UV));
			opp_ctrl_table[i].ddr_khz = get_ddr_khz_by_steps(0);
		} else if (i == OPPI_PERF) {
			opp_ctrl_table[i].vcore_uv = vcore_pmic_to_uv(get_vcore_ptp_volt(VCORE_1_P_15_UV));
			opp_ctrl_table[i].ddr_khz = get_ddr_khz_by_steps(0);
		} else if (i == OPPI_LOW_PWR) {
			opp_ctrl_table[i].vcore_uv = vcore_pmic_to_uv(get_vcore_ptp_volt(VCORE_1_P_05_UV));
			opp_ctrl_table[i].ddr_khz = get_ddr_khz_by_steps(1);
		}

		tag_pr_info("OPP %d: vcore_uv = %u, ddr_khz = %u\n",
			    i, opp_ctrl_table[i].vcore_uv, opp_ctrl_table[i].ddr_khz);
	}

	update_vcore_pwrap_cmd(opp_ctrl_table);
	mutex_unlock(&vcorefs_mutex);

	return 0;
}

static int __init vcorefs_module_init(void)
{
	int r;

	r = init_vcorefs_pwrctrl();
	if (r) {
		tag_pr_notice("*** FAILED TO INIT PWRCTRL (%d) ***\n", r);
		return r;
	}

	r = sysfs_create_group(power_kobj, &vcorefs_attr_group);
	if (r)
		tag_pr_notice("*** FAILED TO CREATE /sys/power/vcorefs (%d) ***\n", r);

	return r;
}

module_init(vcorefs_module_init);
late_initcall_sync(late_init_to_lowpwr_opp);

MODULE_DESCRIPTION("Vcore DVFS Driver v0.5.1");
