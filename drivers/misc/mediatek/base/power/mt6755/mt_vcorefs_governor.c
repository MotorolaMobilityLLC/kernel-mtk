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
#include <linux/mutex.h>

#include "mt_vcorefs_manager.h"
#include "mt_spm_vcore_dvfs.h"
#include "mt_spm_pmic_wrap.h"

#include "mt_ptp.h"
#include "mt_dramc.h"
#include "mt-plat/upmu_common.h"
#include "mt-plat/mt_pmic_wrap.h"
#include "mt_spm.h"
#include "mmdvfs_mgr.h"
#include <mach/fliper.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/irqchip/mt-eic.h>
#include <linux/suspend.h>
#include "mt-plat/aee.h"

__weak int emmc_autok(void)
{
	return 0;
}

__weak int sd_autok(void)
{
	return 0;
}

__weak int sdio_autok(void)
{
	return 0;
}

__weak void mmdvfs_enable(int enable)
{

}

/*
 * PMIC
 */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
#define PMIC_VCORE_ADDR PMIC_BUCK_VCORE_VOSEL_ON_ADDR
#else
#define PMIC_VCORE_ADDR MT6351_PMIC_BUCK_VCORE_VOSEL_ON_ADDR
#endif
#define VCORE_BASE_UV		600000
#define VCORE_STEP_UV		6250

#define VCORE_INVALID		0x80

#define vcore_uv_to_pmic(uv)	/* pmic >= uv */	\
	((((uv) - VCORE_BASE_UV) + (VCORE_STEP_UV - 1)) / VCORE_STEP_UV)

#define vcore_pmic_to_uv(pmic)	\
	(((pmic) * VCORE_STEP_UV) + VCORE_BASE_UV)

/*
 *  sram debug info
 */
void __iomem *vcorefs_sram_base;
u32 spm_vcorefs_err_irq = 154;
/* unsigned int vcorefs_log_mask = ~((0xFFFFFFFF << LAST_KICKER)); */
/* default filter-out GPU message */
unsigned int vcorefs_log_mask = ~((0xFFFFFFFF << LAST_KICKER) | (1U << KIR_GPU));
/*
 * HD segment size
 */
#define DISP_LCM_HD_SIZE (1280 * 800)

#define VCOREFS_SEGMENT_HPM_LPM	1
#define VCOREFS_SEGMENT_LPM		2
#define VCOREFS_SEGMENT_LPM_EXCEPT_OVL	3

/*
 *  product segment
 */
#define MT6750_TURBO_SEGMENT 0x41
#define MT6750_NORMAL_SEGMENT 0x42
#define MT6738_SEGMENT 0x43
#define MT6750_TURBO_5M_SEGMENT 0x45
#define MT6750_NORMAL_5M_SEGMENT 0x46
#define MT6738_5M_SEGMENT 0x4B

/*
 * struct define
 */
static DEFINE_MUTEX(governor_mutex);

struct governor_profile {
	bool vcore_dvfs_en;
	bool vcore_dvs;
	bool ddr_dfs;
	bool isr_debug;

	bool is_fhd_segment;

	int boot_up_opp;
	int late_init_opp;

	int curr_vcore_uv;
	int curr_ddr_khz;

	u32 autok_kir_group;
	u32 active_autok_kir;

	u32 cpu_dvfs_req;

	u32 log_mask;
	u32 segment_code;
	u32 segment_policy;

	u32 total_bw_lpm_threshold;
	u32 total_bw_hpm_threshold;
	u32 perform_bw_lpm_threshold;
	u32 perform_bw_hpm_threshold;
	bool total_bw_enable;
	bool perform_bw_enable;
};

static struct governor_profile governor_ctrl = {
	.vcore_dvfs_en = 1,	/* vcore dvfs feature enable */
	.vcore_dvs = 1,
	.ddr_dfs = 1,
	.isr_debug = 0,

	.is_fhd_segment = true,
	.cpu_dvfs_req = (1 << MD_CAT6_CA_DATALINK | (1 << MD_Position)),
	.boot_up_opp = OPPI_PERF,	/* boot up with HPM */
	.late_init_opp = OPPI_LOW_PWR,	/* late init change to LPM mode */

	.autok_kir_group = ((1 << KIR_AUTOK_EMMC) | (1 << KIR_AUTOK_SDIO) | (1 << KIR_AUTOK_SD)),
	.active_autok_kir = 0,

	.curr_vcore_uv = VCORE_1_P_00_UV,
	.curr_ddr_khz = FDDR_S1_KHZ,

	.total_bw_enable = false,
	.perform_bw_enable = false,
};

static struct opp_profile opp_table[] __nosavedata = {
	/* performance mode */
	[OPP_0] = {
		   .vcore_uv = VCORE_1_P_00_UV,
		   .ddr_khz = FDDR_S0_KHZ,
		   },
	/* low power mode */
	[OPP_1] = {
		   .vcore_uv = VCORE_0_P_90_UV,
		   .ddr_khz = FDDR_S1_KHZ,
		   }
};

int kicker_table[LAST_KICKER];

int gpu_kicker_init_opp = OPPI_LOW_PWR;

int vcorefs_gpu_get_init_opp(void)
{
	return gpu_kicker_init_opp;
}

void vcorefs_gpu_set_init_opp(int opp)
{
	vcorefs_info("gpu_set_init_opp(%d)\n", opp);
	gpu_kicker_init_opp = opp;
}

bool vcorefs_request_init_opp(int kicker, int opp)
{
	bool accept = false;

	mutex_lock(&governor_mutex);
	if (kicker == KIR_GPU) {
		gpu_kicker_init_opp = opp;
		vcorefs_info("init_opp request(kr:%d, opp:%d)\n", kicker, opp);
		accept = true;
	}
	mutex_unlock(&governor_mutex);
	return accept;
}

static char *kicker_name[] = {
	"KIR_MM_16MCAM",
	"KIR_MM_WFD",
	"KIR_MM_MHL",
	"KIR_OVL",
	"KIR_SDIO",
	"KIR_PERF",
	"KIR_SYSFS",
	"KIR_SYSFS_N",
	"KIR_GPU",
	"NUM_KICKER",
	"KIR_LATE_INIT",
	"KIR_SYSFSX",
	"KIR_AUTOK_EMMC",
	"KIR_AUTOK_SDIO",
	"KIR_AUTOK_SD",
	"LAST_KICKER",
};

static unsigned int trans[NUM_TRANS] __nosavedata;

struct timing_debug_profile {
	u32 dvfs_latency_spec;
	u32 long_hpm_latency_count;
	u32 long_lpm_latency_count;
	u32 long_emi_block_time_count;
	u32 max_hpm_latency;
	u32 max_lpm_latency;
	u32 max_emi_block_time;
};

static struct timing_debug_profile timing_debug_ctrl = {
	.dvfs_latency_spec = 32,	/* 1T@32K, about 210us for hpm,lpm latency, max allow 1ms */
	.long_hpm_latency_count = 0,
	.long_lpm_latency_count = 0,
	.long_emi_block_time_count = 0,	/* 1T@26M */
	.max_hpm_latency = 0,
	.max_lpm_latency = 0,
	.max_emi_block_time = 0,
};

void vcorefs_set_log_mask(u32 value)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	gvrctrl->log_mask = value;
	vcorefs_log_mask = value;
}
/*
 * Unit Test API
 */
