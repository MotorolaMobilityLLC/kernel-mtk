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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>

#include <linux/types.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/tick.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/kallsyms.h>

#include <linux/irqchip/mtk-gic.h>

#include <asm/system_misc.h>
#include <mt-plat/sync_write.h>
#include <mach/mtk_gpt.h>
#include <mtk_spm.h>
#include <mtk_spm_dpidle.h>
#include <mtk_spm_idle.h>
#ifdef CONFIG_THERMAL
#include <mach/mtk_thermal.h>
#endif
#ifdef CONFIG_MTK_ACAO_SUPPORT
#include <mtk_mcdi_api.h>
#endif
#include <mtk_idle.h>
#include <mtk_idle_internal.h>
#include <mtk_idle_profile.h>
#include <mtk_spm_reg.h>
#include <mtk_spm_misc.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>

#include "ufs-mtk.h"

#ifdef CONFIG_MTK_DCS
#include <mt-plat/mtk_meminfo.h>
#endif

#include <linux/uaccess.h>
#include <mtk_cpufreq_api.h>

#define IDLE_TAG     "Power/swap "
#define idle_err(fmt, args...)		pr_err(IDLE_TAG fmt, ##args)
#define idle_warn(fmt, args...)		pr_warn(IDLE_TAG fmt, ##args)
#define idle_info(fmt, args...)		pr_debug(IDLE_TAG fmt, ##args)
#define idle_ver(fmt, args...)		pr_debug(IDLE_TAG fmt, ##args)
#define idle_dbg(fmt, args...)		pr_debug(IDLE_TAG fmt, ##args)

#define idle_warn_log(fmt, args...) { \
	if (dpidle_dump_log & DEEPIDLE_LOG_FULL) \
		pr_warn(IDLE_TAG fmt, ##args); \
	}

/* 20170407 Owen fix build error */
#if 0
#define IDLE_GPT GPT4
#endif
#define log2buf(p, s, fmt, args...) \
	(p += snprintf(p, sizeof(s) - strlen(s), fmt, ##args))

#ifndef CONFIG_MTK_ACAO_SUPPORT
static atomic_t is_in_hotplug = ATOMIC_INIT(0);
#else
#define USING_STD_TIMER_OPS
#endif

static bool mt_idle_chk_golden;
static bool mt_dpidle_chk_golden;

#define NR_CMD_BUF		128

void go_to_wfi(void)
{
	isb();
	mb();	/* memory barrier */
	__asm__ __volatile__("wfi" : : : "memory");
}

/* FIXME: early porting */
#if 1
void __attribute__((weak))
bus_dcm_enable(void)
{
	/* FIXME: early porting */
}
void __attribute__((weak))
bus_dcm_disable(void)
{
	/* FIXME: early porting */
}

unsigned int __attribute__((weak))
mt_get_clk_mem_sel(void)
{
	return 1;
}

void __attribute__((weak))
tscpu_cancel_thermal_timer(void)
{

}
void __attribute__((weak))
tscpu_start_thermal_timer(void)
{
	/* FIXME: early porting */
}

void __attribute__((weak)) mtkTTimer_start_timer(void)
{

}

void __attribute__((weak)) mtkTTimer_cancel_timer(void)
{

}

bool __attribute__((weak)) mtk_gpu_sodi_entry(void)
{
	return false;
}

bool __attribute__((weak)) mtk_gpu_sodi_exit(void)
{
	return false;
}

bool __attribute__((weak)) spm_mcdi_can_enter(void)
{
	return false;
}

bool __attribute__((weak)) spm_get_sodi3_en(void)
{
	return false;
}

bool __attribute__((weak)) spm_get_sodi_en(void)
{
	return false;
}

int __attribute__((weak)) hps_del_timer(void)
{
	return 0;
}

int __attribute__((weak)) hps_restart_timer(void)
{
	return 0;
}

void __attribute__((weak)) msdc_clk_status(int *status)
{
	*status = 0x1;
}

wake_reason_t __attribute__((weak)) spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 log_cond, u32 operation_cond)
{
	go_to_wfi();

	return WR_NONE;
}

void __attribute__((weak)) spm_enable_sodi(bool en)
{

}

void __attribute__((weak)) spm_sodi_mempll_pwr_mode(bool pwr_mode)
{

}

wake_reason_t __attribute__((weak)) spm_go_to_sodi3(u32 spm_flags, u32 spm_data, u32 sodi_flags, u32 operation_cond)
{
	return WR_UNKNOWN;
}

wake_reason_t __attribute__((weak)) spm_go_to_sodi(u32 spm_flags, u32 spm_data, u32 sodi_flags, u32 operation_cond)
{
	return WR_UNKNOWN;
}

bool __attribute__((weak)) go_to_mcidle(int cpu)
{
	return false;
}

void __attribute__((weak)) spm_go_to_mcsodi(u32 spm_flags, u32 spm_data, u32 sodi_flags, u32 operation_cond)
{
}

bool __attribute__((weak)) spm_mcsodi_start(void)
{
	return false;
}

int __attribute__((weak)) spm_leave_mcsodi(u32 cpu, u32 operation_cond)
{
	return -1;
}

void __attribute__((weak)) spm_mcdi_switch_on_off(enum spm_mcdi_lock_id id, int mcdi_en)
{

}

int __attribute__((weak)) is_teei_ready(void)
{
	return 1;
}

unsigned long __attribute__((weak)) localtimer_get_counter(void)
{
	return 0;
}

int __attribute__((weak)) localtimer_set_next_event(unsigned long evt)
{
	return 0;
}

void __attribute__((weak)) mcdi_heart_beat_log_dump(void)
{

}

#endif

static bool idle_by_pass_secure_cg;

static unsigned int idle_block_mask[NR_TYPES][NF_CG_STA_RECORD];

static bool clkmux_cond[NR_TYPES];
static unsigned int clkmux_block_mask[NR_TYPES][NF_CLK_CFG];
static unsigned int clkmux_addr[NF_CLK_CFG] = {
	0x10210100,
	0x10210110,
	0x10210120,
	0x10210130,
	0x10210140,
	0x10210150,
	0x10210160,
	0x10210170,
	0x10210180,
	0x10210190,
	0x102101A0,
	0x102101B0,
	0x102101C0,
	0x102101D0,
	0x102101E0
};

/* DeepIdle */
static unsigned int     dpidle_time_criteria = 26000; /* 2ms */
static unsigned long    dpidle_cnt[NR_CPUS] = {0};
static unsigned long    dpidle_f26m_cnt[NR_CPUS] = {0};
static unsigned long    dpidle_block_cnt[NR_REASONS] = {0};
static bool             dpidle_by_pass_cg;
bool                    dpidle_by_pass_pg;
static bool             dpidle_gs_dump_req;
static unsigned int     dpidle_gs_dump_delay_ms = 10000; /* 10 sec */
static unsigned int     dpidle_gs_dump_req_ts;
static unsigned int     dpidle_dump_log = DEEPIDLE_LOG_REDUCED;
static unsigned int     dpidle_run_once;
static bool             dpidle_force_vcore_lp_mode;

/* SODI3 */

static unsigned int     soidle3_pll_block_mask[NR_PLLS] = {0x0};
static unsigned int     soidle3_time_criteria = 65000; /* 5ms */
static unsigned long    soidle3_cnt[NR_CPUS] = {0};
static unsigned long    soidle3_block_cnt[NR_REASONS] = {0};

static bool             soidle3_by_pass_cg;
static bool             soidle3_by_pass_pll;
static bool             soidle3_by_pass_en;
static u32              sodi3_flags = SODI_FLAG_REDUCE_LOG;
static bool             sodi3_force_vcore_lp_mode;
#ifdef SPM_SODI3_PROFILE_TIME
unsigned int            soidle3_profile[4];
#endif

/* SODI */
static unsigned int     soidle_time_criteria = 26000; /* 2ms */
static unsigned long    soidle_cnt[NR_CPUS] = {0};
static unsigned long    soidle_block_cnt[NR_REASONS] = {0};
static bool             soidle_by_pass_cg;
bool                    soidle_by_pass_pg;
static bool             soidle_by_pass_en;
static u32              sodi_flags = SODI_FLAG_REDUCE_LOG;
#ifdef SPM_SODI_PROFILE_TIME
unsigned int            soidle_profile[4];
#endif

/* MCDI */
static unsigned int     mcidle_time_criteria = 3000; /* 3ms */
static unsigned long    mcidle_cnt[NR_CPUS] = {0};
static unsigned long    mcidle_block_cnt[NR_CPUS][NR_REASONS] = { {0}, {0} };

/* MC-SODI */
static unsigned int     mcsodi_time_criteria = 1300; /* 100us */
static unsigned long    mcsodi_cnt[NR_CPUS] = {0};
static unsigned long    mcsodi_block_cnt[NR_REASONS] = {0};
static bool             mcsodi_by_pass_cg;
bool                    mcsodi_by_pass_pg;
static bool             mcsodi_by_pass_en;

/* Slow Idle */
static unsigned long    slidle_cnt[NR_CPUS] = {0};
static unsigned long    slidle_block_cnt[NR_REASONS] = {0};

/* Regular Idle */
static unsigned long    rgidle_cnt[NR_CPUS] = {0};

/* idle_notifier */
static RAW_NOTIFIER_HEAD(mtk_idle_notifier);

int mtk_idle_notifier_register(struct notifier_block *n)
{
	int ret = 0;
	int index = 0;
#ifdef CONFIG_KALLSYMS
	char namebuf[128] = {0};
	const char *symname = NULL;

	symname = kallsyms_lookup((unsigned long)n->notifier_call,
			NULL, NULL, NULL, namebuf);
	if (symname) {
		pr_err("[mt_idle_ntf] <%02d>%08lx (%s)\n",
			index++, (unsigned long)n->notifier_call, symname);
	} else {
		pr_err("[mt_idle_ntf] <%02d>%08lx\n",
			index++, (unsigned long)n->notifier_call);
	}
#else
	pr_err("[mt_idle_ntf] <%02d>%08lx\n",
			index++, (unsigned long)n->notifier_call);
#endif

	ret = raw_notifier_chain_register(&mtk_idle_notifier, n);

	return ret;
}
EXPORT_SYMBOL_GPL(mtk_idle_notifier_register);

void mtk_idle_notifier_unregister(struct notifier_block *n)
{
	raw_notifier_chain_unregister(&mtk_idle_notifier, n);
}
EXPORT_SYMBOL_GPL(mtk_idle_notifier_unregister);

void mtk_idle_notifier_call_chain(unsigned long val)
{
	raw_notifier_call_chain(&mtk_idle_notifier, val, NULL);
}
EXPORT_SYMBOL_GPL(mtk_idle_notifier_call_chain);

#ifndef USING_STD_TIMER_OPS
/* Workaround of static analysis defect*/
int idle_gpt_get_cnt(unsigned int id, unsigned int *ptr)
{
#if 0
	unsigned int val[2] = {0};
	int ret = 0;

	ret = gpt_get_cnt(id, val);
	*ptr = val[0];

	return ret;
#else
	/* Owen 20170407 Fix Build error */
	return 0;
#endif
}

int idle_gpt_get_cmp(unsigned int id, unsigned int *ptr)
{
#if 0
	unsigned int val[2] = {0};
	int ret = 0;

	ret = gpt_get_cmp(id, val);
	*ptr = val[0];

	return ret;
#else
	/* Owen 20170407 Fix Build error */
	return 0;
#endif
}
#endif

static bool next_timer_criteria_check(unsigned int timer_criteria)
{
	unsigned int timer_left = 0;
	bool ret = true;

#ifdef USING_STD_TIMER_OPS
	struct timespec t;

	t = ktime_to_timespec(tick_nohz_get_sleep_length());
	timer_left = t.tv_sec * USEC_PER_SEC + t.tv_nsec / NSEC_PER_USEC;

	if (timer_left < timer_criteria)
		ret = false;
#else
#ifdef CONFIG_SMP
	timer_left = localtimer_get_counter();

	if ((int)timer_left < timer_criteria ||
			((int)timer_left) < 0)
		ret = false;
#else
	unsigned int timer_cmp = 0;

/* Owen 20170407 Fix Build error */
#if 0
	gpt_get_cnt(GPT1, &timer_left);
	gpt_get_cmp(GPT1, &timer_cmp);
#endif
	if ((timer_cmp - timer_left) < timer_criteria)
		ret = false;
#endif
#endif

	return ret;
}

static void timer_setting_before_wfi(bool f26m_off)
{
/* Owen 20170407 Fix Build error */
#if 0
#ifndef USING_STD_TIMER_OPS
#ifdef CONFIG_SMP
	unsigned int timer_left = 0;

	timer_left = localtimer_get_counter();

	if ((int)timer_left <= 0)
		gpt_set_cmp(IDLE_GPT, 1); /* Trigger idle_gpt Timeout imediately */
	else {
		if (f26m_off)
			gpt_set_cmp(IDLE_GPT, div_u64(timer_left, 406.25));
	else
		gpt_set_cmp(IDLE_GPT, timer_left);
	}

	if (f26m_off)
		gpt_set_clk(IDLE_GPT, GPT_CLK_SRC_RTC, GPT_CLK_DIV_1);

	start_gpt(IDLE_GPT);
#else
	gpt_get_cnt(GPT1, &timer_left);
#endif
#endif
#endif
}

static void timer_setting_after_wfi(bool f26m_off)
{
/* Owen 20170407 Fix Build error */
#if 0
#ifndef USING_STD_TIMER_OPS
#ifdef CONFIG_SMP
	if (gpt_check_and_ack_irq(IDLE_GPT)) {
		localtimer_set_next_event(1);
		if (f26m_off)
			gpt_set_clk(IDLE_GPT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1);
	} else {
		/* waked up by other wakeup source */
		unsigned int cnt, cmp;

		idle_gpt_get_cnt(IDLE_GPT, &cnt);
		idle_gpt_get_cmp(IDLE_GPT, &cmp);
		if (unlikely(cmp < cnt)) {
			idle_err("[%s]GPT%d: counter = %10u, compare = %10u\n",
					__func__, IDLE_GPT + 1, cnt, cmp);
			/* BUG(); */
		}

		if (f26m_off) {
			localtimer_set_next_event((cmp - cnt) * 1625 / 4);
			gpt_set_clk(IDLE_GPT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1);
		} else {
		localtimer_set_next_event(cmp - cnt);
		}
		stop_gpt(IDLE_GPT);
	}
#endif
#endif
#endif
}

static unsigned int idle_spm_lock;
static DEFINE_SPINLOCK(idle_spm_spin_lock);

void idle_lock_spm(enum idle_lock_spm_id id)
{
	unsigned long flags;

	spin_lock_irqsave(&idle_spm_spin_lock, flags);
	idle_spm_lock |= (1 << id);
	spin_unlock_irqrestore(&idle_spm_spin_lock, flags);
}

void idle_unlock_spm(enum idle_lock_spm_id id)
{
	unsigned long flags;

	spin_lock_irqsave(&idle_spm_spin_lock, flags);
	idle_spm_lock &= ~(1 << id);
	spin_unlock_irqrestore(&idle_spm_spin_lock, flags);
}

static unsigned int idle_ufs_lock;
static DEFINE_SPINLOCK(idle_ufs_spin_lock);

void idle_lock_by_ufs(unsigned int lock)
{
	unsigned long flags;

	spin_lock_irqsave(&idle_ufs_spin_lock, flags);
	idle_ufs_lock = lock;
	spin_unlock_irqrestore(&idle_ufs_spin_lock, flags);
}

static unsigned int idle_gpu_lock;
static DEFINE_SPINLOCK(idle_gpu_spin_lock);

void idle_lock_by_gpu(unsigned int lock)
{
	unsigned long flags;

	spin_lock_irqsave(&idle_gpu_spin_lock, flags);
	idle_gpu_lock = lock;
	spin_unlock_irqrestore(&idle_gpu_spin_lock, flags);
}

/*
 * SODI3 part
 */
static DEFINE_SPINLOCK(soidle3_condition_mask_spin_lock);

static void enable_soidle3_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&soidle3_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_SO3][grp] &= ~mask;
	spin_unlock_irqrestore(&soidle3_condition_mask_spin_lock, flags);
}

