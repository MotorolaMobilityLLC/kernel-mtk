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
#include <linux/of_fdt.h>
#include <mt-plat/mtk_secure_api.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/* #include <mach/irqs.h> */
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
#include <mtk_spm_idle.h>
#include <mtk_cpuidle.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifndef CONFIG_MTK_ACAO_SUPPORT
#include <mach/mtk_gpt.h>
#endif
#endif
#include <mt-plat/mtk_ccci_common.h>
#include <mtk_spm_misc.h>
#include <mt-plat/upmu_common.h>

#include <mtk_spm_dpidle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>

#include <mtk_power_gs_api.h>

#include <mt-plat/mtk_io.h>

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mtk_hps_internal.h>
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#include <trace/events/mtk_idle_event.h>
#endif
#include <mtk_idle_internal.h>
#include <mtk_idle_profile.h>

/*
 * only for internal debug
 */
#define DPIDLE_TAG     "[DP] "
#define dpidle_dbg(fmt, args...)	pr_debug(DPIDLE_TAG fmt, ##args)

#define SPM_PWAKE_EN            1
#define SPM_BYPASS_SYSPWREQ     1

#define I2C_CHANNEL 2

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

#define CA70_BUS_CONFIG          0xF020002C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */
#define CA71_BUS_CONFIG          0xF020022C  /* (CA7MCUCFG_BASE + 0x1C) - 0x1020011c */

#define SPM_USE_TWAM_DEBUG	0

#define	DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA	20
#define	DPIDLE_LOG_DISCARD_CRITERIA			5000	/* ms */

#define reg_read(addr)         __raw_readl(IOMEM(addr))

enum spm_deepidle_step {
	SPM_DEEPIDLE_ENTER = 0x00000001,
	SPM_DEEPIDLE_ENTER_UART_SLEEP = 0x00000003,
	SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI = 0x00000007,
	SPM_DEEPIDLE_ENTER_WFI = 0x000000ff,
	SPM_DEEPIDLE_LEAVE_WFI = 0x000001ff,
	SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI = 0x000003ff,
	SPM_DEEPIDLE_ENTER_UART_AWAKE = 0x000007ff,
	SPM_DEEPIDLE_LEAVE = 0x00000fff,
	SPM_DEEPIDLE_SLEEP_DPIDLE = 0x80000000
};

void __attribute__((weak)) mt_power_gs_t_dump_dpidle(int count, ...)
{
}

static inline void spm_dpidle_footprint(enum spm_deepidle_step step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
#define CPU_FOOTPRINT_SHIFT	24
	aee_rr_rec_deepidle_val(step | (smp_processor_id() << CPU_FOOTPRINT_SHIFT));
#endif
}

