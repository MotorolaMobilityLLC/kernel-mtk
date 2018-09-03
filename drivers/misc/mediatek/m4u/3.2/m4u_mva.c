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

#include <linux/spinlock.h>
#include "m4u_priv.h"

/* ((va&0xfff)+size+0xfff)>>12 */
#define mva_pageOffset(mva) ((mva)&0xfff)

#define MVA_BLOCK_SIZE_ORDER     20	/* 1M */
#define MVA_MAX_BLOCK_NR        4095	/* 4GB */

#define MVA_BLOCK_SIZE      (1<<MVA_BLOCK_SIZE_ORDER)	/* 0x40000 */
#define MVA_BLOCK_ALIGN_MASK (MVA_BLOCK_SIZE-1)	/* 0x3ffff */
#define MVA_BLOCK_NR_MASK   (MVA_MAX_BLOCK_NR)	/* 0xfff */
#define MVA_BUSY_MASK       (1<<15)	/* 0x8000 */

#define MVA_IS_BUSY(domain_idx, index) \
		((mvaGraph[domain_idx][index]&MVA_BUSY_MASK) != 0)
#define MVA_SET_BUSY(domain_idx, index) \
		(mvaGraph[domain_idx][index] |= MVA_BUSY_MASK)
#define MVA_SET_FREE(domain_idx, index) \
		(mvaGraph[domain_idx][index] & (~MVA_BUSY_MASK))
#define MVA_GET_NR(domain_idx, index)   \
		(mvaGraph[domain_idx][index] & MVA_BLOCK_NR_MASK)

#define MVAGRAPH_INDEX(mva) (mva>>MVA_BLOCK_SIZE_ORDER)

static short mvaGraph[TOTAL_M4U_NUM][MVA_MAX_BLOCK_NR + 1];
static void *mvaInfoGraph[TOTAL_M4U_NUM][MVA_MAX_BLOCK_NR + 1];
/*just be used for single spinlock lock 2 graph*/
static DEFINE_SPINLOCK(gMvaGraph_lock);
/*be used for m4uGraph0*/
static DEFINE_SPINLOCK(gMvaGraph_lock0);
/*be used for m4uGraph1*/
static DEFINE_SPINLOCK(gMvaGraph_lock1);
enum graph_lock_tpye {
	SPINLOCK_MVA_GRAPH0,
	SPINLOCK_MVA_GRAPH1,
	SPINLOCK_COMMON,
	SPINLOCK_INVAILD
};

/*according to lock type, get mva graph lock.*/
static spinlock_t *get_mva_graph_lock(enum graph_lock_tpye type)
{
	switch (type) {
	case SPINLOCK_MVA_GRAPH0:
		return &gMvaGraph_lock0;
	case SPINLOCK_MVA_GRAPH1:
		return &gMvaGraph_lock1;
	case SPINLOCK_COMMON:
		return &gMvaGraph_lock;
	default:
		M4UMSG(
			"fatal error: invalid mva graph lock type(%d)!\n",
				(int)type);
		return NULL;
	}
}

void m4u_mvaGraph_init(void *priv_reserve, int domain_idx)
{
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	spin_lock_irqsave(mva_graph_lock, irq_flags);
	memset(&mvaGraph[domain_idx], 0,
			sizeof(short) * (MVA_MAX_BLOCK_NR + 1));
	memset(mvaInfoGraph[domain_idx], 0,
			sizeof(void *) * (MVA_MAX_BLOCK_NR + 1));
	mvaGraph[domain_idx][0] = (short)(1 | MVA_BUSY_MASK);
	mvaInfoGraph[domain_idx][0] = priv_reserve;
	mvaGraph[domain_idx][1] = MVA_MAX_BLOCK_NR;
	mvaInfoGraph[domain_idx][1] = priv_reserve;
	mvaGraph[domain_idx][MVA_MAX_BLOCK_NR] = MVA_MAX_BLOCK_NR;
	mvaInfoGraph[domain_idx][MVA_MAX_BLOCK_NR] = priv_reserve;

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
}

