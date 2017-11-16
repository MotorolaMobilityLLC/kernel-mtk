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
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/signal.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <mt-plat/hw_watchpoint.h>
#include "hw_watchpoint_internal.h"

#if 0
#define WATCH_DBG
#endif

void __iomem **MT_HW_DBG_BASE;

static unsigned int nr_dbg;

unsigned int hw_watchpoint_nr(void)
{
	return nr_dbg;
}

#ifdef WATCH_DBG
static struct wp_event wp_event;
static int err;
static int my_watch_data;
static int my_watch_data2;
static int my_watch_data3;

int my_wp_handler1(unsigned int addr)
{
	pr_notice("Access my data from an instruction at 0x%x\n", addr);
	return 0;
}

int my_wp_handler2(unsigned int addr)
{
	pr_notice("Access my data from an instruction at 0x%x\n", addr);
	/* trigger exception */
	return 1;
}

void foo(void)
{
	int test;

	init_wp_event(&wp_event, &my_watch_data, &my_watch_data,
		      WP_EVENT_TYPE_ALL, my_wp_handler1);

	err = add_hw_watchpoint(&wp_event);
	if (err) {
		pr_err("add hw watch point failed...\n");
		/* fail to add watchpoing */
	} else {
		/* the memory address is under watching */
		pr_notice("add hw watch point success...\n");

		/* del_hw_watchpoint(&wp_event); */
	}
	/* test watchpoint */
	my_watch_data = 1;
}

void foo2(void)
{
	int test;

	init_wp_event(&wp_event, &my_watch_data2, &my_watch_data2,
		      WP_EVENT_TYPE_ALL, my_wp_handler1);

	err = add_hw_watchpoint(&wp_event);
	if (err) {
		pr_err("add hw watch point failed...\n");
		/* fail to add watchpoing */
	} else {
		/* the memory address is under watching */
		pr_notice("add hw watch point success...\n");

		/* del_hw_watchpoint(&wp_event); */
	}
	/* test watchpoint */
	my_watch_data2 = 2;
}

void foo3(void)
{
	int test;

	init_wp_event(&wp_event, &my_watch_data3, &my_watch_data3,
		      WP_EVENT_TYPE_ALL, my_wp_handler1);

	err = add_hw_watchpoint(&wp_event);
	if (err) {
		pr_err("add hw watch point failed...\n");
		/* fail to add watchpoing */
	} else {
		/* the memory address is under watching */
		pr_notice("add hw watch point success...\n");

		/* del_hw_watchpoint(&wp_event); */
	}

	/* test watchpoint */
	my_watch_data3 = 3;
}

#endif

static struct wp_event *wp_events;
static spinlock_t wp_lock;

/*
 * enable_hw_watchpoint: Enable the H/W watchpoint.
 * Return error code.
 */
int enable_hw_watchpoint(void)
{
	int i;

	for_each_online_cpu(i) {
		if (readl(DBGDSCR(i)) & HDBGEN) {
			pr_err
			    ("halting dbg. Unable to access HW.\n");
			return -EPERM;
		}

		if (readl(DBGDSCR(i)) & MDBGEN) {
			pr_notice("already enabled, DBGDSCR = 0x%x\n",
				  readl(DBGDSCR(i)));
		}

		writel(UNLOCK_KEY, DBGLAR(i));
		writel(~UNLOCK_KEY, DBGOSLAR(i));
		writel(readl(DBGDSCR(i)) | MDBGEN, DBGDSCR(i));
	}

	return 0;
}

/*
 * add_hw_watchpoint: add a watch point.
 * @wp_event: pointer to the struct wp_event.
 * Return error code.
 */
