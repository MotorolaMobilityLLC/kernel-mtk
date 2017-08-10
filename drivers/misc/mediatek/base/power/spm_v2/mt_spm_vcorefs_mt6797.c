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
#include <linux/of_fdt.h>

#include <mt-plat/upmu_common.h>
#include <asm/setup.h>

#include "mt_vcorefs_governor.h"
#include "mt_spm_vcore_dvfs.h"
#include "mt_spm_pmic_wrap.h"
#include "mt_spm_internal.h"
#include "mt_spm_misc.h"

#define DYNAMIC_LOAD 1

#ifdef CONFIG_MTK_RAM_CONSOLE
#define VCOREFS_AEE_RR_REC 1
#else
#define VCOREFS_AEE_RR_REC 0
#endif

/* TIMEOUT */
#define SPM_DVFS_TIMEOUT	10000	/* 10ms */
#define SPM_DVFS_CON_TIMEOUT	2000	/* 2ms */

/* BW threshold for SPM_SW_RSV_4 */
#define HPM_THRES_OFFSET	16
#define LPM_THRES_OFFSET	24

/* get Vcore DVFS current state */
#define get_vcore_sta()		(spm_read(PCM_REG6_DATA) & SPM_VCORE_STA_REG)

/* get Vcore DVFS is progress */
#define is_dvfs_in_progress()	(spm_read(PCM_REG6_DATA) & SPM_FLAG_DVFS_ACTIVE)

/* get F/W screen on/off setting status */
#define get_screen_sta()	(spm_read(SPM_SW_RSV_1) & 0xF)

/* spm to pmic cmd done */
#define spm2pmic_cmd_done()	(spm_read(SPM_DVFS_CON) & (1 << 31))

enum spm_vcorefs_step {
	SPM_VCOREFS_ENTER = 0,
	SPM_VCOREFS_B1,
	SPM_VCOREFS_DVFS_START,
	SPM_VCOREFS_B3,
	SPM_VCOREFS_B4,
	SPM_VCOREFS_B5,
	SPM_VCOREFS_DVFS_END,
	SPM_VCOREFS_B7,
	SPM_VCOREFS_B8,
	SPM_VCOREFS_B9,
	SPM_VCOREFS_LEAVE,
};

/* FW is loaded by dyna_load_fw from binary */
static const u32 vcore_dvfs_binary[] = {
	0x55aa55aa, 0x10007c1f, 0xf0000000
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

static struct pwr_ctrl vcore_dvfs_ctrl = {
	/* default VCORE DVFS is disabled */
	.pcm_flags = (SPM_FLAG_DIS_VCORE_DVS | SPM_FLAG_DIS_VCORE_DFS),

	.wake_src = WAKE_SRC_R12_PCM_TIMER | WAKE_SRC_R12_APWDT_EVENT_B | WAKE_SRC_R12_MD32_SPM_IRQ_B,
	/* SPM general */
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,

	/* VCORE DVFS Logic pwr_ctrl */
	.dvfs_halt_mask_b = 0x07,		/* 5 bit, todo: enable for isp/disp, disable gce */
	.sdio_on_dvfs_req_mask_b = 0,

	.cpu_md_dvfs_erq_merge_mask_b = 1,	/* HPM request by MD */

	.md1_ddr_en_dvfs_halt_mask_b = 0,
	.md2_ddr_en_dvfs_halt_mask_b = 0,

	/* SPM_AP_STANDBY_CON */
	.conn_mask_b = 0,	/* bit[26] */
	.scp_req_mask_b = 0,	/* bit[21] */
	.md2_req_mask_b = 1,	/* bit[20] */
	.md1_req_mask_b = 1,	/* bit[19] */

	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	.vsync_dvfs_halt_mask_b = 0x0,		/* 5 bit */
	.cpu_md_emi_dvfs_req_prot_dis = 1,	/* todo: enable by MD if need check MD_SRCCLKEMA_0 */

	.spm_dvfs_req = 1,			/* set to 1 for keep high after fw loading */
	.spm_dvfs_force_down = 1,
	.cpu_md_dvfs_sop_force_on = 0,

	.emi_bw_dvfs_req_mask = 1,		/* Total BW default disable */
	.emi_boost_dvfs_req_mask_b = 0,		/* C+G BW default disable, enable by fliper */

	.dvfs_halt_src_chk = 1,

	/* +450 SPM_EMI_BW_MODE */
	/* [0]EMI_BW_MODE, [1]EMI_BOOST_MODE default is 0 */
};

struct spm_lp_scen __spm_vcore_dvfs = {
	.pcmdesc = &vcore_dvfs_pcm,
	.pwrctrl = &vcore_dvfs_ctrl,
};

static void set_aee_vcore_dvfs_status(int state)
{
#if VCOREFS_AEE_RR_REC
	u32 value = aee_rr_curr_vcore_dvfs_status();

	value &= ~(0xFF);
	value |= (state & 0xFF);
	aee_rr_rec_vcore_dvfs_status(value);
#endif
}

#define wait_spm_complete_by_condition(condition, timeout)	\
({							\
	int i = 0;					\
	while (!(condition)) {				\
		if (i >= (timeout)) {			\
			i = -EBUSY;			\
			break;				\
		}					\
		udelay(1);				\
		i++;					\
	}						\
	i;						\
})

void dump_pmic_info(void)
{
	u32 ret, reg_val;

	ret = pmic_read_interface_nolock(MT6351_WDTDBG_CON1, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]wdtdbg_con1=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON0, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_ctrl=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON4, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel=0x%x\n", reg_val);

	ret = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON5, &reg_val, 0xffff, 0);
	spm_notice("[PMIC]vcore vosel_on=0x%x\n", reg_val);
}

