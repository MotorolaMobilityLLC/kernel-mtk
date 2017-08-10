/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>	/* udelay */
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/*#include <mach/mt_typedefs.h>*/
/*#include <mach/mt_spm_cpu.h>*/
/*#include <mach/mt_spm_reg.h>*/
#include "mt_spm_mtcmos.h"
#include <mach/mt_spm_mtcmos_internal.h>
#include <mach/mt_clkmgr.h>

#include <mach/mt_freqhopping.h>
#define BUILDERROR

#include "mt_spm_cpu.h"
/*#include "hotplug.h"*/
/**************************************
 * extern
 **************************************/

/**************************************
 * for CPU MTCMOS
 **************************************/
static DEFINE_SPINLOCK(spm_cpu_lock);
#ifdef CONFIG_OF
void __iomem *spm_cpu_base;
void __iomem *clk_apmixed_base;
#define ARMCA15PLL_CON0         (clk_apmixed_base + 0x200)
#define ARMCA15PLL_CON1         (clk_apmixed_base + 0x204)
#define ARMCA15PLL_PWR_CON0     (clk_apmixed_base + 0x20C)
#define AP_PLL_CON3             (clk_apmixed_base + 0x0C)
#define AP_PLL_CON4             (clk_apmixed_base + 0x10)

#endif				/* #ifdef CONFIG_OF */

int spm_mtcmos_cpu_init(void)
{
#ifdef CONFIG_OF
	struct device_node *node;

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
	iomap();		/*for clk function */
	return 0;
#else				/* #ifdef CONFIG_OF */
	return -EINVAL;
#endif				/* #ifdef CONFIG_OF */
}

void spm_mtcmos_cpu_lock(unsigned long *flags)
{
	spin_lock_irqsave(&spm_cpu_lock, *flags);
}

void spm_mtcmos_cpu_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&spm_cpu_lock, *flags);
}

/* Define MTCMOS Bus Protect Mask */
#define MD1_PROT_MASK                    ((0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 19) \
					  |(0x1 << 20) \
					  |(0x1 << 21))
#define CONN_PROT_MASK                   ((0x1 << 13) \
					  |(0x1 << 14))
#define DIS_PROT_MASK                    ((0x1 << 1))
#define MFG_PROT_MASK                    ((0x1 << 21))
#define MP0_CPUTOP_PROT_MASK             ((0x1 << 26) \
					  |(0x1 << 29))
#define MP1_CPUTOP_PROT_MASK             ((0x1 << 30))
#define C2K_PROT_MASK                    ((0x1 << 22) \
					  |(0x1 << 23) \
					  |(0x1 << 24))
#define MDSYS_INTF_INFRA_PROT_MASK       ((0x1 << 3) \
					  |(0x1 << 4))

 /* Define MTCMOS Power Status Mask */

