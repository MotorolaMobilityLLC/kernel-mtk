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
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/debugfs.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>

#include <mtk_cpuidle.h>	/* mtk_cpuidle_init */
#include <mtk_sleep.h>	    /* slp_module_init */
#include <mt-plat/aee.h>	/* aee_xxx */

#include <mtk_spm_irq.h>
#include <mtk_spm_internal.h>
#include <mtk_sspm.h>

DEFINE_SPINLOCK(__spm_lock);

void __attribute__ ((weak)) mtk_idle_cond_check_init(void)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
}

/* Note: implemented in mtk_spm_vcorefs.c */
void  __attribute__ ((weak)) spm_vcorefs_init(void)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
}

/* Note: implemented in mtk_spm_dram.c */
int __attribute__ ((weak)) spm_get_spmfw_idx(void)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
	return 1;
}

/* Note: implemented in mtk_spm_irq.c */
int __attribute__ ((weak)) mtk_spm_irq_register(unsigned int spmirq0)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
	return 0;
}

/* Note: implemented in mtk_cpuidle.c */
int __attribute__ ((weak)) mtk_cpuidle_init(void) { return -EOPNOTSUPP; }

/* Note: implemented in mtk_spm_dram.c */
void __attribute__((weak)) spm_do_dram_config_check(void)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
}

/* Note: implemented in mtk_spm_fs.c */
int __attribute__((weak)) spm_fs_init(void)
{
	aee_sram_printk("NO %s !!!\n", __func__);
	pr_info("[SPM] NO %s !!!\n", __func__);
	return 0;
}

/* Note: implemented in mtk_spm_sleep.c */
int __attribute__((weak)) get_spm_last_wakeup_src(
	struct seq_file *s, void *unused) { return 0; }

/* Note: implemented in mtk_spm_sleep.c */
int __attribute__((weak)) get_spm_sleep_count(
	struct seq_file *s, void *unused) { return 0; }

void __iomem *spm_base;
void __iomem *sleep_reg_md_base;

static struct platform_device *pspmdev;
static struct wakeup_source spm_wakelock;

/* FIXME: should not used externally !!! */
void *mt_spm_base_get(void)
{
	return spm_base;
}
EXPORT_SYMBOL(mt_spm_base_get);

void spm_pm_stay_awake(int sec)
{
	__pm_wakeup_event(&spm_wakelock, HZ * sec);
}

static void spm_register_init(unsigned int *spm_irq_0_ptr)
{
	struct device_node *node;
	unsigned int spmirq0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node)
		pr_info("[SPM] find sleep node failed\n");

	spm_base = of_iomap(node, 0);
	if (!spm_base)
		pr_info("[SPM] base spm_base failed\n");

	spmirq0 = irq_of_parse_and_map(node, 0);
	if (!spmirq0)
		pr_info("[SPM] get spm_irq_0 failed\n");
	*spm_irq_0_ptr = spmirq0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep_reg_md");
	if (!node)
		pr_info("[SPM] find sleep_reg_md node failed\n");

	sleep_reg_md_base = of_iomap(node, 0);
	if (!sleep_reg_md_base)
		pr_info("[SPM] base sleep_reg_md_base failed\n");

	pr_info("[SPM] spm_base = %p, sleep_reg_md_base = %p, spm_irq_0 = %d\n",
		spm_base, sleep_reg_md_base, spmirq0);
}

static ssize_t show_debug_log(
	struct device *dev, struct device_attribute *attr,
	char *buf){
	char *p = buf;

	p += sprintf(p, "for test\n");

	return p - buf;
}

static ssize_t store_debug_log(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(debug_log, 0664, show_debug_log, store_debug_log);

static int spm_probe(struct platform_device *pdev)
{
	int ret;

	ret = device_create_file(&(pdev->dev), &dev_attr_debug_log);

	return 0;
}

static int spm_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id spm_of_ids[] = {
	{.compatible = "mediatek,SLEEP",},
	{}
};

static struct platform_driver spm_dev_drv = {
	.probe = spm_probe,
	.remove = spm_remove,
	.driver = {
	.name = "spm",
	.owner = THIS_MODULE,
	.of_match_table = spm_of_ids,
	},
};

static int spm_sleep_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_spm_sleep_count, &inode->i_private);
}