void m4u_mvaGraph_dump_raw(void)
{
	int i, j;
	unsigned long irq_flags;
	spinlock_t *mva_graph_lock = get_mva_graph_lock(SPINLOCK_COMMON);

	spin_lock_irqsave(mva_graph_lock, irq_flags);
	M4ULOG_HIGH("[M4U_K] dump raw data of mvaGraph:============>\n");
	for (i = 0; i < MVA_DOMAIN_NR; i++)
		for (j = 0; j < MVA_MAX_BLOCK_NR + 1; j++)
			M4ULOG_HIGH("0x%4x: 0x%08x\n", i, mvaGraph[i][j]);
	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
}

void m4u_mvaGraph_dump(unsigned int domain_idx)
{
	unsigned int addr = 0, size = 0;
	unsigned short index = 1, nr = 0;
	int i, max_bit, is_busy;
	short frag[12] = { 0 };
	short nr_free = 0, nr_alloc = 0;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	M4ULOG_HIGH(
		"[M4U_K] mva allocation info dump: domain=%u ==================>\n",
		domain_idx);
	M4ULOG_HIGH("start      size     blocknum    busy\n");

	spin_lock_irqsave(mva_graph_lock, irq_flags);
	for (index = 1; index < MVA_MAX_BLOCK_NR + 1; index += nr) {
		addr = index << MVA_BLOCK_SIZE_ORDER;
		nr = MVA_GET_NR(domain_idx, index);
		size = nr << MVA_BLOCK_SIZE_ORDER;
		if (MVA_IS_BUSY(domain_idx, index)) {
			is_busy = 1;
			nr_alloc += nr;
		} else {		/* mva region is free */
			is_busy = 0;
			nr_free += nr;

			max_bit = 0;
			for (i = 0; i < 12; i++) {
				if (nr & (1 << i))
					max_bit = i;
			}
			frag[max_bit]++;
		}

		M4ULOG_HIGH("0x%08x  0x%08x  %4d    %d\n",
			addr, size, nr, is_busy);
	}

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);

	M4ULOG_HIGH("\n");
	M4ULOG_HIGH(
		"[M4U_K] mva alloc summary: (unit: blocks)========================>\n");
	M4ULOG_HIGH(
		"free: %d , alloc: %d, total: %d\n",
			nr_free, nr_alloc, nr_free + nr_alloc);
	M4ULOG_HIGH(
		"[M4U_K] free region fragments in 2^x blocks unit:===============\n");
	M4ULOG_HIGH(
		"  0     1     2     3     4     5     6     7     8     9     10    11\n");
	M4ULOG_HIGH(
		"%4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d\n",
			frag[0], frag[1], frag[2], frag[3],
			frag[4], frag[5], frag[6],
			frag[7], frag[8], frag[9], frag[10], frag[11]);
	M4ULOG_HIGH(
		"[M4U_K] mva alloc dump done=========================<\n");
}

void *mva_get_priv_ext(unsigned int domain_idx, unsigned int mva)
{
	void *priv = NULL;
	unsigned int index;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return NULL;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	index = MVAGRAPH_INDEX(mva);
	if (index == 0 || index > MVA_MAX_BLOCK_NR) {
		M4UMSG("mvaGraph index is 0. mva=0x%x, domain=%u\n",
			mva, domain_idx);
		return NULL;
	}

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	/* find prev head/tail of this region */
	while (mvaGraph[domain_idx][index] == 0)
		index--;

	if (MVA_IS_BUSY(domain_idx, index))
		priv = mvaInfoGraph[domain_idx][index];

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
	return priv;
}

int mva_foreach_priv(mva_buf_fn_t *fn, void *data,
		unsigned int domain_idx)
{
	unsigned short index = 1, nr = 0;
	unsigned int mva;
	void *priv;
	unsigned long irq_flags;
	int ret;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return -1;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	for (index = 1; index < MVA_MAX_BLOCK_NR + 1; index += nr) {
		mva = index << MVA_BLOCK_SIZE_ORDER;
		nr = MVA_GET_NR(domain_idx, index);
		if (MVA_IS_BUSY(domain_idx, index)) {
			priv = mvaInfoGraph[domain_idx][index];
			ret = fn(priv, mva, mva + nr * MVA_BLOCK_SIZE, data);
			if (ret)
				break;
		}
	}

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
	return 0;
}

