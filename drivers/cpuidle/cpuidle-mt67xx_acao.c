/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>

#include <asm/cpuidle.h>
#include <asm/suspend.h>

#define CREATE_TRACE_POINTS
#include <trace/events/mtk_idle_event.h>

#define USING_TICK_BROADCAST

static DECLARE_BITMAP(cpu_set_0_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_set_0_mask = to_cpumask(cpu_set_0_bits);

static DECLARE_BITMAP(cpu_set_1_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_set_1_mask = to_cpumask(cpu_set_1_bits);

static DECLARE_BITMAP(cpu_set_2_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_set_2_mask = to_cpumask(cpu_set_2_bits);

static unsigned long default_set_0_mask = 0x000F;
static unsigned long default_set_1_mask = 0x0070;
static unsigned long default_set_2_mask = 0x0080;

int __attribute__((weak)) rgidle_enter(int cpu)
{
	return 1;
}

int __attribute__((weak)) mcdi_enter(int cpu)
{
	return 1;
}

static int mtk_rgidle_enter(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv, int index)
{
	rgidle_enter(smp_processor_id());
	return index;
}

static int mtk_mcidle_enter(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv, int index)
{
	mcdi_enter(smp_processor_id());
	return index;
}

static struct cpuidle_driver mt67xx_acao_cpuidle_driver_set_0 = {
	.name             = "mt67xx_acao_cpuidle_set_0",
	.owner            = THIS_MODULE,
	.states[0] = {
		.enter			= mtk_rgidle_enter,
		.exit_latency		= 1,
		.target_residency	= 1,
		.name			= "rgidle",
		.desc			= "wfi"
	},
	.states[1] = {
		.enter			= mtk_mcidle_enter,
		.exit_latency		= 300,
		.target_residency	= 4500,
#ifdef USING_TICK_BROADCAST
		.flags			= CPUIDLE_FLAG_TIMER_STOP,
#endif
		.name			= "mcdi",
		.desc			= "mcdi",
	},
	.state_count = 2,
	.safe_state_index = 0,
};

static struct cpuidle_driver mt67xx_acao_cpuidle_driver_set_1 = {
	.name             = "mt67xx_acao_cpuidle_set_1",
	.owner            = THIS_MODULE,
	.states[0] = {
		.enter			= mtk_rgidle_enter,
		.exit_latency		= 1,
		.target_residency	= 1,
		.name			= "rgidle",
		.desc			= "wfi"
	},
	.states[1] = {
		.enter			= mtk_mcidle_enter,
		.exit_latency		= 300,
		.target_residency	= 1500,
#ifdef USING_TICK_BROADCAST
		.flags			= CPUIDLE_FLAG_TIMER_STOP,
#endif
		.name			= "mcdi",
		.desc			= "mcdi",
	},
	.state_count = 2,
	.safe_state_index = 0,
};

static struct cpuidle_driver mt67xx_acao_cpuidle_driver_set_2 = {
	.name             = "mt67xx_acao_cpuidle_set_2",
	.owner            = THIS_MODULE,
	.states[0] = {
		.enter			= mtk_rgidle_enter,
		.exit_latency		= 1,
		.target_residency	= 1,
		.name			= "rgidle",
		.desc			= "wfi"
	},
	.states[1] = {
		.enter			= mtk_mcidle_enter,
		.exit_latency		= 300,
		.target_residency	= 850,
#ifdef USING_TICK_BROADCAST
		.flags			= CPUIDLE_FLAG_TIMER_STOP,
#endif
		.name			= "mcdi",
		.desc			= "mcdi",
	},
	.state_count = 2,
	.safe_state_index = 0,
};

int __init mt67xx_acao_cpuidle_init(void)
{
	int cpu, ret;
	struct cpuidle_device *dev;

	/*
	 * Initialize idle states data, starting at index 1.
	 * This driver is DT only, if no DT idle states are detected (ret == 0)
	 * let the driver initialization fail accordingly since there is no
	 * reason to initialize the idle driver if only wfi is supported.
	 */
#if 0
	ret = dt_init_idle_driver(drv, mt67xx_v2_idle_state_match, 1);
	if (ret <= 0)
		return ret ? : -ENODEV;
#endif

	cpumask_clear(cpu_set_0_mask);
	cpumask_clear(cpu_set_1_mask);
	cpumask_clear(cpu_set_2_mask);

	cpu_set_0_mask->bits[0] = default_set_0_mask;
	cpu_set_1_mask->bits[0] = default_set_1_mask;
	cpu_set_2_mask->bits[0] = default_set_2_mask;

	mt67xx_acao_cpuidle_driver_set_0.cpumask = cpu_set_0_mask;
	mt67xx_acao_cpuidle_driver_set_1.cpumask = cpu_set_1_mask;
	mt67xx_acao_cpuidle_driver_set_2.cpumask = cpu_set_2_mask;

	ret = cpuidle_register_driver(&mt67xx_acao_cpuidle_driver_set_0);
	if (ret) {
		pr_info("Failed to register cpuidle driver 0\n");
		return ret;
	}

	ret = cpuidle_register_driver(&mt67xx_acao_cpuidle_driver_set_1);
	if (ret) {
		pr_info("Failed to register cpuidle driver 1\n");
		return ret;
	}

	ret = cpuidle_register_driver(&mt67xx_acao_cpuidle_driver_set_2);
	if (ret) {
		pr_info("Failed to register cpuidle driver 2\n");
		return ret;
	}

	/*
	 * Call arch CPU operations in order to initialize
	 * idle states suspend back-end specific data
	 */
	for_each_possible_cpu(cpu) {
		ret = arm_cpuidle_init(cpu);

		/*
		 * Skip the cpuidle device initialization if the reported
		 * failure is a HW misconfiguration/breakage (-ENXIO).
		 */
		if (ret == -ENXIO)
			continue;

		if (ret) {
			pr_info("CPU %d failed to init idle CPU ops\n", cpu);
			goto out_fail;
		}

		dev = kzalloc(sizeof(*dev), GFP_KERNEL);
		if (!dev)
			goto out_fail;
		dev->cpu = cpu;

		ret = cpuidle_register_device(dev);
		if (ret) {
			pr_info("Failed to register cpuidle device for CPU %d\n",
			       cpu);
			kfree(dev);
			goto out_fail;
		}
	}

	return 0;

out_fail:
	while (--cpu >= 0) {
		dev = per_cpu(cpuidle_devices, cpu);
		cpuidle_unregister_device(dev);
		kfree(dev);
	}

	cpuidle_unregister_driver(&mt67xx_acao_cpuidle_driver_set_0);
	cpuidle_unregister_driver(&mt67xx_acao_cpuidle_driver_set_1);
	cpuidle_unregister_driver(&mt67xx_acao_cpuidle_driver_set_2);

	return ret;
}

device_initcall(mt67xx_acao_cpuidle_init);
