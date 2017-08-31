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
 *
 * Note:
 * It copies the template of hwzram_impl_probe.c with the following
 * disclaimer,
 *
 * * Hardware Compressed RAM block device
 * * Copyright (C) 2015 Google Inc.
 * *
 * * Sonny Rao <sonnyrao@chromium.org>
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/fb.h>
#include <asm/irqflags.h>

/* hwzram impl header file */
#include "../hwzram_impl.h"

/* UFOZIP private header file */
#include "ufozip_private.h"

#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
#include <mtk_vcorefs_manager.h>
static struct task_struct *ufozip_task;
#endif

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#define MTK_UFOZIP_USING_CCF
#else
#include <mach/mt_clkmgr.h>  /* For clock mgr APIS. enable_clock() & disable_clock(). */
#endif

#ifdef MTK_UFOZIP_USING_CCF
struct clk *g_ufoclk_enc_sel;
struct clk *g_ufoclk_high_624;
struct clk *g_ufoclk_low_312;
struct clk *g_ufoclk_clk;
#endif

static DEFINE_SPINLOCK(lock);
static int hclk_count;

#define UFOZIP_IN_SCREEN_ON	(0x1)
#define UFOZIP_IN_SCREEN_OFF	(0x0)
static atomic_t ufozip_screen_on;

/* static unsigned int ufozip_default_fifo_size = 64; */
static unsigned int ufozip_default_fifo_size = 256;

/* Write to 32-bit register */
static void ufozip_write_register(struct hwzram_impl *hwz, unsigned int offset, uint64_t val)
{
	writel(val, hwz->regs + offset);
}

#if 0 /* Unused */
/* Read from 32-bit register */
static uint32_t ufozip_read_register(struct hwzram_impl *hwz, unsigned int offset)
{
	uint32_t ret;

	ret = readl(hwz->regs + offset);

	return ret;
}
#endif

/* Configure UFOZIP */
static void set_apb_default_normal(struct hwzram_impl *hwz, UFOZIP_CONFIG *uzcfg)
{
	int val;
	int axi_max_rout, axi_max_wout;

	/*
	 * bit[0] : enable clock for encoder and AXI
	 * bit[16]: bypass enc latency
	 */
	ufozip_write_register(hwz, ENC_DCM, 0x10001);

	/*
	 * bit[0] : enable clock for decoder and AXI
	 * bit[4] : bypass dec latency
	 */
	ufozip_write_register(hwz, DEC_DCM, 0x11);

	/*
	 * bit[12..0] : blocksize
	 * bit[25..20]: cutvalue
	 * bit[26]    : componly mode
	 * bit[27]    : refbyte_en
	 * bit[31..28]: level
	 */
	ufozip_write_register(hwz, ENC_CUT_BLKSIZE,
			(uzcfg->blksize >> 4) |
			(uzcfg->cutValue << 20) |
			(uzcfg->componly_mode << 26) |
			(uzcfg->compress_level << 28) |
			(uzcfg->refbyte_flag << 27) |
			(uzcfg->hybrid_decode << 31));

	/*
	 * bit[7..0]  : lenpos_max
	 * bit[24..8] : limitsize_add16
	 */
	ufozip_write_register(hwz, ENC_LIMIT_LENPOS,
			(uzcfg->limitSize << 8) |
			(uzcfg->hwcfg.lenpos_max));

	/* ligzh, fix default value of usmin and intf_us_margin */
	if (uzcfg->hwcfg.usmin < 0x120)
		uzcfg->hwcfg.usmin = 0x120;
	else if (uzcfg->hwcfg.usmin > 0xecf)
		uzcfg->hwcfg.usmin = 0xecf;

	/* For debug, default: 0x120 */
	ufozip_write_register(hwz, ENC_DSUS,
			(uzcfg->hwcfg.dsmax << 16) |
			(uzcfg->hwcfg.usmin));

	/* intf_us_margin */
	if (uzcfg->hwcfg.enc_intf_usmargin < (uzcfg->hwcfg.usmin + 0x111))
		uzcfg->hwcfg.enc_intf_usmargin = uzcfg->hwcfg.usmin + 0x111;
	else if (uzcfg->hwcfg.enc_intf_usmargin > 0xfa0)
		uzcfg->hwcfg.enc_intf_usmargin = 0xfa0;

	/* us guard margin, default 0x1000 */
	ufozip_write_register(hwz, ENC_INTF_USMARGIN, uzcfg->hwcfg.enc_intf_usmargin);

	/* Read/Write threshold */
	ufozip_write_register(hwz, ENC_INTF_RD_THR, uzcfg->blksize + 128);
	val = (uzcfg->limitSize + 128) >= 0x20000 ? 0x1ffff : (uzcfg->limitSize + 128);
	ufozip_write_register(hwz, ENC_INTF_WR_THR, val);

	/* set "time out value" */
	if (uzcfg->hybrid_decode)
		ufozip_write_register(hwz, DEC_TMO_THR, 0xFFFFFFFF);

#if defined(MT5891)
	/* Unnecessary */
	ufozip_write_register(hwz, ENC_AXI_PARAM0, 0x01023023);
	ufozip_write_register(hwz, DEC_AXI_PARAM0, 0x01023023);
	ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x1);
	ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x1);
	ufozip_write_register(hwz, ENC_AXI_PARAM2, 0x100);
	ufozip_write_register(hwz, DEC_AXI_PARAM2, 0x100);
	ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0404);
	ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0404);
