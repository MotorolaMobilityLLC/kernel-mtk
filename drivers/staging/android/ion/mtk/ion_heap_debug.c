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
#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/fdtable.h>
#include <linux/mutex.h>
#include <mmprofile.h>
#include <mmprofile_function.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include "mtk/mtk_ion.h"
#include "ion_profile.h"
#include "ion_drv_priv.h"
#include "ion_priv.h"
#include "mtk/ion_drv.h"
#include "ion_heap_debug.h"

struct dump_fd_data {
	struct task_struct *p;
	struct seq_file *s;
};

struct ion_system_heap {
	struct ion_heap heap;
	struct ion_page_pool **pools;
	struct ion_page_pool **cached_pools;
};

static int __do_dump_share_fd(const void *data, struct file *file, unsigned fd)
{
	const struct dump_fd_data *d = data;
	struct seq_file *s = d->s;
	struct task_struct *p = d->p;
	struct ion_buffer *buffer;
	struct ion_mm_buffer_info *bug_info;

	buffer = ion_drv_file_to_buffer(file);
	if (IS_ERR_OR_NULL(buffer))
		return 0;

	bug_info = (struct ion_mm_buffer_info *)buffer->priv_virt;
	if (!buffer->handle_count)
		ION_PRINT_LOG_OR_SEQ(s, "0x%p %9d %16s %5d %5d %16s %4d\n",
				     buffer, bug_info->pid, buffer->alloc_dbg, p->pid, p->tgid, p->comm, fd);

	return 0;
}

static int ion_dump_all_share_fds(struct seq_file *s)
{
	struct task_struct *p;
	int res;
	struct dump_fd_data data;

	/* function is not available, just return */
	if (ion_drv_file_to_buffer(NULL) == ERR_PTR(-EPERM))
		return 0;

	ION_PRINT_LOG_OR_SEQ(s, "%18s %9s %16s %5s %5s %16s %4s\n",
			     "buffer", "alloc_pid", "alloc_client", "pid", "tgid", "process", "fd");
	data.s = s;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		task_lock(p);
		data.p = p;
		res = iterate_fd(p->files, 0, __do_dump_share_fd, &data);
		if (res)
			IONMSG("%s failed somehow\n", __func__);
		task_unlock(p);
	}
	read_unlock(&tasklist_lock);
	return 0;
}

