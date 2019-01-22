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
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <linux/random.h>
#include <asm/setup.h>
#include <mtk_eem.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_misc.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_vcorefs_governor.h>
#include <mtk_spm_vcore_dvfs.h>
#include <mt-plat/upmu_common.h>
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
#include <scp_dvfs.h>
#endif /* CONFIG_MTK_TINYSYS_SCP_SUPPORT */
#ifdef CONFIG_MTK_SPM_IN_ATF
#include <mt-plat/mtk_secure_api.h>
#endif /* CONFIG_MTK_SPM_IN_ATF */
#ifdef CONFIG_MTK_DCS
#include <mt-plat/mtk_meminfo.h>
#endif

/**************************************
 * Config and Parameter
 **************************************/
#define LOG_BUF_SIZE		256

#define SPM_WAKE_PERIOD         600	/* sec */

/**************************************
 * Define and Declare
 **************************************/
DEFINE_SPINLOCK(__spm_lock);

#define PCM_TIMER_RAMP_BASE_DPIDLE      80          /*  80/32000 =  2.5 ms */
#define PCM_TIMER_RAMP_BASE_SUSPEND_50MS	0xA0
#define PCM_TIMER_RAMP_BASE_SUSPEND_SHORT	0x7D000 /* 16sec */
#define PCM_TIMER_RAMP_BASE_SUSPEND_LONG	0x927C00 /* 5min */
static u32 pcm_timer_ramp_max_sec_loop = 1;

const char *wakesrc_str[32] = {
	[0] = " R12_PCMTIMER",
	[1] = " R12_SSPM_WDT_EVENT_B",
	[2] = " R12_KP_IRQ_B",
	[3] = " R12_APWDT_EVENT_B",
	[4] = " R12_APXGPT1_EVENT_B",
	[5] = " R12_SYS_TIMER_EVENT_B",
	[6] = " R12_EINT_EVENT_B",
	[7] = " R12_C2K_WDT_IRQ_B",
	[8] = " R12_CCIF0_EVENT_B",
	[9] = " R12_LOWBATTERY_IRQ_B",
	[10] = " R12_SSPM_SPM_IRQ_B",
	[11] = " R12_SCP_IPC_MD2SPM_B",
	[12] = " R12_SCP_WDT_EVENT_B",
	[13] = " R12_PCM_WDT_WAKEUP_B",
	[14] = " R12_USBX_CDSC_B",
	[15] = " R12_USBX_POWERDWN_B",
	[16] = " R12_WAKE_UP_EVENT_MCSODI_SRC1",
	[17] = " R12_EINT_EVENT_SECURE_B",
	[18] = " R12_CCIF1_EVENT_B",
	[19] = " R12_UART0_IRQ_B",
	[20] = " R12_AFE_IRQ_MCU_B",
	[21] = " R12_THERMAL_CTRL_EVENT_B",
	[22] = " R12_SCP_CIRQ_IRQ_B",
	[23] = " R12_WAKE_UP_EVENT_MCSODI_SRC2",
	[24] = " R12_CSYSPWRUPREQ_B",
	[25] = " R12_MD1_WDT_B",
	[26] = " R12_MD2AP_PEER_WAKEUP_EVENT",
	[27] = " R12_SEJ_EVENT_B",
	[28] = " R12_SSPM_WAKEUP_SSPM",
	[29] = " R12_CPU_IRQ_B",
	[30] = " R12_SCP_WAKEUP_SCP",
	[31] = " R12_BIT31",
};

/**************************************
 * Function and API
 **************************************/
int __spm_check_opp_level_impl(int ch)
{
	int opp = vcorefs_get_sw_opp();
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
	int scp_opp = scp_get_dvfs_opp();

	if (scp_opp >= 0)
		opp = min(scp_opp, opp);
#endif /* CONFIG_MTK_TINYSYS_SCP_SUPPORT */

	return opp;
}

int __spm_check_opp_level(int ch)
{
	int level[4] = {4, 3, 1, 0};
	int opp = 0;

	if (ch == 2)
		level[1] = 3;
	else
		level[1] = 2;

	opp = __spm_check_opp_level_impl(ch);

	if (opp < 0 || opp >= ARRAY_SIZE(level)) {
		spm_crit2("%s: error opp number %d\n", __func__, opp);
		opp = 0;
	}

	return level[opp];
}

unsigned int __spm_get_vcore_volt_pmic_val(bool is_vcore_volt_lower, int ch)
{
	unsigned int vcore_volt_pmic_val = 0;
	int opp = 0;

	opp = __spm_check_opp_level_impl(ch);

	if (opp < 0 || opp >= 4) {
		spm_crit2("%s: error opp number %d\n", __func__, opp);
		opp = 0;
	}

	vcore_volt_pmic_val = (is_vcore_volt_lower) ? VOLT_TO_PMIC_VAL(56800) : get_vcore_ptp_volt(opp);

	return vcore_volt_pmic_val;
}

int __spm_get_pcm_timer_val(const struct pwr_ctrl *pwrctrl)
{
	u32 val;

	/* set PCM timer (set to max when disable) */
	if (pwrctrl->timer_val_ramp_en != 0) {
		u32 index;

		get_random_bytes(&index, sizeof(index));

		val = PCM_TIMER_RAMP_BASE_DPIDLE + (index & 0x000000FF);
	} else if (pwrctrl->timer_val_ramp_en_sec != 0) {
		u32 index;

		get_random_bytes(&index, sizeof(index));

		pcm_timer_ramp_max_sec_loop++;
		if (pcm_timer_ramp_max_sec_loop >= 50) {
			pcm_timer_ramp_max_sec_loop = 0;
			/* range 5min to 10min */
			val = PCM_TIMER_RAMP_BASE_SUSPEND_LONG +
				index % PCM_TIMER_RAMP_BASE_SUSPEND_LONG;
		} else {
			/* range 50ms to 16sec50ms */
			val = PCM_TIMER_RAMP_BASE_SUSPEND_50MS +
				index % PCM_TIMER_RAMP_BASE_SUSPEND_SHORT;
		}
	} else {
		if (pwrctrl->timer_val_cust == 0)
			val = pwrctrl->timer_val ? : PCM_TIMER_MAX;
		else
			val = pwrctrl->timer_val_cust;
	}

	return val;
}