unsigned int get_first_valid_mva(unsigned int domain_idx)
{
	unsigned short index = 1, nr = 0;
	unsigned int mva;
	void *priv;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return 0;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	for (index = 1; index < MVA_MAX_BLOCK_NR + 1; index += nr) {
		mva = index << MVA_BLOCK_SIZE_ORDER;
		nr = MVA_GET_NR(domain_idx, index);
		if (MVA_IS_BUSY(domain_idx, index)) {
			priv = mvaInfoGraph[domain_idx][index];
			break;
		}
	}

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
	return mva;
}


void *mva_get_priv(unsigned int mva, unsigned int domain_idx)
{
	void *priv = NULL;
	unsigned int index;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
					__func__, domain_idx);
		return 0;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	index = MVAGRAPH_INDEX(mva);
	if (index == 0 || index > MVA_MAX_BLOCK_NR) {
		M4UMSG("mvaGraph index is 0. mva=0x%x, domain=%u\n",
			mva, domain_idx);
		return NULL;
	}

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	if (MVA_IS_BUSY(domain_idx, index))
		priv = mvaInfoGraph[domain_idx][index];

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);
	return priv;
}

unsigned int m4u_do_mva_alloc(unsigned int domain_idx,
	unsigned long va, unsigned int size, void *priv)
{
	unsigned short s, end;
	unsigned short new_start, new_end;
	unsigned short nr = 0;
	unsigned int mvaRegionStart;
	unsigned long startRequire, endRequire, sizeRequire;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
					__func__, domain_idx);
		return 0;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	if (size == 0)
		return 0;

	/* ----------------------------------------------------- */
	/* calculate mva block number */
	startRequire = va & (~M4U_PAGE_MASK);
	endRequire = (va + size - 1) | M4U_PAGE_MASK;
	sizeRequire = endRequire - startRequire + 1;
	nr = (sizeRequire + MVA_BLOCK_ALIGN_MASK) >> MVA_BLOCK_SIZE_ORDER;
	/* (sizeRequire>>MVA_BLOCK_SIZE_ORDER) +
	 * ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);
	 */

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	/* ----------------------------------------------- */
	/* find first match free region */
	for (s = 1; (s < (MVA_MAX_BLOCK_NR + 1)) &&
				(mvaGraph[domain_idx][s] < nr);
		s += (mvaGraph[domain_idx][s] & MVA_BLOCK_NR_MASK))
		;
	if (s > MVA_MAX_BLOCK_NR) {
		spin_unlock_irqrestore(mva_graph_lock,
							irq_flags);
		M4UMSG(
			"mva_alloc error: no available MVA region for %d blocks! domain=%u\n",
				nr, domain_idx);
#ifdef M4U_PROFILE
		mmprofile_log_ex(M4U_MMP_Events[M4U_MMP_M4U_ERROR],
			MMPROFILE_FLAG_PULSE, size, s);
#endif

		return 0;
	}
	/* ----------------------------------------------- */
	/* alloc a mva region */
	end = s + mvaGraph[domain_idx][s] - 1;

	if (unlikely(nr == mvaGraph[domain_idx][s])) {
		MVA_SET_BUSY(domain_idx, s);
		MVA_SET_BUSY(domain_idx, end);
		mvaInfoGraph[domain_idx][s] = priv;
		mvaInfoGraph[domain_idx][end] = priv;
	} else {
		new_end = s + nr - 1;
		new_start = new_end + 1;
		/* note: new_start may equals to end */
		mvaGraph[domain_idx][new_start] =
				(mvaGraph[domain_idx][s] - nr);
		mvaGraph[domain_idx][new_end] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][s] = mvaGraph[domain_idx][new_end];
		mvaGraph[domain_idx][end] = mvaGraph[domain_idx][new_start];

		mvaInfoGraph[domain_idx][s] = priv;
		mvaInfoGraph[domain_idx][new_end] = priv;
	}

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);

	mvaRegionStart = (unsigned int)s;

	return (mvaRegionStart << MVA_BLOCK_SIZE_ORDER) + mva_pageOffset(va);
}

