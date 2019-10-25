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
#include <asm/cacheflush.h>
#include <linux/bug.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <mach/mtk_clkmgr.h>
#include <mt_freqhopping.h>
#include <mtk_vcorefs_manager.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#define CPU_PRI_PERF 20

#define CONFIG_ARMPLL_CTRL 1

#ifdef CONFIG_ARMPLL_CTRL

#define MCU_FH_PLL0 0
#define MCU_FH_PLL1 1
#define MCU_FH_PLL2 2
#define MCU_FH_PLL3 3

#ifdef CONFIG_OF
void __iomem *clk_apmixed_base1;
#define AP_PLL_CON3_1 (clk_apmixed_base1 + 0x0C)
#define AP_PLL_CON4_1 (clk_apmixed_base1 + 0x10)
#define ARMPLL_LL_CON0_1 (clk_apmixed_base1 + 0x200)
#define ARMPLL_LL_PWR_CON0_1 (clk_apmixed_base1 + 0x20C)
#define ARMPLL_L_CON0_1 (clk_apmixed_base1 + 0x210)
#define ARMPLL_L_PWR_CON0_1 (clk_apmixed_base1 + 0x21C)

#define clk_readl(addr) readl(addr)
#define clk_writel(addr, val) writel_relaxed(val, addr)

static void iomap(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	if (!node)
		pr_debug("[CLK_APMIXED] find node failed\n");
	clk_apmixed_base1 = of_iomap(node, 0);
	if (!clk_apmixed_base1)
		pr_debug("[CLK_APMIXED] base failed\n");
}

void enable_armpll_ll(void)
{
	clk_writel(ARMPLL_LL_PWR_CON0_1,
		   (clk_readl(ARMPLL_LL_PWR_CON0_1) | 0x01));
	udelay(1);
	clk_writel(ARMPLL_LL_PWR_CON0_1,
		   (clk_readl(ARMPLL_LL_PWR_CON0_1) & 0xfffffffd));
	clk_writel(ARMPLL_LL_CON0_1, (clk_readl(ARMPLL_LL_CON0_1) | 0x01));
	udelay(200);
}

void disable_armpll_ll(void)
{
	clk_writel(ARMPLL_LL_CON0_1,
		   (clk_readl(ARMPLL_LL_CON0_1) & 0xfffffffe));
	clk_writel(ARMPLL_LL_PWR_CON0_1,
		   (clk_readl(ARMPLL_LL_PWR_CON0_1) | 0x00000002));
	clk_writel(ARMPLL_LL_PWR_CON0_1,
		   (clk_readl(ARMPLL_LL_PWR_CON0_1) & 0xfffffffe));
}

void enable_armpll_l(void)
{
	clk_writel(ARMPLL_L_PWR_CON0_1,
		   (clk_readl(ARMPLL_L_PWR_CON0_1) | 0x01));
	udelay(1);
	clk_writel(ARMPLL_L_PWR_CON0_1,
		   (clk_readl(ARMPLL_L_PWR_CON0_1) & 0xfffffffd));
	clk_writel(ARMPLL_L_CON0_1, (clk_readl(ARMPLL_L_CON0_1) | 0x01));
	udelay(200);
}

void disable_armpll_l(void)
{
	clk_writel(ARMPLL_L_CON0_1, (clk_readl(ARMPLL_L_CON0_1) & 0xfffffffe));
	clk_writel(ARMPLL_L_PWR_CON0_1,
		   (clk_readl(ARMPLL_L_PWR_CON0_1) | 0x00000002));
	clk_writel(ARMPLL_L_PWR_CON0_1,
		   (clk_readl(ARMPLL_L_PWR_CON0_1) & 0xfffffffe));
}

void switch_armpll_l_hwmode(int enable)
{
	/* ARM CA15*/
	if (enable == 1) {
		clk_writel(AP_PLL_CON3_1,
			   (clk_readl(AP_PLL_CON3_1) & 0xFFFEEEEF));
		clk_writel(AP_PLL_CON4_1,
			   (clk_readl(AP_PLL_CON4_1) & 0xFFFFFFFE));

	} else {
		clk_writel(AP_PLL_CON3_1,
			   (clk_readl(AP_PLL_CON3_1) | 0x00011110));
		clk_writel(AP_PLL_CON4_1,
			   (clk_readl(AP_PLL_CON4_1) | 0x00000001));
	}
}

void switch_armpll_ll_hwmode(int enable)
{
	/* ARM CA7*/
	if (enable == 1) {
		clk_writel(AP_PLL_CON3_1,
			   (clk_readl(AP_PLL_CON3_1) & 0xFF87FFFF));
		clk_writel(AP_PLL_CON4_1,
			   (clk_readl(AP_PLL_CON4_1) & 0xFFFFDFFF));
	} else {
		clk_writel(AP_PLL_CON3_1,
			   (clk_readl(AP_PLL_CON3_1) | 0x00780000));
		clk_writel(AP_PLL_CON4_1,
			   (clk_readl(AP_PLL_CON4_1) | 0x00002000));
	}
}
#endif /* #ifdef CONFIG_OF */

