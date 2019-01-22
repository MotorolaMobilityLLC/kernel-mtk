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

/**
 * @file	mkt_etc.c
 * @brief   Driver for eTC
 *
 */

#define __MTK_ETC_C__

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	#define ETC_ENABLE_TINYSYS_SSPM (0)
#else
	#define ETC_ENABLE_TINYSYS_SSPM (0)
#endif

/*
*=============================================================
* Include files
*=============================================================
*/

/* system includes */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#ifdef __KERNEL__
	#include <linux/topology.h>

	/* local includes (kernel-4.4)*/
	#include <mt-plat/mtk_chip.h>
	#include <mt-plat/mtk_gpio.h>
	#include "mtk_etc.h"
	#include <mt-plat/mtk_devinfo.h>
	#include <regulator/consumer.h>
#endif

#ifdef CONFIG_OF
	#include <linux/cpu.h>
	#include <linux/of.h>
	#include <linux/of_irq.h>
	#include <linux/of_address.h>
	#include <linux/of_fdt.h>
	#include <mt-plat/aee.h>
	#if defined(CONFIG_MTK_CLKMGR)
		#include <mach/mt_clkmgr.h>
	#else
		#include <linux/clk.h>
	#endif
#endif

#if ETC_ENABLE_TINYSYS_SSPM
	#include "sspm_ipi.h"
	#include <sspm_reservedmem_define.h>
#endif /* if ETC_ENABLE_TINYSYS_SSPM */

/************************************************
* Marco definition
************************************************/
#define ETC_ENABLE		(0)
#if ETC_ENABLE
	static unsigned int ctrl_etc_enable = 1;
#else
	static unsigned int ctrl_etc_enable;
#endif

#define LOG_INTERVAL	(2LL * NSEC_PER_SEC)

#define VMAX		(1100000)
#define VMIN		(800000)

#define MIN_VPROC_MV_1000X (600000)
#define MAX_VPROC_MV_1000X (1100000)

/* Get time stmp to known the time period */
static unsigned long long etc_pTime_us, etc_cTime_us, etc_diff_us;
#ifdef __KERNEL__
#define TIME_TH_US 3000
#define ETC_IS_TOO_LONG()   \
	do {	\
		etc_diff_us = etc_cTime_us - etc_pTime_us;	\
		if (etc_diff_us > TIME_TH_US) {				\
			pr_warn(ETC_TAG "caller_addr %p: %llu us\n", __builtin_return_address(0), etc_diff_us); \
		} else if (etc_diff_us < 0) {	\
			pr_warn(ETC_TAG "E: misuse caller_addr %p\n", __builtin_return_address(0)); \
		}	\
	} while (0)
#endif
/************************************************
 * bit operation
************************************************/
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))

/************************************************
 * LOG
************************************************/
#ifdef __KERNEL__
#define ETC_TAG	 "[xxxx_etc] "

#define etc_emerg(fmt, args...)		pr_emerg(ETC_TAG fmt, ##args)
#define etc_alert(fmt, args...)		pr_alert(ETC_TAG fmt, ##args)
#define etc_crit(fmt, args...)		pr_crit(ETC_TAG fmt, ##args)
#define etc_error(fmt, args...)		pr_err(ETC_TAG fmt, ##args)
#define etc_warning(fmt, args...)	pr_warn(ETC_TAG fmt, ##args)
#define etc_notice(fmt, args...)	pr_notice(ETC_TAG fmt, ##args)
#define etc_info(fmt, args...)		pr_info(ETC_TAG fmt, ##args)
#define etc_debug(fmt, args...)		\
	do {	\
		if (etc_log_en)   \
			pr_warn(ETC_TAG""fmt, ##args);       \
	} while (0)

	#ifdef EN_ISR_LOG /* For Interrupt use */
		#define etc_isr_info(fmt, args...)  etc_debug(fmt, ##args)
	#else
		#define etc_isr_info(fmt, args...)
	#endif
#endif

/************************************************
 * REG ACCESS
************************************************/
#ifdef __KERNEL__
	#define etc_read(addr)	__raw_readl((void __iomem *)(addr))
	#define etc_read_field(addr, range)	\
		((etc_read(addr) & BITMASK(range)) >> LSB(range))
	#define etc_write(addr, val)	mt_reg_sync_writel(val, addr)
#endif
/**
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define etc_write_field(addr, range, val)	\
	etc_write(addr, (etc_read(addr) & ~BITMASK(range)) | BITS(range, val))

/************************************************
 * static Variable
************************************************/
#ifdef __KERNEL__
void __iomem *uni_pll_base;
static struct hrtimer etc_timer_log;
#endif
static DEFINE_SPINLOCK(etc_spinlock);
static bool etc_log_en;
static bool etc_timer_en;
static unsigned int etc_VOUT, etc_reg;
/************************************************
* CPU callback
************************************************/
#ifdef __KERNEL__
static int _mt_etc_cpu_CB(struct notifier_block *nfb,
			unsigned long action, void *hcpu);
static struct notifier_block __refdata _mt_etc_cpu_notifier = {
	.notifier_call = _mt_etc_cpu_CB,
};
#endif

/************************************************
* Global function definition
************************************************/
#ifdef __KERNEL__
static long long etc_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}
#endif

