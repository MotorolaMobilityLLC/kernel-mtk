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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <mt-plat/mtk_secure_api.h>

#include <mtk_spm_internal.h>
#include <mtk_sleep.h>

/**************************************
 * Macro and Inline
 **************************************/
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

/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
static char *pwr_ctrl_str[PWR_MAX_COUNT] = {
	[PWR_PCM_FLAGS] = "pcm_flags",
	[PWR_PCM_FLAGS_CUST] = "pcm_flags_cust",
	[PWR_PCM_FLAGS_CUST_SET] = "pcm_flags_cust_set",
	[PWR_PCM_FLAGS_CUST_CLR] = "pcm_flags_cust_clr",
	[PWR_TIMER_VAL] = "timer_val",
	[PWR_TIMER_VAL_CUST] = "timer_val_cust",
	[PWR_TIMER_VAL_RAMP_EN] = "timer_val_ramp_en",
	[PWR_TIMER_VAL_RAMP_EN_SEC] = "timer_val_ramp_en_sec",
	[PWR_WAKE_SRC] = "wake_src",
	[PWR_WAKE_SRC_CUST] = "wake_src_cust",
	[PWR_VCORE_VOLT_PMIC_VAL] = "vcore_volt_pmic_val",
	[PWR_OPP_LEVEL] = "opp_level",
	[PWR_WDT_DISABLE] = "wdt_disable",
	[PWR_WFI_OP] = "wfi_op",
	[PWR_MP0_CPUTOP_IDLE_MASK] = "mp0_cputop_idle_mask",
	[PWR_MP1_CPUTOP_IDLE_MASK] = "mp1_cputop_idle_mask",
	[PWR_MCUSYS_IDLE_MASK] = "mcusys_idle_mask",
	[PWR_MM_MASK_B] = "mm_mask_b",
	[PWR_MD_DDR_EN_0_DBC_EN] = "md_ddr_en_0_dbc_en",
	[PWR_MD_DDR_EN_1_DBC_EN] = "md_ddr_en_1_dbc_en",
	[PWR_MD_MASK_B] = "md_mask_b",
	[PWR_PMCU_MASK_B] = "pmcu_mask_b",
	[PWR_LTE_MASK_B] = "lte_mask_b",
	[PWR_SRCCLKENI_MASK_B] = "srcclkeni_mask_b",
	[PWR_MD_APSRC_1_SEL] = "md_apsrc_1_sel",
	[PWR_MD_APSRC_0_SEL] = "md_apsrc_0_sel",
	[PWR_CONN_DDR_EN_DBC_EN] = "conn_ddr_en_dbc_en",
	[PWR_CONN_MASK_B] = "conn_mask_b",
	[PWR_CONN_APSRC_SEL] = "conn_apsrc_sel",
	[PWR_SPM_APSRC_REQ] = "spm_apsrc_req",
	[PWR_SPM_F26M_REQ] = "spm_f26m_req",
	[PWR_SPM_LTE_REQ] = "spm_lte_req",
	[PWR_SPM_INFRA_REQ] = "spm_infra_req",
	[PWR_SPM_VRF18_REQ] = "spm_vrf18_req",
	[PWR_SPM_DVFS_REQ] = "spm_dvfs_req",
	[PWR_SPM_DVFS_FORCE_DOWN] = "spm_dvfs_force_down",
	[PWR_SPM_DDREN_REQ] = "spm_ddren_req",
	[PWR_SPM_RSV_SRC_REQ] = "spm_rsv_src_req",
	[PWR_SPM_DDREN_2_REQ] = "spm_ddren_2_req",
	[PWR_CPU_MD_DVFS_SOP_FORCE_ON] = "cpu_md_dvfs_sop_force_on",
	[PWR_CSYSPWREQ_MASK] = "csyspwreq_mask",
	[PWR_CCIF0_MD_EVENT_MASK_B] = "ccif0_md_event_mask_b",
	[PWR_CCIF0_AP_EVENT_MASK_B] = "ccif0_ap_event_mask_b",
	[PWR_CCIF1_MD_EVENT_MASK_B] = "ccif1_md_event_mask_b",
	[PWR_CCIF1_AP_EVENT_MASK_B] = "ccif1_ap_event_mask_b",
	[PWR_CCIFMD_MD1_EVENT_MASK_B] = "ccifmd_md1_event_mask_b",
	[PWR_CCIFMD_MD2_EVENT_MASK_B] = "ccifmd_md2_event_mask_b",
	[PWR_DSI0_VSYNC_MASK_B] = "dsi0_vsync_mask_b",
	[PWR_DSI1_VSYNC_MASK_B] = "dsi1_vsync_mask_b",
	[PWR_DPI_VSYNC_MASK_B] = "dpi_vsync_mask_b",
	[PWR_ISP0_VSYNC_MASK_B] = "isp0_vsync_mask_b",
	[PWR_ISP1_VSYNC_MASK_B] = "isp1_vsync_mask_b",
	[PWR_MD_SRCCLKENA_0_INFRA_MASK_B] = "md_srcclkena_0_infra_mask_b",
	[PWR_MD_SRCCLKENA_1_INFRA_MASK_B] = "md_srcclkena_1_infra_mask_b",
	[PWR_CONN_SRCCLKENA_INFRA_MASK_B] = "conn_srcclkena_infra_mask_b",
	[PWR_MD32_SRCCLKENA_INFRA_MASK_B] = "md32_srcclkena_infra_mask_b",
	[PWR_SRCCLKENI_INFRA_MASK_B] = "srcclkeni_infra_mask_b",
	[PWR_MD_APSRC_REQ_0_INFRA_MASK_B] = "md_apsrc_req_0_infra_mask_b",
	[PWR_MD_APSRC_REQ_1_INFRA_MASK_B] = "md_apsrc_req_1_infra_mask_b",
	[PWR_CONN_APSRCREQ_INFRA_MASK_B] = "conn_apsrcreq_infra_mask_b",
	[PWR_MD32_APSRCREQ_INFRA_MASK_B] = "md32_apsrcreq_infra_mask_b",
	[PWR_MD_DDR_EN_0_MASK_B] = "md_ddr_en_0_mask_b",
	[PWR_MD_DDR_EN_1_MASK_B] = "md_ddr_en_1_mask_b",
	[PWR_MD_VRF18_REQ_0_MASK_B] = "md_vrf18_req_0_mask_b",
	[PWR_MD_VRF18_REQ_1_MASK_B] = "md_vrf18_req_1_mask_b",
	[PWR_MD1_DVFS_REQ_MASK] = "md1_dvfs_req_mask",
	[PWR_CPU_DVFS_REQ_MASK] = "cpu_dvfs_req_mask",
	[PWR_EMI_BW_DVFS_REQ_MASK] = "emi_bw_dvfs_req_mask",
	[PWR_MD_SRCCLKENA_0_DVFS_REQ_MASK_B] = "md_srcclkena_0_dvfs_req_mask_b",
	[PWR_MD_SRCCLKENA_1_DVFS_REQ_MASK_B] = "md_srcclkena_1_dvfs_req_mask_b",
	[PWR_CONN_SRCCLKENA_DVFS_REQ_MASK_B] = "conn_srcclkena_dvfs_req_mask_b",
	[PWR_DVFS_HALT_MASK_B] = "dvfs_halt_mask_b",
	[PWR_VDEC_REQ_MASK_B] = "vdec_req_mask_b",
	[PWR_GCE_REQ_MASK_B] = "gce_req_mask_b",
	[PWR_CPU_MD_DVFS_REQ_MERGE_MASK_B] = "cpu_md_dvfs_req_merge_mask_b",
	[PWR_MD_DDR_EN_DVFS_HALT_MASK_B] = "md_ddr_en_dvfs_halt_mask_b",
	[PWR_DSI0_VSYNC_DVFS_HALT_MASK_B] = "dsi0_vsync_dvfs_halt_mask_b",
	[PWR_DSI1_VSYNC_DVFS_HALT_MASK_B] = "dsi1_vsync_dvfs_halt_mask_b",
	[PWR_DPI_VSYNC_DVFS_HALT_MASK_B] = "dpi_vsync_dvfs_halt_mask_b",
	[PWR_ISP0_VSYNC_DVFS_HALT_MASK_B] = "isp0_vsync_dvfs_halt_mask_b",
	[PWR_ISP1_VSYNC_DVFS_HALT_MASK_B] = "isp1_vsync_dvfs_halt_mask_b",
	[PWR_CONN_DDR_EN_MASK_B] = "conn_ddr_en_mask_b",
	[PWR_DISP_REQ_MASK_B] = "disp_req_mask_b",
	[PWR_DISP1_REQ_MASK_B] = "disp1_req_mask_b",
	[PWR_MFG_REQ_MASK_B] = "mfg_req_mask_b",
	[PWR_UFS_SRCCLKENA_MASK_B] = "ufs_srcclkena_mask_b",
	[PWR_UFS_VRF18_REQ_MASK_B] = "ufs_vrf18_req_mask_b",
	[PWR_PS_C2K_RCCIF_WAKE_MASK_B] = "ps_c2k_rccif_wake_mask_b",
	[PWR_L1_C2K_RCCIF_WAKE_MASK_B] = "l1_c2k_rccif_wake_mask_b",
	[PWR_SDIO_ON_DVFS_REQ_MASK_B] = "sdio_on_dvfs_req_mask_b",
	[PWR_EMI_BOOST_DVFS_REQ_MASK_B] = "emi_boost_dvfs_req_mask_b",
	[PWR_CPU_MD_EMI_DVFS_REQ_PROT_DIS] = "cpu_md_emi_dvfs_req_prot_dis",
	[PWR_DRAMC_SPCMD_APSRC_REQ_MASK_B] = "dramc_spcmd_apsrc_req_mask_b",
	[PWR_EMI_BOOST_DVFS_REQ_2_MASK_B] = "emi_boost_dvfs_req_2_mask_b",
	[PWR_EMI_BW_DVFS_REQ_2_MASK] = "emi_bw_dvfs_req_2_mask",
	[PWR_GCE_VRF18_REQ_MASK_B] = "gce_vrf18_req_mask_b",
	[PWR_SPM_WAKEUP_EVENT_MASK] = "spm_wakeup_event_mask",
	[PWR_SPM_WAKEUP_EVENT_EXT_MASK] = "spm_wakeup_event_ext_mask",
	[PWR_MD_DDR_EN_2_0_MASK_B] = "md_ddr_en_2_0_mask_b",
	[PWR_MD_DDR_EN_2_1_MASK_B] = "md_ddr_en_2_1_mask_b",
	[PWR_CONN_DDR_EN_2_MASK_B] = "conn_ddr_en_2_mask_b",
	[PWR_DRAMC_SPCMD_APSRC_REQ_2_MASK_B] = "dramc_spcmd_apsrc_req_2_mask_b",
	[PWR_SPARE1_DDREN_2_MASK_B] = "spare1_ddren_2_mask_b",
	[PWR_SPARE2_DDREN_2_MASK_B] = "spare2_ddren_2_mask_b",
	[PWR_DDREN_EMI_SELF_REFRESH_CH0_MASK_B] = "ddren_emi_self_refresh_ch0_mask_b",
	[PWR_DDREN_EMI_SELF_REFRESH_CH1_MASK_B] = "ddren_emi_self_refresh_ch1_mask_b",
	[PWR_DDREN_MM_STATE_MASK_B] = "ddren_mm_state_mask_b",
	[PWR_DDREN_MD32_APSRC_REQ_MASK_B] = "ddren_md32_apsrc_req_mask_b",
	[PWR_DDREN_DQSSOC_REQ_MASK_B] = "ddren_dqssoc_req_mask_b",
	[PWR_DDREN2_EMI_SELF_REFRESH_CH0_MASK_B] = "ddren2_emi_self_refresh_ch0_mask_b",
	[PWR_DDREN2_EMI_SELF_REFRESH_CH1_MASK_B] = "ddren2_emi_self_refresh_ch1_mask_b",
	[PWR_DDREN2_MM_STATE_MASK_B] = "ddren2_mm_state_mask_b",
	[PWR_DDREN2_MD32_APSRC_REQ_MASK_B] = "ddren2_md32_apsrc_req_mask_b",
	[PWR_DDREN2_DQSSOC_REQ_MASK_B] = "ddren2_dqssoc_req_mask_b",
	[PWR_MP0_CPU0_WFI_EN] = "mp0_cpu0_wfi_en",
	[PWR_MP0_CPU1_WFI_EN] = "mp0_cpu1_wfi_en",
	[PWR_MP0_CPU2_WFI_EN] = "mp0_cpu2_wfi_en",
	[PWR_MP0_CPU3_WFI_EN] = "mp0_cpu3_wfi_en",
	[PWR_MP1_CPU0_WFI_EN] = "mp1_cpu0_wfi_en",
	[PWR_MP1_CPU1_WFI_EN] = "mp1_cpu1_wfi_en",
	[PWR_MP1_CPU2_WFI_EN] = "mp1_cpu2_wfi_en",
	[PWR_MP1_CPU3_WFI_EN] = "mp1_cpu3_wfi_en",
};

