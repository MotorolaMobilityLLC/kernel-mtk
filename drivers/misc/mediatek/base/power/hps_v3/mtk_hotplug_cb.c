/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
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
#include <linux/suspend.h>

#include <asm/cacheflush.h>
#ifdef CONFIG_ARM64
#include <asm/cpu_ops.h>
#endif

#include <mt-plat/mtk_secure_api.h>
#include <mt-plat/mtk_auxadc_intf.h>
#include <linux/topology.h>
#include "mtk_hps_internal.h"
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) \
|| defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6758) \
|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)
#include "include/pmic_regulator.h"
#include "mtk_pmic_regulator.h"
#include "mach/mtk_freqhopping.h"
#endif

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mach/mtk_cpufreq_api.h>
#include <mtk_iccs.h>
#endif

#define BUCK_CTRL_DBLOG		(1)
static int MP0_BUCK_STATUS;
static int MP1_BUCK_STATUS;
static int MP2_BUCK_STATUS;

static struct notifier_block cpu_hotplug_nb;
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)
static struct notifier_block hps_pm_notifier_func;
#endif
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

#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)

	int ret;
#endif
	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (cpu < cpumask_weight(mtk_cpu_cluster0_mask)) {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster0_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2, 0);
					break;
				}
#endif
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) \
|| defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6758) \
|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)
				/*1. Turn on ARM PLL*/
				armpll_control(1, 1);

				/*2. Non-pause FQHP function*/
#if 0
				if (action == CPU_UP_PREPARE_FROZEN)
					mt_pause_armpll(FH_PLL0, 0);
				else
#endif
#ifdef CONFIG_MACH_MT6775
					mt_pause_armpll(FH_PLL1, 0);
#else
					mt_pause_armpll(FH_PLL0, 0);
#endif

				/*3. Switch to HW mode*/
				mp_enter_suspend(0, 1);
#endif
#if !defined(CONFIG_MACH_MT6771) && !defined(CONFIG_MACH_MT6775)
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER,
					       0, 0, 0);
#endif
			}
		} else if ((cpu >= cpumask_weight(mtk_cpu_cluster0_mask)) &&
			(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				  cpumask_weight(mtk_cpu_cluster1_mask)))) {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster1_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2, 0);
					break;
				}
#endif
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) \
|| defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6758) \
|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)

#if !defined(CONFIG_MACH_MT6763) && !defined(CONFIG_MACH_MT6758)
				if (hps_ctxt.init_state == INIT_STATE_DONE) {
#if CPU_BUCK_CTRL

#if defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)

					ret = regulator_enable(cpu_vsram11_id);
					if (ret)
						pr_info("regulator_enable vsram11 failed\n");
					dsb(sy);
					ret = regulator_enable(cpu_vproc11_id);
					if (ret)
						pr_info("regulator_enable vproc11 failed\n");
					dsb(sy);
					MP1_BUCK_STATUS = MP_BUCK_ON;
#else
					/*1. Power ON VSram*/
					ret = buck_enable(VSRAM_DVFS2, 1);
					if (ret != 1)
						WARN_ON(1);
					/*2. Set the stttle time to 3000us*/
					dsb(sy);
					mdelay(3);
					dsb(sy);
					/*3. Power ON Vproc2*/
					hps_power_on_vproc2();
					dsb(sy);
					mdelay(1);
					dsb(sy);
#endif
#endif /* CPU_BUCK_CTRL */
				}
#endif
					/*4. Turn on ARM PLL*/
					armpll_control(2, 1);

					/*5. Non-pause FQHP function*/
#if 0
					if (action == CPU_UP_PREPARE_FROZEN)
						mt_pause_armpll(FH_PLL1, 0);
					else
#endif
#ifdef CONFIG_MACH_MT6775
					mt_pause_armpll(FH_PLL2, 0);
#else
					mt_pause_armpll(FH_PLL1, 0);
#endif

					/*6. Switch to HW mode*/
					mp_enter_suspend(1, 1);
#endif
#if !defined(CONFIG_MACH_MT6771) && !defined(CONFIG_MACH_MT6775)
					mt_secure_call(MTK_SIP_POWER_UP_CLUSTER,
						       1, 0, 0);
#endif
			}
		} else if ((cpu >= (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask))) &&
				(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask) +
				cpumask_weight(mtk_cpu_cluster2_mask))))  {
			first_cpu = cpumask_first_and(cpu_online_mask, mtk_cpu_cluster2_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2, 0);
					break;
				}
#endif
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) \
|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)
				/*1. Turn on ARM PLL*/
				armpll_control(3, 1);

				/*2. Non-pause FQHP function*/
#if 0
				if (action == CPU_UP_PREPARE_FROZEN)
					mt_pause_armpll(FH_PLL2, 0);
				else
#endif
					mt_pause_armpll(FH_PLL2, 0);
				/*3. Switch to HW mode*/
				mp_enter_suspend(2, 1);
#endif
#if !defined(CONFIG_MACH_MT6771) && !defined(CONFIG_MACH_MT6775)
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER,
					       2, 0, 0);
#endif
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
#ifdef CONFIG_MTK_ICCS_SUPPORT
			if (hps_get_iccs_pwr_status(cpu >> 2) == 0xb) {
				mt_cpufreq_set_iccs_frequency_by_cluster(1, cpu >> 2, iccs_get_shared_cluster_freq());
				iccs_set_cache_shared_state(cpu >> 2, 1);
				break;
			}
