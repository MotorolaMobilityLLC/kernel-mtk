#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/memory.h>
#include <linux/memblock.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <mach/emi_mpu.h>

#ifdef CONFIG_ARM_LPAE
#include <mach/mt_lpae.h>
#endif

#include "mtkpasr_drv.h"

/* Struct for parsing rank information (SW view) */
struct view_rank {
	unsigned long start_pfn;	/* The 1st pfn (kernel pfn) */
	unsigned long end_pfn;		/* The pfn after the last valid one (kernel pfn) */
	unsigned long bank_pfn_size;	/* Bank size in PFN */
	unsigned long valid_channel;	/* Channels: 0x00000101 means there are 2 valid channels
					   - 1st & 2nd (MAX: 4 channels) */
};
static struct view_rank rank_info[MAX_RANKS];

/* Basic DRAM configuration */
static struct basic_dram_setting pasrdpd;

/* MTKPASR control variables */
static unsigned int channel_count;
static unsigned int rank_count;
static unsigned int banks_per_rank;
static unsigned long mtkpasr_start_pfn;
static unsigned long mtkpasr_end_pfn;
static unsigned long mtkpasr_segment_bits;

#ifdef CONFIG_ARM_LPAE
#define MAX_KERNEL_PFN	(0x13FFFF)
#define MAX_KPFN_MASK	(0x0FFFFF)
#define KPFN_TO_VIRT	(0x100000)
static unsigned long __init virt_to_kernel_pfn(unsigned long virt)
{
	unsigned long ret = virt;

	if (enable_4G())
		if (virt > MAX_KERNEL_PFN)
			ret = virt & MAX_KPFN_MASK;

	return ret;
}
static unsigned long __init kernel_pfn_to_virt(unsigned long kpfn)
{
	unsigned long ret = kpfn;

	if (enable_4G())
		ret = kpfn | KPFN_TO_VIRT;

	return ret;
}
static unsigned long __init rank_pfn_offset(void)
{
	unsigned long ret = ARCH_PFN_OFFSET;

	if (enable_4G())
		ret = KPFN_TO_VIRT;

	return ret;
}
#else
#define virt_to_kernel_pfn(x)		(x)
#define kernel_pfn_to_virt(x)		(x)
#define rank_pfn_offset()		((unsigned long)ARCH_PFN_OFFSET)
#endif

/* Round up by "base" from "offset" */
static unsigned long __init round_up_base_offset(unsigned long input, unsigned long base, unsigned long offset)
{
	return ((input - offset + base - 1) / base) * base + offset;
}

/* Round down by "base" from "offset" */
static unsigned long __init round_down_base_offset(unsigned long input, unsigned long base, unsigned long offset)
{
	return ((input - offset) / base) * base + offset;
}

/*
 * Check DRAM configuration - transform DRAM setting to temporary bank structure.
 * Return 0 on success, -1 on error.
 */
extern void acquire_dram_setting(struct basic_dram_setting *pasrdpd)__attribute__((weak));
static int __init check_dram_configuration(void)
{
	int chan, rank, check_segment_num;
	unsigned long valid_channel;
	unsigned long check_rank_size, rank_pfn, start_pfn = rank_pfn_offset();

	/* Acquire basic DRAM setting */
	acquire_dram_setting(&pasrdpd);

	/* Parse DRAM setting */
	channel_count = pasrdpd.channel_nr;
	for (rank = 0; rank < MAX_RANKS; ++rank) {
		rank_pfn = 0;
		rank_info[rank].valid_channel = 0x0;
		valid_channel = 0x1;
		check_rank_size = 0x0;
		check_segment_num = 0x0;
		for (chan = 0; chan < channel_count; ++chan) {
			if (pasrdpd.channel[chan].rank[rank].valid_rank) {
				/* # Gb -> # pages */
				rank_pfn += (pasrdpd.channel[chan].rank[rank].rank_size << (27 - PAGE_SHIFT));
				rank_info[rank].valid_channel |= valid_channel;
				/* Sanity check for rank size */
				if (!check_rank_size) {
					check_rank_size = pasrdpd.channel[chan].rank[rank].rank_size;
				} else {
					/* We only support ranks with equal size */
					if (check_rank_size != pasrdpd.channel[chan].rank[rank].rank_size)
						return -1;
				}
				/* Sanity check for segment number */
				if (!check_segment_num) {
					check_segment_num = pasrdpd.channel[chan].rank[rank].segment_nr;
				} else {
					/* We only support ranks with equal segment number */
					if (check_segment_num != pasrdpd.channel[chan].rank[rank].segment_nr)
						return -1;
				}
			}
			valid_channel <<= 8;
		}
		/* Have we found a valid rank */
		if (check_rank_size != 0 && check_segment_num != 0) {
			rank_info[rank].start_pfn = virt_to_kernel_pfn(start_pfn);
			rank_info[rank].end_pfn = virt_to_kernel_pfn(start_pfn + rank_pfn);
			rank_info[rank].bank_pfn_size = rank_pfn/check_segment_num;
			start_pfn = kernel_pfn_to_virt(rank_info[rank].end_pfn);
			pr_debug(
			"Rank[%d] start_pfn[%8lu] end_pfn[%8lu] bank_pfn_size[%8lu] valid_channel[0x%-8lx]\n",
			rank, rank_info[rank].start_pfn, rank_info[rank].end_pfn, rank_info[rank].bank_pfn_size,
			rank_info[rank].valid_channel);
			rank_count++;
			banks_per_rank = check_segment_num;
		} else {
			rank_info[rank].start_pfn = virt_to_kernel_pfn(rank_pfn_offset());
			rank_info[rank].end_pfn = virt_to_kernel_pfn(rank_pfn_offset());
			rank_info[rank].bank_pfn_size = 0;
			rank_info[rank].valid_channel = 0x0;
		}
	}

	return 0;
}