unsigned int m4u_do_mva_alloc_fix(unsigned int domain_idx,
		unsigned long va, unsigned int mva,
		unsigned int size, void *priv)
{
	unsigned short nr = 0;
	unsigned int startRequire, endRequire, sizeRequire;
	unsigned long irq_flags;
	unsigned short startIdx = mva >> MVA_BLOCK_SIZE_ORDER;
	unsigned short endIdx;
	unsigned short region_start, region_end;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return 0;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	if (size == 0)
		return 0;
	if (startIdx == 0 || startIdx > MVA_MAX_BLOCK_NR) {
		M4UMSG("mvaGraph index is 0. index=0x%x, domain=%u\n",
			startIdx, domain_idx);
		return 0;
	}

	mva = mva | (va & M4U_PAGE_MASK);
	/* ----------------------------------------------------- */
	/* calculate mva block number */
	startRequire = mva & (~MVA_BLOCK_ALIGN_MASK);
	endRequire = (mva + size - 1) | MVA_BLOCK_ALIGN_MASK;
	sizeRequire = endRequire - startRequire + 1;
	nr = (sizeRequire + MVA_BLOCK_ALIGN_MASK) >> MVA_BLOCK_SIZE_ORDER;
	/* (sizeRequire>>MVA_BLOCK_SIZE_ORDER) +
	 * ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);
	 */

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	region_start = startIdx;
	/* find prev head of this region */
	while (mvaGraph[domain_idx][region_start] == 0)
		region_start--;

	if (MVA_IS_BUSY(domain_idx, region_start) ||
		(MVA_GET_NR(domain_idx, region_start) <
			nr + startIdx - region_start)) {
		M4UMSG(
			"mva is inuse index=0x%x, mvaGraph=0x%x, domain=%u\n",
			region_start, mvaGraph[domain_idx][region_start],
			domain_idx);
		mva = 0;
		goto out;
	}

	/* carveout startIdx~startIdx+nr-1 out of region_start */
	endIdx = startIdx + nr - 1;
	region_end = region_start + MVA_GET_NR(domain_idx, region_start) - 1;

	if (startIdx == region_start && endIdx == region_end) {
		MVA_SET_BUSY(domain_idx, startIdx);
		MVA_SET_BUSY(domain_idx, endIdx);
	} else if (startIdx == region_start) {
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
		mvaGraph[domain_idx][endIdx + 1] = region_end - endIdx;
		mvaGraph[domain_idx][region_end] =
				mvaGraph[domain_idx][endIdx + 1];
	} else if (endIdx == region_end) {
		mvaGraph[domain_idx][region_start] = startIdx - region_start;
		mvaGraph[domain_idx][startIdx - 1] =
				mvaGraph[domain_idx][region_start];
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
	} else {
		mvaGraph[domain_idx][region_start] = startIdx - region_start;
		mvaGraph[domain_idx][startIdx - 1] =
				mvaGraph[domain_idx][region_start];
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
		mvaGraph[domain_idx][endIdx + 1] = region_end - endIdx;
		mvaGraph[domain_idx][region_end] =
				mvaGraph[domain_idx][endIdx + 1];
	}

	mvaInfoGraph[domain_idx][startIdx] = priv;
	mvaInfoGraph[domain_idx][endIdx] = priv;

out:
	spin_unlock_irqrestore(mva_graph_lock, irq_flags);

	return mva;
}