#if defined(CONFIG_MACH_MT6799)
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src			= WAKE_SRC_FOR_DPIDLE,

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
	.reg_csyspwreq_mask = 1,
	.reg_md_srcclkena_0_infra_mask_b = 1,
	.reg_md_srcclkena_1_infra_mask_b = 0,
	.reg_md_apsrc_req_0_infra_mask_b = 0,
	.reg_md_apsrc_req_1_infra_mask_b = 0,
	.reg_conn_srcclkena_infra_mask_b = 0,
	.reg_conn_infra_req_mask_b = 0,
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
	.reg_mpwfi_op = 1,
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
	.reg_wakeup_event_mask = 0xF1F92218,

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
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,

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
	.reg_csyspwreq_mask = 1,
	.reg_md_srcclkena_0_infra_mask_b = 1,
	.reg_md_srcclkena_1_infra_mask_b = 0,
	.reg_md_apsrc_req_0_infra_mask_b = 0,
	.reg_md_apsrc_req_1_infra_mask_b = 0,
	.reg_conn_srcclkena_infra_mask_b = 1,
	.reg_conn_infra_req_mask_b = 0,
	.reg_sspm_srcclkena_infra_mask_b = 0,
	.reg_sspm_infra_req_mask_b = 1,
	.reg_scp_srcclkena_infra_mask_b = 0,
	.reg_scp_infra_req_mask_b = 1,
	.reg_srcclkeni0_infra_mask_b = 0,
	.reg_srcclkeni1_infra_mask_b = 0,
	.reg_srcclkeni2_infra_mask_b = 1,
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
	.reg_conn_ddr_en_mask_b = 1,

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
	.reg_gce_vrf18_req2_mask_b = 0,

	/* SPM_SRC3_MASK */
	.reg_mpwfi_op = 1,
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
	.reg_conn_ddr_en_dbc_en = 1,
	.reg_sspm_mask_b = 1,
	.reg_md_0_mask_b = 1,
	.reg_md_1_mask_b = 0,
	.reg_scp_mask_b = 1,
	.reg_srcclkeni0_mask_b = 1,
	.reg_srcclkeni1_mask_b = 1,
	.reg_srcclkeni2_mask_b = 1,
	.reg_md_apsrc_1_sel = 0,
	.reg_md_apsrc_0_sel = 0,
	.reg_conn_mask_b = 1,
	.reg_conn_apsrc_sel = 0,
	.reg_md_srcclkena_0_vrf18_mask_b = 1,

	/* SPM_SRC4_MASK */
	.reg_ccif4_ap_event_mask_b = 1,
	.reg_ccif4_md_event_mask_b = 1,
	.reg_ccif5_ap_event_mask_b = 1,
	.reg_ccif5_md_event_mask_b = 1,

	/* SPM_WAKEUP_EVENT_MASK */
	.reg_wakeup_event_mask = 0xF1682208,

	/* SPM_EXT_WAKEUP_EVENT_MASK */
	.reg_ext_wakeup_event_mask = 0xFFFFFFFF,

	/* MCU0_WFI_EN */
	.mcu0_wfi_en = 1,

	/* MCU1_WFI_EN */
	.mcu1_wfi_en = 1,

	/* MCU2_WFI_EN */
	.mcu2_wfi_en = 1,

	/* MCU3_WFI_EN */
	.mcu3_wfi_en = 1,

	/* MCU4_WFI_EN */
	.mcu4_wfi_en = 1,

	/* MCU5_WFI_EN */
	.mcu5_wfi_en = 1,

	/* MCU6_WFI_EN */
	.mcu6_wfi_en = 1,

	/* MCU7_WFI_EN */
	.mcu7_wfi_en = 1,

	/* MCU8_WFI_EN */
	.mcu8_wfi_en = 1,

	/* MCU9_WFI_EN */
	.mcu9_wfi_en = 1,

	/* MCU10_WFI_EN */
	.mcu10_wfi_en = 1,

	/* MCU11_WFI_EN */
	.mcu11_wfi_en = 1,

	/* MCU12_WFI_EN */
	.mcu12_wfi_en = 1,

	/* MCU13_WFI_EN */
	.mcu13_wfi_en = 1,

	/* MCU14_WFI_EN */
	.mcu14_wfi_en = 1,

	/* MCU15_WFI_EN */
	.mcu15_wfi_en = 0,

	/* MCU16_WFI_EN */
	.mcu16_wfi_en = 0,

	/* MCU17_WFI_EN */
	.mcu17_wfi_en = 0,

	/* SPM_RSV_CON2 */
	.spm_rsv_con2 = 0,

	/* Auto-gen End */
};
#elif defined(CONFIG_MACH_MT6775)
static struct pwr_ctrl dpidle_ctrl;
#endif

struct spm_lp_scen __spm_dpidle = {
	.pwrctrl	= &dpidle_ctrl,
};

