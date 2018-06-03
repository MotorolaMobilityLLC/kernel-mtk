/*
 * Copyright (c) 2016 MediaTek Inc.
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

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/bug.h>
#include <asm/cacheflush.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/*#include <mt_secure_api.h>*/
#include <linux/topology.h>
#if 0
static struct notifier_block cpu_hotplug_nb;
static int cpu_hotplug_cb_notifier(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	unsigned int cpu = (long)hcpu;
	struct cpumask cpuhp_cpumask;
	struct cpumask cpu_online_cpumask;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
#if 0
		if ((cpu == 4) || (cpu == 5) || (cpu == 6) || (cpu == 7)) {
			arch_get_cluster_cpus(&cpuhp_cpumask, arch_get_cluster_id(cpu));
			cpumask_and(&cpu_online_cpumask, &cpuhp_cpumask, cpu_online_mask);
			if (!cpumask_weight(&cpu_online_cpumask))
				mt6313_enable(VDVFS13, 1);
		}
#endif
		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		mt_secure_call(MTK_SIP_POWER_DOWN_CORE, cpu, 0, 0);
		arch_get_cluster_cpus(&cpuhp_cpumask, arch_get_cluster_id(cpu));
		cpumask_and(&cpu_online_cpumask, &cpuhp_cpumask, cpu_online_mask);
		if (!cpumask_weight(&cpu_online_cpumask)) {
			/*pr_info("Start to power off cluster %d\n", cpu/4);*/
			mt_secure_call(MTK_SIP_POWER_DOWN_CLUSTER, cpu/4, 0, 0);
			/*pr_info("End of power off cluster %d\n", cpu/4);*/
		}
		break;
#endif /* CONFIG_HOTPLUG_CPU */

	default:
		break;
	}
	return NOTIFY_OK;
}
#endif
static __init int hotplug_cb_init(void)
{
/*	int ret;*/
	int i;
#ifdef CONFIG_ARMPLL_CTRL
	iomap();
#endif
	for (i = 0; i < num_possible_cpus(); i++)
		set_cpu_present(i, true);
#if 0
	cpu_hotplug_nb = (struct notifier_block) {
		.notifier_call	= cpu_hotplug_cb_notifier,
		.priority	= CPU_PRI_PERF + 1,
	};

	ret = register_cpu_notifier(&cpu_hotplug_nb);
	if (ret)
		return ret;
#endif
	pr_info("CPU Hotplug Low Power Notification\n");

	return 0;
}
early_initcall(hotplug_cb_init);
#if 0
static int __init mt_hotplug_late(void)
{
	mt6313_enable(VDVFS13, 0);

	return 0;
}
late_initcall(mt_hotplug_late);
#endif
