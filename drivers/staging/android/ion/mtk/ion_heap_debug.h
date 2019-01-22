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
#ifndef __ION_HEAP_DEBUG_H__
#define __ION_HEAP_DEBUG_H__

#define ION_PRINT_LOG_OR_SEQ(seq_file, fmt, args...) \
do {\
	if (seq_file)\
		seq_printf(seq_file, fmt, ##args);\
	else\
		pr_info(fmt, ##args);\
} while (0)

struct ion_mm_buffer_info {
	struct mutex lock;/*mutex lock on secure buffer*/
	int module_id;
	int fix_module_id;
	unsigned int security;
	unsigned int coherent;
	void *VA;
	unsigned int MVA;
	unsigned int FIXED_MVA;
	unsigned int iova_start;
	unsigned int iova_end;
	ion_phys_addr_t priv_phys;
	struct ion_mm_buf_debug_info dbg_info;
	ion_mm_buf_destroy_callback_t *destroy_fn;
	pid_t pid;
};

static const unsigned int orders[] = {4, 1, 0 };
/* static const unsigned int orders[] = {8, 4, 0}; */
static const int num_orders = ARRAY_SIZE(orders);

int ion_heap_debug_show(struct ion_heap *heap, struct seq_file *s, void *unused);
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
