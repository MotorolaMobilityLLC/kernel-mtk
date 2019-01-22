/****************************************************************************
* This confidential and proprietary software may be used only as authorized *
* by a licensing agreement from ARM Israel.                                 *
* Copyright (C) 2015 ARM Limited or its affiliates. All rights reserved.    *
* The entire notice above must be reproduced on all authorized copies and   *
* copies may only be made to the extent permitted by a licensing agreement  *
* from ARM Israel.                                                          *
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>

/* Registers definitions from shared/hw/include */
#include "dx_reg_base_host.h"
#include "dx_reg_common.h"
#include "dx_host.h"
#include "ssi_regs.h"
#include "ssi_hal.h"
#include "hw_queue_defs.h"
#include "cc_ll_test.h"

/* Module parameters */
static uint32_t cache_params = DX_CACHE_PARAMS_DEFAULT;
static unsigned int dma_coherent_ops = DX_CACHE_PARAMS_DEFAULT;
static int verif_msleep = RESULTS_VERIF_MSLEEP;
static unsigned int test_log_level = 1;
static int test_idx = -1;
module_param(cache_params, uint, 0400);
MODULE_PARM_DESC(cache_params, "ARM DMA master AR/AW-cache parameters");
module_param(dma_coherent_ops, uint, 0400);
MODULE_PARM_DESC(dma_coherent_ops, "1 for arm_coherent_dma_ops, 0 (default) for arm_dma_ops");
module_param(verif_msleep, int, 0400);
MODULE_PARM_DESC(verif_msleep, "[msec] between first and second results verification.");
module_param(test_log_level, uint, 0400);
MODULE_PARM_DESC(test_log_level, "Test logging level: 0 - just errors, 1 - per case status , 2 - detailed logging");
module_param(test_idx, int, 0400);
MODULE_PARM_DESC(test_idx, "Test case selection: Index of test case to run. Default: (-1) all tests.");

/* Test cases */
/* We assume that the allocated data buffers are page aligned (i.e., also cache line aligned) */
struct test_case {
	unsigned long dma_size;	/* Size of DMA bypass operation in bytes */
	unsigned long dma_din_page_offset; /* Offset of DMA input from page start */
	unsigned long dma_dout_page_offset;/* Offset of DMA output from page start */
};
/* Test context */
struct tester_ctx {
	struct resource *res_mem;
	struct resource *res_irq;
	struct device *dev;
	void __iomem *cc_base;
	struct completion desc_comp_event;
	uint8_t *buf_din_p;	/* Allocated input buffer */
	uint8_t *buf_dout_p;	/* Allocated output buffer */
	unsigned long buf_size;	/* Full buffer allocation size in bytes */
	unsigned long dma_size;	/* Size of DMA bypass operation in bytes */
	unsigned long dma_din_buf_offset; /* Offset of DMA input from buf_din start */
	unsigned long dma_dout_buf_offset;/* Offset of DMA output from buf_dout start */
	dma_addr_t dma_din;
	dma_addr_t dma_dout;
};

/* Logging macros */
#define TEST_LOG(level, format, ...) \
	printk(level "DxCC LLtest: " format , ##__VA_ARGS__)
#define TEST_LOG_ERR(format, ...) TEST_LOG(KERN_ERR, format, ##__VA_ARGS__)
#define TEST_LOG_STATUS(format, ...) do { if (test_log_level > 0) TEST_LOG(KERN_NOTICE, format, ##__VA_ARGS__); } while (0)
#define TEST_LOG_TRACE(format, ...) do { if (unlikely(test_log_level > 1)) TEST_LOG(KERN_INFO, format, ##__VA_ARGS__); } while (0)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define CACHE_LINE_MASK (L1_CACHE_BYTES - 1)
#define ALIGN_TO_THIS_CACHE_LINE(ptr) ((unsigned long)(ptr) & ~CACHE_LINE_MASK)
#define ALIGN_TO_NEXT_CACHE_LINE(ptr) ALIGN_TO_THIS_CACHE_LINE((unsigned long)(ptr) + L1_CACHE_BYTES)
/* Return pointer which is at given cache line offset from given pointer up */
/* = ptr + truncate_to_cache_line_size(distance_to_next_cache_line + requested_offset) */
#define PTR_TO_NEXT_CACHE_LINE_OFFSET(ptr, cache_line_offset) \
	(ALIGN_TO_NEXT_CACHE_LINE(ptr) + cache_line_offset)

/* Fill pattern for data buffers before test */
/* The data buffer are filled with incrementing values starting with "offset" */
#define FILL_OFFSET_MARGINS_OUT 0xE0 /* For buffer outside the DMA zone of output */
#define FILL_OFFSET_MAIN_OUT 0x40 /* For output buffer before DMA */
#define FILL_OFFSET_MARGINS_IN 0x10 /* For buffer outside the DMA zone of input */
#define FILL_OFFSET_MAIN_IN 0xA0  /* For input data to be DMAed */

#define DESC_COMPLETION_TIMEOUT_MSEC 2000
#define MAX_AXI_COMP_POLLING 16

#define AXI_ID_CODE 0
#define NS_BIT_VAL 1

#define CC_REG_SPACE_SIZE 0x1000
#define AXIM_MON_BASE_OFFSET SASI_REG_OFFSET(CRY_KERNEL, AXIM_MON_COMP8)
#define DX_AXI_IRQ_MASK ((1 << DX_AXIM_CFG1_BRESPMASK_BIT_SHIFT) | (1 << DX_AXIM_CFG1_RRESPMASK_BIT_SHIFT) |	\
			(1 << DX_AXIM_CFG1_INFLTMASK_BIT_SHIFT) | (1 << DX_AXIM_CFG1_COMPMASK_BIT_SHIFT))
