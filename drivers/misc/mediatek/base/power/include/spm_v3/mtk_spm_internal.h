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

#ifndef __MTK_SPM_INTERNAL_H__
#define __MTK_SPM_INTERNAL_H__

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <mt-plat/aee.h>

#include <mtk_spm.h>
#include <mtk_lpae.h>
#include <mtk_gpio.h>

#if defined(CONFIG_MACH_MT6759)
#define SUP_MCSODI_FS
#endif

/**************************************
 * Config and Parameter
 **************************************/
#define POWER_ON_VAL1_DEF	0x00015820
#define PCM_FSM_STA_DEF		0x00048490
#define SPM_WAKEUP_EVENT_MASK_DEF	0xF0F92218

#define PCM_WDT_TIMEOUT		(30 * 32768)	/* 30s */
#define PCM_TIMER_MAX		(0xffffffff - PCM_WDT_TIMEOUT)

/**************************************
 * Define and Declare
 **************************************/
#define PCM_PWRIO_EN_R0		(1U << 0)
#define PCM_PWRIO_EN_R7		(1U << 7)
#define PCM_RF_SYNC_R0		(1U << 16)
#define PCM_RF_SYNC_R6		(1U << 22)
#define PCM_RF_SYNC_R7		(1U << 23)

#define PCM_SW_INT0		(1U << 0)
#define PCM_SW_INT1		(1U << 1)
#define PCM_SW_INT2		(1U << 2)
#define PCM_SW_INT3		(1U << 3)
#define PCM_SW_INT4		(1U << 4)
#define PCM_SW_INT5		(1U << 5)
#define PCM_SW_INT6		(1U << 6)
#define PCM_SW_INT7		(1U << 7)
#define PCM_SW_INT8		(1U << 8)
#define PCM_SW_INT9		(1U << 9)
#define PCM_SW_INT_ALL		(PCM_SW_INT9 | PCM_SW_INT8 | PCM_SW_INT7 | \
				 PCM_SW_INT6 | PCM_SW_INT5 | PCM_SW_INT4 | \
				 PCM_SW_INT3 | PCM_SW_INT2 | PCM_SW_INT1 | \
				 PCM_SW_INT0)

#define WFI_OP_AND		1
#define WFI_OP_OR		0

#define ISRM_TWAM		(1U << 2)
#define ISRM_PCM_RETURN		(1U << 3)
#define ISRM_RET_IRQ0		(1U << 8)
#define ISRM_RET_IRQ1		(1U << 9)
#define ISRM_RET_IRQ2		(1U << 10)
#define ISRM_RET_IRQ3		(1U << 11)
#define ISRM_RET_IRQ4		(1U << 12)
#define ISRM_RET_IRQ5		(1U << 13)
#define ISRM_RET_IRQ6		(1U << 14)
#define ISRM_RET_IRQ7		(1U << 15)
#define ISRM_RET_IRQ8		(1U << 16)
#define ISRM_RET_IRQ9		(1U << 17)
#define ISRM_RET_IRQ10		(1U << 18)
#define ISRM_RET_IRQ11		(1U << 19)
#define ISRM_RET_IRQ12		(1U << 20)
#define ISRM_RET_IRQ13		(1U << 21)
#define ISRM_RET_IRQ14		(1U << 22)
#define ISRM_RET_IRQ15		(1U << 23)

#define ISRM_RET_IRQ_AUX	(ISRM_RET_IRQ15 | ISRM_RET_IRQ14 | \
				 ISRM_RET_IRQ13 | ISRM_RET_IRQ12 | \
				 ISRM_RET_IRQ11 | ISRM_RET_IRQ10 | \
				 ISRM_RET_IRQ9 | ISRM_RET_IRQ8 | \
				 ISRM_RET_IRQ7 | ISRM_RET_IRQ6 | \
				 ISRM_RET_IRQ5 | ISRM_RET_IRQ4 | \
				 ISRM_RET_IRQ3 | ISRM_RET_IRQ2 | \
				 ISRM_RET_IRQ1)
#define ISRM_ALL_EXC_TWAM	(ISRM_RET_IRQ_AUX /*| ISRM_RET_IRQ0 | ISRM_PCM_RETURN*/)
#define ISRM_ALL		(ISRM_ALL_EXC_TWAM | ISRM_TWAM)