static unsigned int dpidle_log_discard_cnt;
static unsigned int dpidle_log_print_prev_time;

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_dpidle_notify_sspm_before_wfi(bool sleep_dpidle, u32 operation_cond, struct pwr_ctrl *pwrctrl)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= sleep_dpidle ?      SPM_OPT_SLEEP_DPIDLE : 0;
	spm_opt |= univpll_is_used() ? SPM_OPT_UNIVPLL_STAT : 0;
	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE) ? SPM_OPT_VCORE_LP_MODE : 0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) && !sleep_dpidle) ?
					SPM_OPT_XO_UFS_OFF :
					0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_CLKBUF_BBLPM) && !sleep_dpidle) ?
					SPM_OPT_CLKBUF_ENTER_BBLPM :
					0;

	spm_d.u.suspend.spm_opt = spm_opt;
	spm_d.u.suspend.vcore_volt_pmic_val = pwrctrl->vcore_volt_pmic_val;

	ret = spm_to_sspm_command_async(SPM_DPIDLE_ENTER, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_dpidle_notify_sspm_before_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_DPIDLE_ENTER);
	if (ret < 0)
		spm_crit2("SPM_DPIDLE_ENTER async wait: ret %d", ret);
}

static void spm_dpidle_notify_sspm_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_opt |= sleep_dpidle ?      SPM_OPT_SLEEP_DPIDLE : 0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) && !sleep_dpidle) ?
					SPM_OPT_XO_UFS_OFF :
					0;
	spm_opt |= ((operation_cond & DEEPIDLE_OPT_CLKBUF_BBLPM) && !sleep_dpidle) ?
					SPM_OPT_CLKBUF_ENTER_BBLPM :
					0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_DPIDLE_LEAVE, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

void spm_dpidle_notify_sspm_after_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_DPIDLE_LEAVE);
	if (ret < 0)
		spm_crit2("SPM_DPIDLE_LEAVE async wait: ret %d", ret);
}
#else
static void spm_dpidle_notify_sspm_before_wfi(bool sleep_dpidle, u32 operation_cond, struct pwr_ctrl *pwrctrl)
{
}

static void spm_dpidle_notify_sspm_before_wfi_async_wait(void)
{
}

static void spm_dpidle_notify_sspm_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
}

void spm_dpidle_notify_sspm_after_wfi_async_wait(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
	int spm_dormant_sta = 0;

	if (is_cpu_pdn(pwrctrl->pcm_flags))
		spm_dormant_sta = mtk_enter_idle_state(MTK_DPIDLE_MODE);
	else {
		#if defined(CONFIG_MACH_MT6775)
		/* Note:
		 *	To verify cpu not pdn path,
		 *	need to comment out all cmd in CPU_PM_ENTER case
		 *	at gic_cpu_pm_notifier() @ drivers/irqchip/irq-gic-v3.c
		 */
		mt_secure_call(MTK_SIP_KERNEL_SPM_ARGS, SPM_ARGS_DPIDLE, 0, 0);
		mt_secure_call(MTK_SIP_KERNEL_SPM_LEGACY_SLEEP, 0, 0, 0);
		mt_secure_call(MTK_SIP_KERNEL_SPM_ARGS, SPM_ARGS_DPIDLE_FINISH, 0, 0);
		#else
		spm_dormant_sta = mtk_enter_idle_state(MTK_LEGACY_DPIDLE_MODE);
		#endif
	}

	if (spm_dormant_sta < 0)
		pr_err("dpidle spm_dormant_sta(%d) < 0\n", spm_dormant_sta);
}

static void spm_dpidle_pcm_setup_before_wfi(bool sleep_dpidle, u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage = 0;

	spm_dpidle_pre_process(operation_cond, pwrctrl);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = (!sleep_dpidle) ? spm_get_resource_usage() : 0;

#if defined(CONFIG_MACH_MT6775)
	mt_secure_call(MTK_SIP_KERNEL_SPM_DPIDLE_ARGS, pwrctrl->pcm_flags, resource_usage, pwrctrl->pcm_flags1);
#else
	mt_secure_call(MTK_SIP_KERNEL_SPM_DPIDLE_ARGS, pwrctrl->pcm_flags, resource_usage, 0);
#endif
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS, SPM_PWR_CTRL_DPIDLE, PWR_OPP_LEVEL, pwrctrl->opp_level);

	if (sleep_dpidle) {
#if defined(CONFIG_MACH_MT6775)
		mt_secure_call(MTK_SIP_KERNEL_SPM_SLEEP_DPIDLE_ARGS, pwrctrl->timer_val, pwrctrl->wake_src,
			       pwrctrl->pcm_flags1);
#else
		mt_secure_call(MTK_SIP_KERNEL_SPM_SLEEP_DPIDLE_ARGS, pwrctrl->timer_val, pwrctrl->wake_src, 0);
#endif
	}
}