#if !defined(CONFIG_MTK_SPM_IN_ATF)
void __spm_set_cpu_status(int cpu)
{
	if (cpu >= 0 && cpu < 4) {
		spm_write(COMMON_TOP_PWR_ADDR, 0x10b00208);
		spm_write(COMMON_CPU_PWR_ADDR, 0x10b0020C);
		spm_write(ARMPLL_CLK_CON,
				(spm_read(ARMPLL_CLK_CON) & ~(MUXSEL_SC_ARMPLL2_LSB | MUXSEL_SC_ARMPLL3_LSB)) |
				(MUXSEL_SC_CCIPLL_LSB | MUXSEL_SC_ARMPLL1_LSB));
	} else if (cpu >= 4 && cpu < 8) {
		spm_write(COMMON_TOP_PWR_ADDR, 0x10b0021C);
		spm_write(COMMON_CPU_PWR_ADDR, 0x10b00220);
		spm_write(ARMPLL_CLK_CON,
				(spm_read(ARMPLL_CLK_CON) & ~(MUXSEL_SC_ARMPLL1_LSB | MUXSEL_SC_ARMPLL3_LSB)) |
				(MUXSEL_SC_CCIPLL_LSB | MUXSEL_SC_ARMPLL2_LSB));
	} else {
		spm_crit2("%s: error cpu number %d\n", __func__, cpu);
		/* FIXME: */
		/* BUG(); */
	}
}

static void spm_code_swapping(void)
{
	u32 con1;
	int retry = 0, timeout = 5000;

	con1 = spm_read(SPM_WAKEUP_EVENT_MASK);

	spm_write(SPM_WAKEUP_EVENT_MASK, (con1 & ~(0x1)));
	spm_write(SPM_CPU_WAKEUP_EVENT, 1);

	while ((spm_read(SPM_IRQ_STA) & RG_PCM_IRQ_MSK_LSB) == 0) {
		if (retry > timeout) {
			pr_err("[%s] r15: 0x%x, r6: 0x%x, r1: 0x%x, pcmsta: 0x%x, irqsta: 0x%x [%d]\n",
				__func__,
				spm_read(PCM_REG15_DATA), spm_read(PCM_REG6_DATA), spm_read(PCM_REG1_DATA),
				spm_read(PCM_FSM_STA), spm_read(SPM_IRQ_STA), timeout);
			/* FIXME: */
			/* if (spm_read(PCM_REG15_DATA) != 0) */
				/* BUG(); */
		}
		udelay(1);
		retry++;
	}

	spm_write(SPM_CPU_WAKEUP_EVENT, 0);
	spm_write(SPM_WAKEUP_EVENT_MASK, con1);
}

void __spm_reset_and_init_pcm(const struct pcm_desc *pcmdesc)
{
	u32 con1;
	u32 spm_pwr_on_val0_mask = 0;
	u32 spm_pwr_on_val0_read = 0;
	u32 spm_pwr_on_val0_write = 0;

	/* SPM code swapping */
	if ((spm_read(PCM_REG1_DATA) == 0x1) && !(spm_read(PCM_REG15_DATA) == 0x0))
		spm_code_swapping();

	/* backup mem control from r0 to POWER_ON_VAL0 */
	if (!(spm_read(PCM_REG1_DATA) == 0x1))
		spm_pwr_on_val0_mask = 0x30F80000;
	else
		spm_pwr_on_val0_mask = 0x10F80000;

	spm_pwr_on_val0_read = spm_read(SPM_POWER_ON_VAL0);
	spm_pwr_on_val0_read &= spm_pwr_on_val0_mask;

	spm_pwr_on_val0_write = spm_read(PCM_REG0_DATA);
	spm_pwr_on_val0_write &= ~spm_pwr_on_val0_mask;
	spm_pwr_on_val0_write |= spm_pwr_on_val0_read;

	if (spm_read(SPM_POWER_ON_VAL0) != spm_pwr_on_val0_write) {
		spm_crit("VAL0 from 0x%x to 0x%x\n", spm_read(SPM_POWER_ON_VAL0), spm_pwr_on_val0_write);
		spm_write(SPM_POWER_ON_VAL0, spm_pwr_on_val0_write);
	}

	/* disable r0 and r7 to control power */
	spm_write(PCM_PWR_IO_EN, 0);

	/* disable pcm timer after leaving FW */
	spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | (spm_read(PCM_CON1) & ~RG_PCM_TIMER_EN_LSB));

	/* reset PCM */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | PCM_SW_RESET_LSB);
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB);
	if ((spm_read(PCM_FSM_STA) & 0x7fffff) != PCM_FSM_STA_DEF) {
		spm_crit2("reset pcm(PCM_FSM_STA=0x%x)\n", spm_read(PCM_FSM_STA));
		/* FIXME: */
		/* BUG(); */ /* PCM reset failed */
	}

	/* init PCM_CON0 (disable event vector) */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | RG_EN_IM_SLEEP_DVS_LSB);

	/* init PCM_CON1 (disable PCM timer but keep PCM WDT setting) */
	con1 = spm_read(PCM_CON1) & (RG_PCM_WDT_WAKE_LSB);
	spm_write(PCM_CON1, con1 | SPM_REGWR_CFG_KEY | REG_EVENT_LOCK_EN_LSB |
		  REG_SPM_SRAM_ISOINT_B_LSB |
		  (pcmdesc->replace ? 0 : RG_IM_NONRP_EN_LSB) |
		  RG_AHBMIF_APBEN_LSB | RG_SSPM_APB_INTERNAL_EN_LSB);
}

