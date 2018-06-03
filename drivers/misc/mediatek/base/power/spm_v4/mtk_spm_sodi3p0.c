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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mt-plat/mtk_secure_api.h>

/* #include <mach/irqs.h> */
#include <mach/mtk_gpt.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif

#include <mt-plat/mtk_boot.h>
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_io.h>

#include <mtk_spm_sodi3.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_pmic_wrap.h>

#include <mtk_power_gs_api.h>

/**************************************
 * only for internal debug
 **************************************/

#define SODI3_TAG     "[SODI3] "
#define sodi3_err(fmt, args...)		pr_err(SODI3_TAG fmt, ##args)
#define sodi3_warn(fmt, args...)	pr_warn(SODI3_TAG fmt, ##args)
#define sodi3_debug(fmt, args...)	pr_debug(SODI3_TAG fmt, ##args)

#define SPM_PCMWDT_EN               1
#define SPM_BYPASS_SYSPWREQ         1

#define LOG_BUF_SIZE					(256)
#define SODI3_LOGOUT_TIMEOUT_CRITERIA	(20)
#define SODI3_LOGOUT_INTERVAL_CRITERIA	(5000U) /* unit:ms */

static struct pwr_ctrl sodi3_ctrl = {
	.wake_src = WAKE_SRC_FOR_SODI3,

	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_AND,
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_0_dbc_en = 1,
	.md_ddr_en_1_dbc_en = 0,
	.md_mask_b = 1,
	.sspm_mask_b = 1,
	.lte_mask_b = 0,
	.srcclkeni_mask_b = 1,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_ddr_en_dbc_en = 1,
	.conn_mask_b = 1,
	.conn_apsrc_sel = 0,

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_lte_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 0,
	.spm_ddren_req = 0,
	.spm_rsv_src_req = 0,
	.spm_ddren_2_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_SRC_MASK */
#if SPM_BYPASS_SYSPWREQ
	.csyspwreq_mask = 1,
#endif
	.ccif0_md_event_mask_b = 1,
	.ccif0_ap_event_mask_b = 1,
	.ccif1_md_event_mask_b = 1,
	.ccif1_ap_event_mask_b = 1,
	.ccifmd_md1_event_mask_b = 0,
	.ccifmd_md2_event_mask_b = 0,
	.dsi0_vsync_mask_b = 0,
	.dsi1_vsync_mask_b = 0,
	.dpi_vsync_mask_b = 0,
	.isp0_vsync_mask_b = 0,
	.isp1_vsync_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 1,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 1,
	.sspm_srcclkena_infra_mask_b = 0,
	.srcclkeni_infra_mask_b = 0,
	.md_apsrc_req_0_infra_mask_b = 0,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 0,
	.sspm_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 1,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 1,
	.md_vrf18_req_1_mask_b = 0,
	.md1_dvfs_req_mask = 0x3,
	.cpu_dvfs_req_mask = 1,
	.emi_bw_dvfs_req_mask = 1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0,
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 1,
	.cpu_md_dvfs_req_merge_mask_b = 0,
	.md_ddr_en_dvfs_halt_mask_b = 0,
	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,
	.conn_ddr_en_mask_b = 1,
	.disp_req_mask_b = 1,
	.disp1_req_mask_b = 1,
	.mfg_req_mask_b = 0,
	.ufs_srcclkena_mask_b = 0,
	.ufs_vrf18_req_mask_b = 0,
	.ps_c2k_rccif_wake_mask_b = 0,
	.l1_c2k_rccif_wake_mask_b = 0,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
	.dramc_spcmd_apsrc_req_mask_b = 0,
	.emi_boost_dvfs_req_2_mask_b = 0,
	.emi_bw_dvfs_req_2_mask = 0,
	.gce_vrf18_req_mask_b = 0,

	/* SPM_WAKEUP_EVENT_MASK */
	.spm_wakeup_event_mask = 0xF1683A08,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0xFFFFFFFF,