#endif
			mt_secure_call(MTK_SIP_POWER_DOWN_CLUSTER, cpu/4, 0, 0);
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759) \
|| defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6758) \
|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)
			/*pr_info("End of power off cluster %d\n", cpu/4);*/
			switch (cpu/4) {/*Turn off ARM PLL*/
			case 0:
				/*1. Switch to SW mode*/
				mp_enter_suspend(0, 0);

				/*2. Pause FQHP function*/
#ifdef CONFIG_MACH_MT6775
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL1, 0x11);
				else
					mt_pause_armpll(FH_PLL1, 0x01);
#else
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL0, 0x11);
				else
					mt_pause_armpll(FH_PLL0, 0x01);
#endif

				/*3. Turn off ARM PLL*/
				armpll_control(1, 0);
				break;
			case 1:
				/*1. Switch to SW mode*/
				mp_enter_suspend(1, 0);

				/*2. Pause FQHP function*/
#ifdef CONFIG_MACH_MT6775
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL2, 0x11);
				else
					mt_pause_armpll(FH_PLL2, 0x01);
#else
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL1, 0x11);
				else
					mt_pause_armpll(FH_PLL1, 0x01);
#endif

				/*3. Turn off ARM PLL*/
				armpll_control(2, 0);
#if !defined(CONFIG_MACH_MT6763) && !defined(CONFIG_MACH_MT6758)
				if (hps_ctxt.init_state == INIT_STATE_DONE) {
#if CPU_BUCK_CTRL


#if defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775)

					regulator_disable(cpu_vproc11_id);
					regulator_disable(cpu_vsram11_id);
					MP1_BUCK_STATUS = MP_BUCK_OFF;
#else
					/*4. Power off Vproc2*/
					hps_power_off_vproc2();

					/*5. Turn off VSram*/
					ret = buck_enable(VSRAM_DVFS2, 0);
					if (ret == 1)
						WARN_ON(1);
#endif
#endif /* CPU_BUCK_CTRL */
				}
#endif
				break;
			case 2:
				 /*1. Switch to SW mode*/
				mp_enter_suspend(2, 0);
				/*2. Pause FQHP function*/
				if (action == CPU_DEAD_FROZEN)
					mt_pause_armpll(FH_PLL2, 0x11);
				else
					mt_pause_armpll(FH_PLL2, 0x01);

				/*3. Turn off ARM PLL*/
				armpll_control(3, 0);
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
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)
/*HPS PM notifier*/
static int hps_pm_event(struct notifier_block *notifier, unsigned long pm_event,
			void *unused)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&hps_ctxt.lock);
		hps_ctxt.enabled_backup = hps_ctxt.enabled;
		hps_ctxt.enabled = 0;
		mutex_unlock(&hps_ctxt.lock);
		pr_info("[HPS]PM_SUSPEND_PREPARE hps_enabled %d, hps_enabled_backup %d\n",
			hps_ctxt.enabled, hps_ctxt.enabled_backup);
		break;
	case PM_POST_SUSPEND:
		mutex_lock(&hps_ctxt.lock);
		hps_ctxt.enabled = hps_ctxt.enabled_backup;
		mutex_unlock(&hps_ctxt.lock);
		pr_info("[HPS]PM_POST_SUSPEND hps_enabled %d, hps_enabled_backup %d\n",
			hps_ctxt.enabled, hps_ctxt.enabled_backup);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}
#endif

bool cpuhp_is_buck_off(int cluster_idx)
{
	bool ret;

	switch (cluster_idx) {
	case 0:
		if (MP0_BUCK_STATUS == MP_BUCK_OFF)
			ret = true;
		else
			ret = false;
		break;
	case 1:
		if (MP1_BUCK_STATUS == MP_BUCK_OFF)
			ret = true;
		else
			ret = false;
		break;
	case 2:
		if (MP2_BUCK_STATUS == MP_BUCK_OFF)
			ret = true;
		else
			ret = false;
		break;
	default:
		ret = true;
		break;

	}
	return ret;
}

static __init int hotplug_cb_init(void)
{
	int ret;
	int i;

	for (i = setup_max_cpus; i < num_possible_cpus(); i++) {
#ifdef CONFIG_ARM64
		if (!cpu_ops[i])
			WARN_ON(1);
		if (cpu_ops[i]->cpu_prepare(i))
			WARN_ON(1);

		per_cpu(cpu_number, i) = i;
#endif
		set_cpu_present(i, true);
	}


	MP0_BUCK_STATUS = MP1_BUCK_STATUS = MP2_BUCK_STATUS = MP_BUCK_ON;

	mp_enter_suspend(0, 1);/*Switch LL cluster to HW mode*/
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
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)
	hps_pm_notifier_func = (struct notifier_block){
		.notifier_call = hps_pm_event,
		.priority = 0,
	};

	ret = register_pm_notifier(&hps_pm_notifier_func);
	if (ret) {
		pr_debug("Failed to register HPS PM notifier.\n");
		return ret;
	}
	pr_info("HPS PM Notification\n");
#endif

	return 0;
}
early_initcall(hotplug_cb_init);