void vcorefs_set_cpu_dvfs_req(u32 value)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	u32 mask = 0xFFFF;

	mutex_lock(&governor_mutex);
	gvrctrl->cpu_dvfs_req = (value & mask);
	spm_vcorefs_set_cpu_dvfs_req(value, mask);
	mutex_unlock(&governor_mutex);
}

unsigned int vcorefs_get_cpu_dvfs_req(void)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	return gvrctrl->cpu_dvfs_req;
}

static void update_vcore_pwrap_cmd(struct opp_profile *opp_ctrl_table)
{
	u32 diff;

	diff = opp_ctrl_table[OPPI_PERF].vcore_uv - opp_ctrl_table[OPPI_LOW_PWR].vcore_uv;
	trans[TRANS1] = opp_ctrl_table[OPPI_LOW_PWR].vcore_uv + (diff / 3) * 1;
	trans[TRANS2] = opp_ctrl_table[OPPI_LOW_PWR].vcore_uv + (diff / 3) * 2;

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_HPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS2,
				 vcore_uv_to_pmic(trans[TRANS2]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_TRANS1,
				 vcore_uv_to_pmic(trans[TRANS1]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_LPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_HPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_TRANS2,
				 vcore_uv_to_pmic(trans[TRANS2]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_TRANS1,
				 vcore_uv_to_pmic(trans[TRANS1]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND, IDX_SP_VCORE_LPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_HPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_TRANS2,
				 vcore_uv_to_pmic(trans[TRANS2]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_TRANS1,
				 vcore_uv_to_pmic(trans[TRANS1]));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_LPM,
				 vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));

	vcorefs_info("HPM   : %u (0x%x)\n", opp_ctrl_table[OPPI_PERF].vcore_uv,
		     vcore_uv_to_pmic(opp_ctrl_table[OPPI_PERF].vcore_uv));
	vcorefs_info("TRANS2: %u (0x%x)\n", trans[TRANS2], vcore_uv_to_pmic(trans[TRANS2]));
	vcorefs_info("TRANS1: %u (0x%x)\n", trans[TRANS1], vcore_uv_to_pmic(trans[TRANS1]));
	vcorefs_info("LPM   : %u (0x%x)\n", opp_ctrl_table[OPPI_LOW_PWR].vcore_uv,
		     vcore_uv_to_pmic(opp_ctrl_table[OPPI_LOW_PWR].vcore_uv));
}

static int is_fhd_segment(void)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	return gvrctrl->is_fhd_segment;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void vcorefs_early_suspend_cb(struct early_suspend *h)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	if (gvrctrl->cpu_dvfs_req & MD_DISABLE_SCREEN_CHANGE)
		return;

	if (is_fhd_segment())
		spm_vcorefs_set_cpu_dvfs_req(0, gvrctrl->cpu_dvfs_req);	/* set screen OFF state */
}

static void vcorefs_late_resume_cb(struct early_suspend *h)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	if (gvrctrl->cpu_dvfs_req & MD_DISABLE_SCREEN_CHANGE)
		return;

	if (is_fhd_segment())
		spm_vcorefs_set_cpu_dvfs_req(0xFFFF, gvrctrl->cpu_dvfs_req);	/* set screen ON state */
}

static struct early_suspend vcorefs_earlysuspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 50,
	.suspend = vcorefs_early_suspend_cb,	/* after mtkfb_early_suspend */
	.resume = vcorefs_late_resume_cb,	/* before mtkfb_late_resume */
};
#else
static struct notifier_block vcorefs_fb_notif;
static int
vcorefs_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *fb_evdata)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct fb_event *evdata = fb_evdata;
	int blank;

	if (gvrctrl->cpu_dvfs_req & MD_DISABLE_SCREEN_CHANGE)
		return 0;

	/* skip if not fhd segment */
	if (is_fhd_segment() == false)
		return 0;

	/* skip non-interested event immediately */
	if (event != FB_EVENT_BLANK && event != FB_EARLY_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	vcorefs_debug("fb_notify event=%lu blank=%d\n", event, blank);
	if ((blank == FB_BLANK_POWERDOWN) && (event == FB_EVENT_BLANK)) {
		vcorefs_info("Switch to Screen-OFF\n");
		spm_vcorefs_set_cpu_dvfs_req(0, gvrctrl->cpu_dvfs_req);	/* set screen OFF state */
	} else if ((blank == FB_BLANK_UNBLANK) && (event == FB_EARLY_EVENT_BLANK)) {
		vcorefs_info("Switch to Screen-ON\n");
		spm_vcorefs_set_cpu_dvfs_req(0xFFFF, gvrctrl->cpu_dvfs_req);	/* set screen ON state */
	}
	return 0;
}

#endif
/*
 *  sram debug info
 */
static u32 sram_debug_info[7];
char *vcorefs_get_sram_debug_info(char *p)
{
	if (p) {
		p += sprintf(p, "dvs/dfs up_count  : 0x%x / 0x%x\n",
			     spm_read(VCOREFS_SRAM_DVS_UP_COUNT),
			     spm_read(VCOREFS_SRAM_DFS_UP_COUNT));
		p += sprintf(p, "dvs/dfs down_count: 0x%x / 0x%x\n",
			     spm_read(VCOREFS_SRAM_DVS_DOWN_COUNT),
			     spm_read(VCOREFS_SRAM_DFS_DOWN_COUNT));
		p += sprintf(p, "dvfs_up_latency   : 0x%x\n",
			     spm_read(VCOREFS_SRAM_DVFS_UP_LATENCY));
		p += sprintf(p, "dvfs_down_latency  : 0x%x\n",
			     spm_read(VCOREFS_SRAM_DVFS_DOWN_LATENCY));
		p += sprintf(p, "dvfs_latency_spec : 0x%x\n",
			     spm_read(VCOREFS_SRAM_DVFS_LATENCY_SPEC));
		p += sprintf(p, "\n");
	} else {
		sram_debug_info[0] = spm_read(VCOREFS_SRAM_DVS_UP_COUNT);
		sram_debug_info[1] = spm_read(VCOREFS_SRAM_DFS_UP_COUNT);
		sram_debug_info[2] = spm_read(VCOREFS_SRAM_DVS_DOWN_COUNT);
		sram_debug_info[3] = spm_read(VCOREFS_SRAM_DFS_DOWN_COUNT);
		sram_debug_info[4] = spm_read(VCOREFS_SRAM_DVFS_UP_LATENCY);
		sram_debug_info[5] = spm_read(VCOREFS_SRAM_DVFS_DOWN_LATENCY);
		sram_debug_info[6] = spm_read(VCOREFS_SRAM_DVFS_LATENCY_SPEC);
		vcorefs_info("dvs_up_count     : 0x%x\n", sram_debug_info[0]);
		vcorefs_info("dfs_up_count     : 0x%x\n", sram_debug_info[1]);
		vcorefs_info("dvs_down_count   : 0x%x\n", sram_debug_info[2]);
		vcorefs_info("dfs_down_count   : 0x%x\n", sram_debug_info[3]);
		vcorefs_info("dvfs_up_latency  : 0x%x\n", sram_debug_info[4]);
		vcorefs_info("dvfs_down_latency: 0x%x\n", sram_debug_info[5]);
		vcorefs_info("dvfs_latency spec: 0x%x\n", sram_debug_info[6]);
	}

	return p;
}

