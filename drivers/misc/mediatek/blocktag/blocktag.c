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

#include <linux/debugfs.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/cpumask.h>
#include <linux/cputime.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/blk_types.h>
#include <linux/module.h>

#ifdef CONFIG_MTK_EXTMEM
#include <linux/exm_driver.h>
#endif

#include <mt-plat/mtk_blocktag.h>

/* debugfs dentries */
struct dentry *mtk_btag_droot;
struct dentry *mtk_btag_dmem;

/* pid logger: page loger*/
unsigned long long mtk_btag_system_dram_size;
struct page_pid_logger *mtk_btag_pagelogger;
spinlock_t mtk_btag_pagelogger_lock;
unsigned int mtk_btag_used_mem;

unsigned int mtk_btag_used_mem_get(void)
{
	return mtk_btag_used_mem;
}
EXPORT_SYMBOL_GPL(mtk_btag_used_mem_get);

size_t mtk_btag_seq_usedmem(struct seq_file *seq, struct mtk_btag_ringtrace *rt)
{
	size_t used_mem = 0;
	size_t size = 0;

	if (mtk_btag_used_mem) {
		size = (sizeof(struct page_pid_logger)*(mtk_btag_system_dram_size >> PAGE_SHIFT));
		seq_printf(seq, "Page Logger buffer: %llu entries * %zu = %zu bytes\n",
			(mtk_btag_system_dram_size >> PAGE_SHIFT),
			sizeof(struct page_pid_logger),
			size);
		used_mem += size;
	}

	if (rt) {
		size = (sizeof(struct mtk_btag_trace) * rt->max);
		seq_printf(seq, "Debugfs ring buffer: %d traces * %zu = %zu bytes\n",
			rt->max,
			sizeof(struct mtk_btag_trace),
			size);
		used_mem += size;
	}

	return used_mem;
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_usedmem);

#define biolog_fmt "wl:%d%%,%lld,%lld,%d.vm:%lld,%lld,%lld,%lld,%lld.cpu:%llu,%llu,%llu,%llu,%llu,%llu,%llu.pid:%d,"
#define biolog_fmt_wt "wt:%d,%d,%lld."
#define biolog_fmt_rt "rt:%d,%d,%lld."
#define pidlog_fmt "{%05d:%05d:%08d:%05d:%08d}"