/**************************************
 * xxx_ctrl_show Function
 **************************************/
/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
static ssize_t show_pwr_ctrl(const struct pwr_ctrl *pwrctrl, char *buf)
{
	char *p = buf;

	p += sprintf(p, "pcm_flags = 0x%x\n", pwrctrl->pcm_flags);
	p += sprintf(p, "pcm_flags_cust = 0x%x\n", pwrctrl->pcm_flags_cust);
	p += sprintf(p, "pcm_flags_cust_set = 0x%x\n", pwrctrl->pcm_flags_cust_set);
	p += sprintf(p, "pcm_flags_cust_clr = 0x%x\n", pwrctrl->pcm_flags_cust_clr);
	p += sprintf(p, "timer_val = 0x%x\n", pwrctrl->timer_val);
	p += sprintf(p, "timer_val_cust = 0x%x\n", pwrctrl->timer_val_cust);
	p += sprintf(p, "timer_val_ramp_en = 0x%x\n", pwrctrl->timer_val_ramp_en);
	p += sprintf(p, "timer_val_ramp_en_sec = 0x%x\n", pwrctrl->timer_val_ramp_en_sec);
	p += sprintf(p, "wake_src = 0x%x\n", pwrctrl->wake_src);
	p += sprintf(p, "wake_src_cust = 0x%x\n", pwrctrl->wake_src_cust);
	p += sprintf(p, "vcore_volt_pmic_val = 0x%x\n", pwrctrl->vcore_volt_pmic_val);
	p += sprintf(p, "opp_level = 0x%x\n", pwrctrl->opp_level);
	p += sprintf(p, "wdt_disable = 0x%x\n", pwrctrl->wdt_disable);

	/* reduce buf usage (should < PAGE_SIZE) */

	WARN_ON(p - buf >= PAGE_SIZE);

	return p - buf;
}