	/* SPM_SRC3_MASK */
	.md_ddr_en_2_0_mask_b = 0,
	.md_ddr_en_2_1_mask_b = 0,
	.conn_ddr_en_2_mask_b = 0,
	.dramc_spcmd_apsrc_req_2_mask_b = 0,
	.spare1_ddren_2_mask_b = 0,
	.spare2_ddren_2_mask_b = 0,
	.ddren_emi_self_refresh_ch0_mask_b = 0,
	.ddren_emi_self_refresh_ch1_mask_b = 0,
	.ddren_mm_state_mask_b = 0,
	.ddren_sspm_apsrc_req_mask_b = 0,
	.ddren_dqssoc_req_mask_b = 0,
	.ddren2_emi_self_refresh_ch0_mask_b = 0,
	.ddren2_emi_self_refresh_ch1_mask_b = 0,
	.ddren2_mm_state_mask_b = 0,
	.ddren2_sspm_apsrc_req_mask_b = 0,
	.ddren2_dqssoc_req_mask_b = 0,

	/* MP0_CPU0_WFI_EN */
	.mp0_cpu0_wfi_en = 1,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 1,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 1,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 1,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 1,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 1,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 1,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 1,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_sodi3 = {
	.pwrctrl = &sodi3_ctrl,
};

static bool gSpm_sodi3_en = true;
static int by_md2ap_count;


static void spm_sodi3_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	/* FIXME: */
#if 0
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	unsigned int value = 0;
	unsigned int vcore_lp_mode = 0;

	/* Set PMIC wrap table for Vproc/Vsram voltage decreased */
	/* VSRAM_DVFS1 */
	pmic_read_interface_nolock(PMIC_RG_VSRAM_DVFS1_VOSEL_ADDR,
								&value,
								PMIC_RG_VSRAM_DVFS1_VOSEL_MASK,
								PMIC_RG_VSRAM_DVFS1_VOSEL_SHIFT);

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_1_VSRAM_NORMAL,
								value);

	/* VSRAM_DVFS2 */
	pmic_read_interface_nolock(PMIC_RG_VSRAM_DVFS2_VOSEL_ADDR,
								&value,
								PMIC_RG_VSRAM_DVFS2_VOSEL_MASK,
								PMIC_RG_VSRAM_DVFS2_VOSEL_SHIFT);

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_2_VSRAM_NORMAL,
								value);

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	spm_pmic_power_mode(PMIC_PWR_SODI3, 0, 0);

	vcore_lp_mode = !!(operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE);

	pmic_config_interface_nolock(
		PMIC_RG_BUCK_VCORE_HW0_OP_EN_ADDR,
		vcore_lp_mode,
		PMIC_RG_BUCK_VCORE_HW0_OP_EN_MASK,
		PMIC_RG_BUCK_VCORE_HW0_OP_EN_SHIFT);

	pmic_config_interface_nolock(
		PMIC_RG_VSRAM_VCORE_HW0_OP_EN_ADDR,
		vcore_lp_mode,
		PMIC_RG_VSRAM_VCORE_HW0_OP_EN_MASK,
		PMIC_RG_VSRAM_VCORE_HW0_OP_EN_SHIFT);
#endif

	__spm_sync_pcm_flags(pwrctrl);
#endif
}

static void spm_sodi3_post_process(void)
{
#if 0
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
#endif
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_sodi3_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE) ?
		SPM_OPT_VCORE_LP_MODE : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_ENTER_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi3_notify_sspm_before_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_ENTER_SODI3);
	if (ret < 0)
		spm_crit2("SPM_ENTER_SODI3 async wait: ret %d", ret);
}

static void spm_sodi3_notify_sspm_after_wfi(u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_LEAVE_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi3_notify_sspm_after_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_LEAVE_SODI3);
	if (ret < 0)
		spm_crit2("SPM_LEAVE_SODI3 async wait: ret %d", ret);
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static void spm_sodi3_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
}

static void spm_sodi3_notify_sspm_before_wfi_async_wait(void)
{
}

static void spm_sodi3_notify_sspm_after_wfi(u32 operation_cond)
{
}

