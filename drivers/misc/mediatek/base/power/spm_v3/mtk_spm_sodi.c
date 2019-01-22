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

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* #include <mach/irqs.h> */
#include <mach/mtk_gpt.h>

#include <mt-plat/mtk_boot.h>
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_io.h>

#include <mtk_spm_sodi.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_pmic_wrap.h>

#include <mtk_power_gs_api.h>

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mtk_hps_internal.h>
#endif

/**************************************
 * only for internal debug
 **************************************/
#define LOG_BUF_SIZE					(256)
#define SODI_LOGOUT_TIMEOUT_CRITERIA    (20)
#define SODI_LOGOUT_MAXTIME_CRITERIA    (3000)
#define SODI_LOGOUT_INTERVAL_CRITERIA   (5000U)	/* unit:ms */

static struct pwr_ctrl sodi_ctrl;

struct spm_lp_scen __spm_sodi = {
	.pwrctrl = &sodi_ctrl,
};

/* 0:power-down mode, 1:CG mode */
static bool gSpm_SODI_mempll_pwr_mode;
static bool gSpm_sodi_en;
static bool gSpm_lcm_vdo_mode;
static int by_md2ap_count;
static unsigned long int sodi_logout_prev_time;
static int pre_emi_refresh_cnt;
static int memPllCG_prev_status = 1;	/* 1:CG, 0:pwrdn */
static unsigned int logout_sodi_cnt;
static unsigned int logout_selfrefresh_cnt;

static void spm_sodi_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#ifdef CONFIG_MACH_MT6799
	unsigned int value = 0;

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

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_VCORE_SUSPEND,
								pwrctrl->vcore_volt_pmic_val);

#endif
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	spm_pmic_power_mode(PMIC_PWR_SODI, 0, 0);
#endif
	__spm_sync_pcm_flags(pwrctrl);
}

static void spm_sodi_post_process(void)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_sodi_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= univpll_is_used() ? SPM_OPT_UNIVPLL_STAT : 0;
	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;
	spm_d.u.suspend.vcore_volt_pmic_val = pwrctrl->vcore_volt_pmic_val;

	ret = spm_to_sspm_command_async(SPM_ENTER_SODI, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi_notify_sspm_before_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_ENTER_SODI);
	if (ret < 0)
		spm_crit2("SPM_ENTER_SODI async wait: ret %d", ret);
}

static void spm_sodi_notify_sspm_after_wfi(u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_LEAVE_SODI, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi_notify_sspm_after_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_LEAVE_SODI);
	if (ret < 0)
		spm_crit2("SPM_LEAVE_SODI async wait: ret %d", ret);
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static void spm_sodi_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
}

static void spm_sodi_notify_sspm_before_wfi_async_wait(void)
{
}

static void spm_sodi_notify_sspm_after_wfi(u32 operation_cond)
{
}