/*
 * Check whether it is a valid rank
 */
static bool __init is_valid_rank(int rank)
{
	/* Check start/end pfn */
	if (rank_info[rank].start_pfn == rank_info[rank].end_pfn)
		return false;

	/* Check valid_channel */
	if (rank_info[rank].valid_channel == 0x0)
		return false;

	return true;
}

/*
 * Fill mtkpasr_segment_bits
 */
static void __init find_mtkpasr_valid_segment(unsigned long start, unsigned long end)
{
	int num_segment, rank;
	unsigned long spfn, epfn;
	unsigned long rspfn, repfn;
	unsigned long bank_pfn_size;

	num_segment = 0;
	for (rank = 0; rank < MAX_RANKS; ++rank) {
		spfn = kernel_pfn_to_virt(start);
		epfn = kernel_pfn_to_virt(end);
		rspfn = kernel_pfn_to_virt(rank_info[rank].start_pfn);
		repfn = kernel_pfn_to_virt(rank_info[rank].end_pfn);
		if (!is_valid_rank(rank))
			continue;
		bank_pfn_size = rank_info[rank].bank_pfn_size;
		if (epfn >= spfn) {
			/* Intersect */
			if (spfn < repfn && rspfn < epfn) {
				spfn = max(spfn, rspfn);
				epfn = min(epfn, repfn);
				spfn = round_up_base_offset(spfn, bank_pfn_size, rank_pfn_offset());
				epfn = round_down_base_offset(epfn, bank_pfn_size, rank_pfn_offset());
				while (epfn >= (spfn + bank_pfn_size)) {
					mtkpasr_segment_bits |= (1 << ((spfn - rspfn) / bank_pfn_size + num_segment));
					spfn += bank_pfn_size;
				}
			}
		} else {
			/* spfn ~ repfn */
			spfn = max(spfn, rspfn);
			if (spfn < repfn)
				spfn = round_up_base_offset(spfn, bank_pfn_size, rank_pfn_offset());
			while (repfn >= (spfn + bank_pfn_size)) {
				mtkpasr_segment_bits |= (1 << ((spfn - rspfn) / bank_pfn_size + num_segment));
				spfn += bank_pfn_size;
			}
			/* rspfn ~ epfn */
			epfn = min(epfn, repfn);
			if (rspfn < epfn)
				epfn = round_down_base_offset(epfn, bank_pfn_size, rank_pfn_offset());
			while ((epfn - bank_pfn_size) >= rspfn) {
				epfn -= bank_pfn_size;
				mtkpasr_segment_bits |= (1 << ((epfn - rspfn) / bank_pfn_size + num_segment));
			}
		}
		num_segment += 8;	/* HW constraint - 8 segment-bits per rank */
	}
}

/*
 * We will set an offset on which active PASR will be imposed.
 * This is done by acquiring CMA's base and size.
 * Return <0 means "fail to init pasr range"
 *        >0 means "the number of valid banks"
 */
