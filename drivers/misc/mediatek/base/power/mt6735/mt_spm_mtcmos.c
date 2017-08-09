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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <mach/mt_spm_mtcmos.h>
#include <mach/mt_spm_mtcmos_internal.h>
#include <mach/mt_clkmgr.h>

#include "mt_spm_cpu.h"
#include "hotplug.h"

/*
 * for CPU MTCMOS
 */

static DEFINE_SPINLOCK(spm_cpu_lock);
void __iomem *spm_cpu_base;

#define DBG_TEST (1)

int spm_mtcmos_cpu_init(void)
{
	struct device_node *node;
	static int init;

	if (init)
		return 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,SLEEP");
	if (!node) {
		pr_err("find SLEEP node failed\n");
		return -EINVAL;
	}
	spm_cpu_base = of_iomap(node, 0);
	if (!spm_cpu_base) {
		pr_err("base spm_cpu_base failed\n");
		return -EINVAL;
	}
	init = 1;

	return 0;
}

void spm_mtcmos_cpu_lock(unsigned long *flags)
{
	/* mutex_lock(&cpufreq_mutex); */
	spin_lock_irqsave(&spm_cpu_lock, *flags);
}

void spm_mtcmos_cpu_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&spm_cpu_lock, *flags);
	/* mutex_unlock(&cpufreq_mutex); */
}

typedef int (*spm_cpu_mtcmos_ctrl_func) (int state, int chkWfiBeforePdn);
static spm_cpu_mtcmos_ctrl_func spm_cpu_mtcmos_ctrl_funcs[] = {
	spm_mtcmos_ctrl_cpu0,
	spm_mtcmos_ctrl_cpu1,
	spm_mtcmos_ctrl_cpu2,
	spm_mtcmos_ctrl_cpu3,
	spm_mtcmos_ctrl_cpu4,
	spm_mtcmos_ctrl_cpu5,
	spm_mtcmos_ctrl_cpu6,
	spm_mtcmos_ctrl_cpu7
};

int spm_mtcmos_ctrl_cpu(unsigned int cpu, int state, int chkWfiBeforePdn)
{
	return (*spm_cpu_mtcmos_ctrl_funcs[cpu]) (state, chkWfiBeforePdn);
}

int spm_mtcmos_ctrl_cpu0(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA7_CPU0_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU0_L1_PDN,
			  spm_read(SPM_CA7_CPU0_L1_PDN) | L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU0_L1_PDN) & L1_PDN_ACK) !=
		       L1_PDN_ACK)
			;
#endif

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU0) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU0) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU0) != CA7_CPU0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU0) !=
			   CA7_CPU0))
			;
#endif

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU0_L1_PDN,
			  spm_read(SPM_CA7_CPU0_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU0_L1_PDN) & L1_PDN_ACK) != 0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU0_PWR_CON,
			  spm_read(SPM_CA7_CPU0_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA7_CPU1_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU1_L1_PDN,
			  spm_read(SPM_CA7_CPU1_L1_PDN) | L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK) !=
		       L1_PDN_ACK)
			;
#endif

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU1) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU1) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU1) != CA7_CPU1)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU1) !=
			   CA7_CPU1))
			;
#endif

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU1_L1_PDN,
			  spm_read(SPM_CA7_CPU1_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK) != 0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU1_PWR_CON,
			  spm_read(SPM_CA7_CPU1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu2(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA7_CPU2_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU2_L1_PDN,
			  spm_read(SPM_CA7_CPU2_L1_PDN) | L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU2_L1_PDN) & L1_PDN_ACK) !=
		       L1_PDN_ACK)
			;
#endif

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU2) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU2) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU2) != CA7_CPU2)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU2) !=
			   CA7_CPU2))
			;
#endif

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU2_L1_PDN,
			  spm_read(SPM_CA7_CPU2_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU2_L1_PDN) & L1_PDN_ACK) != 0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU2_PWR_CON,
			  spm_read(SPM_CA7_CPU2_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu3(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA7_CPU3_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU3_L1_PDN,
			  spm_read(SPM_CA7_CPU3_L1_PDN) | L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU3_L1_PDN) & L1_PDN_ACK) !=
		       L1_PDN_ACK)
			;
#endif

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU3) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU3) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPU3) != CA7_CPU3)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPU3) !=
			   CA7_CPU3))
			;
#endif

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPU3_L1_PDN,
			  spm_read(SPM_CA7_CPU3_L1_PDN) & ~L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPU3_L1_PDN) & L1_PDN_ACK) != 0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPU3_PWR_CON,
			  spm_read(SPM_CA7_CPU3_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu4(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn) {
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA15_CPU0_STANDBYWFI) == 0) {
				;
			}
		}

		/* mdelay(500); */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU0_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU0_CA15_L1_PDN_ACK) !=
		       CPU0_CA15_L1_PDN_ACK)
			;