int add_hw_watchpoint(struct wp_event *wp_event)
{
	int ret, i, j;
	unsigned long flags;
	unsigned int ctl;

	if (!wp_event)
		return -EINVAL;

	if (!(wp_event->handler))
		return -EINVAL;

	ret = enable_hw_watchpoint();
	if (ret)
		return ret;

	ctl = DBGWCR_VAL;
	if (wp_event->type == WP_EVENT_TYPE_READ)
		ctl |= LSC_LDR;
	else if (wp_event->type == WP_EVENT_TYPE_WRITE)
		ctl |= LSC_STR;
	else if (wp_event->type == WP_EVENT_TYPE_ALL)
		ctl |= LSC_ALL;
	else
		return -EINVAL;

	spin_lock_irqsave(&wp_lock, flags);
	for (i = 0; i < nr_dbg; i++) {
		if (!wp_events[i].in_use) {
			wp_events[i].in_use = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&wp_lock, flags);

	if (i == nr_dbg)
		return -EAGAIN;

	/* enforce word-aligned */
	wp_events[i].virt = wp_event->virt & ~3;
	wp_events[i].phys = wp_event->phys;	/* no use currently */
	wp_events[i].type = wp_event->type;
	wp_events[i].handler = wp_event->handler;
	wp_events[i].auto_disable = wp_event->auto_disable;

	pr_notice("Add watchpoint %d at address 0x%x\n", i, wp_events[i].virt);
	for_each_online_cpu(j) {
		writel(wp_events[i].virt, DBGWVR_BASE(j) + i * 4);
		writel(ctl, DBGWCR_BASE(j) + i * 4);
	}

	return 0;
}
EXPORT_SYMBOL(add_hw_watchpoint);

/*
 * del_hw_watchpoint: delete a watch point.
 * @wp_event: pointer to the struct wp_event.
 * Return error code.
 */
int del_hw_watchpoint(struct wp_event *wp_event)
{
	unsigned long flags;
	int i, j;

	if (!wp_event)
		return -EINVAL;

	spin_lock_irqsave(&wp_lock, flags);
	for (i = 0; i < nr_dbg; i++) {
		if (wp_events[i].in_use
		    && (wp_events[i].virt == wp_event->virt)) {
			wp_events[i].virt = 0;
			wp_events[i].phys = 0;
			wp_events[i].type = 0;
			wp_events[i].handler = NULL;
			wp_events[i].in_use = 0;
			for_each_online_cpu(j) {
				writel(readl(DBGWCR_BASE(j) + i * 4) & (~WP_EN),
				       DBGWCR_BASE(j) + i * 4);
			}
			break;
		}
	}
	spin_unlock_irqrestore(&wp_lock, flags);

	if (i == nr_dbg)
		return -EINVAL;
	else
		return 0;
}

int watchpoint_handler(unsigned long addr, unsigned int fsr,
		       struct pt_regs *regs)
{
	unsigned int wfar, daddr, iaddr;
	int i, ret, j;

	asm volatile ("MRC p15, 0, %0, c6, c0, 0\n":"=r" (wfar) : : "cc");

	daddr = addr & ~3;
	iaddr = regs->ARM_pc;
	pr_err("[WP] addr = 0x%x, DBGWFAR/DFAR = 0x%x\n",
		(unsigned int)addr, wfar);
	pr_err("[WP] daddr = 0x%x, iaddr = 0x%x\n", daddr, iaddr);

	/* update PC to avoid re-execution of the instruction under watching */
	regs->ARM_pc += thumb_mode(regs) ? 2 : 4;

	for (i = 0; i < nr_dbg; i++) {
		if (wp_events[i].in_use && wp_events[i].virt == (daddr)) {
			pr_notice("Watchpoint %d triggers.\n", i);
			if (wp_events[i].handler) {
				if (wp_events[i].auto_disable) {
					for_each_online_cpu(j) {
						writel(readl
						       (DBGWCR_BASE(j) +
							i * 4) & (~WP_EN),
						       DBGWCR_BASE(j) + i * 4);
					}
				}
				ret = wp_events[i].handler(iaddr);
				if (wp_events[i].auto_disable) {
					for_each_online_cpu(j) {
						writel(readl
						       (DBGWCR_BASE(j) +
							i * 4) | (WP_EN),
						       DBGWCR_BASE(j) + i * 4);
					}
				}
				return ret;
			}
			pr_notice("No watchpoint handler. Ignore.\n");
			return 0;
		}
	}

	return 0;
}

#ifdef WATCH_DBG
static struct device_driver hw_drv = {
	.name = "hw_watchpoint",
	.bus = &platform_bus_type,
	.owner = THIS_MODULE
};

static ssize_t watchpoint_test_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "currently run on cpu: %d\n",
			smp_processor_id());
}

static ssize_t watchpoint_test_store(struct device_driver *driver,
				     const char *buf, size_t count)
{
	char *p = (char *)buf;
	unsigned long num;
	int i = 0, j = 0;

	if (kstrtoul(p, 10, &num) != 0) {
		pr_err("[WP] convert %s to unsigned long failed.\n", p);
		return -1;
	}

	switch (num) {
	case 1:
		foo3();
		break;
	case 2:
		my_watch_data3 = 1;
		break;
	case 3:
	default:
		break;
	}

	return count;
}

DRIVER_ATTR(watchpoint_test, 0644, watchpoint_test_show, watchpoint_test_store);
#endif

static int __init hw_watchpoint_init(void)
{
	struct device_node *node;
	int i = 0;
#ifdef WATCH_DBG
	int ret;
#endif
	spin_lock_init(&wp_lock);
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6580-dbg_debug");
	if (!node) {
		pr_warn("[WP] can not find DBG_DEBUG dts node\n");
		return -1;
	}

	of_property_read_u32(node, "num", &nr_dbg);
	pr_notice("[WP] get %d debug interface\n", nr_dbg);

	MT_HW_DBG_BASE = kcalloc(nr_dbg, sizeof(void __iomem *), GFP_KERNEL);
	wp_events = kcalloc(nr_dbg, sizeof(struct wp_event), GFP_KERNEL);

	for (i = 0; i < nr_dbg; ++i) {
		MT_HW_DBG_BASE[i] = of_iomap(node, i);
		if (!MT_HW_DBG_BASE[i]) {
			pr_err("[WP] of_iomap failed for MT_HW_DBG_BASE[%d]\n",
			       i);
			return -ENOMEM;
		}
	}
#ifdef WATCH_DBG

	ret = driver_register(&hw_drv);
	if (ret)
		pr_err("Fail to register mt_hw_drv");

	ret = driver_create_file(&hw_drv, &driver_attr_watchpoint_test);
	if (ret)
		pr_err("Fail to create mt_hw_drv sysfs files");

	driver_create_file(&hw_drv, &driver_attr_watchpoint_test);
#endif
#ifdef CONFIG_ARM_LPAE
	hook_fault_code(0x22, watchpoint_handler, SIGTRAP, 0,
			"watchpoint debug exception");
#else
	hook_fault_code(0x2, watchpoint_handler, SIGTRAP, 0,
			"watchpoint debug exception");
#endif
	pr_notice("[WP] watchpoint handler init.\n");
#ifdef WATCH_DBG
	foo();
	foo2();
#endif

	return 0;
}
arch_initcall(hw_watchpoint_init);