unsigned int m4u_do_mva_alloc_start_from(
		unsigned int domain_idx,
		unsigned long va, unsigned int mva,
		unsigned int size, void *priv)
{
	unsigned short s = 0, end;
	unsigned short new_start, new_end;
	unsigned short nr = 0;
	unsigned int mvaRegionStart;
	unsigned long startRequire, endRequire, sizeRequire;
	unsigned long irq_flags;
	unsigned short startIdx = mva >> MVA_BLOCK_SIZE_ORDER;
	short region_start, region_end, next_region_start = 0;

	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return 0;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	if (size == 0)
		return 0;

	startIdx = (mva + MVA_BLOCK_ALIGN_MASK) >> MVA_BLOCK_SIZE_ORDER;

	/* ----------------------------------------------------- */
	/* calculate mva block number */
	startRequire = va & (~M4U_PAGE_MASK);
	endRequire = (va + size - 1) | M4U_PAGE_MASK;
	sizeRequire = endRequire - startRequire + 1;
	nr = (sizeRequire + MVA_BLOCK_ALIGN_MASK) >> MVA_BLOCK_SIZE_ORDER;
	/* (sizeRequire>>MVA_BLOCK_SIZE_ORDER) +
	 * ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);
	 */

	M4ULOG_MID(
		"%s mva:0x%x, startIdx=%d, size = %d, nr= %d, domain=%u\n",
		__func__,
		mva, startIdx, size, nr, domain_idx);

	spin_lock_irqsave(mva_graph_lock, irq_flags);

	/* find this region */
	for (region_start = 1; (region_start < (MVA_MAX_BLOCK_NR + 1));
		 region_start += (MVA_GET_NR(domain_idx, region_start) &
			MVA_BLOCK_NR_MASK)) {
		if ((mvaGraph[domain_idx][region_start] &
					MVA_BLOCK_NR_MASK) == 0) {
			m4u_mvaGraph_dump(domain_idx);
			m4u_aee_print("%s: s=%d, 0x%x, domain=%u\n",
				__func__, s,
				mvaGraph[domain_idx][region_start],
				domain_idx);
		}
		if ((region_start + MVA_GET_NR(domain_idx,
					region_start)) > startIdx) {
			next_region_start = region_start +
					MVA_GET_NR(domain_idx, region_start);
			break;
		}
	}

	if (region_start > MVA_MAX_BLOCK_NR) {
		M4UMSG(
			"%s:alloc mva fail,no available MVA for %d blocks, domain=%u\n",
				__func__, nr, domain_idx);
		spin_unlock_irqrestore(mva_graph_lock, irq_flags);
		return 0;
	}

	region_end = region_start + MVA_GET_NR(domain_idx, region_start) - 1;

	if (next_region_start == 0) {
		m4u_aee_print(
			"%s: region_start: %d, region_end= %d, region= %d, domain=%u\n",
			__func__, region_start, region_end,
			MVA_GET_NR(domain_idx, region_start),
			domain_idx);
	}

	if (MVA_IS_BUSY(domain_idx, region_start)) {
		M4UMSG("mva is inuse index=%d, mvaGraph=0x%x, domain=%u\n",
			region_start, mvaGraph[domain_idx][region_start],
			domain_idx);
		s = region_start;
	} else {
		if ((region_end - startIdx + 1) < nr)
			s = next_region_start;
		else
			M4UMSG("mva is free region_start=%d, s=%d, domain=%u\n",
				region_start, s, domain_idx);
	}

	M4ULOG_MID(
		"region_start: %d, region_end= %d, region= %d, next_region_start= %d, search start: %d, domain=%u\n",
		region_start, region_end,
		MVA_GET_NR(domain_idx, region_start), next_region_start, s,
		domain_idx);

	/* ----------------------------------------------- */
	if (s != 0) {
		/* find first match free region */
		for (; (s < (MVA_MAX_BLOCK_NR + 1)) &&
						(mvaGraph[domain_idx][s] < nr);
				s += (mvaGraph[domain_idx][s] &
					MVA_BLOCK_NR_MASK)) {
			if ((mvaGraph[domain_idx][s] &
					MVA_BLOCK_NR_MASK) == 0) {
				m4u_aee_print("%s: s=%d, 0x%x, domain=%u\n",
					__func__, s, mvaGraph[domain_idx][s],
					domain_idx);
				m4u_mvaGraph_dump(domain_idx);
			}
		}
	}

	if (s > MVA_MAX_BLOCK_NR) {
		spin_unlock_irqrestore(mva_graph_lock, irq_flags);
		M4UMSG(
			"mva_alloc error: no available MVA region for %d blocks!, domain=%u\n",
					nr, domain_idx);
#ifdef M4U_PROFILE
		mmprofile_log_ex(M4U_MMP_Events[M4U_MMP_M4U_ERROR],
			MMPROFILE_FLAG_PULSE, size, s);
#endif

		return 0;
	}
	/* ----------------------------------------------- */
	if (s == 0) {
		/* same as m4u_do_mva_alloc_fix */
		short endIdx = startIdx + nr - 1;

		region_end =
			region_start + MVA_GET_NR(domain_idx, region_start) - 1;
		M4UMSG(
			"region_start: %d, region_end= %d, startIdx: %d, endIdx= %d, domain=%u\n",
			region_start, region_end, startIdx, endIdx,
			domain_idx);

		if (startIdx == region_start && endIdx == region_end) {
			MVA_SET_BUSY(domain_idx, startIdx);
			MVA_SET_BUSY(domain_idx, endIdx);

	} else if (startIdx == region_start) {
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
		mvaGraph[domain_idx][endIdx + 1] = region_end - endIdx;
		mvaGraph[domain_idx][region_end] =
				mvaGraph[domain_idx][endIdx + 1];
	} else if (endIdx == region_end) {
		mvaGraph[domain_idx][region_start] = startIdx - region_start;
		mvaGraph[domain_idx][startIdx - 1] =
				mvaGraph[domain_idx][region_start];
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
	} else {
		mvaGraph[domain_idx][region_start] = startIdx - region_start;
		mvaGraph[domain_idx][startIdx - 1] =
				mvaGraph[domain_idx][region_start];
		mvaGraph[domain_idx][startIdx] = nr | MVA_BUSY_MASK;
		mvaGraph[domain_idx][endIdx] = mvaGraph[domain_idx][startIdx];
		mvaGraph[domain_idx][endIdx + 1] = region_end - endIdx;
		mvaGraph[domain_idx][region_end] =
				mvaGraph[domain_idx][endIdx + 1];
	}

	mvaInfoGraph[domain_idx][startIdx] = priv;
	mvaInfoGraph[domain_idx][endIdx] = priv;
		s = startIdx;
	} else {
		/* alloc a mva region */
		end = s + mvaGraph[domain_idx][s] - 1;

		if (unlikely(nr == mvaGraph[domain_idx][s])) {
			MVA_SET_BUSY(domain_idx, s);
			MVA_SET_BUSY(domain_idx, end);
			mvaInfoGraph[domain_idx][s] = priv;
			mvaInfoGraph[domain_idx][end] = priv;
		} else {
			new_end = s + nr - 1;
			new_start = new_end + 1;
			/* note: new_start may equals to end */
			mvaGraph[domain_idx][new_start] =
					(mvaGraph[domain_idx][s] - nr);
			mvaGraph[domain_idx][new_end] = nr | MVA_BUSY_MASK;
			mvaGraph[domain_idx][s] = mvaGraph[domain_idx][new_end];
			mvaGraph[domain_idx][end] =
					mvaGraph[domain_idx][new_start];

			mvaInfoGraph[domain_idx][s] = priv;
			mvaInfoGraph[domain_idx][new_end] = priv;
		}
	}
	spin_unlock_irqrestore(mva_graph_lock, irq_flags);

	mvaRegionStart = (unsigned int)s;

	return (mvaRegionStart << MVA_BLOCK_SIZE_ORDER) + mva_pageOffset(va);
}


