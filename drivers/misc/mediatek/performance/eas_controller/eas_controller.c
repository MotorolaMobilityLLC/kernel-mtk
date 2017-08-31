#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>

#include <linux/platform_device.h>
#include "eas_controller.h"

#define TAG "[Boost Controller]"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static struct mutex boost_eas;
#ifdef CONFIG_SCHED_TUNE
static int current_boost_value[NR_CGROUP];
#endif
static int boost_value[NR_CGROUP][EAS_MAX_KIR];
static int debug_boost_value[NR_CGROUP];
static int debug;


/*************************************************************************************/
#ifdef CONFIG_SCHED_TUNE
int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	int final_boost_value = 0, final_boost_value_1 = 0, final_boost_value_2 = -101;
	int boost_1[EAS_MAX_KIR], boost_2[EAS_MAX_KIR];
	int has_set = 0;
	int i;

	mutex_lock(&boost_eas);

	if (cgroup_idx >= NR_CGROUP) {
		mutex_unlock(&boost_eas);
		pr_debug(TAG" cgroup_idx >= NR_CGROUP, error\n");
		return -1;
	}

	boost_value[cgroup_idx][kicker] = value;

	for (i = 0; i < EAS_MAX_KIR; i++) {
		boost_1[i] = boost_value[cgroup_idx][i] / 1000;
		boost_2[i] = boost_value[cgroup_idx][i] % 1000;
	}

	for (i = 0; i < EAS_MAX_KIR; i++) {
		if (boost_value[cgroup_idx][i] != 0) {
			final_boost_value_1 = MAX(boost_1[i], final_boost_value_1);
			final_boost_value_2 = MAX(boost_2[i], final_boost_value_2);
			has_set = 1;
		}
	}

	if (has_set)
		final_boost_value = final_boost_value_1 * 1000 + final_boost_value_2;
	else
		final_boost_value = 0;

	if (final_boost_value > 3000)
		current_boost_value[cgroup_idx] = 3000;
	else if (final_boost_value < -100)
		current_boost_value[cgroup_idx] = -100;
	else
		current_boost_value[cgroup_idx] = final_boost_value;

	if (kicker == EAS_KIR_PERF)
		pr_debug(TAG"kicker:%d, boost:%d, final:%d, current:%d",
				kicker, boost_value[cgroup_idx][kicker],
				final_boost_value, current_boost_value[cgroup_idx]);

	if (!debug)
		if (current_boost_value[cgroup_idx] >= -100 && current_boost_value[cgroup_idx] < 3000)
			boost_value_for_GED_idx(cgroup_idx, current_boost_value[cgroup_idx]);

	mutex_unlock(&boost_eas);

	return current_boost_value[cgroup_idx];
}
#else
int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	return -1;
}
#endif

/*************************************************************************************/
static ssize_t perfmgr_perfserv_fg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000)
		data = 3000;
	else if (data < -100)
		data = -100;

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_FG, data);

	return cnt;
}

static int perfmgr_perfserv_fg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_FG][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_fg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_perfserv_fg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_perfserv_fg_boost_fops = {
	.open = perfmgr_perfserv_fg_boost_open,
	.write = perfmgr_perfserv_fg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static int perfmgr_current_fg_boost_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SCHED_TUNE
	seq_printf(m, "%d\n", current_boost_value[CGROUP_FG]);
#else
	seq_printf(m, "%d\n", -1);
#endif

	return 0;
}

static int perfmgr_current_fg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_current_fg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_current_fg_boost_fops = {
	.open = perfmgr_current_fg_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static ssize_t perfmgr_debug_fg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000) {
		debug_boost_value[CGROUP_FG] = 3000;
		debug = 1;
	} else if (data < -100) {
		debug_boost_value[CGROUP_FG] = -100;
		debug = 1;
	} else {
		debug_boost_value[CGROUP_FG] = data;
		debug = 1;
	}

#ifdef CONFIG_SCHED_TUNE
	if (debug)
		boost_value_for_GED_idx(CGROUP_FG, debug_boost_value[CGROUP_FG]);
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

/*************************************************************************************/
static ssize_t perfmgr_perfserv_bg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000)
		data = 3000;
	else if (data < -100)
		data = -100;

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_BG, data);

	return cnt;
}

