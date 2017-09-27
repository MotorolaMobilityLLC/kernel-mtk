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

#include "mt_cpufreq.h"
#include "mt_vcore_dvfs.h"

#include "mt_spm_internal.h"

#define PER_OPP_DVS_US		600

#define VCORE_STA_HPM		(VCORE_STA_1)

#define get_vcore_sta()		(spm_read(SPM_SLEEP_DVFS_STA) & (VCORE_STA_1))

static struct pwr_ctrl vcore_dvfs_ctrl = {
	.md1_req_mask		= 0,
	.md2_req_mask		= 0,
	.conn_mask		= 0,
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

int spm_set_vcore_dvs_voltage(unsigned int opp)
{
	int r;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	switch (opp) {
	case OPP_0:
		spm_write(SPM_PCM_SRC_REQ, spm_read(SPM_PCM_SRC_REQ) | SR_PCM_F26M_REQ);
		r = wait_pcm_complete_dvs(get_vcore_sta() == VCORE_STA_HPM,
				3 * PER_OPP_DVS_US /* 1.15->1.05->1.15 */);
		break;
	case OPP_1:
		spm_write(SPM_PCM_SRC_REQ, spm_read(SPM_PCM_SRC_REQ) & ~SR_PCM_F26M_REQ);
		r = 0;		/* unrequest, no need to wait */
		break;
	default:
		spm_crit("[VcoreFS] *** FAILED: OPP INDEX IS INCORRECT ***\n");
		spin_unlock_irqrestore(&__spm_lock, flags);
		return -EINVAL;
	}

	if (r >= 0) {	/* DVS pass */
		r = 0;
	} else {
		spm_dump_vcore_dvs_regs(NULL);
		BUG();
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return r;
}

void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;
	unsigned long flags;

	if (dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_VCORE_DVFS].desc);
	else
		BUG();

	spin_lock_irqsave(&__spm_lock, flags);

	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);

	spin_unlock_irqrestore(&__spm_lock, flags);
}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.1");
