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

#define DEBUG 1

#include <linux/debugfs.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/tick.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

#include <linux/vmalloc.h>
#include <linux/blk_types.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/module.h>

#ifdef CONFIG_MTK_EXTMEM
#include <linux/exm_driver.h>
#endif

#include "mtk_mmc_block.h"
#include <mt-plat/mtk_blocktag.h>

#define SECTOR_SHIFT 9

/* ring trace for debugfs */
struct mtk_btag_ringtrace *mt_bio_ringtrace;

/* context buffer of each request queue */
struct mt_bio_context *mt_bio_reqctx;
struct mt_bio_context *mt_ctx_map[MMC_BIOLOG_CONTEXTS] = { 0 };

/* context index for mt_ctx_map */
enum {
	CTX_MMCQD0 = 0,
	CTX_MMCQD1 = 1,
	CTX_MMCQD0_BOOT0 = 2,
	CTX_MMCQD0_BOOT1 = 3,
	CTX_MMCQD0_RPMB  = 4,
	CTX_EXECQ  = 9
};

/* context state for command queue */
enum {
	CMDQ_CTX_NOT_DMA = 0,
	CMDQ_CTX_IN_DMA = 1,
	CMDQ_CTX_QUEUE = 2
};

/* context state for mmcqd */
enum {
	MMCQD_NORMAL = 0,
	MMCQD_CMDQ_MODE_EN = 1
};

/* kernel print buffer */
static char *mt_bio_prbuf;
spinlock_t mt_bio_prbuf_lock;

/* debugfs dentries */
struct dentry *mt_bio_droot;
struct dentry *mt_bio_dklog;
struct dentry *mt_bio_dlog;
struct dentry *mt_bio_dmem;

/* kernel log enable
 * 0: disable
 * 1: enable kernel log
 * 2: enable kernel log with cmdq task trace
 * 3: enable kernel log with cmdq task stage trace
 */
unsigned int mt_bio_klog_enable;
unsigned int mt_bio_used_mem;

#define MT_BIO_TRACE_LATENCY (unsigned long long)(1000000000)
#define MT_BIO_TRACE_TIMEOUT ((MT_BIO_TRACE_LATENCY)*10)

#define biolog_fmt "wl:%d%%,%lld,%lld,%d.vm:%lld,%lld,%lld,%lld,%lld.cpu:%llu,%llu,%llu,%llu,%llu,%llu,%llu.pid:%d,"
#define biolog_fmt_wt "wt:%d,%d,%lld."
#define biolog_fmt_rt "rt:%d,%d,%lld."
#define pidlog_fmt "{%05d:%05d:%08d:%05d:%08d}"

#define REQ_EXECQ  "exe_cq"
#define REQ_MMCQD0 "mmcqd/0"
#define REQ_MMCQD0_BOOT0 "mmcqd/0boot0"
#define REQ_MMCQD0_BOOT1 "mmcqd/0boot1"
#define REQ_MMCQD0_RPMB  "mmcqd/0rpmb"
#define REQ_MMCQD1 "mmcqd/1"


/* queue id:
 * 0=internal storage (emmc:mmcqd0/exe_cq),
 * 1=external storage (t-card:mmcqd1)
 */
static int get_qid_by_name(const char *str)
{
	if (strncmp(str, REQ_EXECQ, strlen(REQ_EXECQ)) == 0)
		return 0;
	if (strncmp(str, REQ_MMCQD0, strlen(REQ_MMCQD0)) == 0)
		return 0;  /* this includes boot0, boot1 */
	if (strncmp(str, REQ_MMCQD1, strlen(REQ_MMCQD1)) == 0)
		return 1;
	return 99;
}