static void disable_soidle3_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&soidle3_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_SO3][grp] |= mask;
	spin_unlock_irqrestore(&soidle3_condition_mask_spin_lock, flags);
}

void enable_soidle3_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	enable_soidle3_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_soidle3_by_bit);

void disable_soidle3_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	disable_soidle3_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_soidle3_by_bit);

#ifdef CONFIG_MACH_MT6799
#define clk_readl(addr)			__raw_readl((void __force __iomem *)(addr))
#define clk_writel(addr, val)	mt_reg_sync_writel(val, addr)

static unsigned int clk_aud_intbus_sel;
void faudintbus_pll2sq(void)
{
	clk_aud_intbus_sel = clk_readl(CLK_CFG(6)) & CLK6_AUDINTBUS_MASK;
	clk_writel(CLK_CFG_CLR(6), CLK6_AUDINTBUS_MASK);
}

void faudintbus_sq2pll(void)
{
	clk_writel(CLK_CFG_SET(6), clk_aud_intbus_sel);
}

static unsigned int clk_ufs_card_sel;
static void clk_ufs_card_switch_backup(void)
{
	clk_ufs_card_sel = clk_readl(CLK_CFG(13)) & CLK13_UFS_CARD_SEL_MASK;
	clk_writel(CLK_CFG_CLR(13), CLK13_UFS_CARD_SEL_MASK);
}

static void clk_ufs_card_switch_restore(void)
{
	clk_writel(CLK_CFG_SET(13), clk_ufs_card_sel);
}
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static bool mtk_idle_cpu_criteria(void)
{
#ifndef CONFIG_MTK_ACAO_SUPPORT
	return ((atomic_read(&is_in_hotplug) == 1) || (num_online_cpus() != 1)) ? false : true;
#else
	/* single core check will be checked mcdi driver for acao case */
	return true;
#endif
}
#endif

#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
#define CPU_PWR_STATUS_MASK 0x6F78
#endif

bool mtk_idle_cpu_wfi_criteria(void)
{
	u32 sta = CPU_PWR_STATUS_MASK & ~spm_read(CPU_IDLE_STA);

	return (sta & (sta - 1)) == 0;
}

static bool soidle3_can_enter(int cpu, int reason)
{
	#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[0]);
	#endif
	#ifdef SPM_SODI_PROFILE_TIME
	gpt_get_cnt(SPM_SODI_PROFILE_APXGPT, &soidle_profile[0]);
	#endif

	/* check previous common criterion */
	if (reason == BY_CLK) {
		if (soidle3_by_pass_cg == 0) {
			if (idle_block_mask[IDLE_TYPE_SO3][NR_GRPS])
				goto out;
		}
		reason = NR_REASONS;
	} else if (reason < NR_REASONS)
		goto out;

	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (soidle3_by_pass_en == 0) {
		if ((spm_get_sodi_en() == 0) || (spm_get_sodi3_en() == 0)) {
			/* if SODI is disabled, SODI3 is also disabled */
			reason = BY_OTH;
			goto out;
		}
	}

	if (!mtk_idle_disp_is_pwm_rosc()) {
		reason = BY_PWM;
		goto out;
	}

	if (soidle3_by_pass_pll == 0) {
		if (!mtk_idle_check_pll(soidle3_pll_condition_mask, soidle3_pll_block_mask)) {
			reason = BY_PLL;
			goto out;
		}

		/* check if univpll is used (sspm not included) */
		if (univpll_is_used()) {
			reason = BY_PLL;
			goto out;
		}
	}
	#endif

	if (!next_timer_criteria_check(soidle3_time_criteria)) {
		reason = BY_TMR;
		goto out;
	}

	/* Check Secure CGs - after other SODI3 criteria PASS */
	if (!idle_by_pass_secure_cg) {
		if (!mtk_idle_check_secure_cg(idle_block_mask)) {
			reason = BY_CLK;
			goto out;
		}
	}

out:
	return mtk_idle_select_state(IDLE_TYPE_SO3, reason);
}

