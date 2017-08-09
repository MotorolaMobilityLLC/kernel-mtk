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

/**************************************
 * for non-CPU MTCMOS
 **************************************/
/* **** */
#if 1
static DEFINE_SPINLOCK(spm_noncpu_lock);

#if 0
void spm_mtcmos_noncpu_lock(unsigned long *flags)
{
	spin_lock_irqsave(&spm_noncpu_lock, *flags);
}

void spm_mtcmos_noncpu_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&spm_noncpu_lock, *flags);
}
#else

#define spm_mtcmos_noncpu_lock(flags) spin_lock_irqsave(&spm_noncpu_lock, flags)
#define spm_mtcmos_noncpu_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_lock, flags)

#endif
#endif				/* end **** */


#define MD2_PWR_STA_MASK    (0x1 << 22)
#define VEN_PWR_STA_MASK    (0x1 << 8)
#define VDE_PWR_STA_MASK    (0x1 << 7)
/* #define IFR_PWR_STA_MASK    (0x1 << 6) */
#define ISP_PWR_STA_MASK    (0x1 << 5)
#define MFG_PWR_STA_MASK    (0x1 << 4)
#define DIS_PWR_STA_MASK    (0x1 << 3)
/* #define DPY_PWR_STA_MASK    (0x1 << 2) */
#define CONN_PWR_STA_MASK   (0x1 << 1)
#define MD1_PWR_STA_MASK    (0x1 << 0)

#if 0
#define PWR_RST_B           (0x1 << 0)
#define PWR_ISO             (0x1 << 1)
#define PWR_ON              (0x1 << 2)
#define PWR_ON_2ND          (0x1 << 3)
#define PWR_CLK_DIS         (0x1 << 4)
#endif

#define SRAM_PDN            (0xf << 8)	/* VDEC, VENC, ISP, DISP */
#define MFG_SRAM_PDN        (0xf << 8)
#define MD_SRAM_PDN         (0x1 << 8)	/* MD1, C2K */
#define CONN_SRAM_PDN       (0x1 << 8)

#define VDE_SRAM_ACK        (0x1 << 12)
#define VEN_SRAM_ACK        (0xf << 12)
#define ISP_SRAM_ACK        (0x3 << 12)
#define DIS_SRAM_ACK        (0x1 << 12)
/* #define MFG_SRAM_ACK        (0x3f << 16) */
/* #define MFG_SRAM_ACK        (0x1 << 16) */
#define MFG_SRAM_ACK        (0x1 << 12)

#define MD1_PROT_MASK        ((0x1<<24) | (0x1<<25) | (0x1<<26) | (0x1<<27) | (0x1<<28))	/* bit 24,25,26,27,28 */
#define MD2_PROT_MASK        ((0x1<<29) | (0x1<<30) | (0x1<<31))	/* bit 29, 30, 31 */
/* bit16 is GCE, Dram dummy read use cqdma, and it is in GCE. */
#define DISP_PROT_MASK       ((0x1<<1))	/* bit 1, 6, 16; if bit6 set, MMSYS PDN, access reg will hang, */
#define MFG_PROT_MASK        ((0x1<<14))	/* bit 14 */
#define CONN_PROT_MASK       ((0x1<<2) | (0x1<<8))	/* bit 2, 8 */

#if defined(CONFIG_ARCH_MT6735M)
/* #define MD_PWRON_BY_CPU */
#elif defined(CONFIG_ARCH_MT6753)
#define MD_PWRON_BY_CPU
#else
/* #define MD_PWRON_BY_CPU */
#endif

int spm_mtcmos_ctrl_vdec(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_cpu_init();

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK) != VDE_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_VDE_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_VDE_PWR_CON, val);

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & VDE_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & VDE_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_RST_B);

		spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_venc(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK) != VEN_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_VEN_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_VEN_PWR_CON, val);

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & VEN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & VEN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_RST_B);

		spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;

#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_isp(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK) != ISP_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_ISP_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_ISP_PWR_CON, val);

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & ISP_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & ISP_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_RST_B);

		spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

#if 0
int spm_mtcmos_ctrl_disp(int state)
{
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | DISP_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) != DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);
#if 0
		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}
#endif
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_DIS_PWR_CON);
		/* val = (val & ~PWR_RST_B) | PWR_CLK_DIS; */
		val = val | PWR_CLK_DIS;
		spm_write(SPM_DIS_PWR_CON, val);

		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_2ND)); */

#if 0
		udelay(1);
		if (spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
			err = 1;
#else
		/* while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) */
		/* || (spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) { */
		/* } */
#endif
	} else {		/* STA_POWER_ON */
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON); */
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_2ND); */
#if 0
		udelay(1);
#else
		/* while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) */
		/* || !(spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) { */
		/* } */
#endif
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
		/* spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B); */

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