static void vcorefs_init_sram_debug(void)
{
	int i;
	struct timing_debug_profile *dbg_ctrl = &timing_debug_ctrl;

	if (!vcorefs_sram_base) {
		vcorefs_err("vcorefs_sram_base is not valid\n");
		return;
	}

	for (i = 0; i < 21; i++)
		vcorefs_set_sram_data(i, 0);

#if 1
	vcorefs_get_sram_debug_info(NULL);
#endif
	vcorefs_info("clean debug sram info\n");
	spm_write(VCOREFS_SRAM_DVS_UP_COUNT, 0);
	spm_write(VCOREFS_SRAM_DFS_UP_COUNT, 0);
	spm_write(VCOREFS_SRAM_DVS_DOWN_COUNT, 0);
	spm_write(VCOREFS_SRAM_DFS_DOWN_COUNT, 0);
	spm_write(VCOREFS_SRAM_DVFS_UP_LATENCY, 0);
	spm_write(VCOREFS_SRAM_DVFS_DOWN_LATENCY, 0);
	spm_write(VCOREFS_SRAM_DVFS_LATENCY_SPEC, dbg_ctrl->dvfs_latency_spec);
	vcorefs_info("dvfs_latency spec set to 0x%x\n", spm_read(VCOREFS_SRAM_DVFS_LATENCY_SPEC));
}

void vcorefs_set_sram_data(int index, u32 data)
{
	spm_write(VCOREFS_SRAM_BASE + index * 4, data);
}

u32 vcorefs_get_sram_data(int index)
{
	return spm_read(VCOREFS_SRAM_BASE + index * 4);
}

void vcorefs_set_dvfs_latency_spec(u32 value)
{
	struct timing_debug_profile *dbg_ctrl = &timing_debug_ctrl;

	dbg_ctrl->dvfs_latency_spec = value;
	spm_write(VCOREFS_SRAM_DVFS_LATENCY_SPEC, value);
}

static irqreturn_t spm_vcorefs_err_handler(int irq, void *dev_id)
{
	u32 hpm_latency, lpm_latency;
	int pll_mode;
	struct timing_debug_profile *dbg_ctrl = &timing_debug_ctrl;

	hpm_latency = spm_read(VCOREFS_SRAM_DVFS_UP_LATENCY);
	lpm_latency = spm_read(VCOREFS_SRAM_DVFS_DOWN_LATENCY);
	pll_mode = spm_vcorefs_get_clk_mem_pll();

#if 0
	u32 assert_pc, block_time;
	/* debug only with special FW */
	assert_pc = spm_read(PCM_REG_DATA_INI);
	block_time = spm_read(SPM_SW_RSV_0);
	vcorefs_err("r6: 0x%x r15:0x%x\n", spm_read(PCM_REG6_DATA), spm_read(PCM_REG15_DATA));
	if ((assert_pc != 0) && (block_time != 0)) {
		dbg_ctrl->long_emi_block_time_count++;
		if (block_time > dbg_ctrl->max_emi_block_time)
			dbg_ctrl->max_emi_block_time = block_time;
		vcorefs_err("see long emi block time: %d (cnt=%d)\n", block_time,
			    dbg_ctrl->long_emi_block_time_count);
	}
#endif

	if (pll_mode == 0) {
		if (((lpm_latency & 0xffff0000) == 0)
		    && (lpm_latency > dbg_ctrl->dvfs_latency_spec)) {
			dbg_ctrl->long_lpm_latency_count++;
			if (lpm_latency > dbg_ctrl->max_lpm_latency)
				dbg_ctrl->max_lpm_latency = lpm_latency;
			vcorefs_err("see long lpm latency: %d (cnt=%d)\n", lpm_latency,
				    dbg_ctrl->long_lpm_latency_count);
		} else if ((hpm_latency & 0xffff0000) == 0) {
			dbg_ctrl->long_hpm_latency_count++;
			if (hpm_latency > dbg_ctrl->max_hpm_latency)
				dbg_ctrl->max_hpm_latency = hpm_latency;
			vcorefs_err("see long* hpm latency: %d (cnt=%d)\n", hpm_latency,
				    dbg_ctrl->long_hpm_latency_count);
		}
	} else if (pll_mode == 1) {
		if (((hpm_latency & 0xffff0000) == 0)
		    && (hpm_latency > dbg_ctrl->dvfs_latency_spec)) {
			dbg_ctrl->long_hpm_latency_count++;
			if (hpm_latency > dbg_ctrl->max_hpm_latency)
				dbg_ctrl->max_hpm_latency = hpm_latency;
			vcorefs_err("see long hpm latency: %d (cnt=%d)\n", hpm_latency,
				    dbg_ctrl->long_hpm_latency_count);
		} else if ((lpm_latency & 0xffff0000) == 0) {
			dbg_ctrl->long_lpm_latency_count++;
			if (lpm_latency > dbg_ctrl->max_lpm_latency)
				dbg_ctrl->max_lpm_latency = lpm_latency;
			vcorefs_err("see long* lpm latency: %d (cnt=%d)\n", lpm_latency,
				    dbg_ctrl->long_lpm_latency_count);
		}
	}
	mt_eint_virq_soft_clr(irq);

	return IRQ_HANDLED;
}

uint32_t get_vcore_dvfs_sram_debug_regs(uint32_t index)
{
	uint32_t value = 0;

	switch (index) {
	case 0:
		value = 7;
		vcorefs_err("get vcore dvfs sram debug regs count = 0x%.8x\n", value);
		break;
	case 1:
		value = sram_debug_info[0];
		vcorefs_err("DVS UP COUNT(0x%x) = 0x%x\n", index, value);
		break;
	case 2:
		value = sram_debug_info[1];
		vcorefs_err("VFS UP COUNT(0x%x) = 0x%x\n", index, value);
		break;
	case 3:
		value = sram_debug_info[2];
		vcorefs_err("DVS DOWN COUNT(0x%x) = 0x%x\n", index, value);
		break;
	case 4:
		value = sram_debug_info[3];
		vcorefs_err("DFS DOWN COUNT(0x%x) = 0x%x\n", index, value);
		break;
	case 5:
		value = sram_debug_info[4];
		vcorefs_err("DVFS UP LATENCY(0x%x) = 0x%x\n", index, value);
		break;
	case 6:
		value = sram_debug_info[5];
		vcorefs_err("DVFS DOWN LATENCY(0x%x) = 0x%x\n", index, value);
		break;
	case 7:
		value = sram_debug_info[6];
		vcorefs_err("DVFS DOWN LATENCY(0x%x) = 0x%x\n", index, value);
		break;
	}

	return value;
}
EXPORT_SYMBOL(get_vcore_dvfs_sram_debug_regs);

/*
 * Governor extern API
 */
bool is_vcorefs_feature_enable(void)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	if (!dram_can_support_fh()) {
		gvrctrl->vcore_dvfs_en = false;
		vcorefs_err("DISABLE DVFS DUE TO NOT SUPPORT DRAM FH\n");
	}

	return gvrctrl->vcore_dvfs_en;
}

int vcorefs_get_num_opp(void)
{
	return NUM_OPP;		/* for ATE script */
}

int vcorefs_get_curr_vcore(void)
{
	int vcore = VCORE_INVALID;

	pwrap_read(PMIC_VCORE_ADDR, &vcore);
	if (vcore >= VCORE_INVALID)
		pwrap_read(PMIC_VCORE_ADDR, &vcore);

	return vcore < VCORE_INVALID ? vcore_pmic_to_uv(vcore) : 0;
}

