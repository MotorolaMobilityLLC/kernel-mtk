/*
* Copyright (c) 2014 MediaTek Inc.
* Author: James Liao <jamesjj.liao@mediatek.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/of.h>
#include <linux/of_address.h>

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clkdev.h>

#include "clk-mtk-v1.h"
#include "clk-mt6799-pg.h"

#include <dt-bindings/clock/mt6799-clk.h>

/*#define TOPAXI_PROTECT_LOCK*/
#ifdef CONFIG_FPGA_EARLY_PORTING
#define IGNORE_MTCMOS_CHECK	1
#endif
#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP  0
#define CONTROL_LIMIT 1
#endif

#define	CHECK_PWR_ST	1

#ifndef GENMASK
#define GENMASK(h, l)	(((U32_C(1) << ((h) - (l) + 1)) - 1) << (l))
#endif

#ifdef CONFIG_ARM64
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif

#define clk_readl(addr)		readl(addr)
#define clk_writel(val, addr)	\
	do { writel(val, addr); wmb(); } while (0)	/* sync_write */
#define clk_setl(mask, addr)	clk_writel(clk_readl(addr) | (mask), addr)
#define clk_clrl(mask, addr)	clk_writel(clk_readl(addr) & ~(mask), addr)

#define mt_reg_sync_writel(v, a) \
	do { \
		__raw_writel((v), IOMEM(a)); \
		mb(); } \
while (0)
#define spm_read(addr)			__raw_readl(IOMEM(addr))
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

/*
 * MTCMOS
 */

#define STA_POWER_DOWN	0
#define STA_POWER_ON	1
#define SUBSYS_PWR_DOWN		0
#define SUBSYS_PWR_ON		1

	struct subsys;

struct subsys_ops {
	int (*enable)(struct subsys *sys);
	int (*disable)(struct subsys *sys);
	int (*get_state)(struct subsys *sys);
};

struct subsys {
	const char *name;
	uint32_t sta_mask;
	void __iomem *ctl_addr;
	uint32_t sram_pdn_bits;
	uint32_t sram_pdn_ack_bits;
	uint32_t bus_prot_mask;
	struct subsys_ops *ops;
};

/*static struct subsys_ops general_sys_ops;*/
static struct subsys_ops MD1_sys_ops;
static struct subsys_ops MM0_sys_ops;
static struct subsys_ops MM1_sys_ops;
static struct subsys_ops MFG0_sys_ops;
static struct subsys_ops MFG1_sys_ops;
static struct subsys_ops MFG2_sys_ops;
static struct subsys_ops MFG3_sys_ops;
static struct subsys_ops ISP_sys_ops;
static struct subsys_ops VDE_sys_ops;
static struct subsys_ops VEN_sys_ops;
static struct subsys_ops IPU_SHUTDOWN_sys_ops;
static struct subsys_ops IPU_SLEEP_sys_ops;
static struct subsys_ops AUDIO_sys_ops;
static struct subsys_ops CAM_sys_ops;
static struct subsys_ops C2K_sys_ops;
static struct subsys_ops MJC_sys_ops;


static void __iomem *infracfg_base;/*infracfg_ao*/
static void __iomem *spm_base;
static void __iomem *smi_common_base;

#define INFRACFG_REG(offset)		(infracfg_base + offset)
#define SPM_REG(offset)			(spm_base + offset)
#define SMI_COMMON_REG(offset)		(smi_common_base + offset)


/* Define MTCMOS power control */
#define PWR_RST_B                        (0x1 << 0)
#define PWR_ISO                          (0x1 << 1)
#define PWR_ON                           (0x1 << 2)
#define PWR_ON_2ND                       (0x1 << 3)
#define PWR_CLK_DIS                      (0x1 << 4)
#define SRAM_CKISO                       (0x1 << 5)
#define SRAM_ISOINT_B                    (0x1 << 6)
#define SLPB_CLAMP                       (0x1 << 7)

/* Define MTCMOS Bus Protect Mask */
#define MFG1_PROT_STEP1_0_MASK           ((0x1 << 6))
#define MFG1_PROT_STEP1_0_ACK_MASK       ((0x1 << 6))
#define MFG1_PROT_STEP1_1_MASK           ((0x1 << 24) \
					  |(0x1 << 25))
#define MFG1_PROT_STEP1_1_ACK_MASK       ((0x1 << 24) \
					  |(0x1 << 25))
#define MFG1_PROT_STEP2_0_MASK           ((0x1 << 4) \
					  |(0x1 << 5))
#define MFG1_PROT_STEP2_0_ACK_MASK       ((0x1 << 4) \
					  |(0x1 << 5))
#define C2K_PROT_STEP1_0_MASK            ((0x1 << 22) \
					  |(0x1 << 23) \
					  |(0x1 << 24))
#define C2K_PROT_STEP1_0_ACK_MASK        ((0x1 << 22) \
					  |(0x1 << 23) \
					  |(0x1 << 24))
#define MD1_PROT_STEP1_0_MASK            ((0x1 << 21) \
					  |(0x1 << 28))
#define MD1_PROT_STEP1_0_ACK_MASK        ((0x1 << 21))
#define MD1_PROT_STEP2_0_MASK            ((0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 19))
#define MD1_PROT_STEP2_0_ACK_MASK        ((0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 19))
#define INFRA_PROT_STEP1_0_MASK          ((0x1 << 15) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 23))
#define INFRA_PROT_STEP1_0_ACK_MASK      ((0x1 << 15) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 23))
#define INFRA_PROT_STEP1_1_MASK          ((0x1 << 8) \
					  |(0x1 << 9) \
					  |(0x1 << 21))
#define INFRA_PROT_STEP1_1_ACK_MASK      ((0x1 << 8) \
					  |(0x1 << 9) \
					  |(0x1 << 21))
#define INFRA_PROT_STEP1_2_MASK          ((0x1 << 0) \
					  |(0x1 << 1) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 24) \
					  |(0x1 << 25))
#define INFRA_PROT_STEP1_2_ACK_MASK      ((0x1 << 0) \
					  |(0x1 << 1) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 24) \
					  |(0x1 << 25))
#define INFRA_PROT_STEP1_3_MASK          ((0x1 << 20) \
					  |(0x1 << 21) \
					  |(0x1 << 22))
#define INFRA_PROT_STEP1_3_ACK_MASK      ((0x1 << 20) \
					  |(0x1 << 21) \
					  |(0x1 << 22))
#define INFRA_PROT_STEP2_0_MASK          ((0x1 << 4) \
					  |(0x1 << 5) \
					  |(0x1 << 7) \
					  |(0x1 << 8) \
					  |(0x1 << 9) \
					  |(0x1 << 12) \
					  |(0x1 << 13) \
					  |(0x1 << 24))
#define INFRA_PROT_STEP2_0_ACK_MASK      ((0x1 << 4) \
					  |(0x1 << 5) \
					  |(0x1 << 7) \
					  |(0x1 << 8) \
					  |(0x1 << 9) \
					  |(0x1 << 12) \
					  |(0x1 << 13) \
					  |(0x1 << 24))
#define INFRA_PROT_STEP2_1_MASK          ((0x1 << 10) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 19))
#define INFRA_PROT_STEP2_1_ACK_MASK      ((0x1 << 10) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 19))
#define INFRA_PROT_STEP2_2_MASK          ((0x1 << 23) \
					  |(0x1 << 24))
#define INFRA_PROT_STEP2_2_ACK_MASK      ((0x1 << 23) \
					  |(0x1 << 24))
#define PERI_PROT_STEP1_0_MASK           ((0x1 << 23))
#define PERI_PROT_STEP1_0_ACK_MASK       ((0x1 << 23))
#define PERI_PROT_STEP1_1_MASK           ((0x1 << 8) \
					  |(0x1 << 9))
#define PERI_PROT_STEP1_1_ACK_MASK       ((0x1 << 8) \
					  |(0x1 << 9))
#define PERI_PROT_STEP2_0_MASK           ((0x1 << 12) \
					  |(0x1 << 13))
#define PERI_PROT_STEP2_0_ACK_MASK       ((0x1 << 12) \
					  |(0x1 << 13))
#define PERI_PROT_STEP2_1_MASK           ((0x1 << 10))
#define PERI_PROT_STEP2_1_ACK_MASK       ((0x1 << 10))
#define AUD_PROT_STEP1_0_MASK            ((0x1 << 14))
#define AUD_PROT_STEP1_0_ACK_MASK        ((0x1 << 14))
#define MJC_PROT_STEP1_0_MASK            ((0x1 << 10))
#define MJC_PROT_STEP1_0_ACK_MASK        ((0x1 << 10))
#define MJC_PROT_STEP2_0_MASK            ((0x1 << 5))
#define MJC_PROT_STEP2_0_ACK_MASK        ((0x1 << 5))
#define MM0_PROT_STEP1_0_MASK            ((0x1 << 18))
#define MM0_PROT_STEP1_0_ACK_MASK        ((0x1 << 18))
#define MM0_PROT_STEP1_1_MASK            ((0x1 << 0) \
					  |(0x1 << 1) \
					  |(0x1 << 4) \
					  |(0x1 << 6) \
					  |(0x1 << 8) \
					  |(0x1 << 10) \
					  |(0x1 << 12) \
					  |(0x1 << 14))
#define MM0_PROT_STEP1_1_ACK_MASK        ((0x1 << 0) \
					  |(0x1 << 1) \
					  |(0x1 << 4) \
					  |(0x1 << 6) \
					  |(0x1 << 8) \
					  |(0x1 << 10) \
					  |(0x1 << 12) \
					  |(0x1 << 14))
#define MM0_PROT_STEP2_0_MASK            ((0x1 << 8) \
					  |(0x1 << 9))
#define MM0_PROT_STEP2_0_ACK_MASK        ((0x1 << 8) \
					  |(0x1 << 9))
#define MM0_PROT_STEP2_1_MASK            ((0x1 << 2) \
					  |(0x1 << 3) \
					  |(0x1 << 4) \
					  |(0x1 << 5) \
					  |(0x1 << 6) \
					  |(0x1 << 7))
#define MM0_PROT_STEP2_1_ACK_MASK        ((0x1 << 2) \
					  |(0x1 << 3) \
					  |(0x1 << 4) \
					  |(0x1 << 5) \
					  |(0x1 << 6) \
					  |(0x1 << 7))
#define CAM_PROT_STEP1_0_MASK            ((0x1 << 8) \
					  |(0x1 << 18))
#define CAM_PROT_STEP1_0_ACK_MASK        ((0x1 << 8) \
					  |(0x1 << 18))
#define CAM_PROT_STEP2_0_MASK            ((0x1 << 19))
#define CAM_PROT_STEP2_0_ACK_MASK        ((0x1 << 19))
#define CAM_PROT_STEP2_1_MASK            ((0x1 << 4))
#define CAM_PROT_STEP2_1_ACK_MASK        ((0x1 << 4))
#define IPU_PROT_STEP1_0_MASK            ((0x1 << 14) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 20))
#define IPU_PROT_STEP1_0_ACK_MASK        ((0x1 << 14) \
					  |(0x1 << 16) \
					  |(0x1 << 17) \
					  |(0x1 << 18) \
					  |(0x1 << 20))
#define IPU_PROT_STEP2_0_MASK            ((0x1 << 7) \
					  |(0x1 << 24))
