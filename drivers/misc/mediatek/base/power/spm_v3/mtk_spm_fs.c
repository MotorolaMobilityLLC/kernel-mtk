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
	[PWR_PCM_FLAGS1] = "pcm_flags1",
	[PWR_PCM_FLAGS1_CUST] = "pcm_flags1_cust",
	[PWR_PCM_FLAGS1_CUST_SET] = "pcm_flags1_cust_set",
	[PWR_PCM_FLAGS1_CUST_CLR] = "pcm_flags1_cust_clr",
	[PWR_TIMER_VAL] = "timer_val",
	[PWR_TIMER_VAL_CUST] = "timer_val_cust",
	[PWR_TIMER_VAL_RAMP_EN] = "timer_val_ramp_en",
	[PWR_TIMER_VAL_RAMP_EN_SEC] = "timer_val_ramp_en_sec",
	[PWR_WAKE_SRC] = "wake_src",
	[PWR_WAKE_SRC_CUST] = "wake_src_cust",
	[PWR_OPP_LEVEL] = "opp_level",
	[PWR_WDT_DISABLE] = "wdt_disable",
	[PWR_SYSPWREQ_MASK] = "syspwreq_mask",
	[PWR_REG_SRCCLKEN0_CTL] = "reg_srcclken0_ctl",
	[PWR_REG_SRCCLKEN1_CTL] = "reg_srcclken1_ctl",
	[PWR_REG_SPM_LOCK_INFRA_DCM] = "reg_spm_lock_infra_dcm",
	[PWR_REG_SRCCLKEN_MASK] = "reg_srcclken_mask",
	[PWR_REG_MD1_C32RM_EN] = "reg_md1_c32rm_en",
	[PWR_REG_MD2_C32RM_EN] = "reg_md2_c32rm_en",
	[PWR_REG_CLKSQ0_SEL_CTRL] = "reg_clksq0_sel_ctrl",
	[PWR_REG_CLKSQ1_SEL_CTRL] = "reg_clksq1_sel_ctrl",
	[PWR_REG_SRCCLKEN0_EN] = "reg_srcclken0_en",
	[PWR_REG_SRCCLKEN1_EN] = "reg_srcclken1_en",
	[PWR_REG_SYSCLK0_SRC_MASK_B] = "reg_sysclk0_src_mask_b",
	[PWR_REG_SYSCLK1_SRC_MASK_B] = "reg_sysclk1_src_mask_b",
	[PWR_REG_SPM_APSRC_REQ] = "reg_spm_apsrc_req",
	[PWR_REG_SPM_F26M_REQ] = "reg_spm_f26m_req",
	[PWR_REG_SPM_INFRA_REQ] = "reg_spm_infra_req",
	[PWR_REG_SPM_DDREN_REQ] = "reg_spm_ddren_req",
	[PWR_REG_SPM_VRF18_REQ] = "reg_spm_vrf18_req",
	[PWR_REG_SPM_DVFS_LEVEL0_REQ] = "reg_spm_dvfs_level0_req",
	[PWR_REG_SPM_DVFS_LEVEL1_REQ] = "reg_spm_dvfs_level1_req",
	[PWR_REG_SPM_DVFS_LEVEL2_REQ] = "reg_spm_dvfs_level2_req",
	[PWR_REG_SPM_DVFS_LEVEL3_REQ] = "reg_spm_dvfs_level3_req",
	[PWR_REG_SPM_DVFS_LEVEL4_REQ] = "reg_spm_dvfs_level4_req",
	[PWR_REG_SPM_SSPM_MAILBOX_REQ] = "reg_spm_sspm_mailbox_req",
	[PWR_REG_SPM_SW_MAILBOX_REQ] = "reg_spm_sw_mailbox_req",
	[PWR_REG_SPM_CKSEL2_REQ] = "reg_spm_cksel2_req",
	[PWR_REG_SPM_CKSEL3_REQ] = "reg_spm_cksel3_req",
	[PWR_REG_CSYSPWREQ_MASK] = "reg_csyspwreq_mask",
	[PWR_REG_MD_SRCCLKENA_0_INFRA_MASK_B] = "reg_md_srcclkena_0_infra_mask_b",
	[PWR_REG_MD_SRCCLKENA_1_INFRA_MASK_B] = "reg_md_srcclkena_1_infra_mask_b",
	[PWR_REG_MD_APSRC_REQ_0_INFRA_MASK_B] = "reg_md_apsrc_req_0_infra_mask_b",
	[PWR_REG_MD_APSRC_REQ_1_INFRA_MASK_B] = "reg_md_apsrc_req_1_infra_mask_b",
	[PWR_REG_CONN_SRCCLKENA_INFRA_MASK_B] = "reg_conn_srcclkena_infra_mask_b",
	[PWR_REG_CONN_INFRA_REQ_MASK_B] = "reg_conn_infra_req_mask_b",
	[PWR_REG_SSPM_SRCCLKENA_INFRA_MASK_B] = "reg_sspm_srcclkena_infra_mask_b",
	[PWR_REG_SSPM_INFRA_REQ_MASK_B] = "reg_sspm_infra_req_mask_b",
	[PWR_REG_SCP_SRCCLKENA_INFRA_MASK_B] = "reg_scp_srcclkena_infra_mask_b",
	[PWR_REG_SCP_INFRA_REQ_MASK_B] = "reg_scp_infra_req_mask_b",
	[PWR_REG_SRCCLKENI0_INFRA_MASK_B] = "reg_srcclkeni0_infra_mask_b",
	[PWR_REG_SRCCLKENI1_INFRA_MASK_B] = "reg_srcclkeni1_infra_mask_b",
	[PWR_REG_SRCCLKENI2_INFRA_MASK_B] = "reg_srcclkeni2_infra_mask_b",
	[PWR_REG_CCIF0_MD_EVENT_MASK_B] = "reg_ccif0_md_event_mask_b",
	[PWR_REG_CCIF0_AP_EVENT_MASK_B] = "reg_ccif0_ap_event_mask_b",
	[PWR_REG_CCIF1_MD_EVENT_MASK_B] = "reg_ccif1_md_event_mask_b",
	[PWR_REG_CCIF1_AP_EVENT_MASK_B] = "reg_ccif1_ap_event_mask_b",
	[PWR_REG_CCIF2_MD_EVENT_MASK_B] = "reg_ccif2_md_event_mask_b",
	[PWR_REG_CCIF2_AP_EVENT_MASK_B] = "reg_ccif2_ap_event_mask_b",
	[PWR_REG_CCIF3_MD_EVENT_MASK_B] = "reg_ccif3_md_event_mask_b",
	[PWR_REG_CCIF3_AP_EVENT_MASK_B] = "reg_ccif3_ap_event_mask_b",
	[PWR_REG_CCIFMD_MD1_EVENT_MASK_B] = "reg_ccifmd_md1_event_mask_b",
	[PWR_REG_CCIFMD_MD2_EVENT_MASK_B] = "reg_ccifmd_md2_event_mask_b",
	[PWR_REG_C2K_PS_RCCIF_WAKE_MASK_B] = "reg_c2k_ps_rccif_wake_mask_b",
	[PWR_REG_C2K_L1_RCCIF_WAKE_MASK_B] = "reg_c2k_l1_rccif_wake_mask_b",
	[PWR_REG_PS_C2K_RCCIF_WAKE_MASK_B] = "reg_ps_c2k_rccif_wake_mask_b",
	[PWR_REG_L1_C2K_RCCIF_WAKE_MASK_B] = "reg_l1_c2k_rccif_wake_mask_b",
	[PWR_REG_DISP2_REQ_MASK_B] = "reg_disp2_req_mask_b",
	[PWR_REG_MD_DDR_EN_0_MASK_B] = "reg_md_ddr_en_0_mask_b",
	[PWR_REG_MD_DDR_EN_1_MASK_B] = "reg_md_ddr_en_1_mask_b",
	[PWR_REG_CONN_DDR_EN_MASK_B] = "reg_conn_ddr_en_mask_b",
	[PWR_REG_DISP0_REQ_MASK_B] = "reg_disp0_req_mask_b",
	[PWR_REG_DISP1_REQ_MASK_B] = "reg_disp1_req_mask_b",
	[PWR_REG_DISP_OD_REQ_MASK_B] = "reg_disp_od_req_mask_b",
	[PWR_REG_MFG_REQ_MASK_B] = "reg_mfg_req_mask_b",
	[PWR_REG_VDEC0_REQ_MASK_B] = "reg_vdec0_req_mask_b",
	[PWR_REG_GCE_REQ_MASK_B] = "reg_gce_req_mask_b",
	[PWR_REG_GCE_VRF18_REQ_MASK_B] = "reg_gce_vrf18_req_mask_b",
	[PWR_REG_LPDMA_REQ_MASK_B] = "reg_lpdma_req_mask_b",
	[PWR_REG_CONN_SRCCLKENA_CKSEL2_MASK_B] = "reg_conn_srcclkena_cksel2_mask_b",
	[PWR_REG_SSPM_APSRC_REQ_DDREN_MASK_B] = "reg_sspm_apsrc_req_ddren_mask_b",
	[PWR_REG_SCP_APSRC_REQ_DDREN_MASK_B] = "reg_scp_apsrc_req_ddren_mask_b",
	[PWR_REG_MD_VRF18_REQ_0_MASK_B] = "reg_md_vrf18_req_0_mask_b",
	[PWR_REG_MD_VRF18_REQ_1_MASK_B] = "reg_md_vrf18_req_1_mask_b",
	[PWR_REG_NEXT_DVFS_LEVEL0_MASK_B] = "reg_next_dvfs_level0_mask_b",
	[PWR_REG_NEXT_DVFS_LEVEL1_MASK_B] = "reg_next_dvfs_level1_mask_b",
	[PWR_REG_NEXT_DVFS_LEVEL2_MASK_B] = "reg_next_dvfs_level2_mask_b",
	[PWR_REG_NEXT_DVFS_LEVEL3_MASK_B] = "reg_next_dvfs_level3_mask_b",
	[PWR_REG_NEXT_DVFS_LEVEL4_MASK_B] = "reg_next_dvfs_level4_mask_b",
	[PWR_REG_SW2SPM_INT0_MASK_B] = "reg_sw2spm_int0_mask_b",
	[PWR_REG_SW2SPM_INT1_MASK_B] = "reg_sw2spm_int1_mask_b",
	[PWR_REG_SW2SPM_INT2_MASK_B] = "reg_sw2spm_int2_mask_b",
	[PWR_REG_SW2SPM_INT3_MASK_B] = "reg_sw2spm_int3_mask_b",
	[PWR_REG_SSPM2SPM_INT0_MASK_B] = "reg_sspm2spm_int0_mask_b",
	[PWR_REG_SSPM2SPM_INT1_MASK_B] = "reg_sspm2spm_int1_mask_b",
	[PWR_REG_SSPM2SPM_INT2_MASK_B] = "reg_sspm2spm_int2_mask_b",
	[PWR_REG_SSPM2SPM_INT3_MASK_B] = "reg_sspm2spm_int3_mask_b",
	[PWR_REG_DQSSOC_REQ_MASK_B] = "reg_dqssoc_req_mask_b",
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	[PWR_REG_GCE_VRF18_REQ2_MASK_B] = "reg_gce_vrf18_req2_mask_b",
#endif
	[PWR_REG_MPWFI_OP] = "reg_mpwfi_op",
	[PWR_REG_SPM_RESOURCE_REQ_RSV1_4_MASK_B] = "reg_spm_resource_req_rsv1_4_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV1_3_MASK_B] = "reg_spm_resource_req_rsv1_3_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV1_2_MASK_B] = "reg_spm_resource_req_rsv1_2_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV1_1_MASK_B] = "reg_spm_resource_req_rsv1_1_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV1_0_MASK_B] = "reg_spm_resource_req_rsv1_0_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV0_4_MASK_B] = "reg_spm_resource_req_rsv0_4_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV0_3_MASK_B] = "reg_spm_resource_req_rsv0_3_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV0_2_MASK_B] = "reg_spm_resource_req_rsv0_2_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV0_1_MASK_B] = "reg_spm_resource_req_rsv0_1_mask_b",
	[PWR_REG_SPM_RESOURCE_REQ_RSV0_0_MASK_B] = "reg_spm_resource_req_rsv0_0_mask_b",
	[PWR_REG_SRCCLKENI2_CKSEL3_MASK_B] = "reg_srcclkeni2_cksel3_mask_b",
	[PWR_REG_SRCCLKENI2_CKSEL2_MASK_B] = "reg_srcclkeni2_cksel2_mask_b",
	[PWR_REG_SRCCLKENI1_CKSEL3_MASK_B] = "reg_srcclkeni1_cksel3_mask_b",
	[PWR_REG_SRCCLKENI1_CKSEL2_MASK_B] = "reg_srcclkeni1_cksel2_mask_b",
	[PWR_REG_SRCCLKENI0_CKSEL3_MASK_B] = "reg_srcclkeni0_cksel3_mask_b",
	[PWR_REG_SRCCLKENI0_CKSEL2_MASK_B] = "reg_srcclkeni0_cksel2_mask_b",
	[PWR_REG_MD_DDR_EN_0_DBC_EN] = "reg_md_ddr_en_0_dbc_en",
	[PWR_REG_MD_DDR_EN_1_DBC_EN] = "reg_md_ddr_en_1_dbc_en",
	[PWR_REG_CONN_DDR_EN_DBC_EN] = "reg_conn_ddr_en_dbc_en",
	[PWR_REG_SSPM_MASK_B] = "reg_sspm_mask_b",
	[PWR_REG_MD_0_MASK_B] = "reg_md_0_mask_b",
	[PWR_REG_MD_1_MASK_B] = "reg_md_1_mask_b",
	[PWR_REG_SCP_MASK_B] = "reg_scp_mask_b",
	[PWR_REG_SRCCLKENI0_MASK_B] = "reg_srcclkeni0_mask_b",
	[PWR_REG_SRCCLKENI1_MASK_B] = "reg_srcclkeni1_mask_b",
	[PWR_REG_SRCCLKENI2_MASK_B] = "reg_srcclkeni2_mask_b",
	[PWR_REG_MD_APSRC_1_SEL] = "reg_md_apsrc_1_sel",
	[PWR_REG_MD_APSRC_0_SEL] = "reg_md_apsrc_0_sel",
	[PWR_REG_CONN_MASK_B] = "reg_conn_mask_b",
	[PWR_REG_CONN_APSRC_SEL] = "reg_conn_apsrc_sel",
	[PWR_REG_MD_SRCCLKENA_0_VRF18_MASK_B] = "reg_md_srcclkena_0_vrf18_mask_b",
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	[PWR_REG_CCIF4_MD_EVENT_MASK_B] = "reg_ccif4_md_event_mask_b",
	[PWR_REG_CCIF4_AP_EVENT_MASK_B] = "reg_ccif4_ap_event_mask_b",
	[PWR_REG_CCIF5_MD_EVENT_MASK_B] = "reg_ccif5_md_event_mask_b",
	[PWR_REG_CCIF5_AP_EVENT_MASK_B] = "reg_ccif5_ap_event_mask_b",