int vcorefs_get_curr_ddr(void)
{
	int ddr_khz;

	ddr_khz = get_dram_data_rate() * 1000;

	return ddr_khz;
}

int vcorefs_get_vcore_by_steps(u32 steps)
{
	int uv = 0;

	switch (steps) {
	case OPP_0:
		uv = vcore_pmic_to_uv(get_vcore_ptp_volt(VCORE_1_P_00_UV));
		break;
	case OPP_1:
		uv = vcore_pmic_to_uv(get_vcore_ptp_volt(VCORE_0_P_90_UV));
		break;
	default:
		break;
	}

	return uv;
}

int vcorefs_get_ddr_by_steps(u32 steps)
{
	int ddr_khz;
	/*FIXME: wait dram_steps_freq */
	ddr_khz = dram_steps_freq(steps) * 1000;

	BUG_ON(ddr_khz < 0);

	return ddr_khz;
}

char *vcorefs_get_dvfs_info(char *p)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct timing_debug_profile *dbg_ctrl = &timing_debug_ctrl;

	int uv = vcorefs_get_curr_vcore();

	p += sprintf(p, "[vcore_dvfs  ]: %d\n", gvrctrl->vcore_dvfs_en);
	p += sprintf(p, "[vcore_dvs   ]: %d\n", gvrctrl->vcore_dvs);
	p += sprintf(p, "[ddr_dfs     ]: %d\n", gvrctrl->ddr_dfs);
	p += sprintf(p, "[isr_debug   ]: %d\n", gvrctrl->isr_debug);
	p += sprintf(p, "[fhd_segment ]: %d\n", gvrctrl->is_fhd_segment);
	p += sprintf(p, "[cpu_dvfs_req]: 0x%x\n", vcorefs_get_cpu_dvfs_req());
	p += sprintf(p, "[log_mask    ]: 0x%x\n", gvrctrl->log_mask);
	p += sprintf(p, "[segment_code]: 0x%x\n", gvrctrl->segment_code);
	p += sprintf(p, "[segment_policy]: 0x%x\n", gvrctrl->segment_policy);
	p += sprintf(p, "\n");

	p += sprintf(p, "[vcore] uv : %u (0x%x)\n", uv, vcore_uv_to_pmic(uv));
	p += sprintf(p, "[ddr  ] khz: %u\n", vcorefs_get_curr_ddr());
	p += sprintf(p, "\n");

	p += sprintf(p, "[perform_bw  ]: en = %d, lpm_thres = 0x%x, hpm_thres =0x%x\n",
		     gvrctrl->perform_bw_enable, gvrctrl->perform_bw_lpm_threshold,
		     gvrctrl->perform_bw_hpm_threshold);
	p += sprintf(p, "[total_bw    ]: en = %d, lpm_thres = 0x%x, hpm_thres =0x%x\n",
		     gvrctrl->total_bw_enable, gvrctrl->total_bw_lpm_threshold,
		     gvrctrl->total_bw_hpm_threshold);
	p += sprintf(p, "\n");
	p = vcorefs_get_sram_debug_info(p);
	p += sprintf(p, "[dvfs_latency_spec]: count=%d\n", dbg_ctrl->dvfs_latency_spec);
	p += sprintf(p, "[hpm_latency*     ]: count=0x%x ,max=%d\n",
		     dbg_ctrl->long_hpm_latency_count, dbg_ctrl->max_hpm_latency);
	p += sprintf(p, "[lpm_latency*     ]: count=0x%x ,max=%d\n",
		     dbg_ctrl->long_lpm_latency_count, dbg_ctrl->max_lpm_latency);
	/*
	   p += sprintf(p, "[emi_block_time*  ]: count=0x%x ,max=%d\n",
	   dbg_ctrl->long_emi_block_time_count, dbg_ctrl->max_emi_block_time);
	 */
	return p;
}

char *get_kicker_name(int id)
{
	return kicker_name[id];
}

char *vcorefs_get_opp_table_info(char *p)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	struct governor_profile *gvrctrl = &governor_ctrl;
	int i;

	if (gvrctrl->segment_policy == VCOREFS_SEGMENT_LPM) {
		i = OPPI_LOW_PWR;
		p += sprintf(p, "[OPP_%d] vcore_uv:    %u (0x%x)\n", i, opp_ctrl_table[i].vcore_uv,
			     vcore_uv_to_pmic(opp_ctrl_table[i].vcore_uv));
		p += sprintf(p, "[OPP_%d] ddr_khz:     %u\n", i, opp_ctrl_table[i].ddr_khz);
		p += sprintf(p, "\n");
	} else {
		for (i = 0; i < NUM_OPP; i++) {
			p += sprintf(p, "[OPP_%d] vcore_uv:    %u (0x%x)\n", i, opp_ctrl_table[i].vcore_uv,
				     vcore_uv_to_pmic(opp_ctrl_table[i].vcore_uv));
			p += sprintf(p, "[OPP_%d] ddr_khz:     %u\n", i, opp_ctrl_table[i].ddr_khz);
			p += sprintf(p, "\n");
		}

		for (i = 0; i < NUM_TRANS; i++)
			p += sprintf(p, "[TRANS%d] vcore_uv: %u (0x%x)\n", i + 1, trans[i],
				     vcore_uv_to_pmic(trans[i]));
	}
	return p;
}

/* set opp_table vcore */
void vcorefs_update_opp_table(char *cmd, int val)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	struct governor_profile *gvrctrl = &governor_ctrl;
	int uv = vcore_pmic_to_uv(val);

	if (gvrctrl->segment_policy == VCOREFS_SEGMENT_LPM) {
		vcorefs_err("segment policy is LPM only not allow opp_table cmd\n");
		return;
	}

	if (!strcmp(cmd, "HPM") && val < VCORE_INVALID) {
		if (uv >= opp_ctrl_table[OPPI_LOW_PWR].vcore_uv) {
			opp_ctrl_table[OPPI_PERF].vcore_uv = uv;
			update_vcore_pwrap_cmd(opp_ctrl_table);
		}
	} else if (!strcmp(cmd, "LPM") && val < VCORE_INVALID) {
		if (uv <= opp_ctrl_table[OPPI_PERF].vcore_uv) {
			opp_ctrl_table[OPPI_LOW_PWR].vcore_uv = uv;
			update_vcore_pwrap_cmd(opp_ctrl_table);
		}
	}
}

int vcorefs_output_kicker_id(char *name)
{
	int i;

	for (i = 0; i < NUM_KICKER; i++) {
		if (!strcmp(kicker_name[i], name))
			return i;
	}
	return -1;
}

