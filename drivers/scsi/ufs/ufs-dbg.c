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

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mt-plat/mtk_gpt.h>
#include <linux/io.h>
#include <linux/scatterlist.h>

#include "ufs-dbg.h"
#include "ufs.h"

#ifndef FPGA_PLATFORM
#include <mt-plat/upmu_common.h>
#endif

enum field_width {
	BYTE = 1,
	WORD = 2,
};

struct desc_field_offset {
	char *name;
	int offset;
	enum field_width width_byte;
};

#ifdef CONFIG_MTK_UFS_DEBUG
#define MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT (500)
/* max dump size is 40KB whitch can be adjusted */
#define UFS_AEE_BUFFER_SIZE (40 * 1024)

struct ufs_mtk_trace_cmd_hlist_struct ufs_mtk_trace_cmd_hlist[MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT];
int ufs_mtk_trace_cmd_ptr = MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT - 1;
int ufs_mtk_trace_cnt;
static spinlock_t ufs_mtk_cmd_dump_lock;
static int ufs_mtk_is_cmd_dump_lock_init;
char ufs_aee_buffer[UFS_AEE_BUFFER_SIZE];


void ufs_mtk_dbg_add_trace(enum ufs_trace_event event, u32 tag,
	u8 lun, u32 transfer_len, sector_t lba, u8 opcode)
{
	int ptr;
	unsigned long flags;

	if (!ufs_mtk_is_cmd_dump_lock_init) {
		spin_lock_init(&ufs_mtk_cmd_dump_lock);
		ufs_mtk_is_cmd_dump_lock_init = 1;
	}

	spin_lock_irqsave(&ufs_mtk_cmd_dump_lock, flags);

	ufs_mtk_trace_cmd_ptr++;

	if (ufs_mtk_trace_cmd_ptr >= MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT)
		ufs_mtk_trace_cmd_ptr = 0;

	ptr = ufs_mtk_trace_cmd_ptr;

	ufs_mtk_trace_cmd_hlist[ptr].event = event;
	ufs_mtk_trace_cmd_hlist[ptr].tag = tag;
	ufs_mtk_trace_cmd_hlist[ptr].transfer_len = transfer_len;
	ufs_mtk_trace_cmd_hlist[ptr].lun = lun;
	ufs_mtk_trace_cmd_hlist[ptr].lba = lba;
	ufs_mtk_trace_cmd_hlist[ptr].opcode = opcode;
	ufs_mtk_trace_cmd_hlist[ptr].time = sched_clock();

	ufs_mtk_trace_cnt++;

	spin_unlock_irqrestore(&ufs_mtk_cmd_dump_lock, flags);
	/*
	 * pr_info("[ufs]%d,%d,op=0x%x,lun=%u,t=%d,lba=%llu,len=%u,time=%llu\n",
	 * ptr, event, opcode, lun, tag, (u64)(lba >> 3), transfer_len, sched_clock());
	 */
}

