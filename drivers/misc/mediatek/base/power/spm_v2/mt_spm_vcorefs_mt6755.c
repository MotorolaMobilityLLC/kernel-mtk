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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include "mt_spm_vcore_dvfs.h"
#include "mt_vcorefs_governor.h"

#include "upmu_common.h"

#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <mach/mt_pmic_wrap.h>

#include "mt_spm_internal.h"
#include "mt_spm_pmic_wrap.h"

/*
 * Define and Declare
 */
#define SPM_DVFS_TIMEOUT 2000
#define SPM_DVFS_HPM		1
#define SPM_DVFS_LPM		0

#define SPM_CHK_GUARD_TIME	10
#define SPM_HPM_HOLD_TIME 1000
/* PCM_REG6_DATA */
#define SPM_FLAG_DVFS_ACTIVE (1<<23)	/* bit 23 */
#define SPM_FLAG_DVFS_STATE (1<<24)
/* bw threshold  for SPM_SW_RSV_5, SPM_SW_RSV_4 */
#define HPM_THRES_OFFSET	16
#define LPM_THRES_OFFSET	24


/* r6[31] = 1 for DVFS UP, 0 for DVFS DOWN */
#define is_dvfs_in_progress() (spm_read(PCM_REG6_DATA) & SPM_FLAG_DVFS_ACTIVE)

#ifdef CONFIG_MTK_RAM_CONSOLE
#define VCOREFS_AEE_RR_REC 1
#else
#define VCOREFS_AEE_RR_REC 0
#endif

#if VCOREFS_AEE_RR_REC
enum spm_vcorefs_step {
	SPM_VCOREFS_ENTER = 0,
	SPM_VCOREFS_DVFS_START,
	SPM_VCOREFS_DVFS_END,
	SPM_VCOREFS_LEAVE,
};

void set_aee_vcore_dvfs_status(int state)
{
	u32 value = aee_rr_curr_vcore_dvfs_status();

	value &= ~(0x000000FF);
	value |= (state & 0x000000FF);
	aee_rr_rec_vcore_dvfs_status(value);
}
#else
void set_aee_vcore_dvfs_status(int state)
{
	/* nothing */
}
#endif

/* FW is loaded by dyna_load_fw from binary */
#if 0
static const u32 vcore_dvfs_binary[] = {
};

static struct pcm_desc vcore_dvfs_pcm = {
	.version = "pcm_vcore_dvfs_v0.1.2_20150323",
	.base = vcore_dvfs_binary,
	.size = 295,
	.sess = 1,
	.replace = 1,
	.vec0 = EVENT_VEC(23, 1, 0, 78),	/* FUNC_MD_VRF18_WAKEUP */
	.vec1 = EVENT_VEC(28, 1, 0, 101),	/* FUNC_MD_VRF18_SLEEP */
	.vec2 = EVENT_VEC(11, 1, 0, 124),	/* FUNC_VCORE_HIGH */
	.vec3 = EVENT_VEC(12, 1, 0, 159),	/* FUNC_VCORE_LOW */
};
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
	.dvfs_halt_mask_b = 0x07,	/* 5 bit *//* todo: enable for isp/disp, disable gce */
	.sdio_on_dvfs_req_mask_b = 0,

	.cpu_md_dvfs_erq_merge_mask_b = 1,	/* HPM request by WFD/MHL/MD */

	.md1_ddr_en_dvfs_halt_mask_b = 0,
	.md2_ddr_en_dvfs_halt_mask_b = 0,

	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	.vsync_dvfs_halt_mask_b = 0x0,	/* 5 bit */
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 1,	/* todo: enable by MD if need check MD_SRCCLKEMA_0 */

	.spm_dvfs_req = 1,	/* set to 1 for keep high after fw loading */
	.spm_dvfs_force_down = 1,
	.cpu_md_dvfs_sop_force_on = 0,

	.emi_bw_dvfs_req_mask = 1,	/* default disable, enable by fliper */
	.emi_boost_dvfs_req_mask_b = 0,	/* default disable, enable by fliper */

	.dvfs_halt_src_chk = 1,

	/* +450 SPM_EMI_BW_MODE */
	/* [0]EMI_BW_MODE, [1]EMI_BOOST_MODE default is 0 */

};

struct spm_lp_scen __spm_vcore_dvfs = {
	/*.pcmdesc = &vcore_dvfs_pcm, */
	.pwrctrl = &vcore_dvfs_ctrl,
};

