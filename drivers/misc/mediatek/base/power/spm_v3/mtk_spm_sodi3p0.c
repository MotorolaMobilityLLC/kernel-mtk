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

#include <mtk_spm_sodi.h>
#include <mtk_spm_sodi3.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mt6337_api.h>

#include <mtk_power_gs_api.h>

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mtk_hps_internal.h>
#endif

/**************************************
 * only for internal debug
 **************************************/
#define PCM_SEC_TO_TICK(sec)	(sec * 32768)
#if defined(CONFIG_MACH_MT6759)
#define SPM_PCMWDT_EN		(0)
#else
#define SPM_PCMWDT_EN		(1)
#endif

static struct pwr_ctrl sodi3_ctrl = {
	.wake_src = WAKE_SRC_FOR_SODI3,

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 1,
#endif

#if defined(CONFIG_MACH_MT6799)
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
	.reg_wakeup_event_mask = 0xF1A92208,

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

#elif defined(CONFIG_MACH_MT6759)
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
	.reg_disp0_req_mask_b = 1,
	.reg_disp1_req_mask_b = 1,
	.reg_disp_od_req_mask_b = 0,
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
	/* .reg_gce_vrf18_req2_mask_b = 0, */

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
	/* .reg_ccif4_ap_event_mask_b = 1, */
	/* .reg_ccif4_md_event_mask_b = 1, */
	/* .reg_ccif5_ap_event_mask_b = 1, */
	/* .reg_ccif5_md_event_mask_b = 1, */

	/* SPM_WAKEUP_EVENT_MASK */
	.reg_wakeup_event_mask = 0xF1282208,

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
	/* .spm_rsv_con2 = 0, */ /* TODO */

	/* Auto-gen End */
#endif
};

struct spm_lp_scen __spm_sodi3 = {
	.pwrctrl = &sodi3_ctrl,
};

static bool gSpm_sodi3_en = true;

static void spm_sodi3_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
#ifndef CONFIG_MACH_MT6759
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

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_VCORE_SUSPEND,
								pwrctrl->vcore_volt_pmic_val);

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

#ifdef CONFIG_MACH_MT6799
	wk_mt6337_set_lp_setting();
#endif
#endif
#endif
	__spm_sync_pcm_flags(pwrctrl);
}

static void spm_sodi3_post_process(void)
{
#ifndef CONFIG_MACH_MT6759
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#ifdef CONFIG_MACH_MT6799
	wk_mt6337_restore_lp_setting();
#endif
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

	spm_opt |= univpll_is_used() ? SPM_OPT_UNIVPLL_STAT : 0;
	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE) ?
		SPM_OPT_VCORE_LP_MODE : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;
	spm_d.u.suspend.vcore_volt_pmic_val = pwrctrl->vcore_volt_pmic_val;

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
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_SODI, PWR_OPP_LEVEL, pwrctrl->opp_level);
}

static void spm_sodi3_pcm_setup_after_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	spm_sodi3_post_process();
}

static void spm_sodi3_setup_wdt(struct pwr_ctrl *pwrctrl, void **api)
{
#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api = NULL;

	if (!get_wd_api(&wd_api)) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
		pwrctrl->wdt_disable = 0;
	} else {
		spm_crit2("FAILED TO GET WD API\n");
		api = NULL;
		pwrctrl->wdt_disable = 1;
	}
	*api = wd_api;
#else
	pwrctrl->wdt_disable = 1;
#endif
}

static void spm_sodi3_resume_wdt(struct pwr_ctrl *pwrctrl, void *api)
{
#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api = (struct wd_api *)api;

	if (!pwrctrl->wdt_disable && wd_api != NULL) {
		wd_api->wd_resume_notify();
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	} else {
		spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
	}
#endif
}

wake_reason_t spm_go_to_sodi3(u32 spm_flags, u32 spm_data, u32 sodi3_flags, u32 operation_cond)
{
	void *api = NULL;
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_sodi3.pwrctrl;
	u32 cpu = spm_data;
	int ch;

	spm_sodi3_footprint(SPM_SODI3_ENTER);

	if (spm_get_sodi_mempll() == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE; /* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE; /* PDN mode */

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* set_pwrctrl_pcm_flags1(pwrctrl, spm_data); */
	/* need be called after set_pwrctrl_pcm_flags1() */
	/* spm_set_dummy_read_addr(false); */

	/* for gps only case */
	if (spm_for_gps_flag) {
		/* sodi3_debug("spm_for_gps_flag %d\n", spm_for_gps_flag); */
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_ULPOSC_OFF;
	}

	pwrctrl->timer_val = PCM_SEC_TO_TICK(2);

	spm_sodi3_setup_wdt(pwrctrl, &api);
	soidle3_before_wfi(cpu);

	/* need be called before spin_lock_irqsave() */
	ch = get_channel_lock(0);
	pwrctrl->opp_level = __spm_check_opp_level(ch);
	pwrctrl->vcore_volt_pmic_val = __spm_get_vcore_volt_pmic_val(true, ch);
	wakesta.dcs_ch = (u32)ch;

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

#ifdef CONFIG_MTK_ICCS_SUPPORT
	iccs_enter_low_power_state();
#endif
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

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_SPM_FLOW);

	spm_sodi3_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, operation_cond);

	spm_sodi3_footprint(SPM_SODI3_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_sodi3_notify_sspm_before_wfi_async_wait();

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[1]);
#endif

	spm_sodi3_footprint_val((1 << SPM_SODI3_ENTER_WFI) |
		(1 << SPM_SODI3_B4) | (1 << SPM_SODI3_B5) | (1 << SPM_SODI3_B6));

	if (sodi3_flags & SODI_FLAG_DUMP_LP_GS)
		mt_power_gs_dump_sodi3();

	spm_trigger_wfi_for_sodi(pwrctrl->pcm_flags);

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[2]);
#endif

	spm_sodi3_footprint(SPM_SODI3_LEAVE_WFI);

	spm_sodi3_notify_sspm_after_wfi(operation_cond);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_sodi3_pcm_setup_after_wfi(pwrctrl, operation_cond);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SPM_FLOW);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS))
		request_uart_to_wakeup();
RESTORE_IRQ:
#endif
	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_AWAKE);

	wr = spm_sodi_output_log(&wakesta, pcmdesc, sodi3_flags|SODI_FLAG_3P0);

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

	soidle3_after_wfi(cpu);
	spm_sodi3_notify_sspm_after_wfi_async_wait();
	spm_sodi3_resume_wdt(pwrctrl, api);

	spm_sodi3_reset_footprint();

	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();

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
