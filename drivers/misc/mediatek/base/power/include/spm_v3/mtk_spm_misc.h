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

#ifndef __MTK_SPM_MISC_H__
#define __MTK_SPM_MISC_H__

#include <linux/irqchip/mtk-gic.h>


/* SPM Early Porting Config Define */
#define SPM_NO_CPUFREQ		(1)
#define SPM_NO_EEM		(1)
#define SPM_NO_TRACE_EVT		(1)
#define SPM_NO_GPT		(1)
#define SPM_NO_WDT		(1)
#define SPM_NO_VCOREDVFS		(1)
#define SPM_NO_CLKBUF		(1)

/* AEE */
#ifdef CONFIG_MTK_RAM_CONSOLE
#define SPM_AEE_RR_REC 1
#else
#define SPM_AEE_RR_REC 0
#endif

/* IRQ */
extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern void mt_irq_unmask_for_sleep(unsigned int irq);

/* UART */
extern int request_uart_to_sleep(void);
extern int request_uart_to_wakeup(void);
extern void mtk_uart_restore(void);
extern void dump_uart_reg(void);

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
extern int is_teei_ready(void);
#endif

/* SODI3 */
extern void soidle3_before_wfi(int cpu);
extern void soidle3_after_wfi(int cpu);

#if SPM_AEE_RR_REC
extern void aee_rr_rec_sodi3_val(u32 val);
extern u32 aee_rr_curr_sodi3_val(void);
#endif

#ifdef SPM_SODI_PROFILE_TIME
extern unsigned int soidle3_profile[4];
#endif

/* SODI */
extern void soidle_before_wfi(int cpu);
extern void soidle_after_wfi(int cpu);
#if SPM_AEE_RR_REC
extern void aee_rr_rec_sodi_val(u32 val);
extern u32 aee_rr_curr_sodi_val(void);
#endif

#ifdef SPM_SODI_PROFILE_TIME
extern unsigned int soidle_profile[4];
#endif

extern bool mtk_gpu_sodi_entry(void);
extern bool mtk_gpu_sodi_exit(void);
extern int hps_del_timer(void);
extern int hps_restart_timer(void);
extern int vcorefs_get_curr_ddr(void);
extern int vcorefs_get_curr_vcore(void);

/* MC-SODI */
extern void mcsodi_before_wfi(int cpu);
extern void mcsodi_after_wfi(int cpu);
extern bool mtk_idle_cpu_wfi_criteria(void);
#if SPM_AEE_RR_REC
extern void aee_rr_rec_mcsodi_val(u32 val);
extern u32 aee_rr_curr_mcsodi_val(void);
#endif

/* Deepidle */
#if SPM_AEE_RR_REC
extern void aee_rr_rec_deepidle_val(u32 val);
extern u32 aee_rr_curr_deepidle_val(void);
#endif

/* Suspend */
#if SPM_AEE_RR_REC
extern void aee_rr_rec_spm_suspend_val(u32 val);
extern u32 aee_rr_curr_spm_suspend_val(void);
#endif

/* Vcore DVFS */
#if SPM_AEE_RR_REC
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);
#endif

/* MCDI */
extern void mcidle_before_wfi(int cpu);
extern void mcidle_after_wfi(int cpu);

#if SPM_AEE_RR_REC
extern unsigned int *aee_rr_rec_mcdi_wfi(void);
#endif

/* snapshot golden setting */
extern int snapshot_golden_setting(const char *func, const unsigned int line);
extern bool is_already_snap_shot;

/* power golden setting */
extern void mt_power_gs_dump_suspend(void);
extern void mt_power_gs_dump_dpidle(void);
extern void mt_power_gs_dump_sodi3(void);
extern bool slp_dump_golden_setting;
extern int slp_dump_golden_setting_type;

/* gpio */
extern void gpio_dump_regs(void);

/* pasr */
extern void mtkpasr_phaseone_ops(void);
extern int configure_mrw_pasr(u32 segment_rank0, u32 segment_rank1);
extern int pasr_enter(u32 *sr, u32 *dpd);
extern int pasr_exit(void);
extern unsigned long mtkpasr_enable_sr;