int vcorefs_set_total_bw_threshold(u32 lpm_threshold, u32 hpm_threshold)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	if ((lpm_threshold < 1) | (lpm_threshold > 127)
	    || (hpm_threshold < 1) | (lpm_threshold > 127))
		return -1;

	gvrctrl->total_bw_lpm_threshold = lpm_threshold;
	gvrctrl->total_bw_hpm_threshold = hpm_threshold;

	mutex_lock(&governor_mutex);

	spm_vcorefs_set_total_bw_threshold(lpm_threshold, hpm_threshold);

	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	if ((lpm_threshold < 1) | (lpm_threshold > 127)
	    || (hpm_threshold < 1) | (hpm_threshold > 127))
		return -1;

	gvrctrl->perform_bw_lpm_threshold = lpm_threshold;
	gvrctrl->perform_bw_hpm_threshold = hpm_threshold;

	mutex_lock(&governor_mutex);

	spm_vcorefs_set_perform_bw_threshold(lpm_threshold, hpm_threshold);

	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_release_hpm(int opp, int vcore, int ddr)
{
	int r = -1;

	if (opp == OPPI_LOW_PWR) {
		r = spm_vcorefs_set_dvfs_hpm_force(OPPI_LOW_PWR, vcore, ddr);
		r = spm_vcorefs_set_dvfs_hpm(OPPI_LOW_PWR, vcore, ddr);
	} else {
		/* FIXME: */
		/* r = spm_vcorefs_set_dvfs_hpm_force(OPPI_LOW_PWR, 0); */
		BUG();
	}
	return r;
}

int vcorefs_handle_kir_sysfsx_req(int opp, int vcore, int ddr)
{
	int r = -1;

	vcorefs_info("hh: vcorefs_handle_kir_sysfsx_req(%d, v:%d, f:%d)\n", opp, vcore, ddr);
	if (opp == OPPI_PERF)
		r = spm_vcorefs_set_dvfs_hpm_force(OPPI_PERF, vcore, ddr);
	else if (opp == OPPI_LOW_PWR)
		r = spm_vcorefs_set_dvfs_lpm_force(OPPI_LOW_PWR, vcore, ddr);
	else if (opp == OPPI_UNREQ) {
		r = spm_vcorefs_set_dvfs_hpm_force(OPPI_LOW_PWR, vcore, ddr);
		r = spm_vcorefs_set_dvfs_lpm_force(OPPI_PERF, vcore, ddr);
	}
	return r;
}

void vcorefs_reload_spm_firmware(int flag)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);

	if ((flag & SPM_FLAG_DIS_VCORE_DVS) != 0)
		gvrctrl->vcore_dvs = 0;
	else
		gvrctrl->vcore_dvs = 1;

	if ((flag & SPM_FLAG_DIS_VCORE_DFS) != 0)
		gvrctrl->ddr_dfs = 0;
	else
		gvrctrl->ddr_dfs = 1;
	vcorefs_info(" re-kick vcore dvfs FW (vcore_dvs=%d ddr_dfs=%d)\n", gvrctrl->vcore_dvs,
		     gvrctrl->ddr_dfs);
	spm_go_to_vcore_dvfs(flag, 0);

	mutex_unlock(&governor_mutex);
}

int governor_debug_store(const char *buf)
{
	int val, val2, r = 0;
	char cmd[32];
	struct governor_profile *gvrctrl = &governor_ctrl;

	if (sscanf(buf, "%31s 0x%x 0x%x", cmd, &val, &val2) == 3 ||
	    sscanf(buf, "%31s %d %d", cmd, &val, &val2) == 3) {
		vcorefs_info("governor_debug: cmd: %s, val: %d(0x%x), val2: %d(0x%x)\n", cmd, val,
			     val, val2, val2);

		if (!strcmp(cmd, "perform_bw_threshold")) {
			/* val1: lpm_threshold,
			   val2: hpm_threshold */
			vcorefs_set_perform_bw_threshold(val, val2);
		}
		if (!strcmp(cmd, "total_bw_threshold")) {
			/* val1: lpm_threshold,
			   val2: hpm_threshold */
			vcorefs_set_total_bw_threshold(val, val2);
		} else {
			r = -EPERM;
		}
	} else if (sscanf(buf, "%31s 0x%x", cmd, &val) == 2 ||
		   sscanf(buf, "%31s %d", cmd, &val) == 2) {
		vcorefs_info("governor_debug: cmd: %s, val: %d(0x%x)\n", cmd, val, val);
		if (gvrctrl->segment_policy == VCOREFS_SEGMENT_LPM)
			vcorefs_err("segment policy is LPM only not allow vcore_debug cmd\n");
		else if (!strcmp(cmd, "vcore_dvs"))
			vcorefs_enable_dvs(val);
		else if (!strcmp(cmd, "ddr_dfs"))
			vcorefs_enable_dfs(val);
		else if (!strcmp(cmd, "isr_debug"))
			vcorefs_enable_debug_isr(val);
		else if (!strcmp(cmd, "cpu_dvfs_req"))
			vcorefs_set_cpu_dvfs_req(val);
		else if (!strcmp(cmd, "load_spm"))
			vcorefs_reload_spm_firmware(val);
		else if (!strcmp(cmd, "dvfs_latency_spec"))
			vcorefs_set_dvfs_latency_spec(val);
		else if (!strcmp(cmd, "log_mask"))
			vcorefs_set_log_mask(val);
		else
			r = -EPERM;
	} else {
		r = -EPERM;
	}

	return r;
}

/*
 * sub-main function
 */
struct dvfs_func {
	int (*do_dvfs)(int, int, int);	/* requested opp, vcore, ddr */
	u32 kicker_mask;
	char *purpose;
};

static struct dvfs_func spm_dvfs_func_list[] = {
	{spm_vcorefs_set_dvfs_hpm_force,
	 (1 << KIR_MM_16MCAM | 1 << KIR_SDIO | 1 << KIR_SYSFS | 1 << KIR_PERF | 1 << KIR_OVL | 1 << KIR_GPU),
	  "set hpm_force"},
	{spm_vcorefs_set_dvfs_hpm, (1 << KIR_MM_WFD | 1 << KIR_MM_MHL | 1 << KIR_SYSFS_N),
	 "set hpm"},
	{vcorefs_release_hpm, (1 << KIR_LATE_INIT), "clear hpm_lpm_forced"},
	{vcorefs_handle_kir_sysfsx_req,
	 (1 << KIR_SYSFSX | (1 << KIR_AUTOK_EMMC | 1 << KIR_AUTOK_SDIO | 1 << KIR_AUTOK_SD)),
	 "set hpm_lpm_force"},
};

int find_spm_dvfs_func_group(int kicker)
{
	int i;
	int id = -1;
	int total = sizeof(spm_dvfs_func_list) / sizeof(spm_dvfs_func_list[0]);

	for (i = 0; i < total; i++) {
		if (spm_dvfs_func_list[i].kicker_mask & (1 << kicker)) {
			id = i;
			break;
		}
	}
	return id;
}

int get_kicker_group_opp(int kicker, int group_id)
{
	u32 group_kickers;
	int id = -1;
	int i, result_opp = (NUM_OPP - 1);	/* the lowest power mode, OPP_1 */
	int total = sizeof(spm_dvfs_func_list) / sizeof(spm_dvfs_func_list[0]);

	if (group_id < 0)
		id = find_spm_dvfs_func_group(kicker);
	else
		id = group_id;

	if (id < 0 || id >= total) {
		vcorefs_err("Invalid group id(%d), return default opp(%d)\n", id, result_opp);
		return result_opp;
	}

	group_kickers = spm_dvfs_func_list[id].kicker_mask;
	for (i = 0; i < NUM_KICKER; i++) {
		if ((group_kickers & 1 << i) != 0) {
			if (kicker_table[i] < 0)
				continue;
			if (kicker_table[i] < result_opp)
				result_opp = kicker_table[i];
		}
	}
	return result_opp;
}


static int set_dvfs_with_opp(struct governor_profile *gvrctrl, struct kicker_config *krconf,
			     struct opp_profile *opp_ctrl_table)
{
	int ret = 0;
	int group_id = -1;
	int expect_vcore_uv;
	int expect_ddr_khz;
	int opp_idx;

