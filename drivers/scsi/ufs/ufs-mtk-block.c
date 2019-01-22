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

#define DEBUG 1
#define SECTOR_SHIFT 12
#define UFS_MTK_BIO_TRACE_LATENCY (unsigned long long)(1000000000)
#define UFS_MTK_BIO_TRACE_TIMEOUT ((UFS_BIO_TRACE_LATENCY)*10)

#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/tick.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/vmalloc.h>
#include <linux/smp.h>
#include <mt-plat/mtk_blocktag.h>
#include "ufs-mtk-block.h"

/* ring trace for debugfs */
struct mtk_btag_ringtrace *ufs_mtk_btag_ringtrace;

/* context buffer of each request queue */
/* lock order: ctx => ringbuf */
struct ufs_mtk_bio_context *ufs_mtk_bio_reqctx;

/* kernel print buffer */
static char *ufs_mtk_bio_prbuf;
spinlock_t ufs_mtk_bio_prbuf_lock;

/* debugfs dentries */
struct dentry *ufs_mtk_bio_droot;
struct dentry *ufs_mtk_bio_dklog;
struct dentry *ufs_mtk_bio_dlog;
struct dentry *ufs_mtk_bio_dmem;

unsigned int ufs_mtk_bio_klog_enable;
unsigned int ufs_mtk_bio_used_mem;

static inline uint16_t chbe16_to_u16(const char *str)
{
	uint16_t ret;

	ret = str[0];
	ret = ret << 8 | str[1];
	return ret;
}

static inline uint32_t chbe32_to_u32(const char *str)
{
	uint32_t ret;

	ret = str[0];
	ret = ret << 8 | str[1];
	ret = ret << 8 | str[2];
	ret = ret << 8 | str[3];
	return ret;
}

#define scsi_cmnd_lba(cmd)  chbe32_to_u32(&cmd->cmnd[2])
#define scsi_cmnd_len(cmd)  chbe16_to_u16(&cmd->cmnd[7])
#define scsi_cmnd_cmd(cmd)  (cmd->cmnd[0])

static struct ufs_mtk_bio_context_task *ufs_mtk_bio_get_task(struct ufs_mtk_bio_context *ctx, unsigned int task_id)
{
	struct ufs_mtk_bio_context_task *tsk = NULL;
	unsigned long flags;
	int i, avail = -1;

	if (!ctx)
		return NULL;

	spin_lock_irqsave(&ctx->lock, flags);

	for (i = 0; i < UFS_BIOLOG_CONTEXT_TASKS; i++) {
		tsk = &ctx->task[i];
		if (tsk->task_id == task_id)
			goto out;
		if ((tsk->task_id < 0) && (avail < 0))
			avail = i;
	}

	if (avail >= 0) {
		tsk = &ctx->task[avail];
		tsk->task_id = task_id;
		tsk->pid = current->pid;
		tsk->cpu_id = smp_processor_id();
		goto out;
	}

	pr_warn("ufs_mtk_bio_get_task: out of task, incoming task id = %d\n", task_id);

	for (i = 0; i < UFS_BIOLOG_CONTEXT_TASKS; i++)
		pr_warn("ufs_mtk_bio_get_task: task[%d]=%d\n", i, ctx->task[i].task_id);

out:
	spin_unlock_irqrestore(&ctx->lock, flags);
	return tsk;
}

static struct ufs_mtk_bio_context *ufs_mtk_bio_curr_ctx(void)
{
	if (!ufs_mtk_bio_reqctx)
		return NULL;

	return &ufs_mtk_bio_reqctx[0];
}

static struct ufs_mtk_bio_context_task *ufs_mtk_bio_curr_task(unsigned int task_id,
	struct ufs_mtk_bio_context **curr_ctx)
{
	struct ufs_mtk_bio_context *ctx;

	ctx = ufs_mtk_bio_curr_ctx();
	if (curr_ctx)
		*curr_ctx = ctx;
	return ufs_mtk_bio_get_task(ctx, task_id);
}

int mtk_btag_pidlog_add_ufs(pid_t pid, __u32 len, int rw)
{
	unsigned long flags;
	struct ufs_mtk_bio_context *ctx;

	ctx = ufs_mtk_bio_curr_ctx();
	if (!ctx)
		return 0;

	spin_lock_irqsave(&ctx->lock, flags);
	mtk_btag_pidlog_insert(&ctx->pidlog, pid, len, rw);
	spin_unlock_irqrestore(&ctx->lock, flags);

	return 1;
}
EXPORT_SYMBOL_GPL(mtk_btag_pidlog_add_ufs);