/* eint */
extern void mt_eint_print_status(void);

#ifdef CONFIG_FPGA_EARLY_PORTING
__attribute__ ((weak))
unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val,
			unsigned int MASK, unsigned int SHIFT)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}

__attribute__ ((weak))
unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val,
			unsigned int MASK, unsigned int SHIFT)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__ ((weak))
unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val,
				unsigned int MASK, unsigned int SHIFT)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif /* CONFIG_FPGA_EARLY_PORTING */

__attribute__ ((weak))
int vcorefs_get_curr_ddr(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return -1;
}

extern int mt_cpu_dormant_init(void);

extern int univpll_is_used(void);

__attribute__ ((weak))
int hps_get_root_id(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}

#ifdef SPM_NO_GPT
#define GPT1            0x0
#define GPT2            0x1
#define GPT3            0x2
#define GPT4            0x3
#define GPT5            0x4
#define GPT6            0x5
#define NR_GPTS         0x6


#define GPT_ONE_SHOT    0x0
#define GPT_REPEAT      0x1
#define GPT_KEEP_GO     0x2
#define GPT_FREE_RUN    0x3


#define GPT_CLK_DIV_1   0x0000
#define GPT_CLK_DIV_2   0x0001
#define GPT_CLK_DIV_3   0x0002
#define GPT_CLK_DIV_4   0x0003
#define GPT_CLK_DIV_5   0x0004
#define GPT_CLK_DIV_6   0x0005
#define GPT_CLK_DIV_7   0x0006
#define GPT_CLK_DIV_8   0x0007
#define GPT_CLK_DIV_9   0x0008
#define GPT_CLK_DIV_10  0x0009
#define GPT_CLK_DIV_11  0x000a
#define GPT_CLK_DIV_12  0x000b
#define GPT_CLK_DIV_13  0x000c
#define GPT_CLK_DIV_16  0x000d
#define GPT_CLK_DIV_32  0x000e
#define GPT_CLK_DIV_64  0x000f


#define GPT_CLK_SRC_SYS 0x0
#define GPT_CLK_SRC_RTC 0x1


#define GPT_NOAUTOEN    0x0001
#define GPT_NOIRQEN     0x0002
__attribute__((weak))
int request_gpt(unsigned int id, unsigned int mode, unsigned int clksrc,
		unsigned int clkdiv, unsigned int cmp,
		void (*func)(unsigned long), unsigned int flags)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int free_gpt(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int start_gpt(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int stop_gpt(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int restart_gpt(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_is_counting(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_set_cmp(unsigned int id, unsigned int val)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_get_cmp(unsigned int id, unsigned int *ptr)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_get_cnt(unsigned int id, unsigned int *ptr)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_check_irq(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_check_and_ack_irq(unsigned int id)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
int gpt_set_clk(unsigned int id, unsigned int clksrc, unsigned int clkdiv)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
__attribute__((weak))
u64 mtk_timer_get_cnt(u8 timer)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif

#ifdef SPM_NO_EEM
__attribute__((weak))
int get_vcore_ptp_volt(unsigned int uv)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif

#ifdef SPM_NO_WDT
__attribute__((weak))
int  mtk_rgu_cfg_dvfsrc(int enable)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif

#ifdef SPM_NO_VCOREDVFS
__attribute__((weak))
int  is_vcorefs_can_work(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}

__attribute__((weak))
u32 log_mask(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif

__attribute__((weak))
int univpll_is_used(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}

__attribute__((weak))
void aee_sram_printk(const char *fmt, ...)
{
	pr_notice("NO %s !!!\n", __func__);
}

#ifdef SPM_NO_CLKBUF
__attribute__((weak))
u32 clk_buf_bblpm_enter_cond(void)
{
	pr_notice("NO %s !!!\n", __func__);
	return 0;
}
#endif

extern struct dram_info *g_dram_info_dummy_read;

#endif  /* __MTK_SPM_MISC_H__ */