#define IPU_PROT_STEP2_0_ACK_MASK        ((0x1 << 7) \
					  |(0x1 << 24))
#define IPU_PROT_STEP2_1_MASK            ((0x1 << 19) \
					  |(0x1 << 21))
#define IPU_PROT_STEP2_1_ACK_MASK        ((0x1 << 19) \
					  |(0x1 << 21))
#define IPU_PROT_STEP2_2_MASK            ((0x1 << 7))
#define IPU_PROT_STEP2_2_ACK_MASK        ((0x1 << 7))
#define ISP_PROT_STEP1_0_MASK            ((0x1 << 6) \
					  |(0x1 << 20))
#define ISP_PROT_STEP1_0_ACK_MASK        ((0x1 << 6) \
					  |(0x1 << 20))
#define ISP_PROT_STEP2_0_MASK            ((0x1 << 21))
#define ISP_PROT_STEP2_0_ACK_MASK        ((0x1 << 21))
#define ISP_PROT_STEP2_1_MASK            ((0x1 << 3))
#define ISP_PROT_STEP2_1_ACK_MASK        ((0x1 << 3))
#define VEN_PROT_STEP1_0_MASK            ((0x1 << 12))
#define VEN_PROT_STEP1_0_ACK_MASK        ((0x1 << 12))
#define VEN_PROT_STEP2_0_MASK            ((0x1 << 6))
#define VEN_PROT_STEP2_0_ACK_MASK        ((0x1 << 6))
#define VDE_PROT_STEP1_0_MASK            ((0x1 << 4))
#define VDE_PROT_STEP1_0_ACK_MASK        ((0x1 << 4))
#define VDE_PROT_STEP2_0_MASK            ((0x1 << 2))
#define VDE_PROT_STEP2_0_ACK_MASK        ((0x1 << 2))

#define DPY_CH0_PROT_BIT_MASK            ((0x1 << 0))
#define DPY_CH0_PROT_BIT_ACK_MASK        ((0x1 << 0))
#define DPY_CH1_PROT_BIT_MASK            ((0x1 << 1))
#define DPY_CH1_PROT_BIT_ACK_MASK        ((0x1 << 1))
#define DPY_CH2_PROT_BIT_MASK            ((0x1 << 2))
#define DPY_CH2_PROT_BIT_ACK_MASK        ((0x1 << 2))
#define DPY_CH3_PROT_BIT_MASK            ((0x1 << 3))
#define DPY_CH3_PROT_BIT_ACK_MASK        ((0x1 << 3))
#define MFG0_PWR_STA_MASK                (0x1 << 1)
#define MFG1_PWR_STA_MASK                (0x1 << 2)
#define MFG2_PWR_STA_MASK                (0x1 << 3)
#define MFG3_PWR_STA_MASK                (0x1 << 4)
#define C2K_PWR_STA_MASK                 (0x1 << 5)
#define MD1_PWR_STA_MASK                 (0x1 << 6)
#define DPY_CH0_PWR_STA_MASK             (0x1 << 7)
#define DPY_CH1_PWR_STA_MASK             (0x1 << 8)
#define DPY_CH2_PWR_STA_MASK             (0x1 << 9)
#define DPY_CH3_PWR_STA_MASK             (0x1 << 10)
#define INFRA_PWR_STA_MASK               (0x1 << 11)
#define AUD_PWR_STA_MASK                 (0x1 << 13)
#define MJC_PWR_STA_MASK                 (0x1 << 14)
#define MM0_PWR_STA_MASK                 (0x1 << 15)
#define MM1_PWR_STA_MASK                 (0x1 << 16)
#define CAM_PWR_STA_MASK                 (0x1 << 17)
#define IPU_PWR_STA_MASK                 (0x1 << 18)
#define ISP_PWR_STA_MASK                 (0x1 << 19)
#define VEN_PWR_STA_MASK                 (0x1 << 20)
#define VDE_PWR_STA_MASK                 (0x1 << 21)
#define DUMMY0_PWR_STA_MASK              (0x1 << 22)
#define DUMMY1_PWR_STA_MASK              (0x1 << 23)
#define DUMMY2_PWR_STA_MASK              (0x1 << 24)
#define DUMMY3_PWR_STA_MASK              (0x1 << 25)

/* Define Non-CPU SRAM Mask */
#define MFG1_SRAM_PDN                    (0xF << 8)
#define MFG1_SRAM_PDN_ACK                (0xF << 24)
#define MFG1_SRAM_PDN_ACK_BIT0           (0x1 << 24)
#define MFG1_SRAM_PDN_ACK_BIT1           (0x1 << 25)
#define MFG1_SRAM_PDN_ACK_BIT2           (0x1 << 26)
#define MFG1_SRAM_PDN_ACK_BIT3           (0x1 << 27)
#define MFG2_SRAM_PDN                    (0xF << 8)
#define MFG2_SRAM_PDN_ACK                (0xF << 24)
#define MFG2_SRAM_PDN_ACK_BIT0           (0x1 << 24)
#define MFG2_SRAM_PDN_ACK_BIT1           (0x1 << 25)
#define MFG2_SRAM_PDN_ACK_BIT2           (0x1 << 26)
#define MFG2_SRAM_PDN_ACK_BIT3           (0x1 << 27)
#define MFG3_SRAM_PDN                    (0xF << 8)
#define MFG3_SRAM_PDN_ACK                (0xF << 24)
#define MFG3_SRAM_PDN_ACK_BIT0           (0x1 << 24)
#define MFG3_SRAM_PDN_ACK_BIT1           (0x1 << 25)
#define MFG3_SRAM_PDN_ACK_BIT2           (0x1 << 26)
#define MFG3_SRAM_PDN_ACK_BIT3           (0x1 << 27)
#define MD1_SRAM_PDN                     (0x7 << 8)
#define MD1_SRAM_PDN_ACK                 (0x0 << 24)
#define MD1_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define MD1_SRAM_PDN_ACK_BIT1            (0x1 << 25)
#define MD1_SRAM_PDN_ACK_BIT2            (0x1 << 26)
#define DPY_CH0_SRAM_PDN                 (0xF << 8)
#define DPY_CH0_SRAM_PDN_ACK             (0x1 << 12)
#define DPY_CH0_SRAM_PDN_ACK_BIT0        (0x1 << 12)
#define DPY_CH0_SRAM_PDN_ACK_BIT1        (0x1 << 13)
#define DPY_CH0_SRAM_PDN_ACK_BIT2        (0x1 << 14)
#define DPY_CH0_SRAM_PDN_ACK_BIT3        (0x1 << 15)
#define DPY_CH1_SRAM_PDN                 (0xF << 8)
#define DPY_CH1_SRAM_PDN_ACK             (0x2 << 12)
#define DPY_CH1_SRAM_PDN_ACK_BIT0        (0x1 << 12)
#define DPY_CH1_SRAM_PDN_ACK_BIT1        (0x1 << 13)
#define DPY_CH1_SRAM_PDN_ACK_BIT2        (0x1 << 14)
#define DPY_CH1_SRAM_PDN_ACK_BIT3        (0x1 << 15)
#define DPY_CH2_SRAM_PDN                 (0xF << 8)
#define DPY_CH2_SRAM_PDN_ACK             (0x4 << 12)
#define DPY_CH2_SRAM_PDN_ACK_BIT0        (0x1 << 12)
#define DPY_CH2_SRAM_PDN_ACK_BIT1        (0x1 << 13)
#define DPY_CH2_SRAM_PDN_ACK_BIT2        (0x1 << 14)
#define DPY_CH2_SRAM_PDN_ACK_BIT3        (0x1 << 15)
#define DPY_CH3_SRAM_PDN                 (0xF << 8)
#define DPY_CH3_SRAM_PDN_ACK             (0x8 << 12)
#define DPY_CH3_SRAM_PDN_ACK_BIT0        (0x1 << 12)
#define DPY_CH3_SRAM_PDN_ACK_BIT1        (0x1 << 13)
#define DPY_CH3_SRAM_PDN_ACK_BIT2        (0x1 << 14)
#define DPY_CH3_SRAM_PDN_ACK_BIT3        (0x1 << 15)
#define INFRA_SRAM_PDN                   (0xF << 8)
#define INFRA_SRAM_PDN_ACK               (0xF << 24)
#define INFRA_SRAM_PDN_ACK_BIT0          (0x1 << 24)
#define INFRA_SRAM_PDN_ACK_BIT1          (0x1 << 25)
#define INFRA_SRAM_PDN_ACK_BIT2          (0x1 << 26)
#define INFRA_SRAM_PDN_ACK_BIT3          (0x1 << 27)
#define AUD_SRAM_PDN                     (0xF << 8)
#define AUD_SRAM_PDN_ACK                 (0xF << 24)
#define AUD_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define AUD_SRAM_PDN_ACK_BIT1            (0x1 << 25)
#define AUD_SRAM_PDN_ACK_BIT2            (0x1 << 26)
#define AUD_SRAM_PDN_ACK_BIT3            (0x1 << 27)
#define MJC_SRAM_PDN                     (0x1 << 8)
#define MJC_SRAM_PDN_ACK                 (0x1 << 24)
#define MJC_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define MM0_SRAM_PDN                     (0x1 << 8)
#define MM0_SRAM_PDN_ACK                 (0x1 << 24)
#define MM0_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define MM1_SRAM_PDN                     (0x1 << 8)
#define MM1_SRAM_PDN_ACK                 (0x1 << 24)
#define MM1_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define CAM_SRAM_PDN                     (0x3 << 8)
#define CAM_SRAM_PDN_ACK                 (0x3 << 24)
#define CAM_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define CAM_SRAM_PDN_ACK_BIT1            (0x1 << 25)
#define IPU_SRAM_PDN                     (0x1 << 8)
#define IPU_SRAM_PDN_ACK                 (0x1 << 24)
#define IPU_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define ISP_SRAM_PDN                     (0x3 << 8)
#define ISP_SRAM_PDN_ACK                 (0x3 << 24)
#define ISP_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define ISP_SRAM_PDN_ACK_BIT1            (0x1 << 25)
#define VEN_SRAM_PDN                     (0xF << 8)
#define VEN_SRAM_PDN_ACK                 (0xF << 24)
#define VEN_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define VEN_SRAM_PDN_ACK_BIT1            (0x1 << 25)
#define VEN_SRAM_PDN_ACK_BIT2            (0x1 << 26)
#define VEN_SRAM_PDN_ACK_BIT3            (0x1 << 27)
#define VDE_SRAM_PDN                     (0x1 << 8)
#define VDE_SRAM_PDN_ACK                 (0x1 << 24)
#define VDE_SRAM_PDN_ACK_BIT0            (0x1 << 24)
#define DUMMY0_SRAM_PDN                  (0xF << 8)
#define DUMMY0_SRAM_PDN_ACK              (0xF << 24)
#define DUMMY0_SRAM_PDN_ACK_BIT0         (0x1 << 24)
#define DUMMY0_SRAM_PDN_ACK_BIT1         (0x1 << 25)
#define DUMMY0_SRAM_PDN_ACK_BIT2         (0x1 << 26)
#define DUMMY0_SRAM_PDN_ACK_BIT3         (0x1 << 27)
#define DUMMY1_SRAM_PDN                  (0xF << 8)
#define DUMMY1_SRAM_PDN_ACK              (0xF << 24)
#define DUMMY1_SRAM_PDN_ACK_BIT0         (0x1 << 24)
#define DUMMY1_SRAM_PDN_ACK_BIT1         (0x1 << 25)
#define DUMMY1_SRAM_PDN_ACK_BIT2         (0x1 << 26)
#define DUMMY1_SRAM_PDN_ACK_BIT3         (0x1 << 27)
#define DUMMY2_SRAM_PDN                  (0xF << 8)
#define DUMMY2_SRAM_PDN_ACK              (0xF << 24)
#define DUMMY2_SRAM_PDN_ACK_BIT0         (0x1 << 24)
#define DUMMY2_SRAM_PDN_ACK_BIT1         (0x1 << 25)
#define DUMMY2_SRAM_PDN_ACK_BIT2         (0x1 << 26)
#define DUMMY2_SRAM_PDN_ACK_BIT3         (0x1 << 27)
#define DUMMY3_SRAM_PDN                  (0xF << 8)
#define DUMMY3_SRAM_PDN_ACK              (0xF << 24)
#define DUMMY3_SRAM_PDN_ACK_BIT0         (0x1 << 24)
#define DUMMY3_SRAM_PDN_ACK_BIT1         (0x1 << 25)
#define DUMMY3_SRAM_PDN_ACK_BIT2         (0x1 << 26)
#define DUMMY3_SRAM_PDN_ACK_BIT3         (0x1 << 27)