void soidle3_before_wfi(int cpu)
{
	timer_setting_before_wfi(true);
}

void soidle3_after_wfi(int cpu)
{
	timer_setting_after_wfi(true);

	soidle3_cnt[cpu]++;
}

/*
 * SODI part
 */
static DEFINE_SPINLOCK(soidle_condition_mask_spin_lock);

static void enable_soidle_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&soidle_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_SO][grp] &= ~mask;
	spin_unlock_irqrestore(&soidle_condition_mask_spin_lock, flags);
}

static void disable_soidle_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&soidle_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_SO][grp] |= mask;
	spin_unlock_irqrestore(&soidle_condition_mask_spin_lock, flags);
}

void enable_soidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	enable_soidle_by_mask(grp, mask);
	/* enable the settings for SODI3 at the same time */
	enable_soidle3_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_soidle_by_bit);

void disable_soidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	disable_soidle_by_mask(grp, mask);
	/* disable the settings for SODI3 at the same time */
	disable_soidle3_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_soidle_by_bit);

static bool soidle_can_enter(int cpu, int reason)
{
	#ifdef SPM_SODI_PROFILE_TIME
	gpt_get_cnt(SPM_SODI_PROFILE_APXGPT, &soidle_profile[0]);
	#endif

	/* check previous common criterion */
	if (reason == BY_CLK) {
		if (soidle_by_pass_cg == 0) {
			if (idle_block_mask[IDLE_TYPE_SO][NR_GRPS])
				goto out;
		}
		reason = NR_REASONS;
	} else if (reason < NR_REASONS)
		goto out;

	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (soidle_by_pass_en == 0) {
		if (spm_get_sodi_en() == 0) {
			reason = BY_OTH;
			goto out;
		}
	}
	#endif

	if (!next_timer_criteria_check(soidle_time_criteria)) {
		reason = BY_TMR;
		goto out;
	}

	/* Check Secure CGs - after other SODI criteria PASS */
	if (!idle_by_pass_secure_cg) {
		if (!mtk_idle_check_secure_cg(idle_block_mask)) {
			reason = BY_CLK;
			goto out;
		}
	}

out:
	return mtk_idle_select_state(IDLE_TYPE_SO, reason);
}

void soidle_before_wfi(int cpu)
{
#ifdef CONFIG_MACH_MT6799
	faudintbus_pll2sq();
#endif

	timer_setting_before_wfi(false);
}

void soidle_after_wfi(int cpu)
{
	timer_setting_after_wfi(false);

#ifdef CONFIG_MACH_MT6799
	faudintbus_sq2pll();
#endif

	soidle_cnt[cpu]++;
}

/*
 * multi-core idle part
 */
static DEFINE_MUTEX(mcidle_locked);
static bool mcidle_can_enter(int cpu, int reason)
{
	/* reset reason */
	reason = NR_REASONS;

#ifdef CONFIG_ARM64
	if (num_online_cpus() == 1) {
		reason = BY_CPU;
		goto mcidle_out;
	}
#else
	if (num_online_cpus() == 1) {
		reason = BY_CPU;
		goto mcidle_out;
	}
#endif

	if (spm_mcdi_can_enter() == 0) {
		reason = BY_OTH;
		goto mcidle_out;
	}

	if (!next_timer_criteria_check(mcidle_time_criteria)) {
		reason = BY_TMR;
		goto mcidle_out;
	}

mcidle_out:
	if (reason < NR_REASONS) {
		mcidle_block_cnt[cpu][reason]++;
		return false;
	}

	return true;
}

void mcidle_before_wfi(int cpu)
{
}

int mcdi_xgpt_wakeup_cnt[NR_CPUS];
void mcidle_after_wfi(int cpu)
{
}

/*
 * MC-SODI part
 */
static DEFINE_SPINLOCK(mcsodi_condition_mask_spin_lock);

static void enable_mcsodi_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_MCSO][grp] &= ~mask;
	spin_unlock_irqrestore(&mcsodi_condition_mask_spin_lock, flags);
}

static void disable_mcsodi_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&mcsodi_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_MCSO][grp] |= mask;
	spin_unlock_irqrestore(&mcsodi_condition_mask_spin_lock, flags);
}

void enable_mcsodi_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	enable_mcsodi_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_mcsodi_by_bit);

void disable_mcsodi_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	disable_mcsodi_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_mcsodi_by_bit);

static bool mcsodi_can_enter(int cpu, int reason)
{
	if (reason == BY_CPU) {
		if (!mtk_idle_cpu_wfi_criteria()) {
			reason = BY_CPU;
			goto out;
		}
		/* if by_cpu, previous common not check cg yet */
		if (!mtk_idle_check_cg(idle_block_mask))
			reason = BY_CLK;
	}

	if (reason == BY_CLK) {
		if (mcsodi_by_pass_cg == false) {
			if (idle_block_mask[IDLE_TYPE_MCSO][NR_GRPS])
				goto out;
		}
	} else if (reason < NR_REASONS) {
		goto out;
	}

	reason = NR_REASONS;

	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (mcsodi_by_pass_en == false) {
		/* check disp ready*/
		if (spm_get_sodi_en() == 0) {
			reason = BY_OTH;
			goto out;
		}
	}
	#endif

	if (!next_timer_criteria_check(mcsodi_time_criteria)) {
		reason = BY_TMR;
		goto out;
	}

	/* Check Secure CGs - after other MC-SODI criteria PASS */
	if (!idle_by_pass_secure_cg) {
		if (!mtk_idle_check_secure_cg(idle_block_mask)) {
			reason = BY_CLK;
			goto out;
		}
	}
out:
	return mtk_idle_select_state(IDLE_TYPE_MCSO, reason);
}

void mcsodi_before_wfi(int cpu)
{
}

void mcsodi_after_wfi(int cpu)
{
}

/*
 * deep idle part
 */
static DEFINE_SPINLOCK(dpidle_condition_mask_spin_lock);

static void enable_dpidle_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&dpidle_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_DP][grp] &= ~mask;
	spin_unlock_irqrestore(&dpidle_condition_mask_spin_lock, flags);
}

static void disable_dpidle_by_mask(int grp, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&dpidle_condition_mask_spin_lock, flags);
	idle_condition_mask[IDLE_TYPE_DP][grp] |= mask;
	spin_unlock_irqrestore(&dpidle_condition_mask_spin_lock, flags);
}

void enable_dpidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	enable_dpidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_dpidle_by_bit);

void disable_dpidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	disable_dpidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_dpidle_by_bit);

static bool dpidle_can_enter(int cpu, int reason)
{
	/* check previous common criterion */
	if (reason == BY_CLK) {
		if (dpidle_by_pass_cg == 0) {
			if (idle_block_mask[IDLE_TYPE_DP][NR_GRPS])
				goto out;
		}
	} else if (reason < NR_REASONS)
		goto out;

	reason = NR_REASONS;

	/* Check Secure CGs - after other deepidle criteria PASS */
	if (!idle_by_pass_secure_cg) {
		if (!mtk_idle_check_secure_cg(idle_block_mask)) {
			reason = BY_CLK;
			goto out;
		}
	}

out:
	return mtk_idle_select_state(IDLE_TYPE_DP, reason);
}

/*
 * slow idle part
 */
static DEFINE_MUTEX(slidle_locked);

static void enable_slidle_by_mask(int grp, unsigned int mask)
{
	mutex_lock(&slidle_locked);
	idle_condition_mask[IDLE_TYPE_SL][grp] &= ~mask;
	mutex_unlock(&slidle_locked);
}

static void disable_slidle_by_mask(int grp, unsigned int mask)
{
	mutex_lock(&slidle_locked);
	idle_condition_mask[IDLE_TYPE_SL][grp] |= mask;
	mutex_unlock(&slidle_locked);
}

void enable_slidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	enable_slidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_slidle_by_bit);

void disable_slidle_by_bit(int id)
{
	int grp = id / 32;
	unsigned int mask = 1U << (id % 32);

	WARN_ON(INVALID_GRP_ID(grp));
	disable_slidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_slidle_by_bit);

static bool slidle_can_enter(int cpu, int reason)
{
	/* check previous common criterion */
	/* reset reason if reason is not BY_CLK */
	if (reason == BY_CLK) {
		if (idle_block_mask[IDLE_TYPE_SL][NR_GRPS])
			goto out;
	}

	reason = NR_REASONS;

	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!mtk_idle_cpu_criteria()) {
		reason = BY_CPU;
		goto out;
	}
	#endif

out:
	if (reason < NR_REASONS) {
		slidle_block_cnt[reason]++;
		return false;
	} else {
		return true;
	}
}

static void slidle_before_wfi(int cpu)
{
	/* struct mtk_irq_mask mask; */
	bus_dcm_enable();
}

static void slidle_after_wfi(int cpu)
{
	bus_dcm_disable();
	slidle_cnt[cpu]++;
}

static void go_to_slidle(int cpu)
{
	slidle_before_wfi(cpu);

	mb();	/* memory barrier */
	__asm__ __volatile__("wfi" : : : "memory");

	slidle_after_wfi(cpu);
}