/* get context id to mt_ctx_map[] by name */
static int get_ctxid_by_name(const char *str)
{
	if (strncmp(str, REQ_EXECQ, strlen(REQ_EXECQ)) == 0)
		return CTX_EXECQ;
	if (strncmp(str, REQ_MMCQD0_RPMB, strlen(REQ_MMCQD0_RPMB)) == 0)
		return CTX_MMCQD0_RPMB;
	if (strncmp(str, REQ_MMCQD0_BOOT0, strlen(REQ_MMCQD0_BOOT0)) == 0)
		return CTX_MMCQD0_BOOT0;
	if (strncmp(str, REQ_MMCQD0_BOOT1, strlen(REQ_MMCQD0_BOOT1)) == 0)
		return CTX_MMCQD0_BOOT1;
	if (strncmp(str, REQ_MMCQD0, strlen(REQ_MMCQD0)) == 0)
		return CTX_MMCQD0;
	if (strncmp(str, REQ_MMCQD1, strlen(REQ_MMCQD1)) == 0)
		return CTX_MMCQD1;
	return -1;
}

static void mt_bio_init_task(struct mt_bio_context_task *tsk)
{
	tsk->task_id = -1;
	tsk->arg = 0;
	tsk->request_start_t = 0;
	tsk->transfer_start_t = 0;
	tsk->transfer_end_t = 0;
	tsk->wait_start_t = 0;
}

static void mt_bio_init_ctx(struct mt_bio_context *ctx, struct task_struct *thread)
{
	int i;

	ctx->pid = task_pid_nr(thread);
	get_task_comm(ctx->comm, thread);
	ctx->qid = get_qid_by_name(ctx->comm);
	spin_lock_init(&ctx->lock);
	ctx->id = get_ctxid_by_name(ctx->comm);
	if (ctx->id >= 0)
		mt_ctx_map[ctx->id] = ctx;
	ctx->period_start_t = sched_clock();

	for (i = 0; i < MMC_BIOLOG_CONTEXT_TASKS; i++)
		mt_bio_init_task(&ctx->task[i]);
}

void mt_bio_queue_alloc(struct task_struct *thread)
{
	int i;
	pid_t pid;

	pid = task_pid_nr(thread);

	for (i = 0; i < MMC_BIOLOG_CONTEXTS; i++)	{
		struct mt_bio_context *ctx = &mt_bio_reqctx[i];

		if (ctx->pid == pid)
			break;
		if (ctx->pid == 0) {
			mt_bio_init_ctx(ctx, thread);
			break;
		}
	}
}

void mt_bio_queue_free(struct task_struct *thread)
{
	int i;
	pid_t pid;

	pid = task_pid_nr(thread);

	for (i = 0; i < MMC_BIOLOG_CONTEXTS; i++)	{
		struct mt_bio_context *ctx = &mt_bio_reqctx[i];

		if (ctx->pid == pid) {
			mt_ctx_map[ctx->id] = NULL;
			memset(ctx, 0, sizeof(struct mt_bio_context));
			break;
		}
	}
}

/* get context correspond to current process */
static struct mt_bio_context *mt_bio_curr_ctx(void)
{
	int i;
	pid_t qd_pid;

	if (!mt_bio_reqctx)
		return NULL;

	qd_pid = task_pid_nr(current);

	for (i = 0; i < MMC_BIOLOG_CONTEXTS; i++)	{
		struct mt_bio_context *ctx = &mt_bio_reqctx[i];

		if (ctx->pid == 0)
			continue;
		if (qd_pid == ctx->pid)
			return ctx;
	}

	return NULL;
}

/* get other queue's context by context id */
static struct mt_bio_context *mt_bio_get_ctx(int id)
{
	if (id < 0 || id >= MMC_BIOLOG_CONTEXTS)
		return NULL;

	return mt_ctx_map[id];
}

/* append a pidlog to given context */
int mtk_btag_pidlog_add_mmc(pid_t pid, __u32 len, int rw)
{
	unsigned long flags;
	struct mt_bio_context *ctx;

	ctx = mt_bio_curr_ctx();
	if (!ctx)
		return 0;

	spin_lock_irqsave(&ctx->lock, flags);
	mtk_btag_pidlog_insert(&ctx->pidlog, pid, len, rw);
	spin_unlock_irqrestore(&ctx->lock, flags);

	return 1;
}
EXPORT_SYMBOL_GPL(mtk_btag_pidlog_add_mmc);