/*
 * Macro and Inline
 */
inline int _wait_spm_dvfs_idle(int timeout)
{
	int i = 0;
	u32 val = spm_read(PCM_REG6_DATA);

	while (!((val & SPM_FLAG_DVFS_ACTIVE) == 0)) {
		if (i >= timeout) {
			i = -EBUSY;
			break;
		}
		udelay(1);
		val = spm_read(PCM_REG6_DATA);
		i++;
	}
	return i;
}


static inline int _wait_spm_dvfs_complete(int opp, int timeout)
{
	int i = 0;
	int target = (opp == OPPI_PERF) ? SPM_DVFS_HPM : SPM_DVFS_LPM;
	u32 val;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	bool partial = ((pwrctrl->pcm_flags & SPM_FLAG_DIS_VCORE_DFS) != 0);

	/* wait 10 usec before check status */
	udelay(SPM_CHK_GUARD_TIME);

	val = spm_read(PCM_REG6_DATA);
	if (partial == true) {
		udelay(SPM_CHK_GUARD_TIME*10);
		spm_vcorefs_info("hh: skip chk idle instead of comp (flags=0x%x) udelay(%d)\n",
				 pwrctrl->pcm_flags, SPM_CHK_GUARD_TIME*10);
		while (!((val & SPM_FLAG_DVFS_ACTIVE) == 0)) {
			if (i >= timeout) {
				i = -EBUSY;
				break;
			}
			udelay(1);
			val = spm_read(PCM_REG6_DATA);
			i++;
		}
	} else {
		while (!(((val & SPM_FLAG_DVFS_ACTIVE) == 0)
			 && ((val & SPM_FLAG_DVFS_STATE) == (target << 24)))) {
			if (i >= timeout) {
				i = -EBUSY;
				break;
			}
			udelay(1);
			val = spm_read(PCM_REG6_DATA);
			i++;
		}
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
	bool is_cpu_md_dvfs_sop_force_on = (val & CPU_MD_DVFS_SOP_FORCE_ON_LSB) ? true : false;
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
	u32 ret, reg_val;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_read_interface_nolock(MT6353_WDTDBG_CON1, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]wdtdbg_con1=0x%x\n", reg_val);
	ret = pmic_read_interface_nolock(MT6353_BUCK_VCORE_HWM_CON0, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_ctrl=0x%x\n", reg_val);
	ret = pmic_read_interface_nolock(MT6353_BUCK_VCORE_VOL_CON1, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel=0x%x\n", reg_val);
	ret = pmic_read_interface_nolock(MT6353_BUCK_VCORE_VOL_CON2, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_on=0x%x\n", reg_val);
#else
	ret = pmic_read_interface_nolock(MT6351_WDTDBG_CON1, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]wdtdbg_con1=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON0, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_ctrl=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON4, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON5, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_on=0x%x\n", reg_val);
#endif
}

static int _check_dvfs_result(int vcore, int ddr)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int vcore_val = 0;
	int ddr_val = 0;
	int r = 0;

	bool dvs_en = ((pwrctrl->pcm_flags & SPM_FLAG_DIS_VCORE_DVS) == 0);
	bool dfs_en = ((pwrctrl->pcm_flags & SPM_FLAG_DIS_VCORE_DFS) == 0);

	if (dvs_en == true) {
		vcore_val = vcorefs_get_curr_vcore();
		if (vcore_val != vcore) {
			spm_vcorefs_err("VCORE FAIL: %d(%d)\n", vcore_val, vcore);
			spm_vcorefs_dump_dvfs_regs(NULL);
			pwrap_dump_all_register(); /* pwrap debug msg */
			dump_pmic_info();
			spm_vcorefs_aee_exception("VCORE FAIL: %d(%d)\n",
							vcore_val, vcorefs_get_curr_vcore());
			r = -1;
		}
	}

	if (dfs_en == true) {
		ddr_val = vcorefs_get_curr_ddr();
		if (ddr_val != ddr) {
			spm_vcorefs_err("DDR FAIL: %d(%d)\n", ddr_val, ddr);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_exception("DDR FAIL: %d(%d)\n",
							ddr_val, vcorefs_get_curr_ddr());
			r = -1;
		}
	}
	return r;
}


static void __go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	struct pcm_desc *pcmdesc = __spm_sodi.pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	u32 sodi_idx;

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353) && defined(CONFIG_ARCH_MT6755)
	if (use_new_spmfw)
		sodi_idx = DYNA_LOAD_PCM_SODI_R;
	else
		sodi_idx = DYNA_LOAD_PCM_SODI;
#else
	sodi_idx = DYNA_LOAD_PCM_SODI;
#endif

	if (!dyna_load_pcm[sodi_idx].ready) {
		spm_vcorefs_err("LOAD FIRMWARE FAIL\n");
		BUG();
	}
	pcmdesc = &(dyna_load_pcm[sodi_idx].desc);

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);
}


