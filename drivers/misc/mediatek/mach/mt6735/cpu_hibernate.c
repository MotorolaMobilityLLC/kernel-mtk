/*
 * ARM Cortex-A7 save/restore for suspend to disk (Hibernation)
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sysrq.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/suspend.h>
#include <linux/mm.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/tlbflush.h>
#include <asm/suspend.h>
#include <asm/cacheflush.h>
#include <asm/system_misc.h>
#include <asm/idmap.h>
#include <mtk_hibernate_core.h>

typedef struct fault_regs {
	unsigned dfar;
	unsigned ifar;
	unsigned ifsr;
	unsigned dfsr;
	unsigned adfsr;
	unsigned aifsr;
} cp15_fault_regs;

typedef struct ns_banked_cp15_context {
	unsigned int cp15_misc_regs[2]; /* cp15 miscellaneous registers */
	unsigned int cp15_ctrl_regs[20];    /* cp15 control registers */
	unsigned int cp15_mmu_regs[16]; /* cp15 mmu registers */
	cp15_fault_regs ns_cp15_fault_regs; /* cp15 fault status registers */
} banked_cp15_context;

typedef struct cpu_context {
	unsigned int banked_regs[32];
	unsigned int timer_data[8]; /* Global timers if the NS world has access to them */
} core_context;

static banked_cp15_context saved_cp15_context;
static core_context saved_core_context;

static void __save_processor_state(struct ns_banked_cp15_context *ctxt)
{
	/* save preempt state and disable it */
	preempt_disable();

	mt_save_generic_timer(saved_core_context.timer_data, 0x0);
	mt_save_banked_registers(saved_core_context.banked_regs);
}

void notrace mtk_save_processor_state(void)
{
	__save_processor_state(&saved_cp15_context);
}
EXPORT_SYMBOL(mtk_save_processor_state);

static void __restore_processor_state(struct ns_banked_cp15_context *ctxt)
{
	mt_restore_banked_registers(saved_core_context.banked_regs);
	mt_restore_generic_timer(saved_core_context.timer_data, 0x0);

#ifdef CONFIG_MTK_ETM
	/* restore ETM module */
	trace_start_dormant();
#endif
	/* restore preempt state */
	preempt_enable();
}

void notrace mtk_restore_processor_state(void)
{
	__restore_processor_state(&saved_cp15_context);
}
EXPORT_SYMBOL(mtk_restore_processor_state);

typedef void (*phys_reset_t)(unsigned long);

/*
 * The framework loads the hibernation image into a linked list anchored
 * at restore_pblist, for swsusp_arch_resume() to copy back to the proper
 * destinations.
 *
 * To make this work if resume is triggered from initramfs, the
 * pagetables need to be switched to allow writes to kernel mem.
 */
void notrace mtk_arch_restore_image(void)
{
	phys_reset_t phys_reset;
	struct pbe *pbe;

	for (pbe = restore_pblist; pbe; pbe = pbe->next)
		copy_page(pbe->orig_address, pbe->address);

#if 0	/* [ALPS01496758]  since CA17 has cache bug, replace with the modified assemlby version */
	/* Clean and invalidate caches */
	flush_cache_all();

	/* Turn off caching */
	cpu_proc_fin();

	/* Push out any further dirty data, and ensure cache is empty */
	flush_cache_all();
#else
	__disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2();
#endif

	/* Take out a flat memory mapping. */
	setup_mm_for_reboot();
	phys_reset = (phys_reset_t)(unsigned long)virt_to_phys(cpu_reset);
	/* Return from cpu_suspend/swsusp_arch_suspend */
	phys_reset((unsigned long)virt_to_phys(cpu_resume));

	/* Should never get here. */
	BUG();
}
EXPORT_SYMBOL(mtk_arch_restore_image);