/*
 * regular idle part
 */
static bool rgidle_can_enter(int cpu, int reason)
{
	return true;
}

static void rgidle_before_wfi(int cpu)
{

}

static void rgidle_after_wfi(int cpu)
{
	if (spm_mcsodi_start() && (spm_leave_mcsodi(cpu, 0) == 0))
		mcsodi_cnt[cpu]++;
	else
		rgidle_cnt[cpu]++;
}

static noinline void go_to_rgidle(int cpu)
{
	rgidle_before_wfi(cpu);

	go_to_wfi();

	rgidle_after_wfi(cpu);
}

/*
 * idle task flow part
 */

/*
 * xxidle_handler return 1 if enter and exit the low power state
 */

static u32 slp_spm_SODI3_flags = {
	SPM_FLAG_DIS_INFRA_PDN |
	SPM_FLAG_DIS_VCORE_DVS |
	SPM_FLAG_DIS_VCORE_DFS |
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)
	SPM_FLAG_DIS_PERI_PDN |
#endif
	SPM_FLAG_ENABLE_ATF_ABORT |
#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	SPM_FLAG_DIS_SSPM_SRAM_SLEEP |
#endif
	SPM_FLAG_SODI_OPTION |
	SPM_FLAG_ENABLE_SODI3
};

static u32 slp_spm_SODI_flags = {
	SPM_FLAG_DIS_INFRA_PDN |
	SPM_FLAG_DIS_VCORE_DVS |
	SPM_FLAG_DIS_VCORE_DFS |
#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)
	SPM_FLAG_DIS_PERI_PDN |
#endif
	SPM_FLAG_ENABLE_ATF_ABORT |
#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	SPM_FLAG_DIS_SSPM_SRAM_SLEEP |
#endif
	SPM_FLAG_SODI_OPTION
};

#if defined(CONFIG_MACH_MT6799)

u32 slp_spm_deepidle_flags = {
	SPM_FLAG_DIS_INFRA_PDN |
	SPM_FLAG_DIS_VCORE_DVS |
	SPM_FLAG_DIS_VCORE_DFS |
	SPM_FLAG_DIS_PERI_PDN |
#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	SPM_FLAG_DIS_SSPM_SRAM_SLEEP |
#endif
	SPM_FLAG_DEEPIDLE_OPTION
};

#elif defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)

u32 slp_spm_deepidle_flags = {
	SPM_FLAG_DIS_INFRA_PDN |
	SPM_FLAG_DIS_VCORE_DVS |
	SPM_FLAG_DIS_VCORE_DFS |
	SPM_FLAG_ENABLE_ATF_ABORT |
#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	SPM_FLAG_DIS_SSPM_SRAM_SLEEP |
#endif
	SPM_FLAG_DEEPIDLE_OPTION
};

#endif /* platform difference */

static u32 slp_spm_MCSODI_flags = {
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
	SPM_FLAG_ENABLE_MCSODI |
#endif
	SPM_FLAG_RUN_COMMON_SCENARIO
};

unsigned int ufs_cb_before_xxidle(void)
{
#if defined(CONFIG_MTK_UFS_BOOTING)
	unsigned int op_cond = 0;
	int ufs_in_hibernate = -1;

	/* Turn OFF/ON XO_UFS only when UFS_H8 */
	ufs_in_hibernate = ufs_mtk_deepidle_hibern8_check();
	op_cond = (ufs_in_hibernate == UFS_H8) ? DEEPIDLE_OPT_XO_UFS_ON_OFF : 0;
	op_cond |= (ufs_in_hibernate == UFS_H8) ? DEEPIDLE_OPT_UFSCARD_MUX_SWITCH : 0;

	return op_cond;
#else
	return 0;
#endif
}

void ufs_cb_after_xxidle(void)
{
#if defined(CONFIG_MTK_UFS_BOOTING)
	ufs_mtk_deepidle_leave();
#endif
}

static inline unsigned int soidle_pre_handler(void)
{
	unsigned int op_cond = 0;

	op_cond = ufs_cb_before_xxidle();


#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifndef CONFIG_MTK_ACAO_SUPPORT
	hps_del_timer();
#endif
#endif

#ifdef CONFIG_THERMAL
	/* cancel thermal hrtimer for power saving */
	mtkTTimer_cancel_timer();
#endif
	return op_cond;
}

static inline void soidle_post_handler(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifndef CONFIG_MTK_ACAO_SUPPORT
	hps_restart_timer();
#endif
#endif

#ifdef CONFIG_THERMAL
	/* restart thermal hrtimer for update temp info */
	mtkTTimer_start_timer();
#endif

	ufs_cb_after_xxidle();
}

static unsigned int dpidle_pre_process(int cpu)
{
	unsigned int op_cond = 0;

	op_cond = ufs_cb_before_xxidle();

	mtk_idle_notifier_call_chain(NOTIFY_DPIDLE_ENTER);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifndef CONFIG_MTK_ACAO_SUPPORT
	hps_del_timer();
#endif

#ifdef CONFIG_THERMAL
	/* cancel thermal hrtimer for power saving */
	mtkTTimer_cancel_timer();
#endif

#ifdef CONFIG_MACH_MT6799
	if (op_cond & DEEPIDLE_OPT_UFSCARD_MUX_SWITCH)
		clk_ufs_card_switch_backup();
	faudintbus_pll2sq();
#endif
#endif

	timer_setting_before_wfi(false);

	/* Check clkmux condition after */
	memset(clkmux_block_mask[IDLE_TYPE_DP],	0, NF_CLK_CFG * sizeof(unsigned int));
	clkmux_cond[IDLE_TYPE_DP] = mtk_idle_check_clkmux(IDLE_TYPE_DP, clkmux_block_mask);

	return op_cond;
}

static void dpidle_post_process(int cpu, unsigned int op_cond)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	timer_setting_after_wfi(false);

#ifdef CONFIG_MACH_MT6799
	faudintbus_sq2pll();

	if (op_cond & DEEPIDLE_OPT_UFSCARD_MUX_SWITCH)
		clk_ufs_card_switch_restore();
#endif

#ifndef CONFIG_MTK_ACAO_SUPPORT
	hps_restart_timer();
#endif

#ifdef CONFIG_THERMAL
	/* restart thermal hrtimer for update temp info */
	mtkTTimer_start_timer();
#endif
#endif

	mtk_idle_notifier_call_chain(NOTIFY_DPIDLE_LEAVE);

	ufs_cb_after_xxidle();

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	spm_dpidle_notify_sspm_after_wfi_async_wait();
#endif

	dpidle_cnt[cpu]++;
}

static bool (*idle_can_enter[NR_TYPES])(int cpu, int reason) = {
	dpidle_can_enter,
	soidle3_can_enter,
	soidle_can_enter,
	mcidle_can_enter,
	mcsodi_can_enter,
	slidle_can_enter,
	rgidle_can_enter,
};

/* Mapping idle_switch to CPUidle C States */
static int idle_stat_mapping_table[NR_TYPES] = {
	CPUIDLE_STATE_DP,
	CPUIDLE_STATE_SO3,
	CPUIDLE_STATE_SO,
	CPUIDLE_STATE_MC,
	CPUIDLE_STATE_MCSO,
	CPUIDLE_STATE_SL,
	CPUIDLE_STATE_RG
};

int mtk_idle_select(int cpu)
{
	int i, reason = NR_REASONS;
#if defined(CONFIG_MTK_UFS_BOOTING)
	unsigned long flags = 0;
	unsigned int ufs_locked;
#endif
#ifdef CONFIG_MTK_DCS
	int ch = 0, ret = -1;
	enum dcs_status dcs_status;
	bool dcs_lock_get = false;
#endif
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	unsigned int mtk_idle_switch;
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* check if firmware loaded or not */
	if (!spm_load_firmware_status()) {
		reason = BY_FRM;
		goto get_idle_idx_2;
	}
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_SMP
	/* check cpu status */
	if (!mtk_idle_cpu_criteria()) {
		reason = BY_CPU;
		goto get_idle_idx_2;
	}
#endif
#endif

#ifndef CONFIG_MTK_ACAO_SUPPORT
	/* only check for non-acao case */
	if (cpu % 4) {
		reason = BY_CPU;
		goto get_idle_idx_2;
	}
#endif

	if (spm_get_resource_usage() == SPM_RESOURCE_ALL) {
		reason = BY_OTH;
		goto get_idle_idx_2;
	}

#if defined(CONFIG_MTK_UFS_BOOTING)
	spin_lock_irqsave(&idle_ufs_spin_lock, flags);
	ufs_locked = idle_ufs_lock;
	spin_unlock_irqrestore(&idle_ufs_spin_lock, flags);

	if (ufs_locked) {
		reason = BY_UFS;
		goto get_idle_idx_2;
	}
#endif

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	if (is_sspm_ipi_lock_spm()) {
		reason = BY_SSPM_IPI;
		goto get_idle_idx_2;
	}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

	/* check idle_spm_lock */
	if (idle_spm_lock) {
		reason = BY_VTG;
		goto get_idle_idx_2;
	}

	/* teei ready */
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MICROTRUST_TEE_SUPPORT
	if (!is_teei_ready()) {
		reason = BY_OTH;
		goto get_idle_idx_2;
	}
#endif
#endif

#ifdef CONFIG_MTK_DCS
	/* check if DCS channel switching */
	ret = dcs_get_dcs_status_trylock(&ch, &dcs_status);
	if (ret) {
		reason = BY_DCS;
		goto get_idle_idx_2;
	}

	dcs_lock_get = true;
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* cg check */
	mtk_idle_switch = 0;
	for (i = 0; i < NR_TYPES - 1; i++)
		mtk_idle_switch |= ((!!idle_switch[i]) << i);
	if (!mtk_idle_switch)
		goto get_idle_idx_2;

	memset(idle_block_mask, 0,
		NR_TYPES * NF_CG_STA_RECORD * sizeof(unsigned int));
	if (!mtk_idle_check_cg(idle_block_mask)) {
		reason = BY_CLK;
		goto get_idle_idx_2;
	}
#endif

get_idle_idx_2:
	/* check if criteria check fail in common part */
	for (i = 0; i < NR_TYPES; i++) {
		if (idle_switch[i]) {
			/* call each idle scenario check functions */
			if (idle_can_enter[i](cpu, reason))
				break;
		}
	}

	/* Prevent potential out-of-bounds vulnerability */
	i = (i >= NR_TYPES) ? (NR_TYPES - 1) : i;

#ifdef CONFIG_MTK_DCS
	if (dcs_lock_get)
		dcs_get_dcs_status_unlock();
#endif
	return i;
}

