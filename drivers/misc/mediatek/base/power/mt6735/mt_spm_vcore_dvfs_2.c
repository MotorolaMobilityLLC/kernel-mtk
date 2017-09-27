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

#include "mt_vcore_dvfs.h"
#include "mt_cpufreq.h"
#include "mt_spm_internal.h"

#define PER_OPP_DVS_US		600

#define VCORE_STA_UHPM		(VCORE_STA_1 | VCORE_STA_0)
#define VCORE_STA_HPM		VCORE_STA_1
#define VCORE_STA_LPM		0

#define get_vcore_sta()		(spm_read(SPM_SLEEP_DVFS_STA) & (VCORE_STA_1 | VCORE_STA_0))


static struct pwr_ctrl vcore_dvfs_ctrl = {
	.md1_req_mask		= 0,
	.conn_mask		= 0,
	.md_vrf18_req_mask_b	= 0,

	/* mask to avoid affecting apsrc_state */
	.md2_req_mask		= 1,
	.ccif0_to_ap_mask	= 1,
	.ccif0_to_md_mask	= 1,
	.ccif1_to_ap_mask	= 1,
	.ccif1_to_md_mask	= 1,
	.ccifmd_md1_event_mask	= 1,
	.ccifmd_md2_event_mask	= 1,
	.gce_req_mask		= 1,
	.disp_req_mask		= 1,
	.mfg_req_mask		= 1,
};

struct spm_lp_scen __spm_vcore_dvfs = {
	.pwrctrl	= &vcore_dvfs_ctrl,
};

char *spm_dump_vcore_dvs_regs(char *p)
{
	if (p) {
		p += sprintf(p, "SLEEP_DVFS_STA: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA));
		p += sprintf(p, "PCM_SRC_REQ   : 0x%x\n", spm_read(SPM_PCM_SRC_REQ));
		p += sprintf(p, "PCM_REG13_DATA: 0x%x\n", spm_read(SPM_PCM_REG13_DATA));
		p += sprintf(p, "PCM_REG12_DATA: 0x%x\n", spm_read(SPM_PCM_REG12_DATA));
		p += sprintf(p, "PCM_REG12_MASK: 0x%x\n", spm_read(SPM_PCM_REG12_MASK));
		p += sprintf(p, "AP_STANBY_CON : 0x%x\n", spm_read(SPM_AP_STANBY_CON));
		p += sprintf(p, "PCM_IM_PTR    : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR), spm_read(SPM_PCM_IM_LEN));
		p += sprintf(p, "PCM_REG6_DATA : 0x%x\n", spm_read(SPM_PCM_REG6_DATA));
		p += sprintf(p, "PCM_REG11_DATA: 0x%x\n", spm_read(SPM_PCM_REG11_DATA));
	} else {
		spm_err("[VcoreFS] SLEEP_DVFS_STA: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA));
		spm_err("[VcoreFS] PCM_SRC_REQ   : 0x%x\n", spm_read(SPM_PCM_SRC_REQ));
		spm_err("[VcoreFS] PCM_REG13_DATA: 0x%x\n", spm_read(SPM_PCM_REG13_DATA));
		spm_err("[VcoreFS] PCM_REG12_DATA: 0x%x\n", spm_read(SPM_PCM_REG12_DATA));
		spm_err("[VcoreFS] PCM_REG12_MASK: 0x%x\n", spm_read(SPM_PCM_REG12_MASK));
		spm_err("[VcoreFS] AP_STANBY_CON : 0x%x\n", spm_read(SPM_AP_STANBY_CON));
		spm_err("[VcoreFS] PCM_IM_PTR    : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR), spm_read(SPM_PCM_IM_LEN));
		spm_err("[VcoreFS] PCM_REG6_DATA : 0x%x\n", spm_read(SPM_PCM_REG6_DATA));
		spm_err("[VcoreFS] PCM_REG11_DATA: 0x%x\n", spm_read(SPM_PCM_REG11_DATA));

		spm_err("[VcoreFS] PCM_REG0_DATA : 0x%x\n", spm_read(SPM_PCM_REG0_DATA));
		spm_err("[VcoreFS] PCM_REG1_DATA : 0x%x\n", spm_read(SPM_PCM_REG1_DATA));
		spm_err("[VcoreFS] PCM_REG2_DATA : 0x%x\n", spm_read(SPM_PCM_REG2_DATA));
		spm_err("[VcoreFS] PCM_REG3_DATA : 0x%x\n", spm_read(SPM_PCM_REG3_DATA));
		spm_err("[VcoreFS] PCM_REG4_DATA : 0x%x\n", spm_read(SPM_PCM_REG4_DATA));
		spm_err("[VcoreFS] PCM_REG5_DATA : 0x%x\n", spm_read(SPM_PCM_REG5_DATA));
		spm_err("[VcoreFS] PCM_REG7_DATA : 0x%x\n", spm_read(SPM_PCM_REG7_DATA));
		spm_err("[VcoreFS] PCM_REG8_DATA : 0x%x\n", spm_read(SPM_PCM_REG8_DATA));
		spm_err("[VcoreFS] PCM_REG9_DATA : 0x%x\n", spm_read(SPM_PCM_REG9_DATA));
		spm_err("[VcoreFS] PCM_REG10_DATA: 0x%x\n", spm_read(SPM_PCM_REG10_DATA));
		spm_err("[VcoreFS] PCM_REG14_DATA: 0x%x\n", spm_read(SPM_PCM_REG14_DATA));
		spm_err("[VcoreFS] PCM_REG15_DATA: %u\n"  , spm_read(SPM_PCM_REG15_DATA));
	}

	return p;
}

static void __go_to_vcore_dvfs(u32 spm_flags, u8 f26m_req, u8 apsrc_req)
{
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	if (dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].desc);
	else
		BUG();

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags /*| SPM_MD_VRF18_DIS*/);
	pwrctrl->pcm_f26m_req = f26m_req;
	pwrctrl->pcm_apsrc_req = apsrc_req;

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);
}

#define wait_pcm_complete_dvs(condition, timeout)	\
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

static bool is_fw_not_existed(void)
{
	return spm_read(SPM_PCM_REG1_DATA) != 0x1 || spm_read(SPM_PCM_REG11_DATA) == 0x55AA55AA;
}

static bool is_fw_not_support_uhpm(void)
{
	struct pcm_desc *pcmdesc;
	u32 pcm_ptr = spm_read(SPM_PCM_IM_PTR);
	u32 vcore_ptr;
	/* u32 sodi_ptr = base_va_to_pa(__spm_sodi.pcmdesc->base); */

	if (dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].desc);
	else
		return 1;

	if (pcmdesc->base_dma)
		vcore_ptr = pcmdesc->base_dma;
	else
		vcore_ptr = base_va_to_pa(pcmdesc->base);

	return pcm_ptr != vcore_ptr /*&& pcm_ptr != sodi_ptr*/;
}

