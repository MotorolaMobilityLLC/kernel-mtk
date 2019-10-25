/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include "mtk_spm_internal.h"
#include "mtk_spm_pmic_wrap.h"
#include "mtk_spm_vcore_dvfs.h"
#include "mtk_vcorefs_governor.h"
#include "upmu_common.h"

#include <asm/setup.h>
#include <linux/of_fdt.h>
#include <mtk_dramc.h>

/*
 * Define and Declare
 */
#define SPM_DVFS_TIMEOUT 2000

#define SPM_CHK_GUARD_TIME 10
#define SPM_HPM_HOLD_TIME 1000

/* PCM_REG6_DATA */
#define SPM_FLAG_DVFS_ACTIVE (1 << 22)
#define SPM_FLAG_DVFS_STATE (0x3 << 23)

/* SPM_SW_RSV_5 */
#define SPM_DVFS_ACTIVE (1 << 2)
#define SPM_DVFS_STATE (3 << 0)
#define SPM_SW_RSV_6_VER2 (1 << 15)

/* bw threshold  for SPM_SW_RSV_5, SPM_SW_RSV_4 */
#define HPM_THRES_OFFSET 24
#define LPM_THRES_OFFSET 16
#define ULPM_THRES_OFFSET 8

#ifdef CONFIG_MTK_RAM_CONSOLE
#define VCOREFS_AEE_RR_REC 1
#else
#define VCOREFS_AEE_RR_REC 0
#endif

int spm_history_opp[NUM_KIR_GROUP];

#if VCOREFS_AEE_RR_REC
enum spm_vcorefs_step {
	SPM_VCOREFS_LEAVE = 0,
	SPM_VCOREFS_ENTER,
	SPM_VCOREFS_DVFS_START,
	SPM_VCOREFS_DVFS_END
};

void set_aee_vcore_dvfs_status(int state)
{
	u32 value = aee_rr_curr_vcore_dvfs_status();

	value &= ~(0x000000FF);
	value |= (state & 0x000000FF);
	aee_rr_rec_vcore_dvfs_status(value);
}
#else
void set_aee_vcore_dvfs_status(int state) { /* nothing */ }
#endif

static struct pwr_ctrl vcore_dvfs_ctrl = {
#if 1
	/* default VCORE DVFS is disabled */
	.pcm_flags = (SPM_FLAG_DIS_VCORE_DVS | SPM_FLAG_DIS_VCORE_DFS),
#endif
	.wake_src = WAKE_SRC_R12_PCM_TIMER,
	/* SPM general */
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,

	/* VCORE DVFS Logic pwr_ctrl */
	.dvfs_halt_mask_b = 0x7, /* 5 bit, enable for isp/disp, disable gce */
	.sdio_on_dvfs_req_mask_b = 0,

	.cpu_md_dvfs_req_merge_mask_b = 1, /* HPM request by WFD/MHL/MD */

	.md_ddr_en_dvfs_halt_mask_b = 0,

	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,

	.cpu_md_emi_dvfs_req_prot_dis = 1,

	.spm_dvfs_req = 1, /* set to 1 for keep high after fw loading */
	.spm_dvfs_force_down = 1,
	.cpu_md_dvfs_sop_force_on = 0,

	.emi_bw_dvfs_req_mask = 1,      /* default disable, enable by fliper */
	.emi_boost_dvfs_req_mask_b = 0, /* default disable, enable by fliper */

	.emi_bw_dvfs_req_2_mask = 1,      /* default disable */
	.emi_boost_dvfs_req_2_mask_b = 0, /* default disable */
	.sw_ctrl_event_on_2 = 0,	  /* default disable */

	.dvfs_halt_src_chk = 1,

	.md_srcclkena_0_2d_dvfs_req_mask_b = 1,
	.md_srcclkena_1_2d_dvfs_req_mask_b = 1,
	.dvfs_up_2d_dvfs_req_mask_b = 1,
	.disable_off_load_lpm = 0,
	.en_sdio_dvfs_setting = 0,
	.sw_ctrl_event_on = 1,
	.rsv6_legacy_version = 0,
	.en_emi_grouping = 0,
	/* +450 SPM_EMI_BW_MODE */
	/* [0]EMI_BW_MODE, [1]EMI_BOOST_MODE default is 0 */

};

struct spm_lp_scen __spm_vcore_dvfs = {
	/*	.pcmdesc = &sodi_ddrdfs_pcm, */
	.pwrctrl = &vcore_dvfs_ctrl,
};

/*
 * Macro and Inline
 */
static inline int _wait_spm_dvfs_idle(int timeout)
{
	int i = 0;
	u32 val = spm_read(SPM_SW_RSV_5);

	while (!((val & SPM_DVFS_ACTIVE) == 0)) {
		if (i >= timeout) {
			i = -EBUSY;
			break;
		}
		udelay(1);
		val = spm_read(SPM_SW_RSV_5);
		i++;
	}
	return i;
}

