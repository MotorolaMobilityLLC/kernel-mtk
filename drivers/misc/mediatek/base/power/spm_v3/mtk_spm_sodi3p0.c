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
#ifdef CONFIG_MTK_SPM_IN_ATF
#include <mt-plat/mtk_secure_api.h>
#endif /* CONFIG_MTK_SPM_IN_ATF */

/* #include <mach/irqs.h> */
#include <mach/mtk_gpt.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif

#include <mt-plat/mtk_boot.h>
#ifdef CONFIG_MTK_SYS_CIRQ
#include <mt-plat/mtk_cirq.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_io.h>

#include <mtk_spm_sodi3.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>


/**************************************
 * only for internal debug
 **************************************/

#define SODI3_TAG     "[SODI3] "
#define sodi3_err(fmt, args...)		pr_err(SODI3_TAG fmt, ##args)
#define sodi3_warn(fmt, args...)	pr_warn(SODI3_TAG fmt, ##args)
#define sodi3_debug(fmt, args...)	pr_debug(SODI3_TAG fmt, ##args)

#define SPM_BYPASS_SYSPWREQ         1

#define LOG_BUF_SIZE					(256)
#define SODI3_LOGOUT_TIMEOUT_CRITERIA	(20)
#define SODI3_LOGOUT_INTERVAL_CRITERIA	(5000U) /* unit:ms */

/* FIXME: update sodi_ctrl */
static struct pwr_ctrl sodi3_ctrl = {
	.wake_src = WAKE_SRC_FOR_SODI3,

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 0,
#endif

	/* Auto-gen Start */

	/* SPM_CLK_CON */
	.reg_srcclken0_ctl = 0,
	.reg_srcclken1_ctl = 0x3,
	.reg_spm_lock_infra_dcm = 1,
	.reg_srcclken_mask = 1,
	.reg_md1_c32rm_en = 0,
	.reg_md2_c32rm_en = 0,
	.reg_clksq0_sel_ctrl = 0,
	.reg_clksq1_sel_ctrl = 1,
	.reg_srcclken0_en = 1,
	.reg_srcclken1_en = 0,
	.reg_sysclk0_src_mask_b = 0,
	.reg_sysclk1_src_mask_b = 0x20,

	/* SPM_SRC_REQ */
	.reg_spm_apsrc_req = 0,
	.reg_spm_f26m_req = 0,
	.reg_spm_infra_req = 0,
	.reg_spm_ddren_req = 0,
	.reg_spm_vrf18_req = 0,
	.reg_spm_dvfs_level0_req = 0,
	.reg_spm_dvfs_level1_req = 0,
	.reg_spm_dvfs_level2_req = 0,
	.reg_spm_dvfs_level3_req = 0,
	.reg_spm_dvfs_level4_req = 0,
	.reg_spm_sspm_mailbox_req = 0,
	.reg_spm_sw_mailbox_req = 0,
	.reg_spm_cksel2_req = 0,
	.reg_spm_cksel3_req = 0,

	/* SPM_SRC_MASK */
	.reg_csyspwreq_mask = 0,
	.reg_md_srcclkena_0_infra_mask_b = 0,
	.reg_md_srcclkena_1_infra_mask_b = 0,
	.reg_md_apsrc_req_0_infra_mask_b = 1,
	.reg_md_apsrc_req_1_infra_mask_b = 0,
	.reg_conn_srcclkena_infra_mask_b = 0,
	.reg_conn_infra_req_mask_b = 1,
	.reg_sspm_srcclkena_infra_mask_b = 0,
	.reg_sspm_infra_req_mask_b = 1,
	.reg_scp_srcclkena_infra_mask_b = 0,
	.reg_scp_infra_req_mask_b = 0,
	.reg_srcclkeni0_infra_mask_b = 0,
	.reg_srcclkeni1_infra_mask_b = 0,
	.reg_srcclkeni2_infra_mask_b = 0,
	.reg_ccif0_md_event_mask_b = 1,
	.reg_ccif0_ap_event_mask_b = 1,
	.reg_ccif1_md_event_mask_b = 1,
	.reg_ccif1_ap_event_mask_b = 1,
	.reg_ccif2_md_event_mask_b = 1,
	.reg_ccif2_ap_event_mask_b = 1,
	.reg_ccif3_md_event_mask_b = 1,
	.reg_ccif3_ap_event_mask_b = 1,
	.reg_ccifmd_md1_event_mask_b = 0,
	.reg_ccifmd_md2_event_mask_b = 0,
	.reg_c2k_ps_rccif_wake_mask_b = 1,
	.reg_c2k_l1_rccif_wake_mask_b = 0,
	.reg_ps_c2k_rccif_wake_mask_b = 1,
	.reg_l1_c2k_rccif_wake_mask_b = 0,
	.reg_disp2_req_mask_b = 0,
	.reg_md_ddr_en_0_mask_b = 1,
	.reg_md_ddr_en_1_mask_b = 0,
	.reg_conn_ddr_en_mask_b = 0,