#define RightWrong(x) ((x) ? "correct" : "error")
int m4u_do_mva_free(unsigned int domain_idx,
		unsigned int mva, unsigned int size)
{
	unsigned short startIdx = mva >> MVA_BLOCK_SIZE_ORDER;
	unsigned short nr;
	unsigned short endIdx;
	unsigned int startRequire, endRequire, sizeRequire;
	short nrRequire;
	unsigned long irq_flags;
	enum graph_lock_tpye lock_type;
	spinlock_t *mva_graph_lock;

	if (domain_idx == 0)
		lock_type = SPINLOCK_MVA_GRAPH0;
	else if (domain_idx == 1)
		lock_type = SPINLOCK_MVA_GRAPH1;
	else {
		M4UMSG("%s error: invalid m4u domain_idx(%d)!\n",
				__func__, domain_idx);
		return -1;
	}
	mva_graph_lock = get_mva_graph_lock(lock_type);

	spin_lock_irqsave(mva_graph_lock, irq_flags);
	if (startIdx == 0 || startIdx > MVA_MAX_BLOCK_NR) {
		spin_unlock_irqrestore(mva_graph_lock, irq_flags);

		M4UMSG("mvaGraph index is 0. mva=0x%x, domain=%u\n",
			mva, domain_idx);
		return -1;
	}
	nr = mvaGraph[domain_idx][startIdx] & MVA_BLOCK_NR_MASK;
	endIdx = startIdx + nr - 1;

	/* -------------------------------- */
	/* check the input arguments */
	/* right condition: startIdx is not NULL &&
	 * region is busy && right module && right size
	 */
	startRequire = mva & (unsigned int)(~M4U_PAGE_MASK);
	endRequire = (mva + size - 1) | (unsigned int)M4U_PAGE_MASK;
	sizeRequire = endRequire - startRequire + 1;
	nrRequire =
		(sizeRequire + MVA_BLOCK_ALIGN_MASK) >> MVA_BLOCK_SIZE_ORDER;
	/* (sizeRequire>>MVA_BLOCK_SIZE_ORDER) +
	 * ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);
	 */
	if (!(startIdx != 0	/* startIdx is not NULL */
		&& MVA_IS_BUSY(domain_idx, startIdx)
		&& (nr == nrRequire))) {
		spin_unlock_irqrestore(mva_graph_lock, irq_flags);
		M4UMSG(
			"error to free mva , domain=%u========================>\n",
			domain_idx);
		M4UMSG("BufSize=%d(unit:0x%xBytes) (expect %d) [%s]\n",
		       nrRequire, MVA_BLOCK_SIZE,
		       nr, RightWrong(nrRequire == nr));
		M4UMSG("mva=0x%x, (IsBusy?)=%d (expect %d) [%s]\n",
		       mva, MVA_IS_BUSY(domain_idx, startIdx), 1,
		       RightWrong(MVA_IS_BUSY(domain_idx, startIdx)));
		m4u_mvaGraph_dump(domain_idx);
		/* m4u_mvaGraph_dump_raw(); */
		return -1;
	}

	mvaInfoGraph[domain_idx][startIdx] = NULL;
	mvaInfoGraph[domain_idx][endIdx] = NULL;

	/* -------------------------------- */
	/* merge with followed region */
	if ((endIdx + 1 <= MVA_MAX_BLOCK_NR) &&
			(!MVA_IS_BUSY(domain_idx, endIdx + 1))) {
		nr += mvaGraph[domain_idx][endIdx + 1];
		mvaGraph[domain_idx][endIdx] = 0;
		mvaGraph[domain_idx][endIdx + 1] = 0;
	}
	/* -------------------------------- */
	/* merge with previous region */
	if ((startIdx - 1 > 0) && (!MVA_IS_BUSY(domain_idx, startIdx - 1))) {
		int pre_nr = mvaGraph[domain_idx][startIdx - 1];

		mvaGraph[domain_idx][startIdx] = 0;
		mvaGraph[domain_idx][startIdx - 1] = 0;
		startIdx -= pre_nr;
		nr += pre_nr;
	}
	/* -------------------------------- */
	/* set region flags */
	mvaGraph[domain_idx][startIdx] = nr;
	mvaGraph[domain_idx][startIdx + nr - 1] = nr;

	spin_unlock_irqrestore(mva_graph_lock, irq_flags);

	return 0;
}