static void mtk_etc_lock(unsigned long *flags)
{
	/* FIXME: lock with SSPM */
	/* get_md32_semaphore(SEMAPHORE_ETC); */
	spin_lock_irqsave(&etc_spinlock, *flags);
#ifdef __KERNEL__
	etc_pTime_us = etc_get_current_time_us();
#endif
}

void mtk_etc_unlock(unsigned long *flags)
{
#ifdef __KERNEL__
	etc_cTime_us = etc_get_current_time_us();
	ETC_IS_TOO_LONG();
#endif
	spin_unlock_irqrestore(&etc_spinlock, *flags);
	/* FIXME: lock with SSPM */
	/* release_md32_semaphore(SEMAPHORE_ETC); */
}

void mtk_etc_init(void)
{
	if (ctrl_etc_enable == 1) {
		mt_secure_call_etc(MTK_SIP_KERNEL_ETC_INIT, 0, 0, 0);
	}
}

void mtk_etc_voltage_change(unsigned int new_vout)
{
	if (ctrl_etc_enable == 1) {
		mt_secure_call_etc(MTK_SIP_KERNEL_ETC_VOLT_CHG, new_vout, 0, 0);
		etc_VOUT = new_vout;
	}
}

void mtk_etc_power_off(void)
{
	mt_secure_call_etc(MTK_SIP_KERNEL_ETC_PWR_OFF, 0, 0, 0);
}

void mtk_dormant_ctrl(unsigned int onOff)
{
	mt_secure_call_etc(MTK_SIP_KERNEL_ETC_DMT_CTL, onOff, 0, 0);
}

#if !(ETC_ENABLE_TINYSYS_SSPM)
#ifdef __KERNEL__
static int _mt_etc_cpu_CB(struct notifier_block *nfb,
	unsigned long action, void *hcpu)
{
	unsigned long flags;
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	enum mtk_etc_cpu_id cluster_id;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask etc_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus, big_cpus;

	/* Current active CPU is belong which cluster */
	cluster_id = arch_get_cluster_id(cpu);

	/* How many active CPU in this cluster, present by bit mask
	*	ex:	B	L	LL
	*		00	1111	0000
	*/
	arch_get_cluster_cpus(&etc_cpumask, cluster_id);

	/* How many active CPU online in this cluster, present by number */
	cpumask_and(&cpu_online_cpumask, &etc_cpumask, cpu_online_mask);
	cpus = cpumask_weight(&cpu_online_cpumask);

	if (etc_log_en)
		etc_error("@%s():%d, cpu = %d, act = %lu, on_cpus = %d, clst = %d, clst_cpu = %d\n"
		, __func__, __LINE__, cpu, action, online_cpus, cluster_id, cpus);

	dev = get_cpu_device(cpu);
	if (dev) {
		arch_get_cluster_cpus(&etc_cpumask, MT_ETC_CPU_B);
		cpumask_and(&cpu_online_cpumask, &etc_cpumask, cpu_online_mask);
		big_cpus = cpumask_weight(&cpu_online_cpumask);

		switch (action) {
		case CPU_DOWN_PREPARE:
			if ((cluster_id == MT_ETC_CPU_B) && (big_cpus == 1)) {
				if (etc_log_en)
					etc_debug("DEAD (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mtk_etc_lock(&flags);
				mtk_etc_power_off();
				mtk_etc_unlock(&flags);
			} else {
				if (etc_log_en)
					etc_error("DEAD (%d), BIG_cc(%d)\n",
						online_cpus, big_cpus);
			}
		break;

		case CPU_ONLINE:
			if ((cluster_id == MT_ETC_CPU_B) && (big_cpus == 1)) {
				if (etc_log_en)
					etc_error("OL (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mtk_etc_lock(&flags);
				mtk_etc_init();
				mtk_etc_unlock(&flags);
			} else {
				if (etc_log_en)
					etc_error("OL  (%d), BIG_cc(%d)\n",
						online_cpus, big_cpus);
			}
		break;
		}
	}

	return NOTIFY_OK;
}

/*
 * timer for log
 */
static enum hrtimer_restart etc_timer_log_func(struct hrtimer *timer)
{
	etc_error("Timer (%d)\n", etc_timer_en);

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	return HRTIMER_RESTART;
}

/************************************************
 * set ETC status by procfs interface
************************************************/
static int etc_enable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ctrl_etc_enable);
	return 0;
}