void _spm_vcorefs_init_reg(void)
{
	u32 mask;

	/* set en_spm2cksys_mem_ck_mux_update for SPM control */
	spm_write((spm_cksys_base + 0x40), (spm_read(spm_cksys_base + 0x40) | (0x1 << 13)));

	/* SPM_EMI_BW_MODE[0]&[1] set to 0 */
	mask = (EMI_BW_MODE_LSB | EMI_BOOST_MODE_LSB);
	spm_write(SPM_EMI_BW_MODE, spm_read(SPM_EMI_BW_MODE) & ~(mask));

	/* Set dummy read address */
	/* This task is done in spm_module_init */

	/* SPM_SW_RSV5[0] init is moved to caller, only need first time */

	/* other setting is set by __spm_set_power_control */

	spm_vcorefs_info("EMI_BW_MODE=0x%8x\n", spm_read(SPM_EMI_BW_MODE));
	spm_vcorefs_info("SPM_SW_RSV_4=0x%8x\n", spm_read(SPM_SW_RSV_4));
	spm_vcorefs_info("SPM_SW_RSV_5=0x%8x\n", spm_read(SPM_SW_RSV_5));
	spm_vcorefs_info("SPM_PASR_DPD_1=0x%8x\n", spm_read(SPM_PASR_DPD_1));
	spm_vcorefs_info("SPM_PASR_DPD_2=0x%8x\n", spm_read(SPM_PASR_DPD_2));
	spm_vcorefs_info("SPM_SRC2_MASK=0x%8x\n", spm_read(SPM_SW_RSV_4));
	spm_vcorefs_info("SPM_SRC_MASK=0x%8x\n", spm_read(SPM_SW_RSV_5));
	spm_vcorefs_info("CLK_CFG_0=0x%8x\n", spm_read(spm_cksys_base + 0x40));
}

/*
 * External Function
 */