static inline int _wait_spm_dvfs_complete(int opp, int timeout)
{
	int i = 0;
	u32 val;

	/* wait 10 usec before check status */
	udelay(SPM_CHK_GUARD_TIME);

	val = spm_read(SPM_SW_RSV_5);

	while (!(((val & SPM_DVFS_ACTIVE) == 0) &&
		 ((val & SPM_DVFS_STATE) == opp))) {
		if (i >= timeout) {
			i = -EBUSY;
			break;
		}
		udelay(1);
		val = spm_read(SPM_SW_RSV_5);
		i++;
	}

	return i;
}

/**************************************
 * Static function
 **************************************/
bool _get_total_bw_enable(void)
{
	bool enabled = true;

	if ((spm_read(SPM_SRC_MASK) & EMI_BW_DVFS_REQ_MASK_LSB) != 0)
		enabled = false;
	else
		enabled = true;

	return enabled;
}

/* get SPM DVFS Logic output */
int _find_spm_dvfs_result(int opp)
{
	int expect_opp;
	u32 val = spm_read(SPM_SRC_REQ);
	bool is_forced_hpm = (val & SPM_DVFS_REQ_LSB) ? true : false;
	bool is_forced_lpm = (val & SPM_DVFS_FORCE_DOWN_LSB) ? true : false;
	bool is_cpu_md_dvfs_sop_force_on =
	    (val & CPU_MD_DVFS_SOP_FORCE_ON_LSB) ? true : false;
	u32 CPU_val = spm_read(CPU_DVFS_REQ) & 0xFFFF;
	u32 MD2SPM_val = spm_read(MD2SPM_DVFS_CON) & 0xFFFF;

	if (is_forced_lpm == true)
		expect_opp = OPPI_LOW_PWR;
	else if (is_forced_hpm == true)
		expect_opp = OPPI_PERF;
	else if (is_cpu_md_dvfs_sop_force_on == true)
		expect_opp = OPPI_PERF;
	else {
		if ((CPU_val & MD2SPM_val) != 0)
			expect_opp = OPPI_PERF;
		else
			expect_opp = OPPI_LOW_PWR;
	}

	return expect_opp;
}

static void dump_pmic_info(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	u32 ret0, ret1, ret2;
	u32 val0, val1, val2;

	ret0 = pmic_read_interface_nolock(MT6355_HWCID, &val0, 0xffff, 0);
	ret1 = pmic_read_interface_nolock(MT6355_SWCID, &val1, 0xffff, 0);
	ret2 =
	    pmic_read_interface_nolock(MT6355_DEW_READ_TEST, &val2, 0xffff, 0);
	spm_crit("[PMIC]hw_cid[000](%d)=0x%x sw_cid[002](%d)=0x%x dew_test[c04](%d)=0x%x\n",
		 ret0, val0, ret1, val1, ret2, val2);

	ret0 = pmic_read_interface_nolock(MT6355_BUCK_VCORE_CON1, &val0, 0xffff,
					  0);
	spm_notice("[PMIC]vcore vosel[1060](%d)=0x%x\n", ret0, val0);

	ret0 = pmic_read_interface_nolock(MT6355_BUCK_VCORE_CFG0, &val0, 0xffff,
					  0);
	spm_crit("[PMIC]vcore slope[1064](%d)=0x%x\n", ret0, val0);
#else
	u32 ret0, ret1, ret2;
	u32 val0, val1, val2;

	ret0 = pmic_read_interface_nolock(MT6351_HWCID, &val0, 0xffff, 0);
	ret1 = pmic_read_interface_nolock(MT6351_SWCID, &val1, 0xffff, 0);
	ret2 =
	    pmic_read_interface_nolock(MT6351_DEW_READ_TEST, &val2, 0xffff, 0);
	spm_notice("[PMIC]hw_cid[200](%d)=0x%x sw_cid[202](%d)=0x%x dew_test[2F4](%d)=0x%x\n",
		   ret0, val0, ret1, val1, ret2, val2);
	ret0 = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON0, &val0, 0xffff,
					  0);
	ret1 = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON5, &val1, 0xffff,
					  0);
	ret1 = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON4, &val2, 0xffff,
					  0);
	spm_notice("[PMIC]vcore ctrl[600](%d)=0x%x vosel_on[608](%d)=0x%x "
		   "vosel[60A](%d)=0x%x\n",
		   ret0, val0, ret1, val1, ret2, val2);

	ret0 = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON3, &val0, 0xffff,
					  0);
	ret1 =
	    pmic_read_interface_nolock(MT6351_VCORE_ANA_CON5, &val1, 0xffff, 0);
	ret2 =
	    pmic_read_interface_nolock(MT6351_VCORE_ANA_CON7, &val2, 0xffff, 0);
	spm_crit("[PMIC]vcore slope[606](%d)=0x%x pfm_rip[458](%d)=0x%x en_lowiq[45C](%d)=0x%x\n",
		 ret0, val0, ret1, val1, ret2, val2);
