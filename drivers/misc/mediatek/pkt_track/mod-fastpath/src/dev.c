/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/version.h>
#include <linux/sysctl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include "tuple.h"
#include "dev.h"
#include "fastpath.h"
#include "fastpath_debug.h"

/*------------------------------------------------------------------------*/
/* MD Direct Tethering only supports some specified network devices,      */
/* which are defined below                                                */
/*------------------------------------------------------------------------*/
const char *fastpath_support_dev_names[] = {
	"ccmni-lan",
	"ccmni0",
	"ccmni1",
	"ccmni2",
	"ccmni3",
	"ccmni4",
	"ccmni5",
	"ccmni6",
	"ccmni7",
	"rndis0"
};

const char *fastpath_data_usage_support_dev_names[] = {
	"ccmni0",
	"ccmni1",
	"ccmni2",
	"ccmni3",
	"ccmni4",
	"ccmni5",
	"ccmni6",
	"ccmni7"
};

const int fastpath_support_dev_num =
	sizeof(fastpath_support_dev_names) /
	sizeof(fastpath_support_dev_names[0]);
const int fastpath_data_usage_support_dev_num =
	sizeof(fastpath_data_usage_support_dev_names) /
	sizeof(fastpath_data_usage_support_dev_names[0]);
const int fastpath_max_lan_dev_id;

bool fastpath_is_support_dev(const char *dev_name)
{
	int i;

	for (i = 0; i < fastpath_support_dev_num; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return true;
		}
	}

	return false;
}

int fastpath_dev_name_to_id(char *dev_name)
{
	int i;

	for (i = 0; i < fastpath_support_dev_num; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return i;
		}
	}

	return -1;
}

const char *fastpath_id_to_dev_name(int id)
{
	if (id < 0 || id >= fastpath_support_dev_num) {
		fp_printk(K_ALET, "%s: Invalid ID[%d].\n", __func__, id);
		WARN_ON(1);
		return NULL;
	}

	return fastpath_support_dev_names[id];
}

const char *fastpath_data_usage_id_to_dev_name(int id)
{
	if (id < 0 || id >= fastpath_data_usage_support_dev_num) {
		fp_printk(K_ALET, "%s: Invalid ID[%d].\n", __func__, id);
		WARN_ON(1);
		return NULL;
	}

	return fastpath_data_usage_support_dev_names[id];
}

bool fastpath_is_lan_dev(char *dev_name)
{
	int i;

	for (i = 0; i <= fastpath_max_lan_dev_id; i++) {
		if (strcmp(fastpath_support_dev_names[i], dev_name) == 0) {
			/* Matched! */
			return true;
		}
	}

	return false;
}