void __spm_kick_im_to_fetch(const struct pcm_desc *pcmdesc)
{
	u32 ptr, len, con0;

	/* tell IM where is PCM code (use slave mode if code existed) */
	if (pcmdesc->base_dma) {
		ptr = pcmdesc->base_dma;
		/* for 4GB mode */
		if (enable_4G())
			MAPPING_DRAM_ACCESS_ADDR(ptr);
	} else {
		ptr = base_va_to_pa(pcmdesc->base);
	}
	len = pcmdesc->size - 1;
	if (spm_read(PCM_IM_PTR) != ptr || spm_read(PCM_IM_LEN) != len || pcmdesc->sess > 2) {
		spm_write(PCM_IM_PTR, ptr);
		spm_write(PCM_IM_LEN, len);
	} else {
		spm_write(PCM_CON1, spm_read(PCM_CON1) | SPM_REGWR_CFG_KEY | RG_IM_SLAVE_LSB);
	}

	/* kick IM to fetch (only toggle IM_KICK) */
	con0 = spm_read(PCM_CON0) & ~(RG_IM_KICK_L_LSB | RG_PCM_KICK_L_LSB);
	spm_write(PCM_CON0, con0 | SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | RG_IM_KICK_L_LSB);
	spm_write(PCM_CON0, con0 | SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB);
}