#endif
#endif
}

static int _check_dvfs_result(int vcore, int ddr, int log_en)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int vcore_val = 0;
	int ddr_val = 0;

	bool dvs_en = ((pwrctrl->pcm_flags & SPM_FLAG_DIS_VCORE_DVS) == 0);
	bool dfs_en = ((pwrctrl->pcm_flags & SPM_FLAG_DIS_VCORE_DFS) == 0);

	if (dvs_en == true) {
		vcore_val = vcorefs_get_curr_vcore();
		if (vcore_val != vcore) {
			spm_vcorefs_err("VCORE FAIL: %d(%d)\n", vcore_val,
					vcore);
			dump_pmic_info();
			spm_vcorefs_dump_dvfs_regs(NULL);
			WARN_ON(1);
		}
	}

	if (dfs_en == true) {
		ddr_val = vcorefs_get_curr_ddr();
		if (ddr_val != ddr) {
			spm_vcorefs_err("DDR FAIL: %d(%d)\n", ddr_val, ddr);
			dump_pmic_info();
			spm_vcorefs_dump_dvfs_regs(NULL);
			WARN_ON(1);
		}
	}
	if (log_en)
		spm_vcorefs_info("%s(dvs_en=%d dfs_en=%d)(vcore=%d ddr=%d)\n",
				 __func__, dvs_en, dfs_en, vcore_val, ddr_val);
	return 0;
}

static void __go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	/* struct pcm_desc *pcmdesc = __spm_vcore_dvfs.pcmdesc; */
	struct pcm_desc *pcmdesc = NULL;
	int pcm_index;

	if (get_ddr_type() == TYPE_LPDDR3)
		pcm_index = DYNA_LOAD_PCM_SODI;
	else
		pcm_index = DYNA_LOAD_PCM_SODI_LPDDR4;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	if (get_ddr_type() == TYPE_LPDDR4)
		pcm_index = DYNA_LOAD_PCM_SODI_LPDDR4_2400;
#endif

	if (dyna_load_pcm[pcm_index].ready)
		pcmdesc = &(dyna_load_pcm[pcm_index].desc);
	else
		WARN_ON(1);

	spm_vcorefs_info("[%s] spm_flags=0x%x fw=%s (ddr_type=%d)\n", __func__,
			 spm_flags, pcmdesc->version, get_ddr_type());

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);
}

void spm_vcorefs_init_state(int opp)
{
	spm_write(SPM_SW_RSV_4, 0);
	spm_write(SPM_SW_RSV_5, 0);

	spm_write(SPM_SW_RSV_5, opp);

	/* SPM_EMI_BW_MODE[0]&[1] set to 0 */
	spm_write(SPM_EMI_BW_MODE, spm_read(SPM_EMI_BW_MODE) &
				       ~(EMI_BW_MODE_LSB | EMI_BOOST_MODE_LSB));
	spm_vcorefs_info("init_state(RSV_5=0x%x)\n", spm_read(SPM_SW_RSV_5));
	spm_vcorefs_info("EMI_BW_MODE=0x%8x\n", spm_read(SPM_EMI_BW_MODE));
}

void spm_update_rsv_6(void)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (pwrctrl->rsv6_legacy_version == 1)
		spm_write(
		    SPM_SW_RSV_6,
		    ((pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b & 0x1) << 0) |
			((pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b & 0x1)
			 << 1) |
			((pwrctrl->dvfs_up_2d_dvfs_req_mask_b & 0x1) << 2) |
			((pwrctrl->disable_off_load_lpm & 0x1) << 3) |
			((pwrctrl->en_sdio_dvfs_setting & 0x1) << 4) |
			((pwrctrl->en_emi_grouping & 0x1) << 18));
	else
		spm_write(
		    SPM_SW_RSV_6,
		    ((pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b & 0x1) << 0) |
			((pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b & 0x1)
			 << 1) |
			((pwrctrl->dvfs_up_2d_dvfs_req_mask_b & 0x1) << 2) |
			((pwrctrl->disable_off_load_lpm & 0x1) << 16) |
			((pwrctrl->en_sdio_dvfs_setting & 0x1) << 17) |
			((pwrctrl->en_emi_grouping & 0x1) << 18));
}
/*
 * External Function
 */

