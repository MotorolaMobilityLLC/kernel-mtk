#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/printk.h>
#include <linux/mm.h>
#include <linux/compat.h>
#include <linux/init.h>
#include <asm/traps.h>
#include <asm/ptrace.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/atomic.h>

static atomic_t reg_0x1001Axxx_cnt = ATOMIC_INIT(1);

void ioremap_debug_hook_func(phys_addr_t phys_addr, size_t size,
		unsigned int mtype)
{
	unsigned long offset = phys_addr & ~PAGE_MASK;
	phys_addr_t target = 0x1001A000;

	target &= PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/* only allow 1st ioremap to address 0x1001AXXX */
	if ((phys_addr + size <= target) ||
			(phys_addr >= (target + PAGE_SIZE)))
		return;

	if (atomic_dec_and_test(&reg_0x1001Axxx_cnt))
		return;

	/* trap other ioremap to 0x1001AXXX */
	pr_err("ioremap to 0x1001AXXX more than once (%pa, %zx)\n",
			&phys_addr, size);
	BUG();
}

static DEFINE_PER_CPU(void *, __prev_undefinstr_pc);
static DEFINE_PER_CPU(int, __prev_undefinstr_counter);

/*
 * return 0 if code is fixed
 * return 1 if the undefined instruction cannot be fixed.
 */
int arm_undefinstr_retry(struct pt_regs *regs, unsigned int instr)
{
	void __user *pc;
	struct thread_info *thread = current_thread_info();
	u32 insn = __opcode_to_mem_arm(BUG_INSTR_VALUE);
	u32 insn = 0;

	pc = (void __user *)instruction_pointer(regs);

	/*
	 * Place the SIGILL ICache Invalidate after the Debugger
	 * Undefined-Instruction Solution.
	 */
	if (((processor_mode(regs) == USR_MODE) ||
				(processor_mode(regs) == SVC_MODE)) &&
			(instr != insn)) {
		void **prev_undefinstr_pc = &get_cpu_var(__prev_undefinstr_pc);
		int *prev_undefinstr_counter = &get_cpu_var(__prev_undefinstr_counter);

		pr_alert("USR_MODE/SVC_MODE Undefined Instruction Address curr:%p pc=%p:%p, instr: 0x%x\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc,
			instr);
		if (*prev_undefinstr_pc != pc) {
			/*
			 * If the current process or program counter is changed...
			 * renew the counter.
			 */
			pr_alert("First Time Recovery curr:%p pc=%p:%p\n",
				(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
			*prev_undefinstr_pc = pc;
			*prev_undefinstr_counter = 0;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__cpuc_flush_icache_all();
			flush_cache_all();
			/*
			 * undo cpu_excp to cancel nest_panic code, see entry.S
			 */
			if (!user_mode(regs))
				thread->cpu_excp--;
			return 0;
		} else if (*prev_undefinstr_counter < 2) {
			pr_alert("%dth Time Recovery curr:%p pc=%p:%p\n",
				*prev_undefinstr_counter + 1,
				(void *)current, (void *)pc,
				(void *)*prev_undefinstr_pc);
			*prev_undefinstr_counter += 1;
			put_cpu_var(__prev_undefinstr_pc);
			put_cpu_var(__prev_undefinstr_counter);
			__cpuc_flush_icache_all();
			flush_cache_all();
			/*
			 * undo cpu_excp to cancel nest_panic code, see entry.S
			 */
			if (!user_mode(regs))
				thread->cpu_excp--;
			return 0;
		}
		*prev_undefinstr_counter += 1;

		if (*prev_undefinstr_counter >= 2) {
			*prev_undefinstr_pc = 0;
			*prev_undefinstr_counter = 0;
		}
		put_cpu_var(__prev_undefinstr_pc);
		put_cpu_var(__prev_undefinstr_counter);
		pr_alert("Go to ARM Notify Die curr:%p pc=%p:%p\n",
			(void *)current, (void *)pc, (void *)*prev_undefinstr_pc);
	}
	return 1;
}