static int perfmgr_perfserv_bg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_BG][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_bg_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_perfserv_bg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_perfserv_bg_boost_fops = {
	.open = perfmgr_perfserv_bg_boost_open,
	.write = perfmgr_perfserv_bg_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
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
	return single_open(file, perfmgr_current_bg_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_current_bg_boost_fops = {
	.open = perfmgr_current_bg_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static ssize_t perfmgr_debug_bg_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000) {
		debug_boost_value[CGROUP_BG] = 3000;
		debug = 1;
	} else if (data < -100) {
		debug_boost_value[CGROUP_BG] = -100;
		debug = 1;
	} else {
		debug_boost_value[CGROUP_BG] = data;
		debug = 1;
	}

#ifdef CONFIG_SCHED_TUNE
	if (debug)
		boost_value_for_GED_idx(CGROUP_BG, debug_boost_value[CGROUP_BG]);
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


/*************************************************************************************/
static ssize_t perfmgr_perfserv_ta_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000)
		data = 3000;
	else if (data < -100)
		data = -100;

	update_eas_boost_value(EAS_KIR_PERF, CGROUP_TA, data);

	return cnt;
}

static int perfmgr_perfserv_ta_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[CGROUP_TA][EAS_KIR_PERF]);

	return 0;
}

static int perfmgr_perfserv_ta_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_perfserv_ta_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_perfserv_ta_boost_fops = {
	.open = perfmgr_perfserv_ta_boost_open,
	.write = perfmgr_perfserv_ta_boost_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static int perfmgr_current_ta_boost_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SCHED_TUNE
	seq_printf(m, "%d\n", current_boost_value[CGROUP_TA]);
#else
	seq_printf(m, "%d\n", -1);
#endif

	return 0;
}

static int perfmgr_current_ta_boost_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_current_ta_boost_show, inode->i_private);
}

static const struct file_operations perfmgr_current_ta_boost_fops = {
	.open = perfmgr_current_ta_boost_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static ssize_t perfmgr_debug_ta_boost_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int data;
	char buf[128];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;

	if (kstrtoint(buf, 10, &data))
		return -1;

	if (data > 3000) {
		debug_boost_value[CGROUP_TA] = 3000;
		debug = 1;
	} else if (data < -100) {
		debug_boost_value[CGROUP_TA] = -100;
		debug = 1;
	} else {
		debug_boost_value[CGROUP_TA] = data;
		debug = 1;
	}

#ifdef CONFIG_SCHED_TUNE
	if (debug)
		boost_value_for_GED_idx(CGROUP_TA, debug_boost_value[CGROUP_TA]);
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
/*************************************************************************************/
void perfmgr_eas_boost_init(void)
{
	int i, j;
	struct proc_dir_entry *boost_dir = NULL;

	mutex_init(&boost_eas);

	boost_dir = proc_mkdir("perfmgr/eas", NULL);
	proc_create("perfserv_fg_boost", 0644, boost_dir, &perfmgr_perfserv_fg_boost_fops);
	proc_create("current_fg_boost", 0644, boost_dir, &perfmgr_current_fg_boost_fops);
	proc_create("debug_fg_boost", 0644, boost_dir, &perfmgr_debug_fg_boost_fops);

	proc_create("perfserv_bg_boost", 0644, boost_dir, &perfmgr_perfserv_bg_boost_fops);
	proc_create("current_bg_boost", 0644, boost_dir, &perfmgr_current_bg_boost_fops);
	proc_create("debug_bg_boost", 0644, boost_dir, &perfmgr_debug_bg_boost_fops);

	proc_create("perfserv_ta_boost", 0644, boost_dir, &perfmgr_perfserv_ta_boost_fops);
	proc_create("current_ta_boost", 0644, boost_dir, &perfmgr_current_ta_boost_fops);
	proc_create("debug_ta_boost", 0644, boost_dir, &perfmgr_debug_ta_boost_fops);

	for (i = 0; i < NR_CGROUP; i++)
		for (j = 0; j < EAS_MAX_KIR; j++)
			boost_value[i][j] = 0;

}

void init_perfmgr_eas_controller(void)
{
	perfmgr_eas_boost_init();
}