/**************************************
 * for non-CPU MTCMOS
 **************************************/
#ifdef TOPAXI_PROTECT_LOCK
static DEFINE_SPINLOCK(spm_noncpu_lock);

#define spm_mtcmos_noncpu_lock(flags)   spin_lock_irqsave(&spm_noncpu_lock, flags)

#define spm_mtcmos_noncpu_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_lock, flags)
#endif


#define POWERON_CONFIG_EN	SPM_REG(0x0000)
#define PWR_STATUS	SPM_REG(0x0190)
#define PWR_STATUS_2ND	SPM_REG(0x0194)
#define MFG0_PWR_CON	SPM_REG(0x0300)
#define MFG1_PWR_CON	SPM_REG(0x0304)
#define MFG2_PWR_CON	SPM_REG(0x0308)
#define MFG3_PWR_CON	SPM_REG(0x030C)
#define C2K_PWR_CON	SPM_REG(0x0310)
#define MD1_PWR_CON	SPM_REG(0x0314)
#define DPY_CH0_PWR_CON	SPM_REG(0x0318)
#define DPY_CH1_PWR_CON	SPM_REG(0x031C)
#define DPY_CH2_PWR_CON	SPM_REG(0x0320)
#define DPY_CH3_PWR_CON	SPM_REG(0x0324)
#define INFRA_PWR_CON	SPM_REG(0x0328)
#define AUD_PWR_CON	SPM_REG(0x0330)
#define MJC_PWR_CON	SPM_REG(0x0334)
#define MM0_PWR_CON	SPM_REG(0x0338)
#define MM1_PWR_CON	SPM_REG(0x033C)
#define CAM_PWR_CON	SPM_REG(0x0340)
#define IPU_PWR_CON	SPM_REG(0x0344)
#define ISP_PWR_CON	SPM_REG(0x0348)
#define VEN_PWR_CON	SPM_REG(0x034C)
#define VDE_PWR_CON	SPM_REG(0x0350)
#define EXT_BUCK_ISO	SPM_REG(0x0394)
#define IPU_SRAM_CON	SPM_REG(0x03A0)

#define SMI0_CLAMP	SMI_COMMON_REG(0x03C0)
#define SMI0_CLAMP_SET	SMI_COMMON_REG(0x03C4)
#define SMI0_CLAMP_CLR	SMI_COMMON_REG(0x03C8)


#define INFRA_TOPAXI_SI0_CTL	INFRACFG_REG(0x0200)
#define INFRA_TOPAXI_PROTECTSTA0	INFRACFG_REG(0x0224)
#define INFRASYS_QAXI_CTRL	INFRACFG_REG(0x0F28)

#define INFRA_TOPAXI_PROTECTEN_SET	INFRACFG_REG(0x02A0)
#define INFRA_TOPAXI_PROTECTSTA1	INFRACFG_REG(0x0228)
#define INFRA_TOPAXI_PROTECTEN_CLR	INFRACFG_REG(0x02A4)

#define INFRA_TOPAXI_PROTECTEN_1_SET	INFRACFG_REG(0x02A8)
#define INFRA_TOPAXI_PROTECTSTA1_1	INFRACFG_REG(0x0258)
#define INFRA_TOPAXI_PROTECTEN_1_CLR	INFRACFG_REG(0x02AC)

#define INFRA_TOPAXI_PROTECTEN_2_CON	INFRACFG_REG(0x02C4)
#define INFRA_TOPAXI_PROTECTEN_2_SET	INFRACFG_REG(0x02C8)
#define INFRA_TOPAXI_PROTECTSTA1_2	INFRACFG_REG(0x02D4)
#define INFRA_TOPAXI_PROTECTEN_2_CLR	INFRACFG_REG(0x02CC)






#define  SPM_PROJECT_CODE    0xB16

/* Define MTCMOS power control */
#define PWR_RST_B                        (0x1 << 0)
#define PWR_ISO                          (0x1 << 1)
#define PWR_ON                           (0x1 << 2)
#define PWR_ON_2ND                       (0x1 << 3)
#define PWR_CLK_DIS                      (0x1 << 4)
#define SRAM_CKISO                       (0x1 << 5)
#define SRAM_ISOINT_B                    (0x1 << 6)
#define SLPB_CLAMP                       (0x1 << 7)







static struct subsys syss[] =	/* NR_SYSS *//* FIXME: set correct value */
{
	[SYS_MD1] = {
		     .name = __stringify(SYS_MD1),
		     .sta_mask = MD1_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = MD1_SRAM_PDN,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MD1_sys_ops,
		     },
	[SYS_MM0] = {
		     .name = __stringify(SYS_MM0),
		     .sta_mask = MM0_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MM0_sys_ops,
		     },
	[SYS_MM1] = {
		     .name = __stringify(SYS_MM1),
		     .sta_mask = MM1_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MM1_sys_ops,
		     },
	[SYS_MFG0] = {
		     .name = __stringify(SYS_MFG0),
		     .sta_mask = MFG0_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MFG0_sys_ops,
		     },
	[SYS_MFG1] = {
		     .name = __stringify(SYS_MFG1),
		     .sta_mask = MFG1_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MFG1_sys_ops,
		     },
	[SYS_MFG2] = {
		     .name = __stringify(SYS_MFG2),
		     .sta_mask = MFG2_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MFG2_sys_ops,
		     },
	[SYS_MFG3] = {
		     .name = __stringify(SYS_MFG3),
		     .sta_mask = MFG3_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MFG3_sys_ops,
		     },
	[SYS_ISP] = {
		     .name = __stringify(SYS_ISP),
		     .sta_mask = ISP_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &ISP_sys_ops,
		     },
	[SYS_VDE] = {
		     .name = __stringify(SYS_VDE),
		     .sta_mask = VDE_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &VDE_sys_ops,
		     },
	[SYS_VEN] = {
		     .name = __stringify(SYS_VEN),
		     .sta_mask = VEN_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &VEN_sys_ops,
		     },
	[SYS_IPU_SHUTDOWN] = {
			   .name = __stringify(SYS_IPU_SHUTDOWN),
			   .sta_mask = IPU_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &IPU_SHUTDOWN_sys_ops,
			   },
	[SYS_IPU_SLEEP] = {
			   .name = __stringify(SYS_IPU_SLEEP),
			   .sta_mask = IPU_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &IPU_SLEEP_sys_ops,
			   },
	[SYS_AUDIO] = {
		       .name = __stringify(SYS_AUDIO),
		       .sta_mask = AUD_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		       .sram_pdn_bits = 0,
		       .sram_pdn_ack_bits = 0,
		       .bus_prot_mask = 0,
		       .ops = &AUDIO_sys_ops,
		       },
	[SYS_CAM] = {
		       .name = __stringify(SYS_CAM),
		       .sta_mask = CAM_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		       .sram_pdn_bits = 0,
		       .sram_pdn_ack_bits = 0,
		       .bus_prot_mask = 0,
		       .ops = &CAM_sys_ops,
		       },
	[SYS_C2K] = {
		     .name = __stringify(SYS_C2K),
		     .sta_mask = C2K_PWR_STA_MASK,
		     /*.ctl_addr = NULL, SPM_C2K_PWR_CON, */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &C2K_sys_ops,
		     },
	[SYS_MJC] = {
			   .name = __stringify(SYS_MJC),
			   .sta_mask = MJC_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &MJC_sys_ops,
			   },
};

LIST_HEAD(pgcb_list);

struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb)
{
	INIT_LIST_HEAD(&pgcb->list);

	list_add(&pgcb->list, &pgcb_list);

	return pgcb;
}

static struct subsys *id_to_sys(unsigned int id)
{
	return id < NR_SYSS ? &syss[id] : NULL;
}

/* sync from mtcmos_ctrl.c  */
#ifdef CONFIG_MTK_RAM_CONSOLE
#if 0 /*add after early porting*/
static void aee_clk_data_rest(void)
{
	aee_rr_rec_clk(0, 0);
	aee_rr_rec_clk(1, 0);
	aee_rr_rec_clk(2, 0);
	aee_rr_rec_clk(3, 0);
}
#endif
#endif

/* auto-gen begin*/
int spm_mtcmos_ctrl_mfg0(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG0" */
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG0_PWR_STA_MASK)) {
				/*  */
		}
#endif
		/* TINFO="Finish to turn off MFG0" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG0_PWR_STA_MASK) != MFG0_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG0_PWR_STA_MASK) != MFG0_PWR_STA_MASK)) {
				/*  */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG0_PWR_CON, spm_read(MFG0_PWR_CON) | PWR_RST_B);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on MFG0" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG1" */
		/* TINFO="Set way_en = 0" */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) & (~(0x1 << 7)));
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA0) & (0x1 << 6)) != (0x1 << 6)) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MFG1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MFG1_PROT_STEP1_0_ACK_MASK)
			!= MFG1_PROT_STEP1_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, MFG1_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & MFG1_PROT_STEP1_1_ACK_MASK)
			!= MFG1_PROT_STEP1_1_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MFG1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MFG1_PROT_STEP2_0_ACK_MASK)
			!= MFG1_PROT_STEP2_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | MFG1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG1_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG1_PWR_CON) & MFG1_SRAM_PDN_ACK) != MFG1_SRAM_PDN_ACK) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG1_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off MFG1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG1_PWR_STA_MASK) != MFG1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG1_PWR_STA_MASK) != MFG1_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG1_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG1_PWR_CON) & MFG1_SRAM_PDN_ACK_BIT0) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG1_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(MFG1_PWR_CON) & MFG1_SRAM_PDN_ACK_BIT1) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG1_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(MFG1_PWR_CON) & MFG1_SRAM_PDN_ACK_BIT2) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG1_PWR_CON, spm_read(MFG1_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG1_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(MFG1_PWR_CON) & MFG1_SRAM_PDN_ACK_BIT3) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MFG1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MFG1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, MFG1_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Set way_en = 1" */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) | (0x1 << 7));
		/* TINFO="Finish to turn on MFG1" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg2(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG2" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | MFG2_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG2_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG2_PWR_CON) & MFG2_SRAM_PDN_ACK) != MFG2_SRAM_PDN_ACK) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG2_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG2_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off MFG2" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG2" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG2_PWR_STA_MASK) != MFG2_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG2_PWR_STA_MASK) != MFG2_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG2_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG2_PWR_CON) & MFG2_SRAM_PDN_ACK_BIT0) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG2_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(MFG2_PWR_CON) & MFG2_SRAM_PDN_ACK_BIT1) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG2_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(MFG2_PWR_CON) & MFG2_SRAM_PDN_ACK_BIT2) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG2_PWR_CON, spm_read(MFG2_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG2_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(MFG2_PWR_CON) & MFG2_SRAM_PDN_ACK_BIT3) {
			/* Need f_fslow_mfg_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn on MFG2" */
		switch_mfg_clk(0);
		switch_mfg_clk(1);

	}
	return err;
}