void ufs_mtk_dbg_dump_trace(char **buff, unsigned long *size, u32 latest_cnt, struct seq_file *m)
{
	int ptr;
	int dump_cnt;
	unsigned long flags;

	if (!ufs_mtk_is_cmd_dump_lock_init) {
		spin_lock_init(&ufs_mtk_cmd_dump_lock);
		ufs_mtk_is_cmd_dump_lock_init = 1;
	}

	spin_lock_irqsave(&ufs_mtk_cmd_dump_lock, flags);

	if (ufs_mtk_trace_cnt > MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT)
		dump_cnt = MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT;
	else
		dump_cnt = ufs_mtk_trace_cnt;

	if (latest_cnt)
		dump_cnt = min_t(u32, latest_cnt, dump_cnt);

	ptr = ufs_mtk_trace_cmd_ptr;

	SPREAD_PRINTF(buff, size, m, "[ufs]cmd history:req_cnt=%d,real_cnt=%d,ptr=%d\n",
		latest_cnt, dump_cnt, ptr);

	while (dump_cnt > 0) {

		if (ufs_mtk_trace_cmd_hlist[ptr].event == UFS_TRACE_TM_SEND ||
			ufs_mtk_trace_cmd_hlist[ptr].event == UFS_TRACE_TM_COMPLETED) {

			SPREAD_PRINTF(buff, size, m, "[ufs]%d-t,%d,tm=0x%x,t=%d,lun=0x%x,data=0x%x,%llu\n",
				ptr,
				ufs_mtk_trace_cmd_hlist[ptr].event,
				ufs_mtk_trace_cmd_hlist[ptr].opcode,
				ufs_mtk_trace_cmd_hlist[ptr].tag,
				ufs_mtk_trace_cmd_hlist[ptr].lun,
				ufs_mtk_trace_cmd_hlist[ptr].transfer_len,
				(u64)ufs_mtk_trace_cmd_hlist[ptr].time
				);

		} else if (ufs_mtk_trace_cmd_hlist[ptr].event == UFS_TRACE_DEV_SEND ||
			ufs_mtk_trace_cmd_hlist[ptr].event == UFS_TRACE_DEV_COMPLETED) {

			SPREAD_PRINTF(buff, size, m,
				"[ufs]%d-d,%d,0x%x,t=%d,lun=0x%x,idn=0x%x,idx=0x%x,sel=0x%x,%llu\n",
				ptr,
				ufs_mtk_trace_cmd_hlist[ptr].event,
				ufs_mtk_trace_cmd_hlist[ptr].opcode,
				ufs_mtk_trace_cmd_hlist[ptr].tag,
				ufs_mtk_trace_cmd_hlist[ptr].lun,
				(u8)ufs_mtk_trace_cmd_hlist[ptr].lba & 0xFF,
				(u8)(ufs_mtk_trace_cmd_hlist[ptr].lba >> 8) & 0xFF,
				(u8)(ufs_mtk_trace_cmd_hlist[ptr].lba >> 16) & 0xFF,
				(u64)ufs_mtk_trace_cmd_hlist[ptr].time
				);

		} else {

			SPREAD_PRINTF(buff, size, m, "[ufs]%d-r,%d,0x%x,t=%d,lun=0x%x,lba=%lld,len=%d,%llu\n",
				ptr,
				ufs_mtk_trace_cmd_hlist[ptr].event,
				ufs_mtk_trace_cmd_hlist[ptr].opcode,
				ufs_mtk_trace_cmd_hlist[ptr].tag,
				ufs_mtk_trace_cmd_hlist[ptr].lun,
				(long long int)ufs_mtk_trace_cmd_hlist[ptr].lba,
				ufs_mtk_trace_cmd_hlist[ptr].transfer_len,
				(u64)ufs_mtk_trace_cmd_hlist[ptr].time
				);
		}

		dump_cnt--;

		ptr--;

		if (ptr < 0)
			ptr = MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT - 1;
	}

	spin_unlock_irqrestore(&ufs_mtk_cmd_dump_lock, flags);
}

void ufs_mtk_dbg_hang_detect_dump(void)
{
	/*
	 * do not touch host to get unipro or mphy information via
	 * dme commands during exception handling since interrupt
	 * or preemption may be disabled.
	 */
	ufshcd_print_host_state(ufs_mtk_hba, 0, NULL, NULL, NULL);

	ufs_mtk_dbg_dump_trace(NULL, NULL, ufs_mtk_hba->nutrs + ufs_mtk_hba->nutrs / 2, NULL);
}

static void ufs_mtk_dbg_dump_feature(struct ufs_hba *hba, struct seq_file *m)
{
	UFS_DEVINFO_PROC_MSG(m, hba->dev, "-- Crypto Features ----\n");
	UFS_DEVINFO_PROC_MSG(m, hba->dev, "Features          = 0x%x\n",
		hba->crypto_feature);
	UFS_DEVINFO_PROC_MSG(m, hba->dev, " HW-FDE           = 0x%x\n",
		UFS_CRYPTO_HW_FDE);
	UFS_DEVINFO_PROC_MSG(m, hba->dev, " HW-FDE Encrypted = 0x%x\n",
		UFS_CRYPTO_HW_FDE_ENCRYPTED);
	UFS_DEVINFO_PROC_MSG(m, hba->dev, " HW-FBE           = 0x%x\n",
		UFS_CRYPTO_HW_FBE);
	UFS_DEVINFO_PROC_MSG(m, hba->dev, " HW-FBE Encrypted = 0x%x\n",
		UFS_CRYPTO_HW_FBE_ENCRYPTED);
	UFS_DEVINFO_PROC_MSG(m, hba->dev, "-----------------------\n");
}

