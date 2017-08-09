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

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	/* Make sure this is visible to other CPUs */
	smp_wmb();
	/* sync_cache_w(&pen_release); */
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

void __cpuinit mt_smp_secondary_init(unsigned int cpu)
{
	pr_debug("Slave cpu init\n");
	HOTPLUG_INFO("platform_secondary_init, cpu: %d\n", cpu);

#ifndef CONFIG_MTK_GIC
	mt_gic_secondary_init();
#endif

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

#if !defined(CONFIG_ARM_PSCI)
	fiq_glue_resume();
#endif
	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

#define MT6735_INFRACFG_AO	0x10001000

static void __init smp_set_boot_addr(void)
{
	static void __iomem *infracfg_ao_base;

	infracfg_ao_base = ioremap(MT6735_INFRACFG_AO, 0x1000);

	if (!infracfg_ao_base)
		pr_err("%s: Unable to map I/O memory\n", __func__);

	/* Write the address of slave startup into boot address
	   register for bootrom power down mode */

	writel_relaxed(virt_to_phys(mt_secondary_startup),
			infracfg_ao_base + 0x800);

	iounmap(infracfg_ao_base);
}

int __cpuinit mt_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

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

#ifdef CONFIG_ARCH_MT6753
	case 4:
		#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE4_MAGIC_NUM, SLAVE4_MAGIC_REG);
		HOTPLUG_INFO("SLAVE4_MAGIC_NUM:%x\n", SLAVE4_MAGIC_NUM);
		#endif
		spm_mtcmos_ctrl_cpu4(STA_POWER_ON, 1);
		break;

	case 5:
		if ((cpu_online(4) == 0) && (cpu_online(6) == 0) &&
		    (cpu_online(7) == 0)) {
			HOTPLUG_INFO("up CPU%d fail, CPU4 first\n", cpu);
			spin_unlock(&boot_lock);
			atomic_dec(&hotplug_cpu_count);
			return -ENOSYS;
		}
		#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE5_MAGIC_NUM, SLAVE5_MAGIC_REG);
		HOTPLUG_INFO("SLAVE5_MAGIC_NUM:%x\n", SLAVE5_MAGIC_NUM);
		#endif
		spm_mtcmos_ctrl_cpu5(STA_POWER_ON, 1);
		break;

	case 6:
		if ((cpu_online(4) == 0) && (cpu_online(5) == 0) &&
		    (cpu_online(7) == 0)) {
			HOTPLUG_INFO("up CPU%d fail, CPU4 first\n", cpu);
			spin_unlock(&boot_lock);
			atomic_dec(&hotplug_cpu_count);
			return -ENOSYS;
		}
		#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE6_MAGIC_NUM, SLAVE6_MAGIC_REG);
		HOTPLUG_INFO("SLAVE6_MAGIC_NUM:%x\n", SLAVE6_MAGIC_NUM);
		#endif
		spm_mtcmos_ctrl_cpu6(STA_POWER_ON, 1);
		break;

	case 7:
		if ((cpu_online(4) == 0) && (cpu_online(5) == 0) &&
		    (cpu_online(6) == 0)) {
			HOTPLUG_INFO("up CPU%d fail, CPU4 first\n", cpu);
			spin_unlock(&boot_lock);
			atomic_dec(&hotplug_cpu_count);
			return -ENOSYS;
		}
		#ifdef CONFIG_MTK_FPGA
		mt_reg_sync_writel(SLAVE7_MAGIC_NUM, SLAVE7_MAGIC_REG);
		HOTPLUG_INFO("SLAVE7_MAGIC_NUM:%x\n", SLAVE7_MAGIC_NUM);
		#endif
		spm_mtcmos_ctrl_cpu7(STA_POWER_ON, 1);
		break;
#endif /* CONFIG_ARCH_MT6753 */

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
	return 0;
}

void __init mt_smp_init_cpus(void)
{
	/* Enable CA7 snoop function */
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
	mcusys_smc_write_phy(virt_to_phys((void *)MP0_AXI_CONFIG),
			     readl((void *)MP0_AXI_CONFIG) & ~ACINACTM);
#else
	mcusys_smc_write(MP0_AXI_CONFIG, readl(MP0_AXI_CONFIG) & ~ACINACTM);
#endif

	/* Enable snoop requests and DVM message requests */
	REG_WRITE((void *)CCI400_SI4_SNOOP_CONTROL,
		  readl((void *)CCI400_SI4_SNOOP_CONTROL) | (SNOOP_REQ |
							DVM_MSG_REQ));
	while (readl((void *)CCI400_STATUS) & CHANGE_PENDING)
		;

	pr_emerg("@@@### num_possible_cpus(): %u ###@@@\n",
		num_possible_cpus());
	pr_emerg("@@@### num_present_cpus(): %u ###@@@\n", num_present_cpus());

#ifndef CONFIG_MTK_GIC
	irq_total_secondary_cpus = num_possible_cpus() - 1;
#endif
}

void __init mt_smp_prepare_cpus(unsigned int max_cpus)
{
#if !defined(CONFIG_ARM_PSCI)

#ifdef CONFIG_MTK_FPGA
	/* write the address of slave startup into the system-wide
	   flags register */
	mt_reg_sync_writel(virt_to_phys(mt_secondary_startup), SLAVE_JUMP_REG);
#endif

	/* Set all cpus into AArch32 */
	mcusys_smc_write(MP0_MISC_CONFIG3,
			 readl(MP0_MISC_CONFIG3) & 0xFFFF0FFF);
	mcusys_smc_write(MP1_MISC_CONFIG3,
			 readl(MP1_MISC_CONFIG3) & 0xFFFF0FFF);

	/* enable bootrom power down mode */
	REG_WRITE(BOOTROM_SEC_CTRL, readl(BOOTROM_SEC_CTRL) | SW_ROM_PD);

	/* write the address of slave startup into boot address register
	   for bootrom power down mode */
	mt_reg_sync_writel(virt_to_phys(mt_secondary_startup),
			   BOOTROM_BOOT_ADDR);
#endif
	/* set boot address */
	smp_set_boot_addr();

	/* initial spm_mtcmos memory map */
	spm_mtcmos_cpu_init();
}

struct smp_operations __initdata mt_smp_ops = {
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