char *spm_vcorefs_dump_dvfs_regs(char *p)
{
	if (p) {
		p += sprintf(p, "MD2SPM_DVFS_CON   : 0x%x\n", spm_read(MD2SPM_DVFS_CON));
		p += sprintf(p, "CPU_DVFS_REQ: 0x%x\n", spm_read(CPU_DVFS_REQ));
		p += sprintf(p, "SPM_SRC_REQ: 0x%x\n", spm_read(SPM_SRC_REQ));
		p += sprintf(p, "SPM_SRC_MASK: 0x%x\n", spm_read(SPM_SRC_MASK));
		p += sprintf(p, "SPM_SRC2_MASK : 0x%x\n", spm_read(SPM_SRC2_MASK));
		p += sprintf(p, "SPM_SW_RSV_4 : 0x%x\n", spm_read(SPM_SW_RSV_4));
		p += sprintf(p, "SPM_SW_RSV_5 : 0x%x\n", spm_read(SPM_SW_RSV_5));
		p += sprintf(p, "PCM_IM_PTR    : 0x%x (%u)\n", spm_read(PCM_IM_PTR),
			     spm_read(PCM_IM_LEN));
		p += sprintf(p, "PCM_REG6_DATA : 0x%x\n", spm_read(PCM_REG6_DATA));
		p += sprintf(p, "PCM_REG15_DATA : 0x%x\n", spm_read(PCM_REG15_DATA));
	} else {
		dump_pmic_info();
		spm_vcorefs_info("MD2SPM_DVFS_CON   : 0x%x\n", spm_read(MD2SPM_DVFS_CON));
		spm_vcorefs_info("CPU_DVFS_REQ: 0x%x\n", spm_read(CPU_DVFS_REQ));
		spm_vcorefs_info("SPM_SRC_REQ: 0x%x\n", spm_read(SPM_SRC_REQ));
		spm_vcorefs_info("SPM_SRC_MASK: 0x%x\n", spm_read(SPM_SRC_MASK));
		spm_vcorefs_info("SPM_SRC2_MASK: 0x%x\n", spm_read(SPM_SRC2_MASK));
		spm_vcorefs_info("SPM_SW_RSV_4  : 0x%x\n", spm_read(SPM_SW_RSV_4));
		spm_vcorefs_info("SPM_SW_RSV_5  : 0x%x\n", spm_read(SPM_SW_RSV_5));
		spm_vcorefs_info("PCM_IM_PTR    : 0x%x (%u)\n", spm_read(PCM_IM_PTR),
				 spm_read(PCM_IM_LEN));
		spm_vcorefs_info("PCM_REG6_DATA : 0x%x\n", spm_read(PCM_REG6_DATA));
		spm_vcorefs_info("PCM_REG0_DATA : 0x%x\n", spm_read(PCM_REG0_DATA));
		spm_vcorefs_info("PCM_REG1_DATA : 0x%x\n", spm_read(PCM_REG1_DATA));
		spm_vcorefs_info("PCM_REG2_DATA : 0x%x\n", spm_read(PCM_REG2_DATA));
		spm_vcorefs_info("PCM_REG3_DATA : 0x%x\n", spm_read(PCM_REG3_DATA));
		spm_vcorefs_info("PCM_REG4_DATA : 0x%x\n", spm_read(PCM_REG4_DATA));
		spm_vcorefs_info("PCM_REG5_DATA : 0x%x\n", spm_read(PCM_REG5_DATA));
		spm_vcorefs_info("PCM_REG7_DATA : 0x%x\n", spm_read(PCM_REG7_DATA));
		spm_vcorefs_info("PCM_REG8_DATA : 0x%x\n", spm_read(PCM_REG8_DATA));
		spm_vcorefs_info("PCM_REG9_DATA : 0x%x\n", spm_read(PCM_REG9_DATA));
		spm_vcorefs_info("PCM_REG10_DATA: 0x%x\n", spm_read(PCM_REG10_DATA));
		spm_vcorefs_info("PCM_REG11_DATA: 0x%x\n", spm_read(PCM_REG11_DATA));
		spm_vcorefs_info("PCM_REG12_DATA: 0x%x\n", spm_read(PCM_REG12_DATA));
		spm_vcorefs_info("PCM_REG13_DATA: 0x%x\n", spm_read(PCM_REG13_DATA));
		spm_vcorefs_info("PCM_REG14_DATA: 0x%x\n", spm_read(PCM_REG14_DATA));
		spm_vcorefs_info("PCM_REG15_DATA: %u\n", spm_read(PCM_REG15_DATA));
		spm_vcorefs_info("PCM_REG12_MASK_B_STA: 0x%x\n", spm_read(PCM_REG12_MASK_B_STA));
		spm_vcorefs_info("PCM_REG12_EXT_DATA: 0x%x\n", spm_read(PCM_REG12_EXT_DATA));
		spm_vcorefs_info("PCM_REG12_EXT_MASK_B_STA: 0x%x\n", spm_read(PCM_REG12_EXT_MASK_B_STA));
	}

	return p;
}

int spm_vcorefs_get_clk_mem_pll(void)
{
	int r;
/*
    -1 : dvfs in progress
    0  : MEMPLL1PLL
    1  : MEMPLL3PLL
*/
	if ((spm_read(PCM_REG6_DATA) & SPM_FLAG_DVFS_ACTIVE))
		r = -1;
	else if ((spm_read(SPM_SW_RSV_5) & 0x1))
		r = 1;
	else
		r = 0;

	return r;
}

/* first time kick SPM FW for vcore dvfs */
void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write((spm_cksys_base + 0x40), (spm_read(spm_cksys_base + 0x40) | (0x1 << 13)));

	_spm_vcorefs_init_reg();

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);

	__go_to_vcore_dvfs(spm_flags, spm_data);

	spm_vcorefs_dump_dvfs_regs(NULL);

	spin_unlock_irqrestore(&__spm_lock, flags);
}