#define ISRS_TWAM		(1U << 2)
#define ISRS_PCM_RETURN		(1U << 3)
#define ISRS_SW_INT0		(1U << 4)

#define ISRC_TWAM		ISRS_TWAM
#define ISRC_ALL_EXC_TWAM	ISRS_PCM_RETURN
#define ISRC_ALL		(ISRC_ALL_EXC_TWAM | ISRC_TWAM)

#define WAKE_MISC_TWAM		(1U << 18)
#define WAKE_MISC_PCM_TIMER	(1U << 19)
#define WAKE_MISC_CPU_WAKE	(1U << 20)

struct pcm_desc {
	const char *version;	/* PCM code version */
	const u32 *base;	/* binary array base */
	dma_addr_t base_dma;	/* dma addr of base */
	const u16 size;		/* binary array size */
	const u8 sess;		/* session number */
	const u8 replace;	/* replace mode */
	const u16 addr_2nd;	/* 2nd binary array size */
	const u16 reserved;	/* for 32bit alignment */

	u32 vec0;		/* event vector 0 config */
	u32 vec1;		/* event vector 1 config */
	u32 vec2;		/* event vector 2 config */
	u32 vec3;		/* event vector 3 config */
	u32 vec4;		/* event vector 4 config */
	u32 vec5;		/* event vector 5 config */
	u32 vec6;		/* event vector 6 config */
	u32 vec7;		/* event vector 7 config */
	u32 vec8;		/* event vector 8 config */
	u32 vec9;		/* event vector 9 config */
	u32 vec10;		/* event vector 10 config */
	u32 vec11;		/* event vector 11 config */
	u32 vec12;		/* event vector 12 config */
	u32 vec13;		/* event vector 13 config */
	u32 vec14;		/* event vector 14 config */
	u32 vec15;		/* event vector 15 config */
};

struct pwr_ctrl {
	/* for SPM */
	u32 pcm_flags;
	u32 pcm_flags_cust;	/* can override pcm_flags */
	u32 pcm_flags_cust_set;	/* set bit of pcm_flags, after pcm_flags_cust */
	u32 pcm_flags_cust_clr;	/* clr bit of pcm_flags, after pcm_flags_cust */
	u32 pcm_flags1;
	u32 pcm_flags1_cust;	/* can override pcm_flags1 */
	u32 pcm_flags1_cust_set;	/* set bit of pcm_flags1, after pcm_flags1_cust */
	u32 pcm_flags1_cust_clr;	/* clr bit of pcm_flags1, after pcm_flags1_cust */
	u32 timer_val;		/* @ 1T 32K */
	u32 timer_val_cust;	/* @ 1T 32K, can override timer_val */
	u32 timer_val_ramp_en;		/* stress for dpidle */
	u32 timer_val_ramp_en_sec;	/* stress for suspend */
	u32 wake_src;
	u32 wake_src_cust;	/* can override wake_src */
	u32 vcore_volt_pmic_val;
	u8 opp_level;
	u8 wdt_disable;		/* disable wdt in suspend */
	u8 syspwreq_mask;	/* make 26M off when attach ICE */

	/* Auto-gen Start */

	/* SPM_CLK_CON */
	u8 reg_srcclken0_ctl;
	u8 reg_srcclken1_ctl;
	u8 reg_spm_lock_infra_dcm;
	u8 reg_srcclken_mask;
	u8 reg_md1_c32rm_en;
	u8 reg_md2_c32rm_en;
	u8 reg_clksq0_sel_ctrl;
	u8 reg_clksq1_sel_ctrl;
	u8 reg_srcclken0_en;
	u8 reg_srcclken1_en;
	u8 reg_sysclk0_src_mask_b;
	u8 reg_sysclk1_src_mask_b;

	/* SPM_SRC_REQ */
	u8 reg_spm_apsrc_req;
	u8 reg_spm_f26m_req;
	u8 reg_spm_infra_req;
	u8 reg_spm_ddren_req;
	u8 reg_spm_vrf18_req;
	u8 reg_spm_dvfs_level0_req;
	u8 reg_spm_dvfs_level1_req;
	u8 reg_spm_dvfs_level2_req;
	u8 reg_spm_dvfs_level3_req;
	u8 reg_spm_dvfs_level4_req;
	u8 reg_spm_sspm_mailbox_req;
	u8 reg_spm_sw_mailbox_req;
	u8 reg_spm_cksel2_req;
	u8 reg_spm_cksel3_req;

