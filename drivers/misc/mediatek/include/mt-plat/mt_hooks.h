#ifndef __MT_HOOKS_H__
#define __MT_HOOKS_H__

extern int __weak arm_undefinstr_retry(struct pt_regs *regs,
		unsigned int instr);

#endif /* end __MT_HOOKS_H__ */