int spm_mtcmos_ctrl_mfg3(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG3" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | MFG3_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG3_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG3_PWR_CON) & MFG3_SRAM_PDN_ACK) != MFG3_SRAM_PDN_ACK) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG3_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG3_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off MFG3" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG3" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG3_PWR_STA_MASK) != MFG3_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG3_PWR_STA_MASK) != MFG3_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG3_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG3_PWR_CON) & MFG3_SRAM_PDN_ACK_BIT0) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG3_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(MFG3_PWR_CON) & MFG3_SRAM_PDN_ACK_BIT1) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG3_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(MFG3_PWR_CON) & MFG3_SRAM_PDN_ACK_BIT2) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(MFG3_PWR_CON, spm_read(MFG3_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG3_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(MFG3_PWR_CON) & MFG3_SRAM_PDN_ACK_BIT3) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn on MFG3" */
	}
	return err;
}

int spm_mtcmos_ctrl_c2k(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off C2K" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, C2K_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & C2K_PROT_STEP1_0_ACK_MASK)
			!= C2K_PROT_STEP1_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & C2K_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & C2K_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off C2K" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on C2K" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & C2K_PWR_STA_MASK) != C2K_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & C2K_PWR_STA_MASK) != C2K_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_RST_B);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, C2K_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on C2K" */
	}
	return err;
}

int spm_mtcmos_ctrl_md1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MD1" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MD1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_STEP1_0_ACK_MASK)
			!= MD1_PROT_STEP1_0_ACK_MASK) {
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MD1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_STEP2_0_ACK_MASK)
			!= MD1_PROT_STEP2_0_ACK_MASK) {
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | MD1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="EXT_BUCK_ISO[0]=1"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) | (0x1 << 0));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON_2ND);
		/* TINFO="Finish to turn off MD1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MD1" */
		/* TINFO="EXT_BUCK_ISO[0]=0"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) & ~(0x1 << 0));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MD1_PWR_STA_MASK) != MD1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK) != MD1_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MD1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_STEP1_0_ACK_MASK)
			!= MD1_PROT_STEP1_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MD1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_STEP2_0_ACK_MASK)
			!= MD1_PROT_STEP2_0_ACK_MASK) {
		}
#endif
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~(0x1 << 8));
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~(0x1 << 9));
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~(0x1 << 10));
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, MD1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, MD1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on MD1" */
	}
	return err;
}

int spm_mtcmos_ctrl_dpy_ch0(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DPY_CH0" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, DPY_CH0_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & DPY_CH0_PROT_BIT_ACK_MASK)
			!= DPY_CH0_PROT_BIT_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | DPY_CH0_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH0_SRAM_PDN_ACK = 1" */
		while ((spm_read(DPY_CH0_PWR_CON) & DPY_CH0_SRAM_PDN_ACK) != DPY_CH0_SRAM_PDN_ACK) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DPY_CH0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & DPY_CH0_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off DPY_CH0" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on DPY_CH0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & DPY_CH0_PWR_STA_MASK) != DPY_CH0_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & DPY_CH0_PWR_STA_MASK) != DPY_CH0_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH0_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(DPY_CH0_PWR_CON) & DPY_CH0_SRAM_PDN_ACK_BIT0) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~(0x1 << 9));
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~(0x1 << 10));
		spm_write(DPY_CH0_PWR_CON, spm_read(DPY_CH0_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, DPY_CH0_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on DPY_CH0" */
	}
	return err;
}

int spm_mtcmos_ctrl_dpy_ch1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DPY_CH1" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, DPY_CH1_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & DPY_CH1_PROT_BIT_ACK_MASK)
			!= DPY_CH1_PROT_BIT_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | DPY_CH1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH1_SRAM_PDN_ACK = 1" */
		while ((spm_read(DPY_CH1_PWR_CON) & DPY_CH1_SRAM_PDN_ACK) != DPY_CH1_SRAM_PDN_ACK) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DPY_CH1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & DPY_CH1_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off DPY_CH1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on DPY_CH1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & DPY_CH1_PWR_STA_MASK) != DPY_CH1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & DPY_CH1_PWR_STA_MASK) != DPY_CH1_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~(0x1 << 8));
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH1_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(DPY_CH1_PWR_CON) & DPY_CH1_SRAM_PDN_ACK_BIT1) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~(0x1 << 10));
		spm_write(DPY_CH1_PWR_CON, spm_read(DPY_CH1_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, DPY_CH1_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on DPY_CH1" */
	}
	return err;
}

int spm_mtcmos_ctrl_dpy_ch2(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DPY_CH2" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, DPY_CH2_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & DPY_CH2_PROT_BIT_ACK_MASK)
			!= DPY_CH2_PROT_BIT_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | DPY_CH2_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH2_SRAM_PDN_ACK = 1" */
		while ((spm_read(DPY_CH2_PWR_CON) & DPY_CH2_SRAM_PDN_ACK) != DPY_CH2_SRAM_PDN_ACK) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DPY_CH2_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & DPY_CH2_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off DPY_CH2" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on DPY_CH2" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & DPY_CH2_PWR_STA_MASK) != DPY_CH2_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & DPY_CH2_PWR_STA_MASK) != DPY_CH2_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~(0x1 << 8));
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~(0x1 << 9));
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH2_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(DPY_CH2_PWR_CON) & DPY_CH2_SRAM_PDN_ACK_BIT2) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		spm_write(DPY_CH2_PWR_CON, spm_read(DPY_CH2_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, DPY_CH2_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on DPY_CH2" */
	}
	return err;
}

int spm_mtcmos_ctrl_dpy_ch3(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DPY_CH3" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, DPY_CH3_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & DPY_CH3_PROT_BIT_ACK_MASK)
			!= DPY_CH3_PROT_BIT_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | DPY_CH3_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH3_SRAM_PDN_ACK = 1" */
		while ((spm_read(DPY_CH3_PWR_CON) & DPY_CH3_SRAM_PDN_ACK) != DPY_CH3_SRAM_PDN_ACK) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DPY_CH3_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & DPY_CH3_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off DPY_CH3" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on DPY_CH3" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & DPY_CH3_PWR_STA_MASK) != DPY_CH3_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & DPY_CH3_PWR_STA_MASK) != DPY_CH3_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~(0x1 << 8));
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~(0x1 << 9));
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~(0x1 << 10));
		spm_write(DPY_CH3_PWR_CON, spm_read(DPY_CH3_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DPY_CH3_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(DPY_CH3_PWR_CON) & DPY_CH3_SRAM_PDN_ACK_BIT3) {
			/* no logic between pdn and pdn_ack */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, DPY_CH3_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on DPY_CH3" */
	}
	return err;
}

int spm_mtcmos_ctrl_aud(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off AUD" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, AUD_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & AUD_PROT_STEP1_0_ACK_MASK)
			!= AUD_PROT_STEP1_0_ACK_MASK) {
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | AUD_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUD_SRAM_PDN_ACK = 1" */
		while ((spm_read(AUD_PWR_CON) & AUD_SRAM_PDN_ACK) != AUD_SRAM_PDN_ACK) {
			/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & AUD_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & AUD_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off AUD" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on AUD" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & AUD_PWR_STA_MASK) != AUD_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & AUD_PWR_STA_MASK) != AUD_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUD_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(AUD_PWR_CON) & AUD_SRAM_PDN_ACK_BIT0) {
			/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUD_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(AUD_PWR_CON) & AUD_SRAM_PDN_ACK_BIT1) {
			/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUD_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(AUD_PWR_CON) & AUD_SRAM_PDN_ACK_BIT2) {
			/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(AUD_PWR_CON, spm_read(AUD_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUD_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(AUD_PWR_CON) & AUD_SRAM_PDN_ACK_BIT3) {
			/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, AUD_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on AUD" */
	}
	return err;
}

int spm_mtcmos_ctrl_mjc(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		mjc_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		/* TINFO="Start to turn off MJC" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, MJC_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & MJC_PROT_STEP1_0_ACK_MASK)
			!= MJC_PROT_STEP1_0_ACK_MASK) {
			/*pr_err("[CCF-2] %s\r\n", __func__);*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_SET, MJC_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & MJC_PROT_STEP2_0_ACK_MASK)
			!= MJC_PROT_STEP2_0_ACK_MASK) {
			/*pr_err("[CCF-3] %s\r\n", __func__);*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | MJC_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MJC_SRAM_PDN_ACK = 1" */
		while ((spm_read(MJC_PWR_CON) & MJC_SRAM_PDN_ACK) != MJC_SRAM_PDN_ACK) {
			/*pr_err("[CCF-4] %s\r\n", __func__);*/
			/*check_mjc_clk_sts();*/
			/* Need f_fmjc_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MJC_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MJC_PWR_STA_MASK)) {
			/*pr_err("[CCF-5] %s\r\n", __func__);*/
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off MJC" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MJC" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MJC_PWR_STA_MASK) != MJC_PWR_STA_MASK)
		|| ((spm_read(PWR_STATUS_2ND) & MJC_PWR_STA_MASK)
			!= MJC_PWR_STA_MASK)) {
			/*pr_err("[CCF-6] %s\n", __func__);*/
				/* No logic between pwr_on and pwr_ack.*/
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MJC_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MJC_PWR_CON) & MJC_SRAM_PDN_ACK_BIT0) {
			/*pr_err("[CCF-7] %s\n", __func__);*/
			/*check_mjc_clk_sts();*/
			/* Need f_fmjc_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_CLR, MJC_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, MJC_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		mjc_mtcmos_patch(state);
		/* TINFO="Finish to turn on MJC" */
	}
	return err;
}