#define MD1_PWR_STA_MASK                 (0x1 << 0)
#define CONN_PWR_STA_MASK                (0x1 << 1)
#define DPY_PWR_STA_MASK                 (0x1 << 2)
#define DIS_PWR_STA_MASK                 (0x1 << 3)
#define MFG_PWR_STA_MASK                 (0x1 << 4)
#define ISP_PWR_STA_MASK                 (0x1 << 5)
#define IFR_PWR_STA_MASK                 (0x1 << 6)
#define VDE_PWR_STA_MASK                 (0x1 << 7)
#define MP0_CPUTOP_PWR_STA_MASK          (0x1 << 8)
#define MP0_CPU0_PWR_STA_MASK            (0x1 << 9)
#define MP0_CPU1_PWR_STA_MASK            (0x1 << 10)
#define MP0_CPU2_PWR_STA_MASK            (0x1 << 11)
#define MP0_CPU3_PWR_STA_MASK            (0x1 << 12)
#define MCU_PWR_STA_MASK                 (0x1 << 14)
#define MP1_CPUTOP_PWR_STA_MASK          (0x1 << 15)
#define MP1_CPU0_PWR_STA_MASK            (0x1 << 16)
#define MP1_CPU1_PWR_STA_MASK            (0x1 << 17)
#define MP1_CPU2_PWR_STA_MASK            (0x1 << 18)
#define MP1_CPU3_PWR_STA_MASK            (0x1 << 19)
#define VEN_PWR_STA_MASK                 (0x1 << 21)
#define MFG_ASYNC_PWR_STA_MASK           (0x1 << 23)
#define AUDIO_PWR_STA_MASK               (0x1 << 24)
#define C2K_PWR_STA_MASK                 (0x1 << 28)
#define MDSYS_INTF_INFRA_PWR_STA_MASK    (0x1 << 29)
/* Define Non-CPU SRAM Mask */
#define MD1_SRAM_PDN                     (0x1 << 8)
#define MD1_SRAM_PDN_ACK                 (0x0 << 12)
#define DIS_SRAM_PDN                     (0x1 << 8)
#define DIS_SRAM_PDN_ACK                 (0x1 << 12)
#define MFG_SRAM_PDN                     (0x1 << 8)
#define MFG_SRAM_PDN_ACK                 (0x1 << 16)
#define ISP_SRAM_PDN                     (0x3 << 8)
#define ISP_SRAM_PDN_ACK                 (0x3 << 12)
#define IFR_SRAM_PDN                     (0xF << 8)
#define IFR_SRAM_PDN_ACK                 (0xF << 12)
#define VDE_SRAM_PDN                     (0x1 << 8)
#define VDE_SRAM_PDN_ACK                 (0x1 << 12)
#define VEN_SRAM_PDN                     (0xF << 8)
#define VEN_SRAM_PDN_ACK                 (0xF << 12)
#define AUDIO_SRAM_PDN                   (0xF << 8)
#define AUDIO_SRAM_PDN_ACK               (0xF << 12)
/* Define CPU SRAM Mask */
#define MP0_CPUTOP_SRAM_PDN              (0x1 << 0)
#define MP0_CPUTOP_SRAM_PDN_ACK          (0x1 << 8)
#define MP0_CPUTOP_SRAM_SLEEP_B          (0x1 << 0)
#define MP0_CPUTOP_SRAM_SLEEP_B_ACK      (0x1 << 8)
#define MP0_CPU0_SRAM_PDN                (0x1 << 0)
#define MP0_CPU0_SRAM_PDN_ACK            (0x1 << 8)
#define MP0_CPU1_SRAM_PDN                (0x1 << 0)
#define MP0_CPU1_SRAM_PDN_ACK            (0x1 << 8)
#define MP0_CPU2_SRAM_PDN                (0x1 << 0)
#define MP0_CPU2_SRAM_PDN_ACK            (0x1 << 8)
#define MP0_CPU3_SRAM_PDN                (0x1 << 0)
#define MP0_CPU3_SRAM_PDN_ACK            (0x1 << 8)
#define MP1_CPUTOP_SRAM_PDN              (0x1 << 0)
#define MP1_CPUTOP_SRAM_PDN_ACK          (0x1 << 8)
#define MP1_CPUTOP_SRAM_SLEEP_B          (0x1 << 0)
#define MP1_CPUTOP_SRAM_SLEEP_B_ACK      (0x1 << 8)
#define MP1_CPU0_SRAM_PDN                (0x1 << 0)
#define MP1_CPU0_SRAM_PDN_ACK            (0x1 << 8)
#define MP1_CPU1_SRAM_PDN                (0x1 << 0)
#define MP1_CPU1_SRAM_PDN_ACK            (0x1 << 8)
#define MP1_CPU2_SRAM_PDN                (0x1 << 0)
#define MP1_CPU2_SRAM_PDN_ACK            (0x1 << 8)
#define MP1_CPU3_SRAM_PDN                (0x1 << 0)
#define MP1_CPU3_SRAM_PDN_ACK            (0x1 << 8)

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

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn) {
			while ((spm_read(CPU_IDLE_STA) & MP0_CPU0_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		}
		/* TINFO="Start to turn off MP0_CPU0" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP0_CPU0_L1_PDN, spm_read(MP0_CPU0_L1_PDN) | MP0_CPU0_SRAM_PDN);
		/* TINFO="Wait until MP0_CPU0_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP0_CPU0_L1_PDN) & MP0_CPU0_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP0_CPU0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP0_CPU0_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP0_CPU0" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP0_CPU1_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP0_CPU1_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		}

		/* TINFO="Finish to turn off MP0_CPU0" */
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK) &&
		    !(spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK))
			spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP0_CPU0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP0_CPU0_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP0_CPU0_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP0_CPU0_L1_PDN, spm_read(MP0_CPU0_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP0_CPU0_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP0_CPU0_L1_PDN) & MP0_CPU0_SRAM_PDN_ACK) {
			/* no ops */
			;
		};
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP0_CPU0_PWR_CON, spm_read(MP0_CPU0_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP0_CPU0" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP0_CPU1_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP0_CPU1" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP0_CPU1_L1_PDN, spm_read(MP0_CPU1_L1_PDN) | MP0_CPU1_SRAM_PDN);
		/* TINFO="Wait until MP0_CPU1_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP0_CPU1_L1_PDN) & MP0_CPU1_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP0_CPU1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP0_CPU1_PWR_STA_MASK)) {
				/*no ops*/
				;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP0_CPU0" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP0_CPU0_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP0_CPU0_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		}

		/* TINFO="Finish to turn off MP0_CPU1" */
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK) &&
		    !(spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK))
			spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP0_CPU1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP0_CPU1_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP0_CPU1_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP0_CPU1_L1_PDN, spm_read(MP0_CPU1_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP0_CPU1_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP0_CPU1_L1_PDN) & MP0_CPU1_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP0_CPU1_PWR_CON, spm_read(MP0_CPU1_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP0_CPU1" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu2(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP0_CPU2_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP0_CPU2" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP0_CPU2_L1_PDN, spm_read(MP0_CPU2_L1_PDN) | MP0_CPU2_SRAM_PDN);
		/* TINFO="Wait until MP0_CPU2_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP0_CPU2_L1_PDN) & MP0_CPU2_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP0_CPU2_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP0_CPU2_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP0_CPU0" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		}

		/* TINFO="Finish to turn off MP0_CPU2" */
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK) &&
		    !(spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK))
			spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP0_CPU2" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP0_CPU2_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP0_CPU2_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP0_CPU2_L1_PDN, spm_read(MP0_CPU2_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP0_CPU2_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP0_CPU2_L1_PDN) & MP0_CPU2_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP0_CPU2_PWR_CON, spm_read(MP0_CPU2_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP0_CPU2" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu3(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP0_CPU3_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP0_CPU3" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP0_CPU3_L1_PDN, spm_read(MP0_CPU3_L1_PDN) | MP0_CPU3_SRAM_PDN);
		/* TINFO="Wait until MP0_CPU3_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP0_CPU3_L1_PDN) & MP0_CPU3_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP0_CPU3_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP0_CPU3_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP0_CPU0" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK | MP0_CPU2_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		}

		/* TINFO="Finish to turn off MP0_CPU3" */
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK) &&
		    !(spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK))
			spm_mtcmos_ctrl_cpusys0(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP0_CPU3" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP0_CPU3_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP0_CPU3_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP0_CPU3_L1_PDN, spm_read(MP0_CPU3_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP0_CPU3_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP0_CPU3_L1_PDN) & MP0_CPU3_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP0_CPU3_PWR_CON, spm_read(MP0_CPU3_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP0_CPU3" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu4(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP1_CPU0_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP1_CPU0" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP1_CPU0_L1_PDN, spm_read(MP1_CPU0_L1_PDN) | MP1_CPU0_SRAM_PDN);
		/* TINFO="Wait until MP1_CPU0_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP1_CPU0_L1_PDN) & MP1_CPU0_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP1_CPU0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP1_CPU0_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);

		/* TINFO="Finish to turn off MP1_CPU0" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP1_CPU1_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP1_CPU1_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		}
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & MP1_CPUTOP_PWR_STA_MASK) &&
		    !(spm_read(PWR_STATUS_2ND) & MP1_CPUTOP_PWR_STA_MASK))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP1_CPU0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP1_CPU0_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP1_CPU0_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP1_CPU0_L1_PDN, spm_read(MP1_CPU0_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPU0_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP1_CPU0_L1_PDN) & MP1_CPU0_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP1_CPU0_PWR_CON, spm_read(MP1_CPU0_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP1_CPU0" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu5(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP1_CPU1_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP1_CPU1" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP1_CPU1_L1_PDN, spm_read(MP1_CPU1_L1_PDN) | MP1_CPU1_SRAM_PDN);
		/* TINFO="Wait until MP1_CPU1_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP1_CPU1_L1_PDN) & MP1_CPU1_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP1_CPU1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP1_CPU1_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP1_CPU1" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP1_CPU0_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP1_CPU0_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		}
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP1_CPU1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP1_CPU1_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP1_CPU1_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP1_CPU1_L1_PDN, spm_read(MP1_CPU1_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPU1_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP1_CPU1_L1_PDN) & MP1_CPU1_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP1_CPU1_PWR_CON, spm_read(MP1_CPU1_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP1_CPU1" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu6(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP1_CPU2_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP1_CPU2" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP1_CPU2_L1_PDN, spm_read(MP1_CPU2_L1_PDN) | MP1_CPU2_SRAM_PDN);
		/* TINFO="Wait until MP1_CPU2_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP1_CPU2_L1_PDN) & MP1_CPU2_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP1_CPU2_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP1_CPU2_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP1_CPU2" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		}
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP1_CPU2" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP1_CPU2_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP1_CPU2_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP1_CPU2_L1_PDN, spm_read(MP1_CPU2_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPU2_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP1_CPU2_L1_PDN) & MP1_CPU2_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP1_CPU2_PWR_CON, spm_read(MP1_CPU2_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP1_CPU2" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpu7(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP1_CPU3_STANDBYWFI_LSB) == 0) {
				/* no ops */
				;
			}
		/* TINFO="Start to turn off MP1_CPU3" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_mtcmos_cpu_lock(&flags);
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP1_CPU3_L1_PDN, spm_read(MP1_CPU3_L1_PDN) | MP1_CPU3_SRAM_PDN);
		/* TINFO="Wait until MP1_CPU3_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP1_CPU3_L1_PDN) & MP1_CPU3_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP1_CPU3_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP1_CPU3_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP1_CPU3" */
		if (!
		    (spm_read(PWR_STATUS) &
		     (MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK))
			&& !(spm_read(PWR_STATUS_2ND) &
			(MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK | MP1_CPU2_PWR_STA_MASK))) {
#ifdef CONFIG_MTK_L2C_SHARE
			if (!IS_L2_BORROWED())
#endif				/* #ifdef CONFIG_MTK_L2C_SHARE */
				spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);
		}
	} else {		/* STA_POWER_ON */
		if (!(spm_read(PWR_STATUS) & CA15_CPUTOP) &&
		    !(spm_read(PWR_STATUS_2ND) & CA15_CPUTOP))
			spm_mtcmos_ctrl_cpusys1(state, chkWfiBeforePdn);

		spm_mtcmos_cpu_lock(&flags);
		/* TINFO="Start to turn on MP1_CPU3" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP1_CPU3_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP1_CPU3_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP1_CPU3_L1_PDN, spm_read(MP1_CPU3_L1_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPU3_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP1_CPU3_L1_PDN) & MP1_CPU3_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP1_CPU3_PWR_CON, spm_read(MP1_CPU3_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MP1_CPU3" */
		spm_mtcmos_cpu_unlock(&flags);
	}
	return 0;
}

