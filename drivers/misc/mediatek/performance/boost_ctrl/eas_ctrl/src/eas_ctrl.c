/*
 * Copyright (C) 2016-2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#define pr_fmt(fmt) "[EAS Controller]"fmt


#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/div64.h>


#include <linux/platform_device.h>
#include <eas_controller.h>

#include <mt-plat/mtk_sched.h>

#ifdef CONFIG_TRACING
#include <linux/kallsyms.h>
#include <linux/trace_events.h>
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static struct mutex boost_eas;
#ifdef CONFIG_SCHED_TUNE
static int current_boost_value[NR_CGROUP];
#endif
static int boost_value[NR_CGROUP][EAS_MAX_KIR];
static int debug_boost_value[NR_CGROUP];
static int debug;

/************************/
#ifdef CONFIG_TRACING
static unsigned long __read_mostly tracing_mark_write_addr;
static inline void __mt_update_tracing_mark_write_addr(void)
{
	if (unlikely(tracing_mark_write_addr == 0))
		tracing_mark_write_addr =
				kallsyms_lookup_name("tracing_mark_write");
}

static inline void eas_ctrl_extend_kernel_trace_begin(char *name,
					int id, int walt, int fpsgo)
{
	__mt_update_tracing_mark_write_addr();
	preempt_disable();
	event_trace_printk(tracing_mark_write_addr, "B|%d|%s|%d|%d|%d\n",
				current->tgid, name, id, walt, fpsgo);
	preempt_enable();
}

static inline void eas_ctrl_extend_kernel_trace_end(void)
{
	__mt_update_tracing_mark_write_addr();
	preempt_disable();
	event_trace_printk(tracing_mark_write_addr, "E\n");
	preempt_enable();
}
#endif

static void walt_mode(int enable)
{
#ifdef CONFIG_SCHED_WALT
	sched_walt_enable(LT_WALT_POWERHAL, enable);
#else
	pr_debug("walt not be configured\n");
#endif
}

void ext_launch_start(void)
{
	pr_debug("ext_launch_start\n");
	/*--feature start from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_start", 0, 1, 0);
#endif

	walt_mode(1);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
}

void ext_launch_end(void)
{
	pr_debug("ext_launch_end\n");
	/*--feature end from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_end", 0, 0, 1);
#endif
	walt_mode(0);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
}
/************************/

static int check_boost_value(int boost_value)
{
	return clamp(boost_value, -100, 5000);
}

/************************/
#ifdef CONFIG_SCHED_TUNE
int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	int final_boost_value = 0, final_boost_value_1 = 0;
	int final_boost_value_2 = -101;
	int first_prio_boost_value = 0;
	int boost_1[EAS_MAX_KIR], boost_2[EAS_MAX_KIR];
	int has_set = 0, first_prio_set = 0;
	int i;

	mutex_lock(&boost_eas);

	if (cgroup_idx >= NR_CGROUP) {
		mutex_unlock(&boost_eas);
		pr_debug(" cgroup_idx >= NR_CGROUP, error\n");
		return -1;
	}

	boost_value[cgroup_idx][kicker] = value;

	for (i = 0; i < EAS_MAX_KIR; i++) {
		if (boost_value[cgroup_idx][i] == 0)
			continue;

		if (boost_value[cgroup_idx][i] == 1100 ||
			boost_value[cgroup_idx][i] == 100) {
			first_prio_boost_value =
				MAX(boost_value[cgroup_idx][i],
						first_prio_boost_value);
			first_prio_set = 1;
		}

		boost_1[i] = boost_value[cgroup_idx][i] / 1000;
		boost_2[i] = boost_value[cgroup_idx][i] % 1000;
		final_boost_value_1 = MAX(boost_1[i], final_boost_value_1);
		final_boost_value_2 = MAX(boost_2[i], final_boost_value_2);
		has_set = 1;
	}

	if (first_prio_set)
		final_boost_value = first_prio_boost_value;
	else if (has_set)
		final_boost_value =
			final_boost_value_1 * 1000 + final_boost_value_2;
	else
		final_boost_value = 0;

	current_boost_value[cgroup_idx] = check_boost_value(final_boost_value);

	if (kicker == EAS_KIR_PERF)
		pr_debug("kicker:%d, boost:%d, final:%d, current:%d",
				kicker, boost_value[cgroup_idx][kicker],
				final_boost_value,
				current_boost_value[cgroup_idx]);
	if (!debug)
		boost_write_for_perf_idx(cgroup_idx,
				current_boost_value[cgroup_idx]);
	mutex_unlock(&boost_eas);

	return current_boost_value[cgroup_idx];
}
#else
int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	return -1;
}
#endif

