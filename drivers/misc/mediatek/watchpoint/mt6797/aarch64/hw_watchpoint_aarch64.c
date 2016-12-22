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
#include <asm/signal.h>
#include <asm/ptrace.h>
#include "hw_watchpoint_aarch64.h"
#include "mt_dbg_aarch64.h"
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <mt-plat/sync_write.h>
#include <asm/system_misc.h>

struct wp_trace_context_t wp_tracer;
#ifdef WATCHPOINT_TEST_SUIT
struct wp_event wp_event;
int err;
volatile int my_watch_data;
int wp_flag;
int my_wp_handler1(phys_addr_t addr)
{
	wp_flag++;
	pr_debug("[MTK WP] Access my data from an instruction at %p\n", &addr);
	return 0;
}

int my_wp_handler2(phys_addr_t addr)
{

	pr_debug("[MTK WP] In my_wp_handler2 Access my data from an instruction at %p\n", &addr);
	/* trigger exception */
	return 0;
}

void wp_test1(void)
{
	int i;

	init_wp_event(&wp_event, (unsigned long)&my_watch_data, (unsigned long)&my_watch_data,
		      WP_EVENT_TYPE_ALL, my_wp_handler1);
	err = add_hw_watchpoint(&wp_event);
	if (err)
		/* fail to add watchpoing */
		pr_debug("[MTK WP] add hw watch point failed...\n");
	else
		/* the memory address is under watching */
		pr_debug("[MTK WP] add hw watch point success...\n");

	for (i = 0; i < num_possible_cpus(); i++) {
#ifdef DBG_REG_DUMP
		dump_dbgregs(i);
		print_dbgregs(i);
#endif
	}
	pr_debug("start to access my_watch_data\n");
	/* test watchpoint */
	my_watch_data = 1;

}

void smp_specific_write(int *p)
{
	*p = 1;
	pr_debug("[MTK WP] wite data in specific address ok,addr=0x%p\n", p);
}

void wp_test2(void)
{
	int i;
	int ret = 0;

	wp_flag = 0;
	pr_debug("[MTK WP] Init wp.. ");
	init_wp_event(&wp_event, (unsigned long)&my_watch_data, (unsigned long)&my_watch_data,
		      WP_EVENT_TYPE_ALL, my_wp_handler1);
	err = add_hw_watchpoint(&wp_event);
	if (err) {
		pr_debug("[MTK WP] add hw watch point failed...\n");
		/* fail to add watchpoing */
	} else {
		/* the memory address is under watching */
		pr_debug("[MTK WP] add hw watch point success...\n");

	}
	pr_debug("[MTK WP] dump standard dbgsys setting\n");
	for (i = 1; i < num_possible_cpus(); i++) {
		ret = cpu_down(i);	/* FIXME Walkround for build error */
		if (ret != 0)
			pr_debug("[MTK WP] cpu %d power down failed\n", i);
		if (!cpu_online(i))
			pr_debug("[MTK WP] cpu %d already power down\n", i);
	}
	for (i = 0; i < num_possible_cpus(); i++) {
		ret = cpu_up(i);
		if (ret != 0)
			pr_debug("[MTK WP] cpu %d power up failed\n", i);
		if (cpu_online(i))
			pr_debug("[MTK WP] cpu %d already power up\n", i);

		pr_debug("[MTK WP] dump dbgsys setting restored\n");
#ifdef DBG_REG_DUMP
		dump_dbgregs(i);
		print_dbgregs(i);
#endif
		ret =
		    smp_call_function_single(i, (smp_call_func_t) smp_specific_write,
					     (void *)&my_watch_data, 1);
		if (ret == 0)
			pr_debug("[MTK WP] cpu %d, wite data in specific address ok\n", i);
		else
			pr_debug("[MTK WP] cpu %d, wite data in specific address failed\n", i);
	}
	if (wp_flag == num_possible_cpus())
		pr_debug("[MTK WP] Watchpoint item2 verfication pass 0x%x\n", wp_flag);
	else
		pr_debug("[MTK WP] Watchpoint item2 verfication failed 0x%x\n", wp_flag);
}

