#ifndef __MT6336_IRQ_H
#define __MT6336_IRQ_H

/*
typedef enum {

} MT6336_IRQ_ENUM;
*/

/* mt6336 irq extern variable */
extern struct task_struct *mt6336_thread_handle;
#if !defined CONFIG_HAS_WAKELOCKS
extern struct wakeup_source mt6336Thread_lock;
#else
extern struct wake_lock mt6336Thread_lock;
#endif
extern struct chr_interrupts mt6336_interrupts[];
/* mt6336 irq extern functions */
extern void MT6336_EINT_SETTING(void);
extern int mt6336_thread_kthread(void *x);

extern void mt6336_enable_interrupt(unsigned int intNo, char *str);
extern void mt6336_mask_interrupt(unsigned int intNo, char *str);
extern void mt6336_unmask_interrupt(unsigned int intNo, char *str);
extern void mt6336_register_interrupt_callback(unsigned int intNo, void (EINT_FUNC_PTR) (void));

#endif /*--__MT6336_IRQ_H--*/