static const struct file_operations spm_sleep_count_fops = {
	.open = spm_sleep_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int spm_last_wakeup_src_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_spm_last_wakeup_src, &inode->i_private);
}

static const struct file_operations spm_last_wakeup_src_fops = {
	.open = spm_last_wakeup_src_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_PM
static int spm_pm_event(struct notifier_block *notifier, unsigned long pm_event,
			void *unused)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	struct spm_data spm_d;
	int ret;
	unsigned long flags;
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	case PM_SUSPEND_PREPARE:
		spin_lock_irqsave(&__spm_lock, flags);
		ret = spm_to_sspm_command(SPM_SUSPEND_PREPARE, &spm_d);
		spin_unlock_irqrestore(&__spm_lock, flags);
		if (ret < 0) {
			pr_err("#@# %s(%d) PM_SUSPEND_PREPARE return %d!!!\n",
				__func__, __LINE__, ret);
			return NOTIFY_BAD;
		}
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:
		spin_lock_irqsave(&__spm_lock, flags);
		ret = spm_to_sspm_command(SPM_POST_SUSPEND, &spm_d);
		spin_unlock_irqrestore(&__spm_lock, flags);
		if (ret < 0) {
			pr_err("#@# %s(%d) PM_POST_SUSPEND return %d!!!\n",
				__func__, __LINE__, ret);
			return NOTIFY_BAD;
		}
		return NOTIFY_DONE;
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
	}
	return NOTIFY_OK;
}

static struct notifier_block spm_pm_notifier_func = {
	.notifier_call = spm_pm_event,
	.priority = 0,
};
#endif /* CONFIG_PM */
#endif /* CONFIG_FPGA_EARLY_PORTING */

static int spm_module_init(void)
{
	unsigned int spm_irq_0 = 0;
	int r = 0;
	int ret = -1;
	struct dentry *spm_dir;
	struct dentry *spm_file;

	wakeup_source_init(&spm_wakelock, "spm");

	spm_register_init(&spm_irq_0);

	/* implemented in mtk_spm_irq.c */
	if (mtk_spm_irq_register(spm_irq_0) != 0)
		r = -EPERM;
#if defined(CONFIG_PM)
	if (spm_fs_init() != 0)
		r = -EPERM;
#endif

	/* implemented in mtk_spm_dram.c */
	spm_do_dram_config_check();

	ret = platform_driver_register(&spm_dev_drv);
	if (ret) {
		pr_debug("fail to register platform driver\n");
		return ret;
	}

	pspmdev = platform_device_register_simple("spm", -1, NULL, 0);
	if (IS_ERR(pspmdev)) {
		pr_debug("Failed to register platform device.\n");
		return -EINVAL;
	}

	spm_dir = debugfs_create_dir("spm", NULL);
	if (spm_dir == NULL) {
		pr_debug("Failed to create spm dir in debugfs.\n");
		return -EINVAL;
	}

	spm_file = debugfs_create_file(
		"spm_sleep_count", 0444,
		spm_dir, NULL, &spm_sleep_count_fops);

	spm_file = debugfs_create_file(
		"spm_last_wakeup_src", 0444,
		spm_dir, NULL, &spm_last_wakeup_src_fops);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_PM
	ret = register_pm_notifier(&spm_pm_notifier_func);
	if (ret) {
		pr_debug("Failed to register PM notifier.\n");
		return ret;
	}
#endif /* CONFIG_PM */
#endif /* CONFIG_FPGA_EARLY_PORTING */
	SMC_CALL(ARGS, SPM_ARGS_SPMFW_IDX, spm_get_spmfw_idx(), 0);

	spm_vcorefs_init();

	return 0;
}

int mtk_spm_init(void)
{
	int ret;

	mtk_cpuidle_init();
	ret = spm_module_init();

	mtk_idle_cond_check_init();
	return ret;
}

MODULE_DESCRIPTION("SPM Driver v0.1");