void wp_test3(void)
{
	int i;
	int ret;

	wp_flag = 0;
	for (i = 0; i < num_possible_cpus(); i++) {
		ret = cpu_up(i);
		if (ret != 0)
			pr_debug("[MTK WP] cpu %d power up failed\n", i);
		if (cpu_online(i))
			pr_debug("[MTK WP] cpu %d already power up\n", i);

		pr_debug("[MTK WP] dump dbgsys setting restored\n");
#ifdef DBG_REG_DUMP
		dump_dbgregs(i);
		print_dbgregs(i);
#endif
		ret =
		    smp_call_function_single(i, (smp_call_func_t) smp_specific_write,
					     (void *)&my_watch_data, 1);
		if (ret == 0)
			pr_debug("[MTK WP] cpu %d, wite data in specific address ok\n", i);
		else
			pr_debug("[MTK WP] cpu %d, wite data in specific address failed\n", i);
	}
	if (wp_flag == num_possible_cpus())
		pr_debug("[MTK WP] Watchpoint item3 verfication pass 0x%x\n", wp_flag);
	else
		pr_debug("[MTK WP] Watchpoint item3 verfication failed 0x%x\n", wp_flag);
}
#endif

void smp_read_MDSCR_EL1_callback(void *info)
{
	unsigned int tmp;
	unsigned int *val = info;

	asm volatile ("mrs %0, MDSCR_EL1":"=r" (tmp));
	*val = tmp;
}

void smp_write_MDSCR_EL1_callback(void *info)
{
	unsigned int *val = info;
	unsigned int tmp = *val;

	asm volatile ("msr  MDSCR_EL1, %0" : : "r" (tmp));
}

void smp_read_OSLSR_EL1_callback(void *info)
{
	unsigned int tmp;
	unsigned int *val = info;

	asm volatile ("mrs %0, OSLSR_EL1":"=r" (tmp));
	*val = tmp;
}

static spinlock_t wp_lock;

int register_wp_context(struct wp_trace_context_t **wp_tracer_context)
{
	*wp_tracer_context = &wp_tracer;
	return 0;
}

/*
 * enable_hw_watchpoint: Enable the H/W watchpoint.
 * Return error code.
 */
int enable_hw_watchpoint(void)
{
	int i;
	unsigned int args;
	int oslsr_el1;
	int edlsr;

	pr_debug("[MTK WP] Hotplug disable\n");
	cpu_hotplug_disable();
	for (i = 0; i < num_possible_cpus(); i++) {
		if (cpu_online(i)) {
			cs_cpu_write(wp_tracer.debug_regs[i], EDLAR, UNLOCK_KEY);
			cs_cpu_write(wp_tracer.debug_regs[i], OSLAR_EL1, ~UNLOCK_KEY);
			args = cs_cpu_read(wp_tracer.debug_regs[i], EDSCR);

			pr_debug("[MTK WP] cpu %d, EDSCR &0x%lx= 0x%x\n", i,
				  ((vmalloc_to_pfn((void *)wp_tracer.debug_regs[i]) << 12) + EDSCR),
				  args);
			if (args & HDE) {
				pr_debug
				    ("[MTK WP] halting debug mode enabled. Unable to access hardware resources.\n");
				return -EPERM;
			}
			smp_call_function_single(i, smp_read_MDSCR_EL1_callback, &args, 1);
			pr_debug("[MTK WP] cpu %d, MDSCR 0x%x\n", i, args);
			if (args & MDBGEN) {
				/* already enabled */
				pr_debug("[MTK WP] already enabled, MDSCR = 0x%x\n", args);
			}
			/*
			 * Since Watchpoint taken from EL1 to EL1, so we have to enable KDE.
			 * refer ARMv8 architecture spec section -
			 * D2.4.1 Enabling debug exceptions from the current Exception level
			 */
			args = args | MDBGEN | KDE;
			smp_call_function_single(i, smp_write_MDSCR_EL1_callback, &args, 1);
			smp_call_function_single(i, smp_read_MDSCR_EL1_callback, &args, 1);
			smp_call_function_single(i, smp_read_OSLSR_EL1_callback, &oslsr_el1, 1);
			edlsr = cs_cpu_read(wp_tracer.debug_regs[i], EDLSR);
			pr_debug("[MTK WP] cpu %d, EDLSR 0x%x, OSLSR_EL1 0x%x\n", i, edlsr,
				  oslsr_el1);
			pr_debug("[MTK WP] cpu %d, MDSCR_EL1 0x%x. (after set MDSCR_EL1)\n", i,
				  args);
		} else {
			pr_debug("[MTK WP] cpu %d, power down(%d) so skip enable_hw_watchpoint\n",
				  i, cpu_online(i));
		}
	}
	pr_debug("[MTK WP] Hotplug enable\n");
	cpu_hotplug_enable();
	return 0;
}