	/* SPM_SRC_MASK */
	u8 reg_csyspwreq_mask;
	u8 reg_md_srcclkena_0_infra_mask_b;
	u8 reg_md_srcclkena_1_infra_mask_b;
	u8 reg_md_apsrc_req_0_infra_mask_b;
	u8 reg_md_apsrc_req_1_infra_mask_b;
	u8 reg_conn_srcclkena_infra_mask_b;
	u8 reg_conn_infra_req_mask_b;
	u8 reg_sspm_srcclkena_infra_mask_b;
	u8 reg_sspm_infra_req_mask_b;
	u8 reg_scp_srcclkena_infra_mask_b;
	u8 reg_scp_infra_req_mask_b;
	u8 reg_srcclkeni0_infra_mask_b;
	u8 reg_srcclkeni1_infra_mask_b;
	u8 reg_srcclkeni2_infra_mask_b;
	u8 reg_ccif0_md_event_mask_b;
	u8 reg_ccif0_ap_event_mask_b;
	u8 reg_ccif1_md_event_mask_b;
	u8 reg_ccif1_ap_event_mask_b;
	u8 reg_ccif2_md_event_mask_b;
	u8 reg_ccif2_ap_event_mask_b;
	u8 reg_ccif3_md_event_mask_b;
	u8 reg_ccif3_ap_event_mask_b;
	u8 reg_ccifmd_md1_event_mask_b;
	u8 reg_ccifmd_md2_event_mask_b;
	u8 reg_c2k_ps_rccif_wake_mask_b;
	u8 reg_c2k_l1_rccif_wake_mask_b;
	u8 reg_ps_c2k_rccif_wake_mask_b;
	u8 reg_l1_c2k_rccif_wake_mask_b;
	u8 reg_disp2_req_mask_b;
	u8 reg_md_ddr_en_0_mask_b;
	u8 reg_md_ddr_en_1_mask_b;
	u8 reg_conn_ddr_en_mask_b;

	/* SPM_SRC2_MASK */
	u8 reg_disp0_req_mask_b;
	u8 reg_disp1_req_mask_b;
	u8 reg_disp_od_req_mask_b;
	u8 reg_mfg_req_mask_b;
	u8 reg_vdec0_req_mask_b;
	u8 reg_gce_req_mask_b;
	u8 reg_gce_vrf18_req_mask_b;
	u8 reg_lpdma_req_mask_b;
	u8 reg_conn_srcclkena_cksel2_mask_b;
	u8 reg_sspm_apsrc_req_ddren_mask_b;
	u8 reg_scp_apsrc_req_ddren_mask_b;
	u8 reg_md_vrf18_req_0_mask_b;
	u8 reg_md_vrf18_req_1_mask_b;
	u8 reg_next_dvfs_level0_mask_b;
	u8 reg_next_dvfs_level1_mask_b;
	u8 reg_next_dvfs_level2_mask_b;
	u8 reg_next_dvfs_level3_mask_b;
	u8 reg_next_dvfs_level4_mask_b;
	u8 reg_sw2spm_int0_mask_b;
	u8 reg_sw2spm_int1_mask_b;
	u8 reg_sw2spm_int2_mask_b;
	u8 reg_sw2spm_int3_mask_b;
	u8 reg_sspm2spm_int0_mask_b;
	u8 reg_sspm2spm_int1_mask_b;
	u8 reg_sspm2spm_int2_mask_b;
	u8 reg_sspm2spm_int3_mask_b;
	u8 reg_dqssoc_req_mask_b;
#ifdef CONFIG_MACH_MT6759
	u8 reg_gce_vrf18_req2_mask_b;
#endif
	/* SPM_SRC3_MASK */
	u8 reg_mpwfi_op;
	u8 reg_spm_resource_req_rsv1_4_mask_b;
	u8 reg_spm_resource_req_rsv1_3_mask_b;
	u8 reg_spm_resource_req_rsv1_2_mask_b;
	u8 reg_spm_resource_req_rsv1_1_mask_b;
	u8 reg_spm_resource_req_rsv1_0_mask_b;
	u8 reg_spm_resource_req_rsv0_4_mask_b;
	u8 reg_spm_resource_req_rsv0_3_mask_b;
	u8 reg_spm_resource_req_rsv0_2_mask_b;
	u8 reg_spm_resource_req_rsv0_1_mask_b;
	u8 reg_spm_resource_req_rsv0_0_mask_b;
	u8 reg_srcclkeni2_cksel3_mask_b;
	u8 reg_srcclkeni2_cksel2_mask_b;
	u8 reg_srcclkeni1_cksel3_mask_b;
	u8 reg_srcclkeni1_cksel2_mask_b;
	u8 reg_srcclkeni0_cksel3_mask_b;
	u8 reg_srcclkeni0_cksel2_mask_b;
	u8 reg_md_ddr_en_0_dbc_en;
	u8 reg_md_ddr_en_1_dbc_en;
	u8 reg_conn_ddr_en_dbc_en;
	u8 reg_sspm_mask_b;
	u8 reg_md_0_mask_b;
	u8 reg_md_1_mask_b;
	u8 reg_scp_mask_b;
	u8 reg_srcclkeni0_mask_b;
	u8 reg_srcclkeni1_mask_b;
	u8 reg_srcclkeni2_mask_b;
	u8 reg_md_apsrc_1_sel;
	u8 reg_md_apsrc_0_sel;
	u8 reg_conn_mask_b;
	u8 reg_conn_apsrc_sel;
	u8 reg_md_srcclkena_0_vrf18_mask_b;

#ifdef CONFIG_MACH_MT6759
	/* SPM_SRC4_MASK */
	u8 reg_ccif4_md_event_mask_b;
	u8 reg_ccif4_ap_event_mask_b;
	u8 reg_ccif5_md_event_mask_b;
	u8 reg_ccif5_ap_event_mask_b;
#endif