char *spm_vcorefs_dump_dvfs_regs(char *p)
{
	if (p) {
		p += sprintf(p, "PCM_IM_PTR: 0x%x (%u)\n", spm_read(PCM_IM_PTR),
			     spm_read(PCM_IM_LEN));
		p += sprintf(p, "SPM_SW_FLAG: 0x%x, SPM_SW_DEBUG: 0x%x, MD2SPM_DVFS_CON: 0x%x, CPU_DVFS_REQ: 0x%x\n",
			     spm_read(SPM_SW_FLAG), spm_read(SPM_SW_DEBUG),
			     spm_read(MD2SPM_DVFS_CON), spm_read(CPU_DVFS_REQ));
		p += sprintf(p, "SPM_SRC_REQ: 0x%x, SPM_SRC_MASK: 0x%x, SPM_SRC2_MASK: 0x%x, SW_CRTL_EVENT:0x%x\n",
			     spm_read(SPM_SRC_REQ), spm_read(SPM_SRC_MASK),
			     spm_read(SPM_SRC2_MASK), spm_read(SW_CRTL_EVENT));
		p += sprintf(
		    p, "RSV_3: 0x%x, RSV_4: 0x%x, RSV_5: 0x%x, RSV_6: 0x%x\n",
		    spm_read(SPM_SW_RSV_3), spm_read(SPM_SW_RSV_4),
		    spm_read(SPM_SW_RSV_5), spm_read(SPM_SW_RSV_6));
		p +=
		    sprintf(p, "R0: 0x%x, R6: 0x%x, R13: 0x%x, R15: 0x%x\n",
			    spm_read(PCM_REG0_DATA), spm_read(PCM_REG6_DATA),
			    spm_read(PCM_REG13_DATA), spm_read(PCM_REG15_DATA));
		p += sprintf(p, "R12: 0x%x, R12_STA: 0x%x, R12_EXT: 0x%x, R12_EXT_STA: 0x%x\n",
			     spm_read(PCM_REG12_DATA),
			     spm_read(PCM_REG12_MASK_B_STA),
			     spm_read(PCM_REG12_EXT_DATA),
			     spm_read(PCM_REG12_EXT_MASK_B_STA));
		p += sprintf(p, "SPM_RSV_CON: 0x%x, SPM_RSV_STA: 0x%x SW_CRTL_EVENT_2: 0x%x\n",
			     spm_read(SPM_RSV_CON), spm_read(SPM_RSV_STA),
			     spm_read(SW_CRTL_EVENT_2));
	} else {
		spm_vcorefs_info("PCM_IM_PTR    : 0x%x (%u)\n",
				 spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));
		spm_vcorefs_info("SPM_SW_FLAG: 0x%x, SPM_SW_DEBUG: 0x%x, MD2SPM_DVFS_CON: 0x%x, CPU_DVFS_REQ: 0x%x\n",
				 spm_read(SPM_SW_FLAG), spm_read(SPM_SW_DEBUG),
				 spm_read(MD2SPM_DVFS_CON),
				 spm_read(CPU_DVFS_REQ));
		spm_vcorefs_info("SPM_SRC_REQ: 0x%x, SPM_SRC_MASK: 0x%x, SPM_SRC2_MASK: 0x%x, SW_CRTL_EVENT:0x%x\n",
				 spm_read(SPM_SRC_REQ), spm_read(SPM_SRC_MASK),
				 spm_read(SPM_SRC2_MASK),
				 spm_read(SW_CRTL_EVENT));
		spm_vcorefs_info(
		    "RSV_3: 0x%x, RSV_4: 0x%x, RSV_5: 0x%x, RSV_6: 0x%x\n",
		    spm_read(SPM_SW_RSV_3), spm_read(SPM_SW_RSV_4),
		    spm_read(SPM_SW_RSV_5), spm_read(SPM_SW_RSV_6));
		spm_vcorefs_info(
		    "R0 : 0x%8x, R1 : 0x%8x, R2 : 0x%8x, R3 : 0x%8x\n",
		    spm_read(PCM_REG0_DATA), spm_read(PCM_REG1_DATA),
		    spm_read(PCM_REG2_DATA), spm_read(PCM_REG3_DATA));
		spm_vcorefs_info(
		    "R4 : 0x%8x, R5 : 0x%8x, R6 : 0x%8x, R7 : 0x%8x\n",
		    spm_read(PCM_REG4_DATA), spm_read(PCM_REG5_DATA),
		    spm_read(PCM_REG6_DATA), spm_read(PCM_REG7_DATA));
		spm_vcorefs_info(
		    "R8 : 0x%8x, R9 : 0x%8x, R10: 0x%8x, R11: 0x%8x\n",
		    spm_read(PCM_REG8_DATA), spm_read(PCM_REG9_DATA),
		    spm_read(PCM_REG10_DATA), spm_read(PCM_REG11_DATA));
		spm_vcorefs_info(
		    "R12: 0x%8x, R13: 0x%8x, R14: 0x%8x, R15: 0x%8x\n",
		    spm_read(PCM_REG12_DATA), spm_read(PCM_REG13_DATA),
		    spm_read(PCM_REG14_DATA), spm_read(PCM_REG15_DATA));
		spm_vcorefs_info(
		    "R12_STA: 0x%x, R12_EXT: 0x%x, R12_EXT_STA: 0x%x\n",
		    spm_read(PCM_REG12_MASK_B_STA),
		    spm_read(PCM_REG12_EXT_DATA),
		    spm_read(PCM_REG12_EXT_MASK_B_STA));
		spm_vcorefs_info("SPM_RSV_CON: 0x%x, SPM_RSV_STA: 0x%x SW_CRTL_EVENT_2: 0x%x\n",
				 spm_read(SPM_RSV_CON), spm_read(SPM_RSV_STA),
				 spm_read(SW_CRTL_EVENT_2));
	}

	return p;
}

