/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Cheng-En Chung <cheng-en.chung@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/irqchip/mtk-gic.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/psci.h>

#include <asm/arch_timer.h>
#include <asm/cacheflush.h>
#include <asm/cpuidle.h>
#include <asm/irqflags.h>
#include <asm/neon.h>
#include <asm/suspend.h>

#include <mt-plat/mtk_dbg.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>

#include <mtk_clkmgr.h>
#include <mtk_cpuidle.h>
#if defined(CONFIG_MTK_RAM_CONSOLE) || defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
#include <mtk_secure_api.h>
#endif
#include <mtk_spm.h>
#include <mtk_spm_misc.h>

#ifdef CONFIG_MTK_RAM_CONSOLE
static volatile void __iomem *mtk_cpuidle_aee_phys_addr;
static volatile void __iomem *mtk_cpuidle_aee_virt_addr;
#endif

#if MTK_CPUIDLE_TIME_PROFILING
static u64 mtk_cpuidle_timestamp[CONFIG_NR_CPUS][MTK_CPUIDLE_TIMESTAMP_COUNT];
static char mtk_cpuidle_timestamp_buf[1024] = { 0 };
#endif

static unsigned int kp_irq_nr;
static unsigned int conn_wdt_irq_nr;
static unsigned int lowbattery_irq_nr;
static unsigned int md1_wdt_irq_nr;
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
static unsigned int c2k_wdt_irq_nr;
#endif
#endif

#define CPU_IDLE_STA_OFFSET 10

static unsigned long dbg_data[40];
static int mtk_cpuidle_initialized;

static void mtk_spm_irq_set_pending(int wakeup_src, int irq_nr)
{
	if (spm_read(SPM_WAKEUP_STA) & wakeup_src)
		mt_irq_set_pending(irq_nr);
}

static void mtk_spm_wakeup_src_restore(void)
{
	/* Set the pending bit for spm wakeup source that is edge triggerd */
	mtk_spm_irq_set_pending(WAKE_SRC_R12_KP_IRQ_B, kp_irq_nr);
	mtk_spm_irq_set_pending(WAKE_SRC_R12_CONN_WDT_IRQ_B, conn_wdt_irq_nr);
	mtk_spm_irq_set_pending(WAKE_SRC_R12_LOWBATTERY_IRQ_B, lowbattery_irq_nr);
	mtk_spm_irq_set_pending(WAKE_SRC_R12_MD1_WDT_B, md1_wdt_irq_nr);
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	mtk_spm_irq_set_pending(WAKE_SRC_R12_C2K_WDT_IRQ_B, c2k_wdt_irq_nr);
#endif
#endif
}

static void mtk_cpuidle_timestamp_init(void)
{
#if MTK_CPUIDLE_TIME_PROFILING
	int i, k;

	for (i = 0; i < CONFIG_NR_CPUS; i++)
		for (k = 0; k < MTK_CPUIDLE_TIMESTAMP_COUNT; k++)
			mtk_cpuidle_timestamp[i][k] = 0;

	kernel_smc_msg(0, 1, virt_to_phys(mtk_cpuidle_timestamp));
#endif
}

static void mtk_cpuidle_timestamp_print(int cpu)
{
#if MTK_CPUIDLE_TIME_PROFILING
	int i;
	char *p;

	request_uart_to_wakeup();

	p = mtk_cpuidle_timestamp_buf;

	p += sprintf(p, "CPU%d", cpu);
	for (i = 0; i < MTK_CPUIDLE_TIMESTAMP_COUNT; i++)
		p += sprintf(p, ",%llu", mtk_cpuidle_timestamp[cpu][i]);

	pr_err("%s\n", mtk_cpuidle_timestamp_buf);
#endif
}

static void mtk_cpuidle_ram_console_init(void)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	mtk_cpuidle_aee_virt_addr = aee_rr_rec_mtk_cpuidle_footprint_va();
	mtk_cpuidle_aee_phys_addr = aee_rr_rec_mtk_cpuidle_footprint_pa();

	WARN_ON(!mtk_cpuidle_aee_virt_addr || !mtk_cpuidle_aee_phys_addr);

	kernel_smc_msg(0, 2, (long) mtk_cpuidle_aee_phys_addr);
#endif
}

static const struct of_device_id kp_irq_match[] = {
	{ .compatible = "mediatek,mt6757-keypad" },
	{},
};
static const struct of_device_id consys_irq_match[] = {
	{ .compatible = "mediatek,mt6757-consys" },
	{},
};
static const struct of_device_id auxadc_irq_match[] = {
	{ .compatible = "mediatek,mt6757-auxadc" },
	{},
};
static const struct of_device_id mdcldma_irq_match[] = {
	{ .compatible = "mediatek,mdcldma" },
	{},
};
static const struct of_device_id ap2c2k_irq_match[] = {
	{ .compatible = "mediatek,ap2c2k_ccif" },
	{},
};