static void ufs_mtk_pr_tsk(struct ufs_mtk_bio_context_task *tsk,
	int stage)
{
	const char *rw = "?";
	__u64 busy_time = 0;
	char str[7][32];

	if (!((ufs_mtk_bio_klog_enable == 2 && stage == 5) || (ufs_mtk_bio_klog_enable == 3)))
		return;

	if (tsk->cmd == 0x28)
		rw = "r";
	else if (tsk->cmd == 0x2A)
		rw = "w";

	if (stage == 5)
		busy_time = tsk->scsi_done_end_t - tsk->request_start_t;

	pr_debug("[BLOCK_TAG] ufs: tsk[%d]-(%d),%d,%s,%02X,pid=%u,len=%d%s%s%s%s%s%s%s\n",
		tsk->task_id,
		tsk->cpu_id,
		stage,
		rw,
		tsk->cmd,
		tsk->pid,
		tsk->len << SECTOR_SHIFT,
		mtk_btag_pr_time(str[0], 32, "queue_cmd", tsk->request_start_t),
		(stage > 1) ? mtk_btag_pr_time(str[1], 32, "send_cmd", tsk->send_cmd_t) : "",
		(stage > 2) ? mtk_btag_pr_time(str[2], 32, "req_compl", tsk->req_compl_t) : "",
		(stage > 3) ? mtk_btag_pr_time(str[3], 32, "done_start", tsk->scsi_done_start_t) : "",
		(stage > 4) ? mtk_btag_pr_time(str[4], 32, "done_end", tsk->scsi_done_end_t) : "",
		(busy_time) ? mtk_btag_pr_time(str[5], 32, "busy", busy_time) : "",
		mtk_btag_pr_speed(str[6], 32, busy_time, tsk->len));
}

void ufs_mtk_biolog_queue_command(unsigned int task_id, struct scsi_cmnd *cmd)
{
	unsigned long flags;
	struct ufs_mtk_bio_context *ctx;
	struct ufs_mtk_bio_context_task *tsk;

	if (!cmd)
		return;

	tsk = ufs_mtk_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;

	tsk->lba = scsi_cmnd_lba(cmd);
	tsk->len = scsi_cmnd_len(cmd);
	tsk->cmd = scsi_cmnd_cmd(cmd);

	tsk->request_start_t = sched_clock();
	tsk->send_cmd_t = 0;
	tsk->req_compl_t = 0;
	tsk->scsi_done_start_t = 0;
	tsk->scsi_done_end_t = 0;

	spin_lock_irqsave(&ctx->lock, flags);
	if (!ctx->period_start_t)
		ctx->period_start_t = tsk->request_start_t;
	spin_unlock_irqrestore(&ctx->lock, flags);

	ufs_mtk_pr_tsk(tsk, 1);

}

void ufs_mtk_biolog_send_command(unsigned int task_id)
{
	struct ufs_mtk_bio_context_task *tsk;

	tsk = ufs_mtk_bio_curr_task(task_id, NULL);
	if (!tsk)
		return;

	tsk->send_cmd_t = sched_clock();
	ufs_mtk_pr_tsk(tsk, 2);
}

void ufs_mtk_biolog_transfer_req_compl(unsigned int task_id)
{
	struct ufs_mtk_bio_context_task *tsk;

	tsk = ufs_mtk_bio_curr_task(task_id, NULL);
	if (!tsk)
		return;

	tsk->req_compl_t = sched_clock();
	ufs_mtk_pr_tsk(tsk, 3);
}

void ufs_mtk_biolog_scsi_done_start(unsigned int task_id)
{
	struct ufs_mtk_bio_context_task *tsk;

	tsk = ufs_mtk_bio_curr_task(task_id, NULL);
	if (!tsk)
		return;

	tsk->scsi_done_start_t = sched_clock();
	ufs_mtk_pr_tsk(tsk, 4);
}