void spm_fw_version(void)
{
	u32 vcorefs_idx;
	struct pcm_desc *pcmdesc;

	vcorefs_idx = spm_get_pcm_vcorefs_index();
	pcmdesc = &(dyna_load_pcm[vcorefs_idx].desc);

	spm_vcorefs_info("VCOREFS_VER     : %s\n", pcmdesc->version);
}

char *spm_vcorefs_dump_dvfs_regs(char *p)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (p) {
		dump_pmic_info();
		p += sprintf(p, "SPM_SW_FLAG     : 0x%x\n", spm_read(SPM_SW_FLAG));
		p += sprintf(p, "MD2SPM_DVFS_CON : 0x%x\n", spm_read(MD2SPM_DVFS_CON));
		p += sprintf(p, "CPU_DVFS_REQ    : 0x%x\n", spm_read(CPU_DVFS_REQ));
		p += sprintf(p, "SPM_SRC_REQ     : 0x%x\n", spm_read(SPM_SRC_REQ));
		p += sprintf(p, "SPM_SRC2_MASK   : 0x%x\n", spm_read(SPM_SRC2_MASK));
		p += sprintf(p, "SPM_SW_RSV_1    : 0x%x\n", spm_read(SPM_SW_RSV_1));
		p += sprintf(p, "SPM_SW_RSV_3    : 0x%x\n", spm_read(SPM_SW_RSV_3));
		p += sprintf(p, "SPM_SW_RSV_4    : 0x%x\n", spm_read(SPM_SW_RSV_4));
		p += sprintf(p, "SPM_SW_RSV_5    : 0x%x\n", spm_read(SPM_SW_RSV_5));
		p += sprintf(p, "PCM_REG6_DATA   : 0x%x\n", spm_read(PCM_REG6_DATA));
		p += sprintf(p, "PCM_IM_PTR      : 0x%x (%u)\n", spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));
	} else {
		dump_pmic_info();
		spm_fw_version();
		spm_vcorefs_info("SPM_SW_FLAG     : 0x%x\n", spm_read(SPM_SW_FLAG));
		spm_vcorefs_info("SPM_SW_DEBUG    : 0x%x\n", spm_read(SPM_SW_DEBUG));
		spm_vcorefs_info("MD2SPM_DVFS_CON : 0x%x\n", spm_read(MD2SPM_DVFS_CON));
		spm_vcorefs_info("CPU_DVFS_REQ    : 0x%x\n", spm_read(CPU_DVFS_REQ));
		spm_vcorefs_info("SPM_SRC_REQ     : 0x%x\n", spm_read(SPM_SRC_REQ));
		spm_vcorefs_info("SPM_SRC_MASK    : 0x%x\n", spm_read(SPM_SRC_MASK));
		spm_vcorefs_info("SPM_SRC2_MASK   : 0x%x\n", spm_read(SPM_SRC2_MASK));
		spm_vcorefs_info("SPM_SCP_MAILBOX : 0x%x\n", spm_read(SPM_SCP_MAILBOX));
		spm_vcorefs_info("SPM_SCP_IRQ     : 0x%x\n", spm_read(SPM_SCP_IRQ));
		spm_vcorefs_info("SPM_SW_RSV_1    : 0x%x\n", spm_read(SPM_SW_RSV_1));
		spm_vcorefs_info("SPM_SW_RSV_3    : 0x%x\n", spm_read(SPM_SW_RSV_3));
		spm_vcorefs_info("SPM_SW_RSV_4    : 0x%x\n", spm_read(SPM_SW_RSV_4));
		spm_vcorefs_info("SPM_SW_RSV_5    : 0x%x\n", spm_read(SPM_SW_RSV_5));
		spm_vcorefs_info("PCM_REG_DATA_INI: 0x%x\n", spm_read(PCM_REG_DATA_INI));
		spm_vcorefs_info("PCM_REG0_DATA   : 0x%x\n", spm_read(PCM_REG0_DATA));
		spm_vcorefs_info("PCM_REG1_DATA   : 0x%x\n", spm_read(PCM_REG1_DATA));
		spm_vcorefs_info("PCM_REG2_DATA   : 0x%x\n", spm_read(PCM_REG2_DATA));
		spm_vcorefs_info("PCM_REG3_DATA   : 0x%x\n", spm_read(PCM_REG3_DATA));
		spm_vcorefs_info("PCM_REG4_DATA   : 0x%x\n", spm_read(PCM_REG4_DATA));
		spm_vcorefs_info("PCM_REG5_DATA   : 0x%x\n", spm_read(PCM_REG5_DATA));
		spm_vcorefs_info("PCM_REG6_DATA   : 0x%x\n", spm_read(PCM_REG6_DATA));
		spm_vcorefs_info("PCM_REG7_DATA   : 0x%x\n", spm_read(PCM_REG7_DATA));
		spm_vcorefs_info("PCM_REG8_DATA   : 0x%x\n", spm_read(PCM_REG8_DATA));
		spm_vcorefs_info("PCM_REG9_DATA   : 0x%x\n", spm_read(PCM_REG9_DATA));
		spm_vcorefs_info("PCM_REG10_DATA  : 0x%x\n", spm_read(PCM_REG10_DATA));
		spm_vcorefs_info("PCM_REG11_DATA  : 0x%x\n", spm_read(PCM_REG11_DATA));
		spm_vcorefs_info("PCM_REG12_DATA  : 0x%x\n", spm_read(PCM_REG12_DATA));
		spm_vcorefs_info("PCM_REG13_DATA  : 0x%x\n", spm_read(PCM_REG13_DATA));
		spm_vcorefs_info("PCM_REG14_DATA  : 0x%x\n", spm_read(PCM_REG14_DATA));
		spm_vcorefs_info("PCM_REG15_DATA  : %u\n"  , spm_read(PCM_REG15_DATA));
		spm_vcorefs_info("PCM_IM_PTR      : 0x%x (%u)\n", spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));

		__check_dvfs_halt_source(pwrctrl->dvfs_halt_src_chk);
	}

	return p;
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