int spm_mtcmos_ctrl_cpusys0(int state, int chkWfiBeforePdn)
{
	int err = 0;
	unsigned long flags;
	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MP1_CPUTOP" */
		/* TINFO="Set bus protect" */
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP0_CPUTOP_IDLE_LSB) == 0) {
				/* no ops */
				;
			}
		spm_mtcmos_cpu_lock(&flags);
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) | MP0_CPUTOP_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) &
			MP0_CPUTOP_PROT_MASK) != MP0_CPUTOP_PROT_MASK) {
			/* no ops */
			;
		}
#else
		spm_topaxi_protect(MP0_CPUTOP_PROT_MASK, 1);
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP0_CPUTOP_L2_PDN, spm_read(MP0_CPUTOP_L2_PDN) | MP0_CPUTOP_SRAM_PDN);
		/* TINFO="Wait until MP1_CPUTOP_SRAM_PDN_ACK = 1" */
		while (!(spm_read(MP0_CPUTOP_L2_PDN) & MP0_CPUTOP_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif

		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn off MP1_CPUTOP" */
	} else {
		spm_mtcmos_cpu_lock(&flags);
		/* STA_POWER_ON */
		/* TINFO="Start to turn on MP1_CPUTOP" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP0_CPUTOP_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP0_CPUTOP_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP0_CPUTOP_L2_PDN, spm_read(MP0_CPUTOP_L2_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPUTOP_SRAM_PDN_ACK = 0" */
		while (spm_read(MP0_CPUTOP_L2_PDN) & MP0_CPUTOP_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP0_CPUTOP_PWR_CON, spm_read(MP0_CPUTOP_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) & ~MP0_CPUTOP_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & MP0_CPUTOP_PROT_MASK) {
			/* no ops */
			;
		}
#else
		spm_topaxi_protect(MP0_CPUTOP_PROT_MASK, 0);
#endif
		spm_mtcmos_cpu_unlock(&flags);

		/* TINFO="Finish to turn on MP1_CPUTOP" */
	}
	return err;
}