void mtk_btag_pidlog_insert(struct mtk_btag_pidlogger *pidlog, pid_t pid, __u32 len, int rw)
{
	int i;
	struct mtk_btag_pidlogger_entry *pe;
	struct mtk_btag_pidlogger_entry_rw *prw;

	for (i = 0; i < MTK_BLOCK_TAG_PIDLOG_ENTRIES; i++) {
		pe = &pidlog->info[i];
		if ((pe->pid == pid) || (pe->pid == 0)) {
			pe->pid = pid;
			prw = (rw) ? &pe->w : &pe->r;
			prw->count++;
			prw->length += len;
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(mtk_btag_pidlog_insert);

static void mtk_btag_pidlog_add(unsigned short pid, __u32 len, int rw)
{
	if (pid != 0xFFFF) {
#ifdef CONFIG_MTK_MMC_BLOCK_IO_LOG
		if (mtk_btag_pidlog_add_mmc(pid, len, rw))
			return;
#endif
#ifdef CONFIG_MTK_UFS_BLOCK_IO_LOG
		mtk_btag_pidlog_add_ufs(pid, len, rw);
#endif
	}
}

/*
 * pidlog: hook function for __blk_bios_map_sg()
 * rw: 0=read, 1=write
 */
void mt_pidlog_map_sg(struct bio_vec *bvec, int rw)
{
	struct page_pid_logger *ppl, tmp;
	unsigned long page_offset;
	unsigned long flags;

	if (!mtk_btag_pagelogger)
		return;

	page_offset = (unsigned long)(__page_to_pfn(bvec->bv_page)) - PHYS_PFN_OFFSET;
	spin_lock_irqsave(&mtk_btag_pagelogger_lock, flags);
	ppl = ((struct page_pid_logger *)mtk_btag_pagelogger) + page_offset;
	tmp.pid1 = ppl->pid1;
	tmp.pid2 = ppl->pid2;
	spin_unlock_irqrestore(&mtk_btag_pagelogger_lock, flags);

	mtk_btag_pidlog_add(tmp.pid1, bvec->bv_len, rw);
	mtk_btag_pidlog_add(tmp.pid2, bvec->bv_len, rw);
}
EXPORT_SYMBOL_GPL(mt_pidlog_map_sg);

/* pidlog: hook function for submit_bio() */
void mt_pidlog_submit_bio(struct bio *bio)
{
	struct bio_vec bvec;
	struct bvec_iter iter;

	if (!mtk_btag_pagelogger)
		return;

	bio_for_each_segment(bvec, bio, iter) {
		struct page_pid_logger *ppl;
		unsigned long flags;

		if (bvec.bv_page) {
			unsigned long page_index;

			page_index = (unsigned long)(__page_to_pfn(bvec.bv_page)) - PHYS_PFN_OFFSET;
			ppl = ((struct page_pid_logger *)mtk_btag_pagelogger) + page_index;
			spin_lock_irqsave(&mtk_btag_pagelogger_lock, flags);
			if (page_index < (mtk_btag_system_dram_size >> PAGE_SHIFT)) {
				if (ppl->pid1 == 0XFFFF && ppl->pid2 != current->pid)
					ppl->pid1 = current->pid;
				else if (ppl->pid1 != current->pid)
					ppl->pid2 = current->pid;
			}
			spin_unlock_irqrestore(&mtk_btag_pagelogger_lock, flags);
		}
	}
}
EXPORT_SYMBOL_GPL(mt_pidlog_submit_bio);

/* pidlog: hook function for filesystem's write_begin() */
void mt_pidlog_write_begin(struct page *p)
{
	struct page_pid_logger *ppl;
	unsigned long flags;
	unsigned long page_index;

	if (!p)
		return;

	page_index = (unsigned long)(__page_to_pfn(p)) - PHYS_PFN_OFFSET;
	ppl = ((struct page_pid_logger *)mtk_btag_pagelogger) + page_index;
	spin_lock_irqsave(&mtk_btag_pagelogger_lock, flags);
	if (page_index < (mtk_btag_system_dram_size >> PAGE_SHIFT)) {
		if (ppl->pid1 == 0XFFFF)
			ppl->pid1 = current->pid;
		else if (ppl->pid1 != current->pid)
			ppl->pid2 = current->pid;
	}
	spin_unlock_irqrestore(&mtk_btag_pagelogger_lock, flags);
}
EXPORT_SYMBOL_GPL(mt_pidlog_write_begin);

/* evaluate vmstat trace from global_page_state() */
void mtk_btag_vmstat_eval(struct mtk_btag_vmstat *vm)
{
	vm->file_pages = ((global_page_state(NR_FILE_PAGES)) << (PAGE_SHIFT - 10));
	vm->file_dirty = ((global_page_state(NR_FILE_DIRTY)) << (PAGE_SHIFT - 10));
	vm->dirtied = ((global_page_state(NR_DIRTIED))	<< (PAGE_SHIFT - 10));
	vm->writeback = ((global_page_state(NR_WRITEBACK))	<< (PAGE_SHIFT - 10));
	vm->written = ((global_page_state(NR_WRITTEN))	<< (PAGE_SHIFT - 10));
}
EXPORT_SYMBOL_GPL(mtk_btag_vmstat_eval);

/* evaluate pidlog trace from context */
void mtk_btag_pidlog_eval(struct mtk_btag_pidlogger *pl, struct mtk_btag_pidlogger *ctx_pl)
{
	int i;

	for (i = 0; i < MTK_BLOCK_TAG_PIDLOG_ENTRIES; i++) {
		if (ctx_pl->info[i].pid == 0)
			break;
	}

	if (i != 0) {
		int size = i * sizeof(struct mtk_btag_pidlogger_entry);

		memcpy(&pl->info[0], &ctx_pl->info[0], size);
		memset(&ctx_pl->info[0], 0, size);
	}
}
EXPORT_SYMBOL_GPL(mtk_btag_pidlog_eval);

static __u64 mtk_btag_cpu_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;

	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = usecs_to_cputime64(idle_time);

	return idle;
}

static __u64 mtk_btag_cpu_iowait_time(int cpu)
{
	__u64 iowait, iowait_time = -1ULL;

	if (cpu_online(cpu))
		iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	else
		iowait = usecs_to_cputime64(iowait_time);

	return iowait;
}

/* evaluate cpu trace from kcpustat_cpu() */
void mtk_btag_cpu_eval(struct mtk_btag_cpu *cpu)
{
	int i;
	__u64 user, nice, system, idle, iowait, irq, softirq;

	user = nice = system = idle = iowait = irq = softirq = 0;

	for_each_possible_cpu(i) {
		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += mtk_btag_cpu_idle_time(i);
		iowait += mtk_btag_cpu_iowait_time(i);
		irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
	}

	cpu->user = cputime64_to_clock_t(user);
	cpu->nice = cputime64_to_clock_t(nice);
	cpu->system = cputime64_to_clock_t(system);
	cpu->idle = cputime64_to_clock_t(idle);
	cpu->iowait = cputime64_to_clock_t(iowait);
	cpu->irq = cputime64_to_clock_t(irq);
	cpu->softirq = cputime64_to_clock_t(softirq);
}
EXPORT_SYMBOL_GPL(mtk_btag_cpu_eval);

static void mtk_btag_throughput_rw_eval(struct mtk_btag_throughput_rw *rw)
{
	__u64 usage;

	usage = rw->usage;

	do_div(usage, 1000000); /* convert ns to ms */

	if (usage && rw->size) {
		rw->speed = (rw->size) / (__u32)usage;  /* bytes/ms */
		rw->speed = (rw->speed*1000) >> 10; /* KB/s */
		rw->usage = usage;
	} else {
		rw->speed = 0;
		rw->size = 0;
		rw->usage = 0;
	}
}
/* calculate throughput */
void mtk_btag_throughput_eval(struct mtk_btag_throughput *tp)
{
	mtk_btag_throughput_rw_eval(&tp->r);
	mtk_btag_throughput_rw_eval(&tp->w);
}
EXPORT_SYMBOL_GPL(mtk_btag_throughput_eval);

/* print trace to kerne log */
void mtk_btag_print_klog(char **ptr, int *len, struct mtk_btag_trace *tr)
{
	int i, n;

#define boundary_check() { *len -= n; *ptr += n; }

	if (tr->throughput.r.usage) {
		n = snprintf(*ptr, *len, biolog_fmt_rt,
			tr->throughput.r.speed,
			tr->throughput.r.size,
			tr->throughput.r.usage);
		boundary_check();
		if (*len < 0)
			return;
	}

	if (tr->throughput.w.usage) {
		n = snprintf(*ptr, *len, biolog_fmt_wt,
			tr->throughput.w.speed,
			tr->throughput.w.size,
			tr->throughput.w.usage);
		boundary_check();
		if (*len < 0)
			return;
	}

	n = snprintf(*ptr, *len, biolog_fmt,
		tr->workload.percent,
		tr->workload.usage,
		tr->workload.period,
		tr->workload.count,
		tr->vmstat.file_pages,
		tr->vmstat.file_dirty,
		tr->vmstat.dirtied,
		tr->vmstat.writeback,
		tr->vmstat.written,
		tr->cpu.user,
		tr->cpu.nice,
		tr->cpu.system,
		tr->cpu.idle,
		tr->cpu.iowait,
		tr->cpu.irq,
		tr->cpu.softirq,
		tr->pid);
	boundary_check();
	if (*len < 0)
		return;

	for (i = 0; i < MTK_BLOCK_TAG_PIDLOG_ENTRIES; i++) {
		struct mtk_btag_pidlogger_entry *pe;

		pe = &tr->pidlog.info[i];

		if (pe->pid == 0)
			break;

		n = snprintf(*ptr, *len, pidlog_fmt,
			pe->pid,
			pe->w.count,
			pe->w.length,
			pe->r.count,
			pe->r.length);
		boundary_check();
		if (*len < 0)
			return;
	}
}
EXPORT_SYMBOL_GPL(mtk_btag_print_klog);

const char *mtk_btag_pr_time(char *out, int size, const char *str, __u64 t)
{
	uint32_t nsec;

	nsec = do_div(t, 1000000000);
	snprintf(out, size, ",%s=[%lu.%06lu]", str, (unsigned long)t, (unsigned long)nsec/1000);
	return out;
}
EXPORT_SYMBOL_GPL(mtk_btag_pr_time);

const char *mtk_btag_pr_speed(char *out, int size, __u64 usage, __u32 bytes)
{
	__u32 speed;

	if (!usage || !bytes)
		return "";

	do_div(usage, 1000); /* convert ns to us */
	speed = 1000 * bytes / (__u32)usage;  /* bytes/ms */
	speed = (speed*1000) >> 10; /* KB/s */

	snprintf(out, size, ",%u KB/s", speed);
	return out;
}
EXPORT_SYMBOL_GPL(mtk_btag_pr_speed);

void mtk_btag_seq_time(struct seq_file *seq, uint64_t time)
{
	uint32_t nsec;

	nsec = do_div(time, 1000000000);
	seq_printf(seq, "[%5lu.%06lu]", (unsigned long)time, (unsigned long)nsec/1000);
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_time);

void mtk_btag_seq_trace(struct seq_file *seq, struct mtk_btag_trace *tr)
{
	int i;

	if (tr->time <= 0)
		return;

	mtk_btag_seq_time(seq, tr->time);
	seq_printf(seq, "q:%d.", tr->qid);

	if (tr->throughput.r.usage)
		seq_printf(seq, biolog_fmt_rt,
			tr->throughput.r.speed,
			tr->throughput.r.size,
			tr->throughput.r.usage);
	if (tr->throughput.w.usage)
		seq_printf(seq, biolog_fmt_wt,
			tr->throughput.w.speed,
			tr->throughput.w.size,
			tr->throughput.w.usage);

	seq_printf(seq, biolog_fmt,
		tr->workload.percent,
		tr->workload.usage,
		tr->workload.period,
		tr->workload.count,
		tr->vmstat.file_pages,
		tr->vmstat.file_dirty,
		tr->vmstat.dirtied,
		tr->vmstat.writeback,
		tr->vmstat.written,
		tr->cpu.user,
		tr->cpu.nice,
		tr->cpu.system,
		tr->cpu.idle,
		tr->cpu.iowait,
		tr->cpu.irq,
		tr->cpu.softirq,
		tr->pid);

	for (i = 0; i < MTK_BLOCK_TAG_PIDLOG_ENTRIES; i++) {
		struct mtk_btag_pidlogger_entry *pe;

		pe = &tr->pidlog.info[i];

		if (pe->pid == 0)
			break;

		seq_printf(seq, pidlog_fmt,
			pe->pid,
			pe->w.count,
			pe->w.length,
			pe->r.count,
			pe->r.length);
	}
	seq_puts(seq, ".\n");
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_trace);

ssize_t mtk_btag_debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	return count;
}
EXPORT_SYMBOL_GPL(mtk_btag_debug_write);

/* seq file operations */
void *mtk_btag_seq_debug_start(struct seq_file *seq, loff_t *pos)
{
	unsigned int idx;

	if (*pos < 0 || *pos >= 1)
		return NULL;

	idx = *pos + 1;
	return (void *) ((unsigned long) idx);
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_debug_start);

void *mtk_btag_seq_debug_next(struct seq_file *seq, void *v, loff_t *pos)
{
	unsigned int idx;

	++*pos;
	if (*pos < 0 || *pos >= 1)
		return NULL;

	idx = *pos + 1;
	return (void *) ((unsigned long) idx);
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_debug_next);

void mtk_btag_seq_debug_stop(struct seq_file *seq, void *v)
{
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_debug_stop);

struct mtk_btag_ringtrace *mtk_btag_ringtrace_alloc(int size)
{
	struct mtk_btag_ringtrace *rt;

	if (size <= 0)
		return NULL;

	rt = kmalloc(sizeof(struct mtk_btag_ringtrace), GFP_NOFS);

	if (!rt)
		return NULL;

	rt->index = 0;
	rt->max = size;
	rt->trace = kmalloc_array(size,
		sizeof(struct mtk_btag_trace), GFP_NOFS);

	if (!rt->trace) {
		kfree(rt);
		return NULL;
	}

	memset(rt->trace, 0, (sizeof(struct mtk_btag_trace) * size));
	spin_lock_init(&rt->lock);

	return rt;
}
EXPORT_SYMBOL_GPL(mtk_btag_ringtrace_alloc);

void mtk_btag_ringtrace_free(struct mtk_btag_ringtrace *rt)
{
	if (!rt)
		return;

	kfree(rt->trace);
	kfree(rt);
}
EXPORT_SYMBOL_GPL(mtk_btag_ringtrace_free);

/* get current trace in debugfs ring buffer */
struct mtk_btag_trace *mtk_btag_curr_trace(struct mtk_btag_ringtrace *rt)
{
	if (rt)
		return &rt->trace[rt->index];
	else
		return NULL;
}
EXPORT_SYMBOL_GPL(mtk_btag_curr_trace);

/* step to next trace in debugfs ring buffer */
struct mtk_btag_trace *mtk_btag_next_trace(struct mtk_btag_ringtrace *rt)
{
	if (rt->index >= rt->max)
		rt->index = 0;
	else
		rt->index++;

	return mtk_btag_curr_trace(rt);
}
EXPORT_SYMBOL_GPL(mtk_btag_next_trace);

void mtk_btag_seq_debug_show_ringtrace(struct seq_file *seq,
	struct mtk_btag_ringtrace *rt)
{
	unsigned long flags;
	int i, end;

	if (!rt)
		return;

	if (rt->index >= rt->max || rt->index < 0)
		rt->index = 0;

	spin_lock_irqsave(&rt->lock, flags);
	end = (rt->index > 0) ? rt->index-1 : rt->max-1;
	for (i = rt->index;;) {
		mtk_btag_seq_trace(seq, &rt->trace[i]);
		if (i == end)
			break;
		i = (i >= rt->max-1) ? 0 : i+1;
	};
	spin_unlock_irqrestore(&rt->lock, flags);
}
EXPORT_SYMBOL_GPL(mtk_btag_seq_debug_show_ringtrace);

static int __init mtk_btag_early_memory_info(void)
{
	phys_addr_t start, end;

	start = memblock_start_of_DRAM();
	end = memblock_end_of_DRAM();
	mtk_btag_system_dram_size = (unsigned long long)(end - start);
	pr_debug("[BLOCK_TAG] DRAM: %pa - %pa, size: 0x%llx\n", &start,
		&end, (unsigned long long)(end - start));
	return 0;
}
fs_initcall(mtk_btag_early_memory_info);

static unsigned long mtk_btag_pidlogger_init(void)
{
	unsigned long count = mtk_btag_system_dram_size >> PAGE_SHIFT;
	unsigned long size = count * sizeof(struct page_pid_logger);

	if (mtk_btag_pagelogger)
		goto init;

	spin_lock_init(&mtk_btag_pagelogger_lock);

#ifdef CONFIG_MTK_EXTMEM
	mtk_btag_pagelogger = extmem_malloc_page_align(size);
#else
	mtk_btag_pagelogger = vmalloc(size);
#endif

init:
	if (mtk_btag_pagelogger) {
		memset(mtk_btag_pagelogger, -1, size);
		return size;
	}
	pr_warn("[BLOCK_TAG] blockio: fail to allocate mtk_btag_pagelogger\n");
	return 0;
}


static int __init mtk_btag_init(void)
{
	mtk_btag_used_mem = mtk_btag_pidlogger_init();
	if (!mtk_btag_used_mem)
		return 0;

	mtk_btag_droot = debugfs_create_dir("blocktag", NULL);

	if (IS_ERR(mtk_btag_droot)) {
		pr_warn("[BLOCK_TAG] fail to create debugfs root: blocktag\n");
		return 0;
	}

	mtk_btag_dmem = debugfs_create_u32("used_mem", 0440, mtk_btag_droot, &mtk_btag_used_mem);

	if (IS_ERR(mtk_btag_dmem))
		pr_warn("[BLOCK_TAG] blockio: fail to create used_mem at debugfs\n");

	return 0;
}

static void __exit mtk_btag_exit(void)
{
}

module_init(mtk_btag_init);
module_exit(mtk_btag_exit);

MODULE_AUTHOR("Perry Hsu <perry.hsu@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Storage Block Tag Trace");