/*
 * EMIBW
 */
void spm_vcorefs_enable_perform_bw(bool enable)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	if (enable) {
		pwrctrl->emi_boost_dvfs_req_mask_b = 1;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) | EMI_BOOST_DVFS_REQ_MASK_B_LSB);
	} else {
		pwrctrl->emi_boost_dvfs_req_mask_b = 0;
		spm_write(SPM_SRC2_MASK, spm_read(SPM_SRC2_MASK) & (~EMI_BOOST_DVFS_REQ_MASK_B_LSB));
	}

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("perform BW enable: %d, SPM_SRC2_MASK: 0x%x\n", enable, spm_read(SPM_SRC2_MASK));
}

int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold)
{
	u32 value;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	value = spm_read(SPM_SW_RSV_4) & (~(0xFF << HPM_THRES_OFFSET | 0xFF << LPM_THRES_OFFSET));
	spm_write(SPM_SW_RSV_4, value | (hpm_threshold << HPM_THRES_OFFSET) | (lpm_threshold << LPM_THRES_OFFSET));
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_crit("perform BW threshold, LPM: %u, HPM: %u, SPM_SW_RSV_4: 0x%x\n",
					lpm_threshold, hpm_threshold, spm_read(SPM_SW_RSV_4));
	return 0;
}

