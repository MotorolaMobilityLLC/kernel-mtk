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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/memory.h>
#include <asm/cacheflush.h>
#include <linux/kdebug.h>
#include <linux/module.h>
#include <mrdump.h>
#ifdef CONFIG_MTK_RAM_CONSOLE
#include <mt-plat/mtk_ram_console.h>
#endif
#include <linux/reboot.h>
#include "ipanic.h"
#include <asm/system_misc.h>
#include "../mrdump/mrdump_private.h"

int __weak ipanic_atflog_buffer(void *data, unsigned char *buffer, size_t sz_buf)
{
	return 0;
}

void __weak sysrq_sched_debug_show_at_AEE(void)
{
	pr_err("%s weak function at %s\n", __func__, __FILE__);
}

void ipanic_recursive_ke(struct pt_regs *regs, struct pt_regs *excp_regs, int cpu)
{
	struct pt_regs saved_regs;

#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_exp_type(AEE_EXP_TYPE_NESTED_PANIC);
#endif
	aee_nested_printf("minidump\n");
	bust_spinlocks(1);
	if (excp_regs != NULL) {
		__mrdump_create_oops_dump(AEE_REBOOT_MODE_NESTED_EXCEPTION, excp_regs, "Kernel NestedPanic");
	} else if (regs != NULL) {
		aee_nested_printf("previous excp_regs NULL\n");
		__mrdump_create_oops_dump(AEE_REBOOT_MODE_NESTED_EXCEPTION, regs, "Kernel NestedPanic");
	} else {
		aee_nested_printf("both NULL\n");
		mrdump_mini_save_regs(&saved_regs);
		__mrdump_create_oops_dump(AEE_REBOOT_MODE_NESTED_EXCEPTION, &saved_regs, "Kernel NestedPanic");
	}

	mrdump_mini_ke_cpu_regs(excp_regs);
	mrdump_mini_per_cpu_regs(cpu, regs, current);
	__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2();

	aee_exception_reboot();
}
EXPORT_SYMBOL(ipanic_recursive_ke);

#if defined(CONFIG_RANDOMIZE_BASE) && defined(CONFIG_ARM64)
static u64 show_kaslr(void)
{
	u64 const kaslr_offset = kimage_vaddr - KIMAGE_VADDR;

	pr_notice("Kernel Offset: 0x%llx from 0x%lx\n", kaslr_offset, KIMAGE_VADDR);
	return kaslr_offset;
}
#else
static u64 show_kaslr(void)
{
	pr_notice("Kernel Offset: disabled\n");
	return 0;
}
#endif

static int common_die(int fiq_step, int reboot_reason, const char *msg,
		      struct pt_regs *regs)
{
	u64 kaslr_offset;

	bust_spinlocks(1);
	aee_disable_api();

	kaslr_offset = show_kaslr();
	print_modules();
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_kaslr_offset(kaslr_offset);
	aee_rr_rec_exp_type(AEE_EXP_TYPE_KE);
	aee_rr_rec_fiq_step(fiq_step);

	aee_rr_rec_scp();
#endif
	__mrdump_create_oops_dump(reboot_reason, regs, msg);

	switch (reboot_reason) {
	case AEE_REBOOT_MODE_KERNEL_OOPS:
		__show_regs(regs);
		dump_stack();
		break;
#ifndef CONFIG_DEBUG_BUGVERBOSE
	case AEE_REBOOT_MODE_KERNEL_PANIC:
		dump_stack();
		break;
#endif
	default:
		/* Don't print anything */
		break;
	}
#ifdef CONFIG_MTK_WQ_DEBUG
	wq_debug_dump();
#endif

	mrdump_mini_ke_cpu_regs(regs);
	__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2();

	aee_exception_reboot();

	return NOTIFY_DONE;
}

int ipanic(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct pt_regs saved_regs;

	mrdump_mini_save_regs(&saved_regs);
	return common_die(AEE_FIQ_STEP_KE_IPANIC_START,
			  AEE_REBOOT_MODE_KERNEL_PANIC,
			  "Kernel Panic", &saved_regs);
}

static int ipanic_die(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	struct die_args *dargs = (struct die_args *)ptr;

	return common_die(AEE_FIQ_STEP_KE_IPANIC_DIE,
			  AEE_REBOOT_MODE_KERNEL_OOPS,
			  "Kernel Oops", dargs->regs);
}

static struct notifier_block panic_blk = {
	.notifier_call = ipanic,
};

static struct notifier_block die_blk = {
	.notifier_call = ipanic_die,
};

int __init aee_ipanic_init(void)
{
	mrdump_init();
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	register_die_notifier(&die_blk);
	LOGI("ipanic: startup\n");
	return 0;
}

arch_initcall(aee_ipanic_init);