int spm_mtcmos_ctrl_mm0(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		mm0_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		#if 0
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ISO);
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ISO);
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ISO);
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ISO);
		spm_write(MJC_PWR_CON, spm_read(MJC_PWR_CON) | PWR_ISO);
		#endif
		/* TINFO="Start to turn off MM0" */
		/* TINFO="Set bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, MM0_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & MM0_PROT_STEP1_1_ACK_MASK)
			!= MM0_PROT_STEP1_1_ACK_MASK) {
			/*pr_err("[CCF-2] %s\r\n", __func__);*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MM0_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MM0_PROT_STEP2_0_ACK_MASK)
			!= MM0_PROT_STEP2_0_ACK_MASK) {
			/*pr_err("[CCF-3] %s\r\n", __func__);*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_SET, MM0_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & MM0_PROT_STEP2_1_ACK_MASK)
			!= MM0_PROT_STEP2_1_ACK_MASK) {
			/*pr_err("[CCF-4] %s\r\n", __func__);*/
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | MM0_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MM0_SRAM_PDN_ACK = 1" */
		while ((spm_read(MM0_PWR_CON) & MM0_SRAM_PDN_ACK) != MM0_SRAM_PDN_ACK) {
			/*pr_err("[CCF-5] %s\r\n", __func__);*/
			/* Need f_fmm_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MM0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MM0_PWR_STA_MASK)) {
			/*pr_err("[CCF-6] %s\r\n", __func__);*/
			/* No logic between pwr_on and pwr_ack. */
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MM0" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MM0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MM0_PWR_STA_MASK) != MM0_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MM0_PWR_STA_MASK) != MM0_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MM0_PWR_CON, spm_read(MM0_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MM0_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MM0_PWR_CON) & MM0_SRAM_PDN_ACK_BIT0) {
			/* Need f_fmm_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MM0_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_CLR, MM0_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, MM0_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		mm0_mtcmos_patch(state);
		/* TINFO="Finish to turn on MM0" */
	}
	return err;
}

int spm_mtcmos_ctrl_mm1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MM1" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | MM1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MM1_SRAM_PDN_ACK = 1" */
		while ((spm_read(MM1_PWR_CON) & MM1_SRAM_PDN_ACK) != MM1_SRAM_PDN_ACK) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MM1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MM1_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off MM1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MM1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MM1_PWR_STA_MASK) != MM1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MM1_PWR_STA_MASK) != MM1_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MM1_PWR_CON, spm_read(MM1_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MM1_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MM1_PWR_CON) & MM1_SRAM_PDN_ACK_BIT0) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn on MM1" */
	}
	return err;
}

int spm_mtcmos_ctrl_cam(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		cam_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		/* TINFO="Start to turn off CAM" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, CAM_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & CAM_PROT_STEP1_0_ACK_MASK)
			!= CAM_PROT_STEP1_0_ACK_MASK) {
			pr_err("[CCF-2] %s\r\n", __func__);
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, CAM_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & CAM_PROT_STEP2_0_ACK_MASK)
			!= CAM_PROT_STEP2_0_ACK_MASK) {
			pr_err("[CCF-3] %s\r\n", __func__);
		}
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_SET, CAM_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & CAM_PROT_STEP2_1_ACK_MASK) != CAM_PROT_STEP2_1_ACK_MASK)
			pr_err("[CCF-4] %s\r\n", __func__);
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | CAM_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK = 1" */
		while ((spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK) != CAM_SRAM_PDN_ACK) {
			/*pr_err("[CCF-5] %s\r\n", __func__);*/
			/* Need f_fsmi_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & CAM_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & CAM_PWR_STA_MASK)) {
			/*pr_err("[CCF-6] %s\r\n", __func__);*/
			/* No logic between pwr_on and pwr_ack.*/
			/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off CAM" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on CAM" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & CAM_PWR_STA_MASK) != CAM_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & CAM_PWR_STA_MASK) != CAM_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK_BIT0) {
			/* Need f_fsmi_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK_BIT1) {
			/* Need f_fsmi_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, CAM_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_CLR, CAM_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, CAM_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on CAM" */
	}
	return err;
}

int spm_mtcmos_ctrl_ipu_shut_down(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		ipu_mtcmos_patch(state);
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ISO);
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ISO);
		/* TINFO="Start to turn off IPU" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, IPU_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & IPU_PROT_STEP1_0_ACK_MASK)
			!= IPU_PROT_STEP1_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, IPU_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & IPU_PROT_STEP2_0_ACK_MASK) != IPU_PROT_STEP2_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, IPU_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & IPU_PROT_STEP2_1_ACK_MASK)
			!= IPU_PROT_STEP2_1_ACK_MASK) {
		}
#endif
		/* TINFO="Set bus protect - step2 : 2" */
		spm_write(SMI0_CLAMP_SET, IPU_PROT_STEP2_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & IPU_PROT_STEP2_2_ACK_MASK) != IPU_PROT_STEP2_2_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="IPU_SRAM_CON[12]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 12));
		/* TINFO="IPU_SRAM_CON[13]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 13));
		/* TINFO="IPU_SRAM_CON[14]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 14));
		/* TINFO="IPU_SRAM_CON[15]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 15));
		/* TINFO="IPU_SRAM_CON[16]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 16));
		/* TINFO="IPU_SRAM_CON[17]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 17));
		/* TINFO="IPU_SRAM_CON[18]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 18));
		/* TINFO="IPU_SRAM_CON[19]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 19));
		/* TINFO="IPU_SRAM_CON[20]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 20));
		/* TINFO="IPU_SRAM_CON[21]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 21));
		/* TINFO="IPU_SRAM_CON[22]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 22));
		/* TINFO="IPU_SRAM_CON[23]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 23));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until IPU_SRAM_CON[24] = 1" */
		while ((spm_read(IPU_SRAM_CON) & (0x1 << 24)) != (0x1 << 24)) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until IPU_SRAM_CON[25] = 1" */
		while ((spm_read(IPU_SRAM_CON) & (0x1 << 25)) != (0x1 << 25)) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & IPU_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & IPU_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="EXT_BUCK_ISO[2]=1"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) | (0x1 << 2));
		/* TINFO="Finish to turn off IPU" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on IPU" */
		/* TINFO="EXT_BUCK_ISO[2]=0"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) & ~(0x1 << 2));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & IPU_PWR_STA_MASK) != IPU_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & IPU_PWR_STA_MASK) != IPU_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_RST_B);
		/* TINFO="IPU_SRAM_CON[12]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 12));
		/* TINFO="IPU_SRAM_CON[13]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 13));
		/* TINFO="IPU_SRAM_CON[14]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 14));
		/* TINFO="IPU_SRAM_CON[15]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 15));
		/* TINFO="IPU_SRAM_CON[16]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 16));
		/* TINFO="IPU_SRAM_CON[17]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 17));
		/* TINFO="IPU_SRAM_CON[18]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="IPU_SRAM_CON[19]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="IPU_SRAM_CON[20]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 20));
		/* TINFO="IPU_SRAM_CON[21]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 21));
		/* TINFO="IPU_SRAM_CON[22]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 22));
		/* TINFO="IPU_SRAM_CON[23]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 23));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until IPU_SRAM_CON[24] = 0" */
		while (spm_read(IPU_SRAM_CON) & (0x1 << 24)) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until IPU_SRAM_CON[25] = 0" */
		while (spm_read(IPU_SRAM_CON) & (0x1 << 25)) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, IPU_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, IPU_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 2" */
		spm_write(SMI0_CLAMP_CLR, IPU_PROT_STEP2_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, IPU_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		ipu_mtcmos_patch(state);
		/* TINFO="Finish to turn on IPU" */
	}
	return err;
}

int spm_mtcmos_ctrl_ipu_sleep(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		ipu_mtcmos_patch(state);
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ISO);
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_ISO);
		/* TINFO="Start to turn off IPU" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, IPU_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & IPU_PROT_STEP1_0_ACK_MASK)
			!= IPU_PROT_STEP1_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, IPU_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & IPU_PROT_STEP2_0_ACK_MASK) != IPU_PROT_STEP2_0_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, IPU_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & IPU_PROT_STEP2_1_ACK_MASK)
			!= IPU_PROT_STEP2_1_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set bus protect - step2 : 2" */
		spm_write(SMI0_CLAMP_SET, IPU_PROT_STEP2_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & IPU_PROT_STEP2_2_ACK_MASK) != IPU_PROT_STEP2_2_ACK_MASK) {
			/*pr_debug("");*/
			/*pr_debug("");*/
		}
#endif
		/* TINFO="IPU_SRAM_CON[0]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 0));
		/* TINFO="IPU_SRAM_CON[1]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 1));
		/* TINFO="IPU_SRAM_CON[4]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 4));
		/* TINFO="IPU_SRAM_CON[5]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 5));
		/* TINFO="IPU_SRAM_CON[6]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 6));
		/* TINFO="IPU_SRAM_CON[7]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 7));
		/* TINFO="IPU_SRAM_CON[8]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 8));
		/* TINFO="IPU_SRAM_CON[9]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 9));
		/* TINFO="IPU_SRAM_CON[10]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 10));
		/* TINFO="IPU_SRAM_CON[11]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 11));
		/* TINFO="IPU_SRAM_CON[12]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 12));
		/* TINFO="IPU_SRAM_CON[13]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 13));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & IPU_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & IPU_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off IPU" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on IPU" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & IPU_PWR_STA_MASK) != IPU_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & IPU_PWR_STA_MASK) != IPU_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_RST_B);
		/* TINFO="IPU_SRAM_CON[4]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 4));
		/* TINFO="IPU_SRAM_CON[5]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 5));
		/* TINFO="IPU_SRAM_CON[6]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 6));
		/* TINFO="IPU_SRAM_CON[7]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 7));
		/* TINFO="IPU_SRAM_CON[8]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 8));
		/* TINFO="IPU_SRAM_CON[9]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 9));
		/* TINFO="IPU_SRAM_CON[10]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 10));
		/* TINFO="IPU_SRAM_CON[11]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 11));
		/* TINFO="Delay 1us" */
		udelay(1);
		/* TINFO="IPU_SRAM_CON[1]=1"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) | (0x1 << 1));
		/* TINFO="IPU_SRAM_CON[0]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 0));
		/* TINFO="IPU_SRAM_CON[12]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 12));
		/* TINFO="IPU_SRAM_CON[13]=0"*/
		spm_write(IPU_SRAM_CON, spm_read(IPU_SRAM_CON) & ~(0x1 << 13));
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, IPU_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, IPU_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 2" */
		spm_write(SMI0_CLAMP_CLR, IPU_PROT_STEP2_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, IPU_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		ipu_mtcmos_patch(state);
		/* TINFO="Finish to turn on IPU" */
	}
	return err;
}

int spm_mtcmos_ctrl_isp(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		isp_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		spm_write(IPU_PWR_CON, spm_read(IPU_PWR_CON) | PWR_ISO);
		/* TINFO="Start to turn off ISP" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, ISP_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & ISP_PROT_STEP1_0_ACK_MASK)
			!= ISP_PROT_STEP1_0_ACK_MASK)
			pr_err("[CCF-2] %s\r\n", __func__);
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, ISP_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & ISP_PROT_STEP2_0_ACK_MASK)
			!= ISP_PROT_STEP2_0_ACK_MASK)
			pr_err("[CCF-3] %s\r\n", __func__);
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_SET, ISP_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & ISP_PROT_STEP2_1_ACK_MASK)
			!= ISP_PROT_STEP2_1_ACK_MASK)
			pr_err("[CCF-4] %s\r\n", __func__);
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & ISP_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & ISP_PWR_STA_MASK)) {
			/*pr_err("[CCF-5] %s\r\n", __func__);*/
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off ISP" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on ISP" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & ISP_PWR_STA_MASK) != ISP_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & ISP_PWR_STA_MASK) != ISP_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until ISP_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK_BIT0) {
			/* Need f_fsmi_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until ISP_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK_BIT1) {
			/* Need f_fsmi_ck for SRAM PDN delay IP. */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, ISP_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(SMI0_CLAMP_CLR, ISP_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, ISP_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		isp_mtcmos_patch(state);
		/* TINFO="Finish to turn on ISP" */
	}
	return err;
}

