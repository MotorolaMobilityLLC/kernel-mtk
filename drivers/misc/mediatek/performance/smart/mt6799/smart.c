/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kobject.h>

#include <linux/platform_device.h>
#include "smart.h"

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#undef TAG
#define TAG "[SMART]"

#define S_LOG(fmt, args...)		pr_debug(TAG fmt, ##args)

static int enable_hps_smart_checking;
/*static unsigned long Y;*/
static unsigned long Y_ms;
static int is_heavy;
static int uevent_enable;

static int app_is_benchmark; /* Is foreground benchmark? set from perfservice */
static int app_is_running;   /* Is app running */
static unsigned long app_load_thresh;
static unsigned long app_tlp_thresh;
static unsigned long app_btask_thresh;
static unsigned long app_up_times;
static unsigned long app_down_times;

static int native_is_running;   /* Is native running */
static unsigned long native_load_thresh;
static unsigned long native_tlp_thresh;
static unsigned long native_btask_thresh;
static unsigned long native_up_times;
static unsigned long native_down_times;

static unsigned long app_up_count;
static unsigned long app_down_count;
static unsigned long native_up_count;
static unsigned long native_down_count;
static unsigned long native_pid;

static struct timeval up_time;
static struct timeval down_time;
static unsigned long elapsed_time;
static struct notifier_block cpu_hotplug_lowpower_nb;

struct smart_data {
	int is_hps_heavy;
	unsigned long check_duration;
	unsigned long valid_duration;
	int is_app_benchmark;
};

struct smart_context {
	struct input_dev   *idev;
	struct miscdevice   mdev;
	struct mutex        s_op_mutex;
	struct smart_data   s_data;
};

struct smart_context *smart_context_obj;


/* Operaton */

void smart_notify_BW_heavy(void)
{
#if 0
	int ret;
	char event_vcore[9] = "DETECT=5"; /* VCORE*/
	char *envp_vcore[2] = { event_vcore, NULL };
#endif
	S_LOG("smart notify BW heavy");
#if 0
	if (smart_context_obj) {
		ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj, KOBJ_CHANGE, envp_vcore);
		if (ret) {
			pr_debug(TAG"kobject_uevent error:%d\n", ret);
			return;
		}
	}
#endif
}

static ssize_t smart_show_hps_check_duration(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct smart_context *cxt = NULL;
	unsigned long duration = 0;

	cxt = smart_context_obj;
	duration = cxt->s_data.check_duration;
	S_LOG("check_duration: %lu\n", duration);
	return snprintf(buf, PAGE_SIZE, "%lu\n", duration);
}

static ssize_t smart_store_hps_check_duration(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long duration = 0;
	int ret = 0;
	struct smart_context *cxt = NULL;

	mutex_lock(&smart_context_obj->s_op_mutex);
	cxt = smart_context_obj;

	ret = kstrtoul(buf, 10, &duration);
	if (ret != 0) {
		S_LOG("invalid format!!\n");
		goto ERR_EXIT;
	}

	cxt->s_data.check_duration = duration;
	S_LOG("check duration:%lu\n", duration);
ERR_EXIT:
	mutex_unlock(&smart_context_obj->s_op_mutex);
	return count;
}

static ssize_t smart_show_hps_valid_duration(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct smart_context *cxt = NULL;
	unsigned long duration = 0;

	cxt = smart_context_obj;
	duration = cxt->s_data.valid_duration;
	S_LOG("valid duration: %lu\n", duration);
	return snprintf(buf, PAGE_SIZE, "%lu\n", duration);
}

static ssize_t smart_store_hps_valid_duration(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long duration = 0;
	int ret = 0;
	struct smart_context *cxt = NULL;

	mutex_lock(&smart_context_obj->s_op_mutex);
	cxt = smart_context_obj;

	ret = kstrtoul(buf, 10, &duration);
	if (ret != 0) {
		S_LOG("invalid format!!\n");
		goto ERR_EXIT;
	}

	cxt->s_data.valid_duration = duration;
	S_LOG("valid duration:%lu\n", duration);
ERR_EXIT:
	mutex_unlock(&smart_context_obj->s_op_mutex);
	return count;
}