#else	    /* end 5891 axi parameters */
	/* AXI Misc parameters 1, for debug */
	ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x10);
	ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x10);
#endif

	/*
	 * Default 0x1222.
	 * In normal mode, 0x1222 for 4KB block size; 0x1000 for block size > 4KB.
	 * In debug mode, minimum 0x330.
	 */
	ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, uzcfg->dictsize);

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable core_abnormal( exceed limitsize) interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 * bit[6]: set to 1 disable zram block interrupt
	 */
	ufozip_write_register(hwz, ENC_INT_MASK, 0x4);

	/* UFOZIP_HWCONFIG */
	switch (uzcfg->hwcfg_sel) {
	case HWCFG_INTF_BUS_BL:
		/* AXI read/write burst length */
		ufozip_write_register(hwz, ENC_INTF_BUS_BL, uzcfg->hwcfg.intf_bus_bl);
		ufozip_write_register(hwz, DEC_INTF_BUS_BL, uzcfg->hwcfg.intf_bus_bl);
		break;
	case HWCFG_TMO_THR:
		/* Timeout */
		ufozip_write_register(hwz, ENC_TMO_THR, uzcfg->hwcfg.tmo_thr);
		ufozip_write_register(hwz, DEC_TMO_THR, uzcfg->hwcfg.tmo_thr);
		break;
	case HWCFG_ENC_DSUS:
		/* For debug, default: 0x120 */
		ufozip_write_register(hwz, ENC_DSUS, uzcfg->hwcfg.enc_dsus);
		break;
	case HWCFG_ENC_LIMIT_LENPOS:
		/* ENC_LIMIT_LENPOS */
		ufozip_write_register(hwz, ENC_LIMIT_LENPOS, (uzcfg->limitSize << 8) |
							(uzcfg->hwcfg.lenpos_max));
		ufozip_write_register(hwz, ENC_INT_MASK, 0x4);
		break;
	case HWCFG_ENC_INTF_USMARGIN:
		/* us guard margin, default 0x1000 */
		ufozip_write_register(hwz, ENC_INTF_USMARGIN, uzcfg->hwcfg.enc_intf_usmargin);
		break;
	case HWCFG_AXI_MAX_OUT:
		axi_max_rout = (uzcfg->hwcfg.axi_max_out & 0xFF000000) | 0x00023023;
		axi_max_wout = uzcfg->hwcfg.axi_max_out & 0x000000FF;
		/* AXI Misc parameters 0. AXI side info and max outstanding. */
		ufozip_write_register(hwz, ENC_AXI_PARAM0, axi_max_rout);
		ufozip_write_register(hwz, DEC_AXI_PARAM0, axi_max_rout);
		/* AXI Misc parameters 1, for debug */
		ufozip_write_register(hwz, ENC_AXI_PARAM1, axi_max_wout);
		ufozip_write_register(hwz, DEC_AXI_PARAM1, axi_max_wout);
		break;
	case HWCFG_INTF_SYNC_VAL:
		/* For debug */
		ufozip_write_register(hwz, ENC_INTF_SYNC_VAL, uzcfg->hwcfg.intf_sync_val);
		ufozip_write_register(hwz, DEC_INTF_SYNC_VAL, uzcfg->hwcfg.intf_sync_val);
		break;
	case HWCFG_DEC_DBG_INBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, (uzcfg->hwcfg.dec_dbg_inbuf_size & 0x1FFFF));
		break;
	case HWCFG_DEC_DBG_OUTBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, (uzcfg->hwcfg.dec_dbg_outbuf_size & 0x1FFFF));
		break;
	case HWCFG_DEC_DBG_INNERBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_INNERBUF_SIZE, (uzcfg->hwcfg.dec_dbg_innerbuf_size & 0x7F));
		break;
	case HWCFG_CORNER_CASE:
		{
			UInt32	lenpos_max;

			ufozip_write_register(hwz, ENC_DCM, 1);
			lenpos_max = 0x2;
			ufozip_write_register(hwz, ENC_LIMIT_LENPOS, (uzcfg->limitSize << 8) |
								(lenpos_max));
			ufozip_write_register(hwz, ENC_INTF_RD_THR, uzcfg->blksize + 128);
			val = (uzcfg->limitSize + 128) >= 0x20000 ? 0x1ffff : (uzcfg->limitSize + 128);
			ufozip_write_register(hwz, ENC_INTF_WR_THR, val);

#if defined(MT5891)
			ufozip_write_register(hwz, ENC_AXI_PARAM0, 0x08023023);
			ufozip_write_register(hwz, DEC_AXI_PARAM0, 0x08023023);
			ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x1);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x1);
			ufozip_write_register(hwz, ENC_AXI_PARAM2, 0x100);
			ufozip_write_register(hwz, DEC_AXI_PARAM2, 0x100);
			ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0404);
			ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0404);