static ssize_t etc_enable_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *pos)
{
	int enable;
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		etc_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	if (enable == 0) {
		etc_error("Disable ETC !!\n");
		mtk_etc_power_off();
		ctrl_etc_enable = 0;
	} else {
		etc_error("Enable ETC !!\n");
		ctrl_etc_enable = enable;
		mtk_etc_init();
	}

out:
	free_page((unsigned long)buf);
	return count;
}

static int etc_log_en_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", etc_log_en);
	return 0;
}

static ssize_t etc_log_en_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *pos)
{
	int ret = -EINVAL;
	int enable;
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		etc_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;
	switch (enable) {
	case 0:
		etc_debug("Disable ETC log !!\n");
		etc_log_en = 0;
		break;

	case 1:
		etc_debug("Enable ETC log !!\n");
		etc_log_en = 1;
		break;

	default:
		if (enable > 1) {
			etc_debug("Enable ETC log !(%d)!\n", enable);
			etc_log_en = 1;
		} else {
			etc_debug("Disable ETC log !(%d)!\n", enable);
			etc_log_en = 0;
		}
	}
out:
	free_page((unsigned long)buf);
	return count;
}

static int etc_timer_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", etc_timer_en);
	return 0;
}

static ssize_t etc_timer_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *pos)
{
	int ret = -EINVAL;
	int enable;
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		etc_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;
	switch (enable) {
	case 0:
		etc_debug("Disable ETC timer!!\n");
		etc_timer_en = 0;
		hrtimer_cancel(&etc_timer_log);
		break;

	case 1:
		etc_debug("Enable ETC timer!!\n");
		etc_timer_en = 1;
		hrtimer_start(&etc_timer_log, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		if (enable > 1) {
			etc_debug("Enable ETC timer!(%d)!\n", enable);
			etc_timer_en = 1;
			hrtimer_start(&etc_timer_log, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		} else {
			etc_debug("Disable ETC timer!(%d)!\n", enable);
			etc_timer_en = 0;
			hrtimer_cancel(&etc_timer_log);
		}
	}
out:
	free_page((unsigned long)buf);
	return count;
}

static int etc_vout_proc_show(struct seq_file *m, void *v)
{
	int vout = 0;

	vout = mt_secure_call_etc(MTK_SIP_KERNEL_ETC_RED, 0x1020260C, 0, 0);
	seq_printf(m, "0x%x (%d uv)\n", vout, 400000 + (vout * 3125));

	return 0;
}

static ssize_t etc_vout_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *pos)
{
	int ret = -EINVAL;
	int vout = 0;
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &vout)) {
		etc_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	if ((vout > MAX_VPROC_MV_1000X) || (vout < MIN_VPROC_MV_1000X)) {
		etc_debug("bad argument!! should be %d ~ %d\n",
			  MIN_VPROC_MV_1000X, MAX_VPROC_MV_1000X);
		goto out;
	}

	ret = 0;

	mtk_etc_voltage_change(vout);

out:
	free_page((unsigned long)buf);

	return count;
}

