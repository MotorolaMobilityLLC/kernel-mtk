/*
 * Hardware Compressed RAM block device
 * Copyright (C) 2015 Google Inc.
 *
 * This file contains the tests
 *
 * Sonny Rao <sonnyrao@chromium.org>
 *
 */

#ifdef CONFIG_HWZRAM_DEBUG
#define DEBUG
#endif

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <asm/page.h>
#include "hwzram_impl.h"


/* find an already probed hwzram instance */
struct hwzram_impl *hwzram_get_impl(void);

static int __init hwzram_init(void)
{
	int ret = 0;

	return ret;
}

static void __exit hwzram_impl_test_exit(void)
{
	int i;
}

module_init(hwzram_impl_test_init);
module_exit(hwzram_impl_test_exit);
