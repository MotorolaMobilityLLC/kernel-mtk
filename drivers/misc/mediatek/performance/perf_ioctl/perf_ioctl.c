/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include "perf_ioctl.h"
#ifdef CONFIG_MTK_FPSGO
#include "../fbc/fbc.h"
#else
#include "../fbc/touch_boost.h"
#endif

#define TAG "PERF_IOCTL"

static struct mutex notify_lock;

/*--------------------DEV OP------------------------*/
static ssize_t device_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[32], cmd[32];
	int arg;

	arg = 0;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%31s %d", cmd, &arg) != 2)
		return -EFAULT;

	return cnt;
}

static int device_show(struct seq_file *m, void *v)
{
	return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}

static long device_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;

	mutex_lock(&notify_lock);

	switch (cmd) {
#ifdef CONFIG_MTK_FPSGO_FBT_GAME
	case FPSGO_QUEUE:
	case FPSGO_DEQUEUE:
		xgf_qudeq_notify(cmd, arg);
		break;
#else
	case FPSGO_QUEUE:
	case FPSGO_DEQUEUE:
		break;
#endif

#ifdef CONFIG_MTK_FPSGO_FBT_UX
	case IOCTL_WRITE_TH:
	case IOCTL_WRITE_FC:
	case IOCTL_WRITE_AS:
	case IOCTL_WRITE_GM:
	case IOCTL_WRITE_IV:
	case IOCTL_WRITE_NR:
	case IOCTL_WRITE_SB:
		fbc_ioctl(cmd, arg);
		break;
#else
	case IOCTL_WRITE_TH:
	case IOCTL_WRITE_FC:
		touch_boost_ioctl(cmd, arg);
		break;
	case IOCTL_WRITE_AS:
	case IOCTL_WRITE_GM:
	case IOCTL_WRITE_IV:
	case IOCTL_WRITE_NR:
	case IOCTL_WRITE_SB:
		break;
#endif

	default:
		pr_debug(TAG "unknown cmd %u\n", cmd);
		ret = -1;
		goto ret_ioctl;
	}

ret_ioctl:
	mutex_unlock(&notify_lock);
	return ret;
}

static const struct file_operations Fops = {
	.unlocked_ioctl = device_ioctl,
	.compat_ioctl = device_ioctl,
	.open = device_open,
	.write = device_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*--------------------INIT------------------------*/
static int __init init_perfctl(void)
{
	struct proc_dir_entry *pe;
	int ret_val = 0;


	pr_debug(TAG"Start to init perf_ioctl driver\n");
#ifdef CONFIG_MTK_FPSGO_FBT_UX
	init_fbc();
#else
	init_touch_boost();
#endif

	mutex_init(&notify_lock);

	ret_val = register_chrdev(DEV_MAJOR, DEV_NAME, &Fops);
	if (ret_val < 0) {
		pr_debug(TAG"%s failed with %d\n",
				"Registering the character device ",
				ret_val);
		goto out_wq;
	}

	pe = proc_create("perfmgr/perf_ioctl", 0664, NULL, &Fops);
	if (!pe) {
		pr_debug(TAG"%s failed with %d\n",
				"Creating file node ",
				ret_val);
		ret_val = -ENOMEM;
		goto out_chrdev;
	}

	pr_debug(TAG"init FBC driver done\n");

	return 0;

out_chrdev:
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
out_wq:
	return ret_val;
}
late_initcall(init_perfctl);