void ufs_mtk_dbg_proc_dump(struct seq_file *m)
{
	ufs_mtk_dbg_dump_feature(ufs_mtk_hba, m);

	/*
	 * do not touch host to get unipro or mphy information via
	 * dme commands during exception handling since interrupt
	 * or preemption may be disabled.
	 */
	ufshcd_print_host_state(ufs_mtk_hba, 0, m, NULL, NULL);

	ufs_mtk_dbg_dump_trace(NULL, NULL, MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT, m);
}

void get_ufs_aee_buffer(unsigned long *vaddr, unsigned long *size)
{
	unsigned long free_size = UFS_AEE_BUFFER_SIZE;
	char *buff;

	if (ufs_mtk_hba == NULL) {
		pr_info("====== Null ufs_mtk_hba, dump skipped ======\n");
		return;
	}

	buff = ufs_aee_buffer;
	ufshcd_print_host_state(ufs_mtk_hba, 0, NULL, &buff, &free_size);
	ufs_mtk_dbg_dump_trace(&buff, &free_size, MAX_UFS_MTK_TRACE_CMD_HLIST_ENTRY_CNT, NULL);

	/* retrun start location */
	*vaddr = (unsigned long)ufs_aee_buffer;
	*size = UFS_AEE_BUFFER_SIZE - free_size;
}

static int ufsdbg_dump_health_desc(struct seq_file *file)
{
	int err = 0;
	int buff_len = QUERY_DESC_HEALTH_MAX_SIZE;
	u8 desc_buf[QUERY_DESC_HEALTH_MAX_SIZE];

	struct desc_field_offset health_desc_field_name[] = {
		{"bLength",             0x00, BYTE},
		{"bDescriptorIDN",      0x01, BYTE},
		{"bPreEOLInfo",         0x02, BYTE},
		{"bDeviceLifeTimeEstA", 0x03, BYTE},
		{"bDeviceLifeTimeEstB", 0x04, BYTE},
	};

	pm_runtime_get_sync(ufs_mtk_hba->dev);
	err = ufshcd_read_desc(ufs_mtk_hba, QUERY_DESC_IDN_HEALTH, 0, desc_buf, buff_len);
	pm_runtime_put_sync(ufs_mtk_hba->dev);

	if (!err) {
		int i;
		struct desc_field_offset *tmp;

		for (i = 0; i < QUERY_DESC_HEALTH_MAX_SIZE; i++) {
			seq_printf(file,
				"Health Descriptor[0x%x] = 0x%x\n",
				i,
				(u8)desc_buf[i]);
		}
		for (i = 0; i < ARRAY_SIZE(health_desc_field_name); i++) {
			tmp = &health_desc_field_name[i];

			if (tmp->width_byte == BYTE) {
				seq_printf(file,
					"Health Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
					tmp->offset,
					tmp->name,
					(u8)desc_buf[tmp->offset]);
			} else if (tmp->width_byte == WORD) {
				seq_printf(file,
					"Health Descriptor[Word offset 0x%x]: %s = 0x%x\n",
					tmp->offset,
					tmp->name,
					*(u16 *)&desc_buf[tmp->offset]);
			} else {
				seq_printf(file,
					"Health Descriptor[offset 0x%x]: %s. Wrong Width = %d",
					tmp->offset, tmp->name, tmp->width_byte);
			}
		}
	} else {
		seq_printf(file, "Reading Health Descriptor failed. err = %d\n",
			err);
	}
	return err;
}

#else
void ufs_mtk_dbg_add_trace(enum ufs_trace_event event, u32 tag,
	u8 lun, u32 transfer_len, sector_t lba, u8 opcode)
{
}
void ufs_mtk_dbg_dump_trace(char **buff, unsigned long *size, u32 latest_cnt, struct seq_file *m)
{
}
void ufs_mtk_dbg_proc_dump(struct seq_file *m)
{
}
void ufs_mtk_dbg_hang_detect_dump(void)
{
}
void get_ufs_aee_buffer(unsigned long *vaddr, unsigned long *size)
{
}
static int ufsdbg_dump_health_desc(struct seq_file *file)
{
}
#endif
EXPORT_SYMBOL(get_ufs_aee_buffer);

