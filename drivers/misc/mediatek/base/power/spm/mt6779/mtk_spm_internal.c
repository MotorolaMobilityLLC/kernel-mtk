/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/random.h>
#include <asm/setup.h>
#include <mtk_spm_internal.h>
#include <mtk_power_gs_api.h>

#define WORLD_CLK_CNTCV_L        (0x10017008)
#define WORLD_CLK_CNTCV_H        (0x1001700C)
static u32 pcm_timer_ramp_max_sec_loop = 1;

char *wakesrc_str[32] = {
	[0] = " STA1_PCM_TIMER",
	[1] = " STA1_SPM_DEBUG",
	[2] = " STA1_KP_IRQ",
	[3] = " STA1_APWDT_EVENT",
	[4] = " STA1_APXGPT_EVENT",
	[5] = " STA1_CONN2AP_WAKEUP",
	[6] = " STA1_EINT_EVENT",
	[7] = " STA1_CONN_WDT_IRQ",
	[8] = " STA1_CCIF0_EVENT",
	[9] = " STA1_LOWBATTERY_IRQ",
	[10] = " STA1_SSPM2SPM_WAKEUP",
	[11] = " STA1_SCP2SPM_WAKEUP",
	[12] = " STA1_ADSP2SPM_WAKEUP",
	[13] = " STA1_PCM_WDT_WAKE",
	[14] = " STA1_USB0_CDSC",
	[15] = " STA1_USB0_POWERDWN",
	[16] = " STA1_SYS_TIMER",
	[17] = " STA1_EINT_EVENT_SECURE",
	[18] = " STA1_CCIF1_EVENT",
	[19] = " STA1_UART0_IRQ",
	[20] = " STA1_AFE_IRQ_MCU",
	[21] = " STA1_THERM_CTRL_EVENT",
	[22] = " STA1_SYS_CIRQ_IRQ",
	[23] = " STA1_MD2AP_PEER_WAKEUP",
	[24] = " STA1_CSYSPWREQ",
	[25] = " STA1_MD_WDT",
	[26] = " STA1_AP2AP_PEER_WAKEUP",
	[27] = " STA1_BY_SEJ",
	[28] = " STA1_CPU_WAKEUP",
	[29] = " STA1_ALL_CPU_IRQ",
	[30] = " STA1_CPU_NOT_WFI",
	[31] = " STA1_MCUSYS_IDLE",
};

/**************************************
 * Function and API
 **************************************/
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

void __spm_set_pwrctrl_pcm_flags(struct pwr_ctrl *pwrctrl, u32 flags)
{
	if (pwrctrl->pcm_flags_cust == 0)
		pwrctrl->pcm_flags = flags;
	else
		pwrctrl->pcm_flags = pwrctrl->pcm_flags_cust;
}

void __spm_set_pwrctrl_pcm_flags1(struct pwr_ctrl *pwrctrl, u32 flags)
{
	if (pwrctrl->pcm_flags1_cust == 0)
		pwrctrl->pcm_flags1 = flags;
	else
		pwrctrl->pcm_flags1 = pwrctrl->pcm_flags1_cust;
}

void __spm_sync_pcm_flags(struct pwr_ctrl *pwrctrl)
{
	/* set PCM flags and data */
	if (pwrctrl->pcm_flags_cust_clr != 0)
		pwrctrl->pcm_flags &= ~pwrctrl->pcm_flags_cust_clr;
	if (pwrctrl->pcm_flags_cust_set != 0)
		pwrctrl->pcm_flags |= pwrctrl->pcm_flags_cust_set;
	if (pwrctrl->pcm_flags1_cust_clr != 0)
		pwrctrl->pcm_flags1 &= ~pwrctrl->pcm_flags1_cust_clr;
	if (pwrctrl->pcm_flags1_cust_set != 0)
		pwrctrl->pcm_flags1 |= pwrctrl->pcm_flags1_cust_set;
}