static ssize_t smart_show_hps_is_heavy(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct smart_context *cxt = NULL;
	int is_heavy = 0;

	cxt = smart_context_obj;
	is_heavy = cxt->s_data.is_hps_heavy;
	S_LOG("is_heavy: %d\n", is_heavy);
	return snprintf(buf, PAGE_SIZE, "%d\n", is_heavy);
}

static ssize_t smart_store_hps_is_heavy(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int is_heavy = 0;
	int ret = 0;
	struct smart_context *cxt = NULL;

	mutex_lock(&smart_context_obj->s_op_mutex);
	cxt = smart_context_obj;

	ret = kstrtoint(buf, 10, &is_heavy);
	if (ret != 0) {
		S_LOG("invalid format!!\n");
		goto ERR_EXIT;
	}

	cxt->s_data.is_hps_heavy = is_heavy;
	S_LOG(" is_heavy:%d\n", is_heavy);
ERR_EXIT:
	mutex_unlock(&smart_context_obj->s_op_mutex);
	return count;
}


DEVICE_ATTR(hps_check_duration, S_IWUSR | S_IRUGO, smart_show_hps_check_duration, smart_store_hps_check_duration);
DEVICE_ATTR(hps_valid_duration, S_IWUSR | S_IRUGO, smart_show_hps_valid_duration, smart_store_hps_valid_duration);
DEVICE_ATTR(hps_is_heavy, S_IWUSR | S_IRUGO, smart_show_hps_is_heavy, smart_store_hps_is_heavy);

static struct attribute *smart_attributes[] = {
	&dev_attr_hps_check_duration.attr,
	&dev_attr_hps_valid_duration.attr,
	&dev_attr_hps_is_heavy.attr,
	NULL
};

static struct attribute_group smart_attribute_group = {
	.attrs = smart_attributes
};

/******* FLIPER SETTING *********/
static ssize_t mt_hps_check_last_duration_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		Y_ms = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_hps_check_last_duration_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", elapsed_time);
	return 0;
}

static ssize_t mt_hps_check_duration_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		Y_ms = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_hps_check_duration_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", Y_ms);
	return 0;
}

static ssize_t mt_hps_is_heavy_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		if (arg == 0) {
			enable_hps_smart_checking = 1;
			is_heavy = 0;
		}
		if (arg == 1)
			enable_hps_smart_checking = 0;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_hps_is_heavy_show(struct seq_file *m, void *v)
{
	if (is_heavy && enable_hps_smart_checking)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

static ssize_t mt_app_is_benchmark_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		if (arg == 0) {
			app_is_benchmark = 0;
			native_up_count = 0;
			native_down_count = 0;
		} else if (arg == 1) {
			app_is_benchmark = 1;
			app_up_count = 0;
			app_down_count = 0;
		}
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_is_benchmark_show(struct seq_file *m, void *v)
{
	if (app_is_benchmark)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

static ssize_t mt_app_is_running_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		if (arg == 0)
			app_is_running = 0;
		else if (arg == 1)
			app_is_running = 1;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_is_running_show(struct seq_file *m, void *v)
{
	if (app_is_running)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

static ssize_t mt_app_load_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		app_load_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_load_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", app_load_thresh);
	return 0;
}

static ssize_t mt_app_tlp_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		app_tlp_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_tlp_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", app_tlp_thresh);
	return 0;
}

static ssize_t mt_app_btask_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		app_btask_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_btask_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", app_btask_thresh);
	return 0;
}

static ssize_t mt_app_up_times_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		app_up_times = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_up_times_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", app_up_times);
	return 0;
}

static ssize_t mt_app_down_times_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		app_down_times = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_app_down_times_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", app_down_times);
	return 0;
}