#endif /* CONFIG_ARMPLL_CTRL */

static DECLARE_BITMAP(cpu_cluster0_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_cluster0_mask = to_cpumask(cpu_cluster0_bits);

static DECLARE_BITMAP(cpu_cluster1_bits, CONFIG_NR_CPUS);
struct cpumask *cpu_cluster1_mask = to_cpumask(cpu_cluster1_bits);

static struct notifier_block cpu_hotplug_lowpower_nb;

static unsigned long default_cluster0_mask = 0x000F;
static unsigned long default_cluster1_mask = 0x00F0;

static int cpu_hotplug_lowpower_notifier(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	unsigned int cpu = (long)hcpu;
	unsigned int first_cpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (cpu < cpumask_weight(cpu_cluster0_mask)) {
			first_cpu = cpumask_first_and(cpu_online_mask,
						      cpu_cluster0_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_ARMPLL_CTRL
				/* turn on arm pll */
				enable_armpll_ll();
				/* non-pause FQHP function */
				mt_pause_armpll(MCU_FH_PLL0, 0);
				/* switch to HW mode */
				switch_armpll_ll_hwmode(1);
#endif
			}
		} else if (cpu < (cpumask_weight(cpu_cluster0_mask) +
				  cpumask_weight(cpu_cluster1_mask))) {
			first_cpu = cpumask_first_and(cpu_online_mask,
						      cpu_cluster1_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_ARMPLL_CTRL
				/* turn on arm pll */
				enable_armpll_l();
				/* non-pause FQHP function */
				mt_pause_armpll(MCU_FH_PLL1, 0);
				/* switch to HW mode */
				switch_armpll_l_hwmode(1);
#endif
				vcorefs_request_dvfs_opp(KIR_CPU, OPPI_LOW_PWR);
			}
		} else {
			pr_debug("CPU%d is not exist\n", cpu);
			WARN_ON(1);
		}

		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		if (cpu < cpumask_weight(cpu_cluster0_mask)) {
			first_cpu = cpumask_first_and(cpu_online_mask,
						      cpu_cluster0_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_ARMPLL_CTRL
				/* switch to SW mode */
				switch_armpll_ll_hwmode(0);
				/* pause FQHP function */
				mt_pause_armpll(MCU_FH_PLL0, 1);
				/* turn off arm pll */
				disable_armpll_ll();
#endif
			}
		} else if (cpu < (cpumask_weight(cpu_cluster0_mask) +
				  cpumask_weight(cpu_cluster1_mask))) {
			first_cpu = cpumask_first_and(cpu_online_mask,
						      cpu_cluster1_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_ARMPLL_CTRL
				/* switch to SW mode */
				switch_armpll_l_hwmode(0);
				/* pause FQHP function */
				mt_pause_armpll(MCU_FH_PLL1, 1);
				/* turn off arm pll */
				disable_armpll_l();
#endif
				vcorefs_request_dvfs_opp(KIR_CPU, OPPI_UNREQ);
			}
		} else {
			pr_debug("CPU%d is not exist\n", cpu);
			WARN_ON(1);
		}
		break;
#endif /* CONFIG_HOTPLUG_CPU */

	default:
		break;
	}

	return NOTIFY_OK;
}

static __init int hotplug_lowpower_init(void)
{
	int ret;

#ifdef CONFIG_ARMPLL_CTRL
	iomap();
#endif

	cpumask_clear(cpu_cluster0_mask);
	cpumask_clear(cpu_cluster1_mask);
	cpu_cluster0_mask->bits[0] = default_cluster0_mask;
	cpu_cluster1_mask->bits[0] = default_cluster1_mask;

	/*
	 * pr_info("Cluster0 CPUs mask: %08X\n",
	 *	(unsigned int)cpumask_bits(cpu_cluster0_mask)[0]);
	 * pr_info("Cluster1 CPUs mask: %08X\n",
	 *	(unsigned int)cpumask_bits(cpu_cluster1_mask)[0]);
	 */

	cpu_hotplug_lowpower_nb = (struct notifier_block){
	    .notifier_call = cpu_hotplug_lowpower_notifier,
	    .priority = CPU_PRI_PERF + 1,
	};

	ret = register_cpu_notifier(&cpu_hotplug_lowpower_nb);
	if (ret)
		return ret;

	pr_info("CPU Hotplug Low Power Notification\n");

	return 0;
}

early_initcall(hotplug_lowpower_init);