	/* struct opp_profile *opp_ctrl_table = opp_table; */
	gvrctrl->curr_vcore_uv = vcorefs_get_curr_vcore();
	gvrctrl->curr_ddr_khz = vcorefs_get_curr_ddr();

	if (krconf->dvfs_opp < 0)
		opp_idx = (NUM_OPP - 1);
	else
		opp_idx = krconf->dvfs_opp;

	if (gvrctrl->vcore_dvs == 1)
		expect_vcore_uv = opp_ctrl_table[opp_idx].vcore_uv;
	else
		expect_vcore_uv = gvrctrl->curr_vcore_uv;

	if (gvrctrl->ddr_dfs == 1)
		expect_ddr_khz = opp_ctrl_table[opp_idx].ddr_khz;
	else
		expect_ddr_khz = gvrctrl->curr_ddr_khz;

	vcorefs_debug_mask(krconf->kicker, "opp: %d, vcore: %d(%d), fddr: %d(%d)\n",
		     krconf->dvfs_opp,
		     expect_vcore_uv, gvrctrl->curr_vcore_uv,
		     expect_ddr_khz, gvrctrl->curr_ddr_khz);

	/* find spm func group by kicker type */
	group_id = find_spm_dvfs_func_group(krconf->kicker);

	if (group_id < 0) {
		vcorefs_err("not find spm func for %s kicker\n", get_kicker_name(krconf->kicker));
		return -1;
	}
	if (krconf->kicker < NUM_KICKER) {
		/* check with history_opp_table to decide the real request opp (dvfs_opp) */
		krconf->dvfs_opp = get_kicker_group_opp(krconf->kicker, group_id);
	}
	vcorefs_debug_mask(krconf->kicker, "[%d]%s, opp: %d\n", group_id, spm_dvfs_func_list[group_id].purpose,
		     krconf->dvfs_opp);

	/* call spm with dvfs_opp */
	ret =
	    spm_dvfs_func_list[group_id].do_dvfs(krconf->dvfs_opp,
						 opp_ctrl_table[krconf->dvfs_opp].vcore_uv,
						 opp_ctrl_table[krconf->dvfs_opp].ddr_khz);

	/* update curr_vcore_uv and curr_ddr_khz */
	gvrctrl->curr_vcore_uv = opp_ctrl_table[krconf->dvfs_opp].vcore_uv;
	gvrctrl->curr_ddr_khz = opp_ctrl_table[krconf->dvfs_opp].ddr_khz;

	return ret;

}

static int do_dvfs_for_performance(struct kicker_config *krconf)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;

	return set_dvfs_with_opp(gvrctrl, krconf, opp_ctrl_table);
}

static int do_dvfs_for_low_power(struct kicker_config *krconf)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;

	return set_dvfs_with_opp(gvrctrl, krconf, opp_ctrl_table);
}

/*
 * main function
 */
int kick_dvfs_by_opp_index(struct kicker_config *krconf)
{
	int r = 0;

	if (krconf->opp == OPPI_PERF)
		r = do_dvfs_for_performance(krconf);
	else
		r = do_dvfs_for_low_power(krconf);

	vcorefs_debug_mask(krconf->kicker, "kick_dvfs_by_opp_index done, r: %d\n", r);

	return r;
}

/*
 * init vcorefs function
 */
int vcorefs_late_init_dvfs(void)
{
	struct kicker_config krconf;
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;
	bool plat_init_done = true;
	int plat_init_opp;
	int disp_w, disp_h;
	int uv;
	u32 mask = 0;

	vcorefs_info("disp_virtual(w:%d, h:%d) disp(w:%d, h:%d)\n",
		primary_display_get_virtual_width(), primary_display_get_virtual_height(),
		primary_display_get_width(), primary_display_get_height());

	disp_w = primary_display_get_virtual_width();
	disp_h = primary_display_get_virtual_height();
	if (disp_w == 0)
		disp_w = primary_display_get_width();
	if (disp_h == 0)
		disp_h = primary_display_get_height();

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if (disp_w * disp_h <= DISP_LCM_HD_SIZE) {
		gvrctrl->is_fhd_segment = false;
		gvrctrl->cpu_dvfs_req = 0;
	}
#else
#if 0 /* apply FHD policy for MD-CA */
	if (disp_w * disp_h <= DISP_LCM_HD_SIZE) {
		gvrctrl->is_fhd_segment = false;
		gvrctrl->cpu_dvfs_req = 0;
	} else {
		gvrctrl->is_fhd_segment = true;
		gvrctrl->cpu_dvfs_req = (1 << MD_CAT6_CA_DATALINK | (1 << MD_Position));
	}
#endif
#endif

	/* update boot_up_opp */
	uv = vcorefs_get_curr_vcore();
	if (opp_ctrl_table[OPPI_PERF].vcore_uv == uv)
		gvrctrl->boot_up_opp = OPPI_PERF;
	else
		gvrctrl->boot_up_opp = OPPI_LOW_PWR;
	vcorefs_info("curr uv=%d boot_up_opp=%d\n", uv, gvrctrl->boot_up_opp);

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	/* mt6750 series */
	if (gvrctrl->segment_code == MT6750_NORMAL_SEGMENT ||
		gvrctrl->segment_code == MT6750_NORMAL_5M_SEGMENT ||
		gvrctrl->segment_code == MT6738_5M_SEGMENT ||
		gvrctrl->segment_code == MT6738_SEGMENT) {
		if (gvrctrl->is_fhd_segment == true) {
			if (spm_read(SPM_POWER_ON_VAL0) & (1 << 14)) {
				gvrctrl->segment_policy = VCOREFS_SEGMENT_LPM;
				gvrctrl->vcore_dvfs_en = false;
				vcorefs_err("disable vcore_dvfs for DRAMC only K LPM(reg=0x%x) by seg=0x%x\n",
				 spm_read(SPM_POWER_ON_VAL0), gvrctrl->segment_code);
			} else {
				gvrctrl->segment_policy = VCOREFS_SEGMENT_LPM_EXCEPT_OVL;
			}
		} else {
			gvrctrl->segment_policy = VCOREFS_SEGMENT_LPM;
			gvrctrl->vcore_dvfs_en = false;
		}
	} else {
		gvrctrl->segment_policy = VCOREFS_SEGMENT_HPM_LPM;
	}
#else
	gvrctrl->segment_policy = VCOREFS_SEGMENT_HPM_LPM;

#endif
	vcorefs_info("segment=0x%x policy=%d vcore_dvfs_en=%d\n",
				 gvrctrl->segment_code, gvrctrl->segment_policy, gvrctrl->vcore_dvfs_en);
	if (gvrctrl->segment_policy == VCOREFS_SEGMENT_LPM) {
		gvrctrl->vcore_dvfs_en = false;
		vcorefs_info("vcore dvfs disable. segment(0x%x) boot_up_opp=%d\n",
			      gvrctrl->segment_code, gvrctrl->boot_up_opp);
		if (!(spm_read(SPM_POWER_ON_VAL0) & (1 << 14)))
			vcorefs_err("boot ddr freq is not expected as LPM (seg=0x%x)\n", gvrctrl->segment_code);
		if (gvrctrl->boot_up_opp != OPPI_LOW_PWR)
			vcorefs_err("boot vcore is not expected as LPM (seg=0x%x)\n", gvrctrl->segment_code);
	} else if (gvrctrl->segment_policy == VCOREFS_SEGMENT_LPM_EXCEPT_OVL) {
		disable_cg_fliper(); /* disable c+g fliper */
		mmdvfs_enable(0); /* disable mm dvfs */
		gvrctrl->cpu_dvfs_req = 0; /* disable MD request */
		/* only allow OVL kicker and GPU kicker */
		mask = (1U << NUM_KICKER) - 1 - (1U << KIR_OVL) - (1U << KIR_GPU);
		vcorefs_info("fake HD segment, adjust kr_mask=0x%x\n", mask);
		vcorefs_set_kr_req_mask(mask);
	}

	spm_vcorefs_set_cpu_dvfs_req(gvrctrl->cpu_dvfs_req, 0xFFFF);

	vcorefs_init_sram_debug();

	is_vcorefs_feature_enable();

	/* SPM_SW_RSV_5[0] init in spm module init */
	/* spm_vcorefs_set_opp_state(gvrctrl->boot_up_opp); */
	if (gvrctrl->vcore_dvfs_en == true)
		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO, gvrctrl->boot_up_opp);
	else
		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DIS_VCORE_DVS |
				     SPM_FLAG_DIS_VCORE_DFS, gvrctrl->boot_up_opp);

	mutex_lock(&governor_mutex);

	/* SPM_SW_RSV_5[0] need init first time */
	/* spm_vcorefs_set_opp_state(gvrctrl->boot_up_opp); */
	/* spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO, 0); */
	plat_init_opp = gvrctrl->late_init_opp;
	if (is_vcorefs_feature_enable()) {
		if (gpu_kicker_init_opp == OPPI_PERF)  {
			kicker_table[KIR_GPU] = OPPI_PERF;
			gvrctrl->late_init_opp = OPPI_PERF;
			plat_init_opp = OPPI_PERF;
		}
		if (plat_init_opp != gvrctrl->boot_up_opp) {
			krconf.kicker = KIR_LATE_INIT;
			krconf.opp = plat_init_opp;
			krconf.dvfs_opp = plat_init_opp;
			kick_dvfs_by_opp_index(&krconf);
		} else {
			vcorefs_info("skip late_init kick(opp=%d)\n", gvrctrl->late_init_opp);
		}
	}

	mutex_unlock(&governor_mutex);
	/* inform manager for governor init down */
	vcorefs_drv_init(plat_init_done, gvrctrl->vcore_dvfs_en, plat_init_opp);

	vcorefs_info("[%s] plat_feature_en: %d, plat_init_opp: %d\n", __func__,
		     gvrctrl->vcore_dvfs_en, plat_init_opp);

	/* mutex_unlock(&governor_mutex); */

	return 0;
}