/* first time kick SPM FW for vcore dvfs */
void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	unsigned long flags;
	u32 val, val2;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	spin_lock_irqsave(&__spm_lock, flags);

	dump_pmic_info();

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);

	__go_to_vcore_dvfs(spm_flags, spm_data);
	mdelay(4);
	val2 = spm_read(PCM_REG15_DATA);
	val = spm_read(SPM_SW_RSV_5);

	if (!(val & SPM_SW_RSV_6_VER2))
		pwrctrl->rsv6_legacy_version = 1;

	spm_vcorefs_info("update rsv6_legacy=%d RSV_5=0x%x, R15=0x%x\n",
			 pwrctrl->rsv6_legacy_version, val, val2);

	spm_vcorefs_dump_dvfs_regs(NULL);

	spin_unlock_irqrestore(&__spm_lock, flags);
}

void spm_vcorefs_enable_perform_bw(bool enable, bool lock)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	if (lock == true)
		spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_boost_dvfs_req_mask_b = 1;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) |
					     EMI_BOOST_DVFS_REQ_MASK_B_LSB);
	} else {
		pwrctrl->emi_boost_dvfs_req_mask_b = 0;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) &
					     (~EMI_BOOST_DVFS_REQ_MASK_B_LSB));
	}

	if (lock == true)
		spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("perform BW enable: %d, SPM_SRC2_MASK: 0x%x\n", enable,
			 spm_read(SPM_SRC2_MASK));
}

int spm_vcorefs_set_perform_bw_threshold(u32 ulpm_threshold, u32 lpm_threshold,
					 u32 hpm_threshold)
{
	u32 value;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	value = spm_read(SPM_SW_RSV_4) &
		(~(0xFF << HPM_THRES_OFFSET | 0xFF << LPM_THRES_OFFSET |
		   0xFF << ULPM_THRES_OFFSET));
	value |= ((hpm_threshold << HPM_THRES_OFFSET) |
		  (lpm_threshold << LPM_THRES_OFFSET) |
		  (ulpm_threshold << ULPM_THRES_OFFSET));
	spm_write(SPM_SW_RSV_4, value);

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("perform BW threshold, ULPM: %u LPM: %u, HPM: %u, SPM_SW_RSV_4: 0x%x\n",
			 ulpm_threshold, lpm_threshold, hpm_threshold,
			 spm_read(SPM_SW_RSV_4));
	return 0;
}

void spm_vcorefs_enable_total_bw(bool enable, bool lock)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	if (lock == true)
		spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_bw_dvfs_req_mask = 0;
		spm_write(SPM_SRC_MASK,
			  spm_read(SPM_SRC_MASK) & (~EMI_BW_DVFS_REQ_MASK_LSB));
	} else {
		pwrctrl->emi_bw_dvfs_req_mask = 1;
		spm_write(SPM_SRC_MASK,
			  spm_read(SPM_SRC_MASK) | EMI_BW_DVFS_REQ_MASK_LSB);
	}

	if (lock == true)
		spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("total BW enable: %d, SPM_SRC_MASK: 0x%x\n", enable,
			 spm_read(SPM_SRC_MASK));
}

int spm_vcorefs_set_total_bw_threshold(u32 ulpm_threshold, u32 lpm_threshold,
				       u32 hpm_threshold)
{
	u32 value;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	value = spm_read(SPM_SW_RSV_3) &
		(~(0xFF << HPM_THRES_OFFSET | 0xFF << LPM_THRES_OFFSET |
		   0xFF << ULPM_THRES_OFFSET));
	value |= ((hpm_threshold << HPM_THRES_OFFSET) |
		  (lpm_threshold << LPM_THRES_OFFSET) |
		  (ulpm_threshold << ULPM_THRES_OFFSET));
	spm_write(SPM_SW_RSV_3, value);

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("total BW threshold, ULPM: %u LPM: %u, HPM: %u, SPM_SW_RSV_4: 0x%x\n",
			 ulpm_threshold, lpm_threshold, hpm_threshold,
			 spm_read(SPM_SW_RSV_3));
	return 0;
}

void spm_vcorefs_enable_ulpm_perform_bw(bool enable, bool lock)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	if (lock == true)
		spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_boost_dvfs_req_2_mask_b = 1;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) |
					     EMI_BOOST_DVFS_REQ_2_MASK_B_LSB);
	} else {
		pwrctrl->emi_boost_dvfs_req_2_mask_b = 0;
		spm_write(SPM_SRC2_MASK,
			  spm_read(SPM_SRC2_MASK) &
			      (~EMI_BOOST_DVFS_REQ_2_MASK_B_LSB));
	}

	if (lock == true)
		spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("ulpm perform BW enable: %d, SPM_SRC2_MASK: 0x%x\n",
			 enable, spm_read(SPM_SRC2_MASK));
}