#endif
	[PWR_REG_WAKEUP_EVENT_MASK] = "reg_wakeup_event_mask",
	[PWR_REG_EXT_WAKEUP_EVENT_MASK] = "reg_ext_wakeup_event_mask",
	[PWR_MCU0_WFI_EN] = "mcu0_wfi_en",
	[PWR_MCU1_WFI_EN] = "mcu1_wfi_en",
	[PWR_MCU2_WFI_EN] = "mcu2_wfi_en",
	[PWR_MCU3_WFI_EN] = "mcu3_wfi_en",
	[PWR_MCU4_WFI_EN] = "mcu4_wfi_en",
	[PWR_MCU5_WFI_EN] = "mcu5_wfi_en",
	[PWR_MCU6_WFI_EN] = "mcu6_wfi_en",
	[PWR_MCU7_WFI_EN] = "mcu7_wfi_en",
	[PWR_MCU8_WFI_EN] = "mcu8_wfi_en",
	[PWR_MCU9_WFI_EN] = "mcu9_wfi_en",
	[PWR_MCU10_WFI_EN] = "mcu10_wfi_en",
	[PWR_MCU11_WFI_EN] = "mcu11_wfi_en",
	[PWR_MCU12_WFI_EN] = "mcu12_wfi_en",
	[PWR_MCU13_WFI_EN] = "mcu13_wfi_en",
	[PWR_MCU14_WFI_EN] = "mcu14_wfi_en",
	[PWR_MCU15_WFI_EN] = "mcu15_wfi_en",
	[PWR_MCU16_WFI_EN] = "mcu16_wfi_en",
	[PWR_MCU17_WFI_EN] = "mcu17_wfi_en",
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	[PWR_SPM_RSV_CON2] = "spm_rsv_con2",
#endif
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
	p += sprintf(p, "pcm_flags1 = 0x%x\n", pwrctrl->pcm_flags1);
	p += sprintf(p, "pcm_flags1_cust = 0x%x\n", pwrctrl->pcm_flags1_cust);
	p += sprintf(p, "pcm_flags1_cust_set = 0x%x\n", pwrctrl->pcm_flags1_cust_set);
	p += sprintf(p, "pcm_flags1_cust_clr = 0x%x\n", pwrctrl->pcm_flags1_cust_clr);
	p += sprintf(p, "timer_val = 0x%x\n", pwrctrl->timer_val);
	p += sprintf(p, "timer_val_cust = 0x%x\n", pwrctrl->timer_val_cust);
	p += sprintf(p, "timer_val_ramp_en = 0x%x\n", pwrctrl->timer_val_ramp_en);
	p += sprintf(p, "timer_val_ramp_en_sec = 0x%x\n", pwrctrl->timer_val_ramp_en_sec);
	p += sprintf(p, "wake_src = 0x%x\n", pwrctrl->wake_src);
	p += sprintf(p, "wake_src_cust = 0x%x\n", pwrctrl->wake_src_cust);
	p += sprintf(p, "opp_level = 0x%x\n", pwrctrl->opp_level);
	p += sprintf(p, "wdt_disable = 0x%x\n", pwrctrl->wdt_disable);
	p += sprintf(p, "syspwreq_mask = 0x%x\n", pwrctrl->syspwreq_mask);

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
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)

