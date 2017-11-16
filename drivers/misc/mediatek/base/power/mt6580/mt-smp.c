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
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <asm/fiq_glue.h>
#include <mt-plat/sync_write.h>
#include <mach/mt_spm_mtcmos.h>
#include <mach/mt_secure_api.h>
#if defined(CONFIG_TRUSTY)
#include <mach/mt_trusty_api.h>
#endif
#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#include <teei_secure_api.h>
#endif

#include "mt-smp.h"
#include "smp.h"
#include "hotplug.h"

#define SLAVE1_MAGIC_REG (SRAMROM_BASE+0x38)
#define SLAVE2_MAGIC_REG (SRAMROM_BASE+0x38)
#define SLAVE3_MAGIC_REG (SRAMROM_BASE+0x38)
#define SLAVE4_MAGIC_REG (SRAMROM_BASE+0x3C)
#define SLAVE5_MAGIC_REG (SRAMROM_BASE+0x3C)
#define SLAVE6_MAGIC_REG (SRAMROM_BASE+0x3C)
#define SLAVE7_MAGIC_REG (SRAMROM_BASE+0x3C)

#define SLAVE1_MAGIC_NUM 0x534C4131
#define SLAVE2_MAGIC_NUM 0x4C415332
#define SLAVE3_MAGIC_NUM 0x41534C33
#define SLAVE4_MAGIC_NUM 0x534C4134
#define SLAVE5_MAGIC_NUM 0x4C415335
#define SLAVE6_MAGIC_NUM 0x41534C36
#define SLAVE7_MAGIC_NUM 0x534C4137

#define SLAVE_JUMP_REG  (SRAMROM_BASE+0x34)

static DEFINE_SPINLOCK(boot_lock);

#define MT6580_INFRACFG_AO	0x10001000
#define SW_ROM_PD		BIT(31)

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	/* */
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

void __cpuinit mt_smp_secondary_init(unsigned int cpu)
{
	pr_debug("Slave cpu init\n");
	HOTPLUG_INFO("platform_secondary_init, cpu: %d\n", cpu);

	mt_gic_secondary_init();

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	fiq_glue_resume();

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit mt_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	static void __iomem *infracfg_ao_base;

	pr_crit("Boot slave CPU\n");

	infracfg_ao_base = ioremap(MT6580_INFRACFG_AO, 0x1000);

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
	if (cpu >= 1 && cpu <= 3)
		mt_secure_call(MC_FC_SET_RESET_VECTOR, virt_to_phys(mt_secondary_startup), cpu, 0);
#elif defined(CONFIG_TRUSTY)
	if (cpu >= 1 && cpu <= 3)
		mt_trusty_call(SMC_FC_CPU_ON, virt_to_phys(mt_secondary_startup), cpu, 0);
#elif defined(CONFIG_MICROTRUST_TEE_SUPPORT)
	if (cpu >= 1 && cpu <= 3)
		teei_secure_call(TEEI_FC_CPU_ON, virt_to_phys(mt_secondary_startup), cpu, 0);
#else
	writel_relaxed(virt_to_phys(mt_secondary_startup), infracfg_ao_base + 0x800);
#endif
	iounmap(infracfg_ao_base);

	atomic_inc(&hotplug_cpu_count);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	HOTPLUG_INFO("mt_smp_boot_secondary, cpu: %d\n", cpu);
	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu);

	switch (cpu) {
	case 1:
#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE1_MAGIC_NUM, SLAVE1_MAGIC_REG);
		HOTPLUG_INFO("SLAVE1_MAGIC_NUM:%x\n", SLAVE1_MAGIC_NUM);
#endif
		spm_mtcmos_ctrl_cpu1(STA_POWER_ON, 1);
		break;
	case 2:
#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE2_MAGIC_NUM, SLAVE2_MAGIC_REG);
		HOTPLUG_INFO("SLAVE2_MAGIC_NUM:%x\n", SLAVE2_MAGIC_NUM);
#endif
		spm_mtcmos_ctrl_cpu2(STA_POWER_ON, 1);
		break;
	case 3:
#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE3_MAGIC_NUM, SLAVE3_MAGIC_REG);
		HOTPLUG_INFO("SLAVE3_MAGIC_NUM:%x\n", SLAVE3_MAGIC_NUM);
#endif
		spm_mtcmos_ctrl_cpu3(STA_POWER_ON, 1);
		break;
	default:
		break;

	}

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		/* */
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	if (pen_release != -1) {
		on_each_cpu((smp_call_func_t) dump_stack, NULL, 0);
		atomic_dec(&hotplug_cpu_count);
		return -ENOSYS;
	}

#ifdef CONFIG_MTK_IRQ_NEW_DESIGN
	gic_clear_primask();
#endif

	return 0;
}

void __init mt_smp_init_cpus(void)
{
	pr_emerg("@@@### num_possible_cpus(): %u ###@@@\n",
		num_possible_cpus());
	pr_emerg("@@@### num_present_cpus(): %u ###@@@\n", num_present_cpus());

	irq_total_secondary_cpus = num_possible_cpus() - 1;
}

void __init mt_smp_prepare_cpus(unsigned int max_cpus)
{
	static void __iomem *infracfg_ao_base;

	infracfg_ao_base = ioremap(MT6580_INFRACFG_AO, 0x1000);

	/* enable bootrom power down mode */
	writel_relaxed(readl(infracfg_ao_base + 0x804) | SW_ROM_PD,
		       infracfg_ao_base + 0x804);

	/* write the address of slave startup into boot address register
	   for bootrom power down mode */
	writel_relaxed(virt_to_phys(mt_secondary_startup),
		       infracfg_ao_base + 0x800);

	iounmap(infracfg_ao_base);

	/* initial spm_mtcmos memory map */
	spm_mtcmos_cpu_init();
}

static struct smp_operations mt6580_smp_ops __initdata = {
	.smp_init_cpus = mt_smp_init_cpus,
	.smp_prepare_cpus = mt_smp_prepare_cpus,
	.smp_secondary_init = mt_smp_secondary_init,
	.smp_boot_secondary = mt_smp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_kill = mt_cpu_kill,
	.cpu_die = mt_cpu_die,
	.cpu_disable = mt_cpu_disable,
#endif
};
CPU_METHOD_OF_DECLARE(mt6580_smp, "mediatek,mt6580-smp", &mt6580_smp_ops);
