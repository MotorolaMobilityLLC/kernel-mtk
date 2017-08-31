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
#include <mt-plat/mtk_secure_api.h>
#ifdef CONFIG_MTK_DCS
#include <mt-plat/mtk_meminfo.h>
#endif
#include <mt-plat/mtk_chip.h>

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

#ifdef CONFIG_MACH_MT6759
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
	[9] = " R12_WAKE_UP_EVENT_MCSODI_SRC1_B",
	[10] = " R12_SSPM_SPM_IRQ_B",
	[11] = " R12_SCP_IPC_MD2SPM_B",
	[12] = " R12_SCP_WDT_EVENT_B",
	[13] = " R12_PCM_WDT_WAKEUP_B",
	[14] = " R12_USBX_CDSC_B",
	[15] = " R12_USBX_POWERDWN_B",
	[16] = " R12_CONN2AP_WAKEUP_B",
	[17] = " R12_EINT_EVENT_SECURE_B",
	[18] = " R12_CCIF1_EVENT_B",
	[19] = " R12_UART0_IRQ_B",
	[20] = " R12_AFE_IRQ_MCU_B",
	[21] = " R12_THERMAL_CTRL_EVENT_B",
	[22] = " R12_SCP_CIRQ_IRQ_B",
	[23] = " R12_CONN2AP_WDT_IRQ_B",
	[24] = " R12_CSYSPWRUPREQ_B",
	[25] = " R12_MD1_WDT_B",
	[26] = " R12_MD2AP_PEER_WAKEUP_EVENT",
	[27] = " R12_SEJ_EVENT_B",
	[28] = " R12_SSPM_WAKEUP_SSPM",
	[29] = " R12_CPU_IRQ_B",
	[30] = " R12_SCP_WAKEUP_SCP",
	[31] = " R12_BIT31",
};
#else
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
#endif

/**************************************
 * Function and API
 **************************************/
int __spm_check_opp_level_impl(int ch)
{
	/* 20170407 Owen fix build error */
	/* int opp = vcorefs_get_sw_opp(); */
	int opp = 0;
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
	int scp_opp = scp_get_dvfs_opp();
#endif

#ifdef CONFIG_MACH_MT6759
	opp = (opp > OPP_1) ? OPP_1 : opp;
#elif defined(CONFIG_MACH_MT6758)
	opp = (opp > OPP_1) ? OPP_1 : opp;
#endif

#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
	if (scp_opp >= 0)
		opp = min(scp_opp, opp);
#endif /* CONFIG_MTK_TINYSYS_SCP_SUPPORT */

	return opp;
}

int __spm_check_opp_level(int ch)
{
	int opp;
#ifdef CONFIG_MACH_MT6759
	int level[4] = {2, 1, 0, 0};
#elif defined(CONFIG_MACH_MT6758)
	int level[4] = {2, 1, 0, 0};
#else
	int level[4] = {4, 3, 1, 0};

	if (ch == 2)
		level[1] = 3;
	else
		level[1] = 2;
#endif
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
	/* 20170407 Owen fix build error */
	/* vcore_volt_pmic_val = (is_vcore_volt_lower) ? VOLT_TO_PMIC_VAL(56800) : get_vcore_ptp_volt(opp); */
	return vcore_volt_pmic_val;
}

void __spm_update_pcm_flags_dcs_workaround(struct pwr_ctrl *pwrctrl, int ch)
{
#ifdef CONFIG_MACH_MT6799
#ifdef CONFIG_MTK_DCS
	/* dcs s0 workaround only applied when ch is 2 */
	if (mt_get_chip_sw_ver() == CHIP_SW_VER_02 &&
		((spm_get_resource_usage() & SPM_RESOURCE_DRAM) || ch == 4))
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_DCSS0_LOW;
#else
	/* dcs s0 work around default off */
	if (mt_get_chip_sw_ver() == CHIP_SW_VER_02)
		pwrctrl->pcm_flags |= SPM_FLAG_DIS_DCSS0_LOW;
#endif
#endif
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
#ifndef CONFIG_MACH_MT6759
	int flag;

	if (spm_read(PCM_REG15_DATA) == 0x0) {
		flag = spm_dvfs_flag_init();
		spm_go_to_vcorefs(flag);
	}
#endif
}

wake_reason_t __spm_output_wake_reason(const struct wake_status *wakesta,
		const struct pcm_desc *pcmdesc, bool suspend, const char *scenario)
{
	int i;
	char buf[LOG_BUF_SIZE] = { 0 };
	char log_buf[1024] = { 0 };
	int log_size = 0;
	wake_reason_t wr = WR_UNKNOWN;

	if (wakesta->assert_pc != 0) {
		/* add size check for vcoredvfs */
		spm_print(suspend, "PCM ASSERT AT %u (%s), r13 = 0x%x, debug_flag = 0x%x\n",
			  wakesta->assert_pc, scenario, wakesta->r13, wakesta->debug_flag);

		/* aee_kernel_warning("SPM Warning", "SPM F/W ASSERT WARNING"); */

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

	log_size += sprintf(log_buf, "wake up by %s, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x, ch = %u\n",
		  buf, wakesta->timer_out, wakesta->r13, wakesta->debug_flag, wakesta->dcs_ch);

	log_size += sprintf(log_buf + log_size,
		  "r12 = 0x%x, r12_ext = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
		  wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
		  wakesta->event_reg, wakesta->isr);

	log_size += sprintf(log_buf + log_size,
		"raw_ext_sta = 0x%x, wake_misc = 0x%x, pcm_flag = 0x%x 0x%x 0x%x, req = 0x%x, resource = 0x%x\n",
		wakesta->raw_ext_sta,
		wakesta->wake_misc,
		spm_read(SPM_SW_FLAG),
		spm_read(SPM_RSV_CON2),
		spm_read(SPM_SW_RSV_4),
		spm_read(SPM_SRC_REQ),
		spm_get_resource_usage());

	WARN_ON(log_size >= 1024);

	spm_print(suspend, "%s", log_buf);

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

	mt_secure_call(MTK_SIP_KERNEL_SPM_DUMMY_READ, rank0_addr, rank1_addr, 0);
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
	/* pr_err("NO %s !!!\n", __func__); */
	return 5401;
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
