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
#include <mt-plat/mtk_secure_api.h>
#include <mt-plat/mtk_auxadc_intf.h>
#include <linux/topology.h>
#include "mtk_hps_internal.h"
#ifdef CONFIG_MACH_MT6799
#include "include/pmic_regulator.h"
#include "mtk_pmic_regulator.h"
#include "mach/mtk_freqhopping.h"
#endif
#define CPU_BUCK_CTRL	(0)

static struct notifier_block cpu_hotplug_nb;

static DECLARE_BITMAP(cpu_cluster0_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster0_mask = to_cpumask(cpu_cluster0_bits);

static DECLARE_BITMAP(cpu_cluster1_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster1_mask = to_cpumask(cpu_cluster1_bits);

static DECLARE_BITMAP(cpu_cluster2_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster2_mask = to_cpumask(cpu_cluster2_bits);

static unsigned long default_cluster0_mask = 0x000F;
static unsigned long default_cluster1_mask = 0x00F0;
static unsigned long default_cluster2_mask = 0x0300;

static int cpu_hotplug_cb_notifier(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	unsigned int cpu = (long)hcpu;
	struct cpumask cpuhp_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int first_cpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (cpu < cpumask_weight(mtk_cpu_cluster0_mask)) {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster0_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#if CPU_BUCK_CTRL
				/*1. Turn on ARM PLL*/
				armpll_control(1, 1);
				/*2. Non-pause FQHP function*/
				if (action == CPU_UP_PREPARE_FROZEN)
					mt_pause_armpll(FH_PLL0, 0, 1);
				else
					mt_pause_armpll(FH_PLL0, 0, 0)

				/*3. Switch to HW mode*/
				mp_enter_suspend(0, 1);
#endif
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER, 0, 0, 0);
			}
		} else if ((cpu >= cpumask_weight(mtk_cpu_cluster0_mask)) &&
			(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				  cpumask_weight(mtk_cpu_cluster1_mask)))) {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster1_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#if CPU_BUCK_CTRL
				if (hps_ctxt.init_state == INIT_STATE_DONE) {
					/*1. Power ON VSram*/
#ifdef CONFIG_MACH_MT6799
					buck_enable(VSRAM_DVFS2, 1);
					/*2. Set the stttle time to 3000us*/
					mdelay(3);
					/*3. Power ON Vproc2*/
					hps_power_on_vproc2();
#endif
					/*4. Turn on ARM PLL*/
					if (action == CPU_UP_PREPARE_FROZEN)
						mt_pause_armpll(FH_PLL1, 0, 1);
					else
						mt_pause_armpll(FH_PLL1, 0, 0);

					/*5. Non-pause FQHP function*/
					mt_pause_armpll(FH_PLL1, 0);
					/*6. Switch to HW mode*/
					mp_enter_suspend(1, 1);
				}
#endif
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER, 1, 0, 0);
			}
		} else if ((cpu >= (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask))) &&
				(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask) +
				cpumask_weight(mtk_cpu_cluster2_mask))))  {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster2_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#if CPU_BUCK_CTRL
				/*1. Turn on ARM PLL*/
				armpll_control(3, 1);

				/*2. Non-pause FQHP function*/
				if (action == CPU_UP_PREPARE_FROZEN)
					mt_pause_armpll(FH_PLL2, 0, 1);
				else
					mt_pause_armpll(FH_PLL2, 0, 0);
#endif
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER, 2, 0, 0);
			}
		}
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
#if CPU_BUCK_CTRL
			switch (cpu/4) {/*Turn off ARM PLL*/
			case 0:
				/*1. Switch to SW mode*/
				mp_enter_suspend(0, 0);

				/*2. Pause FQHP function*/
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL0, 1, 1);
				else
					mt_pause_armpll(FH_PLL0, 1, 0);

				/*3. Turn off ARM PLL*/
				armpll_control(1, 0);
				break;
			case 1:
				/*1. Switch to SW mode*/
				mp_enter_suspend(1, 0);

				/*2. Pause FQHP function*/
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL1, 1, 1);
				else
					mt_pause_armpll(FH_PLL1, 1, 0)

				/*3. Turn off ARM PLL*/
				armpll_control(2, 0);
#ifdef CONFIG_MACH_MT6799
				if (hps_ctxt.init_state == INIT_STATE_DONE) {
					hps_power_off_vproc2(); /*4. Power off Vproc2*/
					buck_enable(VSRAM_DVFS2, 0);/*5. Turn off VSram*/
				}
#endif
				break;
			case 2:
				/*1. Pause FQHP function*/
				mt_pause_armpll(FH_PLL2, 1);

				/*2. Turn off ARM PLL*/
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL2, 1, 1);
				else
					mt_pause_armpll(FH_PLL2, 1, 0);
				break;
			default:
				break;
			}
#endif
		}
		break;
#endif /* CONFIG_HOTPLUG_CPU */

	default:
		break;
	}
	return NOTIFY_OK;
}
static __init int hotplug_cb_init(void)
{
	int ret;
	int i;

	cpumask_clear(mtk_cpu_cluster0_mask);
	cpumask_clear(mtk_cpu_cluster1_mask);
	cpumask_clear(mtk_cpu_cluster2_mask);
	mtk_cpu_cluster0_mask->bits[0] = default_cluster0_mask;
	mtk_cpu_cluster1_mask->bits[0] = default_cluster1_mask;
	mtk_cpu_cluster2_mask->bits[0] = default_cluster2_mask;
	for (i = 0; i < num_possible_cpus(); i++)
		set_cpu_present(i, true);
	cpu_hotplug_nb = (struct notifier_block) {
		.notifier_call	= cpu_hotplug_cb_notifier,
		.priority	= CPU_PRI_PERF + 1,
	};

	ret = register_cpu_notifier(&cpu_hotplug_nb);
	if (ret)
		return ret;

	pr_info("CPU Hotplug Low Power Notification\n");

	return 0;
}
early_initcall(hotplug_cb_init);