int mtk_idle_select_base_on_menu_gov(int cpu, int menu_select_state)
{
	int i = NR_TYPES - 1;
	int state = CPUIDLE_STATE_RG;

	if (menu_select_state < 0)
		return menu_select_state;

	mtk_idle_dump_cnt_in_interval();
#ifdef CONFIG_MTK_ACAO_SUPPORT
	mcdi_heart_beat_log_dump();
#endif

	i = mtk_idle_select(cpu);

	/* residency requirement of ALL C state is satisfied */
	if (menu_select_state == CPUIDLE_STATE_SO3) {
		state = idle_stat_mapping_table[i];
	/* SODI3.0 residency requirement does NOT satisfied */
	} else if (menu_select_state >= CPUIDLE_STATE_SO) {
		if (i == IDLE_TYPE_SO3)
			i = idle_switch[IDLE_TYPE_SO] ? IDLE_TYPE_SO :
				(idle_switch[IDLE_TYPE_MCSO] ? IDLE_TYPE_MCSO : IDLE_TYPE_RG);

		state = idle_stat_mapping_table[i];
	/* DPIDLE, SODI3.0, and SODI residency requirement does NOT satisfied */
	} else if (menu_select_state >= CPUIDLE_STATE_MCSO) {
		if (i <= IDLE_TYPE_SO)
			i = idle_switch[IDLE_TYPE_MCSO] ? IDLE_TYPE_MCSO : IDLE_TYPE_RG;
		state = idle_stat_mapping_table[i];
	} else {
		state = CPUIDLE_STATE_RG;
	}

	return state;
}

int dpidle_enter(int cpu)
{
	int ret = CPUIDLE_STATE_DP;
	u32 operation_cond = 0;

	mtk_idle_ratio_calc_start(IDLE_TYPE_DP, cpu);

	operation_cond |= dpidle_pre_process(cpu);

	if (dpidle_gs_dump_req) {
		unsigned int current_ts = idle_get_current_time_ms();

		if ((current_ts - dpidle_gs_dump_req_ts) >= dpidle_gs_dump_delay_ms) {
			idle_warn("dpidle dump LP golden\n");

			dpidle_gs_dump_req = 0;
			operation_cond |= DEEPIDLE_OPT_DUMP_LP_GOLDEN;
		}
	}

	operation_cond |= dpidle_force_vcore_lp_mode ? DEEPIDLE_OPT_VCORE_LP_MODE :
						(clkmux_cond[IDLE_TYPE_DP] ? DEEPIDLE_OPT_VCORE_LP_MODE : 0);

	spm_go_to_dpidle(slp_spm_deepidle_flags, (u32)cpu, dpidle_dump_log, operation_cond);

	dpidle_post_process(cpu, operation_cond);

	mtk_idle_ratio_calc_stop(IDLE_TYPE_DP, cpu);

	/* For test */
	if (dpidle_run_once)
		idle_switch[IDLE_TYPE_DP] = 0;

	return ret;
}
EXPORT_SYMBOL(dpidle_enter);

int soidle3_enter(int cpu)
{
	int ret = CPUIDLE_STATE_SO3;
	u32 operation_cond = 0;
	unsigned long long soidle3_time = 0;
	static unsigned long long soidle3_residency;

	if (sodi3_flags & SODI_FLAG_RESIDENCY)
		soidle3_time = idle_get_current_time_ms();

	mtk_idle_ratio_calc_start(IDLE_TYPE_SO3, cpu);
	mtk_idle_notifier_call_chain(NOTIFY_SOIDLE3_ENTER);

#ifdef CONFIG_MACH_MT6799
	/* switch audio clock to allow vcore lp mode */
	faudintbus_pll2sq();
	/* backup and clear ufs_card_sel to 0 */
	if (operation_cond & DEEPIDLE_OPT_UFSCARD_MUX_SWITCH)
		clk_ufs_card_switch_backup();
#endif

	operation_cond |= soidle_pre_handler();

	/* clkmux for sodi3 */
	memset(clkmux_block_mask[IDLE_TYPE_SO3], 0, NF_CLK_CFG * sizeof(unsigned int));
	clkmux_cond[IDLE_TYPE_SO3] = mtk_idle_check_clkmux(IDLE_TYPE_SO3, clkmux_block_mask);

#ifdef DEFAULT_MMP_ENABLE
	mmprofile_log_ex(sodi_mmp_get_events()->sodi_enable, MMPROFILE_FLAG_START, 0, 0);
#endif /* DEFAULT_MMP_ENABLE */

	operation_cond |= clkmux_cond[IDLE_TYPE_SO3] ? DEEPIDLE_OPT_VCORE_LP_MODE : 0x0;

	if (sodi3_force_vcore_lp_mode)
		operation_cond |= DEEPIDLE_OPT_VCORE_LP_MODE;

	spm_go_to_sodi3(slp_spm_SODI3_flags, (u32)cpu, sodi3_flags, operation_cond);

	/* Clear SODI_FLAG_DUMP_LP_GS in sodi3_flags */
	sodi3_flags &= (~SODI_FLAG_DUMP_LP_GS);

#ifdef DEFAULT_MMP_ENABLE
	mmprofile_log_ex(sodi_mmp_get_events()->sodi_enable, MMPROFILE_FLAG_END, 0, spm_read(SPM_PASR_DPD_3));
#endif /* DEFAULT_MMP_ENABLE */

	soidle_post_handler();

#ifdef CONFIG_MACH_MT6799
	/* restore ufs_card_sel */
	if (operation_cond & DEEPIDLE_OPT_UFSCARD_MUX_SWITCH)
		clk_ufs_card_switch_restore();
	faudintbus_sq2pll();
#endif

	mtk_idle_notifier_call_chain(NOTIFY_SOIDLE3_LEAVE);
	mtk_idle_ratio_calc_stop(IDLE_TYPE_SO3, cpu);

	if (sodi3_flags & SODI_FLAG_RESIDENCY) {
		soidle3_residency += idle_get_current_time_ms() - soidle3_time;
		idle_dbg("SO3: soidle3_residency = %llu\n", soidle3_residency);
	}

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[3]);
	idle_ver("SODI3: cpu_freq:%u/%u, 1=>2:%u, 2=>3:%u, 3=>4:%u\n",
			mt_cpufreq_get_cur_freq(0), mt_cpufreq_get_cur_freq(1),
			soidle3_profile[1] - soidle3_profile[0],
			soidle3_profile[2] - soidle3_profile[1],
			soidle3_profile[3] - soidle3_profile[2]);
#endif

	return ret;
}
EXPORT_SYMBOL(soidle3_enter);

int soidle_enter(int cpu)
{
	int ret = CPUIDLE_STATE_SO;
	u32 operation_cond = 0;
	unsigned long long soidle_time = 0;
	static unsigned long long soidle_residency;

	if (sodi_flags & SODI_FLAG_RESIDENCY)
		soidle_time = idle_get_current_time_ms();

	mtk_idle_ratio_calc_start(IDLE_TYPE_SO, cpu);
	mtk_idle_notifier_call_chain(NOTIFY_SOIDLE_ENTER);

	operation_cond |= soidle_pre_handler();

#ifdef DEFAULT_MMP_ENABLE
	mmprofile_log_ex(sodi_mmp_get_events()->sodi_enable, MMPROFILE_FLAG_START, 0, 0);
#endif /* DEFAULT_MMP_ENABLE */

	spm_go_to_sodi(slp_spm_SODI_flags, (u32)cpu, sodi_flags, operation_cond);

	/* Clear SODI_FLAG_DUMP_LP_GS in sodi_flags */
	sodi_flags &= (~SODI_FLAG_DUMP_LP_GS);

#ifdef DEFAULT_MMP_ENABLE
	mmprofile_log_ex(sodi_mmp_get_events()->sodi_enable, MMPROFILE_FLAG_END, 0, spm_read(SPM_PASR_DPD_3));
#endif /* DEFAULT_MMP_ENABLE */

	soidle_post_handler();

	mtk_idle_notifier_call_chain(NOTIFY_SOIDLE_LEAVE);
	mtk_idle_ratio_calc_stop(IDLE_TYPE_SO, cpu);

	if (sodi_flags & SODI_FLAG_RESIDENCY) {
		soidle_residency += idle_get_current_time_ms() - soidle_time;
		idle_dbg("SO: soidle_residency = %llu\n", soidle_residency);
	}

#ifdef SPM_SODI_PROFILE_TIME
	gpt_get_cnt(SPM_SODI_PROFILE_APXGPT, &soidle_profile[3]);
	idle_ver("SODI: cpu_freq:%u/%u, 1=>2:%u, 2=>3:%u, 3=>4:%u\n",
			mt_cpufreq_get_cur_freq(0), mt_cpufreq_get_cur_freq(1),
			soidle_profile[1] - soidle_profile[0],
			soidle_profile[2] - soidle_profile[1],
			soidle_profile[3] - soidle_profile[2]);
#endif

	return ret;
}
EXPORT_SYMBOL(soidle_enter);