void spm_vcorefs_enable_ulpm_total_bw(bool enable, bool lock)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	if (lock == true)
		spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_bw_dvfs_req_2_mask = 0;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) &
					     (~EMI_BW_DVFS_REQ_2_MASK_LSB));
		pwrctrl->sw_ctrl_event_on_2 = 1;
		spm_write(SW_CRTL_EVENT_2,
			  spm_read(SW_CRTL_EVENT_2) | SW_CRTL_EVENT_ON_LSB);

	} else {
		pwrctrl->sw_ctrl_event_on_2 = 0;
		spm_write(SW_CRTL_EVENT_2,
			  spm_read(SW_CRTL_EVENT_2) & (~SW_CRTL_EVENT_ON_LSB));
		pwrctrl->emi_bw_dvfs_req_2_mask = 1;
		spm_write(SPM_SRC2_MASK,
			  spm_read(SPM_SRC2_MASK) | EMI_BW_DVFS_REQ_2_MASK_LSB);
	}

	if (lock == true)
		spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("ulpm total BW enable: %d, SPM_SRC2_MASK: 0x%x\n",
			 enable, spm_read(SPM_SRC2_MASK));
}

static void spm_vcorefs_config_hpm(int opp)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (opp == OPPI_PERF) {
		pwrctrl->spm_dvfs_req = 1;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) | (SPM_DVFS_REQ_LSB));
	} else {
		pwrctrl->spm_dvfs_req = 0;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) & (~SPM_DVFS_REQ_LSB));
	}
}

static void spm_vcorefs_config_hpm_non_force(int opp)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (opp == OPPI_PERF) {
		pwrctrl->cpu_md_dvfs_sop_force_on = 1;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) | CPU_MD_DVFS_SOP_FORCE_ON_LSB);
	} else {
		pwrctrl->cpu_md_dvfs_sop_force_on = 0;
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) &
					   (~CPU_MD_DVFS_SOP_FORCE_ON_LSB));
	}
}

static void spm_vcorefs_config_ulpm(int opp)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (opp == OPPI_ULTRA_LOW_PWR) {
		pwrctrl->sw_ctrl_event_on = 0;
		spm_write(SW_CRTL_EVENT,
			  spm_read(SW_CRTL_EVENT) & (~SW_CRTL_EVENT_ON_LSB));
	} else {
		pwrctrl->sw_ctrl_event_on = 1;
		spm_write(SW_CRTL_EVENT,
			  spm_read(SW_CRTL_EVENT) | SW_CRTL_EVENT_ON_LSB);
	}
}

int spm_vcorefs_get_opp(void)
{
	int i = 0;
	u32 val = spm_read(SPM_SW_RSV_5);

	while (!((val & SPM_DVFS_ACTIVE) == 0)) {
		if (i >= SPM_DVFS_TIMEOUT) {
			val = spm_read(SPM_SW_RSV_5);
			spm_vcorefs_err("get_opp timeout(%d): RSV_5=0x%x\n", i,
					val);
			break;
		}
		udelay(1);
		val = spm_read(SPM_SW_RSV_5);
		i++;
	}
	return (val & SPM_DVFS_STATE);
}

int spm_vcorefs_is_offload_lpm_enable(void)
{
	u32 val;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	val = spm_read(SPM_SW_RSV_6);
	if (pwrctrl->rsv6_legacy_version == 1)
		val &= (1 << 3);
	else
		val &= (1 << 16);
	return !val;
}