void reset_watchpoint(void)
{
	int j;
	int i;
	unsigned int args;

	for (j = 0; j < num_possible_cpus(); j++) {
		if (cpu_online(j)) {
			cs_cpu_write(wp_tracer.debug_regs[j], EDLAR, UNLOCK_KEY);
			cs_cpu_write(wp_tracer.debug_regs[j], OSLAR_EL1, ~UNLOCK_KEY);
			args = cs_cpu_read(wp_tracer.debug_regs[j], EDSCR);
			pr_debug("[MTK WP] Reset flow cpu %d, EDSCR 0x%x\n", j, args);
			if (args & HDE) {
				pr_debug("[MTK WP]Reset flow halting debug mode enabled. Unable to reset hardware resources.\n");
				cs_cpu_write(wp_tracer.debug_regs[j], EDLAR, ~UNLOCK_KEY);
				cs_cpu_write(wp_tracer.debug_regs[j], OSLAR_EL1, UNLOCK_KEY);
				return;
			}
			for (i = 0; i < wp_tracer.wp_nr; i++) {
				cs_cpu_write_64(wp_tracer.debug_regs[j], DBGWVR + (i << 4), 0);
				cs_cpu_write(wp_tracer.debug_regs[j], DBGWCR + (i << 4), 0);
				pr_debug("[MTK WP] Reset flow  cpu %d, DBGWVR%d, &0x%p=0x%lx\n", j,
					  i, wp_tracer.debug_regs[j] + DBGWVR + (i << 4),
					  cs_cpu_read_64(wp_tracer.debug_regs[j],
							 DBGWVR + (i << 4)));
				pr_debug("[MTK WP] Reset flow  cpu %d, DBGWCR%d, &0x%p=0x%x\n", j,
					  i, wp_tracer.debug_regs[j] + DBGWCR + (i << 4),
					  cs_cpu_read(wp_tracer.debug_regs[j], DBGWCR + (i << 4)));
			}
			for (i = 0; i < wp_tracer.bp_nr; i++) {
				cs_cpu_write_64(wp_tracer.debug_regs[j], DBGBVR + (i << 4), 0);
				cs_cpu_write(wp_tracer.debug_regs[j], DBGBCR + (i << 4), 0);
				pr_debug("[MTK WP] Reset flow  cpu %d, DBGBVR%d, &0x%p=0x%lx\n", j,
					  i, wp_tracer.debug_regs[j] + DBGBVR + (i << 4),
					  cs_cpu_read_64(wp_tracer.debug_regs[j],
							 DBGBVR + (i << 4)));
				pr_debug("[MTK WP] Reset flow  cpu %d, DBGBCR%d, &0x%p=0x%x\n", j,
					  i, wp_tracer.debug_regs[j] + DBGBCR + (i << 4),
					  cs_cpu_read(wp_tracer.debug_regs[j], DBGBCR + (i << 4)));
			}
			cs_cpu_write(wp_tracer.debug_regs[j], EDLAR, ~UNLOCK_KEY);
			cs_cpu_write(wp_tracer.debug_regs[j], OSLAR_EL1, UNLOCK_KEY);
		} else {
			pr_debug("[MTK WP] cpu %d, power down(%d) so skip adding watchpoint\n", j,
				  cpu_online(j));
		}
	}
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
	for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
		if (!wp_tracer.wp_events[i].in_use) {
			wp_tracer.wp_events[i].in_use = 1;
			break;
		}
		if (wp_tracer.wp_events[i].virt == (wp_event->virt & ~3)) {
			pr_err("This address have been watched in %d's watchpoint\n", i);
			spin_unlock_irqrestore(&wp_lock, flags);
			return -EAGAIN;
		}
	}
	spin_unlock_irqrestore(&wp_lock, flags);

	if (i == MAX_NR_WATCH_POINT)
		return -EAGAIN;

	wp_tracer.wp_events[i].virt = wp_event->virt & ~3;	/* enforce word-aligned */
	wp_tracer.wp_events[i].phys = wp_event->phys;	/* no use currently */
	wp_tracer.wp_events[i].type = wp_event->type;
	wp_tracer.wp_events[i].handler = wp_event->handler;
	wp_tracer.wp_events[i].auto_disable = wp_event->auto_disable;
	pr_debug("[MTK WP] Hotplug disable\n");
	cpu_hotplug_disable();
	pr_debug("[MTK WP] Add watchpoint %d at address %p\n", i, &(wp_tracer.wp_events[i].virt));
	for (j = 0; j < num_possible_cpus(); j++) {
		if (cpu_online(j)) {
			cs_cpu_write_64(wp_tracer.debug_regs[j], DBGWVR + (i << 4),
					wp_tracer.wp_events[i].virt);
			cs_cpu_write(wp_tracer.debug_regs[j], DBGWCR + (i << 4), ctl);

			pr_debug("[MTK WP] cpu %d, DBGWVR%d, &0x%p=0x%lx\n", j, i,
				  wp_tracer.debug_regs[j] + DBGWVR + (i << 4),
				  cs_cpu_read_64(wp_tracer.debug_regs[j], DBGWVR + (i << 4)));

			pr_debug("[MTK WP] cpu %d, DBGWCR%d, &0x%p=0x%x\n", j, i,
				  wp_tracer.debug_regs[j] + DBGWCR + (i << 4),
				  cs_cpu_read(wp_tracer.debug_regs[j], DBGWCR + (i << 4)));

		} else {
			pr_debug("[MTK WP] cpu %d, power down(%d) so skip adding watchpoint\n", j,
				  cpu_online(j));
		}
	}
	pr_debug("[MTK WP] Hotplug enable\n");
	cpu_hotplug_enable();
	return 0;
}

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

	pr_debug("[MTK WP] Hotplug disable\n");
	cpu_hotplug_disable();
	spin_lock_irqsave(&wp_lock, flags);
	for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
		if (wp_tracer.wp_events[i].in_use
		    && (wp_tracer.wp_events[i].virt == wp_event->virt)) {
			wp_tracer.wp_events[i].virt = 0;
			wp_tracer.wp_events[i].phys = 0;
			wp_tracer.wp_events[i].type = 0;
			wp_tracer.wp_events[i].handler = NULL;
			wp_tracer.wp_events[i].in_use = 0;
			for (j = 0; j < num_possible_cpus(); j++) {
				if (cpu_online(j)) {
					cs_cpu_write(wp_tracer.debug_regs[j], DBGWCR + (i << 4),
						     cs_cpu_read(wp_tracer.debug_regs[j],
								 DBGWCR + (i << 4)) & (~WP_EN));
					cs_cpu_write(wp_tracer.debug_regs[j], EDLAR, ~UNLOCK_KEY);
					cs_cpu_write(wp_tracer.debug_regs[j], OSLAR_EL1,
						     UNLOCK_KEY);
				}
			}
			break;
		}
	}
	spin_unlock_irqrestore(&wp_lock, flags);
	pr_debug("[MTK WP] Hotplug enable\n");
	cpu_hotplug_enable();
	if (i == MAX_NR_WATCH_POINT)
		return -EINVAL;
	else
		return 0;
}