#else	    /* end 5891 axi parameters */
			ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x10);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x10);
#endif
			ufozip_write_register(hwz, ENC_INT_MASK, 0x4); /* mask exceed limitsize irq for simu */

			ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0202);
			ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0202);

			axi_max_rout = 0x01023023; /* 0x01000000 | 0x00023023; //0x00023023:default value in [23:0] */
			axi_max_wout = 0x1; /* tc_data & 0x000000FF; */
			ufozip_write_register(hwz, ENC_AXI_PARAM0, axi_max_rout);
			ufozip_write_register(hwz, DEC_AXI_PARAM0, axi_max_rout);
			ufozip_write_register(hwz, ENC_AXI_PARAM1, axi_max_wout);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, axi_max_wout);

			ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, 0x130);
			ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, 0x330);
			ufozip_write_register(hwz, DEC_DBG_INNERBUF_SIZE, 0x2);

			ufozip_write_register(hwz, DEC_INT_MASK, 0xFFFFFFDE);  /* open only dec done and rst_done */
			ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFDE);  /* open only dec done and rst_done */
		}
		break;
	}

	/* Is it hybrid decoding */
	if (uzcfg->hybrid_decode)
		uzcfg->dictsize	= DEC_DICTSIZE;
	else
		if (uzcfg->dictsize > (USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin))
			uzcfg->dictsize	= USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin - 48;

	ufozip_write_register(hwz, DEC_CONFIG, (uzcfg->blksize >> 4) |
					  (uzcfg->refbyte_flag << 20) |
					  (uzcfg->hybrid_decode << 21));
	ufozip_write_register(hwz, DEC_CONFIG2, uzcfg->dictsize);

	ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, uzcfg->hwcfg.dec_dbg_inbuf_size);

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable dec_err interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 */
	ufozip_write_register(hwz, DEC_INT_MASK, 0xFFFFFFFA);

	/* Mode for ZRAM accelerator */
	if (uzcfg->batch_mode == 3)
		ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFF9E); /* open only enc done and rst_done */
	else
		ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFDE); /* open only enc done and rst_done */

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable core_abnormal( exceed limitsize) interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 * bit[6]: set to 1 disable zram block interrupt (equal to ZRAM_INTRP_MASK_CMP)
	 */
	ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFBB);

	ufozip_write_register(hwz, ENC_PROB_STEP, (uzcfg->dictsize << 15) |
					      (uzcfg->hashmask << 2)|
					      (uzcfg->probstep));
	ufozip_write_register(hwz, DEC_PROB_STEP, uzcfg->probstep);
}