	/* SPM_SRC2_MASK */
	.reg_disp0_req_mask_b = 0,
	.reg_disp1_req_mask_b = 0,
	.reg_disp_od_req_mask_b = 0,
	.reg_mfg_req_mask_b = 0,
	.reg_vdec0_req_mask_b = 0,
	.reg_gce_req_mask_b = 0,
	.reg_gce_vrf18_req_mask_b = 0,
	.reg_lpdma_req_mask_b = 0,
	.reg_conn_srcclkena_cksel2_mask_b = 0,
	.reg_sspm_apsrc_req_ddren_mask_b = 1,
	.reg_scp_apsrc_req_ddren_mask_b = 1,
	.reg_md_vrf18_req_0_mask_b = 1,
	.reg_md_vrf18_req_1_mask_b = 0,
	.reg_next_dvfs_level0_mask_b = 1,
	.reg_next_dvfs_level1_mask_b = 1,
	.reg_next_dvfs_level2_mask_b = 1,
	.reg_next_dvfs_level3_mask_b = 1,
	.reg_next_dvfs_level4_mask_b = 1,
	.reg_sw2spm_int0_mask_b = 1,
	.reg_sw2spm_int1_mask_b = 1,
	.reg_sw2spm_int2_mask_b = 1,
	.reg_sw2spm_int3_mask_b = 1,
	.reg_sspm2spm_int0_mask_b = 1,
	.reg_sspm2spm_int1_mask_b = 1,
	.reg_sspm2spm_int2_mask_b = 1,
	.reg_sspm2spm_int3_mask_b = 1,
	.reg_dqssoc_req_mask_b = 0,

	/* SPM_SRC3_MASK */
	.reg_mpwfi_op = 0,
	.reg_spm_resource_req_rsv1_4_mask_b = 0,
	.reg_spm_resource_req_rsv1_3_mask_b = 0,
	.reg_spm_resource_req_rsv1_2_mask_b = 0,
	.reg_spm_resource_req_rsv1_1_mask_b = 0,
	.reg_spm_resource_req_rsv1_0_mask_b = 0,
	.reg_spm_resource_req_rsv0_4_mask_b = 0,
	.reg_spm_resource_req_rsv0_3_mask_b = 0,
	.reg_spm_resource_req_rsv0_2_mask_b = 0,
	.reg_spm_resource_req_rsv0_1_mask_b = 0,
	.reg_spm_resource_req_rsv0_0_mask_b = 0,
	.reg_srcclkeni2_cksel3_mask_b = 0,
	.reg_srcclkeni2_cksel2_mask_b = 0,
	.reg_srcclkeni1_cksel3_mask_b = 0,
	.reg_srcclkeni1_cksel2_mask_b = 0,
	.reg_srcclkeni0_cksel3_mask_b = 0,
	.reg_srcclkeni0_cksel2_mask_b = 0,
	.reg_md_ddr_en_0_dbc_en = 1,
	.reg_md_ddr_en_1_dbc_en = 0,
	.reg_conn_ddr_en_dbc_en = 0,
	.reg_sspm_mask_b = 1,
	.reg_md_0_mask_b = 1,
	.reg_md_1_mask_b = 0,
	.reg_scp_mask_b = 1,
	.reg_srcclkeni0_mask_b = 1,
	.reg_srcclkeni1_mask_b = 1,
	.reg_srcclkeni2_mask_b = 0,
	.reg_md_apsrc_1_sel = 0,
	.reg_md_apsrc_0_sel = 0,
	.reg_conn_mask_b = 0,
	.reg_conn_apsrc_sel = 0,
	.reg_md_srcclkena_0_vrf18_mask_b = 1,

