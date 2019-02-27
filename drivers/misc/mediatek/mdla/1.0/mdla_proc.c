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

#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "mdla_trace.h"
#include "mdla_debug.h"

static const char *proc_dirname = "mdla";
static struct proc_dir_entry *mdla_proc_root;
static struct proc_dir_entry *mdla_pprof;

#define PROC_BUFSIZE 32

static char prof_data[PROC_BUFSIZE];

ssize_t prof_write(struct file *filp,
	const char __user *buff, size_t len, loff_t *ppos)
{
	int in;
	char str[PROC_BUFSIZE], *p;
	//int type;

	in = (len >= PROC_BUFSIZE) ? PROC_BUFSIZE-1 : len;

	if (copy_from_user(&str[0], buff, in))
		return -EFAULT;

	str[in] = 0;
	p = strim(str);
	memcpy(prof_data, p, strlen(p)+1);

	mdla_debug("%s: \"%s\"\n", __func__, prof_data);

	if (!strcmp(prof_data, "start"))
		mdla_profile_start();
	else if (!strcmp(prof_data, "stop"))
		mdla_profile_stop(1);
	else if (!strcmp(prof_data, "try_stop"))
		mdla_profile_stop(0);

	*ppos = *ppos + len;

	return len;
}

ssize_t prof_read(struct file *filp,
	char __user *buff, size_t len, loff_t *ppos)
{
	int read, pos;

	read = strlen(prof_data);
	pos = *ppos;

	if (pos >= read)
		return 0;

	read = read - pos;
	copy_to_user(buff, &prof_data[pos], read);
	*ppos = *ppos + read;

	mdla_debug("%s: \"%s\"\n", __func__, prof_data);

	return read;
}

static const struct file_operations prof_fops = {
	.owner		= THIS_MODULE,
	.read		= prof_read,
	.write		= prof_write,
};

int mdla_procfs_init(void)
{
	mdla_proc_root = proc_mkdir(proc_dirname, NULL);

	if (IS_ERR_OR_NULL(mdla_proc_root)) {
		mdla_proc_root = NULL;
		return -ENOENT;
	}

	mdla_pprof = proc_create("prof", 0600, mdla_proc_root, &prof_fops);
	prof_data[0] = 0;

	return 0;
}

void mdla_procfs_exit(void)
{
	if (!mdla_proc_root)
		return;

	remove_proc_entry("prof", mdla_proc_root);
	remove_proc_entry(proc_dirname, NULL);
	mdla_proc_root = NULL;
}