static char cmd_buf[256];

static int ufs_help_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "\n===============[ufs_help]================\n");

	seq_printf(m, "\n   Commands dump:        echo %x [host_id] > ufs_debug\n",
		UFS_CMDS_DUMP);

	seq_printf(m, "\n   Get Power Mode Status:        echo %x [host_id] > ufs_debug\n",
		UFS_GET_PWR_MODE);

	seq_printf(m, "\n   Dump Health Descriptors:        echo %x [host_id] > ufs_debug\n",
		UFS_DUMP_HEALTH_DESCRIPTOR);

	seq_puts(m, "\n   NOTE: All input data is Hex number!\n");

	seq_puts(m, "\n=============================================\n\n");

	return 0;
}

int ufs_g_count;
/* ========== driver proc interface =========== */
static int ufs_debug_proc_show(struct seq_file *m, void *v)
{
	int cmd = -1;
	int sscanf_num;
	int p1, p2, p3, p4, p5, p6, p7, p8;

	p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = -1;

	cmd_buf[ufs_g_count] = '\0';
	seq_printf(m, "Debug Command:  %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%x %x %x %x %x %x %x %x %x", &cmd,
		&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);

	ufs_g_count = 0;
	cmd_buf[ufs_g_count] = '\0';

	if (cmd == UFS_CMDS_DUMP) {
		ufs_mtk_dbg_proc_dump(m);
	} else if (cmd == UFS_GET_PWR_MODE) {
		seq_printf(m, "Power Mode: tx 0x%x rx 0x%x (1:FAST 2:SLOW 4:FAST_AUTO 5:SLOW_AUTO 7:UNCHANGE)\n"
			, ufs_mtk_hba->pwr_info.pwr_tx, ufs_mtk_hba->pwr_info.pwr_rx);
		seq_printf(m, "Gear: tx 0x%x rx 0x%x\n", ufs_mtk_hba->pwr_info.gear_tx
			, ufs_mtk_hba->pwr_info.gear_rx);
		seq_printf(m, "HS Rate: 0x%x (1:HS_A 2:HS_B)\n", ufs_mtk_hba->pwr_info.hs_rate);
		seq_printf(m, "Lanes: tx 0x%x rx 0x%x\n", ufs_mtk_hba->pwr_info.lane_tx
			, ufs_mtk_hba->pwr_info.lane_rx);
	} else if (cmd == UFS_DUMP_HEALTH_DESCRIPTOR) {
		ufsdbg_dump_health_desc(m);
	} else {
		/* Default print cmd history for aee: JE/NE/ANR/EE/SWT/system api dump */
		seq_puts(m, "==== ufs debug info for aee ====\n");
		ufs_mtk_dbg_proc_dump(m);
	}

	return 0;
}

static ssize_t ufs_debug_proc_write(struct file *file, const char *buf,
	size_t count, loff_t *data)
{
	int ret;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;
	ufs_g_count = count;
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;
	return count;
}

static int ufs_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufs_debug_proc_show, inode->i_private);
}

static const struct file_operations ufs_proc_fops = {
	.open = ufs_proc_open,
	.write = ufs_debug_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ufs_help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufs_help_proc_show, inode->i_private);
}
static const struct file_operations ufs_help_fops = {
	.open = ufs_help_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifndef USER_BUILD_KERNEL
#define PROC_PERM		0660
#else
#define PROC_PERM		0440
#endif

int ufs_mtk_debug_proc_init(void)
{
	struct proc_dir_entry *prEntry;
	kuid_t uid;
	kgid_t gid;

	uid = make_kuid(&init_user_ns, 0);
	gid = make_kgid(&init_user_ns, 1001);

	prEntry = proc_create("ufs_debug", PROC_PERM, NULL, &ufs_proc_fops);

	if (prEntry)
		proc_set_user(prEntry, uid, gid);
	else
		pr_info("[%s]: failed to create /proc/ufs_debug\n", __func__);

	prEntry = proc_create("ufs_help", PROC_PERM, NULL, &ufs_help_fops);

	if (!prEntry)
		pr_info("[%s]: failed to create /proc/ufs_help\n", __func__);

	return 0;
}
EXPORT_SYMBOL_GPL(ufs_mtk_debug_proc_init);