#define DX_AXI_ERR_IRQ_MASK (1 << DX_HOST_IRR_AXI_ERR_INT_BIT_SHIFT)
#define DX_COMP_IRQ_MASK (1 << DX_HOST_IRR_AXIM_COMP_INT_BIT_SHIFT)

/* Define mask for interrupt causes that interest this test */
#define TEST_IRQ_MASK (DX_COMP_IRQ_MASK | DX_AXI_ERR_IRQ_MASK)
#define BYPASS_BYPRODUCT_IRR_MASK ((1 << DX_HOST_IRR_MEM_TO_DIN_INT_BIT_SHIFT) | (1 << DX_HOST_IRR_DOUT_TO_MEM_INT_BIT_SHIFT))

static irqreturn_t cc_isr(int irq, void *ctx_p)
{
	struct tester_ctx *tctx_p = (struct tester_ctx *)ctx_p;
	uint32_t irr;

	if (irq < 0)
		TEST_LOG_TRACE("cc_isr: Invoked for no IRQ.\n");
	/* read the interrupt status */
	irr = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IRR));
	TEST_LOG_TRACE("cc_isr: Got IRR=0x%08X\n", irr);
	if (unlikely(irr == 0)) { /* Probably shared interrupt line */
		TEST_LOG_ERR("cc_isr: empty IRR\n");
		return IRQ_NONE;
	}

	/* clear interrupt - must be before processing events */
	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR), irr);

	/* Completion interrupt - most probable */
	if (likely((irr & DX_COMP_IRQ_MASK) != 0)) {
		complete(&tctx_p->desc_comp_event);
		irr &= ~DX_COMP_IRQ_MASK;
	}

	/* AXI error interrupt */
	if (unlikely((irr & DX_AXI_ERR_IRQ_MASK) != 0)) {
		uint32_t axi_err;
		axi_err = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_MON_ERR));
		TEST_LOG_ERR("cc_isr: AXI completion error: axim_mon_err=0x%08X\n", axi_err);
		irr &= ~DX_AXI_ERR_IRQ_MASK;
	}

	irr &= ~BYPASS_BYPRODUCT_IRR_MASK; /* Masked interrupt causes which are normal for BYPASS flow */

	if (unlikely(irr != 0)) {
		TEST_LOG_STATUS("IRR includes unknown cause bits (0x%08X)\n", irr);
		/* Just warning */
	}

	return IRQ_HANDLED;
}

/**
 * do_bypass() - Initiate BYPASS DMA operation between given DMA buffers
 *	The function assumes that given buffers were already DMA-mapped
 * 
 * @din_dma:	Input data buffer DMA address
 * @dout_dma:	Output data buffer DMA address
 * @dsize:	Size of given buffers in bytes
 */