int spm_vcorefs_set_total_bw_threshold(u32 lpm_threshold, u32 hpm_threshold)
{
	u32 value;
	unsigned long flags;

	if (((hpm_threshold & (~0xFF)) != 0) || ((lpm_threshold & (~0xFF)) != 0)) {
		spm_vcorefs_err("total bw threshold out-of-range !\n");
		spm_vcorefs_err("hpm:%d lpm:%d\n", hpm_threshold, lpm_threshold);
		return -1;
	}

	spin_lock_irqsave(&__spm_lock, flags);

	value = spm_read(SPM_SW_RSV_5) & (~(0xFF << HPM_THRES_OFFSET | 0xFF << LPM_THRES_OFFSET));
	spm_write(SPM_SW_RSV_5,
		  value | (hpm_threshold << HPM_THRES_OFFSET) | (lpm_threshold <<
								 LPM_THRES_OFFSET));

	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold)
{
	u32 value;
	unsigned long flags;

	if (((hpm_threshold & (~0x7F)) != 0) || ((lpm_threshold & (~0x7F)) != 0)) {
		spm_vcorefs_err("perform bw threshold out-of-range !\n");
		spm_vcorefs_err("hpm:%d lpm:%d\n", hpm_threshold, lpm_threshold);
		return -1;
	}
	spin_lock_irqsave(&__spm_lock, flags);

	value = spm_read(SPM_SW_RSV_4) & (~(0xFF << HPM_THRES_OFFSET | 0xFF << LPM_THRES_OFFSET));
	spm_write(SPM_SW_RSV_4,
		  value | (hpm_threshold << HPM_THRES_OFFSET) | (lpm_threshold <<
								 LPM_THRES_OFFSET));

	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

void spm_vcorefs_enable_total_bw(bool enable)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_bw_dvfs_req_mask = 0;
		spm_write(SPM_SRC_MASK, spm_read(SPM_SRC_MASK) & (~EMI_BW_DVFS_REQ_MASK_LSB));
	} else {
		pwrctrl->emi_bw_dvfs_req_mask = 1;
		spm_write(SPM_SRC_MASK, spm_read(SPM_SRC_MASK) | EMI_BW_DVFS_REQ_MASK_LSB);
	}

	spin_unlock_irqrestore(&__spm_lock, flags);
}

void spm_vcorefs_enable_perform_bw(bool enable)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	if (enable == true) {
		pwrctrl->emi_boost_dvfs_req_mask_b = 1;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) | EMI_BOOST_DVFS_REQ_MASK_B_LSB);
	} else {
		pwrctrl->emi_boost_dvfs_req_mask_b = 0;
		spm_write(SPM_SRC2_MASK,
			  spm_read(SPM_SRC2_MASK) & (~EMI_BOOST_DVFS_REQ_MASK_B_LSB));
	}

	spin_unlock_irqrestore(&__spm_lock, flags);
}