int spm_mtcmos_ctrl_ven(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		ven_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		/* TINFO="Start to turn off VEN" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, VEN_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & VEN_PROT_STEP1_0_ACK_MASK)
			!= VEN_PROT_STEP1_0_ACK_MASK)
			pr_err("[CCF-2] %s\r\n", __func__);
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_SET, VEN_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & VEN_PROT_STEP2_0_ACK_MASK) != VEN_PROT_STEP2_0_ACK_MASK)
			pr_err("[CCF-3] %s\r\n", __func__);/*fail here*/
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | VEN_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK = 1" */
		while ((spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK) != VEN_SRAM_PDN_ACK) {
			pr_err("[CCF-4] %s\r\n", __func__);
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VEN_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & VEN_PWR_STA_MASK)) {
			pr_err("[CCF-5] %s\r\n", __func__);
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Finish to turn off VEN" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VEN" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VEN_PWR_STA_MASK) != VEN_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & VEN_PWR_STA_MASK) != VEN_PWR_STA_MASK)) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT0) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT1) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT2) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT3) {
			/*  */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_CLR, VEN_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, VEN_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		ven_mtcmos_patch(state);
		/* TINFO="Finish to turn on VEN" */
	}
	return err;
}

int spm_mtcmos_ctrl_vde(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		vde_mtcmos_patch(state);
		/*pr_err("[CCF-1] %s\r\n", __func__);*/
		/* TINFO="Start to turn off VDE" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET, VDE_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_2) & VDE_PROT_STEP1_0_ACK_MASK) != VDE_PROT_STEP1_0_ACK_MASK)
			pr_err("[CCF-2] %s\r\n", __func__);/*fail here*/
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_SET, VDE_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI0_CLAMP) & VDE_PROT_STEP2_0_ACK_MASK) != VDE_PROT_STEP2_0_ACK_MASK)
			pr_err("[CCF-3] %s\r\n", __func__);
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | VDE_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VDE_SRAM_PDN_ACK = 1" */
		while ((spm_read(VDE_PWR_CON) & VDE_SRAM_PDN_ACK) != VDE_SRAM_PDN_ACK) {
			/*pr_err("[CCF-4] %s\r\n", __func__);*/
			/* Need f_fvdec_ck for SRAM PDN delay IP */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VDE_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & VDE_PWR_STA_MASK)) {
			pr_err("[CCF-5] %s\r\n", __func__);
			/* No logic between pwr_on and pwr_ack.*/
			/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off VDE" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VDE" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VDE_PWR_STA_MASK) != VDE_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & VDE_PWR_STA_MASK) != VDE_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VDE_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VDE_PWR_CON) & VDE_SRAM_PDN_ACK_BIT0) {
			/* Need f_fvdec_ck for SRAM PDN delay IP */
			/*pr_debug("");*/
		}
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(SMI0_CLAMP_CLR, VDE_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR, VDE_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on VDE" */
		vde_mtcmos_patch(state);
	}
	return err;
}

/* auto-gen end*/

/* enable op*/
/*
*static int general_sys_enable_op(struct subsys *sys)
*{
*	return spm_mtcmos_power_on_general_locked(sys, 1, 0);
*}
*/
static int MD1_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_md1(STA_POWER_ON);
}

static int MM0_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("DIS_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mm0(STA_POWER_ON);
}

static int MM1_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("DIS_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mm1(STA_POWER_ON);
}

static int MFG0_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg0(STA_POWER_ON);
}

static int MFG1_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg1(STA_POWER_ON);
}

static int MFG2_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg2(STA_POWER_ON);
}

static int MFG3_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg3(STA_POWER_ON);
}

static int ISP_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("ISP_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_isp(STA_POWER_ON);
}

static int VDE_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("VDE_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_vde(STA_POWER_ON);
}

static int VEN_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("VEN_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_ven(STA_POWER_ON);
}

static int IPU_SHUTDOWN_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_ASYNC_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_ipu_shut_down(STA_POWER_ON);
}

static int IPU_SLEEP_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MFG_ASYNC_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_ipu_sleep(STA_POWER_ON);
}

static int AUDIO_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("AUDIO_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_aud(STA_POWER_ON);
}

static int CAM_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("CAM_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_cam(STA_POWER_ON);
}

static int C2K_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("C2K_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_c2k(STA_POWER_ON);
}

static int MJC_sys_enable_op(struct subsys *sys)
{
	/*pr_debug("MDSYS_INTF_INFRA_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mjc(STA_POWER_ON);
}

/* disable op */
/*
*static int general_sys_disable_op(struct subsys *sys)
*{
*	return spm_mtcmos_power_off_general_locked(sys, 1, 0);
*}
*/
static int MD1_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_md1(STA_POWER_DOWN);
}

static int MM0_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("DIS_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mm0(STA_POWER_DOWN);
}

static int MM1_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("DIS_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mm1(STA_POWER_DOWN);
}

static int MFG0_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg0(STA_POWER_DOWN);
}

static int MFG1_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg1(STA_POWER_DOWN);
}

static int MFG2_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg2(STA_POWER_DOWN);
}

static int MFG3_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg3(STA_POWER_DOWN);
}

static int ISP_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("ISP_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
}

static int VDE_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("VDE_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_vde(STA_POWER_DOWN);
}

static int VEN_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("VEN_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_ven(STA_POWER_DOWN);
}

static int IPU_SHUTDOWN_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_ASYNC_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_ipu_shut_down(STA_POWER_DOWN);
}

static int IPU_SLEEP_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MFG_ASYNC_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_ipu_sleep(STA_POWER_DOWN);
}

static int AUDIO_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("AUDIO_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_aud(STA_POWER_DOWN);
}

static int CAM_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("CAM_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_cam(STA_POWER_DOWN);
}

static int C2K_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("C2K_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_c2k(STA_POWER_DOWN);
}