void __spm_get_wakeup_status(struct wake_status *wakesta)
{
	/* get wakeup event */
	/* backup of PCM_REG12_DATA */
	wakesta->r12 = spm_read(SPM_SW_RSV_0);
	wakesta->r12_ext = spm_read(SPM_WAKEUP_STA);
	wakesta->raw_sta = spm_read(SPM_WAKEUP_STA);
	wakesta->raw_ext_sta = spm_read(SPM_WAKEUP_EXT_STA);
	wakesta->md32pcm_wakeup_sta = spm_read(MD32PCM_WAKEUP_STA);
	wakesta->md32pcm_event_sta = spm_read(MD32PCM_EVENT_STA);
	/* backup of SPM_WAKEUP_MISC */
	wakesta->wake_misc = spm_read(SPM_SW_RSV_5);
	/* backup of SYS_TIMER */
	wakesta->sys_timer = spm_read(SPM_SW_RSV_7);

	/* get sleep time */
	/* backup of PCM_TIMER_OUT */
	wakesta->timer_out = spm_read(SPM_SW_RSV_6);

	/* get other SYS and co-clock status */
	wakesta->r13 = spm_read(PCM_REG13_DATA);
	wakesta->idle_sta = spm_read(SUBSYS_IDLE_STA);
	wakesta->req_sta0 = spm_read(SRC_REQ_STA_0);
	wakesta->req_sta1 = spm_read(SRC_REQ_STA_1);
	wakesta->req_sta2 = spm_read(SRC_REQ_STA_2);
	wakesta->req_sta3 = spm_read(SRC_REQ_STA_3);
	/* get debug flag for PCM execution check */
	wakesta->debug_flag = spm_read(PCM_WDT_LATCH_SPARE_0);
	wakesta->debug_flag1 = spm_read(PCM_WDT_LATCH_SPARE_1);

/* TODO: FIX ME */
#if 0
	/* get backup SW flag status */
	wakesta->b_sw_flag0 = spm_read(SPM_BSI_D0_SR);   /* SPM_BSI_D0_SR */
	wakesta->b_sw_flag1 = spm_read(SPM_BSI_D1_SR);   /* SPM_BSI_D1_SR */
#endif

	/* get ISR status */
	wakesta->isr = spm_read(SPM_IRQ_STA);

	/* get SW flag status */
	wakesta->sw_flag0 = spm_read(SPM_SW_FLAG_0);
	wakesta->sw_flag1 = spm_read(SPM_SW_FLAG_1);

	/* get the numbers of enter DDREN */
	wakesta->num_ddren = spm_read(SPM_SW_RSV_8);

	/* get the numbers of enter MCDSR */
	wakesta->num_mcdsr = spm_read(SPM_SW_RSV_9);

	/* get CLK SETTLE */
	wakesta->clk_settle = spm_read(SPM_CLK_SETTLE); /* SPM_CLK_SETTLE */
	/* check abort */
	wakesta->is_abort = wakesta->debug_flag & DEBUG_ABORT_MASK;
	wakesta->is_abort |= wakesta->debug_flag1 & DEBUG_ABORT_MASK_1;

}

void rekick_vcorefs_scenario(void)
{
/* FIXME: */
}