static int do_bypass(struct tester_ctx *tctx_p, dma_addr_t din_dma, dma_addr_t dout_dma, unsigned long dsize)
{
	void __iomem *cc_base = tctx_p->cc_base;
	unsigned long time_to_timeout;
	uint32_t axi_write_cnt = 0;
	uint32_t axim_mon;
	uint32_t axi_completed;
	int retry;
	int rc = 0;
	HwDesc_s desc;

	HW_DESC_INIT(&desc);
	HW_DESC_SET_FLOW_MODE(&desc, BYPASS);
	HW_DESC_SET_DIN_TYPE(&desc, DMA_DLLI, din_dma, dsize, AXI_ID_CODE, NS_BIT_VAL);
	HW_DESC_SET_DOUT_DLLI(&desc, dout_dma, dsize, AXI_ID_CODE, NS_BIT_VAL, 1);
	axi_write_cnt++;

	/* Enqueue descriptor */
	TEST_LOG_TRACE("Waiting for free slot in HW queue...\n");
	HW_QUEUE_POLL_QUEUE_UNTIL_FREE_SLOTS(0, 1);
	HW_DESC_PUSH_TO_QUEUE(0, &desc);

	/* Process completion */
	TEST_LOG_TRACE("Waiting for bypass completion %u [msec]...\n", DESC_COMPLETION_TIMEOUT_MSEC);
	time_to_timeout = wait_for_completion_timeout(&tctx_p->desc_comp_event, msecs_to_jiffies(DESC_COMPLETION_TIMEOUT_MSEC));
	if (time_to_timeout == 0) {
		rc = -EIO; /* Even if we continue, this is a failure */
		TEST_LOG_ERR("Timeout waiting for AXI completion interrupt. Running ISR manually.\n");
		/* Invoke ISR manually with interrupts masked to avoid race */
		WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR), ~0);
		cc_isr(-1, tctx_p);
		WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR), ~TEST_IRQ_MASK);
		if (try_wait_for_completion(&tctx_p->desc_comp_event) == 0)
			TEST_LOG_ERR("No completion indication found in IRR. Looking for AXI completion anyway.\n");
	}
	axi_completed = 0;
	for (retry = 0; (retry < MAX_AXI_COMP_POLLING) && (axi_completed < axi_write_cnt); retry++) {
		axim_mon = READ_REGISTER(tctx_p->cc_base + AXIM_MON_BASE_OFFSET);
		axi_completed += SASI_REG_FLD_GET(CRY_KERNEL, AXIM_MON_COMP8, VALUE, axim_mon);
	}
	if (axi_completed < axi_write_cnt) {
		TEST_LOG_ERR("Got only %u/%u AXI completions after %d retries\n", axi_completed, axi_write_cnt, retry);
		rc = -EIO;
	}
	if (likely(rc == 0))
		TEST_LOG_TRACE("Bypass completed successfully.\n");
	else
		TEST_LOG_ERR("Bypass operation failed.\n");
	return rc;
}

static void get_test_case(int test_case_idx, struct test_case *case_p)
{
	/*
	   Current test cases actually iterate over the 3 case elements:
	   1. dma_size: 4096, 1024, 256, L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 
	   2. din_offset (from page start): 0, L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 
	   3. dout_offset (from page start): 0, L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 
	*/
	static const unsigned long dma_sizes[] = { 4096, 256, L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 };
	static const unsigned long din_offsets[] = { L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 };
	static const unsigned long dout_offsets[] = { L1_CACHE_BYTES, L1_CACHE_BYTES/2, 8, 4, 2, 1 };
	int size_idx, din_offset_idx, dout_offset_idx;

	if (test_case_idx >= (ARRAY_SIZE(dma_sizes) * ARRAY_SIZE(din_offsets) * ARRAY_SIZE(dout_offsets))) {
		/* Signal that there are no more test cases with dma_size == 0 */
		case_p->dma_size = 0;
		case_p->dma_din_page_offset = 0;
		case_p->dma_dout_page_offset = 0;
	} else {
		/* Test case is composed of the combination of each array */
		size_idx = test_case_idx % ARRAY_SIZE(dma_sizes);
		din_offset_idx = (test_case_idx / ARRAY_SIZE(dma_sizes)) % ARRAY_SIZE(din_offsets);
		dout_offset_idx = ((test_case_idx / ARRAY_SIZE(dma_sizes)) / ARRAY_SIZE(din_offsets)) % ARRAY_SIZE(dout_offsets);
		case_p->dma_size = dma_sizes[size_idx];
		case_p->dma_din_page_offset = din_offsets[din_offset_idx];
		case_p->dma_dout_page_offset = dout_offsets[dout_offset_idx];
	}
}

/**
 * alloc_test_buffers() - Allocate buf_din and buf_dout based on test case
 *	that is already set in tctx_p->dma_size.
 * 
 * @tctx_p:
 */
static int alloc_test_buffers(struct tester_ctx *tctx_p)
{
	/* Allocate margin of 2 pages to allow offset of up to PAGE_SIZE and still keep some margin at end */
	tctx_p->buf_size = PAGE_SIZE * (2 + (tctx_p->dma_size / PAGE_SIZE) + 1); 
	tctx_p->buf_din_p = kmalloc(tctx_p->buf_size, GFP_KERNEL);
	if (unlikely(tctx_p->buf_din_p == NULL)) {
		TEST_LOG_ERR("Failed allocating Din buffer of %lu B\n", tctx_p->buf_size);
		return -ENOMEM;
	}
	if (unlikely(((unsigned long)tctx_p->buf_din_p & PAGE_MASK) != (unsigned long)tctx_p->buf_din_p)) {
		/* kernel is supposed to use the slab so PAGE_SIZE multiple is page aligned */
		TEST_LOG_ERR("Allocated Din buffer is not page aligned (%p)\n", tctx_p->buf_din_p);
		return -EINVAL;
	}
	tctx_p->buf_dout_p = kmalloc(tctx_p->buf_size, GFP_KERNEL);
	if (unlikely(tctx_p->buf_dout_p == NULL)) {
		TEST_LOG_ERR("Failed allocating Dout buffer of %lu B\n", tctx_p->buf_size);
		return -ENOMEM;
	}
	if (unlikely(((unsigned long)tctx_p->buf_dout_p & PAGE_MASK) != (unsigned long)tctx_p->buf_dout_p)) {
		/* kernel is supposed to use the slab so PAGE_SIZE multiple is page aligned */
		TEST_LOG_ERR("Allocated Dout buffer is not page aligned (%p)\n", tctx_p->buf_dout_p);
		return -EINVAL;
	}
	return 0;
}