void ufs_mtk_biolog_scsi_done_end(unsigned int task_id)
{
	struct ufs_mtk_bio_context *ctx;
	struct ufs_mtk_bio_context_task *tsk;
	struct mtk_btag_throughput_rw *tp = NULL;
	unsigned long flags;
	int rw = -1;
	__u64 busy_time;
	__u32 size;

	tsk = ufs_mtk_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;

	tsk->scsi_done_end_t = sched_clock();

	/* return if there's no on-going request  */
	if (!tsk->request_start_t || !tsk->send_cmd_t ||
		!tsk->req_compl_t || !tsk->scsi_done_start_t)
		return;

	spin_lock_irqsave(&ctx->lock, flags);

	if (tsk->cmd == 0x28) {
		rw = 0; /* READ */
		tp = &ctx->throughput.r;
	} else if (tsk->cmd == 0x2A) {
		rw = 1; /* WRITE */
		tp = &ctx->throughput.w;
	}

	/* throughput usage := duration of handling this request */
	busy_time = tsk->scsi_done_end_t - tsk->request_start_t;

	/* workload statistics */
	ctx->workload.count++;

	if (tp) {
		size = tsk->len << SECTOR_SHIFT;
		tp->usage += busy_time;
		tp->size += size;
	}

	spin_unlock_irqrestore(&ctx->lock, flags);

	ufs_mtk_pr_tsk(tsk, 5);
}

/* print trace to kerne log */
static void ufs_mtk_bio_print_klog(struct mtk_btag_trace *tr)
{
	int len;
	char *ptr;
	unsigned long flags;

	len = UFS_BIOLOG_PRINT_BUF-1;
	ptr = &ufs_mtk_bio_prbuf[0];

	spin_lock_irqsave(&ufs_mtk_bio_prbuf_lock, flags);
	mtk_btag_print_klog(&ptr, &len, tr);
	spin_unlock_irqrestore(&ufs_mtk_bio_prbuf_lock, flags);
}

/* evaluate throughput and workload of given context */
static void ufs_mtk_bio_context_eval(struct ufs_mtk_bio_context *ctx)
{
	uint64_t period;

	ctx->workload.usage = ctx->period_usage;

	if (ctx->workload.period > (ctx->workload.usage * 100)) {
		ctx->workload.percent = 1;
	} else {
		period = ctx->workload.period;
		do_div(period, 100);
		ctx->workload.percent = (__u32)ctx->workload.usage / (__u32)period;
	}
	mtk_btag_throughput_eval(&ctx->throughput);
}

/* print context to trace ring buffer */
static struct mtk_btag_trace *ufs_mtk_bio_print_trace(struct ufs_mtk_bio_context *ctx)
{
	struct mtk_btag_trace *tr;
	unsigned long flags;

	spin_lock_irqsave(&ufs_mtk_btag_ringtrace->lock, flags);
	tr = mtk_btag_curr_trace(ufs_mtk_btag_ringtrace);

	if (!tr)
		goto out;

	memset(tr, 0, sizeof(struct mtk_btag_trace));
	tr->pid = ctx->pid;
	tr->qid = ctx->qid;
	mtk_btag_pidlog_eval(&tr->pidlog, &ctx->pidlog);
	mtk_btag_vmstat_eval(&tr->vmstat);
	mtk_btag_cpu_eval(&tr->cpu);
	memcpy(&tr->throughput, &ctx->throughput, sizeof(struct mtk_btag_throughput));
	memcpy(&tr->workload, &ctx->workload, sizeof(struct mtk_btag_workload));

	tr->time = sched_clock();
	mtk_btag_next_trace(ufs_mtk_btag_ringtrace);
out:
	spin_unlock_irqrestore(&ufs_mtk_btag_ringtrace->lock, flags);
	return tr;
}

static void ufs_mtk_bio_ctx_count_usage(struct ufs_mtk_bio_context *ctx, __u64 start, __u64 end)
{
	__u64 busy_in_period;

	if (start < ctx->period_start_t)
		busy_in_period = end - ctx->period_start_t;
	else
		busy_in_period = end - start;

	ctx->period_usage += busy_in_period;
}

