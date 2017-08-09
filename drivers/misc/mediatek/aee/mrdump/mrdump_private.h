#if !defined(__MRDUMP_PRIVATE_H__)

struct pt_regs;

void mrdump_save_current_backtrace(struct pt_regs *regs);
void mrdump_print_crash(struct pt_regs *regs);
extern void __inner_flush_dcache_all(void);
extern void __inner_flush_dcache_L1(void);

#endif