static u32 get_dts_node_irq_nr(const struct of_device_id *matches, int index)
{
	const struct of_device_id *matched_np;
	struct device_node *node;
	unsigned int irq_nr;

	node = of_find_matching_node_and_match(NULL, matches, &matched_np);
	if (!node)
		pr_err("error: cannot find node [%s]\n", matches->compatible);

	irq_nr = irq_of_parse_and_map(node, index);
	if (!irq_nr)
		pr_err("error: cannot property_read [%s]\n", matches->compatible);

	of_node_put(node);
	pr_debug("compatible = %s, irq_nr = %u\n", matched_np->compatible, irq_nr);

	return irq_nr;
}

static int mtk_cpuidle_dts_map(void)
{
	kp_irq_nr = get_dts_node_irq_nr(kp_irq_match, 0);
	conn_wdt_irq_nr = get_dts_node_irq_nr(consys_irq_match, 1);
	lowbattery_irq_nr = get_dts_node_irq_nr(auxadc_irq_match, 0);
	md1_wdt_irq_nr = get_dts_node_irq_nr(mdcldma_irq_match, 2);
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	c2k_wdt_irq_nr = get_dts_node_irq_nr(ap2c2k_irq_match, 1);
#endif
#endif

	return 0;
}

static void mtk_dbg_save_restore(int cpu, int save)
{
	unsigned int cpu_idle_sta;
	int nr_cpu_bit = (1 << CONFIG_NR_CPUS) - 1;

	cpu_idle_sta = (spm_read(CPU_IDLE_STA) >> CPU_IDLE_STA_OFFSET) | (1 << cpu);

	if ((cpu_idle_sta & nr_cpu_bit) == nr_cpu_bit) {
		if (save)
			mt_save_dbg_regs(dbg_data, cpu);
		else
			mt_restore_dbg_regs(dbg_data, cpu);
	} else {
		if (!save)
			mt_copy_dbg_regs(cpu, __builtin_ffs(~cpu_idle_sta) - 1);
	}
}

static void mtk_platform_save_context(int cpu, int idx)
{
	mtk_dbg_save_restore(cpu, 1);
}

static void mtk_platform_restore_context(int cpu, int idx)
{
	if (idx > MTK_MCDI_MODE)
		mtk_spm_wakeup_src_restore();

	mtk_dbg_save_restore(cpu, 0);
}

int mtk_enter_idle_state(int idx)
{
	int cpu, ret;

	if (!mtk_cpuidle_initialized)
		return -EOPNOTSUPP;

	cpu = smp_processor_id();

	mtk_cpuidle_footprint_log(cpu, 0);
	mtk_cpuidle_timestamp_log(cpu, 0);
	ret = cpu_pm_enter();
	if (!ret) {
		mtk_cpuidle_footprint_log(cpu, 1);

		if (cpu < 4)
			switch_armpll_ll_hwmode(1);
		else if (cpu < 8)
			switch_armpll_l_hwmode(1);

		mtk_cpuidle_footprint_log(cpu, 2);
		mtk_platform_save_context(cpu, idx);

		mtk_cpuidle_footprint_log(cpu, 3);
		mtk_cpuidle_timestamp_log(cpu, 1);
		/*
		 * Pass idle state index to cpu_suspend which in turn will
		 * call the CPU ops suspend protocol with idle index as a
		 * parameter.
		 */
		ret = arm_cpuidle_suspend(idx);
		mtk_cpuidle_timestamp_log(cpu, 14);

		mtk_cpuidle_footprint_log(cpu, 12);
		mtk_platform_restore_context(cpu, idx);

		mtk_cpuidle_footprint_log(cpu, 13);

		if (cpu < 4)
			switch_armpll_ll_hwmode(0);
		else if (cpu < 8)
			switch_armpll_l_hwmode(0);

		mtk_cpuidle_footprint_log(cpu, 14);

		cpu_pm_exit();

		mtk_cpuidle_footprint_clr(cpu);
		mtk_cpuidle_timestamp_log(cpu, 15);

		mtk_cpuidle_timestamp_print(cpu);
	}

	return ret ? -1 : idx;
}

int mtk_cpuidle_init(void)
{
	if (mtk_cpuidle_initialized == 1)
		return 0;

	mtk_cpuidle_dts_map();

	mtk_cpuidle_timestamp_init();

	mtk_cpuidle_ram_console_init();

	mtk_cpuidle_initialized = 1;

	return 0;
}