static int MJC_sys_disable_op(struct subsys *sys)
{
	/*pr_debug("MDSYS_INTF_INFRA_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mjc(STA_POWER_DOWN);
}


static int sys_get_state_op(struct subsys *sys)
{
	unsigned int sta = clk_readl(PWR_STATUS);
	unsigned int sta_s = clk_readl(PWR_STATUS_2ND);

	return (sta & sys->sta_mask) && (sta_s & sys->sta_mask);
}

/* ops */
/*
*static struct subsys_ops general_sys_ops = {
*	.enable = general_sys_enable_op,
*	.disable = general_sys_disable_op,
*	.get_state = sys_get_state_op,
*};
*/

static struct subsys_ops MD1_sys_ops = {
	.enable = MD1_sys_enable_op,
	.disable = MD1_sys_disable_op,
	.get_state = sys_get_state_op,
};


static struct subsys_ops MM0_sys_ops = {
	.enable = MM0_sys_enable_op,
	.disable = MM0_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MM1_sys_ops = {
	.enable = MM1_sys_enable_op,
	.disable = MM1_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG0_sys_ops = {
	.enable = MFG0_sys_enable_op,
	.disable = MFG0_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG1_sys_ops = {
	.enable = MFG1_sys_enable_op,
	.disable = MFG1_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG2_sys_ops = {
	.enable = MFG2_sys_enable_op,
	.disable = MFG2_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG3_sys_ops = {
	.enable = MFG3_sys_enable_op,
	.disable = MFG3_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops ISP_sys_ops = {
	.enable = ISP_sys_enable_op,
	.disable = ISP_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops VDE_sys_ops = {
	.enable = VDE_sys_enable_op,
	.disable = VDE_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops VEN_sys_ops = {
	.enable = VEN_sys_enable_op,
	.disable = VEN_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops IPU_SHUTDOWN_sys_ops = {
	.enable = IPU_SHUTDOWN_sys_enable_op,
	.disable = IPU_SHUTDOWN_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops IPU_SLEEP_sys_ops = {
	.enable = IPU_SLEEP_sys_enable_op,
	.disable = IPU_SLEEP_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops AUDIO_sys_ops = {
	.enable = AUDIO_sys_enable_op,
	.disable = AUDIO_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops CAM_sys_ops = {
	.enable = CAM_sys_enable_op,
	.disable = CAM_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops C2K_sys_ops = {
	.enable = C2K_sys_enable_op,
	.disable = C2K_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MJC_sys_ops = {
	.enable = MJC_sys_enable_op,
	.disable = MJC_sys_disable_op,
	.get_state = sys_get_state_op,
};



static int subsys_is_on(enum subsys_id id)
{
	int r;
	struct subsys *sys = id_to_sys(id);

	WARN_ON(!sys);

	r = sys->ops->get_state(sys);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s:%d, sys=%s, id=%d\n", __func__, r, sys->name, id);
#endif				/* MT_CCF_DEBUG */

	return r;
}

#if CONTROL_LIMIT
int allow[NR_SYSS] = {
1,	/*SYS_MD1 = 1,*/
1,	/*SYS_MM0 = 2,*/
0,	/*SYS_MM1 = 3,*/
1,	/*SYS_MFG0 = 4,*/
1,	/*SYS_MFG1 = 5,*/
1,	/*SYS_MFG2 = 6,*/
0,	/*SYS_MFG3 = 7,*/
1,	/*SYS_ISP = 8,*/
1,	/*SYS_VDE = 9,*/
1,	/*SYS_VEN = 10,*/
1,	/*SYS_IPU_SHUTDOWN = 11,*/
1,	/*SYS_IPU_SLEEP = 12,*/
1,	/*SYS_AUDIO = 13,*/
1,	/*SYS_CAM = 14,*/
1,	/*SYS_C2K = 15,*/
1,	/*SYS_MJC = 16,*/
};
#endif
static int enable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);
	struct pg_callbacks *pgcb;

	WARN_ON(!sys);

#if MT_CCF_BRINGUP
	/*pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_MD1:
		spm_mtcmos_ctrl_md1(STA_POWER_ON);
		break;
	case SYS_C2K:
		spm_mtcmos_ctrl_c2k(STA_POWER_ON);
		break;
	default:
		break;
	}
	return 0;
#endif				/* MT_CCF_BRINGUP */

#if CONTROL_LIMIT
	#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);
	#endif
	if (allow[id] == 0) {
		#if MT_CCF_DEBUG
		pr_debug("[CCF] %s: do nothing return\n", __func__);
		#endif
		return 0;
	}
#endif


	mtk_clk_lock(flags);

#if CHECK_PWR_ST
	if (sys->ops->get_state(sys) == SUBSYS_PWR_ON) {
		mtk_clk_unlock(flags);
		return 0;
	}
#endif				/* CHECK_PWR_ST */

	r = sys->ops->enable(sys);
	WARN_ON(r);

	mtk_clk_unlock(flags);

	list_for_each_entry(pgcb, &pgcb_list, list) {
		if (pgcb->after_on)
			pgcb->after_on(id);
	}

	return r;
}

static int disable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);
	struct pg_callbacks *pgcb;

	WARN_ON(!sys);

#if MT_CCF_BRINGUP
	/*pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_MD1:
		spm_mtcmos_ctrl_md1(STA_POWER_DOWN);
		break;
	case SYS_C2K:
		spm_mtcmos_ctrl_c2k(STA_POWER_DOWN);
		break;
	default:
		break;
	}
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if CONTROL_LIMIT
	#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);
	#endif
	if (allow[id] == 0) {
		#if MT_CCF_DEBUG
		pr_debug("[CCF] %s: do nothing return\n", __func__);
		#endif
		return 0;
	}
#endif



	/* TODO: check all clocks related to this subsys are off */
	/* could be power off or not */
	list_for_each_entry_reverse(pgcb, &pgcb_list, list) {
		if (pgcb->before_off)
			pgcb->before_off(id);
	}

	mtk_clk_lock(flags);

#if CHECK_PWR_ST
	if (sys->ops->get_state(sys) == SUBSYS_PWR_DOWN) {
		mtk_clk_unlock(flags);
		return 0;
	}
#endif				/* CHECK_PWR_ST */

	r = sys->ops->disable(sys);
	WARN_ON(r);

	mtk_clk_unlock(flags);

	return r;
}

/*
 * power_gate
 */

struct mt_power_gate {
	struct clk_hw hw;
	struct clk *pre_clk;
	struct clk *pre_clk2;
	enum subsys_id pd_id;
};

#define to_power_gate(_hw) container_of(_hw, struct mt_power_gate, hw)

static int pg_enable(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, pd_id=%u\n", __func__,
		 __clk_get_name(hw->clk), pg->pd_id);
#endif				/* MT_CCF_DEBUG */

	return enable_subsys(pg->pd_id);
}

static void pg_disable(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, pd_id=%u\n", __func__,
		 __clk_get_name(hw->clk), pg->pd_id);
#endif				/* MT_CCF_DEBUG */

	disable_subsys(pg->pd_id);
}

static int pg_is_enabled(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);
#if MT_CCF_BRINGUP
	return 1;
#endif				/* MT_CCF_BRINGUP */

	return subsys_is_on(pg->pd_id);
}

int pg_prepare(struct clk_hw *hw)
{
	int r;
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, pre_sys=%s\n", __func__,
		 __clk_get_name(hw->clk),
		 pg->pre_clk ? __clk_get_name(pg->pre_clk) : "");
#endif				/* MT_CCF_DEBUG */

	if (pg->pre_clk) {
		r = clk_prepare_enable(pg->pre_clk);
		if (r)
			return r;
	}
	if (pg->pre_clk2) {
		r = clk_prepare_enable(pg->pre_clk2);
		if (r)
			return r;
	}

	return pg_enable(hw);

}

void pg_unprepare(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: clk=%s, pre_clk=%s\n", __func__,
		 __clk_get_name(hw->clk),
		 pg->pre_clk ? __clk_get_name(pg->pre_clk) : "");
#endif				/* MT_CCF_DEBUG */

	pg_disable(hw);

	if (pg->pre_clk)
		clk_disable_unprepare(pg->pre_clk);
	if (pg->pre_clk2)
		clk_disable_unprepare(pg->pre_clk2);
}

static const struct clk_ops mt_power_gate_ops = {
	.prepare = pg_prepare,
	.unprepare = pg_unprepare,
	.is_enabled = pg_is_enabled,
};

struct clk *mt_clk_register_power_gate(const char *name,
				       const char *parent_name,
				       struct clk *pre_clk,
				       struct clk *pre_clk2, enum subsys_id pd_id)
{
	struct mt_power_gate *pg;
	struct clk *clk;
	struct clk_init_data init;

	pg = kzalloc(sizeof(*pg), GFP_KERNEL);
	if (!pg)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.flags = CLK_IGNORE_UNUSED;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = parent_name ? 1 : 0;
	init.ops = &mt_power_gate_ops;

	pg->pre_clk = pre_clk;
	pg->pre_clk2 = pre_clk2;
	pg->pd_id = pd_id;
	pg->hw.init = &init;

	clk = clk_register(NULL, &pg->hw);
	if (IS_ERR(clk))
		kfree(pg);

	return clk;
}

#define pg_md1         "pg_md1"
#define pg_mm0         "pg_mm0"
#define pg_mm1         "pg_mm1"
#define pg_mfg0         "pg_mfg0"
#define pg_mfg1         "pg_mfg1"
#define pg_mfg2         "pg_mfg2"
#define pg_mfg3         "pg_mfg3"
#define pg_isp         "pg_isp"
#define pg_vde         "pg_vde"
#define pg_ven         "pg_ven"
#define pg_ipu_shutdown   "pg_ipu_shutdown"
#define pg_ipu_sleep   "pg_ipu_sleep"
#define pg_audio       "pg_audio"
#define pg_cam   "pg_cam"
#define pg_c2k         "pg_c2k"
#define pg_mjc   "pg_mjc"

#define audio_sel	"audio_sel"
#define slow_mfg_sel	"slow_mfg_sel"
#define mjc_sel	"mjc_sel"
#define mm_sel	"mm_sel"
#define smi0_2x_sel	"smi0_2x_sel"
#define vdec_sel	"vdec_sel"
#define infra_smi_l2c	"infra_smi_l2c"
#define img_sel	"img_sel"
#define cam_sel	"cam_sel"
#define ipu_if_sel	"ipu_if_sel"
/* FIXME: set correct value: E */

struct mtk_power_gate {
	int id;
	const char *name;
	const char *parent_name;
	const char *pre_clk_name;
	const char *pre_clk2_name;
	enum subsys_id pd_id;
};

#define PGATE(_id, _name, _parent, _pre_clk, _pd_id) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.pre_clk_name = _pre_clk,		\
		.pd_id = _pd_id,			\
	}

#define PGATE2(_id, _name, _parent, _pre_clk, _pre_clk2, _pd_id) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.pre_clk_name = _pre_clk,		\
		.pre_clk2_name = _pre_clk2,		\
		.pd_id = _pd_id,			\
	}

/* FIXME: all values needed to be verified */
struct mtk_power_gate scp_clks[] __initdata = {
	#if 1
	PGATE2(SCP_SYS_MD1, pg_md1, NULL, NULL, NULL, SYS_MD1),
	PGATE2(SCP_SYS_MM0, pg_mm0, NULL, mm_sel, smi0_2x_sel, SYS_MM0),
	PGATE2(SCP_SYS_MM1, pg_mm1, NULL, NULL, NULL, SYS_MM1),
	PGATE2(SCP_SYS_IPU_SHUTDOWN, pg_ipu_shutdown, NULL, ipu_if_sel, NULL, SYS_IPU_SHUTDOWN),
	PGATE2(SCP_SYS_IPU_SLEEP, pg_ipu_sleep, NULL, ipu_if_sel, NULL, SYS_IPU_SLEEP),
	PGATE2(SCP_SYS_MFG0, pg_mfg0, NULL, NULL, NULL, SYS_MFG0),
	PGATE2(SCP_SYS_MFG1, pg_mfg1, NULL, slow_mfg_sel, NULL, SYS_MFG1),
	PGATE2(SCP_SYS_MFG2, pg_mfg2, NULL, slow_mfg_sel, NULL, SYS_MFG2),
	PGATE2(SCP_SYS_MFG3, pg_mfg3, NULL, NULL, NULL, SYS_MFG3),
	PGATE2(SCP_SYS_ISP, pg_isp, NULL, img_sel, NULL, SYS_ISP),
	PGATE2(SCP_SYS_VDE, pg_vde, NULL, vdec_sel, smi0_2x_sel, SYS_VDE),
	PGATE2(SCP_SYS_VEN, pg_ven, NULL, infra_smi_l2c, NULL, SYS_VEN),
	PGATE2(SCP_SYS_AUDIO, pg_audio, NULL, audio_sel, NULL, SYS_AUDIO),
	PGATE2(SCP_SYS_CAM, pg_cam, NULL, cam_sel, NULL, SYS_CAM),
	PGATE2(SCP_SYS_C2K, pg_c2k, NULL, NULL, NULL, SYS_C2K),
	PGATE2(SCP_SYS_MJC, pg_mjc, NULL, mjc_sel, NULL, SYS_MJC),
	#else
	PGATE(SCP_SYS_MD1, pg_md1, NULL, NULL, SYS_MD1),
	PGATE(SCP_SYS_MM0, pg_mm0, NULL, mm_sel, SYS_MM0),
	PGATE(SCP_SYS_MM1, pg_mm1, NULL, NULL, SYS_MM1),
	PGATE(SCP_SYS_IPU_SHUTDOWN, pg_ipu_shutdown, NULL, NULL, SYS_IPU_SHUTDOWN),
	PGATE(SCP_SYS_IPU_SLEEP, pg_ipu_sleep, NULL, NULL, SYS_IPU_SLEEP),
	PGATE(SCP_SYS_MFG0, pg_mfg0, NULL, NULL, SYS_MFG0),
	PGATE(SCP_SYS_MFG1, pg_mfg1, NULL, slow_mfg_sel, SYS_MFG1),
	PGATE(SCP_SYS_MFG2, pg_mfg2, NULL, slow_mfg_sel, SYS_MFG2),
	PGATE(SCP_SYS_MFG3, pg_mfg3, NULL, NULL, SYS_MFG3),
	PGATE(SCP_SYS_ISP, pg_isp, NULL, img_sel, SYS_ISP),
	PGATE(SCP_SYS_VDE, pg_vde, NULL, vdec_sel, SYS_VDE),
	PGATE(SCP_SYS_VEN, pg_ven, NULL, infra_smi_l2c, SYS_VEN),
	PGATE(SCP_SYS_AUDIO, pg_audio, NULL, audio_sel, SYS_AUDIO),
	PGATE(SCP_SYS_CAM, pg_cam, NULL, cam_sel, SYS_CAM),
	PGATE(SCP_SYS_C2K, pg_c2k, NULL, NULL, SYS_C2K),
	PGATE(SCP_SYS_MJC, pg_mjc, NULL, mjc_sel, SYS_MJC),
	#endif
};

static void __init init_clk_scpsys(void __iomem *infracfg_reg,
					void __iomem *spm_reg,
					void __iomem *smi_common_reg,
					struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;
	struct clk *pre_clk;
	struct clk *pre_clk2;

	infracfg_base = infracfg_reg;
	spm_base = spm_reg;
	smi_common_base = smi_common_reg;

	syss[SYS_MD1].ctl_addr = MD1_PWR_CON;
	syss[SYS_MM0].ctl_addr = MM0_PWR_CON;
	syss[SYS_MM1].ctl_addr = MM1_PWR_CON;
	syss[SYS_MFG0].ctl_addr = MFG0_PWR_CON;
	syss[SYS_MFG1].ctl_addr = MFG1_PWR_CON;
	syss[SYS_MFG2].ctl_addr = MFG2_PWR_CON;
	syss[SYS_MFG3].ctl_addr = MFG3_PWR_CON;
	syss[SYS_ISP].ctl_addr = ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = VDE_PWR_CON;
	syss[SYS_VEN].ctl_addr = VEN_PWR_CON;
	syss[SYS_IPU_SHUTDOWN].ctl_addr = IPU_PWR_CON;
	syss[SYS_IPU_SLEEP].ctl_addr = IPU_PWR_CON;
	syss[SYS_AUDIO].ctl_addr = AUD_PWR_CON;
	syss[SYS_CAM].ctl_addr = CAM_PWR_CON;
	syss[SYS_C2K].ctl_addr = C2K_PWR_CON;
	syss[SYS_MJC].ctl_addr = MJC_PWR_CON;

	for (i = 0; i < ARRAY_SIZE(scp_clks); i++) {
		struct mtk_power_gate *pg = &scp_clks[i];

		pre_clk = pg->pre_clk_name ? __clk_lookup(pg->pre_clk_name) : NULL;
		pre_clk2 = pg->pre_clk2_name ? __clk_lookup(pg->pre_clk2_name) : NULL;

		/*clk = mt_clk_register_power_gate(pg->name, pg->parent_name, pre_clk, pg->pd_id);*/
		clk = mt_clk_register_power_gate(pg->name, pg->parent_name, pre_clk, pre_clk2, pg->pd_id);

		if (IS_ERR(clk)) {
			pr_err("[CCF] %s: Failed to register clk %s: %ld\n",
			       __func__, pg->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[pg->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] %s: pgate %3d: %s\n", __func__, i, pg->name);
#endif				/* MT_CCF_DEBUG */
	}
}

/*
 * device tree support
 */

/* TODO: remove this function */
static struct clk_onecell_data *alloc_clk_data(unsigned int clk_num)
{
	int i;
	struct clk_onecell_data *clk_data;

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return NULL;

	clk_data->clks = kcalloc(clk_num, sizeof(struct clk *), GFP_KERNEL);
	if (!clk_data->clks) {
		kfree(clk_data);
		return NULL;
	}

	clk_data->clk_num = clk_num;

	for (i = 0; i < clk_num; ++i)
		clk_data->clks[i] = ERR_PTR(-ENOENT);

	return clk_data;
}

/* TODO: remove this function */
static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

static void __init mt_scpsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *infracfg_reg;
	void __iomem *spm_reg;
	void __iomem *smi_common_reg;
	int r;

	infracfg_reg = get_reg(node, 0);
	spm_reg = get_reg(node, 1);
	smi_common_reg = get_reg(node, 2);

	if (!infracfg_reg || !spm_reg || !smi_common_reg) {
		pr_err("clk-pg-mt6799: missing reg\n");
		return;
	}

	pr_err("[CCF] clk-pg-mt6799: get reg I\n");

/*
*   pr_debug("[CCF] %s: sys: %s, reg: 0x%p, 0x%p\n",
*		__func__, node->name, infracfg_reg, spm_reg);
*/

	clk_data = alloc_clk_data(SCP_NR_SYSS);

	init_clk_scpsys(infracfg_reg, spm_reg, smi_common_reg, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("[CCF] %s:could not register clock provide\n", __func__);

#if 0/*!MT_CCF_BRINGUP*/
	/* subsys init: per modem owner request, disable modem power first */
	disable_subsys(SYS_MD1);
	disable_subsys(SYS_C2K);
#else				/*power on all subsys for bring up */
#ifndef CONFIG_FPGA_EARLY_PORTING
	/*spm_mtcmos_ctrl_mfg0(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_mfg1(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_mfg2(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_mfg3(STA_POWER_ON);*/
	spm_mtcmos_ctrl_c2k(STA_POWER_DOWN);
	spm_mtcmos_ctrl_md1(STA_POWER_DOWN);
	/*spm_mtcmos_ctrl_aud(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_mjc(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_cam(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_ipu_shut_down(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_ipu_sleep(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_isp(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_ven(STA_POWER_ON);*/
	/*spm_mtcmos_ctrl_vde(STA_POWER_ON);*/
#endif
#endif				/* !MT_CCF_BRINGUP */
}

CLK_OF_DECLARE(mtk_pg_regs, "mediatek,mt6799-scpsys", mt_scpsys_init);

void subsys_if_on(void)
{
	unsigned int sta = spm_read(PWR_STATUS);
	unsigned int sta_s = spm_read(PWR_STATUS_2ND);

	if ((sta & (1U << 1)) && (sta_s & (1U << 1)))
		pr_err("suspend warning: SYS_MFG0 is on!!!\n");
	if ((sta & (1U << 2)) && (sta_s & (1U << 2)))
		pr_err("suspend warning: SYS_MFG1 is on!!!\n");
	if ((sta & (1U << 3)) && (sta_s & (1U << 3)))
		pr_err("suspend warning: SYS_MFG2 is on!!!\n");
	if ((sta & (1U << 5)) && (sta_s & (1U << 5)))
		pr_err("suspend warning: SYS_C2K is on!!!\n");
	if ((sta & (1U << 6)) && (sta_s & (1U << 6)))
		pr_err("suspend warning: SYS_MD1 is on!!!\n");
	if ((sta & (1U << 13)) && (sta_s & (1U << 13)))
		pr_err("suspend warning: SYS_AUD is on!!!\n");
	if ((sta & (1U << 14)) && (sta_s & (1U << 14)))
		pr_err("suspend warning: SYS_MJC is on!!!\n");
	if ((sta & (1U << 15)) && (sta_s & (1U << 15)))
		pr_err("suspend warning: SYS_MM0 is on!!!\n");
	if ((sta & (1U << 17)) && (sta_s & (1U << 17)))
		pr_err("suspend warning: SYS_CAM is on!!!\n");
	if ((sta & (1U << 18)) && (sta_s & (1U << 18)))
		pr_err("suspend warning: SYS_IPU is on!!!\n");
	if ((sta & (1U << 19)) && (sta_s & (1U << 19)))
		pr_err("suspend warning: SYS_ISP is on!!!\n");
	if ((sta & (1U << 20)) && (sta_s & (1U << 20)))
		pr_err("suspend warning: SYS_VEN is on!!!\n");
	if ((sta & (1U << 21)) && (sta_s & (1U << 21)))
		pr_err("suspend warning: SYS_VDE is on!!!\n");
}

#if 1 /*only use for suspend test*/
void mtcmos_force_off(void)
{
	spm_mtcmos_ctrl_mfg2(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg1(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg0(STA_POWER_DOWN);

	spm_mtcmos_ctrl_cam(STA_POWER_DOWN);
	spm_mtcmos_ctrl_ven(STA_POWER_DOWN);
	spm_mtcmos_ctrl_vde(STA_POWER_DOWN);
	spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
	spm_mtcmos_ctrl_ipu_sleep(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mjc(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mm0(STA_POWER_DOWN);

	spm_mtcmos_ctrl_md1(STA_POWER_DOWN);
	spm_mtcmos_ctrl_c2k(STA_POWER_DOWN);

	spm_mtcmos_ctrl_aud(STA_POWER_DOWN);
}
#endif

#if CLK_DEBUG
/*
 * debug / unit test
 */

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

static char last_cmd[128] = "null";

static int test_pg_dump_regs(struct seq_file *s, void *v)
{
	int i;

	for (i = 0; i < NR_SYSS; i++) {
		if (!syss[i].ctl_addr)
			continue;

		seq_printf(s, "%10s: [0x%p]: 0x%08x\n", syss[i].name,
			   syss[i].ctl_addr, clk_readl(syss[i].ctl_addr));
	}

	return 0;
}

static void dump_pg_state(const char *clkname, struct seq_file *s)
{
	struct clk *c = __clk_lookup(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : __clk_get_parent(c);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%17s: %3s, %3d, %3d, %10ld, %17s]\n",
		   __clk_get_name(c),
		   __clk_is_enabled(c) ? "ON" : "off",
		   __clk_get_prepare_count(c),
		   __clk_get_enable_count(c), __clk_get_rate(c), p ? __clk_get_name(p) : "");


	clk_put(c);
}

static int test_pg_dump_state_all(struct seq_file *s, void *v)
{
	static const char *const clks[] = {
		pg_md1,
		pg_dis,
		pg_mfg,
		pg_isp,
		pg_vde,
		pg_ven,
		pg_mfg_async,
		pg_audio,
		pg_cam,
		pg_c2k,
		pg_mdsys_intf_infra,
		pg_mfg_core1,
		pg_mfg_core0,
	};

	int i;

/*	pr_debug("\n");*/
	for (i = 0; i < ARRAY_SIZE(clks); i++)
		dump_pg_state(clks[i], s);

	return 0;
}

static struct {
	const char *name;
	struct clk *clk;
} g_clks[] = {
	{
	.name = pg_md1}, {
	.name = pg_vde}, {
	.name = pg_ven}, {
.name = pg_mfg},};

static int test_pg_1(struct seq_file *s, void *v)
{
	int i;

/*	pr_debug("\n");*/

	for (i = 0; i < ARRAY_SIZE(g_clks); i++) {
		g_clks[i].clk = __clk_lookup(g_clks[i].name);
		if (IS_ERR_OR_NULL(g_clks[i].clk)) {
			seq_printf(s, "clk_get(%s): NULL\n", g_clks[i].name);
			continue;
		}

		clk_prepare_enable(g_clks[i].clk);
		seq_printf(s, "clk_prepare_enable(%s)\n", __clk_get_name(g_clks[i].clk));
	}

	return 0;
}

static int test_pg_2(struct seq_file *s, void *v)
{
	int i;

/*	pr_debug("\n");*/

	for (i = 0; i < ARRAY_SIZE(g_clks); i++) {
		if (IS_ERR_OR_NULL(g_clks[i].clk)) {
			seq_printf(s, "(%s).clk: NULL\n", g_clks[i].name);
			continue;
		}

		seq_printf(s, "clk_disable_unprepare(%s)\n", __clk_get_name(g_clks[i].clk));
		clk_disable_unprepare(g_clks[i].clk);
		clk_put(g_clks[i].clk);
	}

	return 0;
}

static int test_pg_show(struct seq_file *s, void *v)
{
	static const struct {
		int (*fn)(struct seq_file *, void *);
		const char *cmd;
	} cmds[] = {
		{
		.cmd = "dump_regs", .fn = test_pg_dump_regs}, {
		.cmd = "dump_state", .fn = test_pg_dump_state_all}, {
		.cmd = "1", .fn = test_pg_1}, {
	.cmd = "2", .fn = test_pg_2},};

	int i;

/*	pr_debug("last_cmd: %s\n", last_cmd);*/

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (strcmp(cmds[i].cmd, last_cmd) == 0)
			return cmds[i].fn(s, v);
	}

	return 0;
}

static int test_pg_open(struct inode *inode, struct file *file)
{
	return single_open(file, test_pg_show, NULL);
}

static ssize_t test_pg_write(struct file *file,
			     const char __user *buffer, size_t count, loff_t *data)
{
	char desc[sizeof(last_cmd)];
	int len = 0;

/*	pr_debug("count: %zu\n", count);*/
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	strcpy(last_cmd, desc);
	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations test_pg_fops = {
	.owner = THIS_MODULE,
	.open = test_pg_open,
	.read = seq_read,
	.write = test_pg_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init debug_init(void)
{
	static int init;
	struct proc_dir_entry *entry;

/*	pr_debug("init: %d\n", init);*/

	if (init)
		return 0;

	++init;

	entry = proc_create("test_pg", 0, 0, &test_pg_fops);
	if (!entry)
		return -ENOMEM;

	++init;
	return 0;
}

static void __exit debug_exit(void)
{
	remove_proc_entry("test_pg", NULL);
}

module_init(debug_init);
module_exit(debug_exit);

#endif				/* CLK_DEBUG */