#if 0
		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}
#endif

#if 0
		udelay(1);
		if (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK))
			err = 1;

#endif
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~DISP_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
}

#else

int spm_mtcmos_ctrl_disp(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | DISP_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) != DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_DIS_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_DIS_PWR_CON, val);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
			count++;
			if (count > 1000 && count < 1010) {
				pr_debug("there is no fmm_clk, CLK_CFG_0 = 0x%x\n",
					 spm_read(CLK_CFG_0));
			}
			if (count > 2000) {
#if defined(CONFIG_MTK_LEGACY)
				clk_stat_check(SYS_DIS);
#endif
				BUG();
			}
		}

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~DISP_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}
#endif

int spm_mtcmos_ctrl_mfg(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MFG_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) != MFG_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | MFG_SRAM_PDN);

		while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK) != MFG_SRAM_ACK) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_MFG_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_MFG_PWR_CON, val);

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {		/* STA_POWER_ON */
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK) ||
		       !(spm_read(SPM_PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_RST_B);

		spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~MFG_SRAM_PDN);

		while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MFG_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_mdsys1(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD1_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) != MD1_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | MD_SRAM_PDN);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ISO);

		/* enable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val |= 0x40;
		spm_write(C2K_SPM_CTRL, val);

		val = spm_read(SPM_MD_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_MD_PWR_CON, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MD1_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

	} else {		/* STA_POWER_ON */

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ON);
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & MD1_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

#ifdef MD_PWRON_BY_CPU
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~PWR_ISO);

		/* disable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val &= ~(0x40);
		spm_write(C2K_SPM_CTRL, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_RST_B);
#else
		pr_debug("MD power on by SPM\n");
		spm_write(SPM_PCM_PASR_DPD_3, 0xbeef);
		spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0x1);
		while (spm_read(SPM_PCM_PASR_DPD_3)) {
			count++;
			udelay(1);
			if (count > 1000) {
				pr_err("MD power on: SPM no response\n");
				pr_err("PCM_IM_PTR : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR),
				       spm_read(SPM_PCM_IM_LEN));
				BUG();
			}
		}
		/* disable LTE LS ISO */
		val = spm_read(C2K_SPM_CTRL);
		val &= ~(0x40);
		spm_write(C2K_SPM_CTRL, val);

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) | PWR_RST_B);

#endif

		spm_write(SPM_MD_PWR_CON, spm_read(SPM_MD_PWR_CON) & ~MD_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD1_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_mdsys2(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD2_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) != MD2_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | MD_SRAM_PDN);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_C2K_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_C2K_PWR_CON, val);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & MD2_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

	} else {		/* STA_POWER_ON */

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ON);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & MD2_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) | PWR_RST_B);

		spm_write(SPM_C2K_PWR_CON, spm_read(SPM_C2K_PWR_CON) & ~MD_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD2_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_mtcmos_ctrl_connsys(int state)
{
/* **** */
#if 1
	int err = 0;
	/* volatile */unsigned int val;
	unsigned long flags;
	int count = 0;

	spm_mtcmos_noncpu_lock(flags);

	if (state == STA_POWER_DOWN) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | CONN_PROT_MASK);
		while ((spm_read(TOPAXI_PROT_STA1) & CONN_PROT_MASK) != CONN_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | CONN_SRAM_PDN);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_CONN_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_CONN_PWR_CON, val);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

		while ((spm_read(SPM_PWR_STATUS) & CONN_PWR_STA_MASK)
		       || (spm_read(SPM_PWR_STATUS_2ND) & CONN_PWR_STA_MASK)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ON);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_ON_2ND);

		while (!(spm_read(SPM_PWR_STATUS) & CONN_PWR_STA_MASK)
		       || !(spm_read(SPM_PWR_STATUS_2ND) & CONN_PWR_STA_MASK))
			;

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) | PWR_RST_B);

		spm_write(SPM_CONN_PWR_CON, spm_read(SPM_CONN_PWR_CON) & ~CONN_SRAM_PDN);

		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~CONN_PROT_MASK);
		while (spm_read(TOPAXI_PROT_STA1) & CONN_PROT_MASK) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return err;
#else
	return 1;
#endif
}

int spm_topaxi_prot(int bit, int en)
{
/* **** */
#if 1
	unsigned long flags;

	spm_mtcmos_noncpu_lock(flags);

	if (en == 1) {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | (1 << bit));
		while ((spm_read(TOPAXI_PROT_STA1) & (1 << bit)) != (1 << bit)) {
			/* read hw status */
			/* until hw ack ok */
		}
	} else {
		spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~(1 << bit));
		while (spm_read(TOPAXI_PROT_STA1) & (1 << bit)) {
			/* read hw status */
			/* until hw ack ok */
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	return 0;
#else
	return 1;
#endif
}