int spm_vcorefs_set_dvfs_hpm_force(int opp, int vcore, int ddr)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int r = 0;
	unsigned long flags;
	int timeout = SPM_DVFS_TIMEOUT;

	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	spin_lock_irqsave(&__spm_lock, flags);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("wait idle timeout(opp:%d)\n", opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);

	/* MAIN: "DVFS_FORCE_ON" */
	if (opp == OPPI_PERF) {
		pwrctrl->spm_dvfs_force_down = 1;
		pwrctrl->spm_dvfs_req = 1;
		spm_write(SPM_SRC_REQ,
			  spm_read(SPM_SRC_REQ) | (SPM_DVFS_FORCE_DOWN_LSB | SPM_DVFS_REQ_LSB));
	} else {
		pwrctrl->spm_dvfs_req = 0;
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) & (~SPM_DVFS_REQ_LSB));
	}

	/* check spm task done status for HPM */
	if (opp == OPPI_PERF) {
		r = _wait_spm_dvfs_complete(opp, timeout);
		if (r < 0) {
			spm_vcorefs_err("wait complete timeout(opp:%d)\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_warn("set_hpm_force_complete_timeout(opp:%d)\n", opp);
			__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	/* check vcore and ddr for HPM */
	if (opp == OPPI_PERF)
		r = _check_dvfs_result(vcore, ddr);

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return (r > 0) ? 0 : r;
}

int spm_vcorefs_set_dvfs_hpm(int opp, int vcore, int ddr)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int r = 0;
	unsigned long flags;
	int timeout = SPM_DVFS_TIMEOUT;

	bool is_total_bw_enabled = false;

	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	if (opp == OPPI_PERF) {
		/* disable total bw for dvfs result check */
		is_total_bw_enabled = _get_total_bw_enable();
		spm_vcorefs_enable_total_bw(false);
	}

	spin_lock_irqsave(&__spm_lock, flags);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("wait idle timeout(opp:%d)\n", opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);

	/* MAIN: "CPU_MD_DVFS_SOP_FORCE_ON" */
	if (opp == OPPI_PERF) {
		pwrctrl->cpu_md_dvfs_sop_force_on = 1;
		pwrctrl->cpu_md_emi_dvfs_req_prot_dis = 1;
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) | CPU_MD_DVFS_SOP_FORCE_ON_LSB);
		spm_write(SPM_SRC2_MASK,
			  spm_read(SPM_SRC2_MASK) | CPU_MD_EMI_DVFS_REQ_PROT_DIS_LSB);
	} else {
		pwrctrl->cpu_md_dvfs_sop_force_on = 0;
		pwrctrl->cpu_md_emi_dvfs_req_prot_dis = 0;
		spm_write(SPM_SRC2_MASK,
			  spm_read(SPM_SRC2_MASK) & (~CPU_MD_EMI_DVFS_REQ_PROT_DIS_LSB));
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) & (~CPU_MD_DVFS_SOP_FORCE_ON_LSB));
	}

	/* check spm task done status for HPM */
	if (opp == OPPI_PERF) {
		r = _wait_spm_dvfs_complete(opp, timeout);
		if (r < 0) {
			spm_vcorefs_err("wait complete timeout(opp:%d)\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_warn("set_hpm_complete_timeout(opp:%d)\n", opp);
			__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	/* check vcore and ddr for HPM */
	if (opp == OPPI_PERF)
		r = _check_dvfs_result(vcore, ddr);

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	if (opp == OPPI_PERF) {
		/* keep hpm at least 1ms */
		udelay(SPM_HPM_HOLD_TIME);
		/* restore to orignal total bw setting */
		spm_vcorefs_enable_total_bw(is_total_bw_enabled);
	}

	return (r > 0) ? 0 : r;
}

int spm_vcorefs_set_dvfs_lpm_force(int opp, int vcore, int ddr)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	int r = 0;
	unsigned long flags;
	int timeout = SPM_DVFS_TIMEOUT;

	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	spin_lock_irqsave(&__spm_lock, flags);

	r = _wait_spm_dvfs_idle(timeout);
	if (r < 0) {
		spm_vcorefs_err("wait idle timeout(opp:%d)\n", opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);

	/* spm setting for "DVFS_FORCE_DOWN" */
	if (opp == OPPI_LOW_PWR) {
		pwrctrl->spm_dvfs_force_down = 0;
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) & (~SPM_DVFS_FORCE_DOWN_LSB));
	} else {
		pwrctrl->spm_dvfs_force_down = 1;
		spm_write(SPM_SRC_REQ, spm_read(SPM_SRC_REQ) | (SPM_DVFS_FORCE_DOWN_LSB));
	}

	/* check spm task done status for LPM */
	if (opp == OPPI_LOW_PWR) {
		r = _wait_spm_dvfs_complete(opp, timeout);
		if (r < 0) {
			spm_vcorefs_err("wait complete timeout(opp:%d)\n", opp);
			spm_vcorefs_dump_dvfs_regs(NULL);
			spm_vcorefs_aee_warn("set_lpm_force_complete_timeout(opp:%d)\n", opp);
			__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
		}
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	/* check vcore and ddr for LPM */
	if (opp == OPPI_LOW_PWR)
		r = _check_dvfs_result(vcore, ddr);

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return r;
}

int spm_vcorefs_set_total_bw(int opp, int vcore, int ddr)
{
	/* HPM means to disable total bw,
	   avoid trigger LPM event */

	if (opp == OPPI_LOW_PWR)
		spm_vcorefs_enable_total_bw(true);
	else
		spm_vcorefs_enable_total_bw(false);

	return 0;
}

u32 spm_vcorefs_get_MD_status(void)
{
	return spm_read(MD2SPM_DVFS_CON);
}

int spm_vcorefs_set_cpu_dvfs_req(u32 val, u32 mask)
{
	u32 value = (spm_read(CPU_DVFS_REQ) & ~mask) | (val & mask);

	spm_write(CPU_DVFS_REQ, value);
	return 0;
}

bool spm_vcorefs_is_dvfs_in_porgress(void)
{
	if (is_dvfs_in_progress() != 0)
		return true;
	else
		return false;
}

void spm_vcorefs_set_pcm_flag(u32 flag, bool set)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (set == true)
		pwrctrl->pcm_flags |= flag;
	else
		pwrctrl->pcm_flags &= (~flag);

	spm_write(SPM_SW_FLAG, pwrctrl->pcm_flags);
}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.2");