int spm_mtcmos_ctrl_cpusys1(int state, int chkWfiBeforePdn)
{
	unsigned long flags;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MP1_CPUTOP" */
		/* TINFO="Set bus protect" */
		if (chkWfiBeforePdn)
			while ((spm_read(CPU_IDLE_STA) & MP1_CPUTOP_IDLE_LSB) == 0)
				;	/* no ops */

		spm_mtcmos_cpu_lock(&flags);
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) | MP1_CPUTOP_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) &
			MP1_CPUTOP_PROT_MASK) != MP1_CPUTOP_PROT_MASK) {
			/* no ops */
			;
		}
#else
		spm_topaxi_protect(MP1_CPUTOP_PROT_MASK, 1);
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | PWR_ISO);
		/* TINFO="Set SRAM_CKISO = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | SRAM_CKISO);
		/* TINFO="Set SRAM_ISOINT_B = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~SRAM_ISOINT_B);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MP1_CPUTOP_L2_PDN, spm_read(MP1_CPUTOP_L2_PDN) | MP1_CPUTOP_SRAM_PDN);
		/* TINFO="Wait until MP1_CPUTOP_SRAM_PDN_ACK = 1" */
#ifndef CFG_FPGA_PLATFORM
		while (!(spm_read(MP1_CPUTOP_L2_PDN) & MP1_CPUTOP_SRAM_PDN_ACK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MP1_CPUTOP_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MP1_CPUTOP_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
#if 1
		switch_armpll_l_hwmode(0);	/*Switch to SW mode */
		mt_pause_armpll(FH_ARMCA15_PLLID, 1);	/*Pause FQHP function */
		disable_armpll_l();	/*Turn off arm pll */
#endif
		spm_mtcmos_cpu_unlock(&flags);

		/* TINFO="Finish to turn off MP1_CPUTOP" */
	} else {		/* STA_POWER_ON */

		spm_mtcmos_cpu_lock(&flags);
#if 1
		enable_armpll_l();	/*Turn on arm pll */
		mt_pause_armpll(FH_ARMCA15_PLLID, 0);
		/*Non-pause FQHP function */
		switch_armpll_l_hwmode(1);	/*Switch to HW mode */
#endif

		/* TINFO="Start to turn on MP1_CPUTOP" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | PWR_ON_2ND);

#ifndef CFG_FPGA_PLATFORM
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MP1_CPUTOP_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MP1_CPUTOP_PWR_STA_MASK)) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */

		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MP1_CPUTOP_L2_PDN, spm_read(MP1_CPUTOP_L2_PDN) & ~(0x1 << 0));
		/* TINFO="Wait until MP1_CPUTOP_SRAM_PDN_ACK = 0" */
#ifndef CFG_FPGA_PLATFORM
		while (spm_read(MP1_CPUTOP_L2_PDN) & MP1_CPUTOP_SRAM_PDN_ACK) {
			/* no ops */
			;
		}
#endif				/* #ifndef CFG_FPGA_PLATFORM */
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_ISOINT_B = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | SRAM_ISOINT_B);
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="Set SRAM_CKISO = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~SRAM_CKISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MP1_CPUTOP_PWR_CON, spm_read(MP1_CPUTOP_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect" */
#if 1
		/* #ifndef CFG_FPGA_PLATFORM */

		spm_topaxi_protect(MP1_CPUTOP_PROT_MASK, 0);