unsigned int __spm_output_wake_reason(
	const struct wake_status *wakesta, bool suspend, const char *scenario)
{
	int i;
	char buf[LOG_BUF_SIZE] = { 0 };
	char log_buf[1024] = { 0 };
	char *local_ptr;
	int log_size = 0;
	unsigned int wr = WR_UNKNOWN;
	unsigned int spm_26M_off_pct = 0;

	if (wakesta->is_abort != 0) {
		/* add size check for vcoredvfs */
		aee_sram_printk("SPM ABORT (%s), r13 = 0x%x, ",
			scenario, wakesta->r13);
		pr_info("[SPM] ABORT (%s), r13 = 0x%x, ",
			scenario, wakesta->r13);

		aee_sram_printk(" debug_flag = 0x%x 0x%x\n",
			wakesta->debug_flag, wakesta->debug_flag1);
		pr_info(" debug_flag = 0x%x 0x%x\n",
			wakesta->debug_flag, wakesta->debug_flag1);

		aee_sram_printk(" sw_flag = 0x%x 0x%x\n",
			wakesta->sw_flag0, wakesta->sw_flag1);
		pr_info(" sw_flag = 0x%x 0x%x\n",
			wakesta->sw_flag0, wakesta->sw_flag1);

		aee_sram_printk(" b_sw_flag = 0x%x 0x%x\n",
			wakesta->b_sw_flag0, wakesta->b_sw_flag1);
		pr_info(" b_sw_flag = 0x%x 0x%x\n",
			wakesta->b_sw_flag0, wakesta->b_sw_flag1);

		aee_sram_printk(" num = 0x%x 0x%x\n",
			wakesta->num_ddren, wakesta->num_mcdsr);
		pr_info(" num = 0x%x 0x%x\n",
			wakesta->num_ddren, wakesta->num_mcdsr);

		wr =  WR_ABORT;
	}


	if (wakesta->r12 & STA1_PCM_TIMER) {

		if (wakesta->wake_misc & WAKE_MISC_PCM_TIMER_EVENT) {
			local_ptr = wakesrc_str[0];
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_PCM_TIMER;
		}
	}

	if (wakesta->r12 & STA1_SPM_DEBUG) {

		if (wakesta->wake_misc & WAKE_MISC_DVFSRC_IRQ) {
			local_ptr = " DVFSRC";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_DVFSRC;
		}

		if (wakesta->wake_misc & WAKE_MISC_TWAM_IRQ_B) {
			local_ptr = " TWAM";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_TWAM;
		}

		if (wakesta->wake_misc & WAKE_MISC_PMSR_IRQ_B_SET0) {
			local_ptr = " PMSR";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_PMSR;
		}

		if (wakesta->wake_misc & WAKE_MISC_PMSR_IRQ_B_SET1) {
			local_ptr = " PMSR";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_PMSR;
		}

		if (wakesta->wake_misc & WAKE_MISC_PMSR_IRQ_B_SET2) {
			local_ptr = " PMSR";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_PMSR;
		}

		if (wakesta->wake_misc & WAKE_MISC_SPM_ACK_CHK_WAKEUP_0) {
			local_ptr = " SPM_ACK_CHK";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_SPM_ACK_CHK;
		}

		if (wakesta->wake_misc & WAKE_MISC_SPM_ACK_CHK_WAKEUP_1) {
			local_ptr = " SPM_ACK_CHK";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_SPM_ACK_CHK;
		}

		if (wakesta->wake_misc & WAKE_MISC_SPM_ACK_CHK_WAKEUP_2) {
			local_ptr = " SPM_ACK_CHK";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_SPM_ACK_CHK;
		}

		if (wakesta->wake_misc & WAKE_MISC_SPM_ACK_CHK_WAKEUP_3) {
			local_ptr = " SPM_ACK_CHK";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_SPM_ACK_CHK;
		}

		if (wakesta->wake_misc & WAKE_MISC_SPM_ACK_CHK_WAKEUP_ALL) {
			local_ptr = " SPM_ACK_CHK";
			if ((strlen(buf) + strlen(local_ptr)) < LOG_BUF_SIZE)
				strncat(buf, local_ptr, strlen(local_ptr));
			wr = WR_SPM_ACK_CHK;
		}
	}

	for (i = 2; i < 32; i++) {
		if (wakesta->r12 & (1U << i)) {
			if ((strlen(buf) + strlen(wakesrc_str[i])) <
				LOG_BUF_SIZE)
				strncat(buf, wakesrc_str[i],
					strlen(wakesrc_str[i]));

			wr = WR_WAKE_SRC;
		}
	}
	WARN_ON(strlen(buf) >= LOG_BUF_SIZE);

	log_size += sprintf(log_buf,
		"%s wake up by %s, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x 0x%x, ",
		scenario, buf, wakesta->timer_out, wakesta->r13,
		wakesta->debug_flag, wakesta->debug_flag1);

	log_size += sprintf(log_buf + log_size,
		  "r12 = 0x%x, r12_ext = 0x%x, raw_sta = 0x%x, 0x%x, 0x%x 0x%x\n",
		  wakesta->r12, wakesta->r12_ext,
		  wakesta->raw_sta, wakesta->idle_sta,
		  wakesta->md32pcm_wakeup_sta,
		  wakesta->md32pcm_event_sta);

	log_size += sprintf(log_buf + log_size,
		  " req_sta =  0x%x 0x%x 0x%x 0x%x, isr = 0x%x, num = 0x%x 0x%x, ",
		  wakesta->req_sta0, wakesta->req_sta1, wakesta->req_sta2,
		  wakesta->req_sta3, wakesta->isr,
		  wakesta->num_ddren, wakesta->num_mcdsr);

	log_size += sprintf(log_buf + log_size,
		"raw_ext_sta = 0x%x, wake_misc = 0x%x, sw_flag = 0x%x 0x%x 0x%x 0x%x, req = 0x%x,",
		wakesta->raw_ext_sta,
		wakesta->wake_misc,
		wakesta->sw_flag0,
		wakesta->sw_flag1,
		wakesta->b_sw_flag0,
		wakesta->b_sw_flag1,
		spm_read(SPM_SRC_REQ));

	if (!strcmp(scenario, "suspend")) {
		/* calculate 26M off percentage in suspend period */
		if (wakesta->timer_out != 0) {
			spm_26M_off_pct = 100 * spm_read(SPM_SW_RSV_4)
						/ wakesta->timer_out;
		}

		log_size += sprintf(log_buf + log_size,
			" clk_settle = 0x%x, ",
			wakesta->clk_settle);

		log_size += sprintf(log_buf + log_size,
			"wlk_cntcv_l = 0x%x, wlk_cntcv_h = 0x%x, 26M_off_pct = %d\n",
			_golden_read_reg(WORLD_CLK_CNTCV_L),
			_golden_read_reg(WORLD_CLK_CNTCV_H),
			spm_26M_off_pct);
	} else
		log_size += sprintf(log_buf + log_size,
			" clk_settle = 0x%x\n",
			wakesta->clk_settle);

	WARN_ON(log_size >= 1024);

	if (!suspend)
		pr_info("[SPM] %s", log_buf);
	else {
		aee_sram_printk("%s", log_buf);
		pr_info("[SPM] %s", log_buf);
	}

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
// FIXME
}