	/* SPM_WAKEUP_EVENT_MASK */
	u32 reg_wakeup_event_mask;

	/* SPM_EXT_WAKEUP_EVENT_MASK */
	u32 reg_ext_wakeup_event_mask;

	/* MCU0_WFI_EN */
	u8 mcu0_wfi_en;

	/* MCU1_WFI_EN */
	u8 mcu1_wfi_en;

	/* MCU2_WFI_EN */
	u8 mcu2_wfi_en;

	/* MCU3_WFI_EN */
	u8 mcu3_wfi_en;

	/* MCU4_WFI_EN */
	u8 mcu4_wfi_en;

	/* MCU5_WFI_EN */
	u8 mcu5_wfi_en;

	/* MCU6_WFI_EN */
	u8 mcu6_wfi_en;

	/* MCU7_WFI_EN */
	u8 mcu7_wfi_en;

	/* MCU8_WFI_EN */
	u8 mcu8_wfi_en;

	/* MCU9_WFI_EN */
	u8 mcu9_wfi_en;

	/* MCU10_WFI_EN */
	u8 mcu10_wfi_en;

	/* MCU11_WFI_EN */
	u8 mcu11_wfi_en;

	/* MCU12_WFI_EN */
	u8 mcu12_wfi_en;

	/* MCU13_WFI_EN */
	u8 mcu13_wfi_en;

	/* MCU14_WFI_EN */
	u8 mcu14_wfi_en;

	/* MCU15_WFI_EN */
	u8 mcu15_wfi_en;

	/* MCU16_WFI_EN */
	u8 mcu16_wfi_en;

	/* MCU17_WFI_EN */
	u8 mcu17_wfi_en;

#ifdef CONFIG_MACH_MT6759
	u8 spm_rsv_con2;
#endif
	/* Auto-gen End */
};