int ion_heap_debug_show(struct ion_heap *heap, struct seq_file *s, void *unused)
{
	struct ion_device *dev = heap->dev;
	struct rb_node *n;
	struct rb_node *m;
	bool has_orphaned = false;
	struct ion_mm_buffer_info *bug_info;
	struct ion_mm_buf_debug_info *pdbg;
	unsigned long long current_ts;
	size_t fr_sz = 0;
	size_t sec_sz = 0;
	size_t prot_sz = 0;
	size_t cam_sz = 0;
	size_t va2mva_sz = 0;
	size_t mm_sz = 0;

	if (heap->type == (int)ION_HEAP_TYPE_MULTIMEDIA) {
		int i;
		struct ion_system_heap
		*sys_heap = container_of(heap, struct ion_system_heap, heap);

		for (i = 0; i < num_orders; i++) {
			struct ion_page_pool *pool = sys_heap->pools[i];

			ION_PRINT_LOG_OR_SEQ(s,
					     "%d order %u highmem pages in pool = %lu total, dev, 0x%p, heap id: %d\n",
					     pool->high_count, pool->order,
					     (1 << pool->order) * PAGE_SIZE *
					     pool->high_count, dev, heap->id);
			ION_PRINT_LOG_OR_SEQ(s,
					     "%d order %u lowmem pages in pool = %lu total\n",
					     pool->low_count, pool->order,
					     (1 << pool->order) * PAGE_SIZE *
					     pool->low_count);
			pool = sys_heap->cached_pools[i];
			ION_PRINT_LOG_OR_SEQ(s,
					     "%d order %u highmem pages in cached_pool = %lu total\n",
					     pool->high_count, pool->order,
					     (1 << pool->order) * PAGE_SIZE *
					     pool->high_count);
			ION_PRINT_LOG_OR_SEQ(s,
					     "%d order %u lowmem pages in cached_pool = %lu total\n",
					     pool->low_count, pool->order,
					     (1 << pool->order) * PAGE_SIZE *
					     pool->low_count);
		}
		if (heap->flags & ION_HEAP_FLAG_DEFER_FREE)
			ION_PRINT_LOG_OR_SEQ(s, "mm_heap_freelist total_size=%zu\n",
					     ion_heap_freelist_size(heap));
		else
			ION_PRINT_LOG_OR_SEQ(s, "mm_heap defer free disabled\n");
	}
	ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
	ION_PRINT_LOG_OR_SEQ(s,
			     "%18.s %8.s %4.s %3.s %3.s %3.s %7.s %3.s %4.s %4.s %s %s %4.s %4.s %4.s %4.s %s\n",
			     "buffer", "size", "kmap", "ref", "hdl", "mod", "mva", "sec",
			     "flag", "heap_id", "pid(alloc_pid)", "comm(client)", "v1", "v2", "v3", "v4", "dbg_name");

	mutex_lock(&dev->buffer_lock);
	for (n = rb_first(&dev->buffers); n; n = rb_next(n)) {
		struct ion_buffer
		*buffer = rb_entry(n, struct ion_buffer, node);
		int port = 0;
		unsigned int mva = 0;

		bug_info = (struct ion_mm_buffer_info *)buffer->priv_virt;
		port = bug_info->module_id ? bug_info->module_id : bug_info->fix_module_id;
		mva = bug_info->MVA ? bug_info->MVA : bug_info->FIXED_MVA;
		pdbg = &bug_info->dbg_info;

		ION_PRINT_LOG_OR_SEQ(s,
				     "0x%p %8zu %3d %3d %3d %3d %8x %3u %3lu %3d %5d(%5d) %16s %d %d  %d  %d  %s\n",
				     buffer, buffer->size, buffer->kmap_cnt,
				     atomic_read(&buffer->ref.refcount), buffer->handle_count,
				     port, mva, bug_info->security,
				     buffer->flags, buffer->heap->id, buffer->pid, bug_info->pid, buffer->task_comm,
				     pdbg->value1, pdbg->value2, pdbg->value3,
				     pdbg->value4, pdbg->dbg_name);

		if (heap->type == (int)ION_HEAP_TYPE_MULTIMEDIA_SEC) {
			if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA_SEC)
				sec_sz += buffer->size;
			else if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA_PROT)
				prot_sz += buffer->size;
			else if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA_2D_FR)
				fr_sz += buffer->size;
		} else {
			if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA)
				mm_sz += buffer->size;
			else if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA_FOR_CAMERA)
				cam_sz += buffer->size;
			else if (buffer->heap->id == ION_HEAP_TYPE_MULTIMEDIA_MAP_MVA)
				va2mva_sz += buffer->size;
		}

		if (!buffer->handle_count)
			has_orphaned = true;
	}

	if (has_orphaned) {
		ION_PRINT_LOG_OR_SEQ(s, "-----orphaned buffer list:------------------\n");
		ion_dump_all_share_fds(s);
	}

	mutex_unlock(&dev->buffer_lock);

	ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
	if (heap->type == (int)ION_HEAP_TYPE_MULTIMEDIA_SEC) {
		ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "sec-sz:", sec_sz);
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "prot-sz:", prot_sz);
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "2d-fr-sz:", fr_sz);
		ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
	} else {
		ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "mm-sz:", mm_sz);
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "cam-sz:", cam_sz);
		ION_PRINT_LOG_OR_SEQ(s, "%16s %16zu\n", "va2mva-sz:", va2mva_sz);
		ION_PRINT_LOG_OR_SEQ(s, "----------------------------------------------------\n");
	}

	/* dump all handle's backtrace */
	down_read(&dev->lock);
	for (n = rb_first(&dev->clients); n; n = rb_next(n)) {
		struct ion_client
		*client = rb_entry(n, struct ion_client, node);

		if (client->task) {
			char task_comm[TASK_COMM_LEN];

			get_task_comm(task_comm, client->task);
			ION_PRINT_LOG_OR_SEQ(s,
					     "client(0x%p) %s (%s) pid(%u) ================>\n",
					client, task_comm, client->dbg_name, client->pid);
		} else {
			ION_PRINT_LOG_OR_SEQ(s,
					     "client(0x%p) %s (from_kernel) pid(%u) ================>\n",
					client, client->name, client->pid);
		}

		mutex_lock(&client->lock);
		for (m = rb_first(&client->handles); m; m = rb_next(m)) {
			struct ion_handle
			*handle = rb_entry(m, struct ion_handle, node);

			if (handle->buffer->heap->type != (int)heap->type)
				continue;

			ION_PRINT_LOG_OR_SEQ(s,
					     "\thandle=0x%p, buf=0x%p, heap=%d, fd=%4d, ts: %lldms\n",
					     handle, handle->buffer, handle->buffer->heap->id,
					     handle->dbg.fd, handle->dbg.user_ts);
		}
		mutex_unlock(&client->lock);
	}
	current_ts = sched_clock();
	do_div(current_ts, 1000000);
	up_read(&dev->lock);

	return 0;
}
