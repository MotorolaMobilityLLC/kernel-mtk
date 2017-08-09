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

#ifndef __PMIC_IRQ_H
#define __PMIC_IRQ_H

/* pmic irq extern variable */
extern struct task_struct *pmic_thread_handle;
#if !defined CONFIG_HAS_WAKELOCKS
extern struct wakeup_source pmicThread_lock;
#else
extern struct wake_lock pmicThread_lock;
#endif
extern int interrupts_size;
extern struct pmic_interrupts interrupts[];
/* pmic irq extern functions */
extern void PMIC_EINT_SETTING(void);
extern int pmic_thread_kthread(void *x);
void buck_oc_detect(void);

#endif /*--PMIC_IRQ_H--*/