/* Check requests after set/clear mask. */
void ufs_mtk_biolog_check(unsigned long req_mask)
{
	struct ufs_mtk_bio_context *ctx;
	struct mtk_btag_trace *tr = NULL;
	__u64 end_time, period_time;
	unsigned long flags;

	ctx = ufs_mtk_bio_curr_ctx();
	if (!ctx)
		return;

	end_time = sched_clock();

	spin_lock_irqsave(&ctx->lock, flags);

	if (req_mask) {
		if (!ctx->busy_start_t) {  /* busy -> idle */
			ctx->busy_start_t = end_time;
		} else { /* busy -> busy, count usage */
			ufs_mtk_bio_ctx_count_usage(ctx, ctx->busy_start_t, end_time);
		}
	} else {
		if (ctx->busy_start_t) { /* idle -> busy */
			ufs_mtk_bio_ctx_count_usage(ctx, ctx->busy_start_t, end_time);
			ctx->busy_start_t = 0;
		} else {
			pr_err("ufs_mtk_biolog: warning, busy not in period\n");
		}
	}

	period_time = end_time - ctx->period_start_t;

	if (period_time >= UFS_MTK_BIO_TRACE_LATENCY) {
		ctx->period_end_t = end_time;
		ctx->workload.period = period_time;
		ufs_mtk_bio_context_eval(ctx);
		tr = ufs_mtk_bio_print_trace(ctx);
		ctx->period_start_t = end_time;
		ctx->period_end_t = 0;
		ctx->period_usage = 0;
		memset(&ctx->throughput, 0, sizeof(struct mtk_btag_throughput));
		memset(&ctx->workload, 0, sizeof(struct mtk_btag_workload));
	}
	spin_unlock_irqrestore(&ctx->lock, flags);

	if (ufs_mtk_bio_klog_enable && tr)
		ufs_mtk_bio_print_klog(tr);
}

static void ufs_mtk_bio_seq_debug_show_info(struct seq_file *seq)
{
	int i;
	size_t used_mem = 0;
	size_t size;

	if (!ufs_mtk_bio_reqctx)
		return;

	seq_puts(seq, "<Queue Info>\n");
	for (i = 0; i < UFS_BIOLOG_CONTEXTS; i++)	{
		if (ufs_mtk_bio_reqctx[i].pid == 0)
			continue;
		seq_printf(seq, "ufs_mtk_bio_reqctx[%d]=mt_ctx_map[%d],pid:%4d,q:%d\n",
			i,
			ufs_mtk_bio_reqctx[i].id,
			ufs_mtk_bio_reqctx[i].pid,
			ufs_mtk_bio_reqctx[i].qid);
	}
	seq_puts(seq, "<Detail Memory Usage>\n");
	if (ufs_mtk_bio_prbuf) {
		seq_printf(seq, "Kernel log print buffer: %d bytes\n", UFS_BIOLOG_PRINT_BUF);
		used_mem += UFS_BIOLOG_PRINT_BUF;
	}

	used_mem += mtk_btag_seq_usedmem(seq, ufs_mtk_btag_ringtrace);

	if (ufs_mtk_bio_reqctx) {
		size = sizeof(struct ufs_mtk_bio_context) * UFS_BIOLOG_CONTEXTS;
		seq_printf(seq, "Queue context: %d contexts * %zu = %zu bytes\n",
			UFS_BIOLOG_CONTEXTS,
			sizeof(struct ufs_mtk_bio_context),
			size);
		used_mem += size;
	}
	seq_printf(seq, "Total: %zu KB\n", used_mem >> 10);
}

static int ufs_mtk_bio_seq_debug_show(struct seq_file *seq, void *v)
{
	mtk_btag_seq_debug_show_ringtrace(seq, ufs_mtk_btag_ringtrace);
	ufs_mtk_bio_seq_debug_show_info(seq);
	return 0;
}


static const struct seq_operations ufs_mtk_bio_seq_debug_ops = {
	.start  = mtk_btag_seq_debug_start,
	.next   = mtk_btag_seq_debug_next,
	.stop   = mtk_btag_seq_debug_stop,
	.show   = ufs_mtk_bio_seq_debug_show,
};

static int ufs_mtk_bio_seq_debug_open(struct inode *inode, struct file *file)
{
	int rc;

	rc = seq_open(file, &ufs_mtk_bio_seq_debug_ops);
	if (rc == 0) {
		struct seq_file *m = file->private_data;

		m->private = &ufs_mtk_btag_ringtrace;
	}
	return rc;

}

static const struct file_operations ufs_mtk_bio_seq_debug_fops = {
	.owner		= THIS_MODULE,
	.open		= ufs_mtk_bio_seq_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
	.write		= mtk_btag_debug_write,
};