/* code gen by spm_pwr_ctrl_atf.pl, need struct pwr_ctrl */
enum pwr_ctrl_enum {
	PWR_PCM_FLAGS,
	PWR_PCM_FLAGS_CUST,
	PWR_PCM_FLAGS_CUST_SET,
	PWR_PCM_FLAGS_CUST_CLR,
	PWR_PCM_FLAGS1,
	PWR_PCM_FLAGS1_CUST,
	PWR_PCM_FLAGS1_CUST_SET,
	PWR_PCM_FLAGS1_CUST_CLR,
	PWR_TIMER_VAL,
	PWR_TIMER_VAL_CUST,
	PWR_TIMER_VAL_RAMP_EN,
	PWR_TIMER_VAL_RAMP_EN_SEC,
	PWR_WAKE_SRC,
	PWR_WAKE_SRC_CUST,
	PWR_OPP_LEVEL,
	PWR_WDT_DISABLE,
	PWR_SYSPWREQ_MASK,
	PWR_REG_SRCCLKEN0_CTL,
	PWR_REG_SRCCLKEN1_CTL,
	PWR_REG_SPM_LOCK_INFRA_DCM,
	PWR_REG_SRCCLKEN_MASK,
	PWR_REG_MD1_C32RM_EN,
	PWR_REG_MD2_C32RM_EN,
	PWR_REG_CLKSQ0_SEL_CTRL,
	PWR_REG_CLKSQ1_SEL_CTRL,
	PWR_REG_SRCCLKEN0_EN,
	PWR_REG_SRCCLKEN1_EN,
	PWR_REG_SYSCLK0_SRC_MASK_B,
	PWR_REG_SYSCLK1_SRC_MASK_B,
	PWR_REG_SPM_APSRC_REQ,
	PWR_REG_SPM_F26M_REQ,
	PWR_REG_SPM_INFRA_REQ,
	PWR_REG_SPM_DDREN_REQ,
	PWR_REG_SPM_VRF18_REQ,
	PWR_REG_SPM_DVFS_LEVEL0_REQ,
	PWR_REG_SPM_DVFS_LEVEL1_REQ,
	PWR_REG_SPM_DVFS_LEVEL2_REQ,
	PWR_REG_SPM_DVFS_LEVEL3_REQ,
	PWR_REG_SPM_DVFS_LEVEL4_REQ,
	PWR_REG_SPM_SSPM_MAILBOX_REQ,
	PWR_REG_SPM_SW_MAILBOX_REQ,
	PWR_REG_SPM_CKSEL2_REQ,
	PWR_REG_SPM_CKSEL3_REQ,
	PWR_REG_CSYSPWREQ_MASK,
	PWR_REG_MD_SRCCLKENA_0_INFRA_MASK_B,
	PWR_REG_MD_SRCCLKENA_1_INFRA_MASK_B,
	PWR_REG_MD_APSRC_REQ_0_INFRA_MASK_B,
	PWR_REG_MD_APSRC_REQ_1_INFRA_MASK_B,
	PWR_REG_CONN_SRCCLKENA_INFRA_MASK_B,
	PWR_REG_CONN_INFRA_REQ_MASK_B,
	PWR_REG_SSPM_SRCCLKENA_INFRA_MASK_B,
	PWR_REG_SSPM_INFRA_REQ_MASK_B,
	PWR_REG_SCP_SRCCLKENA_INFRA_MASK_B,
	PWR_REG_SCP_INFRA_REQ_MASK_B,
	PWR_REG_SRCCLKENI0_INFRA_MASK_B,
	PWR_REG_SRCCLKENI1_INFRA_MASK_B,
	PWR_REG_SRCCLKENI2_INFRA_MASK_B,
	PWR_REG_CCIF0_MD_EVENT_MASK_B,
	PWR_REG_CCIF0_AP_EVENT_MASK_B,
	PWR_REG_CCIF1_MD_EVENT_MASK_B,
	PWR_REG_CCIF1_AP_EVENT_MASK_B,
	PWR_REG_CCIF2_MD_EVENT_MASK_B,
	PWR_REG_CCIF2_AP_EVENT_MASK_B,
	PWR_REG_CCIF3_MD_EVENT_MASK_B,
	PWR_REG_CCIF3_AP_EVENT_MASK_B,
	PWR_REG_CCIFMD_MD1_EVENT_MASK_B,
	PWR_REG_CCIFMD_MD2_EVENT_MASK_B,
	PWR_REG_C2K_PS_RCCIF_WAKE_MASK_B,
	PWR_REG_C2K_L1_RCCIF_WAKE_MASK_B,
	PWR_REG_PS_C2K_RCCIF_WAKE_MASK_B,
	PWR_REG_L1_C2K_RCCIF_WAKE_MASK_B,
	PWR_REG_DISP2_REQ_MASK_B,
	PWR_REG_MD_DDR_EN_0_MASK_B,
	PWR_REG_MD_DDR_EN_1_MASK_B,
	PWR_REG_CONN_DDR_EN_MASK_B,
	PWR_REG_DISP0_REQ_MASK_B,
	PWR_REG_DISP1_REQ_MASK_B,
	PWR_REG_DISP_OD_REQ_MASK_B,
	PWR_REG_MFG_REQ_MASK_B,
	PWR_REG_VDEC0_REQ_MASK_B,
	PWR_REG_GCE_REQ_MASK_B,
	PWR_REG_GCE_VRF18_REQ_MASK_B,
	PWR_REG_LPDMA_REQ_MASK_B,
	PWR_REG_CONN_SRCCLKENA_CKSEL2_MASK_B,
	PWR_REG_SSPM_APSRC_REQ_DDREN_MASK_B,
	PWR_REG_SCP_APSRC_REQ_DDREN_MASK_B,
	PWR_REG_MD_VRF18_REQ_0_MASK_B,
	PWR_REG_MD_VRF18_REQ_1_MASK_B,
	PWR_REG_NEXT_DVFS_LEVEL0_MASK_B,
	PWR_REG_NEXT_DVFS_LEVEL1_MASK_B,
	PWR_REG_NEXT_DVFS_LEVEL2_MASK_B,
	PWR_REG_NEXT_DVFS_LEVEL3_MASK_B,
	PWR_REG_NEXT_DVFS_LEVEL4_MASK_B,
	PWR_REG_SW2SPM_INT0_MASK_B,
	PWR_REG_SW2SPM_INT1_MASK_B,
	PWR_REG_SW2SPM_INT2_MASK_B,
	PWR_REG_SW2SPM_INT3_MASK_B,
	PWR_REG_SSPM2SPM_INT0_MASK_B,
	PWR_REG_SSPM2SPM_INT1_MASK_B,
	PWR_REG_SSPM2SPM_INT2_MASK_B,
	PWR_REG_SSPM2SPM_INT3_MASK_B,
	PWR_REG_DQSSOC_REQ_MASK_B,
#ifdef CONFIG_MACH_MT6759
	PWR_REG_GCE_VRF18_REQ2_MASK_B,
#endif
	PWR_REG_MPWFI_OP,
	PWR_REG_SPM_RESOURCE_REQ_RSV1_4_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV1_3_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV1_2_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV1_1_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV1_0_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV0_4_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV0_3_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV0_2_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV0_1_MASK_B,
	PWR_REG_SPM_RESOURCE_REQ_RSV0_0_MASK_B,
	PWR_REG_SRCCLKENI2_CKSEL3_MASK_B,
	PWR_REG_SRCCLKENI2_CKSEL2_MASK_B,
	PWR_REG_SRCCLKENI1_CKSEL3_MASK_B,
	PWR_REG_SRCCLKENI1_CKSEL2_MASK_B,
	PWR_REG_SRCCLKENI0_CKSEL3_MASK_B,
	PWR_REG_SRCCLKENI0_CKSEL2_MASK_B,
	PWR_REG_MD_DDR_EN_0_DBC_EN,
	PWR_REG_MD_DDR_EN_1_DBC_EN,
	PWR_REG_CONN_DDR_EN_DBC_EN,
	PWR_REG_SSPM_MASK_B,
	PWR_REG_MD_0_MASK_B,
	PWR_REG_MD_1_MASK_B,
	PWR_REG_SCP_MASK_B,
	PWR_REG_SRCCLKENI0_MASK_B,
	PWR_REG_SRCCLKENI1_MASK_B,
	PWR_REG_SRCCLKENI2_MASK_B,
	PWR_REG_MD_APSRC_1_SEL,
	PWR_REG_MD_APSRC_0_SEL,
	PWR_REG_CONN_MASK_B,
	PWR_REG_CONN_APSRC_SEL,
	PWR_REG_MD_SRCCLKENA_0_VRF18_MASK_B,
#ifdef CONFIG_MACH_MT6759
	PWR_REG_CCIF4_MD_EVENT_MASK_B,
	PWR_REG_CCIF4_AP_EVENT_MASK_B,
	PWR_REG_CCIF5_MD_EVENT_MASK_B,
	PWR_REG_CCIF5_AP_EVENT_MASK_B,
#endif
	PWR_REG_WAKEUP_EVENT_MASK,
	PWR_REG_EXT_WAKEUP_EVENT_MASK,
	PWR_MCU0_WFI_EN,
	PWR_MCU1_WFI_EN,
	PWR_MCU2_WFI_EN,
	PWR_MCU3_WFI_EN,
	PWR_MCU4_WFI_EN,
	PWR_MCU5_WFI_EN,
	PWR_MCU6_WFI_EN,
	PWR_MCU7_WFI_EN,
	PWR_MCU8_WFI_EN,
	PWR_MCU9_WFI_EN,
	PWR_MCU10_WFI_EN,
	PWR_MCU11_WFI_EN,
	PWR_MCU12_WFI_EN,
	PWR_MCU13_WFI_EN,
	PWR_MCU14_WFI_EN,
	PWR_MCU15_WFI_EN,
	PWR_MCU16_WFI_EN,
	PWR_MCU17_WFI_EN,
#ifdef CONFIG_MACH_MT6759
	PWR_SPM_RSV_CON2,
#endif
	PWR_MAX_COUNT,
};