static int init_vcorefs_config(void)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct opp_profile *opp_ctrl_table = opp_table;
	int opp;

	mutex_lock(&governor_mutex);
	gvrctrl->curr_vcore_uv = vcorefs_get_curr_vcore();
	BUG_ON(gvrctrl->curr_vcore_uv == 0);
	gvrctrl->curr_ddr_khz = vcorefs_get_curr_ddr();

	for (opp = 0; opp < NUM_OPP; opp++) {
		opp_ctrl_table[opp].vcore_uv = vcorefs_get_vcore_by_steps(opp);
		opp_ctrl_table[opp].ddr_khz = vcorefs_get_ddr_by_steps(opp);
		vcorefs_info("OPP %d: vcore_uv: %d, ddr_khz: %d\n", opp,
			     opp_ctrl_table[opp].vcore_uv, opp_ctrl_table[opp].ddr_khz);
	}

	vcorefs_info("curr_vcore_uv: %d, curr_ddr_khz: %d\n", gvrctrl->curr_vcore_uv,
		     gvrctrl->curr_ddr_khz);

	update_vcore_pwrap_cmd(opp_ctrl_table);
	mutex_unlock(&governor_mutex);

	return 0;
}

static int vcorefs_governor_pm_callback(struct notifier_block *nb, unsigned long action, void *ptr)
{
	/* vcorefs_info("governor_pm_callback(action=%lu)", action); */
	switch (action) {
	case PM_POST_HIBERNATION:
		{
			vcorefs_info
			    ("restore sram debug info(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			     sram_debug_info[0], sram_debug_info[1], sram_debug_info[2],
			     sram_debug_info[3], sram_debug_info[4], sram_debug_info[5],
			     sram_debug_info[6]);
			vcorefs_info("sram count and latency clear to zero\n");
			spm_write(VCOREFS_SRAM_DVS_UP_COUNT, 0);
			spm_write(VCOREFS_SRAM_DFS_UP_COUNT, 0);
			spm_write(VCOREFS_SRAM_DVS_DOWN_COUNT, 0);
			spm_write(VCOREFS_SRAM_DFS_DOWN_COUNT, 0);
			spm_write(VCOREFS_SRAM_DVFS_UP_LATENCY, 0);
			spm_write(VCOREFS_SRAM_DVFS_DOWN_LATENCY, 0);
			spm_write(VCOREFS_SRAM_DVFS_LATENCY_SPEC, sram_debug_info[6]);
		}
		break;
	case PM_HIBERNATION_PREPARE:
		{
			struct kicker_config krconf;

			sram_debug_info[0] = spm_read(VCOREFS_SRAM_DVS_UP_COUNT);
			sram_debug_info[1] = spm_read(VCOREFS_SRAM_DFS_UP_COUNT);
			sram_debug_info[2] = spm_read(VCOREFS_SRAM_DVS_DOWN_COUNT);
			sram_debug_info[3] = spm_read(VCOREFS_SRAM_DFS_DOWN_COUNT);
			sram_debug_info[4] = spm_read(VCOREFS_SRAM_DVFS_UP_LATENCY);
			sram_debug_info[5] = spm_read(VCOREFS_SRAM_DVFS_DOWN_LATENCY);
			sram_debug_info[6] = spm_read(VCOREFS_SRAM_DVFS_LATENCY_SPEC);
			vcorefs_info
			    ("backup sram debug info(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			     sram_debug_info[0], sram_debug_info[1], sram_debug_info[2],
			     sram_debug_info[3], sram_debug_info[4], sram_debug_info[5],
			     sram_debug_info[6]);
			if (is_vcorefs_feature_enable()) {
				krconf.kicker = KIR_SYSFS;
				krconf.opp = OPPI_PERF;
				krconf.dvfs_opp = OPPI_PERF;
				kick_dvfs_by_opp_index(&krconf);
			}
			vcorefs_set_feature_en(false);
		}
		break;
	case PM_RESTORE_PREPARE:
	case PM_SUSPEND_PREPARE:
	case PM_POST_SUSPEND:
	case PM_POST_RESTORE:
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int __init vcorefs_module_init(void)
{
	int r;
	struct governor_profile *gvrctrl = &governor_ctrl;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-vcorefs");
	if (!node)
		vcorefs_err("find VCORE_DVFS node failed\n");

	vcorefs_sram_base = of_iomap(node, 0);
	if (!vcorefs_sram_base) {
		vcorefs_err("FAILED TO MAP SRAM MEMORY OF VCORE DVFS\n");
		return -ENOMEM;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,spm_vcorefs_err_eint");
	if (!node) {
		vcorefs_err("find spm_vcorefs_err_eint failed\n");
	} else {
		int ret;
		u32 ints[2];

		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);
		spm_vcorefs_err_irq = irq_of_parse_and_map(node, 0);
		ret =
		    request_irq(spm_vcorefs_err_irq, spm_vcorefs_err_handler,
				IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND, "spm_vcorefs_err_eint", NULL);
	}

	gvrctrl->segment_code = get_devinfo_with_index(21) & 0xff;
	vcorefs_info("efuse segment code = 0x%x\n", gvrctrl->segment_code);

	pm_notifier(vcorefs_governor_pm_callback, 0);

	r = init_vcorefs_config();
	if (r) {
		vcorefs_err("FAILED TO INIT CONFIG (%d)\n", r);
		return r;
	}

	r = init_vcorefs_sysfs();
	if (r) {
		vcorefs_err("FAILED TO CREATE /sys/power/vcorefs (%d)\n", r);
		return r;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&vcorefs_earlysuspend_desc);	/* set Screen ON/OFF state to SPM for MD DVFS control */
#else
	memset(&vcorefs_fb_notif, 0, sizeof(vcorefs_fb_notif));
	vcorefs_fb_notif.notifier_call = vcorefs_fb_notifier_callback;
	r = fb_register_client(&vcorefs_fb_notif);
#endif
	vcorefs_set_log_mask(vcorefs_log_mask);
	vcorefs_info("vcorefs_sram_base = %p, spm_vcorefs_err_irq = %d\n", vcorefs_sram_base,
		     spm_vcorefs_err_irq);
	return r;
}
module_init(vcorefs_module_init);

/*late_init is called after dyna_load_fw ready */
/* late_initcall_sync(vcorefs_late_init_dvfs); */

void vcorefs_set_feature_enable(bool enable)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);
	gvrctrl->vcore_dvfs_en = enable;
	vcorefs_info("%s DVFS feature\n", enable ? "Enable" : "Disable");
	mutex_unlock(&governor_mutex);
}

