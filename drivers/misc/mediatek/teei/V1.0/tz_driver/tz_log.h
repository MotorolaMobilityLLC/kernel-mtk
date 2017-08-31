/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _TRUSTY_LOG_H_
#define _TRUSTY_LOG_H_

#define TZ_LOG_SIZE           (PAGE_SIZE * 64)
#define TZ_LINE_BUFFER_SIZE   256

#define TZ_LOG_RATELIMIT_INTERVAL	(1 * HZ)
#define TZ_LOG_RATELIMIT_BURST		200

/*
 * Ring buffer that supports one secure producer thread and one
 * linux side consumer thread.
 */
struct log_rb {
	volatile uint32_t alloc;
	volatile uint32_t put;
	uint32_t sz;
	volatile char data[0];
} __packed;


struct tz_log_state {
	struct device *dev;

	/*
	 * This lock is here to ensure only one consumer will read
	 * from the log ring buffer at a time.
	 */
	spinlock_t lock;
	struct log_rb *log;
	uint32_t get;

	struct page *log_pages;

	struct notifier_block call_notifier;
	struct notifier_block panic_notifier;
	char line_buffer[TZ_LINE_BUFFER_SIZE];
};


int tz_log_probe(struct platform_device *pdev);
int tz_log_remove(struct platform_device *pdev);

#endif