enum {
	SPM_SUSPEND,
	SPM_RESUME,
	SPM_DPIDLE_ENTER,
	SPM_DPIDLE_LEAVE,
	SPM_ENTER_SODI,
	SPM_LEAVE_SODI,
	SPM_ENTER_SODI3,
	SPM_LEAVE_SODI3,
#ifdef SUP_MCSODI_FS
	SPM_ENTER_MCSODI,
	SPM_LEAVE_MCSODI,
#endif
	SPM_SUSPEND_PREPARE,
	SPM_POST_SUSPEND,
	SPM_DPIDLE_PREPARE,
	SPM_POST_DPIDLE,
	SPM_SODI_PREPARE,
	SPM_POST_SODI,
	SPM_SODI3_PREPARE,
	SPM_POST_SODI3,
#ifdef SUP_MCSODI_FS
	SPM_MCSODI_PREPARE,
	SPM_POST_MCSODI,
#endif
	SPM_VCORE_PWARP_CMD,
	SPM_PWR_CTRL_SUSPEND,
	SPM_PWR_CTRL_DPIDLE,
	SPM_PWR_CTRL_SODI,
	SPM_PWR_CTRL_SODI3,
#ifdef SUP_MCSODI_FS
	SPM_PWR_CTRL_MCSODI,
#endif
	SPM_PWR_CTRL_VCOREFS,
};

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT

enum {
	SPM_OPT_SLEEP_DPIDLE  = (1 << 0),
	SPM_OPT_UNIVPLL_STAT  = (1 << 1),
	SPM_OPT_GPS_STAT      = (1 << 2),
	SPM_OPT_VCORE_LP_MODE = (1 << 3),
	SPM_OPT_XO_UFS_OFF    = (1 << 4),
	NF_SPM_OPT
};

struct spm_data {
	unsigned int cmd;
	union {
		struct {
			unsigned int sys_timestamp_l;
			unsigned int sys_timestamp_h;
			unsigned int sys_src_clk_l;
			unsigned int sys_src_clk_h;
			unsigned int spm_opt;
			unsigned int vcore_volt_pmic_val;
			unsigned int reserved;
		} suspend;
		struct {
			unsigned int root_id;
		} notify;
		struct {
			unsigned int pcm_flags;
		} vcorefs;
	} u;
};

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

struct wake_status {
	u32 assert_pc;		/* PCM_REG_DATA_INI */
	u32 r12;		/* PCM_REG12_DATA */
	u32 r12_ext;		/* PCM_REG12_DATA */
	u32 raw_sta;		/* SLEEP_ISR_RAW_STA */
	u32 raw_ext_sta;	/* SPM_WAKEUP_EXT_STA */
	u32 wake_misc;		/* SLEEP_WAKEUP_MISC */
	u32 timer_out;		/* PCM_TIMER_OUT */
	u32 r13;		/* PCM_REG13_DATA */
	u32 idle_sta;		/* SLEEP_SUBSYS_IDLE_STA */
	u32 debug_flag;		/* PCM_PASR_DPD_3 */
	u32 event_reg;		/* PCM_EVENT_REG_STA */
	u32 isr;		/* SLEEP_ISR_STATUS */
	u32 log_index;
	u32 dcs_ch;
};

