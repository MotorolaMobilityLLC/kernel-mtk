#ifndef __MTK_HOOKS_H__
#define __MTK_HOOKS_H__

extern int __weak arm_undefinstr_retry(struct pt_regs *regs,
		unsigned int instr);
extern void __weak ioremap_debug_hook_func(phys_addr_t phys_addr,
		size_t size, pgprot_t prot);
extern int __weak mem_fault_debug_hook(struct pt_regs *regs);

#endif /* __MTK_HOOKS_H__ */
