/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <mt-plat/sync_write.h>
#include <mtk_swpm_common.h>


/****************************************************************************
 *  Macro Definitions
 ****************************************************************************/
#define swpm_pmu_read(cpu, offs)		\
	__raw_readl(pmu_base[cpu] + (offs))
#define swpm_pmu_write(cpu, offs, val)	\
	mt_reg_sync_writel(val, pmu_base[cpu] + (offs))

#define PMU_EVENT_L2D_CACHE             (0x16)
#define PMU_EVENT_LOAD_STALL            (0xE7)
#define PMU_EVENT_STORE_STALL           (0xE8)
#define PMU_CNTENSET_EVT_CNT            (0x7) /* 3 event counter enabled */
#define PMU_CNTENSET_C                  (0x1 << 31)
#define PMU_EVTYPER_INC_EL2             (0x1 << 27)
#define PMU_PMCR_E                      (0x1)
#define OFF_PMEVCNTR_0                  (0x000)
#define OFF_PMEVCNTR_1                  (0x008)
#define OFF_PMEVCNTR_2                  (0x010)
#define OFF_PMEVCNTR_3                  (0x018)
#define OFF_PMEVCNTR_4                  (0x020)
#define OFF_PMEVCNTR_5                  (0x028)
#define OFF_PMCCNTR_L                   (0x0F8)
#define OFF_PMCCNTR_H                   (0x0FC)
#define OFF_PMEVTYPER0                  (0x400)
#define OFF_PMEVTYPER1                  (0x404)
#define OFF_PMEVTYPER2                  (0x408)
#define OFF_PMEVTYPER3                  (0x40C)
#define OFF_PMEVTYPER4                  (0x410)
#define OFF_PMEVTYPER5                  (0x414)
#define OFF_PMCNTENSET                  (0xC00)
#define OFF_PMCNTENCLR                  (0xC20)
#define OFF_PMCFGR                      (0xE00)
#define OFF_PMCR                        (0xE04)
#define OFF_PMLAR                       (0xFB0)
#define OFF_PMLSR                       (0xFB4)

/****************************************************************************
 *  Type Definitions
 ****************************************************************************/
enum pmu_event {
	PMUEVT_L2_ACCESS,
	PMUEVT_LOAD_STALL,
	PMUEVT_STORE_STALL,
	PMUEVT_CYCLE_CNT,

	NR_PMU_EVENT
};

/****************************************************************************
 *  Local Variables
 ****************************************************************************/
static int swpm_hotplug_cb(struct notifier_block *nfb,
				unsigned long action, void *hcpu);
static struct notifier_block __refdata swpm_hotplug_notifier = {
	.notifier_call = swpm_hotplug_cb,
};
static void __iomem *pmu_base[NR_CPUS];

/****************************************************************************
 *  Global Variables
 ****************************************************************************/
const struct of_device_id swpm_of_ids[] = {
	{.compatible = "mediatek,dbgapb_pmu",},
	{}
};

/****************************************************************************
 *  Static Function
 ****************************************************************************/
static int swpm_hotplug_cb(struct notifier_block *nfb,
				unsigned long action, void *hcpu)
{
	/* unsigned int cpu = (unsigned long)hcpu; */

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_DOWN_PREPARE:
		/* swpm_stop_pmu(cpu); */
		break;
	case CPU_DOWN_FAILED:
	case CPU_ONLINE:
		/* swpm_start_pmu(cpu); */
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

/***************************************************************************
 *  API
 ***************************************************************************/
#if 0 /* TODO: need this? */
void swpm_start_pmu(int cpu)
{
	if (cpu >= num_possible_cpus())
		return;

	/* release PMU reg access lock */
	swpm_pmu_write(cpu, OFF_PMLAR, 0xC5ACCE55);
	/* wait for ready */
	while (swpm_pmu_read(cpu, OFF_PMLSR) != 1)
		;
	/* set event type */
	swpm_pmu_write(cpu, OFF_PMEVTYPER0,
		PMU_EVTYPER_INC_EL2 | PMU_EVENT_L2D_CACHE);
	swpm_pmu_write(cpu, OFF_PMEVTYPER1,
		PMU_EVTYPER_INC_EL2 | PMU_EVENT_LOAD_STALL);
	swpm_pmu_write(cpu, OFF_PMEVTYPER2,
		PMU_EVTYPER_INC_EL2 | PMU_EVENT_STORE_STALL);
	/* enable counters */
	swpm_pmu_write(cpu, OFF_PMCNTENSET,
		PMU_CNTENSET_C | PMU_CNTENSET_EVT_CNT);
	swpm_pmu_write(cpu, OFF_PMCR,
		swpm_pmu_read(cpu, OFF_PMCR) | PMU_PMCR_E);
}

void swpm_stop_pmu(int cpu)
{
	if (cpu >= num_possible_cpus())
		return;

	/* disable all counters */
	swpm_pmu_write(cpu, OFF_PMCR,
		swpm_pmu_read(cpu, OFF_PMCR) & ~PMU_PMCR_E);
}
#endif

int swpm_platform_init(struct platform_device *pdev)
{
	int ret = 0;
	int i;

	for (i = 0; i < num_possible_cpus(); i++) {
		pmu_base[i] = of_iomap(pdev->dev.of_node, i);

		if (pmu_base[i] == NULL)
			swpm_err("MAP CPU%d PMU addr failed\n", i);
	}

	/* TODO: register callback to MCDI */
	register_hotcpu_notifier(&swpm_hotplug_notifier);

	return ret;
}

