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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_ftl

#if !defined(_TRACE_MT_FTL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MT_FTL_H

#include <linux/tracepoint.h>

TRACE_EVENT(mt_ftl_write,
	TP_PROTO(const struct mt_ftl_blk *dev, sector_t sector, int len),

	TP_ARGS(dev, sector, len),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(sector_t, sector)
		__field(int,	len)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->sector = sector;
		__entry->len = len;
	),

	TP_printk("ubi%d sector %llu len %d",
		__entry->ubi_num,
		(unsigned long long)__entry->sector,
		__entry->len)
);

TRACE_EVENT(mt_ftl_read,
	TP_PROTO(const struct mt_ftl_blk *dev, sector_t sector, int len),

	TP_ARGS(dev, sector, len),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(sector_t, sector)
		__field(int,	len)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->sector = sector;
		__entry->len = len;
	),

	TP_printk("ubi%d sector %llu len %d",
		__entry->ubi_num,
		(unsigned long long)__entry->sector,
		__entry->len)
);

TRACE_EVENT(mt_ftl_leb_write,
	TP_PROTO(const struct mt_ftl_blk *dev, int lnum, int offset, int len),

	TP_ARGS(dev, lnum, offset, len),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	lnum)
		__field(int,	offset)
		__field(int,	len)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->lnum = lnum;
		__entry->offset = offset;
		__entry->len = len;
	),

	TP_printk("ubi%d lnum %d offset %d len %d",
		__entry->ubi_num,
		__entry->lnum,
		__entry->offset,
		__entry->len)
	);

TRACE_EVENT(mt_ftl_leb_read,
	TP_PROTO(const struct mt_ftl_blk *dev, int lnum, int offset, int len),

	TP_ARGS(dev, lnum, offset, len),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	lnum)
		__field(int,	offset)
		__field(int,	len)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->lnum = lnum;
		__entry->offset = offset;
		__entry->len = len;
	),

	TP_printk("ubi%d lnum %d offset %d len %d",
		__entry->ubi_num,
		__entry->lnum,
		__entry->offset,
		__entry->len)
);

TRACE_EVENT(mt_ftl_commit,
	TP_PROTO(const struct mt_ftl_blk *dev),

	TP_ARGS(dev),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
	),

	TP_printk("ubi%d",
		__entry->ubi_num)
);

TRACE_EVENT(mt_ftl_commit_done,
	TP_PROTO(const struct mt_ftl_blk *dev, int ret),

	TP_ARGS(dev, ret),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	ret)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->ret = ret;
	),

	TP_printk("ubi%d %d",
		__entry->ubi_num,
		__entry->ret)
);

TRACE_EVENT(mt_ftl_commitPMT,
	TP_PROTO(const struct mt_ftl_blk *dev, int leb, bool replay, bool commit),

	TP_ARGS(dev, leb, replay, commit),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	leb)
		__field(bool,	replay)
		__field(bool,	commit)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->leb = leb;
		__entry->replay = replay;
		__entry->commit = commit;
	),

	TP_printk("ubi%d leb %d replay %d commit %d",
		__entry->ubi_num,
		__entry->leb,
		__entry->replay,
		__entry->commit)
);

TRACE_EVENT(mt_ftl_discard,
	TP_PROTO(const struct mt_ftl_blk *dev, sector_t sector, unsigned int nr_sec),

	TP_ARGS(dev, sector, nr_sec),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(sector_t, sector)
		__field(int,	nr_sec)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->sector = sector;
		__entry->nr_sec = nr_sec;
	),

	TP_printk("ubi%d sector %llu nr_sec %d",
		__entry->ubi_num,
		(unsigned long long)__entry->sector,
		__entry->nr_sec)
);

TRACE_EVENT(mt_ftl_write_page,
	TP_PROTO(const struct mt_ftl_blk *dev),

	TP_ARGS(dev),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
	),

	TP_printk("ubi%d",
		__entry->ubi_num)
);

TRACE_EVENT(mt_ftl_forced_flush,
	TP_PROTO(const struct mt_ftl_blk *dev),

	TP_ARGS(dev),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
	),

	TP_printk("ubi%d",
		__entry->ubi_num)
);

TRACE_EVENT(mt_ftl_get_free_block,
	TP_PROTO(const struct mt_ftl_blk *dev, int leb, int page),

	TP_ARGS(dev, leb, page),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	leb)
		__field(int,	page)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->leb = leb;
		__entry->page = page;
	),

	TP_printk("ubi%d leb %d page %d",
		__entry->ubi_num,
		__entry->leb,
		__entry->page)
);

TRACE_EVENT(mt_ftl_gc,
	TP_PROTO(const struct mt_ftl_blk *dev, bool ispmt, bool replay),

	TP_ARGS(dev, ispmt, replay),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(bool,	ispmt)
		__field(bool,	replay)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->ispmt = ispmt;
		__entry->replay = replay;
	),

	TP_printk("ubi%d ispmt %d replay %d",
		__entry->ubi_num,
		__entry->ispmt,
		__entry->replay)
);

TRACE_EVENT(mt_ftl_gc_done,
	TP_PROTO(const struct mt_ftl_blk *dev, int leb, int page, int src_leb),

	TP_ARGS(dev, leb, page, src_leb),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	leb)
		__field(int,	page)
		__field(int,	src_leb)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->leb = leb;
		__entry->page = page;
		__entry->src_leb = src_leb;
	),

	TP_printk("ubi%d leb %d page %d src_leb %d",
		__entry->ubi_num,
		__entry->leb,
		__entry->page,
		__entry->src_leb)
);

TRACE_EVENT(mt_ftl_gc_error,
	TP_PROTO(const struct mt_ftl_blk *dev, int err),

	TP_ARGS(dev, err),

	TP_STRUCT__entry(
		__field(int,	ubi_num)
		__field(int,	err)
	),

	TP_fast_assign(
		__entry->ubi_num = dev->ubi_num;
		__entry->err = err;
	),

	TP_printk("ubi%d err %d",
		__entry->ubi_num,
		__entry->err)
);

#endif /* _TRACE_MT_FTL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
