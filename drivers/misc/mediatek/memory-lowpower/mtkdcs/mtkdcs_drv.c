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

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <mt-plat/mtk_meminfo.h>

/* Memory lowpower private header file */
#include "../internal.h"

static enum dcs_status sys_dcs_status;
static int dcs_initialized;
static int normal_channel_num;
static int lowpower_channel_num;
static DEFINE_SPINLOCK(dcs_lock);

/* dummy IPI APIs */
enum dcs_status dummy_ipi_read_dcs_status(void)
{
	return DCS_NORMAL;
}
int dummy_dram_channel_num(void)
{
	return 2;
}
int dummy_ipi_do_dcs(enum dcs_status status)
{
	return 0;
}

/*
 * dcs_dram_channel_switch
 *
 * Send a IPI call to SSPM to perform dynamic channel switch.
 * The dynamic channel switch only performed in stable status.
 * i.e., DCS_NORMAL or DCS_LOWPOWER.
 * @status: channel mode
 *
 * return 0 on success or error code
 */
int dcs_dram_channel_switch(enum dcs_status status)
{
	if (!spin_trylock(&dcs_lock))
		return -EBUSY;

	if ((sys_dcs_status < DCS_BUSY) &&
		(status < DCS_BUSY) &&
		(sys_dcs_status != DCS_BUSY)) {
		dummy_ipi_do_dcs(status);
		sys_dcs_status = DCS_BUSY;
	}
	spin_unlock(&dcs_lock);
	/*
	 * assume we success doing channel switch, the sys_dcs_status
	 * should be updated in the ISR
	 */
	spin_lock(&dcs_lock);
	sys_dcs_status = status;
	spin_unlock(&dcs_lock);
	return 0;
}

/*
 * dcs_get_channel_num_trylock
 * return the number of DRAM channels and get the dcs lock
 * @num: address storing the number of DRAM channels.
 *
 * return 0 on success or error code
 */
int dcs_get_channel_num_trylock(int *num)
{
	if (!spin_trylock(&dcs_lock))
		goto BUSY;

	switch (sys_dcs_status) {
	case DCS_NORMAL:
		*num = normal_channel_num;
		break;
	case DCS_LOWPOWER:
		*num = lowpower_channel_num;
		break;
	default:
		goto BUSY;
	}
	return 0;
BUSY:
	*num = -1;
	return -EBUSY;
}

/*
 * dcs_get_channel_num_unlock
 * unlock the dcs lock
 */
void dcs_get_channel_num_unlock(void)
{
	spin_unlock(&dcs_lock);
}

/*
 * dcs_initialied
 *
 * return true if dcs is initialized
 */
bool dcs_initialied(void)
{
	return dcs_initialized;
}

static int __init mtkdcs_init(void)
{
	/* read system dcs status */
	sys_dcs_status = dummy_ipi_read_dcs_status();
	/* read number of dram channels */
	normal_channel_num = dummy_dram_channel_num();
	lowpower_channel_num = (normal_channel_num / 2) ?
		(normal_channel_num / 2) : normal_channel_num;

	dcs_initialized = true;

	return 0;
}

static void __exit mtkdcs_exit(void)
{
}

device_initcall(mtkdcs_init);
module_exit(mtkdcs_exit);