void __spm_init_pcm_register(void)
{
	/* init r0 with POWER_ON_VAL0 */
	spm_write(PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL0));
	spm_write(PCM_PWR_IO_EN, PCM_RF_SYNC_R0);
	spm_write(PCM_PWR_IO_EN, 0);

	/* init r7 with POWER_ON_VAL1 */
	spm_write(PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
	spm_write(PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
	spm_write(PCM_PWR_IO_EN, 0);
}

void __spm_init_event_vector(const struct pcm_desc *pcmdesc)
{
	/* init event vector register */
	spm_write(PCM_EVENT_VECTOR0, pcmdesc->vec0);
	spm_write(PCM_EVENT_VECTOR1, pcmdesc->vec1);
	spm_write(PCM_EVENT_VECTOR2, pcmdesc->vec2);
	spm_write(PCM_EVENT_VECTOR3, pcmdesc->vec3);
	spm_write(PCM_EVENT_VECTOR4, pcmdesc->vec4);
	spm_write(PCM_EVENT_VECTOR5, pcmdesc->vec5);
	spm_write(PCM_EVENT_VECTOR6, pcmdesc->vec6);
	spm_write(PCM_EVENT_VECTOR7, pcmdesc->vec7);
	spm_write(PCM_EVENT_VECTOR8, pcmdesc->vec8);
	spm_write(PCM_EVENT_VECTOR9, pcmdesc->vec9);
	spm_write(PCM_EVENT_VECTOR10, pcmdesc->vec10);
	spm_write(PCM_EVENT_VECTOR11, pcmdesc->vec11);
	spm_write(PCM_EVENT_VECTOR12, pcmdesc->vec12);
	spm_write(PCM_EVENT_VECTOR13, pcmdesc->vec13);
	spm_write(PCM_EVENT_VECTOR14, pcmdesc->vec14);
	spm_write(PCM_EVENT_VECTOR15, pcmdesc->vec15);

	/* event vector will be enabled by PCM itself */
}

void __spm_src_req_update(const struct pwr_ctrl *pwrctrl)
{
	unsigned int resource_usage = spm_get_resource_usage();

	u8 spm_apsrc_req = (resource_usage & SPM_RESOURCE_DRAM)    ? 1 : pwrctrl->reg_spm_apsrc_req;
	u8 spm_ddren_req = (resource_usage & SPM_RESOURCE_DRAM)    ? 1 : pwrctrl->reg_spm_ddren_req;
	u8 spm_infra_req = (resource_usage & SPM_RESOURCE_MAINPLL) ? 1 : pwrctrl->reg_spm_infra_req;
	u8 spm_f26m_req  = (resource_usage & SPM_RESOURCE_CK_26M)  ? 1 : pwrctrl->reg_spm_f26m_req;

	/* SPM_SRC_REQ */
	spm_write(SPM_SRC_REQ,
		((spm_apsrc_req & 0x1) << 0) |
		((spm_f26m_req & 0x1) << 1) |
		((spm_infra_req & 0x1) << 2) |
		((spm_ddren_req & 0x1) << 3) |
		((pwrctrl->reg_spm_vrf18_req & 0x1) << 4) |
		((pwrctrl->reg_spm_dvfs_level0_req & 0x1) << 5) |
		((pwrctrl->reg_spm_dvfs_level1_req & 0x1) << 6) |
		((pwrctrl->reg_spm_dvfs_level2_req & 0x1) << 7) |
		((pwrctrl->reg_spm_dvfs_level3_req & 0x1) << 8) |
		((pwrctrl->reg_spm_dvfs_level4_req & 0x1) << 9) |
		((pwrctrl->reg_spm_sspm_mailbox_req & 0x1) << 10) |
		((pwrctrl->reg_spm_sw_mailbox_req & 0x1) << 11) |
		((pwrctrl->reg_spm_cksel2_req & 0x1) << 12) |
		((pwrctrl->reg_spm_cksel3_req & 0x1) << 13));
}

void __spm_set_power_control(const struct pwr_ctrl *pwrctrl)
{
	/* Auto-gen Start */

	/* SPM_CLK_CON */
	spm_write(SPM_CLK_CON,
		((pwrctrl->reg_srcclken0_ctl & 0x3) << 0) |
		((pwrctrl->reg_srcclken1_ctl & 0x3) << 2) |
		((pwrctrl->reg_spm_lock_infra_dcm & 0x1) << 5) |
		((pwrctrl->reg_srcclken_mask & 0x7) << 6) |
		((pwrctrl->reg_md1_c32rm_en & 0x1) << 9) |
		((pwrctrl->reg_md2_c32rm_en & 0x1) << 10) |
		((pwrctrl->reg_clksq0_sel_ctrl & 0x1) << 11) |
		((pwrctrl->reg_clksq1_sel_ctrl & 0x1) << 12) |
		((pwrctrl->reg_srcclken0_en & 0x1) << 13) |
		((pwrctrl->reg_srcclken1_en & 0x1) << 14) |
		((pwrctrl->reg_sysclk0_src_mask_b & 0x7f) << 16) |
		((pwrctrl->reg_sysclk1_src_mask_b & 0x7f) << 23));

	/* SPM_SRC_REQ */
	spm_write(SPM_SRC_REQ,
		((pwrctrl->reg_spm_apsrc_req & 0x1) << 0) |
		((pwrctrl->reg_spm_f26m_req & 0x1) << 1) |
		((pwrctrl->reg_spm_infra_req & 0x1) << 2) |
		((pwrctrl->reg_spm_ddren_req & 0x1) << 3) |
		((pwrctrl->reg_spm_vrf18_req & 0x1) << 4) |
		((pwrctrl->reg_spm_dvfs_level0_req & 0x1) << 5) |
		((pwrctrl->reg_spm_dvfs_level1_req & 0x1) << 6) |
		((pwrctrl->reg_spm_dvfs_level2_req & 0x1) << 7) |
		((pwrctrl->reg_spm_dvfs_level3_req & 0x1) << 8) |
		((pwrctrl->reg_spm_dvfs_level4_req & 0x1) << 9) |
		((pwrctrl->reg_spm_sspm_mailbox_req & 0x1) << 10) |
		((pwrctrl->reg_spm_sw_mailbox_req & 0x1) << 11) |
		((pwrctrl->reg_spm_cksel2_req & 0x1) << 12) |
		((pwrctrl->reg_spm_cksel3_req & 0x1) << 13));

	/* SPM_SRC_MASK */
	spm_write(SPM_SRC_MASK,
		((pwrctrl->reg_csyspwreq_mask & 0x1) << 0) |
		((pwrctrl->reg_md_srcclkena_0_infra_mask_b & 0x1) << 1) |
		((pwrctrl->reg_md_srcclkena_1_infra_mask_b & 0x1) << 2) |
		((pwrctrl->reg_md_apsrc_req_0_infra_mask_b & 0x1) << 3) |
		((pwrctrl->reg_md_apsrc_req_1_infra_mask_b & 0x1) << 4) |
		((pwrctrl->reg_conn_srcclkena_infra_mask_b & 0x1) << 5) |
		((pwrctrl->reg_conn_infra_req_mask_b & 0x1) << 6) |
		((pwrctrl->reg_sspm_srcclkena_infra_mask_b & 0x1) << 7) |
		((pwrctrl->reg_sspm_infra_req_mask_b & 0x1) << 8) |
		((pwrctrl->reg_scp_srcclkena_infra_mask_b & 0x1) << 9) |
		((pwrctrl->reg_scp_infra_req_mask_b & 0x1) << 10) |
		((pwrctrl->reg_srcclkeni0_infra_mask_b & 0x1) << 11) |
		((pwrctrl->reg_srcclkeni1_infra_mask_b & 0x1) << 12) |
		((pwrctrl->reg_srcclkeni2_infra_mask_b & 0x1) << 13) |
		((pwrctrl->reg_ccif0_md_event_mask_b & 0x1) << 14) |
		((pwrctrl->reg_ccif0_ap_event_mask_b & 0x1) << 15) |
		((pwrctrl->reg_ccif1_md_event_mask_b & 0x1) << 16) |
		((pwrctrl->reg_ccif1_ap_event_mask_b & 0x1) << 17) |
		((pwrctrl->reg_ccif2_md_event_mask_b & 0x1) << 18) |
		((pwrctrl->reg_ccif2_ap_event_mask_b & 0x1) << 19) |
		((pwrctrl->reg_ccif3_md_event_mask_b & 0x1) << 20) |
		((pwrctrl->reg_ccif3_ap_event_mask_b & 0x1) << 21) |
		((pwrctrl->reg_ccifmd_md1_event_mask_b & 0x1) << 22) |
		((pwrctrl->reg_ccifmd_md2_event_mask_b & 0x1) << 23) |
		((pwrctrl->reg_c2k_ps_rccif_wake_mask_b & 0x1) << 24) |
		((pwrctrl->reg_c2k_l1_rccif_wake_mask_b & 0x1) << 25) |
		((pwrctrl->reg_ps_c2k_rccif_wake_mask_b & 0x1) << 26) |
		((pwrctrl->reg_l1_c2k_rccif_wake_mask_b & 0x1) << 27) |
		((pwrctrl->reg_disp2_req_mask_b & 0x1) << 28) |
		((pwrctrl->reg_md_ddr_en_0_mask_b & 0x1) << 29) |
		((pwrctrl->reg_md_ddr_en_1_mask_b & 0x1) << 30) |
		((pwrctrl->reg_conn_ddr_en_mask_b & 0x1) << 31));

	/* SPM_SRC2_MASK */
	spm_write(SPM_SRC2_MASK,
		((pwrctrl->reg_disp0_req_mask_b & 0x1) << 0) |
		((pwrctrl->reg_disp1_req_mask_b & 0x1) << 1) |
		((pwrctrl->reg_disp_od_req_mask_b & 0x1) << 2) |
		((pwrctrl->reg_mfg_req_mask_b & 0x1) << 3) |
		((pwrctrl->reg_vdec0_req_mask_b & 0x1) << 4) |
		((pwrctrl->reg_gce_req_mask_b & 0x1) << 5) |
		((pwrctrl->reg_gce_vrf18_req_mask_b & 0x1) << 6) |
		((pwrctrl->reg_lpdma_req_mask_b & 0x1) << 7) |
		((pwrctrl->reg_conn_srcclkena_cksel2_mask_b & 0x1) << 8) |
		((pwrctrl->reg_sspm_apsrc_req_ddren_mask_b & 0x1) << 9) |
		((pwrctrl->reg_scp_apsrc_req_ddren_mask_b & 0x1) << 10) |
		((pwrctrl->reg_md_vrf18_req_0_mask_b & 0x1) << 11) |
		((pwrctrl->reg_md_vrf18_req_1_mask_b & 0x1) << 12) |
		((pwrctrl->reg_next_dvfs_level0_mask_b & 0x1) << 13) |
		((pwrctrl->reg_next_dvfs_level1_mask_b & 0x1) << 14) |
		((pwrctrl->reg_next_dvfs_level2_mask_b & 0x1) << 15) |
		((pwrctrl->reg_next_dvfs_level3_mask_b & 0x1) << 16) |
		((pwrctrl->reg_next_dvfs_level4_mask_b & 0x1) << 17) |
		((pwrctrl->reg_sw2spm_int0_mask_b & 0x1) << 18) |
		((pwrctrl->reg_sw2spm_int1_mask_b & 0x1) << 19) |
		((pwrctrl->reg_sw2spm_int2_mask_b & 0x1) << 20) |
		((pwrctrl->reg_sw2spm_int3_mask_b & 0x1) << 21) |
		((pwrctrl->reg_sspm2spm_int0_mask_b & 0x1) << 22) |
		((pwrctrl->reg_sspm2spm_int1_mask_b & 0x1) << 23) |
		((pwrctrl->reg_sspm2spm_int2_mask_b & 0x1) << 24) |
		((pwrctrl->reg_sspm2spm_int3_mask_b & 0x1) << 25) |
		((pwrctrl->reg_dqssoc_req_mask_b & 0x1) << 26));

	/* SPM_SRC3_MASK */
	spm_write(SPM_SRC3_MASK,
		((pwrctrl->reg_mpwfi_op & 0x1) << 0) |
		((pwrctrl->reg_spm_resource_req_rsv1_4_mask_b & 0x1) << 1) |
		((pwrctrl->reg_spm_resource_req_rsv1_3_mask_b & 0x1) << 2) |
		((pwrctrl->reg_spm_resource_req_rsv1_2_mask_b & 0x1) << 3) |
		((pwrctrl->reg_spm_resource_req_rsv1_1_mask_b & 0x1) << 4) |
		((pwrctrl->reg_spm_resource_req_rsv1_0_mask_b & 0x1) << 5) |
		((pwrctrl->reg_spm_resource_req_rsv0_4_mask_b & 0x1) << 6) |
		((pwrctrl->reg_spm_resource_req_rsv0_3_mask_b & 0x1) << 7) |
		((pwrctrl->reg_spm_resource_req_rsv0_2_mask_b & 0x1) << 8) |
		((pwrctrl->reg_spm_resource_req_rsv0_1_mask_b & 0x1) << 9) |
		((pwrctrl->reg_spm_resource_req_rsv0_0_mask_b & 0x1) << 10) |
		((pwrctrl->reg_srcclkeni2_cksel3_mask_b & 0x1) << 11) |
		((pwrctrl->reg_srcclkeni2_cksel2_mask_b & 0x1) << 12) |
		((pwrctrl->reg_srcclkeni1_cksel3_mask_b & 0x1) << 13) |
		((pwrctrl->reg_srcclkeni1_cksel2_mask_b & 0x1) << 14) |
		((pwrctrl->reg_srcclkeni0_cksel3_mask_b & 0x1) << 15) |
		((pwrctrl->reg_srcclkeni0_cksel2_mask_b & 0x1) << 16) |
		((pwrctrl->reg_md_ddr_en_0_dbc_en & 0x1) << 17) |
		((pwrctrl->reg_md_ddr_en_1_dbc_en & 0x1) << 18) |
		((pwrctrl->reg_conn_ddr_en_dbc_en & 0x1) << 19) |
		((pwrctrl->reg_sspm_mask_b & 0x1) << 20) |
		((pwrctrl->reg_md_0_mask_b & 0x1) << 21) |
		((pwrctrl->reg_md_1_mask_b & 0x1) << 22) |
		((pwrctrl->reg_scp_mask_b & 0x1) << 23) |
		((pwrctrl->reg_srcclkeni0_mask_b & 0x1) << 24) |
		((pwrctrl->reg_srcclkeni1_mask_b & 0x1) << 25) |
		((pwrctrl->reg_srcclkeni2_mask_b & 0x1) << 26) |
		((pwrctrl->reg_md_apsrc_1_sel & 0x1) << 27) |
		((pwrctrl->reg_md_apsrc_0_sel & 0x1) << 28) |
		((pwrctrl->reg_conn_mask_b & 0x1) << 29) |
		((pwrctrl->reg_conn_apsrc_sel & 0x1) << 30) |
		((pwrctrl->reg_md_srcclkena_0_vrf18_mask_b & 0x1) << 31));

	/* SPM_WAKEUP_EVENT_MASK */
	spm_write(SPM_WAKEUP_EVENT_MASK,
		((pwrctrl->reg_wakeup_event_mask & 0xffffffff) << 0));

	/* SPM_EXT_WAKEUP_EVENT_MASK */
	spm_write(SPM_EXT_WAKEUP_EVENT_MASK,
		((pwrctrl->reg_ext_wakeup_event_mask & 0xffffffff) << 0));

	/* MCU0_WFI_EN */
	spm_write(MCU0_WFI_EN,
		((pwrctrl->mcu0_wfi_en & 0x1) << 0));

	/* MCU1_WFI_EN */
	spm_write(MCU1_WFI_EN,
		((pwrctrl->mcu1_wfi_en & 0x1) << 0));

	/* MCU2_WFI_EN */
	spm_write(MCU2_WFI_EN,
		((pwrctrl->mcu2_wfi_en & 0x1) << 0));

	/* MCU3_WFI_EN */
	spm_write(MCU3_WFI_EN,
		((pwrctrl->mcu3_wfi_en & 0x1) << 0));

	/* MCU4_WFI_EN */
	spm_write(MCU4_WFI_EN,
		((pwrctrl->mcu4_wfi_en & 0x1) << 0));

	/* MCU5_WFI_EN */
	spm_write(MCU5_WFI_EN,
		((pwrctrl->mcu5_wfi_en & 0x1) << 0));

	/* MCU6_WFI_EN */
	spm_write(MCU6_WFI_EN,
		((pwrctrl->mcu6_wfi_en & 0x1) << 0));

	/* MCU7_WFI_EN */
	spm_write(MCU7_WFI_EN,
		((pwrctrl->mcu7_wfi_en & 0x1) << 0));

	/* MCU8_WFI_EN */
	spm_write(MCU8_WFI_EN,
		((pwrctrl->mcu8_wfi_en & 0x1) << 0));

	/* MCU9_WFI_EN */
	spm_write(MCU9_WFI_EN,
		((pwrctrl->mcu9_wfi_en & 0x1) << 0));

	/* MCU10_WFI_EN */
	spm_write(MCU10_WFI_EN,
		((pwrctrl->mcu10_wfi_en & 0x1) << 0));

	/* MCU11_WFI_EN */
	spm_write(MCU11_WFI_EN,
		((pwrctrl->mcu11_wfi_en & 0x1) << 0));

	/* MCU12_WFI_EN */
	spm_write(MCU12_WFI_EN,
		((pwrctrl->mcu12_wfi_en & 0x1) << 0));

	/* MCU13_WFI_EN */
	spm_write(MCU13_WFI_EN,
		((pwrctrl->mcu13_wfi_en & 0x1) << 0));

	/* MCU14_WFI_EN */
	spm_write(MCU14_WFI_EN,
		((pwrctrl->mcu14_wfi_en & 0x1) << 0));

	/* MCU15_WFI_EN */
	spm_write(MCU15_WFI_EN,
		((pwrctrl->mcu15_wfi_en & 0x1) << 0));

	/* MCU16_WFI_EN */
	spm_write(MCU16_WFI_EN,
		((pwrctrl->mcu16_wfi_en & 0x1) << 0));

	/* MCU17_WFI_EN */
	spm_write(MCU17_WFI_EN,
		((pwrctrl->mcu17_wfi_en & 0x1) << 0));
	/* Auto-gen End */

	/* for gps only case at suspend and sodi3 */
	if (spm_for_gps_flag) {
		u32 value;

		/* spm_crit2("for gps only case\n"); */
		value = spm_read(SPM_CLK_CON);
		value &= (~(0x1 << 6));
		value &= (~(0x1 << 13));
		value |= (0x1 << 1);
		value &= (~(0x1 << 0));
		spm_write(SPM_CLK_CON, value);

		value = spm_read(SPM_SRC3_MASK);
		value &= (~(0x1 << 25));
		spm_write(SPM_SRC3_MASK, value);

		value = spm_read(SPM_SRC_MASK);
		value &= (~(0x1 << 12));
		spm_write(SPM_SRC_MASK, value);
	}

	spm_write(SPM_SW_RSV_5,
			(spm_read(SPM_SW_RSV_5) & ~(0xff << 8 | 0x3 << 1)) |
			(1 << (pwrctrl->opp_level + 8)));
}

void __spm_set_wakeup_event(const struct pwr_ctrl *pwrctrl)
{
	u32 val, mask, isr;

	val = __spm_get_pcm_timer_val(pwrctrl);

	spm_write(PCM_TIMER_VAL, val);
	spm_write(PCM_CON1, spm_read(PCM_CON1) | SPM_REGWR_CFG_KEY | RG_PCM_TIMER_EN_LSB);

	/* unmask AP wakeup source */
	if (pwrctrl->wake_src_cust == 0)
		mask = pwrctrl->wake_src;
	else
		mask = pwrctrl->wake_src_cust;

	if (pwrctrl->syspwreq_mask)
		mask &= ~WAKE_SRC_R12_CSYSPWRUPREQ_B;
	spm_write(SPM_WAKEUP_EVENT_MASK, ~mask);

	/* unmask SPM ISR (keep TWAM setting) */
	isr = spm_read(SPM_IRQ_MASK) & REG_TWAM_IRQ_MASK_LSB;
	spm_write(SPM_IRQ_MASK, isr | ISRM_RET_IRQ_AUX);
}

void __spm_kick_pcm_to_run(struct pwr_ctrl *pwrctrl)
{
	u32 con0;

	/* init register to match PCM expectation */
	spm_write(SPM_MAS_PAUSE_MASK_B, 0xffffffff);
	spm_write(SPM_MAS_PAUSE2_MASK_B, 0xffffffff);
	spm_write(PCM_REG_DATA_INI, 0);

	/* set PCM flags and data */
	if (pwrctrl->pcm_flags_cust_clr != 0)
		pwrctrl->pcm_flags &= ~pwrctrl->pcm_flags_cust_clr;
	if (pwrctrl->pcm_flags_cust_set != 0)
		pwrctrl->pcm_flags |= pwrctrl->pcm_flags_cust_set;
	if (pwrctrl->pcm_flags1_cust_clr != 0)
		pwrctrl->pcm_flags1 &= ~pwrctrl->pcm_flags1_cust_clr;
	if (pwrctrl->pcm_flags1_cust_set != 0)
		pwrctrl->pcm_flags1 |= pwrctrl->pcm_flags1_cust_set;
	spm_write(SPM_SW_FLAG, pwrctrl->pcm_flags);
	/* cannot modify pcm_flags1[15:12] which is from bootup setting */
	spm_write(SPM_RSV_CON2, (spm_read(SPM_RSV_CON2) & 0xf000) | (pwrctrl->pcm_flags1 & 0xfff));

	/* enable r0 and r7 to control power */
	spm_write(PCM_PWR_IO_EN, PCM_PWRIO_EN_R0 | PCM_PWRIO_EN_R7);

	/* kick PCM to run (only toggle PCM_KICK) */
	con0 = spm_read(PCM_CON0) & ~(RG_IM_KICK_L_LSB | RG_PCM_KICK_L_LSB);
	spm_write(PCM_CON0, con0 | SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | RG_PCM_KICK_L_LSB);
	spm_write(PCM_CON0, con0 | SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB);
}

void __spm_clean_after_wakeup(void)
{
	/* [Vcorefs] can not switch back to POWER_ON_VAL0 here,
	 * the FW stays in VCORE DVFS which use r0 to Ctrl MEM
	 */
	/* disable r0 and r7 to control power */
	/* spm_write(PCM_PWR_IO_EN, 0); */

	/* clean CPU wakeup event */
	spm_write(SPM_CPU_WAKEUP_EVENT, 0);

	/* [Vcorefs] not disable pcm timer here, due to the
	 * following vcore dvfs will use it for latency check
	 */
	/* clean PCM timer event */
	/* spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | (spm_read(PCM_CON1) & ~RG_PCM_TIMER_EN_LSB)); */

	/* clean wakeup event raw status (for edge trigger event) */
	spm_write(SPM_WAKEUP_EVENT_MASK, ~0);

	/* clean ISR status (except TWAM) */
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) | ISRM_ALL_EXC_TWAM);
	spm_write(SPM_IRQ_STA, ISRC_ALL_EXC_TWAM);
	spm_write(SPM_SWINT_CLR, PCM_SW_INT_ALL);
}
#endif /* CONFIG_MTK_SPM_IN_ATF */