	/* SPM_WAKEUP_EVENT_MASK */
	.reg_wakeup_event_mask = 0xF0F92218,

	/* SPM_EXT_WAKEUP_EVENT_MASK */
	.reg_ext_wakeup_event_mask = 0xFFFFFFFF,

	/* SLEEP_MCU0_WFI_EN */
	.mcu0_wfi_en = 1,

	/* SLEEP_MCU1_WFI_EN */
	.mcu1_wfi_en = 1,

	/* SLEEP_MCU2_WFI_EN */
	.mcu2_wfi_en = 1,

	/* SLEEP_MCU3_WFI_EN */
	.mcu3_wfi_en = 1,

	/* SLEEP_MCU4_WFI_EN */
	.mcu4_wfi_en = 1,

	/* SLEEP_MCU5_WFI_EN */
	.mcu5_wfi_en = 1,

	/* SLEEP_MCU6_WFI_EN */
	.mcu6_wfi_en = 1,

	/* SLEEP_MCU7_WFI_EN */
	.mcu7_wfi_en = 1,

	/* SLEEP_MCU8_WFI_EN */
	.mcu8_wfi_en = 1,

	/* SLEEP_MCU9_WFI_EN */
	.mcu9_wfi_en = 1,

	/* SLEEP_MCU10_WFI_EN */
	.mcu10_wfi_en = 1,

	/* SLEEP_MCU11_WFI_EN */
	.mcu11_wfi_en = 1,

	/* SLEEP_MCU12_WFI_EN */
	.mcu12_wfi_en = 1,

	/* SLEEP_MCU13_WFI_EN */
	.mcu13_wfi_en = 1,

	/* SLEEP_MCU14_WFI_EN */
	.mcu14_wfi_en = 1,

	/* SLEEP_MCU15_WFI_EN */
	.mcu15_wfi_en = 0,

	/* SLEEP_MCU16_WFI_EN */
	.mcu16_wfi_en = 0,

	/* SLEEP_MCU17_WFI_EN */
	.mcu17_wfi_en = 0,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_sodi3 = {
	/*.pcmdesc = &sodi3_pcm,*/
	.pwrctrl = &sodi3_ctrl,
};

static bool gSpm_sodi3_en = true;


static void spm_sodi3_pre_process(u32 operation_cond)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	unsigned int vcore_lp_mode = !!(operation_cond);

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	spm_pmic_power_mode(PMIC_PWR_SODI3, 0, 0);

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
}