#ifdef SUP_MCSODI_FS
static ssize_t mcsodi_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_mcsodi.pwrctrl, buf);
}
#endif

#endif

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
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS1])) {
		pwrctrl->pcm_flags1 = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS1, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS1_CUST])) {
		pwrctrl->pcm_flags1_cust = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS1_CUST, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS1_CUST_SET])) {
		pwrctrl->pcm_flags1_cust_set = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS1_CUST_SET, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_PCM_FLAGS1_CUST_CLR])) {
		pwrctrl->pcm_flags1_cust_clr = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_PCM_FLAGS1_CUST_CLR, val);
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
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_OPP_LEVEL])) {
		pwrctrl->opp_level = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_OPP_LEVEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_WDT_DISABLE])) {
		pwrctrl->wdt_disable = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_WDT_DISABLE, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SYSPWREQ_MASK])) {
		pwrctrl->syspwreq_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SYSPWREQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKEN0_CTL])) {
		pwrctrl->reg_srcclken0_ctl = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKEN0_CTL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKEN1_CTL])) {
		pwrctrl->reg_srcclken1_ctl = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKEN1_CTL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_LOCK_INFRA_DCM])) {
		pwrctrl->reg_spm_lock_infra_dcm = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_LOCK_INFRA_DCM, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKEN_MASK])) {
		pwrctrl->reg_srcclken_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKEN_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD1_C32RM_EN])) {
		pwrctrl->reg_md1_c32rm_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD1_C32RM_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD2_C32RM_EN])) {
		pwrctrl->reg_md2_c32rm_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD2_C32RM_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CLKSQ0_SEL_CTRL])) {
		pwrctrl->reg_clksq0_sel_ctrl = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CLKSQ0_SEL_CTRL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CLKSQ1_SEL_CTRL])) {
		pwrctrl->reg_clksq1_sel_ctrl = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CLKSQ1_SEL_CTRL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKEN0_EN])) {
		pwrctrl->reg_srcclken0_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKEN0_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKEN1_EN])) {
		pwrctrl->reg_srcclken1_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKEN1_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SYSCLK0_SRC_MASK_B])) {
		pwrctrl->reg_sysclk0_src_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SYSCLK0_SRC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SYSCLK1_SRC_MASK_B])) {
		pwrctrl->reg_sysclk1_src_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SYSCLK1_SRC_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_APSRC_REQ])) {
		pwrctrl->reg_spm_apsrc_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_APSRC_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_F26M_REQ])) {
		pwrctrl->reg_spm_f26m_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_F26M_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_INFRA_REQ])) {
		pwrctrl->reg_spm_infra_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_INFRA_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DDREN_REQ])) {
		pwrctrl->reg_spm_ddren_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DDREN_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_VRF18_REQ])) {
		pwrctrl->reg_spm_vrf18_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_VRF18_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DVFS_LEVEL0_REQ])) {
		pwrctrl->reg_spm_dvfs_level0_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DVFS_LEVEL0_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DVFS_LEVEL1_REQ])) {
		pwrctrl->reg_spm_dvfs_level1_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DVFS_LEVEL1_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DVFS_LEVEL2_REQ])) {
		pwrctrl->reg_spm_dvfs_level2_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DVFS_LEVEL2_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DVFS_LEVEL3_REQ])) {
		pwrctrl->reg_spm_dvfs_level3_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DVFS_LEVEL3_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_DVFS_LEVEL4_REQ])) {
		pwrctrl->reg_spm_dvfs_level4_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_DVFS_LEVEL4_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_SSPM_MAILBOX_REQ])) {
		pwrctrl->reg_spm_sspm_mailbox_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_SSPM_MAILBOX_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_SW_MAILBOX_REQ])) {
		pwrctrl->reg_spm_sw_mailbox_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_SW_MAILBOX_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_CKSEL2_REQ])) {
		pwrctrl->reg_spm_cksel2_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_CKSEL2_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_CKSEL3_REQ])) {
		pwrctrl->reg_spm_cksel3_req = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_CKSEL3_REQ, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CSYSPWREQ_MASK])) {
		pwrctrl->reg_csyspwreq_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CSYSPWREQ_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_SRCCLKENA_0_INFRA_MASK_B])) {
		pwrctrl->reg_md_srcclkena_0_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_SRCCLKENA_0_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_SRCCLKENA_1_INFRA_MASK_B])) {
		pwrctrl->reg_md_srcclkena_1_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_SRCCLKENA_1_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_APSRC_REQ_0_INFRA_MASK_B])) {
		pwrctrl->reg_md_apsrc_req_0_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_APSRC_REQ_0_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_APSRC_REQ_1_INFRA_MASK_B])) {
		pwrctrl->reg_md_apsrc_req_1_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_APSRC_REQ_1_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_SRCCLKENA_INFRA_MASK_B])) {
		pwrctrl->reg_conn_srcclkena_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_SRCCLKENA_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_INFRA_REQ_MASK_B])) {
		pwrctrl->reg_conn_infra_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_INFRA_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM_SRCCLKENA_INFRA_MASK_B])) {
		pwrctrl->reg_sspm_srcclkena_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM_SRCCLKENA_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM_INFRA_REQ_MASK_B])) {
		pwrctrl->reg_sspm_infra_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM_INFRA_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SCP_SRCCLKENA_INFRA_MASK_B])) {
		pwrctrl->reg_scp_srcclkena_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SCP_SRCCLKENA_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SCP_INFRA_REQ_MASK_B])) {
		pwrctrl->reg_scp_infra_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SCP_INFRA_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI0_INFRA_MASK_B])) {
		pwrctrl->reg_srcclkeni0_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI0_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI1_INFRA_MASK_B])) {
		pwrctrl->reg_srcclkeni1_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI1_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI2_INFRA_MASK_B])) {
		pwrctrl->reg_srcclkeni2_infra_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI2_INFRA_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF0_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif0_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF0_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF0_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif0_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF0_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF1_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif1_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF1_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF1_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif1_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF1_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF2_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif2_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF2_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF2_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif2_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF2_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF3_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif3_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF3_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF3_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif3_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF3_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIFMD_MD1_EVENT_MASK_B])) {
		pwrctrl->reg_ccifmd_md1_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIFMD_MD1_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIFMD_MD2_EVENT_MASK_B])) {
		pwrctrl->reg_ccifmd_md2_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIFMD_MD2_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_C2K_PS_RCCIF_WAKE_MASK_B])) {
		pwrctrl->reg_c2k_ps_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_C2K_PS_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_C2K_L1_RCCIF_WAKE_MASK_B])) {
		pwrctrl->reg_c2k_l1_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_C2K_L1_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_PS_C2K_RCCIF_WAKE_MASK_B])) {
		pwrctrl->reg_ps_c2k_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_PS_C2K_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_L1_C2K_RCCIF_WAKE_MASK_B])) {
		pwrctrl->reg_l1_c2k_rccif_wake_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_L1_C2K_RCCIF_WAKE_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_DISP2_REQ_MASK_B])) {
		pwrctrl->reg_disp2_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_DISP2_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_DDR_EN_0_MASK_B])) {
		pwrctrl->reg_md_ddr_en_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_DDR_EN_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_DDR_EN_1_MASK_B])) {
		pwrctrl->reg_md_ddr_en_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_DDR_EN_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_DDR_EN_MASK_B])) {
		pwrctrl->reg_conn_ddr_en_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_DDR_EN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_DISP0_REQ_MASK_B])) {
		pwrctrl->reg_disp0_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_DISP0_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_DISP1_REQ_MASK_B])) {
		pwrctrl->reg_disp1_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_DISP1_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_DISP_OD_REQ_MASK_B])) {
		pwrctrl->reg_disp_od_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_DISP_OD_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MFG_REQ_MASK_B])) {
		pwrctrl->reg_mfg_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MFG_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_VDEC0_REQ_MASK_B])) {
		pwrctrl->reg_vdec0_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_VDEC0_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_GCE_REQ_MASK_B])) {
		pwrctrl->reg_gce_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_GCE_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_GCE_VRF18_REQ_MASK_B])) {
		pwrctrl->reg_gce_vrf18_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_GCE_VRF18_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_LPDMA_REQ_MASK_B])) {
		pwrctrl->reg_lpdma_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_LPDMA_REQ_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_SRCCLKENA_CKSEL2_MASK_B])) {
		pwrctrl->reg_conn_srcclkena_cksel2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_SRCCLKENA_CKSEL2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM_APSRC_REQ_DDREN_MASK_B])) {
		pwrctrl->reg_sspm_apsrc_req_ddren_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM_APSRC_REQ_DDREN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SCP_APSRC_REQ_DDREN_MASK_B])) {
		pwrctrl->reg_scp_apsrc_req_ddren_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SCP_APSRC_REQ_DDREN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_VRF18_REQ_0_MASK_B])) {
		pwrctrl->reg_md_vrf18_req_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_VRF18_REQ_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_VRF18_REQ_1_MASK_B])) {
		pwrctrl->reg_md_vrf18_req_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_VRF18_REQ_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_NEXT_DVFS_LEVEL0_MASK_B])) {
		pwrctrl->reg_next_dvfs_level0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_NEXT_DVFS_LEVEL0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_NEXT_DVFS_LEVEL1_MASK_B])) {
		pwrctrl->reg_next_dvfs_level1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_NEXT_DVFS_LEVEL1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_NEXT_DVFS_LEVEL2_MASK_B])) {
		pwrctrl->reg_next_dvfs_level2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_NEXT_DVFS_LEVEL2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_NEXT_DVFS_LEVEL3_MASK_B])) {
		pwrctrl->reg_next_dvfs_level3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_NEXT_DVFS_LEVEL3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_NEXT_DVFS_LEVEL4_MASK_B])) {
		pwrctrl->reg_next_dvfs_level4_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_NEXT_DVFS_LEVEL4_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SW2SPM_INT0_MASK_B])) {
		pwrctrl->reg_sw2spm_int0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SW2SPM_INT0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SW2SPM_INT1_MASK_B])) {
		pwrctrl->reg_sw2spm_int1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SW2SPM_INT1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SW2SPM_INT2_MASK_B])) {
		pwrctrl->reg_sw2spm_int2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SW2SPM_INT2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SW2SPM_INT3_MASK_B])) {
		pwrctrl->reg_sw2spm_int3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SW2SPM_INT3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM2SPM_INT0_MASK_B])) {
		pwrctrl->reg_sspm2spm_int0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM2SPM_INT0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM2SPM_INT1_MASK_B])) {
		pwrctrl->reg_sspm2spm_int1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM2SPM_INT1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM2SPM_INT2_MASK_B])) {
		pwrctrl->reg_sspm2spm_int2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM2SPM_INT2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM2SPM_INT3_MASK_B])) {
		pwrctrl->reg_sspm2spm_int3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM2SPM_INT3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_DQSSOC_REQ_MASK_B])) {
		pwrctrl->reg_dqssoc_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_DQSSOC_REQ_MASK_B, val);
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_GCE_VRF18_REQ2_MASK_B])) {
		pwrctrl->reg_gce_vrf18_req_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_GCE_VRF18_REQ2_MASK_B, val);