static void spm_sodi3_notify_sspm_after_wfi_async_wait(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

static void spm_sodi3_pcm_setup_before_wfi(
	u32 cpu, struct pcm_desc *pcmdesc, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage;

	spm_sodi3_pre_process(pwrctrl, operation_cond);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();
	mt_secure_call(MTK_SIP_KERNEL_SPM_SODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
}

static void spm_sodi3_pcm_setup_after_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	spm_sodi3_post_process();
}

static wake_reason_t spm_sodi3_output_log(
	struct wake_status *wakesta, struct pcm_desc *pcmdesc, u32 sodi3_flags)
{
	static unsigned long int sodi3_logout_prev_time;
	static int pre_emi_refresh_cnt;
	static int memPllCG_prev_status = 1;	/* 1:CG, 0:pwrdn */
	static unsigned int logout_sodi3_cnt;
	static unsigned int logout_selfrefresh_cnt;

	wake_reason_t wr = WR_NONE;
	unsigned long int sodi3_logout_curr_time = 0;
	int need_log_out = 0;

	if (!(sodi3_flags & SODI_FLAG_REDUCE_LOG) ||
			(sodi3_flags & SODI_FLAG_RESIDENCY)) {
		sodi3_warn("self_refresh = 0x%x, sw_flag = 0x%x, 0x%x\n",
				spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
				spm_read(DUMMY1_PWR_CON));
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false, "sodi3");
		if (sodi3_flags & SODI_FLAG_RESOURCE_USAGE)
			spm_resource_req_dump();
	} else {
		/*
		 * Log reduction mechanism, print debug information criteria :
		 * 1. SPM assert
		 * 2. Not wakeup by GPT
		 * 3. Residency is less than 20ms
		 * 4. Enter/no emi self-refresh change
		 * 5. Time from the last output log is larger than 5 sec
		 * 6. No wakeup event
		 * 7. CG/PD mode change
		*/
		sodi3_logout_curr_time = spm_get_current_time_ms();

		if (wakesta->assert_pc != 0) {
			need_log_out = 1;
		} else if ((wakesta->r12 & R12_APXGPT1_EVENT_B) == 0) {
			if (wakesta->r12 & R12_MD2AP_PEER_EVENT_B) {
				/* wake up by R12_MD2AP_PEER_EVENT_B */
				if ((by_md2ap_count >= 5) ||
				    ((sodi3_logout_curr_time - sodi3_logout_prev_time) > 20U)) {
					need_log_out = 1;
					by_md2ap_count = 0;
				} else if (by_md2ap_count == 0) {
					need_log_out = 1;
				}
				by_md2ap_count++;
			} else {
				need_log_out = 1;
			}
		} else if (wakesta->timer_out <= SODI3_LOGOUT_TIMEOUT_CRITERIA) {
			need_log_out = 1;
		} else if ((spm_read(SPM_PASR_DPD_0) == 0 && pre_emi_refresh_cnt > 0) ||
				(spm_read(SPM_PASR_DPD_0) > 0 && pre_emi_refresh_cnt == 0)) {
			need_log_out = 1;
		} else if ((sodi3_logout_curr_time - sodi3_logout_prev_time) > SODI3_LOGOUT_INTERVAL_CRITERIA) {
			need_log_out = 1;
		} else if (wakesta->r12 == 0) {
			need_log_out = 1;
		} else {
			int mem_status = 0;

			if (((spm_read(SPM_SW_FLAG) & SPM_FLAG_SODI_CG_MODE) != 0) ||
				((spm_read(DUMMY1_PWR_CON) & DUMMY1_PWR_ISO_LSB) != 0))
				mem_status = 1;

			if (memPllCG_prev_status != mem_status) {
				memPllCG_prev_status = mem_status;
				need_log_out = 1;
			}
		}

		logout_sodi3_cnt++;
		logout_selfrefresh_cnt += spm_read(SPM_PASR_DPD_0);
		pre_emi_refresh_cnt = spm_read(SPM_PASR_DPD_0);

		if (need_log_out == 1) {
			sodi3_logout_prev_time = sodi3_logout_curr_time;

			if ((wakesta->assert_pc != 0) || (wakesta->r12 == 0)) {
				sodi3_err("WAKE UP BY ASSERT, SELF_REFRESH = 0x%x, SW_FLAG = 0x%x, 0x%x\n",
						spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON));

				sodi3_err("SODI3_CNT = %d, SELF_REFRESH_CNT = 0x%x, SPM_PC = 0x%0x, R13 = 0x%x, DEBUG_FLAG = 0x%x\n",
						logout_sodi3_cnt, logout_selfrefresh_cnt,
						wakesta->assert_pc, wakesta->r13, wakesta->debug_flag);

				sodi3_err("R12 = 0x%x, R12_E = 0x%x, RAW_STA = 0x%x, IDLE_STA = 0x%x, EVENT_REG = 0x%x, ISR = 0x%x\n",
						wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
						wakesta->event_reg, wakesta->isr);
				wr = WR_PCM_ASSERT;
			} else {
				char buf[LOG_BUF_SIZE] = { 0 };
				int i;

				if (wakesta->r12 & WAKE_SRC_R12_PCM_TIMER) {
					if (wakesta->wake_misc & WAKE_MISC_PCM_TIMER)
						strcat(buf, " PCM_TIMER");

					if (wakesta->wake_misc & WAKE_MISC_TWAM)
						strcat(buf, " TWAM");

					if (wakesta->wake_misc & WAKE_MISC_CPU_WAKE)
						strcat(buf, " CPU");
				}
				for (i = 1; i < 32; i++) {
					if (wakesta->r12 & (1U << i)) {
						strcat(buf, wakesrc_str[i]);
						wr = WR_WAKE_SRC;
					}
				}
				WARN_ON(strlen(buf) >= LOG_BUF_SIZE);

				sodi3_warn("wake up by %s, self_refresh = 0x%x, sw_flag = 0x%x, 0x%x\n",
						buf, spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON));

				sodi3_warn("sodi3_cnt = %d, self_refresh_cnt = 0x%x, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x\n",
						logout_sodi3_cnt, logout_selfrefresh_cnt,
						wakesta->timer_out, wakesta->r13, wakesta->debug_flag);

				sodi3_warn("r12 = 0x%x, r12_e = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
						wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
						wakesta->event_reg, wakesta->isr);

				if (sodi3_flags & SODI_FLAG_RESOURCE_USAGE)
					spm_resource_req_dump();
			}
			logout_sodi3_cnt = 0;
			logout_selfrefresh_cnt = 0;
		}
	}

	return wr;
}

