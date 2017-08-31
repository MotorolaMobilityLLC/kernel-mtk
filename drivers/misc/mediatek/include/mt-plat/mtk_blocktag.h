/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef _MTK_BLOCKTAG_H
#define _MTK_BLOCKTAG_H

#include <linux/types.h>
#include <linux/sched.h>

#if defined(CONFIG_MTK_BLOCK_TAG)

#define MTK_BLOCK_TAG_PIDLOG_ENTRIES 50

struct page_pid_logger {
	unsigned short pid1;
	unsigned short pid2;
};

#ifdef CONFIG_MTK_EXTMEM
extern void *extmem_malloc_page_align(size_t bytes);
#endif

struct mtk_btag_workload {
	__u64 period;  /* period time (ns) */
	__u64 usage;   /* busy time (ns) */
	__u32 percent; /* workload */
	__u32 count;   /* access count */
};

struct mtk_btag_throughput_rw {
	__u64 usage;  /* busy time (ns) */
	__u32 size;   /* transferred bytes */
	__u32 speed;  /* KB/s */
};

struct mtk_btag_throughput {
	struct mtk_btag_throughput_rw r;  /* read */
	struct mtk_btag_throughput_rw w;  /* write */
};

struct mtk_btag_vmstat {
	__u64 file_pages;
	__u64 file_dirty;
	__u64 dirtied;
	__u64 writeback;
	__u64 written;
};

struct mtk_btag_pidlogger_entry_rw {
	__u16 count;
	__u32 length;
};

struct mtk_btag_pidlogger_entry {
	__u16 pid;
	struct mtk_btag_pidlogger_entry_rw r; /* read */
	struct mtk_btag_pidlogger_entry_rw w; /* write */
};

struct mtk_btag_pidlogger {
	__u16 current_pid;
	struct mtk_btag_pidlogger_entry info[MTK_BLOCK_TAG_PIDLOG_ENTRIES];
};

struct mtk_btag_cpu {
	__u64 user;
	__u64 nice;
	__u64 system;
	__u64 idle;
	__u64 iowait;
	__u64 irq;
	__u64 softirq;
};

/* Trace: entry of the ring buffer */
struct mtk_btag_trace {
	uint64_t time;
	pid_t pid;
	u32 qid;
	struct mtk_btag_workload workload;
	struct mtk_btag_throughput throughput;
	struct mtk_btag_vmstat vmstat;
	struct mtk_btag_pidlogger pidlog;
	struct mtk_btag_cpu cpu;
};

/* Ring Trace */
struct mtk_btag_ringtrace {
	struct mtk_btag_trace *trace;
	spinlock_t lock;
	int index;
	int max;
};

struct mtk_btag_ringtrace *mtk_btag_ringtrace_alloc(int size);
void mtk_btag_ringtrace_free(struct mtk_btag_ringtrace *rt);
struct mtk_btag_trace *mtk_btag_curr_trace(struct mtk_btag_ringtrace *rt);
struct mtk_btag_trace *mtk_btag_next_trace(struct mtk_btag_ringtrace *rt);

unsigned int mtk_btag_used_mem_get(void);
int mtk_btag_pidlog_add_mmc(pid_t pid, __u32 len, int rw);
int mtk_btag_pidlog_add_ufs(pid_t pid, __u32 len, int rw);
void mtk_btag_pidlog_insert(struct mtk_btag_pidlogger *pidlog, pid_t pid, __u32 len, int rw);

void mtk_btag_cpu_eval(struct mtk_btag_cpu *cpu);
void mtk_btag_pidlog_eval(struct mtk_btag_pidlogger *pl, struct mtk_btag_pidlogger *ctx_pl);
void mtk_btag_throughput_eval(struct mtk_btag_throughput *tp);
void mtk_btag_vmstat_eval(struct mtk_btag_vmstat *vm);

const char *mtk_btag_pr_time(char *out, int size, const char *str, __u64 t);
const char *mtk_btag_pr_speed(char *out, int size, __u64 usage, __u32 bytes);
void mtk_btag_print_klog(char **ptr, int *len, struct mtk_btag_trace *tr);

void mtk_btag_seq_trace(struct seq_file *seq, struct mtk_btag_trace *tr);
void mtk_btag_seq_time(struct seq_file *seq, uint64_t time);
size_t mtk_btag_seq_usedmem(struct seq_file *seq, struct mtk_btag_ringtrace *rt);
void mtk_btag_seq_debug_show_ringtrace(struct seq_file *seq,
	struct mtk_btag_ringtrace *rt);

ssize_t mtk_btag_debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos);
void *mtk_btag_seq_debug_start(struct seq_file *seq, loff_t *pos);
void *mtk_btag_seq_debug_next(struct seq_file *seq, void *v, loff_t *pos);
void mtk_btag_seq_debug_stop(struct seq_file *seq, void *v);

#endif

#endif