int spm_vcorefs_set_opp(int opp, int vcore, int ddr, bool non_force)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int r = 0;
	unsigned long flags;
	int timeout = SPM_DVFS_TIMEOUT;
	bool is_total_bw_enabled = false;

	spin_lock_irqsave(&__spm_lock, flags);
	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("wait idle timeout(opp:%d)\n", opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);
	if (non_force == true) {
		if (opp == OPPI_PERF) {
			/* disable total bw for dvfs result check */
			is_total_bw_enabled = _get_total_bw_enable();
			spm_vcorefs_enable_total_bw(false, false);
		}
		spm_vcorefs_config_hpm_non_force(opp);
		if (spm_history_opp[KIR_GROUP_HPM] != OPPI_ULTRA_LOW_PWR)
			spm_vcorefs_config_ulpm(opp);
	} else {
		spm_vcorefs_config_hpm(opp);
		if (spm_history_opp[KIR_GROUP_HPM_NON_FORCE] !=
		    OPPI_ULTRA_LOW_PWR)
			spm_vcorefs_config_ulpm(opp);
	}

	/* check spm task done status for HPM */
	if (opp == OPPI_PERF) {
		r = _wait_spm_dvfs_complete(opp, timeout);
		if (r < 0) {
			spm_vcorefs_err("wait complete timeout(opp:%d)\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_warn(
			    "set_hpm_force_complete_timeout(opp:%d)\n", opp);
			__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	/* check vcore and ddr for HPM */
	if (opp == OPPI_PERF) {
		r = _check_dvfs_result(vcore, ddr, 0);
		if (r < 0) {
			spm_vcorefs_err("chk result fail opp:%d\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			WARN_ON(1);
		}
	}

	if (non_force == true) {
		if (opp == OPPI_PERF) {
			/* keep hpm at least 1ms */
			udelay(SPM_HPM_HOLD_TIME);
			/* restore to orignal total bw setting */
			spm_vcorefs_enable_total_bw(is_total_bw_enabled, false);
		}
		spm_history_opp[KIR_GROUP_HPM_NON_FORCE] = opp;
	} else {
		spm_history_opp[KIR_GROUP_HPM] = opp;
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return (r > 0) ? 0 : r;
}

int spm_vcorefs_set_opp_fix(int opp, int vcore, int ddr)
{
	int r = 0;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;
	int timeout = SPM_DVFS_TIMEOUT;
	static int last_fixed_opp = OPPI_UNREQ;
	static struct pwr_ctrl pre_vcore_dvfs_ctrl;

	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	spin_lock_irqsave(&__spm_lock, flags);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("wait idle timeout(opp:%d)\n", opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);
	if (opp != OPPI_UNREQ && last_fixed_opp == OPPI_UNREQ) {
		pre_vcore_dvfs_ctrl.disable_off_load_lpm =
		    pwrctrl->disable_off_load_lpm;
		pre_vcore_dvfs_ctrl.spm_dvfs_req = pwrctrl->spm_dvfs_req;
		pre_vcore_dvfs_ctrl.sw_ctrl_event_on =
		    pwrctrl->sw_ctrl_event_on;
		pre_vcore_dvfs_ctrl.md_srcclkena_0_2d_dvfs_req_mask_b =
		    pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b;
		pre_vcore_dvfs_ctrl.md_srcclkena_1_2d_dvfs_req_mask_b =
		    pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b;
		spm_vcorefs_info(
		    "fixed opp backup(opp:%d, last_opp:%d) (%d, %d, %d, %d, %d)\n",
		    opp, last_fixed_opp, pre_vcore_dvfs_ctrl.spm_dvfs_req,
		    pre_vcore_dvfs_ctrl.sw_ctrl_event_on,
		    pre_vcore_dvfs_ctrl.md_srcclkena_0_2d_dvfs_req_mask_b,
		    pre_vcore_dvfs_ctrl.md_srcclkena_1_2d_dvfs_req_mask_b,
		    pre_vcore_dvfs_ctrl.disable_off_load_lpm);
	}

	pwrctrl->disable_off_load_lpm = (opp == OPPI_UNREQ) ? 0 : 1;
	pwrctrl->dvfs_up_2d_dvfs_req_mask_b = (opp == OPPI_UNREQ) ? 1 : 0;
	switch (opp) {
	case OPPI_PERF:
		pwrctrl->spm_dvfs_req = 1;
		pwrctrl->spm_dvfs_force_down = 1;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) |
			      (SPM_DVFS_REQ_LSB | SPM_DVFS_FORCE_DOWN_LSB));
		pwrctrl->sw_ctrl_event_on = 1;
		spm_write(SW_CRTL_EVENT,
			  spm_read(SW_CRTL_EVENT) | SW_CRTL_EVENT_ON_LSB);
		break;
	case OPPI_LOW_PWR:
		pwrctrl->spm_dvfs_req = 0;
		pwrctrl->spm_dvfs_force_down = 0;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) &
			      ~(SPM_DVFS_REQ_LSB | SPM_DVFS_FORCE_DOWN_LSB));
		pwrctrl->sw_ctrl_event_on = 1;
		spm_write(SW_CRTL_EVENT,
			  spm_read(SW_CRTL_EVENT) | SW_CRTL_EVENT_ON_LSB);
		break;
	case OPPI_ULTRA_LOW_PWR:
		pwrctrl->spm_dvfs_req = 0;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) & (~SPM_DVFS_REQ_LSB));
		pwrctrl->sw_ctrl_event_on = 0;
		spm_write(SW_CRTL_EVENT,
			  spm_read(SW_CRTL_EVENT) & (~SW_CRTL_EVENT_ON_LSB));
		pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b = 0;
		pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b = 0;
		break;
	case OPPI_UNREQ:
		if (last_fixed_opp != OPPI_UNREQ) {
			pwrctrl->spm_dvfs_req =
			    pre_vcore_dvfs_ctrl.spm_dvfs_req;
			pwrctrl->sw_ctrl_event_on =
			    pre_vcore_dvfs_ctrl.sw_ctrl_event_on;
			pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b =
			    pre_vcore_dvfs_ctrl
				.md_srcclkena_0_2d_dvfs_req_mask_b;
			pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b =
			    pre_vcore_dvfs_ctrl
				.md_srcclkena_1_2d_dvfs_req_mask_b;
			pwrctrl->disable_off_load_lpm =
			    pre_vcore_dvfs_ctrl.disable_off_load_lpm;
			spm_vcorefs_info(
			    "fixed opp restore(opp:%d, last_opp:%d) (%d, %d, "
			    "%d, %d, %d)\n",
			    opp, last_fixed_opp,
			    pre_vcore_dvfs_ctrl.spm_dvfs_req,
			    pre_vcore_dvfs_ctrl.sw_ctrl_event_on,
			    pre_vcore_dvfs_ctrl
				.md_srcclkena_0_2d_dvfs_req_mask_b,
			    pre_vcore_dvfs_ctrl
				.md_srcclkena_1_2d_dvfs_req_mask_b,
			    pre_vcore_dvfs_ctrl.disable_off_load_lpm);
		}
		spm_write(SW_CRTL_EVENT, (pwrctrl->sw_ctrl_event_on & 0x1)
					     << 0);
		pwrctrl->spm_dvfs_force_down = 1;
		if (pwrctrl->spm_dvfs_req)
			spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) |
						   (SPM_DVFS_REQ_LSB |
						    SPM_DVFS_FORCE_DOWN_LSB));
		else
			spm_write(SPM_SRC_REQ, (spm_read(SPM_SRC_REQ) |
						SPM_DVFS_FORCE_DOWN_LSB) &
						   (~SPM_DVFS_REQ_LSB));
		break;
	default:
		WARN_ON(1);
	}
	spm_update_rsv_6();
	last_fixed_opp = opp;

	/* check spm task done status for LPM */
	if (opp != OPPI_UNREQ) {
		r = _wait_spm_dvfs_complete(opp, timeout);
		if (r < 0) {
			spm_vcorefs_err("wait complete timeout(opp:%d)\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_warn(
			    "set_lpm_force_complete_timeout(opp:%d)\n", opp);
			__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	/* check vcore and ddr for LPM */
	if (opp != OPPI_UNREQ) {
		r = _check_dvfs_result(vcore, ddr, 1);
		if (r < 0) {
			spm_vcorefs_err("chk result fail opp:%d\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			WARN_ON(1);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return (r > 0) ? 0 : r;
}

u32 spm_vcorefs_get_MD_status(void) { return spm_read(MD2SPM_DVFS_CON); }

int spm_vcorefs_set_cpu_dvfs_req(u32 val, u32 mask)
{
	u32 value;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	value = (spm_read(CPU_DVFS_REQ) & ~mask) | (val & mask);
	spm_write(CPU_DVFS_REQ, value);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

int spm_msdc_dvfs_setting(int msdc, bool enable)
{
	unsigned long flags;
	int r = 0;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int timeout = SPM_DVFS_TIMEOUT;

	if (msdc != MSDC2_DVFS) {
		spm_vcorefs_err("invalid sdio msdc(%d)\n", msdc);
		return -1;
	}

	spin_lock_irqsave(&__spm_lock, flags);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("msdc pre-wait idle timeout\n");
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	pwrctrl->en_sdio_dvfs_setting = enable;
	spm_update_rsv_6();

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("msdc post-wait idle timeout\n");
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	spm_vcorefs_info(
	    "%s(msdc: %d, en: %d) SPM_SW_RSV_6=0x%x\n", __func__, msdc,
	    enable, spm_read(SPM_SW_RSV_6));
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

void spm_vcoefs_MD_LPM_req(bool enable)
{

	unsigned long flags;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	spin_lock_irqsave(&__spm_lock, flags);

	if (enable) {
		pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b = 1;
		pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b = 1;
		pwrctrl->disable_off_load_lpm = 0;
	} else {
		pwrctrl->md_srcclkena_0_2d_dvfs_req_mask_b = 0;
		pwrctrl->md_srcclkena_1_2d_dvfs_req_mask_b = 0;
		pwrctrl->disable_off_load_lpm = 1;
	}
	spm_update_rsv_6();
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_info("%s(%d) sw_rsv_6=0x%x\n", __func__, enable,
			 spm_read(SPM_SW_RSV_6));
}

void spm_vcorefs_off_load_lpm_req(bool enable)
{
	unsigned long flags;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	spin_lock_irqsave(&__spm_lock, flags);

	pwrctrl->disable_off_load_lpm = !enable;

	spm_update_rsv_6();
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_info("%s(%d) sw_rsv_6=0x%x\n", __func__,
			 enable, spm_read(SPM_SW_RSV_6));
}

void spm_vcorefs_emi_grouping_req(bool enable)
{

	unsigned long flags;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	spin_lock_irqsave(&__spm_lock, flags);

	if (enable)
		pwrctrl->en_emi_grouping = 1;
	else
		pwrctrl->en_emi_grouping = 0;

	spm_update_rsv_6();
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_info("%s(%d) sw_rsv_6=0x%x\n", __func__,
			 enable, spm_read(SPM_SW_RSV_6));
}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.3");
