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



#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static struct mutex boost_eas;
static int current_fg_boost_value;
static int boost_value[MAX_KIR];
static int fg_debug_boost_value;
static int debug;


/*************************************************************************************/
#ifdef CONFIG_SCHED_TUNE
static int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	int final_boost_value = -1;
	int i;

	mutex_lock(&boost_eas);

	for (i = 0; i < MAX_KIR; i++) {
		if (cgroup_idx == CGROUP_FG)
			final_boost_value = MAX(boost_value[i], final_boost_value);
	}

	if (final_boost_value > 3000)
		current_fg_boost_value = 3000;
	else if (final_boost_value < 0)
		current_fg_boost_value = -1;
	else
		current_fg_boost_value = final_boost_value;

	if (!debug) {
		if (current_fg_boost_value > 0)
			boost_value_for_GED_idx(CGROUP_FG, current_fg_boost_value);
		else
			boost_value_for_GED_idx(CGROUP_FG, 0);
	}

	mutex_unlock(&boost_eas);

	return current_fg_boost_value;
}
#else
static int update_eas_boost_value(int kicker, int cgroup_idx, int value)
{
	return -1;
}
#endif

/*************************************************************************************/
#ifdef CONFIG_SCHED_TUNE
int perfmgr_kick_fg_boost(int kicker, int value)
{
	boost_value[kicker] = value;
	return update_eas_boost_value(kicker, CGROUP_FG, boost_value[kicker]);
}
#else
int perfmgr_kick_fg_boost(int kicker, int value)
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
		boost_value[KIR_PERF] = 3000;
	else if (data < 0)
		boost_value[KIR_PERF] = -1;
	else
		boost_value[KIR_PERF] = data;

	update_eas_boost_value(KIR_PERF, CGROUP_FG, boost_value[KIR_PERF]);

	return cnt;
}

static int perfmgr_perfserv_fg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", boost_value[KIR_PERF]);

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
	seq_printf(m, "%d\n", current_fg_boost_value);
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
		fg_debug_boost_value = 3000;
		debug = 1;
	} else if (data < 0) {
		fg_debug_boost_value = -1;
		debug = 0;
	} else {
		fg_debug_boost_value = data;
		debug = 1;
	}

#ifdef CONFIG_SCHED_TUNE
	if (debug)
		boost_value_for_GED_idx(CGROUP_FG, fg_debug_boost_value);
#endif

	return cnt;
}

static int perfmgr_debug_fg_boost_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fg_debug_boost_value);

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
void perfmgr_eas_boost_init(void)
{
	int i;
	struct proc_dir_entry *boost_dir = NULL;

	mutex_init(&boost_eas);

	boost_dir = proc_mkdir("perfmgr/eas", NULL);

	proc_create("perfserv_fg_boost", 0644, boost_dir, &perfmgr_perfserv_fg_boost_fops);
	proc_create("current_fg_boost", 0644, boost_dir, &perfmgr_current_fg_boost_fops);
	proc_create("debug_fg_boost", 0644, boost_dir, &perfmgr_debug_fg_boost_fops);

	current_fg_boost_value = -1;
	for (i = 0; i < MAX_KIR; i++)
		boost_value[i] = -1;

	fg_debug_boost_value = -1;
	debug = 0;
}

void init_perfmgr_eas_controller(void)
{
	perfmgr_eas_boost_init();
}