static void spm_sodi_notify_sspm_after_wfi_async_wait(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

void spm_trigger_wfi_for_sodi(u32 pcm_flags)
{
	int spm_dormant_sta;

	if (is_cpu_pdn(pcm_flags))
		spm_dormant_sta = mtk_enter_idle_state(MTK_SODI_MODE);
	else
		spm_dormant_sta = mtk_enter_idle_state(MTK_LEGACY_SODI_MODE);

	if (spm_dormant_sta < 0)
		sodi_pr_err("spm_dormant_sta(%d) < 0\n", spm_dormant_sta);
}

static void spm_sodi_pcm_setup_before_wfi(
	u32 cpu, struct pcm_desc *pcmdesc, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage;

	spm_sodi_pre_process(pwrctrl, operation_cond);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();
	mt_secure_call(MTK_SIP_KERNEL_SPM_SODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_SODI, PWR_OPP_LEVEL, pwrctrl->opp_level);
}

static void spm_sodi_pcm_setup_after_wfi(u32 operation_cond)
{
	spm_sodi_post_process();
}

static inline bool spm_sodi_assert(struct wake_status *wakesta)
{
	return (wakesta->assert_pc != 0) || (wakesta->r12 == 0);
}

static bool spm_sodi_is_not_gpt_event(struct wake_status *wakesta, long int curr_time)
{
	bool logout = false;

	if ((wakesta->r12 & R12_APXGPT1_EVENT_B) == 0) {
		if (wakesta->r12 & R12_MD2AP_PEER_WAKEUP_EVENT) {
			/* wake up by R12_MD2AP_PEER_WAKEUP_EVENT */
			if ((by_md2ap_count >= 5) ||
			    ((curr_time - sodi_logout_prev_time) > 20U)) {
				logout = true;
				by_md2ap_count = 0;
			} else if (by_md2ap_count == 0) {
				logout = true;
			}
			by_md2ap_count++;
		} else {
			logout = true;
		}
	}
	return logout;
}

static inline bool spm_sodi_abnormal_residency(struct wake_status *wakesta)
{
	return (wakesta->timer_out <= SODI_LOGOUT_TIMEOUT_CRITERIA) ||
			(wakesta->timer_out >= SODI_LOGOUT_MAXTIME_CRITERIA);
}

static inline bool spm_sodi_change_emi_state(struct wake_status *wakesta)
{
	return (spm_read(SPM_PASR_DPD_0) == 0 && pre_emi_refresh_cnt > 0) ||
				(spm_read(SPM_PASR_DPD_0) > 0 && pre_emi_refresh_cnt == 0);
}

static inline bool spm_sodi_last_logout(long int curr_time)
{
	return (curr_time - sodi_logout_prev_time) > SODI_LOGOUT_INTERVAL_CRITERIA;
}

static inline bool spm_sodi_memPllCG(void)
{
	return ((spm_read(SPM_SW_FLAG) & SPM_FLAG_SODI_CG_MODE) != 0) ||
			((spm_read(DUMMY1_PWR_CON) & DUMMY1_PWR_ISO_LSB) != 0);
}

static bool spm_sodi_mem_mode_change(void)
{
	bool logout = false;
	int mem_status = 0;

	if (((spm_read(SPM_SW_FLAG) & SPM_FLAG_SODI_CG_MODE) != 0) ||
		((spm_read(DUMMY1_PWR_CON) & DUMMY1_PWR_ISO_LSB) != 0))
		mem_status = 1;

	if (memPllCG_prev_status != mem_status) {
		memPllCG_prev_status = mem_status;
		logout = true;
	}
	return logout;
}


wake_reason_t spm_sodi_output_log(
	struct wake_status *wakesta, struct pcm_desc *pcmdesc, u32 flags, u32 operation_cond)
{
	wake_reason_t wr = WR_NONE;
	unsigned long int sodi_logout_curr_time = 0;
	int need_log_out = 0;

#if defined(CONFIG_MACH_MT6799)
	/* Note: Print EMI Idle Fail */
	if (spm_read(SPM_SW_RSV_3) & 0x1)
		so_pr_info(flags, "SPM CHECK EMI IDLE FAIL\n");
#endif

	if (!(flags & SODI_FLAG_REDUCE_LOG) ||
			(flags & SODI_FLAG_RESIDENCY)) {
		so_pr_notice(flags, "self_refresh = 0x%x, sw_flag = 0x%x, 0x%x, oper_cond = 0x%0x\n",
				spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
				spm_read(DUMMY1_PWR_CON), operation_cond);
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false, (flags&SODI_FLAG_3P0) ? "sodi3" : "sodi");
		if (flags & SODI_FLAG_RESOURCE_USAGE)
			spm_resource_req_dump();
	} else {
		sodi_logout_curr_time = spm_get_current_time_ms();

		if (spm_sodi_assert(wakesta))
			need_log_out = SODI_LOGOUT_ASSERT;
		else if (spm_sodi_is_not_gpt_event(wakesta, sodi_logout_curr_time))
			need_log_out = SODI_LOGOUT_NOT_GPT_EVENT;
		else if (spm_sodi_abnormal_residency(wakesta))
			need_log_out = SODI_LOGOUT_RESIDENCY_ABNORMAL;
		else if (spm_sodi_change_emi_state(wakesta))
			need_log_out = SODI_LOGOUT_EMI_STATE_CHANGE;
		else if (spm_sodi_last_logout(sodi_logout_curr_time))
			need_log_out = SODI_LOGOUT_LONG_INTERVAL;
		else if (spm_sodi_mem_mode_change())
			need_log_out = SODI_LOGOUT_CG_PD_STATE_CHANGE;

		logout_sodi_cnt++;
		logout_selfrefresh_cnt += spm_read(SPM_PASR_DPD_0);
		pre_emi_refresh_cnt = spm_read(SPM_PASR_DPD_0);

		if (need_log_out != SODI_LOGOUT_NONE) {
			sodi_logout_prev_time = sodi_logout_curr_time;

			if (need_log_out == SODI_LOGOUT_ASSERT) {
				if (wakesta->assert_pc != 0) {
					so_pr_err(flags, "Warning: wakeup reason is WR_PCM_ASSERT!\n");
					wr = WR_PCM_ASSERT;
				} else if (wakesta->r12 == 0) {
					so_pr_err(flags, "Warning: wakeup reason is WR_UNKNOWN!\n");
					wr = WR_UNKNOWN;
				}
				so_pr_err(flags, "SELF_REFRESH = 0x%x, SW_FLAG = 0x%x, 0x%x, SODI_CNT = %d, SELF_REFRESH_CNT = 0x%x, ASSERT_PC = 0x%0x, R13 = 0x%x, DEBUG_FLAG = 0x%x, R12 = 0x%x, R12_E = 0x%x, RAW_STA = 0x%x, IDLE_STA = 0x%x, EVENT_REG = 0x%x, ISR = 0x%x\n",
						spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON), logout_sodi_cnt,
						logout_selfrefresh_cnt, wakesta->assert_pc, wakesta->r13,
						wakesta->debug_flag, wakesta->r12, wakesta->r12_ext, wakesta->raw_sta,
						wakesta->idle_sta, wakesta->event_reg, wakesta->isr);
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

				so_pr_notice(flags, "wake up by %s, self_refresh = 0x%x, sw_flag = 0x%x, 0x%x, %d, 0x%x, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, res_usage = 0x%x, op_cond = 0x%x, %d\n",
						buf, spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON), logout_sodi_cnt, logout_selfrefresh_cnt,
						wakesta->timer_out, wakesta->r13, wakesta->debug_flag,
						wakesta->r12, wakesta->r12_ext, wakesta->raw_sta,
						wakesta->idle_sta, wakesta->event_reg, wakesta->isr,
						spm_get_resource_usage(), operation_cond, need_log_out);
			}
			logout_sodi_cnt = 0;
			logout_selfrefresh_cnt = 0;
		}
	}

	return wr;
}