/* evaluate throughput and workload of given context */
static void mt_bio_context_eval(struct mt_bio_context *ctx)
{
	struct mt_bio_context_task *tsk;
	uint64_t min, period;
	int i;

	min = ctx->period_end_t;

	/* for all tasks if there is an on-going request */
	for (i = 0; i < MMC_BIOLOG_CONTEXT_TASKS; i++) {
		tsk = &ctx->task[i];
		if (tsk->task_id >= 0) {
			if (tsk->request_start_t > 0 &&
				tsk->request_start_t >= ctx->period_start_t &&
				tsk->request_start_t < min)
				min = tsk->request_start_t;
		}
	}
	ctx->period_usage += (ctx->period_end_t - min);
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

/* print trace to kerne log */
static void mt_bio_print_klog(struct mtk_btag_trace *tr)
{
	int len;
	char *ptr;
	unsigned long flags;

	if (!mt_bio_prbuf)
		return;

	len = MMC_BIOLOG_PRINT_BUF-1;
	ptr = &mt_bio_prbuf[0];

	spin_lock_irqsave(&mt_bio_prbuf_lock, flags);
	mtk_btag_print_klog(&ptr, &len, tr);
	spin_unlock_irqrestore(&mt_bio_prbuf_lock, flags);
}

/* print context to trace ring buffer */
static void mt_bio_print_trace(struct mt_bio_context *ctx)
{
	struct mtk_btag_trace *tr;
	struct mt_bio_context *pid_ctx = ctx;
	unsigned long flags;

	if (ctx->id == CTX_EXECQ)
		pid_ctx = mt_bio_get_ctx(CTX_MMCQD0);

	spin_lock_irqsave(&mt_bio_ringtrace->lock, flags);
	tr = mtk_btag_curr_trace(mt_bio_ringtrace);

	if (!tr)
		goto out;

	memset(tr, 0, sizeof(struct mtk_btag_trace));
	tr->pid = ctx->pid;
	tr->qid = ctx->qid;
	memcpy(&tr->throughput, &ctx->throughput, sizeof(struct mtk_btag_throughput));
	memcpy(&tr->workload, &ctx->workload, sizeof(struct mtk_btag_workload));

	if (pid_ctx)
		mtk_btag_pidlog_eval(&tr->pidlog, &pid_ctx->pidlog);

	mtk_btag_vmstat_eval(&tr->vmstat);
	mtk_btag_cpu_eval(&tr->cpu);

	tr->time = sched_clock();

	if (mt_bio_klog_enable)
		mt_bio_print_klog(tr);

	mtk_btag_next_trace(mt_bio_ringtrace);
out:
	spin_unlock_irqrestore(&mt_bio_ringtrace->lock, flags);
}


static struct mt_bio_context_task *mt_bio_get_task(struct mt_bio_context *ctx, unsigned int task_id)
{
	struct mt_bio_context_task *tsk;
	int i, avail = -1;

	if (!ctx)
		return NULL;

	for (i = 0; i < MMC_BIOLOG_CONTEXT_TASKS; i++) {
		tsk = &ctx->task[i];
		if (tsk->task_id == task_id)
			return tsk;
		if ((tsk->task_id < 0) && (avail < 0))
			avail = i;
	}

	if (avail >= 0) {
		tsk = &ctx->task[avail];
		tsk->task_id = task_id;
		return tsk;
	}

	pr_warn("mt_bio_get_task: out of task in context %s\n", ctx->comm);

	return NULL;
}

static struct mt_bio_context_task *mt_bio_curr_task(unsigned int task_id, struct mt_bio_context **curr_ctx)
{
	struct mt_bio_context *ctx;

	ctx = mt_bio_curr_ctx();
	if (curr_ctx)
		*curr_ctx = ctx;
	return mt_bio_get_task(ctx, task_id);
}

static void mt_pr_cmdq_tsk_final(struct mt_bio_context_task *tsk,
	int stage, __u64 busy_time, __u64 end_time)
{
	int rw;
	__u32 sectors;
	char str[7][32];

	if (!((mt_bio_klog_enable == 2 && stage == 5) || (mt_bio_klog_enable == 3)))
		return;

	rw = tsk->arg & (1<<30);  /* write: 0, read: 1 */
	sectors = tsk->arg & 0xFFFF;

	pr_debug("[BLOCK_TAG] cmdq: tsk[%d],%d,%s,len=%d%s%s%s%s%s%s%s\n",
		tsk->task_id,
		stage,
		(rw)?"r":"w",
		sectors << SECTOR_SHIFT,
		mtk_btag_pr_time(str[0], 32, "req_start", tsk->request_start_t),
		(stage > 1) ? mtk_btag_pr_time(str[1], 32, "dma_start", tsk->transfer_start_t) : "",
		(stage > 2) ? mtk_btag_pr_time(str[2], 32, "dma_end", tsk->transfer_end_t) : "",
		(stage > 3) ? mtk_btag_pr_time(str[3], 32, "isdone_start", tsk->wait_start_t) : "",
		(stage > 4) ? mtk_btag_pr_time(str[4], 32, "isdone_end", end_time) : "",
		(busy_time) ? mtk_btag_pr_time(str[5], 32, "busy", busy_time) : "",
		mtk_btag_pr_speed(str[6], 32, busy_time, sectors));
}

#define mt_pr_cmdq_tsk(tsk, stage) mt_pr_cmdq_tsk_final(tsk, stage, 0, 0)

void mt_biolog_cmdq_check(void)
{
	struct mt_bio_context *ctx;
	__u64 end_time, period_time;

	ctx = mt_bio_curr_ctx();
	if (!ctx)
		return;

	end_time = sched_clock();
	period_time = end_time - ctx->period_start_t;

	if (period_time >= MT_BIO_TRACE_LATENCY) {
		ctx->period_end_t = end_time;
		ctx->workload.period = period_time;
		mt_bio_context_eval(ctx);
		mt_bio_print_trace(ctx);
		ctx->period_start_t = end_time;
		ctx->period_end_t = 0;
		ctx->period_usage = 0;
		memset(&ctx->throughput, 0, sizeof(struct mtk_btag_throughput));
		memset(&ctx->workload, 0, sizeof(struct mtk_btag_workload));
	}
}

/* Command Queue Hook: stage1: queue task */
void mt_biolog_cmdq_queue_task(unsigned int task_id, struct mmc_request *req)
{
	struct mt_bio_context *ctx;
	struct mt_bio_context_task *tsk;

	if (!req)
		return;
	if (!req->sbc)
		return;

	tsk = mt_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;

	if (ctx->state == CMDQ_CTX_NOT_DMA)
		ctx->state = CMDQ_CTX_QUEUE;

	tsk->arg = req->sbc->arg;
	tsk->request_start_t = sched_clock();
	tsk->transfer_start_t = 0;
	tsk->transfer_end_t = 0;
	tsk->wait_start_t = 0;

	if (!ctx->period_start_t)
		ctx->period_start_t = tsk->request_start_t;

	mt_pr_cmdq_tsk(tsk, 1);
}

static void mt_bio_ctx_count_usage(struct mt_bio_context *ctx, __u64 start, __u64 end)
{
	__u64 busy_in_period;

	if (start < ctx->period_start_t)
		busy_in_period = end - ctx->period_start_t;
	else
		busy_in_period = end - start;

	ctx->period_usage += busy_in_period;
}

/* Command Queue Hook: stage2: dma start */
void mt_biolog_cmdq_dma_start(unsigned int task_id)
{
	struct mt_bio_context_task *tsk;
	struct mt_bio_context *ctx;

	tsk = mt_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;
	tsk->transfer_start_t = sched_clock();

	/* count first queue task time in workload usage,
	 * if it was not overlapped with DMA
	 */
	if (ctx->state == CMDQ_CTX_QUEUE)
		mt_bio_ctx_count_usage(ctx, tsk->request_start_t, tsk->transfer_start_t);
	ctx->state = CMDQ_CTX_IN_DMA;
	mt_pr_cmdq_tsk(tsk, 2);
}

/* Command Queue Hook: stage3: dma end */
void mt_biolog_cmdq_dma_end(unsigned int task_id)
{
	struct mt_bio_context_task *tsk;
	struct mt_bio_context *ctx;

	tsk = mt_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;
	tsk->transfer_end_t = sched_clock();
	ctx->state = CMDQ_CTX_NOT_DMA;
	mt_pr_cmdq_tsk(tsk, 3);
}

/* Command Queue Hook: stage4: isdone start */
void mt_biolog_cmdq_isdone_start(unsigned int task_id, struct mmc_request *req)
{
	struct mt_bio_context_task *tsk;

	tsk = mt_bio_curr_task(task_id, NULL);
	if (!tsk)
		return;
	tsk->wait_start_t = sched_clock();
	mt_pr_cmdq_tsk(tsk, 4);
}

/* Command Queue Hook: stage5: isdone end */
void mt_biolog_cmdq_isdone_end(unsigned int task_id)
{
	int rw;
	__u32 bytes;
	__u64 end_time, busy_time;
	struct mt_bio_context *ctx;
	struct mt_bio_context_task *tsk;
	struct mtk_btag_throughput_rw *tp;

	tsk = mt_bio_curr_task(task_id, &ctx);
	if (!tsk)
		return;

	/* return if there's no on-going request  */
	if (!tsk->request_start_t || !tsk->transfer_start_t || !tsk->wait_start_t)
		return;

	end_time = sched_clock();

	/* throughput usage := duration of handling this request */
	rw = tsk->arg & (1<<30);  /* write: 0, read: 1 */
	bytes = tsk->arg & 0xFFFF;
	bytes = bytes << SECTOR_SHIFT;
	busy_time = end_time - tsk->request_start_t;
	tp = (rw) ? &ctx->throughput.r : &ctx->throughput.w;
	tp->usage += busy_time;
	tp->size += bytes;

	/* workload statistics */
	ctx->workload.count++;

	/* count DMA time in workload usage */

	/* count isdone time in workload usage, if it was not overlapped with DMA */
	if (ctx->state == CMDQ_CTX_NOT_DMA)
		mt_bio_ctx_count_usage(ctx, tsk->transfer_start_t, end_time);
	else
		mt_bio_ctx_count_usage(ctx, tsk->transfer_start_t, tsk->transfer_end_t);

	mt_pr_cmdq_tsk_final(tsk, 5, busy_time, end_time);

	mt_bio_init_task(tsk);
}


/* MMC Queue Hook: check function at mmc_blk_issue_rw_rq() */
void mt_biolog_mmcqd_req_check(void)
{
	struct mt_bio_context *ctx;
	__u64 end_time, period_time;

	ctx = mt_bio_curr_ctx();
	if (!ctx)
		return;

	/* skip mmcqd0, if command queue is applied */
	if ((ctx->id == CTX_MMCQD0) && (ctx->state == MMCQD_CMDQ_MODE_EN))
		return;

	end_time = sched_clock();
	period_time = end_time - ctx->period_start_t;

	if (period_time >= MT_BIO_TRACE_LATENCY) {
		ctx->period_end_t = end_time;
		ctx->workload.period = period_time;
		mt_bio_context_eval(ctx);
		mt_bio_print_trace(ctx);
		ctx->period_start_t = end_time;
		ctx->period_end_t = 0;
		ctx->period_usage = 0;
		memset(&ctx->throughput, 0, sizeof(struct mtk_btag_throughput));
		memset(&ctx->workload, 0, sizeof(struct mtk_btag_workload));
	}
}

/* MMC Queue Hook: request start function at mmc_start_req() */
void mt_biolog_mmcqd_req_start(struct mmc_host *host)
{
	struct mt_bio_context *ctx;
	struct mt_bio_context_task *tsk;

	tsk = mt_bio_curr_task(0, &ctx);
	if (!tsk)
		return;
	tsk->request_start_t = sched_clock();

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	if ((ctx->id == CTX_MMCQD0) &&
		(ctx->state == MMCQD_NORMAL) &&
		host->card->ext_csd.cmdq_mode_en)
		ctx->state = MMCQD_CMDQ_MODE_EN;
#endif
}

/* MMC Queue Hook: request end function at mmc_start_req() */
void mt_biolog_mmcqd_req_end(struct mmc_data *data)
{
	int rw;
	__u32 size;
	struct mt_bio_context *ctx;
	struct mt_bio_context_task *tsk;
	struct mtk_btag_throughput_rw *tp;
	__u64 end_time, busy_time;

	end_time = sched_clock();

	if (!data)
		return;

	if (data->flags == MMC_DATA_WRITE)
		rw = 0;
	else if (data->flags == MMC_DATA_READ)
		rw = 1;
	else
		return;

	tsk = mt_bio_curr_task(0, &ctx);
	if (!tsk)
		return;

	/* return if there's no on-going request */
	if (!tsk->request_start_t)
		return;

	size = (data->blocks * data->blksz);
	busy_time = end_time - tsk->request_start_t;

	/* workload statistics */
	ctx->workload.count++;

	/* count request handling time in workload usage */
	mt_bio_ctx_count_usage(ctx, tsk->request_start_t, end_time);

	/* throughput statistics */
	tp = (rw) ? &ctx->throughput.r : &ctx->throughput.w; /* write: 0, read: 1 */
	tp->usage += busy_time;
	tp->size += size;

	/* re-init task to indicate no on-going request */
	mt_bio_init_task(tsk);
}

static void mt_bio_seq_debug_show_info(struct seq_file *seq)
{
	int i;
	size_t used_mem = 0;
	size_t size;

	if (!mt_bio_reqctx)
		return;

	seq_puts(seq, "<Queue Info>\n");
	for (i = 0; i < MMC_BIOLOG_CONTEXTS; i++)	{
		if (mt_bio_reqctx[i].pid == 0)
			continue;
		seq_printf(seq, "mt_bio_reqctx[%d]=mt_ctx_map[%d]=%s,pid:%4d,q:%d\n",
			i,
			mt_bio_reqctx[i].id,
			mt_bio_reqctx[i].comm,
			mt_bio_reqctx[i].pid,
			mt_bio_reqctx[i].qid);
	}
	seq_puts(seq, "<Detail Memory Usage>\n");
	if (mt_bio_prbuf) {
		seq_printf(seq, "Kernel log print buffer: %d bytes\n", MMC_BIOLOG_PRINT_BUF);
		used_mem += MMC_BIOLOG_PRINT_BUF;
	}

	used_mem += mtk_btag_seq_usedmem(seq, mt_bio_ringtrace);

	if (mt_bio_reqctx) {
		size = sizeof(struct mt_bio_context) * MMC_BIOLOG_CONTEXTS;
		seq_printf(seq, "Queue context: %d contexts * %zu = %zu bytes\n",
			MMC_BIOLOG_CONTEXTS,
			sizeof(struct mt_bio_context),
			size);
		used_mem += size;
	}
	seq_printf(seq, "Total: %zu KB\n", used_mem >> 10);
}

static int mt_bio_seq_debug_show(struct seq_file *seq, void *v)
{
	mtk_btag_seq_debug_show_ringtrace(seq, mt_bio_ringtrace);
	mt_bio_seq_debug_show_info(seq);
	return 0;
}

static const struct seq_operations mt_bio_seq_debug_ops = {
	.start  = mtk_btag_seq_debug_start,
	.next   = mtk_btag_seq_debug_next,
	.stop   = mtk_btag_seq_debug_stop,
	.show   = mt_bio_seq_debug_show,
};

static int mt_bio_seq_debug_open(struct inode *inode, struct file *file)
{
	int rc;

	rc = seq_open(file, &mt_bio_seq_debug_ops);
	if (rc == 0) {
		struct seq_file *m = file->private_data;

		m->private = &mt_bio_ringtrace;
	}
	return rc;

}

static const struct file_operations mt_bio_seq_debug_fops = {
	.owner		= THIS_MODULE,
	.open		= mt_bio_seq_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
	.write		= mtk_btag_debug_write,
};

static void mt_mmc_biolog_free(void)
{
	mtk_btag_ringtrace_free(mt_bio_ringtrace);
	kfree(mt_bio_prbuf);
	kfree(mt_bio_reqctx);
	mt_bio_prbuf = NULL;
	mt_bio_ringtrace = NULL;
	mt_bio_reqctx = NULL;
}

int mt_mmc_biolog_init(void)
{
	mt_bio_ringtrace = mtk_btag_ringtrace_alloc(MMC_BIOLOG_RINGBUF_MAX);
	mt_bio_prbuf = kmalloc(MMC_BIOLOG_PRINT_BUF, GFP_NOFS);
	mt_bio_reqctx = kmalloc_array(MMC_BIOLOG_CONTEXTS,
		sizeof(struct mt_bio_context), GFP_NOFS);

	if (!mt_bio_ringtrace || !mt_bio_reqctx || !mt_bio_prbuf)
		goto error_out;

	memset(mt_bio_reqctx, 0, (sizeof(struct mt_bio_context) * MMC_BIOLOG_CONTEXTS));

	mt_bio_used_mem =  (unsigned int)(sizeof(struct mtk_btag_trace) * MMC_BIOLOG_RINGBUF_MAX);
	mt_bio_used_mem += (unsigned int)(sizeof(struct mt_bio_context) * MMC_BIOLOG_CONTEXTS);
	mt_bio_used_mem += mtk_btag_used_mem_get();

	mt_bio_droot = debugfs_create_dir("blockio_mmc", NULL);

	if (IS_ERR(mt_bio_droot)) {
		pr_warn("[BLOCK_TAG] blockio: fail to create debugfs root\n");
		goto out;
	}

	mt_bio_dmem = debugfs_create_u32("used_mem", 0440, mt_bio_droot, &mt_bio_used_mem);

	if (IS_ERR(mt_bio_dmem)) {
		pr_warn("[BLOCK_TAG] blockio: fail to create used_mem at debugfs\n");
		goto out;
	}

	mt_bio_dklog = debugfs_create_u32("klog_enable", 0660, mt_bio_droot, &mt_bio_klog_enable);

	if (IS_ERR(mt_bio_dklog)) {
		pr_warn("[BLOCK_TAG] blockio: fail to create klog_enable at debugfs\n");
		goto out;
	}

	mt_bio_dlog = debugfs_create_file("blockio", S_IFREG | S_IRUGO, NULL, (void *)0, &mt_bio_seq_debug_fops);

	if (IS_ERR(mt_bio_dlog)) {
		pr_warn("[BLOCK_TAG] blockio: fail to create log at debugfs\n");
		goto out;
	}

	spin_lock_init(&mt_bio_prbuf_lock);

out:
	return 0;

error_out:
	mt_mmc_biolog_free();
	return 0;
}
EXPORT_SYMBOL_GPL(mt_mmc_biolog_init);

int mt_mmc_biolog_exit(void)
{
	mt_mmc_biolog_free();
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek MMC Block IO Log");