static ssize_t mt_native_is_running_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		if (arg == 0)
			native_is_running = 0;
		else if (arg == 1)
			native_is_running = 1;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_is_running_show(struct seq_file *m, void *v)
{
	if (native_is_running)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

static ssize_t mt_native_load_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_load_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_load_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_load_thresh);
	return 0;
}

static ssize_t mt_native_tlp_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_tlp_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_tlp_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_tlp_thresh);
	return 0;
}

static ssize_t mt_native_btask_thresh_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_btask_thresh = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_btask_thresh_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_btask_thresh);
	return 0;
}

static ssize_t mt_native_up_times_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_up_times = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_up_times_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_up_times);
	return 0;
}

static ssize_t mt_native_down_times_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_down_times = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_down_times_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_down_times);
	return 0;
}

static ssize_t mt_native_pid_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		native_pid = arg;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_native_pid_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%lu\n", native_pid);
	return 0;
}

static ssize_t mt_hps_uevent_enable_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret, len = 0;
	unsigned long arg;

	len = (cnt < (sizeof(buf) - 1)) ? cnt : (sizeof(buf) - 1);

	if (copy_from_user(&buf, ubuf, len))
		return -EFAULT;
	buf[cnt] = '\0';

	if (!kstrtoul(buf, 0, (unsigned long *)&arg)) {
		ret = 0;
		if (arg == 0)
			uevent_enable = 0;
		if (arg == 1)
			uevent_enable = 1;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;

}

static int mt_hps_uevent_enable_show(struct seq_file *m, void *v)
{
	if (uevent_enable)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

/*** Seq operation of mtprof ****/
static int mt_hps_check_last_duration_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_hps_check_last_duration_show, inode->i_private);
}

static const struct file_operations mt_hps_check_last_duration_fops = {
	.open = mt_hps_check_last_duration_open,
	.write = mt_hps_check_last_duration_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_hps_check_duration_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_hps_check_duration_show, inode->i_private);
}

static const struct file_operations mt_hps_check_duration_fops = {
	.open = mt_hps_check_duration_open,
	.write = mt_hps_check_duration_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_hps_is_heavy_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_hps_is_heavy_show, inode->i_private);
}

static const struct file_operations mt_hps_is_heavy_fops = {
	.open = mt_hps_is_heavy_open,
	.write = mt_hps_is_heavy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_is_benchmark_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_is_benchmark_show, inode->i_private);
}

static const struct file_operations mt_app_is_benchmark_fops = {
	.open = mt_app_is_benchmark_open,
	.write = mt_app_is_benchmark_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_is_running_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_is_running_show, inode->i_private);
}

static const struct file_operations mt_app_is_running_fops = {
	.open = mt_app_is_running_open,
	.write = mt_app_is_running_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_load_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_load_thresh_show, inode->i_private);
}

static const struct file_operations mt_app_load_thresh_fops = {
	.open = mt_app_load_thresh_open,
	.write = mt_app_load_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_tlp_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_tlp_thresh_show, inode->i_private);
}

static const struct file_operations mt_app_tlp_thresh_fops = {
	.open = mt_app_tlp_thresh_open,
	.write = mt_app_tlp_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_btask_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_btask_thresh_show, inode->i_private);
}

static const struct file_operations mt_app_btask_thresh_fops = {
	.open = mt_app_btask_thresh_open,
	.write = mt_app_btask_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_up_times_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_up_times_show, inode->i_private);
}

static const struct file_operations mt_app_up_times_fops = {
	.open = mt_app_up_times_open,
	.write = mt_app_up_times_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_app_down_times_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_app_down_times_show, inode->i_private);
}

static const struct file_operations mt_app_down_times_fops = {
	.open = mt_app_down_times_open,
	.write = mt_app_down_times_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_is_running_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_is_running_show, inode->i_private);
}

static const struct file_operations mt_native_is_running_fops = {
	.open = mt_native_is_running_open,
	.write = mt_native_is_running_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_load_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_load_thresh_show, inode->i_private);
}

