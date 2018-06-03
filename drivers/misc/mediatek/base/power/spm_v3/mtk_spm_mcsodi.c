/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
/* FIXME: Need to add MCSODI_ARGS SMC call */
#if 0
#include <mt-plat/mtk_secure_api.h>
#endif

#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_mcsodi.h>
#include <mtk_idle_profile.h>

#define MCSODI_LOGOUT_MAX_INTERVAL   (5000U)	/* unit:ms */

static struct pwr_ctrl mcsodi_ctrl;

struct spm_lp_scen __spm_mcsodi = {
	.pwrctrl = &mcsodi_ctrl,
	/* .pcmdesc = &mcsodi_pcm, */
};

static DEFINE_SPINLOCK(mcsodi_busy_spin_lock);
static bool mcsodi_start;

bool spm_mcsodi_start(void)
{
	return mcsodi_start;
}

static void spm_output_log(struct wake_status *wakesta)
{
	unsigned long int curr_time = 0;
	static unsigned long int pre_time;
	static unsigned int logout_mcsodi_cnt;

	curr_time = spm_get_current_time_ms();

	if ((curr_time - pre_time > MCSODI_LOGOUT_MAX_INTERVAL)) {
		ms_warn("sw_flag = 0x%x, %d, timer_out= %u r13 = 0x%x, debug_flag = 0x%x, r12 = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
					spm_read(SPM_SW_FLAG), logout_mcsodi_cnt,
					wakesta->timer_out, wakesta->r13, wakesta->debug_flag,
					wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
					wakesta->event_reg, wakesta->isr);
		pre_time = curr_time;
		logout_mcsodi_cnt = 0;
	} else {
		logout_mcsodi_cnt++;
	}
}

static void spm_mcsodi_wfi(void)
{
	isb();
	mb();	/* memory barrier */
	__asm__ __volatile__("wfi" : : : "memory");
}

static void spm_trigger_wfi_for_mcsodi(u32 pcm_flags)
{
/* TODO: Need to check ATF CPU legacy flow */
#if 0
	if (mtk_idle_cpu_wfi_criteria()) {
		int spm_dormant_sta;

		spm_dormant_sta = mtk_enter_idle_state(MTK_MCSODI_MODE);
		if (spm_dormant_sta < 0)
			ms_err("SMC ret(%d)\n", spm_dormant_sta);
	} else {
		spm_mcsodi_wfi();
	}
#else
	spm_mcsodi_wfi();
#endif
}

static void spm_mcsodi_pcm_setup_before_wfi(
	u32 cpu, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
/* FIXME: Need to add MCSODI_ARGS SMC call */
#if 0
	unsigned int resource_usage;

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();

	mt_secure_call(MTK_SIP_KERNEL_SPM_MCSODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_MCSODI, PWR_OPP_LEVEL, pwrctrl->opp_level);
#endif
}

static void spm_mcsodi_pcm_setup_after_wfi(u32 operation_cond)
{
}

void spm_go_to_mcsodi(u32 spm_flags, u32 cpu, u32 mcsodi_flags, u32 operation_cond)
{
	struct pwr_ctrl *pwrctrl = __spm_mcsodi.pwrctrl;
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_busy_spin_lock, flags);

	if (!mtk_idle_cpu_wfi_criteria())
		goto mcsodi_abort;

	mcsodi_start = true;
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	mcsodi_before_wfi(cpu);

	spm_mcsodi_footprint(SPM_MCSODI_ENTER);

#if defined(CONFIG_MTK_SERIAL)
	if (request_uart_to_sleep())
		goto mcsodi_abort;
#endif

	spm_mcsodi_footprint(SPM_MCSODI_SPM_FLOW);

	spm_mcsodi_footprint_val((1 << SPM_MCSODI_ENTER_WFI) |
				(1 << SPM_MCSODI_B2) | (1 << SPM_MCSODI_B3) |
				(1 << SPM_MCSODI_B4) | (1 << SPM_MCSODI_B5) |
				(1 << SPM_MCSODI_B6));

	spm_mcsodi_pcm_setup_before_wfi(cpu, pwrctrl, operation_cond);

	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);

	spm_trigger_wfi_for_mcsodi(pwrctrl->pcm_flags);

	return;

mcsodi_abort:
	spm_mcsodi_reset_footprint();
	mcsodi_start = false;
	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);
}

int spm_leave_mcsodi(u32 cpu, u32 operation_cond)
{
	struct wake_status wakesta;
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_busy_spin_lock, flags);

	if (mcsodi_start == false) {
		spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);
		return -1;
	}

	spm_mcsodi_footprint(SPM_MCSODI_LEAVE_WFI);
#if defined(CONFIG_MTK_SERIAL)
	request_uart_to_wakeup();
#endif
	__spm_get_wakeup_status(&wakesta);
	spm_mcsodi_pcm_setup_after_wfi(operation_cond);

	spm_output_log(&wakesta);
	mcsodi_after_wfi(cpu);
	spm_mcsodi_reset_footprint();
	mcsodi_start = false;

	spin_unlock_irqrestore(&mcsodi_busy_spin_lock, flags);

	return 0;
}

void spm_mcsodi_init(void)
{
}