/**
 * fill_buf() - Fill buffer with incrementing bytes values
 * 
 * @buf_p:		The buffer to fill
 * @buf_size:		The size in bytes to fill
 * @fill_in_reverse:	When "true" - fill from end to start
 * @fill_start_val:	The byte value to start with
 */
static void fill_buf(uint8_t *buf_p, unsigned long buf_size, bool fill_in_reverse, uint8_t fill_start_val)
{
	uint8_t cur_val = fill_start_val;

	if (fill_in_reverse)
		buf_p += (buf_size - 1);
	while (buf_size > 0) {
		*buf_p = cur_val;
		buf_size--;
		cur_val++;
		if (fill_in_reverse)
			buf_p--;
		else
			buf_p++;
	}
}

/**
 * pattern_diff() - Compare data buffer to expected fill pattern
 *	Returns number of mismatching segments (>=0)
 * 
 * @buf_desc:		Buffer description/name
 * @buf_p:		The buffer to check
 * @buf_size:		The buffer size in bytes
 * @fill_in_reverse:	When "true" - check from end to start (reverse fill)
 * @fill_start_val:	The byte value to start with
 */
static int pattern_diff(const char* buf_desc, uint8_t *buf_p, unsigned long buf_size, bool fill_in_reverse, uint8_t fill_start_val)
{
	uint8_t cur_val = fill_start_val;
	uint8_t mismatch_start_val = 0, mismatch_end_val;
	uint8_t *cur_buf_p = buf_p;
	uint8_t *mismatch_start_p = NULL;
	uint8_t *mismatch_end_p;
	long mismatch_len, mismatch_len_total = 0;
	int inc_val = 1;
	unsigned int mismatch_cnt = 0;

	if (fill_in_reverse) {
		cur_buf_p += (buf_size - 1);
		inc_val = -1;
	}
	while (buf_size > 0) {
		//TEST_LOG_TRACE("*** cur_buf_p=%p val=0x%02X exp=0x%02X\n", cur_buf_p, *cur_buf_p, cur_val);
		if (*cur_buf_p != cur_val) {
			if (mismatch_start_p == NULL) {/* mismatch start */
				mismatch_start_p = cur_buf_p;
				mismatch_start_val = cur_val;
			}
		} else if (mismatch_start_p != NULL) {
			if (fill_in_reverse) {
				mismatch_end_p = mismatch_start_p;
				mismatch_end_val = mismatch_start_val;
				mismatch_start_p = cur_buf_p + 1;
				mismatch_start_val = cur_val - 1;
			} else {
				mismatch_end_p = cur_buf_p - 1;
				mismatch_end_val = cur_val - 1;
			}
			mismatch_len = 1 + mismatch_end_p - mismatch_start_p;
			TEST_LOG_STATUS("%s: Data mismatch of %ld B at offset %ld (%p-%p). Expected values from 0x%02X %sto 0x%02X\n",
				buf_desc, mismatch_len, (long)(mismatch_start_p - buf_p), mismatch_start_p, mismatch_end_p,
				mismatch_start_val, fill_in_reverse ? "down " : "", mismatch_end_val);
			if (test_log_level > 2)
				print_hex_dump_bytes(buf_desc, DUMP_PREFIX_ADDRESS, mismatch_start_p, mismatch_len);
			else if (test_log_level > 1) { /* Dump at most L1_CACHE_BYTES per mismatch */
				if (mismatch_len > L1_CACHE_BYTES) {
					TEST_LOG_STATUS("* Dumping only first %u B of mismatch data:\n", L1_CACHE_BYTES);
					print_hex_dump_bytes(buf_desc, DUMP_PREFIX_ADDRESS, mismatch_start_p, L1_CACHE_BYTES);
				} else {
					print_hex_dump_bytes(buf_desc, DUMP_PREFIX_ADDRESS, mismatch_start_p, mismatch_len);
				}
			}
			mismatch_len_total += mismatch_len;
			mismatch_cnt++;
			mismatch_start_p = NULL; /* Prepare for next mismatch segment */
		}
		buf_size--;
		cur_val++;
		cur_buf_p += inc_val;
	}
	if (mismatch_cnt > 0)
		TEST_LOG_ERR("%s: Mismatch of %ld B in %u segments\n", buf_desc, mismatch_len_total, mismatch_cnt);
	return mismatch_cnt;
}