static void spm_dpidle_pcm_setup_after_wfi(bool sleep_dpidle, u32 operation_cond)
{
	spm_dpidle_post_process();
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = wakesrc;
		else
			__spm_dpidle.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = 0;
		else
			__spm_dpidle.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

static unsigned int spm_output_wake_reason(struct wake_status *wakesta,
											struct pcm_desc *pcmdesc,
											u32 log_cond,
											u32 operation_cond)
{
	unsigned int wr = WR_NONE;
	unsigned long int dpidle_log_print_curr_time = 0;
	bool log_print = false;
	static bool timer_out_too_short;

	if (mtk_idle_latency_profile_is_on())
		return wr;

#if defined(CONFIG_MACH_MT6799)
	/* Note: Print EMI Idle Fail */
	if (spm_read(SPM_SW_RSV_3) & 0x1)
		pr_info("DP: SPM CHECK EMI IDLE FAIL\n");
#endif

	if (log_cond & DEEPIDLE_LOG_FULL) {
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false, "dpidle");
		dpidle_dbg("oper_cond = %x\n", operation_cond);

		if (log_cond & DEEPIDLE_LOG_RESOURCE_USAGE)
			spm_resource_req_dump();
	} else if (log_cond & DEEPIDLE_LOG_REDUCED) {
		/* Determine print SPM log or not */
		dpidle_log_print_curr_time = spm_get_current_time_ms();

		if (wakesta->assert_pc != 0)
			log_print = true;
#if 0
		/* Not wakeup by GPT */
		else if ((wakesta->r12 & (0x1 << 4)) == 0)
			log_print = true;
		else if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			log_print = true;
#endif
		else if ((dpidle_log_print_curr_time - dpidle_log_print_prev_time) > DPIDLE_LOG_DISCARD_CRITERIA)
			log_print = true;

		if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			timer_out_too_short = true;

		/* Print SPM log */
		if (log_print == true) {
			dpidle_dbg("dpidle_log_discard_cnt = %d, timer_out_too_short = %d, oper_cond = %x\n",
						dpidle_log_discard_cnt,
						timer_out_too_short,
						operation_cond);
			wr = __spm_output_wake_reason(wakesta, pcmdesc, false, "dpidle");

			if (log_cond & DEEPIDLE_LOG_RESOURCE_USAGE)
				spm_resource_req_dump();

			dpidle_log_print_prev_time = dpidle_log_print_curr_time;
			dpidle_log_discard_cnt = 0;
			timer_out_too_short = false;
		} else {
			dpidle_log_discard_cnt++;

			wr = WR_NONE;
		}
	}

#ifdef CONFIG_MTK_ECCCI_DRIVER
	if (wakesta->r12 & WAKE_SRC_R12_MD2AP_PEER_WAKEUP_EVENT)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif

	return wr;
}

#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758) || \
	defined(CONFIG_MACH_MT6775)
#define DPIDLE_ACTIVE_TIME	1 /* sec */
static struct timeval pre_dpidle_time;

int dpidle_active_status(void)
{
	struct timeval current_time;

	do_gettimeofday(&current_time);

	if ((current_time.tv_sec - pre_dpidle_time.tv_sec) > DPIDLE_ACTIVE_TIME)
		return 0;
	else
		return 1;
}
EXPORT_SYMBOL(dpidle_active_status);
#endif

unsigned int spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 log_cond, u32 operation_cond)
{
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	unsigned int wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	u32 cpu = smp_processor_id();
	int ch;

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER);

	pwrctrl = __spm_dpidle.pwrctrl;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
