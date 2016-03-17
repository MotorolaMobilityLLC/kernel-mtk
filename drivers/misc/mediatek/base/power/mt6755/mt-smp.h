/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

extern void mt_secondary_startup(void);
extern void mt_gic_secondary_init(void);
extern unsigned int irq_total_secondary_cpus;

extern void __init mt_smp_prepare_cpus(unsigned int max_cpus);
extern void mt_smp_secondary_init(unsigned int cpu);
extern int mt_smp_boot_secondary(unsigned int cpu, struct task_struct *idle);
extern int mt_cpu_kill(unsigned int cpu);