static int watchpoint_handler(unsigned long addr, unsigned int esr, struct pt_regs *regs)
{
	unsigned long wfar, daddr, iaddr;
	int i, ret, j;
	/* Notes
	 * wfar is the watched data address which is accessed . and it is from FAR_EL1
	 */
	asm volatile ("mrs %0, FAR_EL1":"=r" (wfar));
	daddr = addr & ~3;
	iaddr = regs->pc;	/* this is the instruction address that access the data that is watching. */
	pr_debug("[MTK WP] addr = 0x%lx, DBGWFAR/DFAR = 0x%lx\n", (unsigned long)addr, wfar);
	pr_debug("[MTK WP] daddr = 0x%lx, iaddr = 0x%lx\n", daddr, iaddr);

	/* update PC to avoid re-execution of the instruction under watching */
	regs->pc += compat_thumb_mode(regs) ? 2 : 8;

	for (i = 0; i < MAX_NR_WATCH_POINT; i++) {
		if (wp_tracer.wp_events[i].in_use && wp_tracer.wp_events[i].virt == (daddr)) {
			pr_debug("[MTK WP] Watchpoint %d triggers.\n", i);
			if (!(wp_tracer.wp_events[i].handler)) {
				pr_debug("[MTK WP] No watchpoint handler. Ignore.\n");
				return 0;
			}
			if (wp_tracer.wp_events[i].auto_disable) {
				for (j = 0; j < num_possible_cpus(); j++) {
					if (cpu_online(j)) {
						cs_cpu_write(wp_tracer.debug_regs[j],
							     DBGWCR + (i << 4),
							     cs_cpu_read
							     (wp_tracer.debug_regs[j],
							      DBGWCR +
							      (i << 4)) & (~WP_EN));
					} else {
						pr_debug("[MTK WP] cpu %d, power down(%d) so skip adding watchpoint auto-disable\n",
						     j, cpu_online(j));
					}
				}
			}
			ret = wp_tracer.wp_events[i].handler(iaddr);
			if (wp_tracer.wp_events[i].auto_disable) {
				for (j = 0; j < num_possible_cpus(); j++) {
					if (cpu_online(j)) {
						cs_cpu_write(wp_tracer.debug_regs[j],
							     DBGWCR + (i << 4),
							     cs_cpu_read
							     (wp_tracer.debug_regs[j],
							      DBGWCR + (i << 4)) | WP_EN);
					} else {
						pr_debug("[MTK WP] cpu %d, power down(%d) so skip watchpoint auto-disable\n",
						     j, cpu_online(j));
					}
				}
			}
			return ret;
		}
	}

	return 0;
}