static ssize_t suspend_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_suspend.pwrctrl, buf);
}

static ssize_t dpidle_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_dpidle.pwrctrl, buf);
}

static ssize_t sodi3_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_sodi3.pwrctrl, buf);
}

static ssize_t sodi_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_sodi.pwrctrl, buf);
}

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static ssize_t vcore_dvfs_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_vcorefs.pwrctrl, buf);
}
#endif


/**************************************
 * xxx_ctrl_store Function
 **************************************/
/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
static ssize_t store_pwr_ctrl(int id, struct pwr_ctrl *pwrctrl, const char *buf, size_t count)
{
	u32 val;
	char cmd[64];

	if (sscanf(buf, "%63s %x", cmd, &val) != 2)
		return -EPERM;

	spm_debug("pwr_ctrl: cmd = %s, val = 0x%x\n", cmd, val);


	if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS])) {
		pwrctrl->pcm_flags = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS_CUST])) {
		pwrctrl->pcm_flags_cust = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS_CUST, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS_CUST_SET])) {
		pwrctrl->pcm_flags_cust_set = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS_CUST_SET, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS_CUST_CLR])) {
		pwrctrl->pcm_flags_cust_clr = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS_CUST_CLR, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_TIMER_VAL])) {
		pwrctrl->timer_val = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_TIMER_VAL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_TIMER_VAL_CUST])) {
		pwrctrl->timer_val_cust = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_TIMER_VAL_CUST, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_TIMER_VAL_RAMP_EN])) {
		pwrctrl->timer_val_ramp_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_TIMER_VAL_RAMP_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_TIMER_VAL_RAMP_EN_SEC])) {
		pwrctrl->timer_val_ramp_en_sec = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_TIMER_VAL_RAMP_EN_SEC, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_WAKE_SRC])) {
		pwrctrl->wake_src = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_WAKE_SRC, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_WAKE_SRC_CUST])) {
		pwrctrl->wake_src_cust = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_WAKE_SRC_CUST, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_VCORE_VOLT_PMIC_VAL])) {
		pwrctrl->vcore_volt_pmic_val = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_VCORE_VOLT_PMIC_VAL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_OPP_LEVEL])) {
		pwrctrl->opp_level = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_OPP_LEVEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_WDT_DISABLE])) {
		pwrctrl->wdt_disable = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_WDT_DISABLE, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_WFI_OP])) {
		pwrctrl->wfi_op = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_WFI_OP, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP0_CPUTOP_IDLE_MASK])) {
		pwrctrl->mp0_cputop_idle_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP0_CPUTOP_IDLE_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP1_CPUTOP_IDLE_MASK])) {
		pwrctrl->mp1_cputop_idle_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP1_CPUTOP_IDLE_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCUSYS_IDLE_MASK])) {
		pwrctrl->mcusys_idle_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCUSYS_IDLE_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MM_MASK_B])) {
		pwrctrl->mm_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MM_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_0_DBC_EN])) {
		pwrctrl->md_ddr_en_0_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_0_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_1_DBC_EN])) {
		pwrctrl->md_ddr_en_1_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_1_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_MASK_B])) {
		pwrctrl->md_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PMCU_MASK_B])) {
		pwrctrl->pmcu_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PMCU_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_LTE_MASK_B])) {
		pwrctrl->lte_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_LTE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SRCCLKENI_MASK_B])) {
		pwrctrl->srcclkeni_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SRCCLKENI_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_APSRC_1_SEL])) {
		pwrctrl->md_apsrc_1_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_APSRC_1_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_APSRC_0_SEL])) {
		pwrctrl->md_apsrc_0_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_APSRC_0_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_DDR_EN_DBC_EN])) {
		pwrctrl->conn_ddr_en_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_DDR_EN_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_MASK_B])) {
		pwrctrl->conn_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_APSRC_SEL])) {
		pwrctrl->conn_apsrc_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_APSRC_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_APSRC_REQ])) {
		pwrctrl->spm_apsrc_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_APSRC_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_F26M_REQ])) {
		pwrctrl->spm_f26m_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_F26M_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_LTE_REQ])) {
		pwrctrl->spm_lte_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_LTE_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_INFRA_REQ])) {
		pwrctrl->spm_infra_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_INFRA_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_VRF18_REQ])) {
		pwrctrl->spm_vrf18_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_VRF18_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_DVFS_REQ])) {
		pwrctrl->spm_dvfs_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_DVFS_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_DVFS_FORCE_DOWN])) {
		pwrctrl->spm_dvfs_force_down = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_DVFS_FORCE_DOWN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_DDREN_REQ])) {
		pwrctrl->spm_ddren_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_DDREN_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_RSV_SRC_REQ])) {
		pwrctrl->spm_rsv_src_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_RSV_SRC_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_DDREN_2_REQ])) {
		pwrctrl->spm_ddren_2_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_DDREN_2_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CPU_MD_DVFS_SOP_FORCE_ON])) {
		pwrctrl->cpu_md_dvfs_sop_force_on = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CPU_MD_DVFS_SOP_FORCE_ON, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CSYSPWREQ_MASK])) {
		pwrctrl->csyspwreq_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CSYSPWREQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIF0_MD_EVENT_MASK_B])) {
		pwrctrl->ccif0_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIF0_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIF0_AP_EVENT_MASK_B])) {
		pwrctrl->ccif0_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIF0_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIF1_MD_EVENT_MASK_B])) {
		pwrctrl->ccif1_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIF1_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIF1_AP_EVENT_MASK_B])) {
		pwrctrl->ccif1_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIF1_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIFMD_MD1_EVENT_MASK_B])) {
		pwrctrl->ccifmd_md1_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIFMD_MD1_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CCIFMD_MD2_EVENT_MASK_B])) {
		pwrctrl->ccifmd_md2_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CCIFMD_MD2_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DSI0_VSYNC_MASK_B])) {
		pwrctrl->dsi0_vsync_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DSI0_VSYNC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DSI1_VSYNC_MASK_B])) {
		pwrctrl->dsi1_vsync_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DSI1_VSYNC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DPI_VSYNC_MASK_B])) {
		pwrctrl->dpi_vsync_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DPI_VSYNC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_ISP0_VSYNC_MASK_B])) {
		pwrctrl->isp0_vsync_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_ISP0_VSYNC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_ISP1_VSYNC_MASK_B])) {
		pwrctrl->isp1_vsync_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_ISP1_VSYNC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_SRCCLKENA_0_INFRA_MASK_B])) {
		pwrctrl->md_srcclkena_0_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_SRCCLKENA_0_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_SRCCLKENA_1_INFRA_MASK_B])) {
		pwrctrl->md_srcclkena_1_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_SRCCLKENA_1_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_SRCCLKENA_INFRA_MASK_B])) {
		pwrctrl->conn_srcclkena_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_SRCCLKENA_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD32_SRCCLKENA_INFRA_MASK_B])) {
		pwrctrl->md32_srcclkena_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD32_SRCCLKENA_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SRCCLKENI_INFRA_MASK_B])) {
		pwrctrl->srcclkeni_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SRCCLKENI_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_APSRC_REQ_0_INFRA_MASK_B])) {
		pwrctrl->md_apsrc_req_0_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_APSRC_REQ_0_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_APSRC_REQ_1_INFRA_MASK_B])) {
		pwrctrl->md_apsrc_req_1_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_APSRC_REQ_1_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_APSRCREQ_INFRA_MASK_B])) {
		pwrctrl->conn_apsrcreq_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_APSRCREQ_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD32_APSRCREQ_INFRA_MASK_B])) {
		pwrctrl->md32_apsrcreq_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD32_APSRCREQ_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_0_MASK_B])) {
		pwrctrl->md_ddr_en_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_1_MASK_B])) {
		pwrctrl->md_ddr_en_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_VRF18_REQ_0_MASK_B])) {
		pwrctrl->md_vrf18_req_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_VRF18_REQ_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_VRF18_REQ_1_MASK_B])) {
		pwrctrl->md_vrf18_req_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_VRF18_REQ_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD1_DVFS_REQ_MASK])) {
		pwrctrl->md1_dvfs_req_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD1_DVFS_REQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CPU_DVFS_REQ_MASK])) {
		pwrctrl->cpu_dvfs_req_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CPU_DVFS_REQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_EMI_BW_DVFS_REQ_MASK])) {
		pwrctrl->emi_bw_dvfs_req_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_EMI_BW_DVFS_REQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_SRCCLKENA_0_DVFS_REQ_MASK_B])) {
		pwrctrl->md_srcclkena_0_dvfs_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_SRCCLKENA_0_DVFS_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_SRCCLKENA_1_DVFS_REQ_MASK_B])) {
		pwrctrl->md_srcclkena_1_dvfs_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_SRCCLKENA_1_DVFS_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_SRCCLKENA_DVFS_REQ_MASK_B])) {
		pwrctrl->conn_srcclkena_dvfs_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_SRCCLKENA_DVFS_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DVFS_HALT_MASK_B])) {
		pwrctrl->dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_VDEC_REQ_MASK_B])) {
		pwrctrl->vdec_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_VDEC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_GCE_REQ_MASK_B])) {
		pwrctrl->gce_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_GCE_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CPU_MD_DVFS_REQ_MERGE_MASK_B])) {
		pwrctrl->cpu_md_dvfs_req_merge_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CPU_MD_DVFS_REQ_MERGE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_DVFS_HALT_MASK_B])) {
		pwrctrl->md_ddr_en_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DSI0_VSYNC_DVFS_HALT_MASK_B])) {
		pwrctrl->dsi0_vsync_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DSI0_VSYNC_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DSI1_VSYNC_DVFS_HALT_MASK_B])) {
		pwrctrl->dsi1_vsync_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DSI1_VSYNC_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DPI_VSYNC_DVFS_HALT_MASK_B])) {
		pwrctrl->dpi_vsync_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DPI_VSYNC_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_ISP0_VSYNC_DVFS_HALT_MASK_B])) {
		pwrctrl->isp0_vsync_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_ISP0_VSYNC_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_ISP1_VSYNC_DVFS_HALT_MASK_B])) {
		pwrctrl->isp1_vsync_dvfs_halt_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_ISP1_VSYNC_DVFS_HALT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_DDR_EN_MASK_B])) {
		pwrctrl->conn_ddr_en_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_DDR_EN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DISP_REQ_MASK_B])) {
		pwrctrl->disp_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DISP_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DISP1_REQ_MASK_B])) {
		pwrctrl->disp1_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DISP1_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MFG_REQ_MASK_B])) {
		pwrctrl->mfg_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MFG_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_UFS_SRCCLKENA_MASK_B])) {
		pwrctrl->ufs_srcclkena_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_UFS_SRCCLKENA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_UFS_VRF18_REQ_MASK_B])) {
		pwrctrl->ufs_vrf18_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_UFS_VRF18_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PS_C2K_RCCIF_WAKE_MASK_B])) {
		pwrctrl->ps_c2k_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PS_C2K_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_L1_C2K_RCCIF_WAKE_MASK_B])) {
		pwrctrl->l1_c2k_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_L1_C2K_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SDIO_ON_DVFS_REQ_MASK_B])) {
		pwrctrl->sdio_on_dvfs_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SDIO_ON_DVFS_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_EMI_BOOST_DVFS_REQ_MASK_B])) {
		pwrctrl->emi_boost_dvfs_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_EMI_BOOST_DVFS_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CPU_MD_EMI_DVFS_REQ_PROT_DIS])) {
		pwrctrl->cpu_md_emi_dvfs_req_prot_dis = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CPU_MD_EMI_DVFS_REQ_PROT_DIS, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DRAMC_SPCMD_APSRC_REQ_MASK_B])) {
		pwrctrl->dramc_spcmd_apsrc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DRAMC_SPCMD_APSRC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_EMI_BOOST_DVFS_REQ_2_MASK_B])) {
		pwrctrl->emi_boost_dvfs_req_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_EMI_BOOST_DVFS_REQ_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_EMI_BW_DVFS_REQ_2_MASK])) {
		pwrctrl->emi_bw_dvfs_req_2_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_EMI_BW_DVFS_REQ_2_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_GCE_VRF18_REQ_MASK_B])) {
		pwrctrl->gce_vrf18_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_GCE_VRF18_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_WAKEUP_EVENT_MASK])) {
		pwrctrl->spm_wakeup_event_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_WAKEUP_EVENT_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_WAKEUP_EVENT_EXT_MASK])) {
		pwrctrl->spm_wakeup_event_ext_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_WAKEUP_EVENT_EXT_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_2_0_MASK_B])) {
		pwrctrl->md_ddr_en_2_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_2_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MD_DDR_EN_2_1_MASK_B])) {
		pwrctrl->md_ddr_en_2_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MD_DDR_EN_2_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_CONN_DDR_EN_2_MASK_B])) {
		pwrctrl->conn_ddr_en_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_CONN_DDR_EN_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DRAMC_SPCMD_APSRC_REQ_2_MASK_B])) {
		pwrctrl->dramc_spcmd_apsrc_req_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DRAMC_SPCMD_APSRC_REQ_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPARE1_DDREN_2_MASK_B])) {
		pwrctrl->spare1_ddren_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPARE1_DDREN_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPARE2_DDREN_2_MASK_B])) {
		pwrctrl->spare2_ddren_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPARE2_DDREN_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN_EMI_SELF_REFRESH_CH0_MASK_B])) {
		pwrctrl->ddren_emi_self_refresh_ch0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN_EMI_SELF_REFRESH_CH0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN_EMI_SELF_REFRESH_CH1_MASK_B])) {
		pwrctrl->ddren_emi_self_refresh_ch1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN_EMI_SELF_REFRESH_CH1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN_MM_STATE_MASK_B])) {
		pwrctrl->ddren_mm_state_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN_MM_STATE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN_MD32_APSRC_REQ_MASK_B])) {
		pwrctrl->ddren_md32_apsrc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN_MD32_APSRC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN_DQSSOC_REQ_MASK_B])) {
		pwrctrl->ddren_dqssoc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN_DQSSOC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN2_EMI_SELF_REFRESH_CH0_MASK_B])) {
		pwrctrl->ddren2_emi_self_refresh_ch0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN2_EMI_SELF_REFRESH_CH0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN2_EMI_SELF_REFRESH_CH1_MASK_B])) {
		pwrctrl->ddren2_emi_self_refresh_ch1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN2_EMI_SELF_REFRESH_CH1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN2_MM_STATE_MASK_B])) {
		pwrctrl->ddren2_mm_state_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN2_MM_STATE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN2_MD32_APSRC_REQ_MASK_B])) {
		pwrctrl->ddren2_md32_apsrc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN2_MD32_APSRC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_DDREN2_DQSSOC_REQ_MASK_B])) {
		pwrctrl->ddren2_dqssoc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_DDREN2_DQSSOC_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP0_CPU0_WFI_EN])) {
		pwrctrl->mp0_cpu0_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP0_CPU0_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP0_CPU1_WFI_EN])) {
		pwrctrl->mp0_cpu1_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP0_CPU1_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP0_CPU2_WFI_EN])) {
		pwrctrl->mp0_cpu2_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP0_CPU2_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP0_CPU3_WFI_EN])) {
		pwrctrl->mp0_cpu3_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP0_CPU3_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP1_CPU0_WFI_EN])) {
		pwrctrl->mp1_cpu0_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP1_CPU0_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP1_CPU1_WFI_EN])) {
		pwrctrl->mp1_cpu1_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP1_CPU1_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP1_CPU2_WFI_EN])) {
		pwrctrl->mp1_cpu2_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP1_CPU2_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MP1_CPU3_WFI_EN])) {
		pwrctrl->mp1_cpu3_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MP1_CPU3_WFI_EN, val);
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t suspend_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	return store_pwr_ctrl(SPM_PWR_CTRL_SUSPEND, __spm_suspend.pwrctrl, buf, count);
}