/*
 *  pcm_flag control ( dvs / dfs / isr_debug_on)
 */
int vcorefs_enable_dvs(bool enable)
{
	struct opp_profile *opp_ctrl_table = opp_table;

	mutex_lock(&governor_mutex);

	/* dvs disabled: only adjust freq, voltage should be HPM  */
	if (enable == false) {
		/* set force_HPM before disable dvs */
		spm_vcorefs_set_dvfs_hpm_force(OPPI_PERF, opp_ctrl_table[OPPI_PERF].vcore_uv,
					       opp_ctrl_table[OPPI_PERF].ddr_khz);

		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DIS_VCORE_DVS, 0);
		/* release force_HPM */
		spm_vcorefs_set_dvfs_hpm_force(OPPI_LOW_PWR, opp_ctrl_table[OPPI_LOW_PWR].vcore_uv,
					       opp_ctrl_table[OPPI_LOW_PWR].ddr_khz);
	} else {
		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO, 0);
	}

	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_enable_dfs(bool enable)
{
	struct opp_profile *opp_ctrl_table = opp_table;

	mutex_lock(&governor_mutex);

	/* dfs disabled: only adjust voltage, freq should be LPM */
	if (enable == false) {
		/*set force_LPM before disable dfs */
		spm_vcorefs_set_dvfs_lpm_force(OPPI_LOW_PWR, opp_ctrl_table[OPPI_LOW_PWR].vcore_uv,
					       opp_ctrl_table[OPPI_LOW_PWR].ddr_khz);
		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DIS_VCORE_DFS, 0);
		/* release force_LPM */
		spm_vcorefs_set_dvfs_lpm_force(OPPI_PERF, opp_ctrl_table[OPPI_PERF].vcore_uv,
					       opp_ctrl_table[OPPI_PERF].ddr_khz);
	} else {
		spm_go_to_vcore_dvfs(SPM_FLAG_RUN_COMMON_SCENARIO, 0);
	}

	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_enable_debug_isr(bool enable)
{
	struct governor_profile *gvrctrl = &governor_ctrl;
	int flag = SPM_FLAG_RUN_COMMON_SCENARIO;

	vcorefs_info("enable_debug_isr(%d)\n", enable);
	mutex_lock(&governor_mutex);

	if (gvrctrl->vcore_dvs == false)
		flag |= SPM_FLAG_DIS_VCORE_DVS;
	if (gvrctrl->ddr_dfs == false)
		flag |= SPM_FLAG_DIS_VCORE_DFS;

	if (enable == true)
		flag |= SPM_FLAG_EN_MET_DBG_FOR_VCORE_DVFS;

	spm_go_to_vcore_dvfs(flag, 0);

	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_enable_perform_bw(bool enable)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);
	gvrctrl->perform_bw_enable = enable;
	spm_vcorefs_enable_perform_bw(enable);
	mutex_unlock(&governor_mutex);
	return 0;
}

int vcorefs_enable_total_bw(bool enable)
{
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);
	gvrctrl->total_bw_enable = enable;
	spm_vcorefs_enable_total_bw(enable);
	mutex_unlock(&governor_mutex);
	return 0;
}

/*
 * AutoK related API
 */
void governor_apply_E2_SDIO_DVFS(void)
{
	vcorefs_info("check SRAM AUTOK REMARK=0x%x\n", spm_read(VCOREFS_SRAM_AUTOK_REMARK));
#if 0
	if (spm_read(VCOREFS_SRAM_AUTOK_REMARK) == VALID_AUTOK_REMARK_VAL) {
		int flag = (SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_EN_E2_SDIO_SOLUTION);
		vcorefs_crit("reload FW with flag=0x%x for E2-SDIO_ECO\n", flag);
		vcorefs_reload_spm_firmware(flag);
	}
#endif
}

void governor_autok_manager(void)
{
	int r;

	vcorefs_info("autok flow start\n");
	/* autok callback */
	r = emmc_autok();
	vcorefs_info("emmc_autok r: %d\n", r);
	r = sd_autok();
	vcorefs_info("sd_autok r: %d\n", r);
	r = sdio_autok();
	vcorefs_info("sdio_autok r: %d\n", r);

	governor_apply_E2_SDIO_DVFS();

	vcorefs_info("autok flow end\n");

}

bool governor_autok_check(int kicker, int opp)
{
	int is_autok = true;
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);
	if (!((1 << kicker) & gvrctrl->autok_kir_group)) {
		is_autok = false;
	} else if (gvrctrl->active_autok_kir != 0 && gvrctrl->active_autok_kir != kicker) {
		vcorefs_err("Not allow kir:%d autok ( other kir:%d on-going)\n", kicker,
			    gvrctrl->active_autok_kir);
		is_autok = false;
	} else {
		is_autok = true;
	}
	mutex_unlock(&governor_mutex);

	return is_autok;
}

bool governor_autok_lock_check(int kicker, int opp)
{
	bool lock_r = true;
	struct governor_profile *gvrctrl = &governor_ctrl;

	mutex_lock(&governor_mutex);

	if (gvrctrl->active_autok_kir == 0) {
		gvrctrl->active_autok_kir = kicker;	/* start autok */
		lock_r = true;
	} else if (kicker == gvrctrl->active_autok_kir) {
		lock_r = true;	/* continue autok */
	} else {
		BUG();
	}

	if (opp == OPPI_UNREQ) {
		gvrctrl->active_autok_kir = 0;
		lock_r = false;
	}
	mutex_unlock(&governor_mutex);
	return lock_r;
}
