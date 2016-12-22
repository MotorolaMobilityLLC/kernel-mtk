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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/printk.h>
#include <linux/mm_types.h>
#include <linux/seq_file.h>
#include <mt-plat/mt_chip.h>

#include <core.h>
#include "mt_sd.h"
#include "dbg.h"


#include <linux/proc_fs.h>
#include <queue.h>

#include <mt-plat/partition.h>

struct mmc_blk_data {
	spinlock_t lock;
	struct gendisk *disk;
	struct mmc_queue queue;

	unsigned int usage;
	unsigned int read_only;
};

u64 msdc_get_user_capacity(struct msdc_host *host)
{
	u64 capacity = 0;
	u32 legacy_capacity = 0;
	struct mmc_card *card;

	if ((host != NULL) && (host->mmc != NULL) && (host->mmc->card != NULL))
		card = host->mmc->card;
	else
		return 0;

	card = host->mmc->card;
	if (mmc_card_mmc(card)) {
		if (card->csd.read_blkbits) {
			legacy_capacity =
				(2 << (card->csd.read_blkbits - 1))
				* card->csd.capacity;
		} else {
			legacy_capacity = card->csd.capacity;
			ERR_MSG("XXX read_blkbits = 0 XXX");
		}
		capacity =
			(u64)(card->ext_csd.sectors) * 512 > legacy_capacity ?
			(u64)(card->ext_csd.sectors) * 512 : legacy_capacity;
	} else if (mmc_card_sd(card)) {
		capacity = (u64) (card->csd.capacity)
			<< (card->csd.read_blkbits);
	}
	return capacity;
}


int offset = 0;

#ifdef MTK_MSDC_USE_CACHE
unsigned long long g_cache_part_start;
unsigned long long g_cache_part_end;
unsigned long long g_usrdata_part_start;
unsigned long long g_usrdata_part_end;

int msdc_can_apply_cache(unsigned long long start_addr,
	unsigned int size)
{
	/* since cache, userdata partition are connected,
	   so check it as an area, else do check them separately */
	if (!g_cache_part_start && !g_cache_part_end &&
		!g_usrdata_part_start && !g_usrdata_part_end)
		return 0;
	if (g_cache_part_end == g_usrdata_part_start) {
		if ((start_addr < g_cache_part_start) ||
		    (start_addr + size >= g_usrdata_part_end)) {
			return 0;
		}
	} else {
		if (((start_addr < g_cache_part_start) ||
		     (start_addr + size >= g_cache_part_end))
		 && ((start_addr < g_usrdata_part_start) ||
		     (start_addr + size >= g_usrdata_part_end))) {
			return 0;
		}
	}

	return 1;
}

void msdc_get_cache_region(struct work_struct *work)
{
	struct hd_struct *lp_hd_struct = NULL;

	lp_hd_struct = get_part("cache");
	if (likely(lp_hd_struct)) {
		g_cache_part_start = lp_hd_struct->start_sect;
		g_cache_part_end = g_cache_part_start + lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
	} else {
		g_cache_part_start = (sector_t)(-1);
		g_cache_part_end = (sector_t)(-1);
		pr_err("There is no cache info\n");
	}

	lp_hd_struct = NULL;
	lp_hd_struct = get_part("userdata");
	if (likely(lp_hd_struct)) {
		g_usrdata_part_start = lp_hd_struct->start_sect;
		g_usrdata_part_end = g_usrdata_part_start
			+ lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
	} else {
		g_usrdata_part_start = (sector_t)(-1);
		g_usrdata_part_end = (sector_t)(-1);
		pr_err("There is no userdata info\n");
	}

	pr_err("cache(0x%llX~0x%llX, usrdata(0x%llX~0x%llX)\n",
		g_cache_part_start, g_cache_part_end,
		g_usrdata_part_start, g_usrdata_part_end);
}
EXPORT_SYMBOL(msdc_get_cache_region);
static struct delayed_work get_cache_info;
static int __init init_get_cache_work(void)
{
	INIT_DELAYED_WORK(&get_cache_info, msdc_get_cache_region);
	schedule_delayed_work(&get_cache_info, 100);
	return 0;
}
#endif
#ifdef CONFIG_MTK_EMMC_SUPPORT_OTP
unsigned long long otp_part_start;
unsigned long long otp_part_end;

void msdc_get_otp_region(struct work_struct *work)
{
	struct hd_struct *lp_hd_struct = NULL;

	lp_hd_struct = get_part("otp");
	if (likely(lp_hd_struct)) {
		otp_part_start = lp_hd_struct->start_sect;
		otp_part_end = otp_part_start + lp_hd_struct->nr_sects;
		put_part(lp_hd_struct);
	} else {
		pr_err("There is no otp partition\n");
	}
}