void __spm_get_wakeup_status(struct wake_status *wakesta)
{
	/* get PC value if PCM assert (pause abort) */
	wakesta->assert_pc = spm_read(PCM_REG_DATA_INI);

	/* get wakeup event */
	wakesta->r12 = spm_read(SPM_SW_RSV_0);        /* backup of PCM_REG12_DATA */
	wakesta->r12_ext = spm_read(PCM_REG12_EXT_DATA);
	wakesta->raw_sta = spm_read(SPM_WAKEUP_STA);
	wakesta->raw_ext_sta = spm_read(SPM_WAKEUP_EXT_STA);
	wakesta->wake_misc = spm_read(SPM_BSI_D0_SR);	/* backup of SPM_WAKEUP_MISC */

	/* get sleep time */
	wakesta->timer_out = spm_read(SPM_BSI_D1_SR);	/* backup of PCM_TIMER_OUT */

	/* get other SYS and co-clock status */
	wakesta->r13 = spm_read(PCM_REG13_DATA);
	wakesta->idle_sta = spm_read(SUBSYS_IDLE_STA);

	/* get debug flag for PCM execution check */
	wakesta->debug_flag = spm_read(SPM_SW_DEBUG);

	/* get special pattern (0xf0000 or 0x10000) if sleep abort */
	wakesta->event_reg = spm_read(SPM_BSI_D2_SR);	/* PCM_EVENT_REG_STA */

	/* get ISR status */
	wakesta->isr = spm_read(SPM_IRQ_STA);
}