void spm_vcorefs_screen_on_setting(int dvfs_mode, u32 md_dvfs_req)
{
	int timer = 0;

	spm_write(SPM_SW_RSV_1, (spm_read(SPM_SW_RSV_1) & (~0xF)) | dvfs_mode);

	spm_write(SPM_CPU_WAKEUP_EVENT, 1);
	timer = wait_spm_complete_by_condition(get_screen_sta() == SPM_SCREEN_SETTING_DONE, SPM_DVFS_TIMEOUT);
	if (timer < 0) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		BUG();
	}
	spm_write(SPM_SW_RSV_1, (spm_read(SPM_SW_RSV_1) & ~(1 << 4)) | md_dvfs_req << 4);
	spm_write(SPM_CPU_WAKEUP_EVENT, 0);

	spm_write(SPM_SW_RSV_1, (spm_read(SPM_SW_RSV_1) & (~0xF)) | SPM_CLEAN_WAKE_EVENT_DONE);
}

void spm_mask_wakeup_event(int val)
{
	spm_write(SPM_SW_RSV_1, (spm_read(SPM_SW_RSV_1) & ~(1 << 5)) | (val << 5));
	spm_write(SPM_SW_RSV_1, (spm_read(SPM_SW_RSV_1) & ~(1 << 7)) | (val << 7));
}

/*
 * SPM DVFS Function
 */