struct spm_lp_scen {
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl;
	struct wake_status *wakestatus;
};

extern spinlock_t __spm_lock;

extern struct spm_lp_scen __spm_suspend;
extern struct spm_lp_scen __spm_dpidle;
extern struct spm_lp_scen __spm_sodi3;
extern struct spm_lp_scen __spm_sodi;
extern struct spm_lp_scen __spm_mcdi;
#ifdef SUP_MCSODI_FS
extern struct spm_lp_scen __spm_mcsodi;
#endif
extern struct spm_lp_scen __spm_vcorefs;

extern int __spm_check_opp_level(int ch);
extern unsigned int __spm_get_vcore_volt_pmic_val(bool is_vcore_volt_lower, int ch);
extern void __spm_update_pcm_flags_dcs_workaround(struct pwr_ctrl *pwrctrl, int ch);

extern int __spm_get_pcm_timer_val(const struct pwr_ctrl *pwrctrl);
extern void __spm_sync_pcm_flags(struct pwr_ctrl *pwrctrl);

extern void __spm_get_wakeup_status(struct wake_status *wakesta);
extern wake_reason_t __spm_output_wake_reason(const struct wake_status *wakesta,
		const struct pcm_desc *pcmdesc, bool suspend, const char *scenario);

extern void __spm_sync_vcore_dvfs_power_control(struct pwr_ctrl *dest_pwr_ctrl, const struct pwr_ctrl *src_pwr_ctrl);
extern void rekick_vcorefs_scenario(void);

/* set dram dummy read address */
void spm_set_dummy_read_addr(int debug);

extern int spm_fs_init(void);

extern int spm_golden_setting_cmp(bool en);
extern void __spm_set_pcm_wdt(int en);
extern u32 _spm_get_wake_period(int pwake_time, wake_reason_t last_wr);

extern int get_channel_lock(bool blocking);
extern void get_channel_unlock(void);

/**************************************
 * Macro and Inline
 **************************************/
#define EVENT_VEC(event, resume, imme, pc)	\
	(((pc) << 16) |				\
	 (!!(imme) << 7) |			\
	 (!!(resume) << 6) |			\
	 ((event) & 0x3f))

#define spm_emerg(fmt, args...)		pr_emerg("[SPM] " fmt, ##args)
#define spm_alert(fmt, args...)		pr_alert("[SPM] " fmt, ##args)
#define spm_crit(fmt, args...)		pr_crit("[SPM] " fmt, ##args)
#define spm_err(fmt, args...)		pr_err("[SPM] " fmt, ##args)
#define spm_warn(fmt, args...)		pr_warn("[SPM] " fmt, ##args)
#define spm_notice(fmt, args...)	pr_notice("[SPM] " fmt, ##args)
#define spm_info(fmt, args...)		pr_info("[SPM] " fmt, ##args)
#define spm_debug(fmt, args...)		pr_info("[SPM] " fmt, ##args)	/* pr_debug show nothing */

/* just use in suspend flow for important log due to console suspend */
#define spm_crit2(fmt, args...)		\
do {					\
	aee_sram_printk(fmt, ##args);	\
	spm_crit(fmt, ##args);		\
} while (0)

#define wfi_with_sync()					\
do {							\
	isb();						\
	mb();						\
	__asm__ __volatile__("wfi" : : : "memory");	\
} while (0)

static inline u32 base_va_to_pa(const u32 *base)
{
	phys_addr_t pa = virt_to_phys(base);

	MAPPING_DRAM_ACCESS_ADDR(pa);	/* for 4GB mode */
	return (u32) pa;
}

static inline void set_pwrctrl_pcm_flags(struct pwr_ctrl *pwrctrl, u32 flags)
{
	if (pwrctrl->pcm_flags_cust == 0)
		pwrctrl->pcm_flags = flags;
	else
		pwrctrl->pcm_flags = pwrctrl->pcm_flags_cust;
}

static inline void set_pwrctrl_pcm_flags1(struct pwr_ctrl *pwrctrl, u32 flags)
{
	if (pwrctrl->pcm_flags1_cust == 0)
		pwrctrl->pcm_flags1 = flags;
	else
		pwrctrl->pcm_flags1 = pwrctrl->pcm_flags1_cust;
}

#endif