#endif
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU0_CA15_L1_PDN_ISO);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU0) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU0) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);

		if (!
		    (spm_read(SPM_PWR_STATUS) &
		     (CA15_CPU1 | CA15_CPU2 | CA15_CPU3))
		    && !(spm_read(SPM_PWR_STATUS_2ND) &
			 (CA15_CPU1 | CA15_CPU2 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU0) != CA15_CPU0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU0) !=
			   CA15_CPU0))
			;
#endif

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) &
			  ~CPU0_CA15_L1_PDN_ISO);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) & ~CPU0_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU0_CA15_L1_PDN_ACK) !=
		       0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA15_CPU0_PWR_CON,
			  spm_read(SPM_CA15_CPU0_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu5(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA15_CPU1_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU1_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU1_CA15_L1_PDN_ACK) !=
		       CPU1_CA15_L1_PDN_ACK)
			;
#endif
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU1_CA15_L1_PDN_ISO);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU1) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU1) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);

		if (!
		    (spm_read(SPM_PWR_STATUS) &
		     (CA15_CPU0 | CA15_CPU2 | CA15_CPU3))
		    && !(spm_read(SPM_PWR_STATUS_2ND) &
			 (CA15_CPU0 | CA15_CPU2 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU1) != CA15_CPU1)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU1) !=
			   CA15_CPU1))
			;
#endif

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) &
			  ~CPU1_CA15_L1_PDN_ISO);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) & ~CPU1_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU1_CA15_L1_PDN_ACK) !=
		       0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA15_CPU1_PWR_CON,
			  spm_read(SPM_CA15_CPU1_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu6(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA15_CPU2_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU2_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU2_CA15_L1_PDN_ACK) !=
		       CPU2_CA15_L1_PDN_ACK)
			;
#endif
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU2_CA15_L1_PDN_ISO);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU2) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU2) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);

		if (!
		    (spm_read(SPM_PWR_STATUS) &
		     (CA15_CPU0 | CA15_CPU1 | CA15_CPU3))
		    && !(spm_read(SPM_PWR_STATUS_2ND) &
			 (CA15_CPU0 | CA15_CPU1 | CA15_CPU3)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU2) != CA15_CPU2)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU2) !=
			   CA15_CPU2))
			;
#endif

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) &
			  ~CPU2_CA15_L1_PDN_ISO);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) & ~CPU2_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU2_CA15_L1_PDN_ACK) !=
		       0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA15_CPU2_PWR_CON,
			  spm_read(SPM_CA15_CPU2_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpu7(int state, int chkWfiBeforePdn)
{
	unsigned long flags;
#ifndef CONFIG_MTK_FPGA
	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA15_CPU3_STANDBYWFI) == 0)
				;

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU3_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU3_CA15_L1_PDN_ACK) !=
		       CPU3_CA15_L1_PDN_ACK)
			;
#endif
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) | CPU3_CA15_L1_PDN_ISO);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU3) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU3) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);

		if (!
		    (spm_read(SPM_PWR_STATUS) &
		     (CA15_CPU0 | CA15_CPU1 | CA15_CPU2))
		    && !(spm_read(SPM_PWR_STATUS_2ND) &
			 (CA15_CPU0 | CA15_CPU1 | CA15_CPU2)))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
	} else {		/* STA_POWER_ON */

		if (!(spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPU3) != CA15_CPU3)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPU3) !=
			   CA15_CPU3))
			;
#endif

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) &
			  ~CPU3_CA15_L1_PDN_ISO);
		spm_write(SPM_CA15_L1_PWR_CON,
			  spm_read(SPM_CA15_L1_PWR_CON) & ~CPU3_CA15_L1_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L1_PWR_CON) & CPU3_CA15_L1_PDN_ACK) !=
		       0)
			;
#endif
		udelay(1);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | SRAM_ISOINT_B);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA15_CPU3_PWR_CON,
			  spm_read(SPM_CA15_CPU3_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}
#endif
	return 0;
}

int spm_mtcmos_ctrl_cpusys0(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn) {
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA7_CPUTOP_STANDBYWFI) == 0) {
				;
			}
		}

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | SRAM_CKISO);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);
		spm_write(SPM_CA7_CPUTOP_L2_PDN,
			  spm_read(SPM_CA7_CPUTOP_L2_PDN) | L2_SRAM_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPUTOP_L2_PDN) & L2_SRAM_PDN_ACK) !=
		       L2_SRAM_PDN_ACK)
			;