#endif
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MPWFI_OP])) {
		pwrctrl->reg_mpwfi_op = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MPWFI_OP, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV1_4_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv1_4_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV1_4_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV1_3_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv1_3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV1_3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV1_2_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv1_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV1_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV1_1_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv1_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV1_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV1_0_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv1_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV1_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV0_4_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv0_4_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV0_4_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV0_3_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv0_3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV0_3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV0_2_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv0_2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV0_2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV0_1_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv0_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV0_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SPM_RESOURCE_REQ_RSV0_0_MASK_B])) {
		pwrctrl->reg_spm_resource_req_rsv0_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SPM_RESOURCE_REQ_RSV0_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI2_CKSEL3_MASK_B])) {
		pwrctrl->reg_srcclkeni2_cksel3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI2_CKSEL3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI2_CKSEL2_MASK_B])) {
		pwrctrl->reg_srcclkeni2_cksel2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI2_CKSEL2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI1_CKSEL3_MASK_B])) {
		pwrctrl->reg_srcclkeni1_cksel3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI1_CKSEL3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI1_CKSEL2_MASK_B])) {
		pwrctrl->reg_srcclkeni1_cksel2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI1_CKSEL2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI0_CKSEL3_MASK_B])) {
		pwrctrl->reg_srcclkeni0_cksel3_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI0_CKSEL3_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI0_CKSEL2_MASK_B])) {
		pwrctrl->reg_srcclkeni0_cksel2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI0_CKSEL2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_DDR_EN_0_DBC_EN])) {
		pwrctrl->reg_md_ddr_en_0_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_DDR_EN_0_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_DDR_EN_1_DBC_EN])) {
		pwrctrl->reg_md_ddr_en_1_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_DDR_EN_1_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_DDR_EN_DBC_EN])) {
		pwrctrl->reg_conn_ddr_en_dbc_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_DDR_EN_DBC_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SSPM_MASK_B])) {
		pwrctrl->reg_sspm_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SSPM_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_0_MASK_B])) {
		pwrctrl->reg_md_0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_1_MASK_B])) {
		pwrctrl->reg_md_1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SCP_MASK_B])) {
		pwrctrl->reg_scp_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SCP_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI0_MASK_B])) {
		pwrctrl->reg_srcclkeni0_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI0_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI1_MASK_B])) {
		pwrctrl->reg_srcclkeni1_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI1_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_SRCCLKENI2_MASK_B])) {
		pwrctrl->reg_srcclkeni2_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_SRCCLKENI2_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_APSRC_1_SEL])) {
		pwrctrl->reg_md_apsrc_1_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_APSRC_1_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_APSRC_0_SEL])) {
		pwrctrl->reg_md_apsrc_0_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_APSRC_0_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_MASK_B])) {
		pwrctrl->reg_conn_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CONN_APSRC_SEL])) {
		pwrctrl->reg_conn_apsrc_sel = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CONN_APSRC_SEL, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_MD_SRCCLKENA_0_VRF18_MASK_B])) {
		pwrctrl->reg_md_srcclkena_0_vrf18_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_MD_SRCCLKENA_0_VRF18_MASK_B, val);
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF4_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif4_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF4_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF4_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif4_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF4_AP_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF5_MD_EVENT_MASK_B])) {
		pwrctrl->reg_ccif5_md_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF5_MD_EVENT_MASK_B, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_CCIF5_AP_EVENT_MASK_B])) {
		pwrctrl->reg_ccif5_ap_event_mask_b = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_CCIF5_AP_EVENT_MASK_B, val);