#if defined(CONFIG_MACH_MT6775)
	if (is_big_buck_pdn_by_spm()) {
		spm_data |= (SPM_RSV_CON2_BIG_BUCK_ON_EN |
			     SPM_RSV_CON2_BIG_BUCK_OFF_EN);
	}

	set_pwrctrl_pcm_flags1(pwrctrl, spm_data);
#endif
	/* need be called after set_pwrctrl_pcm_flags1() */
	/* spm_set_dummy_read_addr(false); */

	/* need be called before spin_lock_irqsave() */
	ch = get_channel_lock(0);
	pwrctrl->opp_level = __spm_check_opp_level(ch);
	pwrctrl->vcore_volt_pmic_val =
						__spm_get_vcore_volt_pmic_val(
							!!(operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE),
							ch);
	wakesta.dcs_ch = (u32)ch;

	/* update pcm_flags with dcs flag */
	__spm_update_pcm_flags_dcs_workaround(pwrctrl, ch);

	spin_lock_irqsave(&__spm_lock, flags);

#ifdef CONFIG_MTK_ICCS_SUPPORT
	iccs_enter_low_power_state();
#endif
	profile_dp_start(PIDX_SSPM_BEFORE_WFI);
	spm_dpidle_notify_sspm_before_wfi(false, operation_cond, pwrctrl);
	profile_dp_end(PIDX_SSPM_BEFORE_WFI);

	profile_dp_start(PIDX_PRE_IRQ_PROCESS);
#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif
	profile_dp_end(PIDX_PRE_IRQ_PROCESS);

	profile_dp_start(PIDX_PCM_SETUP_BEFORE_WFI);
	spm_dpidle_pcm_setup_before_wfi(false, cpu, pcmdesc, pwrctrl, operation_cond);
	profile_dp_end(PIDX_PCM_SETUP_BEFORE_WFI);

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	profile_dp_start(PIDX_SSPM_BEFORE_WFI_ASYNC_WAIT);
	spm_dpidle_notify_sspm_before_wfi_async_wait();
	profile_dp_end(PIDX_SSPM_BEFORE_WFI_ASYNC_WAIT);

	/* Dump low power golden setting */
	if (operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN)
		mt_power_gs_dump_dpidle();

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_SLEEP);