static void ufs_mtk_bio_init_task(struct ufs_mtk_bio_context_task *tsk)
{
	tsk->task_id = -1;
	tsk->pid = 0;
	tsk->lba = 0;
	tsk->len = 0;
	tsk->request_start_t = 0;
	tsk->send_cmd_t = 0;
	tsk->req_compl_t = 0;
	tsk->scsi_done_start_t = 0;
	tsk->scsi_done_end_t = 0;
}

static void ufs_mtk_bio_init_ctx(struct ufs_mtk_bio_context *ctx)
{
	int i;

	ctx->pid = 0;
	ctx->qid = 0;
	spin_lock_init(&ctx->lock);
	ctx->id = 0;
	ctx->period_start_t = sched_clock();

	for (i = 0; i < UFS_BIOLOG_CONTEXT_TASKS; i++)
		ufs_mtk_bio_init_task(&ctx->task[i]);
}

static void ufs_mtk_biolog_free(void)
{
	mtk_btag_ringtrace_free(ufs_mtk_btag_ringtrace);
	kfree(ufs_mtk_bio_prbuf);
	kfree(ufs_mtk_bio_reqctx);
	ufs_mtk_btag_ringtrace = NULL;
	ufs_mtk_bio_prbuf = NULL;
	ufs_mtk_bio_reqctx = NULL;
}

int ufs_mtk_biolog_init(void)
{
	ufs_mtk_btag_ringtrace = mtk_btag_ringtrace_alloc(UFS_BIOLOG_RINGBUF_MAX);
	ufs_mtk_bio_prbuf = kmalloc(UFS_BIOLOG_PRINT_BUF, GFP_NOFS);
	ufs_mtk_bio_reqctx = kmalloc_array(UFS_BIOLOG_CONTEXTS,
		sizeof(struct ufs_mtk_bio_context), GFP_NOFS);

	if (!ufs_mtk_btag_ringtrace || !ufs_mtk_bio_reqctx || !ufs_mtk_bio_prbuf)
		goto error_out;

	memset(ufs_mtk_bio_reqctx, 0, (sizeof(struct ufs_mtk_bio_context) * UFS_BIOLOG_CONTEXTS));

	ufs_mtk_bio_init_ctx(&ufs_mtk_bio_reqctx[0]);

	ufs_mtk_bio_used_mem =  (unsigned int)(sizeof(struct mtk_btag_trace) * UFS_BIOLOG_RINGBUF_MAX);
	ufs_mtk_bio_used_mem += (unsigned int)(sizeof(struct ufs_mtk_bio_context) * UFS_BIOLOG_CONTEXTS);

	ufs_mtk_bio_droot = debugfs_create_dir("blockio_ufs", NULL);

	if (IS_ERR(ufs_mtk_bio_droot)) {
		pr_warn("[BLOCK_TAG] ufs: fail to create debugfs root\n");
		goto out;
	}

	ufs_mtk_bio_dmem = debugfs_create_u32("used_mem", 0440, ufs_mtk_bio_droot, &ufs_mtk_bio_used_mem);

	if (IS_ERR(ufs_mtk_bio_dmem)) {
		pr_warn("[BLOCK_TAG] ufs: fail to create used_mem at debugfs\n");
		goto out;
	}

	ufs_mtk_bio_dklog = debugfs_create_u32("klog_enable", 0660, ufs_mtk_bio_droot, &ufs_mtk_bio_klog_enable);

	if (IS_ERR(ufs_mtk_bio_dklog)) {
		pr_warn("[BLOCK_TAG] ufs: fail to create klog_enable at debugfs\n");
		goto out;
	}

	ufs_mtk_bio_dlog = debugfs_create_file("blockio", S_IFREG | S_IRUGO,
		ufs_mtk_bio_droot, (void *)0, &ufs_mtk_bio_seq_debug_fops);

	if (IS_ERR(ufs_mtk_bio_dlog)) {
		pr_warn("[BLOCK_TAG] ufs: fail to create log at debugfs\n");
		goto out;
	}

	spin_lock_init(&ufs_mtk_bio_prbuf_lock);

out:
	return 0;

error_out:
	ufs_mtk_biolog_free();
	return 0;
}

int ufs_mtk_biolog_exit(void)
{
	ufs_mtk_biolog_free();
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek UFS Block IO Log");

