#ifndef _BACKTRACE_H
#define _BACKTRACE_H

/* defined in backstrace32.asm */
extern asmlinkage void c_backtrace_ramconsole_print(unsigned long fp, int pmode);

#endif /* _BACKTRACE_H */