int mcidle_enter(int cpu)
{
	int ret = CPUIDLE_STATE_MC;

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	go_to_mcidle(cpu);
	mcidle_cnt[cpu] += 1;
#endif

	return ret;
}
EXPORT_SYMBOL(mcidle_enter);

int mcsodi_enter(int cpu)
{
	int ret = CPUIDLE_STATE_MCSO;

	spm_go_to_mcsodi(slp_spm_MCSODI_flags, (u32)cpu, sodi_flags, 0);

	if (spm_leave_mcsodi(cpu, 0) == 0)
		mcsodi_cnt[cpu]++;

	return ret;
}
EXPORT_SYMBOL(mcsodi_enter);

int slidle_enter(int cpu)
{
	int ret = CPUIDLE_STATE_SL;

	go_to_slidle(cpu);

	return ret;
}
EXPORT_SYMBOL(slidle_enter);

int rgidle_enter(int cpu)
{
	int ret = CPUIDLE_STATE_RG;

	mtk_idle_ratio_calc_start(IDLE_TYPE_RG, cpu);

	go_to_rgidle(cpu);

	mtk_idle_ratio_calc_stop(IDLE_TYPE_RG, cpu);

	return ret;
}
EXPORT_SYMBOL(rgidle_enter);

/*
 * debugfs
 */
static char dbg_buf[4096] = { 0 };
static char cmd_buf[512] = { 0 };

#undef mt_idle_log
#define mt_idle_log(fmt, args...)	log2buf(p, dbg_buf, fmt, ##args)

/* idle_state */
static int _idle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int idle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _idle_state_open, inode->i_private);
}

static ssize_t idle_state_read(struct file *filp,
			       char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i, len = 0;
	char *p = dbg_buf;

	mt_idle_log("********** idle state dump **********\n");

	for (i = 0; i < nr_cpu_ids; i++) {
		mt_idle_log("dpidle_cnt[%d]=%lu, dpidle_26m[%d]=%lu, soidle3_cnt[%d]=%lu, soidle_cnt[%d]=%lu, ",
			i, dpidle_cnt[i], i, dpidle_f26m_cnt[i], i, soidle3_cnt[i], i, soidle_cnt[i]);
		mt_idle_log("mcidle_cnt[%d]=%lu, mcsodi_cnt[%d]=%lu, slidle_cnt[%d]=%lu, rgidle_cnt[%d]=%lu\n",
			i, mcidle_cnt[i], i, mcsodi_cnt[i], i, slidle_cnt[i], i, rgidle_cnt[i]);
	}

	mt_idle_log("\n********** variables dump **********\n");
	for (i = 0; i < NR_TYPES; i++)
		mt_idle_log("%s_switch=%d, ", mtk_get_idle_name(i), idle_switch[i]);

	mt_idle_log("\n");
	mt_idle_log("idle_ratio_en = %u\n", mtk_idle_get_ratio_status());
	mt_idle_log("twam_handler:%s (clk:%s)\n",
					(mtk_idle_get_twam()->running)?"on":"off",
					(mtk_idle_get_twam()->speed_mode)?"speed":"normal");

	mt_idle_log("bypass_secure_cg = %u\n", idle_by_pass_secure_cg);

	mt_idle_log("\n********** idle command help **********\n");
	mt_idle_log("status help:   cat /sys/kernel/debug/cpuidle/idle_state\n");
	mt_idle_log("switch on/off: echo switch mask > /sys/kernel/debug/cpuidle/idle_state\n");
	mt_idle_log("idle ratio profile: echo ratio 1/0 > /sys/kernel/debug/cpuidle/idle_state\n");

	mt_idle_log("soidle3 help:  cat /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("soidle help:   cat /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("dpidle help:   cat /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("mcidle help:   cat /sys/kernel/debug/cpuidle/mcidle_state\n");
	mt_idle_log("slidle help:   cat /sys/kernel/debug/cpuidle/slidle_state\n");
	mt_idle_log("rgidle help:   cat /sys/kernel/debug/cpuidle/rgidle_state\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t idle_state_write(struct file *filp,
				const char __user *userbuf, size_t count, loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int idx;
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %x", cmd, &param) == 2) {
		if (!strcmp(cmd, "switch")) {
			for (idx = 0; idx < NR_TYPES; idx++)
				idle_switch[idx] = (param & (1U << idx)) ? 1 : 0;
		} else if (!strcmp(cmd, "ratio")) {
			if (param == 1)
				mtk_idle_enable_ratio_calc();
			else
				mtk_idle_disable_ratio_calc();
		} else if (!strcmp(cmd, "spmtwam_clk")) {
			mtk_idle_get_twam()->speed_mode = param;
		} else if (!strcmp(cmd, "spmtwam_sel")) {
			mtk_idle_get_twam()->sel = param;
		} else if (!strcmp(cmd, "spmtwam")) {
#if !defined(CONFIG_FPGA_EARLY_PORTING)
			idle_dbg("spmtwam_event = %d\n", param);
			if (param >= 0)
				mtk_idle_twam_enable((u32)param);
			else
				mtk_idle_twam_disable();
#endif
		} else if (!strcmp(cmd, "bypass_secure_cg")) {
			idle_by_pass_secure_cg = param;
		}
		return count;
	}

	return -EINVAL;
}

static const struct file_operations idle_state_fops = {
	.open = idle_state_open,
	.read = idle_state_read,
	.write = idle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* mcidle_state */
static int _mcidle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int mcidle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _mcidle_state_open, inode->i_private);
}

static ssize_t mcidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int len = 0;
	int cpus, reason;
	char *p = dbg_buf;

	mt_idle_log("*********** deep idle state ************\n");
	mt_idle_log("mcidle_time_criteria=%u\n", mcidle_time_criteria);

	for (cpus = 0; cpus < nr_cpu_ids; cpus++) {
		mt_idle_log("cpu:%d\n", cpus);
		for (reason = 0; reason < NR_REASONS; reason++) {
			mt_idle_log("[%d]mcidle_block_cnt[%s]=%lu\n",
				reason, mtk_get_reason_name(reason), mcidle_block_cnt[cpus][reason]);
		}
		mt_idle_log("\n");
	}

	mt_idle_log("\n********** mcidle command help **********\n");
	mt_idle_log("mcidle help:   cat /sys/kernel/debug/cpuidle/mcidle_state\n");
	mt_idle_log("switch on/off: echo [mcidle] 1/0 > /sys/kernel/debug/cpuidle/mcidle_state\n");
	mt_idle_log("modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/mcidle_state\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcidle_state_write(struct file *filp,
				  const char __user *userbuf,
				  size_t count,
				  loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "mcidle"))
			idle_switch[IDLE_TYPE_MC] = param;
		else if (!strcmp(cmd, "time"))
			mcidle_time_criteria = param;

		return count;
	} else if (!kstrtoint(cmd_buf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_MC] = param;

		return count;
	}

	return -EINVAL;
}

static const struct file_operations mcidle_state_fops = {
	.open = mcidle_state_open,
	.read = mcidle_state_read,
	.write = mcidle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* dpidle_state */
static int _dpidle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int dpidle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _dpidle_state_open, inode->i_private);
}

static ssize_t dpidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i, k, len = 0;
	char *p = dbg_buf;

	mt_idle_log("*********** deep idle state ************\n");
	mt_idle_log("dpidle_time_criteria=%u\n", dpidle_time_criteria);

	for (i = 0; i < NR_REASONS; i++)
		mt_idle_log("[%d]dpidle_block_cnt[%s]=%lu\n", i, mtk_get_reason_name(i), dpidle_block_cnt[i]);
	mt_idle_log("\n");

	for (i = 0; i < NR_GRPS; i++) {
		mt_idle_log("[%02d]dpidle_condition_mask[%-8s]=0x%08x\t\tdpidle_block_mask[%-8s]=0x%08x\n", i,
				mtk_get_cg_group_name(i), idle_condition_mask[IDLE_TYPE_DP][i],
				mtk_get_cg_group_name(i), idle_block_mask[IDLE_TYPE_DP][i]);
	}

	mt_idle_log("dpidle pg_stat=0x%08x\n", idle_block_mask[IDLE_TYPE_DP][NR_GRPS + 1]);

	for (i = 0; i < NR_GRPS; i++)
		for (k = 0; k < 32; k++)
			dpidle_blocking_stat[i][k] = 0;

	mt_idle_log("dpidle_clkmux_cond = %d\n", clkmux_cond[IDLE_TYPE_DP]);
	for (i = 0; i < NF_CLK_CFG; i++)
		mt_idle_log("[%02d]block_cond(0x%08x)=0x%08x\n",
							i,
							clkmux_addr[i],
							clkmux_block_mask[IDLE_TYPE_DP][i]);

	mt_idle_log("dpidle_by_pass_cg=%u\n", dpidle_by_pass_cg);
	mt_idle_log("dpidle_by_pass_pg=%u\n", dpidle_by_pass_pg);
	mt_idle_log("dpidle_dump_log = %u\n", dpidle_dump_log);
	mt_idle_log("([0]: Reduced, [1]: Full, [2]: resource_usage\n");
	mt_idle_log("force VCORE lp mode = %u\n", dpidle_force_vcore_lp_mode);

	mt_idle_log("\n*********** dpidle command help  ************\n");
	mt_idle_log("dpidle help:   cat /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("switch on/off: echo [dpidle] 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("cpupdn on/off: echo cpupdn 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/dpidle_state\n");
	mt_idle_log("bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t dpidle_state_write(struct file *filp,
									const char __user *userbuf,
									size_t count,
									loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "dpidle"))
			idle_switch[IDLE_TYPE_DP] = param;
		else if (!strcmp(cmd, "enable"))
			enable_dpidle_by_bit(param);
		else if (!strcmp(cmd, "once"))
			dpidle_run_once = param;
		else if (!strcmp(cmd, "disable"))
			disable_dpidle_by_bit(param);
		else if (!strcmp(cmd, "time"))
			dpidle_time_criteria = param;
		else if (!strcmp(cmd, "bypass"))
			dpidle_by_pass_cg = param;
		else if (!strcmp(cmd, "bypass_pg")) {
			dpidle_by_pass_pg = param;
			idle_warn("bypass_pg = %d\n", dpidle_by_pass_pg);
		} else if (!strcmp(cmd, "golden")) {
			dpidle_gs_dump_req = param;

			if (dpidle_gs_dump_req)
				dpidle_gs_dump_req_ts = idle_get_current_time_ms();
		} else if (!strcmp(cmd, "golden_delay_ms")) {
			dpidle_gs_dump_delay_ms = (param >= 0) ? param : 0;
		} else if (!strcmp(cmd, "log"))
			dpidle_dump_log = param;
		else if (!strcmp(cmd, "force_vcore"))
			dpidle_force_vcore_lp_mode = param;

		return count;
	} else if (!kstrtoint(cmd_buf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_DP] = param;

		return count;
	}

	return -EINVAL;
}