static int verify_results(struct tester_ctx *tctx_p)
{
	int err_cnt;

	err_cnt = pattern_diff("Dout:", tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset, tctx_p->dma_size, false, FILL_OFFSET_MAIN_IN);
	err_cnt += pattern_diff("Dout(before):", tctx_p->buf_dout_p, tctx_p->dma_dout_buf_offset, true, FILL_OFFSET_MARGINS_OUT);
	err_cnt += pattern_diff("Dout(after):", tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset + tctx_p->dma_size, 4 * L1_CACHE_BYTES,
		false, FILL_OFFSET_MARGINS_OUT);
	if (unlikely(err_cnt != 0)) /* Convert mismatch count to error code */
		return -EINVAL;
	else
		return 0;
}

/**
 * fill_data_buffers() - Fill data buffers with known patterns
 */
static void fill_data_buffers(struct tester_ctx *tctx_p)
{
	/* Fill part where DMA is done with incrementing values */
	fill_buf(tctx_p->buf_din_p + tctx_p->dma_din_buf_offset, tctx_p->dma_size, false, FILL_OFFSET_MAIN_IN);
	fill_buf(tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset, tctx_p->dma_size, false, FILL_OFFSET_MAIN_OUT);
	/* Now fill margins ("background") */
	fill_buf(tctx_p->buf_din_p, tctx_p->dma_din_buf_offset, true, FILL_OFFSET_MARGINS_IN);
	fill_buf(tctx_p->buf_din_p + tctx_p->dma_din_buf_offset + tctx_p->dma_size, 4 * L1_CACHE_BYTES, false, FILL_OFFSET_MARGINS_IN);
	fill_buf(tctx_p->buf_dout_p, tctx_p->dma_dout_buf_offset, true, FILL_OFFSET_MARGINS_OUT);
	fill_buf(tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset + tctx_p->dma_size, 4 * L1_CACHE_BYTES, false, FILL_OFFSET_MARGINS_OUT);
}

static int map4dma(struct tester_ctx *tctx_p)
{
	tctx_p->dma_din = dma_map_single(tctx_p->dev, tctx_p->buf_din_p + tctx_p->dma_din_buf_offset, tctx_p->dma_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(tctx_p->dev, tctx_p->dma_din))) {
		tctx_p->dma_din = 0;
		return -ENOMEM;
	}
	/* For output we use BIDIRECTIONAL to avoid cache invalidation (i.e., reference data loss) if we use _FROM_DEVICE */
	tctx_p->dma_dout = dma_map_single(tctx_p->dev, tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset, tctx_p->dma_size, DMA_BIDIRECTIONAL);
	if (unlikely(dma_mapping_error(tctx_p->dev, tctx_p->dma_dout))) {
		tctx_p->dma_dout = 0;
		return -ENOMEM;
	}
	return 0;
}

static void unmap4dma(struct tester_ctx *tctx_p)
{
	if (tctx_p->dma_din != 0) {
		dma_unmap_single(tctx_p->dev, tctx_p->dma_din, tctx_p->dma_size, DMA_TO_DEVICE);
		tctx_p->dma_din = 0;
	}
	if (tctx_p->dma_dout != 0) {
		dma_unmap_single(tctx_p->dev, tctx_p->dma_dout, tctx_p->dma_size, DMA_BIDIRECTIONAL);
		tctx_p->dma_dout = 0;
	}
}

static int exec_test_case(struct tester_ctx *tctx_p, struct test_case *case_p)
{
	int rc = 0;

	/* Get test case into context */
	tctx_p->dma_size = case_p->dma_size;
	tctx_p->dma_din_buf_offset = case_p->dma_din_page_offset;
	tctx_p->dma_dout_buf_offset = case_p->dma_dout_page_offset;
	/* Offset offsets into second page of buffer (at same page offset) to have margins */
	tctx_p->dma_din_buf_offset += PAGE_SIZE;
	tctx_p->dma_dout_buf_offset += PAGE_SIZE;
	rc = alloc_test_buffers(tctx_p);
	if (unlikely(rc != 0))
		goto test_done;
	fill_data_buffers(tctx_p);
	rc = map4dma(tctx_p);
	if (unlikely(rc != 0))
		goto test_done;
	TEST_LOG_TRACE("Dispatching bypass of %lu B from din.va=%p/.dma=0x%llX to dout.va=%p/.dma=%llX\n",
		tctx_p->dma_size,
		tctx_p->buf_din_p + tctx_p->dma_din_buf_offset, (unsigned long long)tctx_p->dma_din,
		tctx_p->buf_dout_p + tctx_p->dma_dout_buf_offset, (unsigned long long)tctx_p->dma_dout);
	rc = do_bypass(tctx_p, tctx_p->dma_din, tctx_p->dma_dout, tctx_p->dma_size);
	/* Continue data checking even if had error in BYPASS flow - maybe operation completed without proper operation */
	/* Verify output buffer contents */
	unmap4dma(tctx_p); /* Must sync back DMA buffers before checking contents */
	TEST_LOG_TRACE("Verifying results\n");
	if (unlikely((verify_results(tctx_p) != 0) && (rc == 0)))
		rc = -EINVAL;
	if (verif_msleep > 0)	/* If 0, avoid msleep completely (i.e., no processor yield) */
		msleep(verif_msleep);
	if (verif_msleep >= 0) { /* If negative, skip second verification */
		TEST_LOG_TRACE("Verifying results again after msleep(%d)\n", verif_msleep);
		if (unlikely((verify_results(tctx_p) != 0) && (rc == 0)))
		    rc = -EINVAL;
	}

test_done:
	unmap4dma(tctx_p); /* No effect if second time, but required in case of error exit */
	/* Free buffers */
	if (tctx_p->buf_dout_p != NULL)
		kfree(tctx_p->buf_dout_p);
	if (tctx_p->buf_din_p != NULL)
		kfree(tctx_p->buf_din_p);

	/* Return test results */
	return rc;
}