static ssize_t dpidle_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	return store_pwr_ctrl(SPM_PWR_CTRL_DPIDLE, __spm_dpidle.pwrctrl, buf, count);
}

static ssize_t sodi3_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	return store_pwr_ctrl(SPM_PWR_CTRL_SODI3, __spm_sodi3.pwrctrl, buf, count);
}

static ssize_t sodi_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	return store_pwr_ctrl(SPM_PWR_CTRL_SODI, __spm_sodi.pwrctrl, buf, count);
}

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static ssize_t vcore_dvfs_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				     const char *buf, size_t count)
{

	/* return store_pwr_ctrl(SPM_PWR_CTRL_VCOREFS, __spm_vcorefs.pwrctrl, buf, count); */
	return 0;
}
#endif

/**************************************
 * fm_suspend Function
 **************************************/
static ssize_t fm_suspend_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *p = buf;

	WARN_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

/**************************************
 * Init Function
 **************************************/
DEFINE_ATTR_RW(suspend_ctrl);
DEFINE_ATTR_RW(dpidle_ctrl);
DEFINE_ATTR_RW(sodi3_ctrl);
DEFINE_ATTR_RW(sodi_ctrl);
#if !defined(CONFIG_FPGA_EARLY_PORTING)
DEFINE_ATTR_RW(vcore_dvfs_ctrl);
#endif
DEFINE_ATTR_RO(fm_suspend);

static struct attribute *spm_attrs[] = {
	/* for spm_lp_scen.pwrctrl */
	__ATTR_OF(suspend_ctrl),
	__ATTR_OF(dpidle_ctrl),
	__ATTR_OF(sodi3_ctrl),
	__ATTR_OF(sodi_ctrl),
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	__ATTR_OF(vcore_dvfs_ctrl),
#endif
	__ATTR_OF(fm_suspend),

	/* must */
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name = "spm",
	.attrs = spm_attrs,
};

int spm_fs_init(void)
{
	int r;

	/* create /sys/power/spm/xxx */
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
		spm_err("FAILED TO CREATE /sys/power/spm (%d)\n", r);

	return r;
}

MODULE_DESCRIPTION("SPM-FS Driver v0.1");