#endif
		spm_mtcmos_cpu_unlock(&flags);
		/* TINFO="Finish to turn on MP1_CPUTOP" */
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
	} else {		/* STA_POWER_ON */

		spm_mtcmos_ctrl_cpu4(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu5(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu6(STA_POWER_ON, 1);
		spm_mtcmos_ctrl_cpu7(STA_POWER_ON, 1);
	}
}

bool spm_cpusys0_can_power_down(void)
{
	return !(spm_read(PWR_STATUS) &
		 (MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK |
		  MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK |
		  MP1_CPUTOP_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK |
		  MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK))
	    && !(spm_read(PWR_STATUS_2ND) &
		 (MP1_CPU0_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK |
		  MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK |
		  MP1_CPUTOP_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK |
		  MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK));
}

bool spm_cpusys1_can_power_down(void)
{
	return !(spm_read(PWR_STATUS) &
		 (MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK |
		  MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK |
		  MP0_CPUTOP_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK |
		  MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK))
	    && !(spm_read(PWR_STATUS_2ND) &
		 (MP0_CPU0_PWR_STA_MASK | MP0_CPU1_PWR_STA_MASK |
		  MP0_CPU2_PWR_STA_MASK | MP0_CPU3_PWR_STA_MASK |
		  MP0_CPUTOP_PWR_STA_MASK | MP1_CPU1_PWR_STA_MASK |
		  MP1_CPU2_PWR_STA_MASK | MP1_CPU3_PWR_STA_MASK));
}

#ifdef CONFIG_OF
void iomap(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-apmixedsys");
	if (!node)
		pr_debug("[CLK_APMIXED] find node failed\n");
	clk_apmixed_base = of_iomap(node, 0);
	if (!clk_apmixed_base)
		pr_debug("[CLK_APMIXED] base failed\n");
}

void enable_armpll_l(void)
{
	spm_write(ARMCA15PLL_PWR_CON0, (spm_read(ARMCA15PLL_PWR_CON0) | 0x01));
	udelay(2);
	spm_write(ARMCA15PLL_PWR_CON0, (spm_read(ARMCA15PLL_PWR_CON0) & 0xfffffffd));
	spm_write(ARMCA15PLL_CON1, (spm_read(ARMCA15PLL_CON1) | 0x80000000));
	spm_write(ARMCA15PLL_CON0, (spm_read(ARMCA15PLL_CON0) | 0x01));
	udelay(100);
}

void disable_armpll_l(void)
{
	spm_write(ARMCA15PLL_CON0, (spm_read(ARMCA15PLL_CON0) & 0xfffffffe));
	spm_write(ARMCA15PLL_PWR_CON0, (spm_read(ARMCA15PLL_PWR_CON0) | 0x00000002));
	spm_write(ARMCA15PLL_PWR_CON0, (spm_read(ARMCA15PLL_PWR_CON0) & 0xfffffffe));
}

void switch_armpll_l_hwmode(int enable)
{
	/* ARM CA15 */
	if (enable == 1) {
		spm_write(AP_PLL_CON3, (spm_read(AP_PLL_CON3) & 0xff87ffff));
		spm_write(AP_PLL_CON4, (spm_read(AP_PLL_CON4) & 0xffffcfff));
	} else {
		spm_write(AP_PLL_CON3, (spm_read(AP_PLL_CON3) | 0x00780000));
		spm_write(AP_PLL_CON4, (spm_read(AP_PLL_CON4) | 0x00003000));
	}
}

void switch_armpll_ll_hwmode(int enable)
{
	/* ARM CA7 */
	if (enable == 1) {
		spm_write(AP_PLL_CON3, (spm_read(AP_PLL_CON3) & 0xfffeeeef));
		spm_write(AP_PLL_CON4, (spm_read(AP_PLL_CON4) & 0xfffffefe));
	} else {
		spm_write(AP_PLL_CON3, (spm_read(AP_PLL_CON3) | 0x00011110));
		spm_write(AP_PLL_CON4, (spm_read(AP_PLL_CON4) | 0x00000101));
	}
}
#endif


static int mt_spm_mtcmos_init(void)
{
	return 0;
}
module_init(mt_spm_mtcmos_init);