/**
 * sanity_checks() - Sanity checks to verify CC registers space is accessible
 * 
 * @tctx_p:
 */
static int sanity_checks(struct tester_ctx *tctx_p)
{
	uint32_t signature_val, hw_ver;
#ifdef DX_ROM_VERSION
	uint32_t rom_ver, sram_threshold;
#endif
	int rc = 0;

	/* Check SIGNATURE */
	signature_val = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_CC_SIGNATURE));
	if (signature_val != DX_DEV_SIGNATURE) {
		TEST_LOG_ERR("Invalid CC signature: SIGNATURE=0x%08X != expected=0x%08X\n",
			signature_val, (uint32_t)DX_DEV_SIGNATURE);
		/* No sense reading additional registers if the signature is incorrect */
		return -EINVAL;
	}
	TEST_LOG_TRACE("  CC SIGNATURE=0x%08X\n", signature_val);

	/* Check HW Version */
	hw_ver = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_VERSION));
	if (hw_ver != DX_HW_VERSION) {
		TEST_LOG_ERR("HW version mismatch: HOST_VERSION=0x%08X != expected=0x%08X\n",
			hw_ver, DX_HW_VERSION);
		rc = -EINVAL;
	}
	TEST_LOG_TRACE("  HW Version=0x%08X\n", hw_ver);

#ifdef DX_ROM_VERSION
	/* Check ROM Version */
	rom_ver = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR0));
	if (rom_ver != DX_ROM_VERSION) {
		TEST_LOG_ERR("ROM version mismatch: ROM version=0x%08X != expected=0x%08X\n",
			rom_ver, DX_ROM_VERSION);
		rc = -EINVAL;
	}
	TEST_LOG_TRACE("  ROM Version=0x%08X\n", rom_ver);
#endif

#ifdef DX_ROM_VERSION
	/* Check SRAM threshold */
	sram_threshold = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_SRAM_THRESHOLD));
	if ((sram_threshold == 0) ||
	    (sram_threshold >= DX_CC_SRAM_SIZE) ||
	    ((sram_threshold & 0x3) != 0)) {
		TEST_LOG_ERR("Invalid SRAM threshold 0x%08X (SRAM_SIZE=0x%08X)\n",
			sram_threshold, DX_CC_SRAM_SIZE);
		rc = -EINVAL;
	}
	TEST_LOG_TRACE("  SRAM Threshold = 0x%08X\n", sram_threshold);
#endif /*DX_ROM_VERSION*/

	return rc;
}

static int init_cc_resources(struct platform_device *plat_dev)
{
	struct tester_ctx *tctx_p;
	struct resource *req_mem_cc_regs = NULL;
	bool isr_registered = false;
	uint32_t tmp;
	int rc;

	tctx_p = kzalloc(sizeof(struct tester_ctx), GFP_KERNEL);
	if (unlikely(tctx_p == NULL)) {
		TEST_LOG_ERR("Failed allocating memory for tester context.\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&plat_dev->dev, tctx_p);
	tctx_p->dev = &plat_dev->dev;

	/* Get device resources */
	/* First CC registers space */
	tctx_p->res_mem = platform_get_resource(plat_dev, IORESOURCE_MEM, 0);
	if (unlikely(tctx_p->res_mem == NULL)) {
		TEST_LOG_ERR("Failed getting IO memory resource for CC registers\n");
		rc = -ENODEV;
		goto init_cc_res_err;
	}
	TEST_LOG_TRACE("Got CC MEM resource (%s): start=0x%llX end=0x%llX\n",
		tctx_p->res_mem->name,
		(unsigned long long)tctx_p->res_mem->start,
		(unsigned long long)tctx_p->res_mem->end);
	/* Map registers space */
	req_mem_cc_regs = request_mem_region(tctx_p->res_mem->start, resource_size(tctx_p->res_mem), "dx_cc44p_regs");
	if (unlikely(req_mem_cc_regs == NULL)) {
		TEST_LOG_ERR("Couldn't allocate registers memory region of size 0x%08X at 0x%08X\n",
			(unsigned int)tctx_p->res_mem->start, (unsigned int)resource_size(tctx_p->res_mem));
		rc = -EBUSY;
		goto init_cc_res_err;
	}
	tctx_p->cc_base = ioremap(tctx_p->res_mem->start, resource_size(tctx_p->res_mem));
	if (unlikely(tctx_p->cc_base == NULL)) {
		TEST_LOG_ERR("ioremap[CC](0x%08X,0x%08X) failed\n",
			(unsigned int)tctx_p->res_mem->start, (unsigned int)resource_size(tctx_p->res_mem));
		rc = -ENOMEM;
		goto init_cc_res_err;
	}
	TEST_LOG_TRACE("CC registers mapped from bus address 0x%08X to virtual address 0x%p\n",
		(unsigned int)tctx_p->res_mem->start, tctx_p->cc_base);
	rc = sanity_checks(tctx_p);
	if (unlikely(rc != 0))
		goto init_cc_res_err;

	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CACHE_PARAMS), cache_params);
	tmp = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CACHE_PARAMS));
	if (tmp != cache_params) {
		TEST_LOG_ERR("Failed setting cache parameters. Wrote 0x%08x but read 0x%08X\n",
			cache_params, tmp);
		rc = -EIO;
		goto init_cc_res_err;
	}
	TEST_LOG_STATUS("Cache params set to 0x%08X\n", tmp);