/****************/
static ssize_t perfmgr_perfserv_fg_boost_write(struct file *filp
		, const char *ubuf, size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	data = check_boost_value(data);

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_FG, data);

	return cnt;
}

static int perfmgr_perfserv_fg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_FG][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_fg_boost_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, perfmgr_perfserv_fg_boost_show,
			 inode->i_private);
}

static const struct file_operations perfmgr_perfserv_fg_boost_fops = {
	.open = perfmgr_perfserv_fg_boost_open,
	.write = perfmgr_perfserv_fg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
/************************************************/
static int perfmgr_current_fg_boost_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SCHED_TUNE
	seq_printf(m, "%d\n", current_boost_value[CGROUP_FG]);
#else
	seq_printf(m, "%d\n", -1);
#endif

	return 0;
}

static int perfmgr_current_fg_boost_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, perfmgr_current_fg_boost_show,
			 inode->i_private);
}

static const struct file_operations perfmgr_current_fg_boost_fops = {
	.open = perfmgr_current_fg_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
/************************************************************/
static ssize_t perfmgr_debug_fg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	debug_boost_value[CGROUP_FG] = check_boost_value(data);
	if (debug_boost_value[CGROUP_FG])
		debug = 1;
	else
		debug = 0;

#ifdef CONFIG_SCHED_TUNE
	if (debug) {
		boost_write_for_perf_idx(CGROUP_FG,
			 debug_boost_value[CGROUP_FG]);
	}
#endif

	return cnt;
}

static int perfmgr_debug_fg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", debug_boost_value[CGROUP_FG]);

	return 0;
}

static int perfmgr_debug_fg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_debug_fg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_debug_fg_boost_fops = {
	.open = perfmgr_debug_fg_boost_open,
	.write = perfmgr_debug_fg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/******************************************************/
static ssize_t perfmgr_perfserv_bg_boost_write(struct file *filp,
		 const char *ubuf, size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	data = check_boost_value(data);

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_BG, data);

	return cnt;
}

static int perfmgr_perfserv_bg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_BG][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_bg_boost_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, perfmgr_perfserv_bg_boost_show,
			 inode->i_private);
}

static const struct file_operations perfmgr_perfserv_bg_boost_fops = {
	.open = perfmgr_perfserv_bg_boost_open,
	.write = perfmgr_perfserv_bg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*******************************************************/
static int perfmgr_current_bg_boost_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SCHED_TUNE
	seq_printf(m, "%d\n", current_boost_value[CGROUP_BG]);
#else
	seq_printf(m, "%d\n", -1);
#endif

	return 0;
}

static int perfmgr_current_bg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_current_bg_boost_show
		, inode->i_private);
}

static const struct file_operations perfmgr_current_bg_boost_fops = {
	.open = perfmgr_current_bg_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/**************************************************/
static ssize_t perfmgr_debug_bg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	debug_boost_value[CGROUP_BG] = check_boost_value(data);
	if (debug_boost_value[CGROUP_BG])
		debug = 1;
	else
		debug = 0;

#ifdef CONFIG_SCHED_TUNE
	if (debug) {
		boost_write_for_perf_idx(CGROUP_BG,
		 debug_boost_value[CGROUP_BG]);
	}
#endif

	return cnt;
}

static int perfmgr_debug_bg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", debug_boost_value[CGROUP_BG]);

	return 0;
}

static int perfmgr_debug_bg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_debug_bg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_debug_bg_boost_fops = {
	.open = perfmgr_debug_bg_boost_open,
	.write = perfmgr_debug_bg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


/************************************************/
static ssize_t perfmgr_perfserv_ta_boost_write(struct file *filp,
		 const char *ubuf, size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	data = check_boost_value(data);

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_TA, data);

	return cnt;
}

static int perfmgr_perfserv_ta_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_TA][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_ta_boost_open(struct inode *inode,
						 struct file *file)
{
	return single_open(file, perfmgr_perfserv_ta_boost_show,
				 inode->i_private);
}

static const struct file_operations perfmgr_perfserv_ta_boost_fops = {
	.open = perfmgr_perfserv_ta_boost_open,
	.write = perfmgr_perfserv_ta_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/************************************************/
static ssize_t perfmgr_boot_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int cgroup, data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (sscanf(buf, "%d %d", &cgroup, &data) != 2)
		return -1;

	data = check_boost_value(data);

	if (cgroup >= 0 && cgroup < NR_CGROUP)
		update_eas_boost_value(EAS_KIR_BOOT, cgroup, data);

	return cnt;
}

static int perfmgr_boot_boost_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < NR_CGROUP; i++)
		seq_printf(m, "%d\n", boost_value[i][EAS_KIR_BOOT]);

	return 0;
}