int wp_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;

	pr_debug("[MTK WP] watchpoint_probe\n");
	of_property_read_u32(pdev->dev.of_node, "num", &wp_tracer.nr_dbg);
	pr_debug("[MTK WP] get %d debug interface\n", wp_tracer.nr_dbg);
	wp_tracer.debug_regs =
	    kmalloc(sizeof(void *) * (unsigned long)wp_tracer.nr_dbg, GFP_KERNEL);
	if (!wp_tracer.debug_regs) {
		pr_err("[MTK WP] Failed to allocate watchpoint register array\n");
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < wp_tracer.nr_dbg; i++) {
		wp_tracer.debug_regs[i] = of_iomap(pdev->dev.of_node, i);

		if (wp_tracer.debug_regs[i] == NULL)
			pr_debug("[MTK WP] debug_interface %d devicetree mapping failed\n", i);
		else
			pr_debug("[MTK WP] debug_interface %d @ vm:0x%p pm:0x%lx\n", i,
				  wp_tracer.debug_regs[i],
				  vmalloc_to_pfn((void *)wp_tracer.debug_regs[i]) << 12);
	}
	asm volatile ("mrs %0, ID_AA64DFR0_EL1":"=r" (wp_tracer.id_aaa64dfr0_el1));
	wp_tracer.wp_nr = ((wp_tracer.id_aaa64dfr0_el1 & (0xf << 20)) >> 20) + 1;
	wp_tracer.bp_nr = ((wp_tracer.id_aaa64dfr0_el1 & (0xf << 12)) >> 12) + 1;
	pr_debug("[MTK WP] Probe:Reset watchpoints");
	reset_watchpoint();
out:
	return ret;
}

static const struct of_device_id dbg_of_ids[] = {
	{.compatible = "mediatek,mt6797-dbg_debug",},
	{}
};

static struct platform_driver wp_driver = {
	.probe = wp_probe,
	.driver = {
		   .name = "wp",
		   .bus = &platform_bus_type,
		   .owner = THIS_MODULE,
		   .of_match_table = dbg_of_ids,
		   },
};


#define  WATCHPOINT_TEST_SUIT
#ifdef WATCHPOINT_TEST_SUIT
static ssize_t wp_test_suit_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "==Watchpoint test==\n"
			"1. test watchpoints in online cpu\n"
			"2. verified all cpu's watchpoint after hotplug\n"
			"3. verified all cpu's watchpoing after suspend/resume. Not:you have to run test item 1 or 2 to init watchpoint first\n");
}

static ssize_t wp_test_suit_store(struct device_driver *driver, const char *buf, size_t count)
{
	char *p = (char *)buf;
	unsigned long int num;
	ssize_t ret;

	ret = kstrtoul(p, 10, &num);
	if (ret == 0) {
		switch (num) {
			/* Test watchpoint Function */
		case 1:
			wp_test1();
			break;
		case 2:
			wp_test2();
			break;
		case 3:
			wp_test3();
		default:
			break;
		}
	}
	return count;
}

DRIVER_ATTR(wp_test_suit, 0664, wp_test_suit_show, wp_test_suit_store);
#endif
static int __init hw_watchpoint_init(void)
{
	int err;
#ifdef WATCHPOINT_TEST_SUIT
	int ret;
#endif
	return 0;

	spin_lock_init(&wp_lock);
	err = platform_driver_register(&wp_driver);
	if (err) {
		pr_err("[MTK WP] watchpoint registration failed\n");
		return err;
	}
#ifdef WATCHPOINT_TEST_SUIT
	ret = driver_create_file(&wp_driver.driver, &driver_attr_wp_test_suit);
	if (ret)
		pr_err("[MTK WP] Fail to create systracker_drv sysfs files");
#endif
	hook_debug_fault_code(DBG_ESR_EVT_HWWP, watchpoint_handler, SIGTRAP, TRAP_HWBKPT,
			      "hw-watchpoint handler");
	pr_debug("[MTK WP] watchpoint handler init.\n");

	return 0;
}

/*
   EXPORT_SYMBOL(add_hw_watchpoint);
   */
arch_initcall(hw_watchpoint_init);