#endif
		ndelay(1500);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPUTOP) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPUTOP) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else { /* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA7_CPUTOP) != CA7_CPUTOP)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA7_CPUTOP) !=
			   CA7_CPUTOP))
			;
#endif

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA7_CPUTOP_L2_PDN,
			  spm_read(SPM_CA7_CPUTOP_L2_PDN) & ~L2_SRAM_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA7_CPUTOP_L2_PDN) & L2_SRAM_PDN_ACK) !=
		       0)
			;
#endif
		ndelay(900);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | SRAM_ISOINT_B);
		ndelay(100);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA7_CPUTOP_PWR_CON,
			  spm_read(SPM_CA7_CPUTOP_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

int spm_mtcmos_ctrl_cpusys1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* enable register control */
	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(SPM_SLEEP_TIMER_STA) &
				CA15_CPUTOP_STANDBYWFI) == 0)
				;
		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ISO);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | SRAM_CKISO);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);

		spm_write(SPM_CA15_L2_PWR_CON,
			  spm_read(SPM_CA15_L2_PWR_CON) | CA15_L2_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L2_PWR_CON) & CA15_L2_PDN_ACK) !=
		       CA15_L2_PDN_ACK)
			;
#endif
		spm_write(SPM_CA15_L2_PWR_CON,
			  spm_read(SPM_CA15_L2_PWR_CON) | CA15_L2_PDN_ISO);
		ndelay(1500);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_RST_B);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_CLK_DIS);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ON);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) != 0)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP) != 0))
			;
#endif

		spm_mtcmos_cpu_unlock(&flags);
	} else { /* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ON);
		udelay(1);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_ON_2ND);
#ifndef CONFIG_MTK_FPGA
		while (((spm_read(SPM_PWR_STATUS) & CA15_CPUTOP) != CA15_CPUTOP)
		       || ((spm_read(SPM_PWR_STATUS_2ND) & CA15_CPUTOP) !=
			   CA15_CPUTOP))
			;
#endif
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_ISO);

		spm_write(SPM_CA15_L2_PWR_CON,
			  spm_read(SPM_CA15_L2_PWR_CON) & ~CA15_L2_PDN_ISO);
		spm_write(SPM_CA15_L2_PWR_CON,
			  spm_read(SPM_CA15_L2_PWR_CON) & ~CA15_L2_PDN);
#ifndef CONFIG_MTK_FPGA
		while ((spm_read(SPM_CA15_L2_PWR_CON) & CA15_L2_PDN_ACK) != 0)
			;
#endif
		ndelay(900);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | SRAM_ISOINT_B);
		ndelay(100);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~SRAM_CKISO);

		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CA15_CPUTOP_PWR_CON,
			  spm_read(SPM_CA15_CPUTOP_PWR_CON) | PWR_RST_B);

		spm_mtcmos_cpu_unlock(&flags);
	}

	return 0;
}

void spm_mtcmos_ctrl_cpusys1_init_1st_bring_up(int state)
{

	if (state == STA_POWER_DOWN) {
		spm_mtcmos_ctrl_cpu7(STA_POWER_DOWN, 0);
		spm_mtcmos_ctrl_cpu6(STA_POWER_DOWN, 0);
		spm_mtcmos_ctrl_cpu5(STA_POWER_DOWN, 0);
		spm_mtcmos_ctrl_cpu4(STA_POWER_DOWN, 0);
	} else { /* STA_POWER_ON */

		spm_mtcmos_ctrl_cpu4(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu5(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu6(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu7(STA_POWER_ON, 1);
	}
}

bool spm_cpusys0_can_power_down(void)
{
	return !(spm_read(SPM_PWR_STATUS) & (CA7_CPU1 | CA7_CPU2 | CA7_CPU3)) &&
	    !(spm_read(SPM_PWR_STATUS_2ND) & (CA7_CPU1 | CA7_CPU2 | CA7_CPU3));
}

bool spm_cpusys1_can_power_down(void)
{
	return !(spm_read(SPM_PWR_STATUS) &
		 (CA7_CPU0 | CA7_CPU1 | CA7_CPU2 | CA7_CPU3 | CA7_CPUTOP |
		  CA15_CPU1 | CA15_CPU2 | CA15_CPU3))
	    && !(spm_read(SPM_PWR_STATUS_2ND) &
		 (CA7_CPU0 | CA7_CPU1 | CA7_CPU2 | CA7_CPU3 | CA7_CPUTOP |
		  CA15_CPU1 | CA15_CPU2 | CA15_CPU3));
}

/*
 * for non-CPU MTCMOS
 */
int spm_mtcmos_ctrl_vdec(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_venc(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_isp(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_disp(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_mfg(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_mdsys1(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_mdsys2(int state)
{
	return 1;
}

int spm_mtcmos_ctrl_connsys(int state)
{
	return 1;
}

int spm_topaxi_prot(int bit, int en)
{
	return 1;
}
