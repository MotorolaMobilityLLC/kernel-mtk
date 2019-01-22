/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/miscdevice.h>	/* for misc_register, and SYNTH_MINOR */
#include <linux/proc_fs.h>

#ifdef CONFIG_TRACING
#include <linux/kallsyms.h>
#include <linux/trace_events.h>
#endif

#ifdef CONFIG_CPU_FREQ
#include <linux/cpufreq.h>
#endif


#define TAG "[cpuLoading]"

static unsigned long poltime_nsecs; /*set nanoseconds to polling time*/
static int poltime_secs; /*set seconds to polling time*/
static int onoff; /*master switch*/
static int over_threshold; /*threshold value for sent uevent*/
static int uevent_enable; /*sent uevent switch*/
static int cpu_num; /*cpu number*/
static int prev_cpu_loading; /*record previous cpu loading*/
static bool reset_flag; /*check if need to reset value*/

static struct mutex cpu_loading_lock;
static struct workqueue_struct *wq;
static struct hrtimer hrt1;
static void notify_cpu_loading_timeout(void);
static DECLARE_WORK(cpu_loading_timeout_work, (void *) notify_cpu_loading_timeout);/*statically create work */

struct cpu_info {
	cputime64_t time;
};

/* cpu loading tracking */
static struct cpu_info *cur_wall_time, *cur_idle_time, *prev_wall_time, *prev_idle_time;

struct cpu_loading_context   {
	struct miscdevice mdev;
};

struct cpu_loading_context *cpu_loading_object;

#ifdef CONFIG_TRACING
static unsigned long __read_mostly tracing_mark_write_addr;

static inline void __mt_update_tracing_mark_write_addr(void)
{
	if (unlikely(tracing_mark_write_addr == 0))
		tracing_mark_write_addr =
			kallsyms_lookup_name("tracing_mark_write");
}

void cpu_loading_trace_print(char *module, char *string)
{
	__mt_update_tracing_mark_write_addr();
}
#endif

/*hrtimer trigger*/
static void enable_cpu_loading_timer(void)
{
	ktime_t ktime;

	ktime = ktime_set(poltime_secs, poltime_nsecs);
	hrtimer_start(&hrt1, ktime, HRTIMER_MODE_REL);
	cpu_loading_trace_print("cpu_loading", "enable timer");
}

/*close hrtimer*/
static void disable_cpu_loading_timer(void)
{
	hrtimer_cancel(&hrt1);
	cpu_loading_trace_print("cpu_loading", "disable timer");
}

static enum hrtimer_restart handle_cpu_loading_timeout(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &cpu_loading_timeout_work);/*submit work[handle_cpu_loading_timeout_work] to workqueue*/
	return HRTIMER_NORESTART;
}

/* reset value if anyone change switch */
/* 1.uevent_enable  */
/* 2.poltime_secs   */
/* 3.over_threshold */
/* 4.poltime_nsecs  */

void reset_cpu_loading(void)
{

	int i;


	for_each_possible_cpu(i) {
		cur_wall_time[i].time = prev_wall_time[i].time = 0;
		cur_idle_time[i].time = prev_idle_time[i].time = 0;
	}

	prev_cpu_loading = 0;

}

/*update info*/
int update_cpu_loading(void)
{
	int j, ret;
	int i, tmp_cpu_loading = 0;

	char *envp[2];
	int string_size = 15;
	char  event_string[string_size];

	cputime64_t wall_time = 0, idle_time = 0;

	envp[0] = event_string;
	envp[1] = NULL;

	mutex_lock(&cpu_loading_lock);

	cpu_loading_trace_print("cpu_loading", "update cpu_loading");

	for_each_possible_cpu(j) {
		/*idle time include iowait time*/
		cur_idle_time[j].time = get_cpu_idle_time(j, &cur_wall_time[j].time, 1);
	}

	if (reset_flag) {
		cpu_loading_trace_print("cpu_loading", "return reset");

		reset_flag = false;/*meet global value at first,and then set info to false  */

		mutex_unlock(&cpu_loading_lock);

		return 0;
	}

	for_each_possible_cpu(i) {
		wall_time += cur_wall_time[i].time - prev_wall_time[i].time;
		idle_time += cur_idle_time[i].time - prev_idle_time[i].time;
	}

	tmp_cpu_loading = div_u64((100 * (wall_time - idle_time)), wall_time);


	if (tmp_cpu_loading > over_threshold &&  over_threshold > prev_cpu_loading) {
		prev_cpu_loading = tmp_cpu_loading;

		/*send uevent*/
		if (uevent_enable) {
			strlcpy(event_string, "over=1", string_size);

			ret = kobject_uevent_env(&cpu_loading_object->mdev.this_device->kobj, KOBJ_CHANGE, envp);
			if (ret != 0) {
				cpu_loading_trace_print("cpu_loading", "uevent failed");
				mutex_unlock(&cpu_loading_lock);
				return -1;
			}
			cpu_loading_trace_print("cpu_loading", "sent uevent over success");
		}
		mutex_unlock(&cpu_loading_lock);
		return 1;
	}

	prev_cpu_loading = tmp_cpu_loading;

	cpu_loading_trace_print("cpu_loading", "no change situation");
	mutex_unlock(&cpu_loading_lock);
	return 3;
}