static const struct file_operations dpidle_state_fops = {
	.open = dpidle_state_open,
	.read = dpidle_state_read,
	.write = dpidle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* soidle3_state */
static int _soidle3_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int soidle3_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _soidle3_state_open, inode->i_private);
}

static ssize_t soidle3_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i, len = 0;
	char *p = dbg_buf;

	mt_idle_log("*********** soidle3 state ************\n");
	mt_idle_log("soidle3_time_criteria=%u\n", soidle3_time_criteria);

	for (i = 0; i < NR_REASONS; i++)
		mt_idle_log("[%d]soidle3_block_cnt[%s]=%lu\n", i, mtk_get_reason_name(i), soidle3_block_cnt[i]);
	mt_idle_log("\n");

	for (i = 0; i < NR_PLLS; i++) {
		mt_idle_log("[%02d]soidle3_pll_condition_mask[%-8s]=0x%08x\t\tsoidle3_pll_block_mask[%-8s]=0x%08x\n", i,
			mtk_get_pll_group_name(i), soidle3_pll_condition_mask[i],
			mtk_get_pll_group_name(i), soidle3_pll_block_mask[i]);
	}
	mt_idle_log("\n");

	for (i = 0; i < NR_GRPS; i++) {
		mt_idle_log("[%02d]soidle3_condition_mask[%-8s]=0x%08x\t\tsoidle3_block_mask[%-8s]=0x%08x\n", i,
			mtk_get_cg_group_name(i), idle_condition_mask[IDLE_TYPE_SO3][i],
			mtk_get_cg_group_name(i), idle_block_mask[IDLE_TYPE_SO3][i]);
	}

	mt_idle_log("soidle3 pg_stat=0x%08x\n", idle_block_mask[IDLE_TYPE_SO3][NR_GRPS + 1]);

	mt_idle_log("sodi3_clkmux_cond = %d\n",  clkmux_cond[IDLE_TYPE_SO3]);
	for (i = 0; i < NF_CLK_CFG; i++)
		mt_idle_log("[%02d]block_cond(0x%08x)=0x%08x\n",
							i,
							clkmux_addr[i],
							clkmux_block_mask[IDLE_TYPE_SO3][i]);

	mt_idle_log("soidle3_bypass_pll=%u\n", soidle3_by_pass_pll);
	mt_idle_log("soidle3_bypass_cg=%u\n", soidle3_by_pass_cg);
	mt_idle_log("soidle3_bypass_en=%u\n", soidle3_by_pass_en);
	mt_idle_log("sodi3_flags=0x%x\n", sodi3_flags);
	mt_idle_log("sodi3_force_vcore_lp_mode=%d\n", sodi3_force_vcore_lp_mode);

	mt_idle_log("\n*********** soidle3 command help  ************\n");
	mt_idle_log("soidle3 help:  cat /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("switch on/off: echo [soidle3] 1/0 > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("bypass pll:    echo bypass_pll 1/0 > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("bypass en:     echo bypass_en 1/0 > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("sodi3 flags:   echo sodi3_flags value > /sys/kernel/debug/cpuidle/soidle3_state\n");
	mt_idle_log("\t[0] reduce log, [1] residency, [2] resource usage\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t soidle3_state_write(struct file *filp,
									const char __user *userbuf,
									size_t count,
									loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "soidle3"))
			idle_switch[IDLE_TYPE_SO3] = param;
		else if (!strcmp(cmd, "enable"))
			enable_soidle3_by_bit(param);
		else if (!strcmp(cmd, "disable"))
			disable_soidle3_by_bit(param);
		else if (!strcmp(cmd, "time"))
			soidle3_time_criteria = param;
		else if (!strcmp(cmd, "bypass_pll")) {
			soidle3_by_pass_pll = param;
			idle_dbg("bypass_pll = %d\n", soidle3_by_pass_pll);
		} else if (!strcmp(cmd, "bypass")) {
			soidle3_by_pass_cg = param;
			idle_dbg("bypass = %d\n", soidle3_by_pass_cg);
		} else if (!strcmp(cmd, "bypass_en")) {
			soidle3_by_pass_en = param;
			idle_dbg("bypass_en = %d\n", soidle3_by_pass_en);
		} else if (!strcmp(cmd, "sodi3_flags")) {
			sodi3_flags = param;
			idle_dbg("sodi3_flags = 0x%x\n", sodi3_flags);
		} else if (!strcmp(cmd, "sodi3_force_vcore_lp_mode")) {
			sodi3_force_vcore_lp_mode = param ? true : false;
			idle_dbg("sodi3_force_vcore_lp_mode = %d\n", sodi3_force_vcore_lp_mode);
		}
		return count;
	} else if (!kstrtoint(cmd_buf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_SO3] = param;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations soidle3_state_fops = {
	.open = soidle3_state_open,
	.read = soidle3_state_read,
	.write = soidle3_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* soidle_state */
static int _soidle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int soidle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _soidle_state_open, inode->i_private);
}

static ssize_t soidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i, len = 0;
	char *p = dbg_buf;

	mt_idle_log("*********** soidle state ************\n");
	mt_idle_log("soidle_time_criteria=%u\n", soidle_time_criteria);

	for (i = 0; i < NR_REASONS; i++)
		mt_idle_log("[%d]soidle_block_cnt[%s]=%lu\n", i, mtk_get_reason_name(i), soidle_block_cnt[i]);
	mt_idle_log("\n");

	for (i = 0; i < NR_GRPS; i++) {
		mt_idle_log("[%02d]soidle_condition_mask[%-8s]=0x%08x\t\tsoidle_block_mask[%-8s]=0x%08x\n", i,
			mtk_get_cg_group_name(i), idle_condition_mask[IDLE_TYPE_SO][i],
			mtk_get_cg_group_name(i), idle_block_mask[IDLE_TYPE_SO][i]);
	}

	mt_idle_log("soidle pg_stat=0x%08x\n", idle_block_mask[IDLE_TYPE_SO][NR_GRPS + 1]);

	mt_idle_log("soidle_bypass_cg=%u\n", soidle_by_pass_cg);
	mt_idle_log("soidle_by_pass_pg=%u\n", soidle_by_pass_pg);
	mt_idle_log("soidle_bypass_en=%u\n", soidle_by_pass_en);
	mt_idle_log("sodi_flags=0x%x\n", sodi_flags);

	mt_idle_log("\n*********** soidle command help  ************\n");
	mt_idle_log("soidle help:   cat /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("switch on/off: echo [soidle] 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("bypass en:     echo bypass_en 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("sodi flags:    echo sodi_flags value > /sys/kernel/debug/cpuidle/soidle_state\n");
	mt_idle_log("\t[0] reduce log, [1] residency, [2] resource usage\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t soidle_state_write(struct file *filp,
									const char __user *userbuf,
									size_t count,
									loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "soidle"))
			idle_switch[IDLE_TYPE_SO] = param;
		else if (!strcmp(cmd, "enable"))
			enable_soidle_by_bit(param);
		else if (!strcmp(cmd, "disable"))
			disable_soidle_by_bit(param);
		else if (!strcmp(cmd, "time"))
			soidle_time_criteria = param;
		else if (!strcmp(cmd, "bypass")) {
			soidle_by_pass_cg = param;
			idle_dbg("bypass = %d\n", soidle_by_pass_cg);
		} else if (!strcmp(cmd, "bypass_pg")) {
			soidle_by_pass_pg = param;
			idle_warn("bypass_pg = %d\n", soidle_by_pass_pg);
		} else if (!strcmp(cmd, "bypass_en")) {
			soidle_by_pass_en = param;
			idle_dbg("bypass_en = %d\n", soidle_by_pass_en);
		} else if (!strcmp(cmd, "sodi_flags")) {
			sodi_flags = param;
			idle_dbg("sodi_flags = 0x%x\n", sodi_flags);
		}
		return count;
	} else if (!kstrtoint(cmd_buf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_SO] = param;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations soidle_state_fops = {
	.open = soidle_state_open,
	.read = soidle_state_read,
	.write = soidle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* mcsodi_state */
static int _mcsodi_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int mcsodi_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _mcsodi_state_open, inode->i_private);
}