static void spm_sodi3_post_process(void)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_sodi3_notify_sspm_before_wfi(u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	spm_opt |= univpll_is_used() ? SPM_OPT_UNIVPLL_STAT : 0;
	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= operation_cond ? SPM_OPT_VCORE_LP_MODE   : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command(SPM_ENTER_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi3_notify_sspm_after_wfi(void)
{
	int ret;
	struct spm_data spm_d;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	ret = spm_to_sspm_command(SPM_LEAVE_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static void spm_sodi3_notify_sspm_before_wfi(u32 operation_cond)
{
}

static void spm_sodi3_notify_sspm_after_wfi(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

#ifdef CONFIG_MTK_SPM_IN_ATF

static void spm_sodi3_pcm_setup_before_wfi(
	u32 cpu, struct pcm_desc *pcmdesc, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage;

	spm_sodi3_notify_sspm_before_wfi(operation_cond);

	spm_sodi3_pre_process(operation_cond);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();
	mt_secure_call(MTK_SIP_KERNEL_SPM_SODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_SODI, PWR_OPP_LEVEL, pwrctrl->opp_level);
}

static void spm_sodi3_pcm_setup_after_wfi(void)
{
	spm_sodi3_notify_sspm_after_wfi();

	spm_sodi3_post_process();
}

#else /* defined(CONFIG_MTK_SPM_IN_ATF) */

static void spm_sodi3_pcm_setup_before_wfi(
	u32 cpu, struct pcm_desc *pcmdesc, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage;

	__spm_set_cpu_status(cpu);
	__spm_reset_and_init_pcm(pcmdesc);
	__spm_kick_im_to_fetch(pcmdesc);
	__spm_init_pcm_register();
	__spm_init_event_vector(pcmdesc);
	__spm_sync_vcore_dvfs_power_control(pwrctrl, __spm_vcorefs.pwrctrl);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();
	pwrctrl->reg_spm_vrf18_req = (resource_usage & SPM_RESOURCE_MAINPLL) ? 1 : 0;
	pwrctrl->reg_spm_apsrc_req = (resource_usage & SPM_RESOURCE_DRAM)    ? 1 : 0;
	pwrctrl->reg_spm_ddren_req = (resource_usage & SPM_RESOURCE_DRAM)    ? 1 : 0;
	pwrctrl->reg_spm_f26m_req  = (resource_usage & SPM_RESOURCE_CK_26M)  ? 1 : 0;

	__spm_set_power_control(pwrctrl);
	__spm_set_wakeup_event(pwrctrl);

	/* enable pcm wdt */
	__spm_set_pcm_wdt(1);

	spm_sodi3_notify_sspm_before_wfi(operation_cond);

	spm_sodi3_pre_process(operation_cond);

	__spm_kick_pcm_to_run(pwrctrl);
}

static void spm_sodi3_pcm_setup_after_wfi(void)
{
	spm_sodi3_notify_sspm_after_wfi();

	spm_sodi3_post_process();

	/* disable pcm wdt */
	__spm_set_pcm_wdt(0);

	__spm_clean_after_wakeup();

}
#endif /* CONFIG_MTK_SPM_IN_ATF */

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
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false);
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
		} else if ((wakesta->r12 & (0x1 << 4)) == 0) {
			need_log_out = 1;
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

				if (wakesta->r12 & WAKE_SRC_R12_PCMTIMER) {
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
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	int wd_ret;
	struct wd_api *wd_api;
#endif
	struct wake_status wakesta;
	unsigned long flags;
#ifdef CONFIG_MTK_GIC_V3_EXT
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_sodi3.pwrctrl;
	u32 cpu = spm_data;
	int ch;

	if (dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND].desc);
	else
		spm_crit2("dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND].ready %d",
			dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND].ready);

	spm_sodi3_footprint(SPM_SODI3_ENTER);

	if (spm_get_sodi_mempll() == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE; /* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE; /* PDN mode */

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* set_pwrctrl_pcm_flags1(pwrctrl, spm_data); */
	/* need be called after set_pwrctrl_pcm_flags1() */
	spm_set_dummy_read_addr(false);

	/* for gps only case */
	if (spm_for_gps_flag) {
		/* sodi3_debug("spm_for_gps_flag %d\n", spm_for_gps_flag); */
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_ULPOSC_OFF;
	}

	pwrctrl->timer_val = 2 * 32768;	/* 2 sec */

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret)
		wd_api->wd_suspend_notify();
#endif

	soidle3_before_wfi(cpu);

	/* need be called before spin_lock_irqsave() */
	ch = get_channel_lock();
	pwrctrl->opp_level = __spm_check_opp_level(ch);

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

#ifdef CONFIG_MTK_GIC_V3_EXT
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#ifdef CONFIG_MTK_SYS_CIRQ
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_SLEEP);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (request_uart_to_sleep()) {
		wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_SPM_FLOW);

	spm_sodi3_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, operation_cond);

	spm_sodi3_footprint_val((1 << SPM_SODI3_ENTER_WFI) |
				(1 << SPM_SODI3_B3) | (1 << SPM_SODI3_B4) |
				(1 << SPM_SODI3_B5) | (1 << SPM_SODI3_B6));

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[1]);
#endif

	spm_trigger_wfi_for_sodi(pwrctrl->pcm_flags);

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[2]);
#endif

	spm_sodi3_footprint(SPM_SODI3_LEAVE_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_sodi3_pcm_setup_after_wfi();

	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_AWAKE);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	request_uart_to_wakeup();
#endif

	wr = spm_sodi3_output_log(&wakesta, pcmdesc, sodi3_flags);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SPM_FLOW);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
RESTORE_IRQ:
#endif

#ifdef CONFIG_MTK_SYS_CIRQ
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#ifdef CONFIG_MTK_GIC_V3_EXT
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	/* need be called after spin_unlock_irqrestore() */
	get_channel_unlock();

	soidle3_after_wfi(cpu);

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	if (!wd_ret)
		wd_api->wd_resume_notify();
#endif

	spm_sodi3_reset_footprint();
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
