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
#include <linux/string.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <mt-plat/mtk_secure_api.h>

#ifdef CONFIG_ARM64
#include <linux/irqchip/mtk-gic.h>
#endif
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
/* #include <mach/mtk_clkmgr.h> */
#include <mtk_cpuidle.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mtk_spm_misc.h>
#include <mtk_spm_sleep.h>
#include <mtk_dramc.h>

#include <mtk_spm_internal.h>
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#include <mtk_spm_pmic_wrap.h>
#include <mtk_pmic_api_buck.h>
#endif /* CONFIG_FPGA_EARLY_PORTING */

#include <mt-plat/mtk_ccci_common.h>

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
#include <mt-plat/mtk_usb2jtag.h>
#endif

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include <sspm_define.h>
#include <sspm_timesync.h>
#endif

/* #include <mtk_power_gs_api.h> */

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mtk_hps_internal.h>
#endif

/**************************************
 * only for internal debug
 **************************************/
#ifdef CONFIG_MTK_LDVT
#define SPM_PWAKE_EN            0
#define SPM_PCMWDT_EN           0
#define SPM_BYPASS_SYSPWREQ     1
#else
#define SPM_PWAKE_EN            1
#define SPM_PCMWDT_EN           1
#define SPM_BYPASS_SYSPWREQ     1
#endif

static int spm_dormant_sta;
int spm_ap_mdsrc_req_cnt;

struct wake_status suspend_info[20];
u32 log_wakesta_cnt;
u32 log_wakesta_index;
u8 spm_snapshot_golden_setting;

struct wake_status spm_wakesta; /* record last wakesta */
static unsigned int spm_sleep_count;

/**************************************
 * SW code for suspend
 **************************************/
#define SPM_SYSCLK_SETTLE       99	/* 3ms */

#define WAIT_UART_ACK_TIMES     10	/* 10 * 10us */

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_SSPM_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_SSPM_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_MD2AP_PEER_EVENT_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B)
#else
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_SSPM_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_SSPM_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_MD2AP_PEER_EVENT_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_SEJ_WDT_GPT_B)
#endif /* #if defined(CONFIG_MICROTRUST_TEE_SUPPORT) */

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

int __attribute__ ((weak)) mtk_enter_idle_state(int idx)
{
	pr_err("NO %s !!!\n", __func__);
	return -1;
}

void __attribute__((weak)) mt_cirq_clone_gic(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) mt_cirq_enable(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) mt_cirq_flush(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) mt_cirq_disable(void)
{
	pr_err("NO %s !!!\n", __func__);
}

enum spm_suspend_step {
	SPM_SUSPEND_ENTER = 0x00000001,
	SPM_SUSPEND_ENTER_UART_SLEEP = 0x00000003,
	SPM_SUSPEND_ENTER_WFI = 0x000000ff,
	SPM_SUSPEND_LEAVE_WFI = 0x000001ff,
	SPM_SUSPEND_ENTER_UART_AWAKE = 0x000003ff,
	SPM_SUSPEND_LEAVE = 0x000007ff,
};

static inline void spm_suspend_footprint(enum spm_suspend_step step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_spm_suspend_val(step);
#endif
}