static ssize_t mcsodi_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i, len = 0;
	char *p = dbg_buf;

	mt_idle_log("*********** mcsodi state ************\n");
	mt_idle_log("mcsodi_time_criteria=%u\n", mcsodi_time_criteria);

	for (i = 0; i < NR_REASONS; i++)
		mt_idle_log("[%d]mcsodi_block_cnt[%s]=%lu\n", i, mtk_get_reason_name(i), mcsodi_block_cnt[i]);
	mt_idle_log("\n");

	for (i = 0; i < NR_GRPS; i++) {
		mt_idle_log("[%02d]mcsodi_condition_mask[%-8s]=0x%08x\t\tmcsodi_block_mask[%-8s]=0x%08x\n", i,
			mtk_get_cg_group_name(i), idle_condition_mask[IDLE_TYPE_MCSO][i],
			mtk_get_cg_group_name(i), idle_block_mask[IDLE_TYPE_MCSO][i]);
	}

	mt_idle_log("mcsodi pg_stat=0x%08x\n", idle_block_mask[IDLE_TYPE_MCSO][NR_GRPS + 1]);

	mt_idle_log("mcsodi_bypass_cg=%u\n", mcsodi_by_pass_cg);
	mt_idle_log("mcsodi_by_pass_pg=%u\n", mcsodi_by_pass_pg);
	mt_idle_log("mcsodi_bypass_en=%u\n", mcsodi_by_pass_en);
	mt_idle_log("SPM_BSI_CLK_SR=0x%x\n", idle_readl(SPM_BSI_CLK_SR));
	mt_idle_log("CPU_IDLE_STA=0x%x\n", idle_readl(CPU_IDLE_STA));

	mt_idle_log("\n*********** mcsodi command help  ************\n");
	mt_idle_log("mcsodi help:   cat /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("switch on/off: echo [mcsodi] 1/0 > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("bypass en:     echo bypass_en 1/0 > /sys/kernel/debug/cpuidle/mcsodi_state\n");
	mt_idle_log("bypass pg:     echo bypass_pg 1/0 > /sys/kernel/debug/cpuidle/mcsodi_state\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcsodi_state_write(struct file *filp,
									const char __user *userbuf,
									size_t count,
									loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "mcsodi"))
			idle_switch[IDLE_TYPE_MCSO] = param;
		else if (!strcmp(cmd, "enable"))
			enable_mcsodi_by_bit(param);
		else if (!strcmp(cmd, "disable"))
			disable_mcsodi_by_bit(param);
		else if (!strcmp(cmd, "time"))
			mcsodi_time_criteria = param;
		else if (!strcmp(cmd, "bypass")) {
			mcsodi_by_pass_cg = !!param;
			idle_dbg("bypass = %d\n", mcsodi_by_pass_cg);
		} else if (!strcmp(cmd, "bypass_pg")) {
			mcsodi_by_pass_pg = !!param;
			idle_warn("bypass_pg = %d\n", mcsodi_by_pass_pg);
		} else if (!strcmp(cmd, "bypass_en")) {
			mcsodi_by_pass_en = !!param;
			idle_dbg("bypass_en = %d\n", mcsodi_by_pass_en);
		}
		return count;
	} else if (!kstrtoint(cmd_buf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_MCSO] = param;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations mcsodi_state_fops = {
	.open = mcsodi_state_open,
	.read = mcsodi_state_read,
	.write = mcsodi_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* slidle_state */
static int _slidle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int slidle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _slidle_state_open, inode->i_private);
}

static ssize_t slidle_state_read(struct file *filp, char __user *userbuf, size_t count,
				 loff_t *f_pos)
{
	int i, len = 0;
	char *p = dbg_buf;

	mt_idle_log("*********** slow idle state ************\n");

	for (i = 0; i < NR_REASONS; i++)
		mt_idle_log("[%d]slidle_block_cnt[%s]=%lu\n", i, mtk_get_reason_name(i), slidle_block_cnt[i]);
	mt_idle_log("\n");

	for (i = 0; i < NR_GRPS; i++) {
		mt_idle_log("[%02d]slidle_condition_mask[%-8s]=0x%08x\t\tslidle_block_mask[%-8s]=0x%08x\n", i,
			mtk_get_cg_group_name(i), idle_condition_mask[IDLE_TYPE_SL][i],
			mtk_get_cg_group_name(i), idle_block_mask[IDLE_TYPE_SL][i]);
	}

	mt_idle_log("\n********** slidle command help **********\n");
	mt_idle_log("slidle help:   cat /sys/kernel/debug/cpuidle/slidle_state\n");
	mt_idle_log("switch on/off: echo [slidle] 1/0 > /sys/kernel/debug/cpuidle/slidle_state\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t slidle_state_write(struct file *filp, const char __user *userbuf,
				  size_t count, loff_t *f_pos)
{
	char cmd[NR_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(userbuf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "slidle"))
			idle_switch[IDLE_TYPE_SL] = param;
		else if (!strcmp(cmd, "enable"))
			enable_slidle_by_bit(param);
		else if (!strcmp(cmd, "disable"))
			disable_slidle_by_bit(param);

		return count;
	} else if (!kstrtoint(userbuf, 10, &param) == 1) {
		idle_switch[IDLE_TYPE_SL] = param;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations slidle_state_fops = {
	.open = slidle_state_open,
	.read = slidle_state_read,
	.write = slidle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* CG/PLL/MTCMOS register dump */
static int _reg_dump_open(struct seq_file *s, void *data)
{
	return 0;
}

static int reg_dump_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _reg_dump_open, inode->i_private);
}

static ssize_t reg_dump_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int len = 0;
	char *p = dbg_buf;

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t reg_dump_write(struct file *filp,
									const char __user *userbuf,
									size_t count,
									loff_t *f_pos)
{
	count = min(count, sizeof(cmd_buf) - 1);

	return count;
}

static const struct file_operations reg_dump_fops = {
	.open = reg_dump_open,
	.read = reg_dump_read,
	.write = reg_dump_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* debugfs entry */
static struct dentry *root_entry;

static int mtk_cpuidle_debugfs_init(void)
{
	/* TODO: check if debugfs_create_file() failed */
	/* Initialize debugfs */
	root_entry = debugfs_create_dir("cpuidle", NULL);
	if (!root_entry) {
		idle_err("Can not create debugfs `dpidle_state`\n");
		return 1;
	}

	debugfs_create_file("idle_state", 0644, root_entry, NULL, &idle_state_fops);
	debugfs_create_file("dpidle_state", 0644, root_entry, NULL, &dpidle_state_fops);
	debugfs_create_file("soidle3_state", 0644, root_entry, NULL, &soidle3_state_fops);
	debugfs_create_file("soidle_state", 0644, root_entry, NULL, &soidle_state_fops);
	debugfs_create_file("mcidle_state", 0644, root_entry, NULL, &mcidle_state_fops);
	debugfs_create_file("mcsodi_state", 0644, root_entry, NULL, &mcsodi_state_fops);
	debugfs_create_file("slidle_state", 0644, root_entry, NULL, &slidle_state_fops);
	debugfs_create_file("reg_dump", 0644, root_entry, NULL, &reg_dump_fops);

	return 0;
}

#ifndef CONFIG_MTK_ACAO_SUPPORT
/* CPU hotplug notifier, for informing whether CPU hotplug is working */
static int mtk_idle_cpu_callback(struct notifier_block *nfb,
				   unsigned long action, void *hcpu)
{
	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
		atomic_inc(&is_in_hotplug);
		break;

	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
	case CPU_DOWN_FAILED:
	case CPU_DOWN_FAILED_FROZEN:
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		atomic_dec(&is_in_hotplug);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mtk_idle_cpu_notifier = {
	.notifier_call = mtk_idle_cpu_callback,
	.priority   = INT_MAX,
};

static int mtk_idle_hotplug_cb_init(void)
{
	register_cpu_notifier(&mtk_idle_cpu_notifier);

	return 0;
}
#endif

void mtk_idle_gpt_init(void)
{
/* 20170407 Owen fix build error */
#if 0
#ifndef USING_STD_TIMER_OPS
	int err = 0;

	err = request_gpt(IDLE_GPT, GPT_ONE_SHOT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1,
			  0, NULL, GPT_NOAUTOEN);

	if (err)
		idle_warn("[%s] fail to request GPT %d\n", __func__, IDLE_GPT + 1);
#endif
#endif
}

static void mtk_idle_profile_init(void)
{
	mtk_idle_twam_init();
	mtk_idle_block_setting(IDLE_TYPE_DP, dpidle_cnt, dpidle_block_cnt, idle_block_mask[IDLE_TYPE_DP]);
	mtk_idle_block_setting(IDLE_TYPE_SO3, soidle3_cnt, soidle3_block_cnt, idle_block_mask[IDLE_TYPE_SO3]);
	mtk_idle_block_setting(IDLE_TYPE_SO, soidle_cnt, soidle_block_cnt, idle_block_mask[IDLE_TYPE_SO]);
	mtk_idle_block_setting(IDLE_TYPE_MCSO, mcsodi_cnt, mcsodi_block_cnt, idle_block_mask[IDLE_TYPE_MCSO]);
	mtk_idle_block_setting(IDLE_TYPE_SL, slidle_cnt, slidle_block_cnt, idle_block_mask[IDLE_TYPE_SL]);
	mtk_idle_block_setting(IDLE_TYPE_RG, rgidle_cnt, NULL, NULL);
}

void mtk_cpuidle_framework_init(void)
{
	idle_ver("[%s]entry!!\n", __func__);

	iomap_init();
	mtk_cpuidle_debugfs_init();

#ifndef CONFIG_MTK_ACAO_SUPPORT
	mtk_idle_hotplug_cb_init();
#endif

	mtk_idle_gpt_init();

#if !defined(CONFIG_MACH_MT6759) && !defined(CONFIG_MACH_MT6758)
	dpidle_by_pass_pg = true;
#endif

	mtk_idle_profile_init();
}
EXPORT_SYMBOL(mtk_cpuidle_framework_init);

module_param(mt_idle_chk_golden, bool, 0644);
module_param(mt_dpidle_chk_golden, bool, 0644);