#define spm_print(suspend, fmt, args...)	\
do {						\
	if (!suspend)				\
		spm_debug(fmt, ##args);		\
	else					\
		spm_crit2(fmt, ##args);		\
} while (0)

void rekick_vcorefs_scenario(void)
{
	int flag;

	if (spm_read(PCM_REG15_DATA) == 0x0) {
		flag = spm_dvfs_flag_init();
		spm_go_to_vcorefs(flag);
	}
}

wake_reason_t __spm_output_wake_reason(const struct wake_status *wakesta,
		const struct pcm_desc *pcmdesc, bool suspend, const char *scenario)
{
	int i;
	char buf[LOG_BUF_SIZE] = { 0 };
	wake_reason_t wr = WR_UNKNOWN;

	if (wakesta->assert_pc != 0) {
		/* add size check for vcoredvfs */
		spm_print(suspend, "PCM ASSERT AT %u (%s), r13 = 0x%x, debug_flag = 0x%x\n",
			  wakesta->assert_pc, scenario, wakesta->r13, wakesta->debug_flag);

		aee_kernel_warning("SPM Warning", "SPM F/W ASSERT WARNING");

		return WR_PCM_ASSERT;
	}

	if (wakesta->r12 & WAKE_SRC_R12_PCMTIMER) {
		if (wakesta->wake_misc & WAKE_MISC_PCM_TIMER) {
			strcat(buf, " PCM_TIMER");
			wr = WR_PCM_TIMER;
		}
		if (wakesta->wake_misc & WAKE_MISC_TWAM) {
			strcat(buf, " TWAM");
			wr = WR_WAKE_SRC;
		}
		if (wakesta->wake_misc & WAKE_MISC_CPU_WAKE) {
			strcat(buf, " CPU");
			wr = WR_WAKE_SRC;
		}
	}
	for (i = 1; i < 32; i++) {
		if (wakesta->r12 & (1U << i)) {
			if ((strlen(buf) + strlen(wakesrc_str[i])) < LOG_BUF_SIZE)
				strncat(buf, wakesrc_str[i], strlen(wakesrc_str[i]));

			wr = WR_WAKE_SRC;
		}
	}
	WARN_ON(strlen(buf) >= LOG_BUF_SIZE);

	spm_print(suspend, "wake up by %s, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x, ch = %u\n",
		  buf, wakesta->timer_out, wakesta->r13, wakesta->debug_flag, wakesta->dcs_ch);

	spm_print(suspend,
		  "r12 = 0x%x, r12_ext = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
		  wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
		  wakesta->event_reg, wakesta->isr);

	spm_print(suspend,
		"raw_ext_sta = 0x%x, wake_misc = 0x%x, pcm_flag = 0x%x 0x%x 0x%x, req = 0x%x, resource = 0x%x\n",
		wakesta->raw_ext_sta,
		wakesta->wake_misc,
		spm_read(SPM_SW_FLAG),
		spm_read(SPM_RSV_CON2),
		spm_read(SPM_SW_RSV_4),
		spm_read(SPM_SRC_REQ),
		spm_get_resource_usage());

	return wr;
}

long int spm_get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

void spm_set_dummy_read_addr(int debug)
{
	u64 rank0_addr, rank1_addr;
	u32 dram_rank_num;

#ifdef CONFIG_MTK_DRAMC
	dram_rank_num = g_dram_info_dummy_read->rank_num;
	rank0_addr = g_dram_info_dummy_read->rank_info[0].start;
	if (dram_rank_num == 1)
		rank1_addr = rank0_addr;
	else
		rank1_addr = g_dram_info_dummy_read->rank_info[1].start;
#else
	dram_rank_num = 1;
	rank0_addr = rank1_addr = 0x40000000;
#endif /* CONFIG_MTK_DRAMC */

	if (debug) {
		spm_crit("dram_rank_num: %d\n", dram_rank_num);
		spm_crit("dummy read addr: rank0: 0x%llx, rank1: 0x%llx\n", rank0_addr, rank1_addr);
	}

	MAPPING_DRAM_ACCESS_ADDR(rank0_addr);
	MAPPING_DRAM_ACCESS_ADDR(rank1_addr);

	if (debug)
		spm_crit("dummy read addr(4GB: %d): rank0: 0x%llx, rank1: 0x%llx\n",
				enable_4G(), rank0_addr, rank1_addr);

#if !defined(CONFIG_MTK_SPM_IN_ATF)
	spm_write(SPM_PASR_DPD_2, rank0_addr & 0xffffffff);
	spm_write(SPM_PASR_DPD_3, rank1_addr & 0xffffffff);
	if ((rank0_addr >> 32) & 0x1)
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) | (1 << 3));
	else
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) & ~(1 << 3));
	if ((rank1_addr >> 32) & 0x1)
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) | (1 << 4));
	else
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) & ~(1 << 4));
#else
	mt_secure_call(MTK_SIP_KERNEL_SPM_DUMMY_READ, rank0_addr, rank1_addr, 0);