#ifdef CONFIG_ARM
	if (dma_coherent_ops != 0) {
		TEST_LOG_STATUS("Enabling coherent DMA operations.\n");
		set_dma_ops(tctx_p->dev, &arm_coherent_dma_ops);
		if (cache_params == 0)
			TEST_LOG_ERR("* Using coherent DMA operations with cache params set to 0!\n");
	} else {
		TEST_LOG_STATUS("Using default dma_ops.\n");
		if (cache_params != 0)
			TEST_LOG_ERR("* Using default DMA operations with cache params set to 0x%08X!\n", cache_params);
	}
#else
//#warning Not an ARM(32) platform - Using default DMA operations
	TEST_LOG_STATUS("Using default dma_ops.\n");
#endif
	/* Setup IRQ resources */
	init_completion(&tctx_p->desc_comp_event); /* For signaling from ISR */
	tctx_p->res_irq = platform_get_resource(plat_dev, IORESOURCE_IRQ, 0);
	if (unlikely(tctx_p->res_irq == NULL)) {
		TEST_LOG_ERR("Failed getting IRQ resource\n");
		rc = -ENODEV;
		goto init_cc_res_err;
	}
	rc = request_irq(tctx_p->res_irq->start, cc_isr, IRQF_SHARED, "cc_ll_test", tctx_p);
	if (unlikely(rc != 0)) {
		TEST_LOG_ERR("Could not register to interrupt %d\n", (unsigned int)tctx_p->res_irq->start);
		goto init_cc_res_err;
	}
	isr_registered = true;
	/* Clear all pending interrupts */
	TEST_LOG_TRACE("  IRR=0x%08X\n", READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR)));
	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_ICR), ~0);
	/* Unmask all AXI interrupt sources in AXI_CFG1 register */
	tmp = READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1));
	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1), tmp & ~DX_AXI_IRQ_MASK);
	TEST_LOG_TRACE("  AXIM_CFG1=0x%08X\n", READ_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(CRY_KERNEL, AXIM_CFG1)));
	/* Unmask relevant interrupt cause bits */
	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR),
		~TEST_IRQ_MASK);

	return 0;

init_cc_res_err:
	if (isr_registered) {
		free_irq(tctx_p->res_irq->start, tctx_p);
		tctx_p->res_irq = NULL;
	}
	if (tctx_p->cc_base != NULL) {
		iounmap(tctx_p->cc_base);
		tctx_p->cc_base = NULL;
	}
	if (req_mem_cc_regs != NULL) {
		release_mem_region(tctx_p->res_mem->start, resource_size(tctx_p->res_mem));
		tctx_p->res_mem = NULL;
	}

	return rc;
}

static void cleanup_cc_resources(struct platform_device *plat_dev)
{
	struct tester_ctx *tctx_p = (struct tester_ctx *)dev_get_drvdata(&plat_dev->dev);

	TEST_LOG_STATUS("Cleaning up driver resources.\n");
	/* Mask all interrupts */
	WRITE_REGISTER(tctx_p->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_IMR), ~0);
	/* Free IRQ resources */
	free_irq(tctx_p->res_irq->start, tctx_p);
	tctx_p->res_irq = NULL;
	/* Free memory resources */
	iounmap(tctx_p->cc_base);
	tctx_p->cc_base = NULL;
	release_mem_region(tctx_p->res_mem->start, resource_size(tctx_p->res_mem));
	tctx_p->res_mem = NULL;
}

