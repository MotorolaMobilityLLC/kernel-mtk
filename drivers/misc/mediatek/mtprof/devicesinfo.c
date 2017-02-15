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

#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/pid.h>

#include <linux/irq.h>
#include <linux/interrupt.h>

#include <linux/stacktrace.h>

#include <linux/printk.h>
#include "internal.h"
#include "devicesinfo.h"

/* //////////////////////////////////////////////////////// */
/* --------------------------------------------------- */
/* Real work */
/* --------------------------------------------------- */
/*                     Define Proc entry               */
/* --------------------------------------------------- */
MT_DEBUG_ENTRY(device_info);

#define SANDISK_MID	0x45
#define SAMSUNG_MID	0x15

static int mdl_num = 0xff;


/*dram_info order must be the same with part number order in custom_memory.h*/
static struct dram_info dram_info[] = {
	{"Samsung", "K3QF6F60AM", "3G", "LPDDR3"},
	{"Samsung", "K3QF4F40BM", "4G", "LPDDR3"},
	{"Micron", "EDFP164A3PD", "3G", "LPDDR3"},
	{"Micron", "MT52L512M64D4GN", "4G", "LPDDR3"},
};

static struct emmc_info emmc_info[] = {
	{"Samsung", "KLMBG2JETD-B041", "32G", "eMMC5.0",
		SAMSUNG_MID, 0x42},
	{"Sandisk", "SDINADF4-32G-893", "32G", "eMMC5.0",
		SANDISK_MID, 0x46343033},
	{"Samsung", "KLMCG4JETD-B041", "64G", "eMMC5.0",
		SAMSUNG_MID, 0x43},
	{"Sandisk", "SDINADF4-64G-893", "64G", "eMMC5.0",
		SANDISK_MID, 0x46343036},
};

static int mt_device_info_show(struct seq_file *m, void *v)
{
	int emmc_fg = 0xff;
	int emmc_mid = 0;

	if (mdl_num > ARRAY_SIZE(dram_info)) {
		SEQ_printf(m, "[devinfo][Error]: mdl_num %d\n", mdl_num);
		return 0;
	}

	/*Display Dram info*/
	SEQ_printf(m, "Dram info:\n");
	SEQ_printf(m, "    Type: %s\n", dram_info[mdl_num].type);
	SEQ_printf(m, "    Vendor: %s\n", dram_info[mdl_num].vendor);
	SEQ_printf(m, "    Part Number: %s\n", dram_info[mdl_num].flash_name);
	SEQ_printf(m, "    Size: %s\n", dram_info[mdl_num].size);

	emmc_mid = emmc_raw_cid[0] >> 24;
/*
	SEQ_printf(m, "\n");
	SEQ_printf(m, "emmc_raw_cid[0]: 0x%x\n", emmc_raw_cid[0]);
	SEQ_printf(m, "emmc_raw_cid[1]: 0x%x\n", emmc_raw_cid[1]);
	SEQ_printf(m, "emmc_mid: 0x%x\n", emmc_mid);
*/

	if (emmc_mid != 0) {
		switch (emmc_mid) {
		case SANDISK_MID:
			if (emmc_raw_cid[1] == emmc_info[1].pnm)
				emmc_fg = 1;/*32G SDINADF4-32G-893*/
			else if (emmc_raw_cid[1] == emmc_info[3].pnm)
				emmc_fg = 3;/*64G SDINADF4-64G-893*/
			break;
		case SAMSUNG_MID:
			if ((emmc_raw_cid[0]&0xff) == emmc_info[0].pnm)
				emmc_fg = 0;/*32G KLMBG2JETD-B041*/
			else if ((emmc_raw_cid[0]&0xff) == emmc_info[2].pnm)
				emmc_fg = 2;/*64G KLMCG4JETD-B041*/
			break;
		default:
			emmc_fg = 0xff;
			break;
		}

		if (emmc_fg > ARRAY_SIZE(emmc_info)) {
			SEQ_printf(m, "[devinfo][Error]: emmc_fg: %d\n",
				emmc_fg);
			return 0;
		}

		/*Display eMMC info*/
		SEQ_printf(m, "\n");
		SEQ_printf(m, "eMMC info:\n");
		SEQ_printf(m, "    Type: %s\n", emmc_info[emmc_fg].type);
		SEQ_printf(m, "    Vendor: %s\n", emmc_info[emmc_fg].vendor);
		SEQ_printf(m, "    Part Number: %s\n",
			 emmc_info[emmc_fg].flash_name);
		SEQ_printf(m, "    Size: %s\n", emmc_info[emmc_fg].size);
	}

	return 0;
}

static ssize_t mt_device_info_write(struct file *filp,
	const char *ubuf, size_t cnt, loff_t *data)
{
	return cnt;
}

static int __init init_mt_device_info(void)
{
	char *ptr;
	struct proc_dir_entry *pe;

	pe = proc_create("devicesinfo", 0664, NULL, &mt_device_info_fops);
	if (!pe)
		return -ENOMEM;

	ptr = strstr(saved_command_line, "mdl_num=");
	ptr += strlen("mdl_num=");
	mdl_num = simple_strtol(ptr, NULL, 10);

	return 0;
}

device_initcall(init_mt_device_info);