#if defined(CONFIG_MTK_SERIAL)
	if (!(operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	trace_dpidle_rcuidle(cpu, 1);
#endif

	profile_dp_end(PIDX_ENTER_TOTAL);

	spm_trigger_wfi_for_dpidle(pwrctrl);

	profile_dp_start(PIDX_LEAVE_TOTAL);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	trace_dpidle_rcuidle(cpu, 0);
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_LEAVE_WFI);

#if defined(CONFIG_MTK_SERIAL)
	if (!(operation_cond & DEEPIDLE_OPT_DUMP_LP_GOLDEN))
		request_uart_to_wakeup();

RESTORE_IRQ:
#endif

	profile_dp_start(PIDX_SSPM_AFTER_WFI);
	spm_dpidle_notify_sspm_after_wfi(false, operation_cond);
	profile_dp_end(PIDX_SSPM_AFTER_WFI);

	spm_dpidle_footprint(SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	profile_dp_start(PIDX_PCM_SETUP_AFTER_WFI);
	spm_dpidle_pcm_setup_after_wfi(false, operation_cond);
	profile_dp_end(PIDX_PCM_SETUP_AFTER_WFI);

	spm_dpidle_footprint(SPM_DEEPIDLE_ENTER_UART_AWAKE);

	wr = spm_output_wake_reason(&wakesta, pcmdesc, log_cond, operation_cond);

	profile_dp_start(PIDX_POST_IRQ_PROCESS);
#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif
	profile_dp_end(PIDX_POST_IRQ_PROCESS);

	spin_unlock_irqrestore(&__spm_lock, flags);

	/* need be called after spin_unlock_irqrestore() */
	get_channel_unlock();

	spm_dpidle_footprint(0);

	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();

#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758) || \
	defined(CONFIG_MACH_MT6775)
	do_gettimeofday(&pre_dpidle_time);
#endif

	return wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 * pwake_time:
 *    >= 0  = specific wakeup period
 */
unsigned int spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data)
{
	u32 sec = 0;
	u32 dpidle_timer_val = 0;
	u32 dpidle_wake_src = 0;
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
#if defined(CONFIG_WATCHDOG) && defined(CONFIG_MTK_WATCHDOG) && \
	defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api;
	int wd_ret;
#endif
	static unsigned int last_wr = WR_NONE;
	/* struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc; */
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	int cpu = smp_processor_id();
	int ch;

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER);

	pwrctrl = __spm_dpidle.pwrctrl;

	/* backup original dpidle setting */
	dpidle_timer_val = pwrctrl->timer_val;
	dpidle_wake_src = pwrctrl->wake_src;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
#if defined(CONFIG_MACH_MT6775)
	if (is_big_buck_pdn_by_spm()) {
		spm_data |= (SPM_RSV_CON2_BIG_BUCK_ON_EN |
			     SPM_RSV_CON2_BIG_BUCK_OFF_EN);
	}

	set_pwrctrl_pcm_flags1(pwrctrl, spm_data);
#endif
	/* need be called after set_pwrctrl_pcm_flags1() */
	/* spm_set_dummy_read_addr(false); */

#if SPM_PWAKE_EN
	sec = _spm_get_wake_period(-1, last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

	pwrctrl->wake_src = spm_get_sleep_wakesrc();

#if defined(CONFIG_WATCHDOG) && defined(CONFIG_MTK_WATCHDOG) && \
	defined(CONFIG_MTK_WD_KICKER)
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
	} else
		spm_crit2("FAILED TO GET WD API\n");
#endif

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
	spm_dpidle_notify_sspm_before_wfi(true, DEEPIDLE_OPT_VCORE_LP_MODE, pwrctrl);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_crit2("sleep_deepidle, sec = %u, wakesrc = 0x%x [%u][%u]\n",
		sec, pwrctrl->wake_src,
		is_cpu_pdn(pwrctrl->pcm_flags),
		is_infra_pdn(pwrctrl->pcm_flags));

	spm_dpidle_pcm_setup_before_wfi(true, cpu, pcmdesc, pwrctrl, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_dpidle_notify_sspm_before_wfi_async_wait();

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_SLEEP);

#if defined(CONFIG_MTK_SERIAL)
	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_WFI);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	trace_dpidle_rcuidle(cpu, 1);
#endif

	spm_trigger_wfi_for_dpidle(pwrctrl);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	trace_dpidle_rcuidle(cpu, 0);
#endif

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_LEAVE_WFI);

#if defined(CONFIG_MTK_SERIAL)
	request_uart_to_wakeup();
RESTORE_IRQ:
#endif

	spm_dpidle_notify_sspm_after_wfi(false, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_dpidle_pcm_setup_after_wfi(true, 0);

	spm_dpidle_footprint(SPM_DEEPIDLE_SLEEP_DPIDLE | SPM_DEEPIDLE_ENTER_UART_AWAKE);

	last_wr = __spm_output_wake_reason(&wakesta, pcmdesc, true, "sleep_dpidle");

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

#if defined(CONFIG_WATCHDOG) && defined(CONFIG_MTK_WATCHDOG) && \
	defined(CONFIG_MTK_WD_KICKER)
	if (!wd_ret) {
		if (!pwrctrl->wdt_disable)
			wd_api->wd_resume_notify();
		else
			spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	}
#endif

	/* restore original dpidle setting */
	pwrctrl->timer_val = dpidle_timer_val;
	pwrctrl->wake_src = dpidle_wake_src;

	spm_dpidle_notify_sspm_after_wfi_async_wait();

	spm_dpidle_footprint(0);

	if (last_wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();

	return last_wr;
}

void spm_deepidle_init(void)
{
	spm_deepidle_chip_init();
}

MODULE_DESCRIPTION("SPM-DPIdle Driver v0.1");