static struct pwr_ctrl suspend_ctrl = {
	.wake_src = WAKE_SRC_FOR_SUSPEND,

	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_AND,
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_0_dbc_en = 0x1,
	.md_ddr_en_1_dbc_en = 0,
	.md_mask_b = 0x1,
	.sspm_mask_b = 0x1,
	.lte_mask_b = 0,
	.srcclkeni_mask_b = 0x1,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_ddr_en_dbc_en = 0x1,
	.conn_mask_b = 0x1,
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
	.csyspwreq_mask = 0x1,
#endif
	.ccif0_md_event_mask_b = 0x1,
	.ccif0_ap_event_mask_b = 0x1,
	.ccif1_md_event_mask_b = 0x1,
	.ccif1_ap_event_mask_b = 0x1,
	.ccifmd_md1_event_mask_b = 0,
	.ccifmd_md2_event_mask_b = 0,
	.dsi0_vsync_mask_b = 0,
	.dsi1_vsync_mask_b = 0,
	.dpi_vsync_mask_b = 0,
	.isp0_vsync_mask_b = 0,
	.isp1_vsync_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 0x1,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0x1,
	.sspm_srcclkena_infra_mask_b = 0,
	.srcclkeni_infra_mask_b = 0,
	.md_apsrc_req_0_infra_mask_b = 0,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 0,
	.sspm_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 0x1,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 0x1,
	.md_vrf18_req_1_mask_b = 0,
	.md1_dvfs_req_mask = 0x1,
	.cpu_dvfs_req_mask = 0x1,
	.emi_bw_dvfs_req_mask = 0x1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0,
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 0,
	.cpu_md_dvfs_req_merge_mask_b = 0,
	.md_ddr_en_dvfs_halt_mask_b = 0,
	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,
	.conn_ddr_en_mask_b = 0x1,
	.disp_req_mask_b = 0,
	.disp1_req_mask_b = 0,
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
	.spm_wakeup_event_mask = 0xF1783A18,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0xffffffff,

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
	.mp0_cpu0_wfi_en = 0x1,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 0x1,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 0x1,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 0x1,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 0x1,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 0x1,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 0x1,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 0x1,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_suspend = {
	.pwrctrl = &suspend_ctrl,
	.wakestatus = &suspend_info[0],
};

static void spm_trigger_wfi_for_sleep(struct pwr_ctrl *pwrctrl)
{
	/* FIXME: */
#if 0
	if (is_cpu_pdn(pwrctrl->pcm_flags))
		spm_dormant_sta = mtk_enter_idle_state(MTK_SUSPEND_MODE);
	else
		spm_dormant_sta = mtk_enter_idle_state(MTK_LEGACY_SUSPEND_MODE);
#else
	mt_secure_call(MTK_SIP_KERNEL_SPM_LEGACY_SLEEP, 0, 0, 0);
#endif

	if (spm_dormant_sta < 0)
		spm_crit2("spm_dormant_sta %d", spm_dormant_sta);

	if (is_infra_pdn(pwrctrl->pcm_flags))
		mtk_uart_restore();
}

static void spm_suspend_pcm_setup_before_wfi(u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl)
{
#ifdef CONFIG_MTK_ICCS_SUPPORT
	iccs_enter_low_power_state();
#endif

	spm_suspend_pre_process(pwrctrl);

	spm_set_sysclk_settle();
	__spm_sync_pcm_flags(pwrctrl);
	pwrctrl->timer_val = __spm_get_pcm_timer_val(pwrctrl);

	mt_secure_call(MTK_SIP_KERNEL_SPM_SUSPEND_ARGS, pwrctrl->pcm_flags, pwrctrl->pcm_flags1, pwrctrl->timer_val);
}

static void spm_suspend_pcm_setup_after_wfi(u32 cpu, struct pwr_ctrl *pwrctrl)
{
	spm_suspend_post_process(pwrctrl);
}

static wake_reason_t spm_output_wake_reason(struct wake_status *wakesta, struct pcm_desc *pcmdesc)
{
	wake_reason_t wr;
	int ddr_status = 0;
	int vcore_status = 0;

	if (spm_sleep_count >= 0xfffffff0)
		spm_sleep_count = 0;
	else
		spm_sleep_count++;

	wr = __spm_output_wake_reason(wakesta, pcmdesc, true, "suspend");

	memcpy(&suspend_info[log_wakesta_cnt], wakesta, sizeof(struct wake_status));
	suspend_info[log_wakesta_cnt].log_index = log_wakesta_index;

	if (log_wakesta_cnt >= 10) {
		log_wakesta_cnt = 0;
		spm_snapshot_golden_setting = 0;
	} else {
		log_wakesta_cnt++;
		log_wakesta_index++;
	}

	if (log_wakesta_index >= 0xFFFFFFF0)
		log_wakesta_index = 0;

	ddr_status = vcorefs_get_curr_ddr();
	/* FIXME: */
	/* vcore_status = vcorefs_get_curr_vcore(); */

	spm_crit2("suspend dormant state = %d, ddr = %d, vcore = %d, spm_sleep_count = %d\n",
		  spm_dormant_sta, ddr_status, vcore_status, spm_sleep_count);
	if (spm_ap_mdsrc_req_cnt != 0)
		spm_crit2("warning: spm_ap_mdsrc_req_cnt = %d, r7[ap_mdsrc_req] = 0x%x\n",
			  spm_ap_mdsrc_req_cnt, spm_read(SPM_POWER_ON_VAL1) & (1 << 17));

	if (wakesta->r12 & WAKE_SRC_R12_EINT_EVENT_B)
		mt_eint_print_status();

#ifdef CONFIG_MTK_CCCI_DEVICES
	/* if (wakesta->r13 & 0x18) { */
		spm_crit2("dump ID_DUMP_MD_SLEEP_MODE");
		exec_ccci_kern_func_by_md_id(0, ID_DUMP_MD_SLEEP_MODE, NULL, 0);
	/* } */
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_ECCCI_DRIVER
	if (wakesta->r12 & WAKE_SRC_R12_MD2AP_PEER_EVENT_B)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
	if (wakesta->r12 & WAKE_SRC_R12_CCIF1_EVENT_B)
		exec_ccci_kern_func_by_md_id(2, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif
#endif
	return wr;
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_suspend.pwrctrl->wake_src = wakesrc;
		else
			__spm_suspend.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_suspend.pwrctrl->wake_src = 0;
		else
			__spm_suspend.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

/*
 * wakesrc: WAKE_SRC_XXX
 */
u32 spm_get_sleep_wakesrc(void)
{
	return __spm_suspend.pwrctrl->wake_src;
}

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_PMIC
/* #include <cust_pmic.h> */
#if !defined(DISABLE_DLPT_FEATURE)
/* extern int get_dlpt_imix_spm(void); */
int __attribute__((weak)) get_dlpt_imix_spm(void)
{
	pr_err("NO %s !!!\n", __func__);
	return 0;
}
#endif
#endif
#endif

wake_reason_t spm_go_to_sleep(u32 spm_flags, u32 spm_data)
{
	u32 sec = 2;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api;
	int wd_ret;
#endif
	static wake_reason_t last_wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl;
	u32 cpu = 0;

	spm_suspend_footprint(SPM_SUSPEND_ENTER);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_PMIC
#if !defined(DISABLE_DLPT_FEATURE)
	get_dlpt_imix_spm();
#endif
#endif
#endif

	pwrctrl = __spm_suspend.pwrctrl;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* set_pwrctrl_pcm_flags1(pwrctrl, spm_data); */

	/* FIXME: */
#if 0
	/* for gps only case */
	if (spm_for_gps_flag) {
		spm_crit2("spm_for_gps_flag %d\n", spm_for_gps_flag);
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_ULPOSC_OFF;
	}
#endif

#if SPM_PWAKE_EN
	sec = _spm_get_wake_period(-1, last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
	} else
		spm_crit2("FAILED TO GET WD API\n");
#endif

	spin_lock_irqsave(&__spm_lock, flags);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_crit2("sec = %u, wakesrc = 0x%x (%u)(%u)\n",
		  sec, pwrctrl->wake_src, is_cpu_pdn(pwrctrl->pcm_flags),
		  is_infra_pdn(pwrctrl->pcm_flags));

	spm_suspend_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl);

	spm_suspend_footprint(SPM_SUSPEND_ENTER_UART_SLEEP);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_suspend_footprint(SPM_SUSPEND_ENTER_WFI);

	spm_trigger_wfi_for_sleep(pwrctrl);

	spm_suspend_footprint(SPM_SUSPEND_LEAVE_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	/* record last wakesta */
	__spm_get_wakeup_status(&spm_wakesta);

	spm_suspend_footprint(SPM_SUSPEND_ENTER_UART_AWAKE);

	spm_suspend_pcm_setup_after_wfi(0, pwrctrl);

	/* record last wakesta */
	last_wr = spm_output_wake_reason(&spm_wakesta, pcmdesc);
#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);

#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	if (!wd_ret) {
		if (!pwrctrl->wdt_disable)
			wd_api->wd_resume_notify();
		else
			spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	}
#endif

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
	if (usb2jtag_mode())
		mt_usb2jtag_resume();
#endif
	spm_suspend_footprint(0);

	if (last_wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();

	return last_wr;
}

int get_spm_sleep_count(struct seq_file *s, void *unused)
{
	seq_printf(s, "%d\n", spm_sleep_count);
	return 0;
}

int get_spm_last_wakeup_src(struct seq_file *s, void *unused)
{
	seq_printf(s, "0x%x\n", spm_wakesta.r12);
	return 0;
}

void spm_output_sleep_option(void)
{
	spm_notice("PWAKE_EN:%d, PCMWDT_EN:%d, BYPASS_SYSPWREQ:%d\n",
		   SPM_PWAKE_EN, SPM_PCMWDT_EN, SPM_BYPASS_SYSPWREQ);
}

/* record last wakesta */
u32 spm_get_last_wakeup_src(void)
{
	return spm_wakesta.r12;
}

u32 spm_get_last_wakeup_misc(void)
{
	return spm_wakesta.wake_misc;
}
MODULE_DESCRIPTION("SPM-Sleep Driver v0.1");