static void UFOZIP_HwInit(struct hwzram_impl *hwz)
{
	u32 batch_blocknum = 16;
	int zip_blksize = 4096;
	u32 limit_ii;

	UFOZIP_CONFIG   uzip_config;

	pr_info("%s start ...\n", __func__);

	limit_ii = 4096;
	pr_info("UFOZIP test blksize=%x, limitsize=%x\n", zip_blksize, limit_ii);

	uzip_config.compress_level	= 1;
	uzip_config.refbyte_flag	= 1;
	uzip_config.hwcfg.lenpos_max	= 0x7e;
	uzip_config.hwcfg.dsmax		= 0xff0;
	uzip_config.hwcfg.usmin		= 0x0120;
	uzip_config.hwcfg.enc_intf_usmargin	= uzip_config.hwcfg.usmin + 0x111;
	uzip_config.hwcfg.dec_dbg_inbuf_size	= 0x400;
	uzip_config.componly_mode	= 0;
	uzip_config.cutValue		= 8;
	uzip_config.blksize		= zip_blksize;
	uzip_config.dictsize		= 1 << 10;
	uzip_config.hashmask		= 2 * 1024 - 1;
	uzip_config.limitSize		= limit_ii;
	uzip_config.batch_mode		= 3;
	uzip_config.batch_blocknum	= batch_blocknum;
	uzip_config.batch_srcbuflen	= 16 * 4096;
	uzip_config.hwcfg_sel		= HWCFG_NONE;
	uzip_config.hwcfg.intf_bus_bl	= 0x00000404;
	uzip_config.probstep		= 0;
	uzip_config.hybrid_decode	= 0;
	set_apb_default_normal(hwz, &uzip_config);

	pr_info("%s finish ...\n", __func__);
}
static void ufozip_platform_init(struct hwzram_impl *hwz,
				 unsigned int vendor, unsigned int device)
{
	if (vendor == ZRAM_VENDOR_ID_MEDIATEK) {
		UFOZIP_HwInit(hwz);
		pr_info("%s: done\n", __func__);
	} else {
		pr_warn("%s: mismatched vendor %u, should be %u\n",
				__func__, vendor, ZRAM_VENDOR_ID_MEDIATEK);
	}
}

/* bus clock */
static int enable_ufozip_clock(void);
static void disable_ufozip_clock(void);
static void ufozip_hclkctrl(enum platform_ops ops)
{
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	if (ops == COMP_ENABLE || ops == DECOMP_ENABLE) {
		if (hclk_count++ == 0) {
#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
			wake_up_process(ufozip_task);
#endif
#ifdef MTK_UFOZIP_USING_CCF
			if (enable_ufozip_clock())
				pr_err("%s: ops(%d)\n", __func__, ops);
#endif
		}
	} else {
		if (--hclk_count == 0) {
#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
			wake_up_process(ufozip_task);
#endif
#ifdef MTK_UFOZIP_USING_CCF
			disable_ufozip_clock();
#endif
		}
	}
	spin_unlock_irqrestore(&lock, flags);
}

static void ufozip_clock_control(enum platform_ops ops)
{
	ufozip_hclkctrl(ops);
}

#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
/* vcore fs control - this may sleep */
static void ufozip_vcorectrl(bool set_high)
{
	int err = 0;

	if (set_high) {
		vcorefs_request_dvfs_opp(KIR_UFO, OPP_1);
		/* after this, ufozip can run at high freq */
		err = clk_set_parent(g_ufoclk_enc_sel, g_ufoclk_high_624);
	} else {
		err = clk_set_parent(g_ufoclk_enc_sel, g_ufoclk_low_312);
		/* before this, ufozip should run at low freq */
		vcorefs_request_dvfs_opp(KIR_UFO, OPP_UNREQ);
	}

	if (err)
		pr_warn("%s(%d)\n", __func__, (int)set_high);
}