int msdc_check_otp_ops(unsigned int opcode, unsigned long long start_addr,
	unsigned int size)
{
	if ((otp_part_start == 0) || (otp_part_end == 0))
		return 0;
	if ((opcode == MMC_SET_WRITE_PROT) ||
		(opcode == MMC_CLR_WRITE_PROT) ||
		(opcode == MMC_SEND_WRITE_PROT) ||
		(opcode == 31)) {
		if ((start_addr > otp_part_end) ||
				(start_addr < otp_part_start) ||
				((start_addr + size) > otp_part_end)) {
			pr_err("OTP operation invalid address %llx\n", start_addr);
			return 1;
		}
	}
	return 0;
}
static struct delayed_work get_otp_info;
static int __init init_get_otp_work(void)
{
	INIT_DELAYED_WORK(&get_otp_info, msdc_get_otp_region);
	schedule_delayed_work(&get_otp_info, 100);
	return 0;
}
#endif
u32 msdc_get_other_capacity(struct msdc_host *host, char *name)
{
	u32 device_other_capacity = 0;
	int i;
	struct mmc_card *card;

	if ((host != NULL) && (host->mmc != NULL) && (host->mmc->card != NULL))
		card = host->mmc->card;
	else
		return 0;

	for (i = 0; i < card->nr_parts; i++) {
		if (!name) {
			device_other_capacity += card->part[i].size;
		} else if (strcmp(name, card->part[i].name) == 0) {
			device_other_capacity = card->part[i].size;
			break;
		}
	}

	return device_other_capacity;
}

int msdc_get_reserve(void)
{
	u32 l_offset = 0;
	u32 l_otp_reserve = 0;
	u32 l_mtk_reserve = 0;

	return l_offset + l_otp_reserve + l_mtk_reserve;
}
EXPORT_SYMBOL(msdc_get_reserve);

u64 msdc_get_capacity(int get_emmc_total)
{
	u64 user_size = 0;
	u32 other_size = 0;
	u64 total_size = 0;
	int index = 0;
	struct msdc_host *host;

	for (index = 0; index < HOST_MAX_NUM; ++index) {
		host = mtk_msdc_host[index];
		if ((host != NULL) && (host->hw->boot)) {
			user_size = msdc_get_user_capacity(host);
#ifdef CONFIG_MTK_EMMC_SUPPORT
			if (get_emmc_total)
				other_size =
					msdc_get_other_capacity(host, NULL);
#endif
			break;
		}
	}
	total_size = user_size + (u64) other_size;
	return total_size / 512;
}
EXPORT_SYMBOL(msdc_get_capacity);

struct gendisk *mmc_get_disk(struct mmc_card *card)
{
	struct mmc_blk_data *md;
	/* struct gendisk *disk; */

	BUG_ON(!card);
	md = mmc_get_drvdata(card);
	BUG_ON(!md);
	BUG_ON(!md->disk);

	return md->disk;
}

static struct proc_dir_entry *proc_emmc;

static inline int emmc_proc_info(struct seq_file *m, struct hd_struct *this)
{
	char *no_partition_name = "n/a";

	return seq_printf(m, "emmc_p%d: %8.8x %8.8x \"%s\"\n", this->partno,
		(unsigned int)this->start_sect,
		(unsigned int)this->nr_sects,
		((this->info) ?
			(char *)(this->info->volname) : no_partition_name));
}

static int proc_emmc_show(struct seq_file *m, void *v)
{
	struct disk_part_iter piter;
	struct hd_struct *part;
	struct msdc_host *host;
	struct gendisk *disk;

	/* emmc always in slot0 */
	host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN, 0);
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);
	disk = mmc_get_disk(host->mmc->card);

	seq_puts(m, "partno:    start_sect   nr_sects  partition_name\n");
	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter)))
		emmc_proc_info(m, part);
	disk_part_iter_exit(&piter);

	return 0;
}

static int proc_emmc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_emmc_show, NULL);
}

static const struct file_operations proc_emmc_fops = {
	.open = proc_emmc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void msdc_proc_emmc_create(void)
{
	proc_emmc = proc_create("emmc", 0, NULL, &proc_emmc_fops);
}

#ifdef MTK_MSDC_USE_CACHE
late_initcall_sync(init_get_cache_work);
#endif
#ifdef CONFIG_MTK_EMMC_SUPPORT_OTP
late_initcall_sync(init_get_otp_work);
#endif