static void dump_arm_info(void)
{
#if defined(CONFIG_ARM) && defined(SECURE_LINUX)
	uint32_t cp15_val, cacheline_size;
	uint32_t ccsidr, midr, scr, nsacr;

	TEST_LOG_TRACE("ARM core info:\n");
	asm volatile("mrc p15, 0, %0, c0, c0, 1" : "=r" (cp15_val));
	cacheline_size =  4 << ((cp15_val >> 16) & 0xf);
	TEST_LOG_TRACE("  CP15(L1_CACHE_BYTES) = %u , Kconfig(L1_CACHE_BYTES) = %u\n",
		cacheline_size, L1_CACHE_BYTES);
	asm volatile("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));
	asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (midr));
	asm volatile("mrc p15, 0, %0, c1, c1, 0" : "=r" (scr));
	asm volatile("mrc p15, 0, %0, c1, c1, 2" : "=r" (nsacr));
	TEST_LOG_TRACE("  CCSIDR=0x%08X MIDR=0x%08X SCR=0x%08X NSACR=0x%08X\n", ccsidr, midr, scr, nsacr);
#endif
}

static int ccll_tester_probe(struct platform_device *plat_dev)
{
	int rc, tests_cnt, failed_tests = 0;
	struct tester_ctx *tctx_p;
	struct test_case cur_test_case;

	dump_arm_info();
	TEST_LOG_TRACE("Creating test device\n");
	
	rc = init_cc_resources(plat_dev);
	if (unlikely(rc != 0))
		return rc;
	tctx_p = dev_get_drvdata(&plat_dev->dev);
	if (test_idx < 0) { /* Run all tests */
		get_test_case(0, &cur_test_case);
		for (tests_cnt = 0; cur_test_case.dma_size > 0; tests_cnt++) {
			TEST_LOG_STATUS("%d. size=%lu  din_offset=%lu  dout_offset=%lu\n",
				tests_cnt, cur_test_case.dma_size, cur_test_case.dma_din_page_offset, cur_test_case.dma_dout_page_offset);
			rc = exec_test_case(tctx_p, &cur_test_case);
			if (unlikely(rc != 0))
				failed_tests++;
			get_test_case(tests_cnt + 1, &cur_test_case); /* Get next case */
		}
	} else { /* Run specific test case */
		get_test_case(test_idx, &cur_test_case);
		if (cur_test_case.dma_size == 0) {
			TEST_LOG_ERR("Invalid test_idx parameter = %d.\n", test_idx);
			rc = -EINVAL;
		} else {
			TEST_LOG_STATUS("%d. size=%lu  din_offset=%lu  dout_offset=%lu\n",
				test_idx, cur_test_case.dma_size, cur_test_case.dma_din_page_offset, cur_test_case.dma_dout_page_offset);
			rc = exec_test_case(tctx_p, &cur_test_case);
			if (unlikely(rc != 0))
				failed_tests++;
		}
		tests_cnt = 1;
	}
	if (failed_tests > 0) {
		TEST_LOG_STATUS("%d of %d tests failed!\n", failed_tests, tests_cnt);
		cleanup_cc_resources(plat_dev);
	} else {
		if (tests_cnt > 1)
			TEST_LOG_STATUS("All (%d) tests passed.\n", tests_cnt);
		else /* Single test run */
			TEST_LOG_STATUS("Test #%d passed.\n", test_idx);
	}
	while(1)
		printk("test end\n");
	return (failed_tests > 0) ? -EIO : 0; /* Return error if any test failed */
}

static int ccll_tester_remove(struct platform_device *plat_dev)
{
	TEST_LOG_STATUS("Test device is removed.\n");
	cleanup_cc_resources(plat_dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ssi_dev_of_match[] = {
	{.compatible = "mediatek,dxcc_pub"},
	{}
};
MODULE_DEVICE_TABLE(of, ssi_dev_of_match);
#endif

static struct platform_driver ccll_tester_driver = {
	.driver = {
		   .name = "ssi_cc_ll_tester",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ssi_dev_of_match,
#endif
	},
	.probe = ccll_tester_probe,
	.remove = ccll_tester_remove,
};

int __init ccll_tester_init(void)
{
	int rc;

	TEST_LOG_STATUS("ARM CC low-level tester module loaded.\n");
	rc = platform_driver_register(&ccll_tester_driver);

	return rc;
}

void __exit ccll_tester_exit(void)
{
	platform_driver_unregister(&ccll_tester_driver);
	TEST_LOG_STATUS("Deregistered tester driver.\n");
	TEST_LOG_STATUS("ARM CC low-level tester module unloaded.\n");
}


/* Module info. */
module_init(ccll_tester_init);
module_exit(ccll_tester_exit);
MODULE_DESCRIPTION("ARM CryptoCell Integration Test Module");
MODULE_VERSION("1.0");
MODULE_AUTHOR("ARM");
MODULE_LICENSE("GPL v2");