int __attribute__ ((weak)) get_dynamic_period(
	int first_use, int first_wakeup_time, int battery_capacity_level)
{
	/* pr_info("NO %s !!!\n", __func__); */
	return 5401;
}

u32 __spm_get_wake_period(int pwake_time, unsigned int last_wr)
{
	int period = SPM_WAKE_PERIOD;

	if (pwake_time < 0) {
		/* use FG to get the period of 1% battery decrease */
		period = get_dynamic_period(last_wr != WR_PCM_TIMER
				? 1 : 0, SPM_WAKE_PERIOD, 1);
		if (period <= 0) {
			pr_info("[SPM] CANNOT GET PERIOD FROM FUEL GAUGE\n");
			period = SPM_WAKE_PERIOD;
		}
	} else {
		period = pwake_time;
		aee_sram_printk("pwake = %d\n", pwake_time);
		pr_info("[SPM] pwake = %d\n", pwake_time);
	}

	if (period > 36 * 3600)	/* max period is 36.4 hours */
		period = 36 * 3600;

	return period;
}

/* FIXME:
 * 1. Need to move to
 * "drivers/misc/mediatek/base/power/spm/mt3967/sleep_def.h" later
 * 2. Need to check w/ DE for the correct value.
 */

void __sync_vcore_ctrl_pcm_flag(u32 oper_cond, u32 *flag)
{
	if (oper_cond & DEEPIDLE_OPT_VCORE_LOW_VOLT)
		*flag &= ~SPM_FLAG1_DISABLE_VCORE_LP_0P600V;
	else
		*flag |= SPM_FLAG1_DISABLE_VCORE_LP_0P600V;
}

MODULE_DESCRIPTION("SPM-Internal Driver v0.1");