wake_reason_t spm_go_to_sodi3(u32 spm_flags, u32 spm_data, u32 sodi3_flags, u32 operation_cond)
{
#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	int wd_ret;
	struct wd_api *wd_api;
#endif
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_sodi3.pwrctrl;
	u32 cpu = spm_data;

	spm_sodi3_footprint(SPM_SODI3_ENTER);

	if (spm_get_sodi_mempll() == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE; /* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE; /* PDN mode */

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

#if 0
	/* for gps only case */
	if (spm_for_gps_flag) {
		/* sodi3_debug("spm_for_gps_flag %d\n", spm_for_gps_flag); */
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_ULPOSC_OFF;
	}
#endif

	pwrctrl->timer_val = 2 * 32768;	/* 2 sec */

#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
	} else
		spm_crit2("FAILED TO GET WD API\n");
#endif

	soidle3_before_wfi(cpu);

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

	spm_sodi3_notify_sspm_before_wfi(pwrctrl, operation_cond);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_SLEEP);

	spm_sodi3_footprint(SPM_SODI3_ENTER_SPM_FLOW);

	spm_sodi3_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, operation_cond);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_sodi3_notify_sspm_before_wfi_async_wait();

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[1]);
#endif

	spm_sodi3_footprint_val((1 << SPM_SODI3_ENTER_WFI) |
		(1 << SPM_SODI3_B4) | (1 << SPM_SODI3_B5) | (1 << SPM_SODI3_B6));

	if (sodi3_flags & SODI_FLAG_DUMP_LP_GS)
		; /* mt_power_gs_dump_sodi3(); */

	spm_trigger_wfi_for_sodi(pwrctrl->pcm_flags);

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[2]);
#endif

	spm_sodi3_footprint(SPM_SODI3_LEAVE_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS))
		request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	spm_sodi3_notify_sspm_after_wfi(operation_cond);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_sodi3_pcm_setup_after_wfi(pwrctrl, operation_cond);

	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_AWAKE);

	wr = spm_sodi3_output_log(&wakesta, pcmdesc, sodi3_flags);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SPM_FLOW);

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	soidle3_after_wfi(cpu);

	spm_sodi3_notify_sspm_after_wfi_async_wait();

#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	if (!wd_ret) {
		if (!pwrctrl->wdt_disable)
			wd_api->wd_resume_notify();
		else
			spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	}
#endif

	spm_sodi3_reset_footprint();

#if 1
	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();
#endif

	return wr;
}

void spm_enable_sodi3(bool en)
{
	gSpm_sodi3_en = en;
}

bool spm_get_sodi3_en(void)
{
	return gSpm_sodi3_en;
}

void spm_sodi3_init(void)
{
	sodi3_debug("spm_sodi3_init\n");
	spm_sodi3_aee_init();
}

MODULE_DESCRIPTION("SPM-SODI3 Driver v0.1");