static const struct file_operations mt_native_load_thresh_fops = {
	.open = mt_native_load_thresh_open,
	.write = mt_native_load_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_tlp_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_tlp_thresh_show, inode->i_private);
}

static const struct file_operations mt_native_tlp_thresh_fops = {
	.open = mt_native_tlp_thresh_open,
	.write = mt_native_tlp_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_btask_thresh_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_btask_thresh_show, inode->i_private);
}

static const struct file_operations mt_native_btask_thresh_fops = {
	.open = mt_native_btask_thresh_open,
	.write = mt_native_btask_thresh_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_up_times_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_up_times_show, inode->i_private);
}

static const struct file_operations mt_native_up_times_fops = {
	.open = mt_native_up_times_open,
	.write = mt_native_up_times_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_down_times_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_down_times_show, inode->i_private);
}

static const struct file_operations mt_native_down_times_fops = {
	.open = mt_native_down_times_open,
	.write = mt_native_down_times_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_native_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_native_pid_show, inode->i_private);
}

static const struct file_operations mt_native_pid_fops = {
	.open = mt_native_pid_open,
	.write = mt_native_pid_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mt_hps_uevent_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_hps_uevent_enable_show, inode->i_private);
}

static const struct file_operations mt_hps_uevent_enable_fops = {
	.open = mt_hps_uevent_enable_open,
	.write = mt_hps_uevent_enable_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*----------------hps notifier--------------------*/

#if 0
static DECLARE_BITMAP(cpu_cluster0_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_cluster0_mask = to_cpumask(cpu_cluster0_bits);

static DECLARE_BITMAP(cpu_cluster1_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_cluster1_mask = to_cpumask(cpu_cluster1_bits);

static unsigned long default_cluster0_mask = 0x000F;
static unsigned long default_cluster1_mask = 0x00F0;
#endif
static int cpu_hotplug_lowpower_notifier(struct notifier_block *self,
		unsigned long action, void *hcpu)
{
	int ret;
	char event_hps[9] = "DETECT=4"; /* HPS*/
	char event_tlp[9] = "DETECT=6"; /* TLP*/
	char event_act[9] = "ACTION=1"; /* N/A*/
	char *envp_hps[3] = { event_hps, event_act, NULL };
	char *envp_tlp[3] = { event_tlp, event_act, NULL };

	if (!uevent_enable)
		return NOTIFY_OK;

	switch (action) {
	case CPU_ONLINE:
		if (cpumask_weight(cpu_online_mask) == 8) {
			do_gettimeofday(&up_time);
			pr_debug(TAG"all core on, TS:%lu\n", up_time.tv_usec);

			if (smart_context_obj) {
				ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
				 KOBJ_CHANGE, envp_tlp);
				if (ret) {
					pr_debug(TAG"kobject_uevent error:%d\n", ret);
					return -3;
				}
			}
		}
		break;

	case CPU_DEAD:
		if (cpumask_weight(cpu_online_mask) == 7) {
			do_gettimeofday(&down_time);
			pr_debug(TAG"all core on disable, TS:%lu\n", down_time.tv_usec);
			elapsed_time =
				(down_time.tv_sec - up_time.tv_sec) * 1000 +
				(down_time.tv_usec - up_time.tv_usec) / 1000;
			if (elapsed_time > Y_ms) {
				is_heavy = 1;

				if (smart_context_obj) {
					ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
						KOBJ_CHANGE, envp_hps);
					if (ret) {
						pr_debug(TAG"kobject_uevent error:%d\n", ret);
						return -3;
					}
				}
			}
			pr_debug(TAG"all core on time = %lu, is_heavy = %d\n", elapsed_time, is_heavy);
		}
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}


void mt_smart_update_sysinfo(unsigned int cur_loads, unsigned int cur_tlp, unsigned int btask)
{
	int ret;
	char event_running[9]  = "DETECT=8"; /* running */
	char event_act_up[9]   = "ACTION=1"; /* up   */
	char event_act_down[9] = "ACTION=0"; /* down */
	char *envp_up[3] = { event_running, event_act_up, NULL };
	char *envp_down[3] = { event_running, event_act_down, NULL };
#if 0
	char event_native[9]      = "DETECT=9"; /* running */
	char *envp_native_up[3] = { event_native, event_act_up, NULL };
	char *envp_native_down[3] = { event_native, event_act_down, NULL };
	int  pid = 0;
#endif

#if 0
	pr_crit(TAG"get_sysinfo benchmark:%d, cur_load:%d, cur_tlp:%d, btask:%d\n",
	app_is_benchmark, cur_loads, cur_tlp, btask);
#endif

	if (app_is_benchmark == 1) { /* foreground app is JAVA benchmark */
		/* benchmark is not running */
		if (app_is_running == 0) {
			if (cur_loads >= app_load_thresh && btask >= app_btask_thresh)
				app_up_count++;
			else
				app_up_count = 0;

			if (app_up_count >= app_up_times) {
				pr_crit(TAG"get_sysinfo benchmark is running!!!\n");
				app_is_running = 1;
				if (smart_context_obj) {
					ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
					KOBJ_CHANGE, envp_up);
					if (ret) {
						pr_debug(TAG"kobject_uevent error:%d\n", ret);
						return;
					}
				}
			}
		} else { /* app_is_running == 1 */
			if (cur_loads < app_load_thresh || btask < app_btask_thresh)
				app_down_count++;
			else
				app_down_count = 0;

			if (app_down_count >= app_down_times) {
				pr_crit(TAG"get_sysinfo benchmark is not running!!!\n");
				app_is_running = 0;
				if (smart_context_obj) {
					ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
					KOBJ_CHANGE, envp_down);
					if (ret)
						pr_debug(TAG"kobject_uevent error:%d\n", ret);
				}
			}
		}
	}
#if 0
	else { /* detect native benchmark */
		if (native_is_running == 0) {
			if (btask >= native_btask_thresh)
				native_up_count++;
			else
				native_up_count = 0;

			if (native_up_count >= native_up_times) {
				pr_crit(TAG"get_sysinfo MATCH!!!!!\n");
				native_is_running = 1;
				native_up_count = 0; /* reset */
				sched_max_util_task(NULL, &pid, NULL, NULL);	/* the heavist task */
				native_pid = pid;
				if (smart_context_obj) { /* native is running */
					ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
					KOBJ_CHANGE, envp_native_up);
					if (ret)
						pr_debug(TAG"kobject_uevent error:%d\n", ret);
				}
			}
		} else {
			if (btask < native_btask_thresh)
				native_down_count++;

			if (native_down_count >= native_down_times) {
				pr_crit(TAG"get_sysinfo EXIT!!!!!\n");
				native_is_running = 0;
				native_down_count = 0; /* reset */
				if (smart_context_obj) { /* native is stop */
					ret = kobject_uevent_env(&smart_context_obj->mdev.this_device->kobj,
					KOBJ_CHANGE, envp_native_down);
					if (ret)
						pr_debug(TAG"kobject_uevent error:%d\n", ret);
				}
			}

		}
	}