static int etc_reg_proc_show(struct seq_file *m, void *v)
{
	etc_reg = (etc_reg == 0x1020260C) ? 0x1020260C : etc_reg;
	seq_printf(m, "0x%x = 0x%x\n",
		etc_reg,
		mt_secure_call_etc(MTK_SIP_KERNEL_ETC_RED, etc_reg, 0, 0));

	return 0;
}

static ssize_t etc_reg_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);

	if (!buf)
		return -ENOMEM;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 16, &etc_reg)) {
		etc_error("bad argument!! Should be a number.\n");
		goto out;
	}

	mt_secure_call_etc(MTK_SIP_KERNEL_ETC_ANK, etc_reg, 0, 0);

out:
	free_page((unsigned long)buf);

	return count;
}

static int etc_reg_dump_proc_show(struct seq_file *m, void *v)
{
	int value = 0;
	unsigned int i, addr;

	if (ctrl_etc_enable == 0) {
		seq_puts(m, "etc is not turned on\n");
		return 0;
	}

	for (i = 0; i < 0xF0; i += 4) {
		addr = 0x10202600 + i;
		value = mt_secure_call_etc(MTK_SIP_KERNEL_ETC_RED, addr, 0, 0);
		seq_printf(m, "reg %x = 0x%X\n", addr, value);
	}

	return 0;
}


#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
		.write		  = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(etc_enable);
PROC_FOPS_RW(etc_log_en);
PROC_FOPS_RW(etc_timer);
PROC_FOPS_RW(etc_vout);
PROC_FOPS_RW(etc_reg);
PROC_FOPS_RO(etc_reg_dump);

static int create_procfs(void)
{
	struct proc_dir_entry *etc_dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry etc_entries[] = {
		PROC_ENTRY(etc_enable),
		PROC_ENTRY(etc_log_en),
		PROC_ENTRY(etc_timer),
		PROC_ENTRY(etc_vout),
		PROC_ENTRY(etc_reg),
		PROC_ENTRY(etc_reg_dump),
	};

	etc_dir = proc_mkdir("etc", NULL);
	if (!etc_dir) {
		etc_error("[%s]: mkdir /proc/etc failed\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(etc_entries); i++) {
		if (!proc_create(etc_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, etc_dir, etc_entries[i].fops)) {
			etc_error("[%s]: create /proc/etc/%s failed\n", __func__, etc_entries[i].name);
			return -3;
		}
	}
	return 0;
}

static int etc_probe(struct platform_device *pdev)
{
	/* enable etc */
	if (ctrl_etc_enable == 0) {
		etc_error("[ETC] default off !!");
		return 0;
	}

	mtk_etc_init();
	register_hotcpu_notifier(&_mt_etc_cpu_notifier);
	return 0;
}

static int etc_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* into dormant mode or disable etc */
	/* mtk_dormant_ctrl(1); */
	/* mtk_etc_power_off(); */
	return 0;
}

static int etc_resume(struct platform_device *pdev)
{
	/* enable etc */
	/* mtk_etc_init(); */
	return 0;
}
static struct platform_driver etc_driver = {
	.remove		= NULL,
	.shutdown	= NULL,
	.probe		= etc_probe,
	.suspend	= etc_suspend,
	.resume		= etc_resume,
	.driver		= {
		.name   = "mt-etc",
	},
};

static int __init etc_init(void)
{
	int err = 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-apmixedsys");
	if (node) {
		uni_pll_base = of_iomap(node, 0);
		etc_debug("[ETC] uni_pll_base = 0x%p\n", uni_pll_base);
	} else {
		etc_debug("[ETC] Not get uni_pll_base address !!\n");
		return -ENOENT;
	}

	/* init timer for log / volt */
	hrtimer_init(&etc_timer_log, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	etc_timer_log.function = etc_timer_log_func;

	create_procfs();

	err = platform_driver_register(&etc_driver);
	if (err) {
		etc_debug("ETC driver callback register failed..\n");
		return err;
	}
	return 0;
}

static void __exit etc_exit(void)
{
	etc_debug("etc de-initialization\n");
}


late_initcall(etc_init);
#endif /* __KERNEL__ */
#else  /* if ETC_ENABLE_TINYSYS_SSPM */

#endif /* ETC_ENABLE_TINYSYS_SSPM */



MODULE_DESCRIPTION("MediaTek ETC Driver v0.1");
MODULE_LICENSE("GPL");

#undef __MTK_ETC_C__