#endif /* CONFIG_MTK_SPM_IN_ATF */
}

void __spm_set_pcm_wdt(int en)
{
	/* enable PCM WDT (normal mode) to start count if needed */
	if (en) {
		u32 con1;

		con1 = spm_read(PCM_CON1) & ~(RG_PCM_WDT_WAKE_LSB);
		spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | con1);

		if (spm_read(PCM_TIMER_VAL) > PCM_TIMER_MAX)
			spm_write(PCM_TIMER_VAL, PCM_TIMER_MAX);
		spm_write(PCM_WDT_VAL, spm_read(PCM_TIMER_VAL) + PCM_WDT_TIMEOUT);
		spm_write(PCM_CON1, con1 | SPM_REGWR_CFG_KEY | RG_PCM_WDT_EN_LSB);
	} else {
		spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | (spm_read(PCM_CON1) &
		~RG_PCM_WDT_EN_LSB));
	}

}

int __attribute__ ((weak)) get_dynamic_period(int first_use, int first_wakeup_time,
					      int battery_capacity_level)
{
	pr_err("NO %s !!!\n", __func__);
	return 5400;
}

u32 _spm_get_wake_period(int pwake_time, wake_reason_t last_wr)
{
	int period = SPM_WAKE_PERIOD;

	if (pwake_time < 0) {
		/* use FG to get the period of 1% battery decrease */
		period = get_dynamic_period(last_wr != WR_PCM_TIMER ? 1 : 0, SPM_WAKE_PERIOD, 1);
		if (period <= 0) {
			spm_warn("CANNOT GET PERIOD FROM FUEL GAUGE\n");
			period = SPM_WAKE_PERIOD;
		}
	} else {
		period = pwake_time;
		spm_crit2("pwake = %d\n", pwake_time);
	}

	if (period > 36 * 3600)	/* max period is 36.4 hours */
		period = 36 * 3600;

	return period;
}

int get_channel_lock(bool blocking)
{
#ifdef CONFIG_MTK_DCS
	int ch = 0, ret = -1;
	enum dcs_status dcs_status;

	if (blocking)
		ret = dcs_get_dcs_status_lock(&ch, &dcs_status);
	else
		ret = dcs_get_dcs_status_trylock(&ch, &dcs_status);

	if (ret) {
		aee_kernel_warning("DCS Warring", "dcs_get_dcs_status_lock busy");
		return -1;
	}

	return ch;
#else
	/* return get_dram_channel_number(); */
	return 4; /* FIXME */
#endif
}

void get_channel_unlock(void)
{
#ifdef CONFIG_MTK_DCS
	dcs_get_dcs_status_unlock();

#else
	/* Nothing */
#endif
}

MODULE_DESCRIPTION("SPM-Internal Driver v0.1");
