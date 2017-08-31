/*
 * Copyright (C) 2017 MediaTek Inc.
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
#define RAW_PART_READ 0
#define RAW_PART_READ_SECTOR 1
#define RAW_PART_READ_CACHE 2
#define RAW_PART_SLC_WRITE 3
#define RAW_PART_TLC_BLOCK_ACH 4
#define RAW_PART_TLC_BLOCK_SW 5
#define RAW_PART_ERASE 6
#define MNTL_PART_READ 7
#define MNTL_PART_READ_SECTOR 8
#define MNTL_PART_SLC_WRITE 9
#define MNTL_PART_TLC_BLOCK_ACH 10
#define MNTL_PART_TLC_BLOCK_SW 11
#define MNTL_PART_ERASE 12
#define MAX_PROF_ITEM 13

#ifdef __D_PFM__

#ifndef MODULE_NAME
#define MODULE_NAME	"NAND"
#endif

#include <linux/time.h>

struct prof_struct {
	u64 valid;
	u64 op_time;
	u64 op_length;
};

extern struct timeval g_now;
extern struct timeval g_begin;
extern u32 idx_prof;
extern struct prof_struct g_pf_struct[15];
extern char *prof_str[13];


#define PFM_INIT() memset(g_pf_struct, 0, sizeof(g_pf_struct))

#define PFM_BEGIN() do_gettimeofday(&g_begin)

#define PFM_END_OP(n, p) \
do { \
	do_gettimeofday(&g_now); \
	g_pf_struct[n].op_time += (g_now.tv_sec * 1000000 + g_now.tv_usec) - \
		(g_begin.tv_sec * 1000000 + g_begin.tv_usec); \
	g_pf_struct[n].op_length += p; \
	g_pf_struct[n].valid += 1; \
} while (0)

#define PFM_DUMP_D(m) \
do { \
	for (idx_prof = 0; idx_prof < MAX_PROF_ITEM; idx_prof++) { \
		if (m) { \
			seq_printf(m, \
			"%s\titem\t%s -\ttakse:\t%lld\tusec\t,\tdata length:\t%lld\tbytes, call times #\t%lld\n", \
			MODULE_NAME, prof_str[idx_prof], g_pf_struct[idx_prof].op_time, \
			g_pf_struct[idx_prof].op_length, g_pf_struct[idx_prof].valid); \
		} else { \
			pr_info( \
			"%s\titem\t%s -\ttakse:\t%lld\tusec\t,\tdata length:\t%lld\tbytes, call times #\t%lld\n", \
			MODULE_NAME, prof_str[idx_prof], g_pf_struct[idx_prof].op_time, \
			g_pf_struct[idx_prof].op_length, g_pf_struct[idx_prof].valid); \
		} \
	} \
} while (0)

#else
#define PFM_INIT()
#define PFM_BEGIN()
#define PFM_END_OP(n, p)
#define PFM_DUMP_D(m)

#endif

#ifdef __REPLAY_CALL__
struct call_sequence {
	u8 op_type;
	u32 block;
	u32 page;
	u32 *call_address;
	u32 times;
};

extern struct call_sequence call_trace[4096];
extern u32 call_idx, temp_idx;
extern bool record_en;
extern char *call_str[6];
extern bool mntl_record_en;

#define CALL_TRACE_INIT() \
do { \
	memset(call_trace, 0, sizeof(call_trace));\
	call_idx = 0; \
	record_en = FALSE; \
	mntl_record_en = TRUE; \
} while (0)

#define CALL_TRACE(m) \
do { \
	if (record_en) { \
		call_trace[call_idx].call_address = __builtin_return_address(0); \
		call_trace[call_idx].op_type = m; \
		call_idx = (call_idx + 1) % 4096; \
	} \
} while (0)

#define DUMP_CALL_TRACE(m) \
do { \
	record_en = FALSE; \
	mntl_record_en = FALSE; \
	for (temp_idx = call_idx + 1 ; temp_idx != call_idx; (temp_idx = (temp_idx + 1) % 4096)) { \
		if (!call_trace[temp_idx].op_type) \
			continue; \
		if (m) { \
			seq_printf(m, \
			"%s\tcall times\t%d\tcaller:\t%pS\tblock\t%d\tpage\t%d\n", \
			call_str[call_trace[temp_idx].op_type], call_trace[temp_idx].times, \
			call_trace[temp_idx].call_address, call_trace[temp_idx].block, \
			call_trace[temp_idx].page); \
		} else { \
			pr_info( \
			"%s\tcall times\t%d\tcaller:\t%pS\tblock\t%d\tpage\t%d\n", \
			call_str[call_trace[temp_idx].op_type], call_trace[temp_idx].times, \
			call_trace[temp_idx].call_address, call_trace[temp_idx].block, \
			call_trace[temp_idx].page); \
		} \
	} \
} while (0)
#else
#define CALL_TRACE_INIT()
#define CALL_TRACE(m)
#define DUMP_CALL_TRACE(m)

#endif
