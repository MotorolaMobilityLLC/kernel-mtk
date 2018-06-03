/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
/* FIXME: Need to add MCSODI_ARGS SMC call */
#if 0
#include <mt-plat/mtk_secure_api.h>
#endif

#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_mcsodi.h>
#include <mtk_idle_profile.h>

#define MCSODI_LOGOUT_MAX_INTERVAL   (5000U)	/* unit:ms */

#if defined(CONFIG_MACH_MT6799)
static struct pwr_ctrl mcsodi_ctrl = {
	.wake_src = WAKE_SRC_FOR_MCSODI,

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 1,
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
	.reg_md_srcclkena_0_infra_mask_b = 1,
	.reg_md_srcclkena_1_infra_mask_b = 0,
	.reg_md_apsrc_req_0_infra_mask_b = 0,
	.reg_md_apsrc_req_1_infra_mask_b = 0,
	.reg_conn_srcclkena_infra_mask_b = 0,
	.reg_conn_infra_req_mask_b = 0,
	.reg_sspm_srcclkena_infra_mask_b = 0,
	.reg_sspm_infra_req_mask_b = 1,
	.reg_scp_srcclkena_infra_mask_b = 0,
	.reg_scp_infra_req_mask_b = 1,
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
	.reg_disp2_req_mask_b = 1,
	.reg_md_ddr_en_0_mask_b = 1,
	.reg_md_ddr_en_1_mask_b = 0,
	.reg_conn_ddr_en_mask_b = 0,

	/* SPM_SRC2_MASK */
	.reg_disp0_req_mask_b = 1,
	.reg_disp1_req_mask_b = 1,
	.reg_disp_od_req_mask_b = 1,
	.reg_mfg_req_mask_b = 0,
	.reg_vdec0_req_mask_b = 0,
	.reg_gce_req_mask_b = 1,
	.reg_gce_vrf18_req_mask_b = 1,
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
	.reg_srcclkeni1_mask_b = 0,
	.reg_srcclkeni2_mask_b = 0,
	.reg_md_apsrc_1_sel = 0,
	.reg_md_apsrc_0_sel = 0,
	.reg_conn_mask_b = 0,
	.reg_conn_apsrc_sel = 0,
	.reg_md_srcclkena_0_vrf18_mask_b = 1,

	/* SPM_WAKEUP_EVENT_MASK */
	.reg_wakeup_event_mask = 0xF0A92208,

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
#elif defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
/* TODO: Wait SPM auto-gen */
static struct pwr_ctrl mcsodi_ctrl;
#else
#error "Not support MC-SODI"
#endif

/* please put firmware to vendor/mediatek/proprietary/hardware/spm/mtxxxx/ */
struct spm_lp_scen __spm_mcsodi = {
	.pwrctrl = &mcsodi_ctrl,
	/* .pcmdesc = &mcsodi_pcm, */
};

static DEFINE_SPINLOCK(mcsodi_busy_spin_lock);
static volatile bool mcsodi_start;

bool spm_mcsodi_start(void)
{
	return mcsodi_start;
}

static void spm_output_log(struct wake_status *wakesta)
{
	unsigned long int curr_time = 0;
	static unsigned long int pre_time;
	static unsigned int logout_mcsodi_cnt;

	curr_time = spm_get_current_time_ms();

	if ((curr_time - pre_time > MCSODI_LOGOUT_MAX_INTERVAL)) {
		ms_warn("sw_flag = 0x%x, %d, timer_out= %u r13 = 0x%x, debug_flag = 0x%x, r12 = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
					spm_read(SPM_SW_FLAG), logout_mcsodi_cnt,
					wakesta->timer_out, wakesta->r13, wakesta->debug_flag,
					wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
					wakesta->event_reg, wakesta->isr);
		pre_time = curr_time;
		logout_mcsodi_cnt = 0;
	} else {
		logout_mcsodi_cnt++;
	}
}

static void spm_mcsodi_wfi(void)
{
	isb();
	mb();
	__asm__ __volatile__("wfi" : : : "memory");
}

static void spm_trigger_wfi_for_mcsodi(u32 pcm_flags)
{
/* TODO: Need to check ATF CPU legacy flow */
#if 0
	if (mtk_idle_cpu_wfi_criteria()) {
		int spm_dormant_sta;

		spm_dormant_sta = mtk_enter_idle_state(MTK_MCSODI_MODE);
		if (spm_dormant_sta < 0)
			ms_err("SMC ret(%d)\n", spm_dormant_sta);
	} else {
		spm_mcsodi_wfi();
	}
#else
	spm_mcsodi_wfi();
#endif
}

static void spm_mcsodi_pcm_setup_before_wfi(
	u32 cpu, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
/* FIXME: Need to add MCSODI_ARGS SMC call */
#if 0
	unsigned int resource_usage;

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();

	mt_secure_call(MTK_SIP_KERNEL_SPM_MCSODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_MCSODI, PWR_OPP_LEVEL, pwrctrl->opp_level);
#endif
}

static void spm_mcsodi_pcm_setup_after_wfi(u32 operation_cond)
{
}

void spm_go_to_mcsodi(u32 spm_flags, u32 cpu, u32 mcsodi_flags, u32 operation_cond)
{
	struct pwr_ctrl *pwrctrl = __spm_mcsodi.pwrctrl;
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_busy_spin_lock, flags);

	if (!mtk_idle_cpu_wfi_criteria())
		goto mcsodi_abort;

	mcsodi_start = true;
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	mcsodi_before_wfi(cpu);

	spm_mcsodi_footprint(SPM_MCSODI_ENTER);

	if (request_uart_to_sleep())
		goto mcsodi_abort;

	spm_mcsodi_footprint(SPM_MCSODI_SPM_FLOW);

	spm_mcsodi_footprint_val((1 << SPM_MCSODI_ENTER_WFI) |
				(1 << SPM_MCSODI_B2) | (1 << SPM_MCSODI_B3) |
				(1 << SPM_MCSODI_B4) | (1 << SPM_MCSODI_B5) |
				(1 << SPM_MCSODI_B6));

	spm_mcsodi_pcm_setup_before_wfi(cpu, pwrctrl, operation_cond);

	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);

	spm_trigger_wfi_for_mcsodi(pwrctrl->pcm_flags);

	return;

mcsodi_abort:
	spm_mcsodi_reset_footprint();
	mcsodi_start = false;
	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);
}

int spm_leave_mcsodi(u32 cpu, u32 operation_cond)
{
	struct wake_status wakesta;
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_busy_spin_lock, flags);

	if (mcsodi_start == false) {
		spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);
		return -1;
	}

	spm_mcsodi_footprint(SPM_MCSODI_LEAVE_WFI);
	request_uart_to_wakeup();
	__spm_get_wakeup_status(&wakesta);
	spm_mcsodi_pcm_setup_after_wfi(operation_cond);

	spm_output_log(&wakesta);
	mcsodi_after_wfi(cpu);
	spm_mcsodi_reset_footprint();
	mcsodi_start = false;

	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);

	return 0;
}

void spm_mcsodi_init(void)
{
}

