#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <mt-plat/mt_typedefs.h>
#include <mach/mt_power_gs.h>

void mt_power_gs_dump_dpidle(void)
{
	mt_power_gs_compare("DPIdle", MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode,
		MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode_len);
}

static int dump_dpidle_read(struct seq_file *m, void *v)
{
	seq_puts(m, "mt_power_gs : dpidle\n");
	mt_power_gs_dump_dpidle();

	return 0;
}

static void __exit mt_power_gs_dpidle_exit(void)
{
	remove_proc_entry("dump_dpidle", mt_power_gs_dir);
}

static int proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, dump_dpidle_read, NULL);
}

static const struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init mt_power_gs_dpidle_init(void)
{
	struct proc_dir_entry *mt_entry = NULL;

	if (!mt_power_gs_dir) {
		pr_err("[%s]: mkdir /proc/mt_power_gs failed\n", __func__);
	} else {
		mt_entry = proc_create("dump_dpidle", S_IRUGO | S_IWUSR | S_IWGRP, mt_power_gs_dir, &proc_fops);
		if (NULL == mt_entry)
			return -ENOMEM;
	}
	return 0;
}

module_init(mt_power_gs_dpidle_init);
module_exit(mt_power_gs_dpidle_exit);

MODULE_DESCRIPTION("MT Power Golden Setting - dpidle");