#endif
}


/*--------------------INIT------------------------*/
static int init_smart_obj(void)
{
	int ret;

	/* dev init */
	struct smart_context *obj = kzalloc(sizeof(*obj), GFP_KERNEL);

#if 0
	if (!obj) {
		pr_debug(TAG"kzalloc error\n");
		return -1;
	}
#endif

	mutex_init(&obj->s_op_mutex);
	smart_context_obj = obj;
	smart_context_obj->mdev.minor = MISC_DYNAMIC_MINOR;
	smart_context_obj->mdev.name = "m_smart_misc";
	ret = misc_register(&smart_context_obj->mdev);
	if (ret) {
		pr_debug(TAG"misc_register error:%d\n", ret);
		return -2;
	}

	ret = sysfs_create_group(&smart_context_obj->mdev.this_device->kobj, &smart_attribute_group);
	if (ret < 0) {
		pr_debug(TAG"sysfs_create_group error:%d\n", ret);
		return -3;
	}
	ret = kobject_uevent(&smart_context_obj->mdev.this_device->kobj, KOBJ_ADD);
	if (ret) {
		pr_debug(TAG"kobject_uevent error:%d\n", ret);
		return -4;
	}

	/* init data */
	smart_context_obj->s_data.check_duration = 5000;
	smart_context_obj->s_data.valid_duration = 0;
	smart_context_obj->s_data.is_hps_heavy = 0;

	return 0;
}

	static
