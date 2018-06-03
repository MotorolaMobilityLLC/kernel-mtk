/* Mediatek STAR MAC network driver.
 *
 * Copyright (c) 2016-2017 MediaTek Inc.
 *
 * program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _STAR_PROCFS_H_
#define _STAR_PROCFS_H_

#include <linux/proc_fs.h>

struct star_proc_file {
	const char * const name;
	const struct file_operations *fops;
};

struct star_procfs {
	struct net_device *ndev;
	struct proc_dir_entry *root;
	struct proc_dir_entry **entry;
};

int star_init_procfs(void);
void star_exit_procfs(void);

#endif /* _STAR_PROCFS_H_ */