int __init mtkpasr_init_range(unsigned long start_pfn, unsigned long end_pfn)
{
	int ret = 0;
	int rank;
	unsigned long pfn_bank_alignment = 0;
	unsigned long vseg, seg_num = 0;

	/* Check DRAM configuration */
	ret = check_dram_configuration();
	if (ret < 0)
		goto out;

	/* Find out which rank "start_pfn" belongs to */
	for (rank = 0; rank < MAX_RANKS; ++rank) {
		if (kernel_pfn_to_virt(start_pfn) < kernel_pfn_to_virt(rank_info[rank].end_pfn) &&
				kernel_pfn_to_virt(start_pfn) >=
				kernel_pfn_to_virt(rank_info[rank].start_pfn)) {
			pfn_bank_alignment = rank_info[rank].bank_pfn_size;
			break;
		}
	}

	/* Sanity check */
	if (!pfn_bank_alignment) {
		ret = -1;
		goto out;
	}

	/* 1st attempted bank size */
	bank_pfns = pfn_bank_alignment;

	/* Find out which rank "end_pfn" belongs to */
	for (rank = 0; rank < MAX_RANKS; ++rank) {
		if (kernel_pfn_to_virt(end_pfn) <= kernel_pfn_to_virt(rank_info[rank].end_pfn) &&
				kernel_pfn_to_virt(end_pfn) >
				kernel_pfn_to_virt(rank_info[rank].start_pfn)) {
			pfn_bank_alignment = rank_info[rank].bank_pfn_size;
			break;
		}
	}

	/* Sanity check: only allow equal bank size */
	if (bank_pfns != pfn_bank_alignment) {
		ret = -2;
		goto out;
	}

	/* Determine mtkpasr_start_pfn/end */
	mtkpasr_start_pfn = round_up_base_offset(start_pfn, pfn_bank_alignment, ARCH_PFN_OFFSET);
	mtkpasr_end_pfn = round_down_base_offset(end_pfn, pfn_bank_alignment, ARCH_PFN_OFFSET);

	/* Map PASR start/end kernel pfn to DRAM segments */
	find_mtkpasr_valid_segment(mtkpasr_start_pfn, mtkpasr_end_pfn);

	/* How many segments */
	vseg = mtkpasr_segment_bits;
	ret = 0;
	do {
		if (vseg & 0x1)
			ret++;
		vseg >>= 1;
	} while (++seg_num < BITS_PER_LONG);

	pr_debug("Start_pfn[%8lu] End_pfn[%8lu] Valid_segment[0x%8lx] Segments[%u]\n",
			mtkpasr_start_pfn, mtkpasr_end_pfn, mtkpasr_segment_bits, ret);

out:
	return ret;
}

/*
 * Give bank, this function will return its (start_pfn, end_pfn) and corresponding rank
 * Return -1 means no valid banks, ranks
 *        0  means no corresponding rank
 *        >0 means there are corresponding bank, rank (Caller should subtract 1 to get the correct rank number)
 */
int __init query_bank_rank_information(int bank, unsigned long *spfn, unsigned long *epfn, int *segn)
{
	int seg_num = 0, rank, num_segment = 0;
	unsigned long vseg = mtkpasr_segment_bits, vmask;

	/* Reset */
	*spfn = 0;
	*epfn = 0;

	/* Which segment */
	do {
		if (vseg & 0x1) {
			/* Found! */
			if (!bank)
				break;
			bank--;
		}
		vseg >>= 1;
		seg_num++;
	} while (seg_num < BITS_PER_LONG);

	/* Sanity check */
	if (seg_num == BITS_PER_LONG)
		return -1;

	/* Corresponding segment */
	*segn = seg_num;

	/* Which rank */
	vseg = mtkpasr_segment_bits;
	for (rank = 0; rank < MAX_RANKS; ++rank) {
		if (is_valid_rank(rank)) {
			num_segment = (kernel_pfn_to_virt(rank_info[rank].end_pfn) -
					kernel_pfn_to_virt(rank_info[rank].start_pfn)) /
				rank_info[rank].bank_pfn_size;
			if (seg_num < num_segment) {
				*spfn = virt_to_kernel_pfn(kernel_pfn_to_virt(rank_info[rank].start_pfn) +
						seg_num * rank_info[rank].bank_pfn_size);
				*epfn = virt_to_kernel_pfn(kernel_pfn_to_virt(*spfn) +
						rank_info[rank].bank_pfn_size);
				/* Fixup to meet bank range definition ??? */
				if (*epfn <= *spfn)
					*epfn = kernel_pfn_to_virt(*epfn);
				break;
			}
			seg_num -= num_segment;
			vseg >>= num_segment;
		}
	}

	/* Sanity check */
	if (rank == MAX_RANKS)
		return -1;

	/* Query rank information */
	vmask = (1 << num_segment) - 1;
	if ((vseg & vmask) == vmask)
		return (rank + 1);

	return 0;
}