#endif
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_WAKEUP_EVENT_MASK])) {
		pwrctrl->reg_wakeup_event_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_WAKEUP_EVENT_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_REG_EXT_WAKEUP_EVENT_MASK])) {
		pwrctrl->reg_ext_wakeup_event_mask = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_REG_EXT_WAKEUP_EVENT_MASK, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU0_WFI_EN])) {
		pwrctrl->mcu0_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU0_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU1_WFI_EN])) {
		pwrctrl->mcu1_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU1_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU2_WFI_EN])) {
		pwrctrl->mcu2_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU2_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU3_WFI_EN])) {
		pwrctrl->mcu3_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU3_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU4_WFI_EN])) {
		pwrctrl->mcu4_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU4_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU5_WFI_EN])) {
		pwrctrl->mcu5_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU5_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU6_WFI_EN])) {
		pwrctrl->mcu6_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU6_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU7_WFI_EN])) {
		pwrctrl->mcu7_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU7_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU8_WFI_EN])) {
		pwrctrl->mcu8_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU8_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU9_WFI_EN])) {
		pwrctrl->mcu9_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU9_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU10_WFI_EN])) {
		pwrctrl->mcu10_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU10_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU11_WFI_EN])) {
		pwrctrl->mcu11_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU11_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU12_WFI_EN])) {
		pwrctrl->mcu12_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU12_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU13_WFI_EN])) {
		pwrctrl->mcu13_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU13_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU14_WFI_EN])) {
		pwrctrl->mcu14_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU14_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU15_WFI_EN])) {
		pwrctrl->mcu15_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU15_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU16_WFI_EN])) {
		pwrctrl->mcu16_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU16_WFI_EN, val);
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_MCU17_WFI_EN])) {
		pwrctrl->mcu17_wfi_en = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_MCU17_WFI_EN, val);
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	} else if (!strcmp(cmd, pwr_ctrl_str[PWR_SPM_RSV_CON2])) {
		pwrctrl->spm_rsv_con2 = val;
		mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
				id, PWR_SPM_RSV_CON2, val);
#endif
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
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)

#ifdef SUP_MCSODI_FS
static ssize_t mcsodi_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	return store_pwr_ctrl(SPM_PWR_CTRL_MCSODI, __spm_mcsodi.pwrctrl, buf, count);
}
#endif

#endif

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
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)
#ifdef SUP_MCSODI_FS
DEFINE_ATTR_RW(mcsodi_ctrl);
#endif
#endif
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
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)
#ifdef SUP_MCSODI_FS
	__ATTR_OF(mcsodi_ctrl),
#endif
#endif
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