int spm_set_vcore_dvfs(int opp, u32 md_dvfs_req, int kicker, int user_opp)
{
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;
	u8 dvfs_req;
	u32 target_sta, req;
	int t_dvfs = 0, t_progress = 0;

	if (!is_vcorefs_fw(DYNAMIC_LOAD)) {
		spm_vcorefs_crit("NOT VCORE F/W\n");
		return 0;
	}

	spin_lock_irqsave(&__spm_lock, flags);

	set_aee_vcore_dvfs_status(SPM_VCOREFS_ENTER);

	/* check DVFS idle */
	t_progress = wait_spm_complete_by_condition(!is_dvfs_in_progress(), SPM_DVFS_TIMEOUT);
	if (t_progress < 0) {
		spm_vcorefs_crit("WAIT DVFS IDLE TIMEOUT: %d, opp: %d\n", SPM_DVFS_TIMEOUT, opp);
		spm_vcorefs_dump_dvfs_regs(NULL);
		BUG();
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_START);

	switch (opp) {
	case OPPI_PERF:
		spm_vcorefs_screen_on_setting(SPM_DVFS_NORMAL, MASK_MD_DVFS_REQ);
		dvfs_req = 1;
		target_sta = SPM_SCREEN_ON_HPM;
		break;
	case OPPI_LOW_PWR:
		spm_vcorefs_screen_on_setting(SPM_DVFS_NORMAL, MASK_MD_DVFS_REQ);
		dvfs_req = 0;
		target_sta = SPM_SCREEN_ON_LPM;
		break;
	case OPPI_ULTRA_LOW_PWR:
		spm_vcorefs_screen_on_setting(SPM_DVFS_ULPM, md_dvfs_req);
		dvfs_req = 0;
		target_sta = SPM_SCREEN_ON_ULPM;
		break;
	default:
		return -EINVAL;
	}

	req = spm_read(SPM_SRC_REQ) & ~(SPM_DVFS_REQ_LSB);
	spm_write(SPM_SRC_REQ, req | (dvfs_req << 5));
	pwrctrl->spm_dvfs_req = dvfs_req;

	if (kicker == KIR_REESPI || kicker == KIR_TEESPI)
		udelay(410);

	set_aee_vcore_dvfs_status(SPM_VCOREFS_DVFS_END);

	if (opp == OPPI_PERF) {

		/* check DVFS timer */
		t_dvfs = wait_spm_complete_by_condition(get_vcore_sta() == target_sta, SPM_DVFS_TIMEOUT);
		if (t_dvfs < 0) {
			spm_vcorefs_crit("DVFS TIMEOUT: %d, req: 0x%x, target: 0x%x\n",
							SPM_DVFS_TIMEOUT, dvfs_req, target_sta);
			spm_vcorefs_dump_dvfs_regs(NULL);
			BUG();
		}

		if (kicker == KIR_REESPI || kicker == KIR_TEESPI) {
			if (user_opp == OPPI_PERF)
				spm_mask_wakeup_event(1);
			else
				spm_mask_wakeup_event(0);
		}

		spm_vcorefs_crit("req: 0x%x, target: 0x%x, t_dvfs: %d(%d)\n",
								dvfs_req, target_sta, t_dvfs, t_progress);
	} else {
		if (kicker == KIR_REESPI || kicker == KIR_TEESPI)
			spm_mask_wakeup_event(0);

		if (kicker == KIR_SCP)
			udelay(410);

		spm_vcorefs_crit("req: 0x%x, target: 0x%x\n", dvfs_req, target_sta);
	}

	set_aee_vcore_dvfs_status(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return t_dvfs;
}

static void _spm_vcorefs_init_reg(void)
{
	u32 mask;

	/* set en_spm2cksys_mem_ck_mux_update for SPM control */
	/* spm_write((spm_cksys_base + 0x40), (spm_read(spm_cksys_base + 0x40) | (0x1 << 13))); */

	/* SPM_EMI_BW_MODE[0]&[1] set to 0 */
	mask = (EMI_BW_MODE_LSB | EMI_BOOST_MODE_LSB);
	spm_write(SPM_EMI_BW_MODE, spm_read(SPM_EMI_BW_MODE) & ~(mask));
}

static void spm_vcorefs_spi_check(void)
{
	int retry = 0, timeout = 1000000;

	while (kicker_table[KIR_REESPI] == OPP_0 || kicker_table[KIR_TEESPI] == OPP_0) {
		if (retry > timeout)
			BUG();

		udelay(1);
		retry++;
	}
}

static void __go_to_vcore_dvfs(u32 spm_flags, u8 spm_data)
{
	unsigned long flags;
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl;

#if DYNAMIC_LOAD
	u32 vcorefs_idx = spm_get_pcm_vcorefs_index();

	if (dyna_load_pcm[vcorefs_idx].ready) {
		pcmdesc = &(dyna_load_pcm[vcorefs_idx].desc);
		pwrctrl = __spm_vcore_dvfs.pwrctrl;
	} else {
		spm_vcorefs_err("[%s] dyna load F/W fail\n", __func__);
		BUG();
	}
#else
	pcmdesc = __spm_vcore_dvfs.pcmdesc;
	pwrctrl = __spm_vcore_dvfs.pwrctrl;
#endif

	if (!is_vcorefs_fw(DYNAMIC_LOAD))
		spm_vcorefs_spi_check();

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);

	spin_lock_irqsave(&__spm_lock, flags);

	_spm_vcorefs_init_reg();

	__spm_clean_after_wakeup();

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);

	spin_unlock_irqrestore(&__spm_lock, flags);

#if SPM_AEE_RR_REC
	aee_rr_rec_spm_common_scenario_val(SPM_COMMON_SCENARIO_SODI);
#endif
}

void spm_vcorefs_init_dvfs_con(void)
{
	int timeout;

	/* spm to pmic cmd done */
	spm_write(SPM_DVFS_CON, 0x0);
	timeout = wait_spm_complete_by_condition(spm2pmic_cmd_done() == PMIC_ACK_DONE, SPM_DVFS_CON_TIMEOUT);
	if (timeout < 0) {
		spm_vcorefs_crit("DVFS_CON TIMEOUT: %d\n", SPM_DVFS_CON_TIMEOUT);
		spm_vcorefs_dump_dvfs_regs(NULL);
		BUG();
	}
}

void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	__go_to_vcore_dvfs(spm_flags, 0);

}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.1");