static int ufozip_entry(void *p)
{
#define HIGH_FREQ	(0x2)
#define LOW_FREQ	(0x1)
#define KEEP_FREQ	(0x0)	/* no actions */
#define LOW_COUNT	(10)
#define TIME_AFTER	(HZ/LOW_COUNT/5)

	int curr_freq = LOW_FREQ;
	int set_freq = KEEP_FREQ;
	int requested_low_count = 0;
	unsigned long flags;

	/* Call freezer_do_not_count to skip me */
	freezer_do_not_count();

	/* Start actions */
	do {
		/* Start running */
		set_current_state(TASK_RUNNING);

		/* Check action */
		spin_lock_irqsave(&lock, flags);
		if (hclk_count > 0)
			set_freq = HIGH_FREQ;
		else
			set_freq = LOW_FREQ;
		spin_unlock_irqrestore(&lock, flags);

		/* If system is in screen-off, no promotion of clock */
		if (atomic_read(&ufozip_screen_on) == UFOZIP_IN_SCREEN_OFF) {
			if (curr_freq == LOW_FREQ) {
				requested_low_count = 0;
				goto no_action;
			}
			/* Set it to low freq directly */
			requested_low_count = LOW_COUNT;
			set_freq = LOW_FREQ;
		}

		/* Shall we apply action */
		if (set_freq == curr_freq)
			set_freq = KEEP_FREQ;

		/* One more check for LOW_FREQ action */
		if (set_freq == LOW_FREQ) {
			if (requested_low_count++ < LOW_COUNT)
				set_freq = KEEP_FREQ;
			/* If no actions in 0.2s, let it be in low freq. */
		} else {
			requested_low_count = 0;
		}

		/* Apply action */
		if (set_freq == LOW_FREQ) {
			ufozip_vcorectrl(false);
			requested_low_count = 0;
		} else if (set_freq == HIGH_FREQ) {
			ufozip_vcorectrl(true);
		}

		/* Record current level */
		if (set_freq != KEEP_FREQ)
			curr_freq = set_freq;

no_action:
		/* Schedule me */
		if (requested_low_count) {
			schedule_timeout_interruptible(TIME_AFTER);
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	} while (1);

	return 0;
}
#endif

#ifdef CONFIG_PM
/* FB event notifier */
static int ufozip_fb_event(struct notifier_block *notifier, unsigned long event, void *data)
{
	struct fb_event *fb_event = data;
	int new_status;

	if (event != FB_EVENT_BLANK)
		return NOTIFY_DONE;

	new_status = *(int *)fb_event->data ? 1 : 0;

	if (new_status == 0)
		atomic_set(&ufozip_screen_on, UFOZIP_IN_SCREEN_ON);
	else
		atomic_set(&ufozip_screen_on, UFOZIP_IN_SCREEN_OFF);

	pr_alert("%s: screen(%s)\n", __func__, new_status ? "off" : "on");

#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
	/* Wake up ufozip_task to check whether we should change clock */
	wake_up_process(ufozip_task);
#endif

	return NOTIFY_OK;
}

static struct notifier_block fb_notifier_block = {
	.notifier_call = ufozip_fb_event,
	.priority = 0,
};

static int __init ufozip_init_pm_ops(void)
{
	if (fb_register_client(&fb_notifier_block) != 0)
		return -1;

	return 0;
}
#endif

#ifdef CONFIG_OF

static int prepare_ufozip_clock(void);
static int ufozip_of_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hwzram_impl *hwz;
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int irq = platform_get_irq(pdev, 0);

	pr_devel("%s starting: got irq %d\n", __func__, irq);

	/*
	 * Set dma_mask -
	 * UFOZIP supports 34-bit address for accessing DRAM.
	 * Only higher 32bits(bit 33-2) can be configured and
	 * the lower 2bits(bit 1-0) are fixed zero.
	 * Thus, the addresses are 4-Byte's aligned.
	 */
	if (dma_set_mask(dev, DMA_BIT_MASK(34))) {
		pr_warn("%s: no dma_mask\n", __func__);
		return -EINVAL;
	}

	pr_info("ufozip_of_probe clock init ...\n");

#ifdef MTK_UFOZIP_USING_CCF
	g_ufoclk_enc_sel = devm_clk_get(&pdev->dev, "ufo_sel");
	WARN_ON(IS_ERR(g_ufoclk_enc_sel));

	g_ufoclk_high_624 = devm_clk_get(&pdev->dev, "clock_high_624");
	WARN_ON(IS_ERR(g_ufoclk_high_624));

	g_ufoclk_low_312 = devm_clk_get(&pdev->dev, "clock_low_312");
	WARN_ON(IS_ERR(g_ufoclk_low_312));

	g_ufoclk_clk = devm_clk_get(&pdev->dev, "ufo_clk");
	WARN_ON(IS_ERR(g_ufoclk_clk));
#endif

	if (prepare_ufozip_clock()) {
		pr_warn("%s: failed to prepare clock\n", __func__);
		return -EINVAL;
	}

	if (enable_ufozip_clock()) {
		pr_warn("%s: failed to enable clock\n", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_PM
	if (ufozip_init_pm_ops()) {
		pr_warn("%s: failed to init pm ops\n", __func__);
		return -EINVAL;
	}
#endif

	/* Screen-on by default */
	atomic_set(&ufozip_screen_on, UFOZIP_IN_SCREEN_ON);

#ifdef MTK_UFOZIP_VCORE_DVFS_CONTROL
	ufozip_task = kthread_run(ufozip_entry, NULL, "ufozip_task");
#endif

	/* Initialization of hwzram_impl */
	hwz = hwzram_impl_init(&pdev->dev, ufozip_default_fifo_size,
			mem->start, irq, ufozip_platform_init, ufozip_clock_control);

	disable_ufozip_clock();

	platform_set_drvdata(pdev, hwz);

	if (IS_ERR(hwz)) {
		pr_warn("%s: failed to initialize hwzram_impl\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int ufozip_of_remove(struct platform_device *pdev)
{
	struct hwzram_impl *hwz = platform_get_drvdata(pdev);

	hwzram_impl_destroy(hwz);

	return 0;
}

static const struct of_device_id ufozip_of_match[] = {
	{ .compatible = "mediatek,ufozip", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ufozip_of_match);

static int prepare_ufozip_clock(void)
{
#ifdef MTK_UFOZIP_USING_CCF
	int err;

	err = clk_prepare(g_ufoclk_clk);
	err |= clk_prepare(g_ufoclk_enc_sel);
	err |= clk_set_parent(g_ufoclk_enc_sel, g_ufoclk_low_312);

	return err;
#else
	return 0;
#endif
}

static void unprepare_ufozip_clock(void)
{
#ifdef MTK_UFOZIP_USING_CCF
	clk_unprepare(g_ufoclk_enc_sel);
	clk_unprepare(g_ufoclk_clk);
#endif
}

static int enable_ufozip_clock(void)
{
#ifdef MTK_UFOZIP_USING_CCF
	int err;

	err = clk_enable(g_ufoclk_clk);
	err |= clk_enable(g_ufoclk_enc_sel);

	return err;
#else
	return 0;
#endif
}

/* might sleep */
static void disable_ufozip_clock(void)
{
#ifdef MTK_UFOZIP_USING_CCF
	clk_disable(g_ufoclk_enc_sel);
	clk_disable(g_ufoclk_clk);
#endif
}

static int mt_ufozip_suspend(struct device *dev)
{
	struct hwzram_impl *hwz = dev_get_drvdata(dev);
	unsigned long flags;

	pr_info("%s\n", __func__);

	spin_lock_irqsave(&lock, flags);
	if (hclk_count) {
		spin_unlock_irqrestore(&lock, flags);
		pr_warn("%s: UFOZIP is in use.\n", __func__);
		return -1;
	}
	spin_unlock_irqrestore(&lock, flags);

	if (enable_ufozip_clock())
		pr_warn("%s: failed to enable clock\n", __func__);

	hwzram_impl_suspend(hwz);

	disable_ufozip_clock();

	unprepare_ufozip_clock();

	return 0;
}

static int mt_ufozip_resume(struct device *dev)
{
	struct hwzram_impl *hwz = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	if (prepare_ufozip_clock())
		pr_warn("%s: failed to prepare clock\n", __func__);

	if (enable_ufozip_clock())
		pr_warn("%s: failed to enable clock\n", __func__);

	UFOZIP_HwInit(hwz); /* SPM ON @ INIT*/
	hwzram_impl_resume(hwz);

	disable_ufozip_clock();

	return 0;
}

static const struct dev_pm_ops mt_ufozip_pm_ops = {
	.suspend = mt_ufozip_suspend,
	.resume = mt_ufozip_resume,
	.freeze = mt_ufozip_suspend,
	.thaw = mt_ufozip_resume,
	.restore = mt_ufozip_resume,
};

static struct platform_driver ufozip_of_driver = {
	.probe		= ufozip_of_probe,
	.remove		= ufozip_of_remove,
	.driver		= {
		.name	= "ufozip",
		.pm = &mt_ufozip_pm_ops,
		.of_match_table = of_match_ptr(ufozip_of_match),
	},
};

module_platform_driver(ufozip_of_driver);
#endif

module_param(ufozip_default_fifo_size, uint, 0644);
MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Hardware Memory compression accelerator UFOZIP");