static void notify_cpu_loading_timeout(void)
{
	int i;

	mutex_lock(&cpu_loading_lock);

	if (reset_flag)
		reset_cpu_loading();
	else{

		for_each_possible_cpu(i) {
			prev_wall_time[i].time = cur_wall_time[i].time;
			prev_idle_time[i].time = cur_idle_time[i].time;

		}
	}

	mutex_unlock(&cpu_loading_lock);

	update_cpu_loading();
	enable_cpu_loading_timer();

}

/*default setting*/
void init_cpu_loading_value(void)
{

	mutex_lock(&cpu_loading_lock);

	/*default setting*/
	over_threshold = 85;
	poltime_secs = 10;
	poltime_nsecs = 0;
	onoff = 0;
	uevent_enable = 1;
	prev_cpu_loading = 0;

	mutex_unlock(&cpu_loading_lock);
}

static int mt_onof_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", onoff);
	return 0;
}

static ssize_t mt_onof_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	char buf[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val)
		enable_cpu_loading_timer();
	else
		disable_cpu_loading_timer();

	mutex_lock(&cpu_loading_lock);

	onoff  = val;
	reset_flag = true;

	mutex_unlock(&cpu_loading_lock);

	return cnt;
}

static int mt_onof_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_onof_show, inode->i_private);
}

static const struct file_operations mt_onof_fops = {
	.open = mt_onof_open,
	.write = mt_onof_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_poltime_secs_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", poltime_secs);
	return 0;
}

static ssize_t mt_poltime_secs_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	int ret, val;
	char buf[64];


	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	ret = kstrtoint(buf, 10, &val);

	if (ret < 0)
		return ret;

	if (val > INT_MAX || val < 0)
		return -EINVAL;

	/*check both poltime_secs and poltime_nsecs can't be zero */
	if (!(poltime_nsecs | val))
		return -EINVAL;

	mutex_lock(&cpu_loading_lock);

	poltime_secs  = val;
	reset_flag = true;

	mutex_unlock(&cpu_loading_lock);

	return cnt;
}

static int mt_poltime_secs_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_poltime_secs_show, inode->i_private);
}

static const struct file_operations mt_poltime_secs_fops = {
	.open = mt_poltime_secs_open,
	.write = mt_poltime_secs_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_poltime_nsecs_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", poltime_nsecs);
	return 0;
}

static ssize_t mt_poltime_nsecs_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	unsigned long ret, val;

	char buf[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;

	ret = _kstrtoul(buf, 10, &val);

	if (ret < 0)
		return ret;

	if (val > UINT_MAX || val < 0)
		return -EINVAL;

	/*check both poltime_secs and poltime_nsecs can't be zero */
	if (!(poltime_secs | val))
		return -EINVAL;

	mutex_lock(&cpu_loading_lock);

	poltime_nsecs  = val;
	reset_flag = true;

	mutex_unlock(&cpu_loading_lock);

	return cnt;
}

static int mt_poltime_nsecs_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_poltime_nsecs_show, inode->i_private);
}

static const struct file_operations mt_poltime_nsecs_fops = {
	.open = mt_poltime_nsecs_open,
	.write = mt_poltime_nsecs_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_overThrshold_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", over_threshold);
	return 0;
}

static ssize_t mt_overThrshold_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	int ret;
	int val;

	char buf[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;


	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

	mutex_lock(&cpu_loading_lock);

	over_threshold = val;
	reset_flag = true;

	mutex_unlock(&cpu_loading_lock);

	return cnt;
}

static int mt_overThrshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_overThrshold_show, inode->i_private);
}