wake_reason_t spm_go_to_sodi(u32 spm_flags, u32 spm_data, u32 sodi_flags, u32 operation_cond)
{
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_sodi.pwrctrl;
	u32 cpu = spm_data;
	int ch;

	spm_sodi_footprint(SPM_SODI_ENTER);

	if (spm_get_sodi_mempll() == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE; /* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE; /* PDN mode */

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* set_pwrctrl_pcm_flags1(pwrctrl, spm_data); */
	/* need be called after set_pwrctrl_pcm_flags1() */
	/* spm_set_dummy_read_addr(false); */

	soidle_before_wfi(cpu);

	/* need be called before spin_lock_irqsave() */
	ch = get_channel_lock(0);
	pwrctrl->opp_level = __spm_check_opp_level(ch);
	pwrctrl->vcore_volt_pmic_val = __spm_get_vcore_volt_pmic_val(true, ch);
	wakesta.dcs_ch = (u32)ch;

	/* update pcm_flags with dcs flag */
	__spm_update_pcm_flags_dcs_workaround(pwrctrl, ch);

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

#ifdef CONFIG_MTK_ICCS_SUPPORT
	iccs_enter_low_power_state();
#endif
	spm_sodi_notify_sspm_before_wfi(pwrctrl, operation_cond);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_sodi_footprint(SPM_SODI_ENTER_UART_SLEEP);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi_flags & SODI_FLAG_DUMP_LP_GS)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	spm_sodi_footprint(SPM_SODI_ENTER_SPM_FLOW);

	spm_sodi_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, operation_cond);

	spm_sodi_footprint(SPM_SODI_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_sodi_notify_sspm_before_wfi_async_wait();

	spm_sodi_footprint_val((1 << SPM_SODI_ENTER_WFI) |
		(1 << SPM_SODI_B4) | (1 << SPM_SODI_B5) | (1 << SPM_SODI_B6));

	if (sodi_flags & SODI_FLAG_DUMP_LP_GS)
		mt_power_gs_dump_sodi3();

	spm_trigger_wfi_for_sodi(pwrctrl->pcm_flags);

	spm_sodi_footprint(SPM_SODI_LEAVE_WFI);

	spm_sodi_notify_sspm_after_wfi(operation_cond);

	spm_sodi_footprint(SPM_SODI_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_sodi_pcm_setup_after_wfi(operation_cond);

	spm_sodi_footprint(SPM_SODI_LEAVE_SPM_FLOW);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi_flags & SODI_FLAG_DUMP_LP_GS))
		request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	spm_sodi_footprint(SPM_SODI_ENTER_UART_AWAKE);

	wr = spm_sodi_output_log(&wakesta, pcmdesc, sodi_flags, operation_cond);

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	/* need be called after spin_unlock_irqrestore() */
	get_channel_unlock();

	soidle_after_wfi(cpu);

	spm_sodi_notify_sspm_after_wfi_async_wait();

	spm_sodi_reset_footprint();

	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();

	return wr;
}

void spm_sodi_set_vdo_mode(bool vdo_mode)
{
	gSpm_lcm_vdo_mode = vdo_mode;
}

bool spm_get_cmd_mode(void)
{
	return !gSpm_lcm_vdo_mode;
}

void spm_sodi_mempll_pwr_mode(bool pwr_mode)
{
	gSpm_SODI_mempll_pwr_mode = pwr_mode;
}

bool spm_get_sodi_mempll(void)
{
	return gSpm_SODI_mempll_pwr_mode;
}

void spm_enable_sodi(bool en)
{
	gSpm_sodi_en = en;
}

bool spm_get_sodi_en(void)
{
	return gSpm_sodi_en;
}

void spm_sodi_init(void)
{
	spm_sodi_aee_init();
}

MODULE_DESCRIPTION("SPM-SODI Driver v0.1");