int spm_set_vcore_dvs_voltage(unsigned int opp)
{
	u8 f26m_req, apsrc_req;
	u32 target_sta, req;
	int timeout, r = 0;
	bool not_existed, not_support;
	unsigned long flags;
	struct pcm_desc *pcmdesc;

	if (dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].desc);
	else
		return -EINVAL;

	if (opp == OPPI_PERF_ULTRA) {
		f26m_req = 1;
		apsrc_req = 1;
		target_sta = VCORE_STA_UHPM;
	} else if (opp == OPPI_PERF) {
		f26m_req = 1;
		apsrc_req = 0;
		target_sta = VCORE_STA_HPM;
	} else if (opp == OPPI_LOW_PWR) {
		f26m_req = 0;
		apsrc_req = 0;
		target_sta = VCORE_STA_LPM;
	} else {
		return -EINVAL;
	}

	spin_lock_irqsave(&__spm_lock, flags);
	not_existed = is_fw_not_existed();
	not_support = is_fw_not_support_uhpm();

	if (not_existed || (opp == OPPI_PERF_ULTRA && not_support)) {
		__go_to_vcore_dvfs(SPM_VCORE_DVFS_EN, f26m_req, apsrc_req);
	} else {
		req = spm_read(SPM_PCM_SRC_REQ) & ~(SR_PCM_F26M_REQ | SR_PCM_APSRC_REQ);
		spm_write(SPM_PCM_SRC_REQ, req | (f26m_req << 1) | apsrc_req);
	}

	if (opp < OPPI_LOW_PWR) {
		/* normal FW fetch time + 1.15->1.05->1.15->1.25V transition time */
		timeout = 2 * pcmdesc->size + 3 * PER_OPP_DVS_US;

		r = wait_pcm_complete_dvs(get_vcore_sta() == target_sta, timeout);

		if (r >= 0) {	/* DVS pass */
			r = 0;
		} else {
			spm_err("[VcoreFS] OPP: %u (%u)(%u)\n", opp, not_existed, not_support);
			spm_dump_vcore_dvs_regs(NULL);
			BUG();
		}
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return r;
}

void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

	__go_to_vcore_dvfs(spm_flags, 0, 0);

	spm_crit("[VcoreFS] STA: 0x%x, REQ: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA), spm_read(SPM_PCM_SRC_REQ));

	spin_unlock_irqrestore(&__spm_lock, flags);
}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.2");