static const struct file_operations mt_overThrshold_fops = {
	.open = mt_overThrshold_open,
	.write = mt_overThrshold_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_uevent_enble_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", uevent_enable);
	return 0;
}

static ssize_t mt_uevent_enble_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;
	char buf[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;

	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);

	if (ret < 0)
		return ret;

	mutex_lock(&cpu_loading_lock);

	uevent_enable = val;
	reset_flag = true;

	mutex_unlock(&cpu_loading_lock);

	return cnt;
}

static int mt_uevent_enble_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_uevent_enble_show, inode->i_private);
}

static const struct file_operations mt_uevent_enble_fops = {
	.open = mt_uevent_enble_open,
	.write = mt_uevent_enble_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_prev_cpu_loading_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", prev_cpu_loading);
	return 0;
}

static int mt_prev_cpu_loading_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_prev_cpu_loading_show, inode->i_private);
}

static const struct file_operations mt_prev_cpu_loading_fops = {
	.open = mt_prev_cpu_loading_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*--------------------INIT------------------------*/
static int init_cpu_loading_kobj(void)
{
	int ret;

	cpu_loading_object = kzalloc(sizeof(struct cpu_loading_context), GFP_KERNEL);

	/* dev init */

	cpu_loading_object->mdev.name = "cpu_loading";
	ret = misc_register(&cpu_loading_object->mdev);
	if (ret) {
		pr_debug(TAG"misc_register error:%d\n", ret);
		return -2;
	}

	ret = kobject_uevent(&cpu_loading_object->mdev.this_device->kobj, KOBJ_ADD);

	if (ret) {
		cpu_loading_trace_print("cpu_loading", "uevent creat  fail");
		return -4;
	}

	return 0;
}

static int __init init_cpu_loading(void)
{
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *cpu_loading_dir = NULL;
	int i;

	/*calc cpu num*/
	cpu_num = num_possible_cpus();

	cur_wall_time  =  kcalloc(cpu_num, sizeof(struct cpu_info), GFP_KERNEL);
	cur_idle_time  =  kcalloc(cpu_num, sizeof(struct cpu_info), GFP_KERNEL);
	prev_wall_time  =  kcalloc(cpu_num, sizeof(struct cpu_info), GFP_KERNEL);
	prev_idle_time  =  kcalloc(cpu_num, sizeof(struct cpu_info), GFP_KERNEL);

	for_each_possible_cpu(i) {
		prev_wall_time[i].time = cur_wall_time[i].time = 0;
		prev_idle_time[i].time = cur_idle_time[i].time = 0;
	}

	/*mutex init*/
	mutex_init(&cpu_loading_lock);

	/*workqueue init*/
	wq = create_singlethread_workqueue("cpu_loading_work");
	if (!wq)
		return -ENOMEM;

	/*timer init*/
	hrtimer_init(&hrt1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt1.function = &handle_cpu_loading_timeout;

	/* dev init */
	init_cpu_loading_kobj();

	init_cpu_loading_value();
	/* poting */
	cpu_loading_dir = proc_mkdir("cpu_loading", NULL);

	if (!cpu_loading_dir)
		return -ENOMEM;

	proc_entry = proc_create("onof", 0664, cpu_loading_dir, &mt_onof_fops);
	if (!proc_entry)
		return -ENOMEM;

	proc_entry = proc_create("poltime_secs", 0664, cpu_loading_dir, &mt_poltime_secs_fops);
	if (!proc_entry)
		return -ENOMEM;

	proc_entry = proc_create("poltime_nsecs", 0664, cpu_loading_dir, &mt_poltime_nsecs_fops);
	if (!proc_entry)
		return -ENOMEM;

	proc_entry = proc_create("overThrhld", 0664, cpu_loading_dir, &mt_overThrshold_fops);
	if (!proc_entry)
		return -ENOMEM;

	proc_entry = proc_create("uevent_enable", 0664, cpu_loading_dir, &mt_uevent_enble_fops);
	if (!proc_entry)
		return -ENOMEM;

	proc_entry = proc_create("prev_cpu_loading", 0644, cpu_loading_dir, &mt_prev_cpu_loading_fops);
	if (!proc_entry)
		return -ENOMEM;

	return 0;
}

void exit_cpu_loading(void)
{
	kfree(cpu_loading_object);
	kfree(cur_wall_time);
	kfree(cur_idle_time);
	kfree(prev_wall_time);
	kfree(prev_idle_time);
}

module_init(init_cpu_loading);
module_exit(exit_cpu_loading);

