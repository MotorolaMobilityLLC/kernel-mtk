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

/*
 * ISP_Pass1_CAM:
 * Define 2 ftrace event:
 *        1. enter event
 *        2. leave event
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM met_events_camsys
#if !defined(_TRACE_CAMSYS_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CAMSYS_EVENTS_H
#include <linux/tracepoint.h>

TRACE_EVENT(ISP_Pass1_CAM_enter,
	TP_PROTO(int imgo_en, int rrzo_en, int imgo_bpp, int rrzo_bpp,
		int imgo_w_in_byte, int imgo_h_in_byte, int rrzo_w_in_byte,
		int rrzo_h_in_byte, int rrz_src_w, int rrz_src_h, int rrz_dst_w,
		int rrz_dst_h, int rrz_hori_step, int rrz_vert_step),
	TP_ARGS(imgo_en, rrzo_en, imgo_bpp, rrzo_bpp, imgo_w_in_byte,
		imgo_h_in_byte, rrzo_w_in_byte, rrzo_h_in_byte,
		rrz_src_w, rrz_src_h, rrz_dst_w, rrz_dst_h,
		rrz_hori_step, rrz_vert_step),
	TP_STRUCT__entry(
		__field(int, imgo_en)
		__field(int, rrzo_en)
		__field(int, imgo_bpp)
		__field(int, rrzo_bpp)
		__field(int, imgo_w_in_byte)
		__field(int, imgo_h_in_byte)
		__field(int, rrzo_w_in_byte)
		__field(int, rrzo_h_in_byte)
		__field(int, rrz_src_w)
		__field(int, rrz_src_h)
		__field(int, rrz_dst_w)
		__field(int, rrz_dst_h)
		__field(int, rrz_hori_step)
		__field(int, rrz_vert_step)
	),
	TP_fast_assign(
		__entry->imgo_en = imgo_en;
		__entry->rrzo_en = rrzo_en;
		__entry->imgo_bpp = imgo_bpp;
		__entry->rrzo_bpp = rrzo_bpp;
		__entry->imgo_w_in_byte = imgo_w_in_byte;
		__entry->imgo_h_in_byte = imgo_h_in_byte;
		__entry->rrzo_w_in_byte = rrzo_w_in_byte;
		__entry->rrzo_h_in_byte = rrzo_h_in_byte;
		__entry->rrz_src_w = rrz_src_w;
		__entry->rrz_src_h = rrz_src_h;
		__entry->rrz_dst_w = rrz_dst_w;
		__entry->rrz_dst_h = rrz_dst_h;
		__entry->rrz_hori_step = rrz_hori_step;
		__entry->rrz_vert_step = rrz_vert_step;
	),
	TP_printk("imgo_en=%d, rrzo_en=%d, imgo_bpp=%d, rrzo_bpp=%d,\n"
		"imgo_xsize=%d, imgo_ysize=%d, rrzo_xsize=%d, rrzo_ysize=%d,\n"
		"rrz_src_w=%d, rrz_src_h=%d, rrz_dst_w=%d, rrz_dst_h=%d,\n"
		"rrz_hori_step=%d, rrz_vert_step=%d\n",
		__entry->imgo_en,
		__entry->rrzo_en,
		__entry->imgo_bpp,
		__entry->rrzo_bpp,
		__entry->imgo_w_in_byte,
		__entry->imgo_h_in_byte,
		__entry->rrzo_w_in_byte,
		__entry->rrzo_h_in_byte,
		__entry->rrz_src_w,
		__entry->rrz_src_h,
		__entry->rrz_dst_w,
		__entry->rrz_dst_h,
		__entry->rrz_hori_step,
		__entry->rrz_vert_step)
);


TRACE_EVENT(ISP_Pass1_CAM_leave,
	TP_PROTO(int dummy),
	TP_ARGS(dummy),
	TP_STRUCT__entry(
	__field(int, dummy)
	),
	TP_fast_assign(
		__entry->dummy  = dummy;
	),
	TP_printk("%s", "")
);


#endif /* _TRACE_CAMSYS_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ./inc
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE met_events_camsys
#include <trace/define_trace.h>