static int perfmgr_boot_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_boot_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_boot_boost_fops = {
	.open = perfmgr_boot_boost_open,
	.write = perfmgr_boot_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/************************************************/
static int perfmgr_current_ta_boost_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SCHED_TUNE
	seq_printf(m, "%d\n", current_boost_value[CGROUP_TA]);
#else
	seq_printf(m, "%d\n", -1);
#endif

	return 0;
}

static int perfmgr_current_ta_boost_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, perfmgr_current_ta_boost_show,
			 inode->i_private);
}

static const struct file_operations perfmgr_current_ta_boost_fops = {
	.open = perfmgr_current_ta_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/**********************************/
static ssize_t perfmgr_debug_ta_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	debug_boost_value[CGROUP_TA] = check_boost_value(data);
	if (debug_boost_value[CGROUP_TA])
		debug = 1;
	else
		debug = 0;

#ifdef CONFIG_SCHED_TUNE
	if (debug) {
		boost_write_for_perf_idx(CGROUP_TA,
		 debug_boost_value[CGROUP_TA]);
	}
#endif

	return cnt;
}

static int perfmgr_debug_ta_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", debug_boost_value[CGROUP_TA]);

	return 0;
}

static int perfmgr_debug_ta_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_debug_ta_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_debug_ta_boost_fops = {
	.open = perfmgr_debug_ta_boost_open,
	.write = perfmgr_debug_ta_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/********************************************************************/
static int ext_launch_state;
static ssize_t perfmgr_perfserv_ext_launch_mon_write(struct file *filp,
		const char *ubuf, size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data) {
		ext_launch_start();
		ext_launch_state = 1;
	} else {
		ext_launch_end();
		ext_launch_state = 0;
	}

	pr_debug("perfmgr_perfserv_ext_launch_mon");
	return cnt;
}

static int perfmgr_perfserv_ext_launch_mon_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ext_launch_state);

	return 0;
}

static int perfmgr_perfserv_ext_launch_mon_open(struct inode *inode,
							struct file *file)
{
	return single_open(file, perfmgr_perfserv_ext_launch_mon_show,
							inode->i_private);
}

static const struct file_operations perfmgr_perfserv_ext_launch_mon_fops = {
	.open = perfmgr_perfserv_ext_launch_mon_open,
	.write = perfmgr_perfserv_ext_launch_mon_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*******************************************/
void perfmgr_eas_boost_init(void)
{
	int i, j;
	struct proc_dir_entry *boost_dir = NULL;

	mutex_init(&boost_eas);

	boost_dir = proc_mkdir("perfmgr/boost_ctrl/eas_ctrl", NULL);
	proc_create("perfserv_fg_boost", 0644, boost_dir,
					 &perfmgr_perfserv_fg_boost_fops);
	proc_create("current_fg_boost", 0644, boost_dir,
					 &perfmgr_current_fg_boost_fops);
	proc_create("debug_fg_boost", 0644, boost_dir,
					 &perfmgr_debug_fg_boost_fops);

	proc_create("perfserv_bg_boost", 0644, boost_dir,
					 &perfmgr_perfserv_bg_boost_fops);
	proc_create("current_bg_boost", 0644, boost_dir,
					 &perfmgr_current_bg_boost_fops);
	proc_create("debug_bg_boost", 0644, boost_dir,
					 &perfmgr_debug_bg_boost_fops);

	proc_create("perfserv_ta_boost", 0644, boost_dir,
					 &perfmgr_perfserv_ta_boost_fops);
	proc_create("current_ta_boost", 0644, boost_dir,
					 &perfmgr_current_ta_boost_fops);
	proc_create("debug_ta_boost", 0644, boost_dir,
					 &perfmgr_debug_ta_boost_fops);
	proc_create("boot_boost", 0644, boost_dir,
					&perfmgr_boot_boost_fops);
	/*--ext_launch--*/
	proc_create("perfserv_ext_launch_mon", 0644, boost_dir,
					&perfmgr_perfserv_ext_launch_mon_fops);

	for (i = 0; i < NR_CGROUP; i++)
		for (j = 0; j < EAS_MAX_KIR; j++)
			boost_value[i][j] = 0;

}

void init_perfmgr_eas_controller(void)
{
	perfmgr_eas_boost_init();
}