int __init init_smart(void)
{
	int ret;
	struct proc_dir_entry *pe;
	struct proc_dir_entry *smart_dir = NULL;

	/* dev init */
	init_smart_obj();

	/* poting */
	smart_dir = proc_mkdir("perfmgr/smart", NULL);

	pr_debug(TAG"init smart driver start\n");
	pe = proc_create("hps_check_last_duration", 0644, smart_dir, &mt_hps_check_last_duration_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("hps_check_duration", 0644, smart_dir, &mt_hps_check_duration_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("hps_is_heavy", 0644, smart_dir, &mt_hps_is_heavy_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("hps_uevent_enable", 0644, smart_dir, &mt_hps_uevent_enable_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_is_benchmark", 0644, smart_dir, &mt_app_is_benchmark_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_is_running", 0644, smart_dir, &mt_app_is_running_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_load_thresh", 0644, smart_dir, &mt_app_load_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_tlp_thresh", 0644, smart_dir, &mt_app_tlp_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_btask_thresh", 0644, smart_dir, &mt_app_btask_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_up_times", 0644, smart_dir, &mt_app_up_times_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("app_down_times", 0644, smart_dir, &mt_app_down_times_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_is_running", 0644, smart_dir, &mt_native_is_running_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_load_thresh", 0644, smart_dir, &mt_native_load_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_tlp_thresh", 0644, smart_dir, &mt_native_tlp_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_btask_thresh", 0644, smart_dir, &mt_native_btask_thresh_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_up_times", 0644, smart_dir, &mt_native_up_times_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_down_times", 0644, smart_dir, &mt_native_down_times_fops);
	if (!pe)
		return -ENOMEM;
	pe = proc_create("native_pid", 0644, smart_dir, &mt_native_pid_fops);
	if (!pe)
		return -ENOMEM;

	Y_ms = 5000;

	is_heavy = 0;
	app_is_benchmark = 0;
	app_is_running = 0;
	app_load_thresh = 100; /* loading should > 100 */
	app_tlp_thresh = 100;  /* don't use */
	app_btask_thresh = 1;  /* btask should >= 1 */
	app_up_times = 10;       /* 40ms * 10 = 400ms */
	app_down_times = 100;    /* 40ms * 10 = 4000ms */

	native_is_running = 0;
	native_load_thresh = 250; /* loading should < 250 */
	native_tlp_thresh = 100;  /* don't use */
	native_btask_thresh = 1;  /* btask should == 1 */
	native_up_times = 200;    /* 40ms * 200 = 8000ms */
	native_down_times = 2000; /* 40ms * 2000 = 80000ms */
	native_pid = 0;

#if 1
	uevent_enable = 1; /* smart.c will send uevent to user space */
#else
	uevent_enable = 0;
#endif
	cpu_hotplug_lowpower_nb = (struct notifier_block) {
		.notifier_call	= cpu_hotplug_lowpower_notifier,
			.priority	= CPU_PRI_PERF + 1,
	};

	ret = register_cpu_notifier(&cpu_hotplug_lowpower_nb);
	if (ret)
		return ret;

	pr_debug(TAG"init smart driver done\n");

	return 0;
}
late_initcall(init_smart);

