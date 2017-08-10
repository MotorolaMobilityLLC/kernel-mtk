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
#include "clk-mt6755-pg.h"

#include <dt-bindings/clock/mt6755-clk.h>

#define VLTE_SUPPORT
#ifdef VLTE_SUPPORT
/* #include <mach/mt_gpio.h> */
/* #include <mach/upmu_common.h> */
#endif /* VLTE_SUPPORT */

/*#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP	0
#endif*/
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP	1
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
	do {	\
		__raw_writel((v), IOMEM((a)));   \
		mb();  /* for mt_reg_sync_writel() */ \
	} while (0)
#define spm_read(addr)			__raw_readl(IOMEM(addr))
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

/*
 * MTCMOS
 */

#define STA_POWER_DOWN	0
#define STA_POWER_ON	1
#define SUBSYS_PWR_DOWN		0
#define SUBSYS_PWR_ON		1
#define PWR_CLK_DIS             (1U << 4)
#define PWR_ON_2ND              (1U << 3)
#define PWR_ON                  (1U << 2)
#define PWR_ISO                 (1U << 1)
#define PWR_RST_B               (1U << 0)

struct subsys;

struct subsys_ops {
	int (*enable)(struct subsys *sys);
	int (*disable)(struct subsys *sys);
	int (*get_state)(struct subsys *sys);
};

struct subsys {
	const char		*name;
	uint32_t		sta_mask;
	void __iomem		*ctl_addr;
	uint32_t		sram_pdn_bits;
	uint32_t		sram_pdn_ack_bits;
	uint32_t		bus_prot_mask;
	struct subsys_ops	*ops;
};

/*static struct subsys_ops general_sys_ops;*/
static struct subsys_ops MD1_sys_ops;
static struct subsys_ops C2K_sys_ops;
static struct subsys_ops CONN_sys_ops;
static struct subsys_ops MFG_sys_ops;
static struct subsys_ops DIS_sys_ops;
static struct subsys_ops ISP_sys_ops;
static struct subsys_ops VDE_sys_ops;
static struct subsys_ops VEN_sys_ops;
static struct subsys_ops AUD_sys_ops;
/*static struct subsys_ops IFR_sys_ops;*/
static void __iomem *infracfg_base;
static void __iomem *spm_base;
static void __iomem *infra_base;

#define INFRACFG_REG(offset)		(infracfg_base + offset)
#define SPM_REG(offset)			(spm_base + offset)
#define INFRA_REG(offset)			(infra_base + offset)

/**************************************
 * for non-CPU MTCMOS
 **************************************/
static DEFINE_SPINLOCK(spm_noncpu_lock);
#define spm_mtcmos_noncpu_lock(flags)   spin_lock_irqsave(&spm_noncpu_lock, flags)

#define spm_mtcmos_noncpu_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_lock, flags)

/* FIXME: set correct value: S */
#define POWERON_CONFIG_EN			SPM_REG(0x0000)
#define PWR_STATUS			SPM_REG(0x0180) /* correct */
#define PWR_STATUS_2ND	SPM_REG(0x0184) /* correct */

#define MD1_PWR_CON			SPM_REG(0x0320) /* correct */
#define C2K_PWR_CON			SPM_REG(0x0328) /* correct */
#define CONN_PWR_CON		SPM_REG(0x032c) /* correct */
#define DIS_PWR_CON			SPM_REG(0x030c)
#define MFG_PWR_CON			SPM_REG(0x0338)
#define ISP_PWR_CON			SPM_REG(0x0308)
#define VDE_PWR_CON			SPM_REG(0x0300)
#define VEN_PWR_CON			SPM_REG(0x0304)
#define PCM_IM_PTR			SPM_REG(0x0020) /* correct */
#define PCM_IM_LEN			SPM_REG(0x0024) /* correct */
#define MD_EXT_BUCK_ISO SPM_REG(0x0390)
#define AUDIO_PWR_CON			SPM_REG(0x0314) /* correct */
#define MFG_ASYNC_PWR_CON			SPM_REG(0x0334) /* correct */
#define IFR_PWR_CON			SPM_REG(0x0318) /* correct */
#define SLEEP_CPU_WAKEUP_EVENT	SPM_REG(0x00B0) /* correct */
#define PCM_PASR_DPD_3		SPM_REG(0x063C) /* correct */
#define MDSYS_INTF_INFRA_PWR_CON       SPM_REG(0x0360)

#define INFRA_TOPAXI_PROTECTEN		INFRACFG_REG(0x0220) /* correct */
#define INFRA_TOPAXI_PROTECTSTA1	INFRACFG_REG(0x0228) /* correct */
#define INFRA_TOPAXI_PROTECTEN_1	INFRACFG_REG(0x0250) /* correct */
#define INFRA_TOPAXI_PROTECTSTA1_1	INFRACFG_REG(0x0258) /* correct */
#define C2K_SPM_CTRL			INFRACFG_REG(0x0368) /* correct */


#define INFRA_BUS_IDLE_STA1	INFRA_REG(0x0180)
#define INFRA_BUS_IDLE_STA2	INFRA_REG(0x0184)
#define INFRA_BUS_IDLE_STA3	INFRA_REG(0x0188)
#define INFRA_BUS_IDLE_STA4	INFRA_REG(0x018c)
#define INFRA_BUS_IDLE_STA5	INFRA_REG(0x0190)



#define SPM_PROJECT_CODE		0xb16
/* MCU/CPU */
#define PWR_RST_B_BIT			BIT(0)
#define PWR_ISO_BIT				BIT(1)
#define PWR_ON_BIT				BIT(2)
#define PWR_ON_2ND_BIT			BIT(3)
#define PWR_CLK_DIS_BIT			BIT(4)

#define MD1_PWR_STA_MASK		BIT(0)
#define CONN_PWR_STA_MASK		BIT(1)
#define DIS_PWR_STA_MASK		BIT(3)
#define MFG_PWR_STA_MASK		BIT(4)
#define ISP_PWR_STA_MASK		BIT(5)
#define VDE_PWR_STA_MASK		BIT(7)
#define VEN_PWR_STA_MASK		BIT(21)
#define IFR_PWR_STA_MASK                 (0x1 << 6)
#define MFG_ASYNC_PWR_STA_MASK		BIT(23)
#define AUDIO_PWR_STA_MASK		BIT(24)
#define C2K_PWR_STA_MASK		BIT(28)
#define MDSYS_INTF_INFRA_PWR_STA_MASK		BIT(29)
/* XXX_PWR_CON */
#define SRAM_PDN            (0xf << 8) /* VDEC, VENC, ISP, DISP */



/* PDN_ACK, all 4b from 12,  ref. mtcmos_ctrl2.c */
#define VDE_SRAM_ACK        (0x1 << 12)
#define VEN_SRAM_ACK        (0xf << 12)
#define ISP_SRAM_ACK        (0x3 << 12)
#define DIS_SRAM_ACK        (0x1 << 12)
#define MFG_SRAM_ACK        (0x1 << 16) /* jade */
/* BUS Protect? ref. mtcmos_ctrl2.c, INFRA_TOPAXI_PROTECTEN */
#define IFR_SRAM_PDN_ACK                 (0xF << 12)
#define DIS_PROT_MASK		((0x1<<1)) /* bit 1, 6, 16; if bit6 set, MMSYS PDN, access reg will hang, */
#define MFG_PROT_MASK			((0x1<<21))
#define MD1_PROT_MASK       ((0x1<<16) | (0x1<<17) | (0x1<<18) | (0x1<<19) | (0x1<<20) | (0x1<<21) | (0x1<<28))
#define MD1_PROT_CHECK_MASK       ((0x1<<16) | (0x1<<17) | (0x1<<18) | (0x1<<19) | (0x1<<20) | (0x1<<21))
#define C2K_PROT_MASK       ((0x1<<22) | (0x1<<23) | (0x1<<24)) /* bit 29, 30, 31 */
#define CONN_PROT_MASK      ((0x1<<13) | (0x1<<14))/* bit 2, 8 */

#define MDSYS_INTF_INFRA_PROT_MASK       ((0x1 << 3) \
					  |(0x1 << 4))
#define CONN_SRAM_PDN                     (0x1 << 8)
#define MD1_SRAM_PDN                     (0x1 << 8)
#define MD1_SRAM_PDN_ACK                 (0x0 << 12)
#define C2K_SRAM_PDN                     (0x1 << 8)
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
static struct subsys syss[] =	/* NR_SYSS */ /* FIXME: set correct value */
{
	[SYS_MD1] = {
		.name = __stringify(SYS_MD1),
		.sta_mask = MD1_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_MD_PWR_CON, */
		.sram_pdn_bits = MD1_SRAM_PDN,
		.sram_pdn_ack_bits = 0, /* GENMASK(15, 12), */
		.bus_prot_mask = MD1_PROT_MASK,
		.ops = &MD1_sys_ops,
	},
	[SYS_MD2] = {
		.name = __stringify(SYS_MD2),
		.sta_mask = C2K_PWR_STA_MASK,
		/*.ctl_addr = NULL, SPM_C2K_PWR_CON, */
		.sram_pdn_bits = C2K_SRAM_PDN,
		.sram_pdn_ack_bits = 0,
		.bus_prot_mask = C2K_PROT_MASK,
		.ops = &C2K_sys_ops,
	},
	[SYS_CONN] = {
		.name = __stringify(SYS_CONN),
		.sta_mask = CONN_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_CONN_PWR_CON, */
		.sram_pdn_bits = CONN_SRAM_PDN,
		.sram_pdn_ack_bits = 0,
		.bus_prot_mask = 0,
		.ops = &CONN_sys_ops,
	},
	[SYS_DIS] = {
		.name = __stringify(SYS_DIS),
		.sta_mask = DIS_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_DIS_PWR_CON, */
		.sram_pdn_bits = DIS_SRAM_PDN,
		.sram_pdn_ack_bits = DIS_SRAM_ACK,
		.bus_prot_mask = DIS_PROT_MASK,
		.ops = &DIS_sys_ops,
	},
	[SYS_MFG] = {
		.name = __stringify(SYS_MFG),
		.sta_mask = MFG_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_MFG_PWR_CON, */
		.sram_pdn_bits = MFG_SRAM_PDN,
		.sram_pdn_ack_bits = MFG_SRAM_ACK,
		.bus_prot_mask = MFG_PROT_MASK,
		.ops = &MFG_sys_ops,
	},
	[SYS_ISP] = {
		.name = __stringify(SYS_ISP),
		.sta_mask = ISP_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_ISP_PWR_CON, */
		.sram_pdn_bits = ISP_SRAM_PDN,
		.sram_pdn_ack_bits = ISP_SRAM_ACK,
		.bus_prot_mask = 0,
		.ops = &ISP_sys_ops,
	},
	[SYS_VDE] = {
		.name = __stringify(SYS_VDE),
		.sta_mask = VDE_PWR_STA_MASK,
		/* .ctl_addr = NULL, SPM_VDE_PWR_CON, */
		.sram_pdn_bits = VDE_SRAM_PDN,
		.sram_pdn_ack_bits = VDE_SRAM_ACK,
		.bus_prot_mask = 0,
		.ops = &VDE_sys_ops,
	},
	[SYS_VEN] = {
		.name = __stringify(SYS_VEN),
		.sta_mask = VEN_PWR_STA_MASK,
		/*.ctl_addr = 0, SPM_VEN_PWR_CON, */
		.sram_pdn_bits = VEN_SRAM_PDN,
		.sram_pdn_ack_bits = VEN_SRAM_ACK,
		.bus_prot_mask = 0,
		.ops = &VEN_sys_ops,
	},

	[SYS_AUD] = {
		.name = __stringify(SYS_AUD),
		.sta_mask = AUDIO_PWR_STA_MASK,
		/*.ctl_addr = 0, SPM_VEN_PWR_CON, */
		.sram_pdn_bits = AUDIO_SRAM_PDN,
		.sram_pdn_ack_bits = AUDIO_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ops = &AUD_sys_ops,
	},
#if 0
	[SYS_IFR] = {
		.name = __stringify(SYS_IFR),
		.sta_mask = IFR_PWR_STA_MASK,
		/*.ctl_addr = 0, SPM_VEN_PWR_CON, */
		.sram_pdn_bits = IFR_SRAM_PDN,
		.sram_pdn_ack_bits = IFR_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ops = &IFR_sys_ops,
	},
#endif
};

static struct pg_callbacks *g_pgcb;

struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb)
{
	struct pg_callbacks *old_pgcb = g_pgcb;

	g_pgcb = pgcb;
	return old_pgcb;
}

#define _TOPAXI_TIMEOUT_CNT_ 4000
int spm_topaxi_protect(unsigned int mask_value, int en)
{
	unsigned long flags;
	int count = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;

	do_gettimeofday(&tm_s);
	spm_mtcmos_noncpu_lock(flags);

	if (en == 1) {
		spm_write(INFRA_TOPAXI_PROTECTEN, spm_read(INFRA_TOPAXI_PROTECTEN) | (mask_value));
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & (mask_value)) != (mask_value)) {
			count++;
			if (count > _TOPAXI_TIMEOUT_CNT_)
				break;
		}
	} else {
		spm_write(INFRA_TOPAXI_PROTECTEN, spm_read(INFRA_TOPAXI_PROTECTEN) & ~(mask_value));
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & (mask_value)) {
			count++;
			if (count > _TOPAXI_TIMEOUT_CNT_)
				break;
		}
	}

	spm_mtcmos_noncpu_unlock(flags);

	if (count > _TOPAXI_TIMEOUT_CNT_) {
		do_gettimeofday(&tm_e);
		tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000000 + (tm_e.tv_usec - tm_s.tv_usec);
		pr_err("TOPAXI Bus Protect Timeout Error (%d us)(%d) !!\n", tm_val, count);
		pr_err("mask_value = 0x%x (%d)\n", mask_value, en);
		pr_err("INFRA_TOPAXI_PROTECTEN = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN));
		pr_err("INFRA_TOPAXI_PROTECTSTA1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1));
		pr_err("INFRA_TOPAXI_PROTECTEN_1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN_1));
		pr_err("INFRA_TOPAXI_PROTECTSTA1_1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA1_1));
		pr_err("INFRA_BUS_IDLE_STA1 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA1));
		pr_err("INFRA_BUS_IDLE_STA2 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA2));
		pr_err("INFRA_BUS_IDLE_STA3 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA3));
		pr_err("INFRA_BUS_IDLE_STA4 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA4));
		pr_err("INFRA_BUS_IDLE_STA5 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA5));
		BUG();
	}
	return 0;
}

void reset_infra_md(void)
{
	unsigned long flags;
	int count = 0;
	u32 infra_topaxi_protecten = 0;
	u32 infra_topaxi_protecten_1 = 0;

	spm_mtcmos_noncpu_lock(flags);
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));
/* step.1 */
	infra_topaxi_protecten = spm_read(INFRA_TOPAXI_PROTECTEN);
	infra_topaxi_protecten_1 = spm_read(INFRA_TOPAXI_PROTECTEN_1);
/* step.2 */
	spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) | MD1_PROT_MASK);
	spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) | C2K_PROT_MASK);
/* step.3 */
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_CHECK_MASK) !=
		       MD1_PROT_CHECK_MASK) {
				count++;
				if (count > 2000) {
					pr_err("Check MDSYS bus protect ready TIMEOUT!\r\n");
					break;
				}
		}
		count = 0;
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & C2K_PROT_MASK) != C2K_PROT_MASK) {
			count++;
				if (count > 2000) {
					pr_err("Check C2K bus protect ready TIMEOUT!\r\n");
					break;
				}
		}
/* step.4 */
	spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) | MDSYS_INTF_INFRA_PROT_MASK);
/* step.5 */
		count = 0;
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MDSYS_INTF_INFRA_PROT_MASK) !=
		       MDSYS_INTF_INFRA_PROT_MASK) {
				count++;
				if (count > 2000) {
					pr_err("Check INFRA_MD bus protect ready TIMEOUT!\r\n");
					break;
			}
		}
/* step.6 */
	spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_RST_B);
	spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_RST_B);
/* step.7 */
	spm_write(INFRA_TOPAXI_PROTECTEN, infra_topaxi_protecten);
	spm_write(INFRA_TOPAXI_PROTECTEN_1, infra_topaxi_protecten_1);
	spm_mtcmos_noncpu_unlock(flags);
}

static struct subsys *id_to_sys(unsigned int id)
{
	return id < NR_SYSS ? &syss[id] : NULL;
}

/** sync from mt_spm_mtcmos.c for bring up */
static int spm_mtcmos_ctrl_connsys(int state)
{
	int err = 0;
	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));
	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off CONN" */
		/* TINFO="Set bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) | CONN_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & CONN_PROT_MASK) != CONN_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}
#else
		spm_topaxi_protect(CONN_PROT_MASK, 1);
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & CONN_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & CONN_PWR_STA_MASK))
			;
#endif

		/* TINFO="Finish to turn off CONN" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on CONN" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & CONN_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & CONN_PWR_STA_MASK))
			;
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) & ~CONN_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & CONN_PROT_MASK)
			;
#else
		spm_topaxi_protect(CONN_PROT_MASK, 0);
#endif
		/* TINFO="Finish to turn on CONN" */
	}
	return err;
}

static int spm_mtcmos_ctrl_mdsys1(int state)
{
	int err = 0;
	int count = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MD1" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) | MD1_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_CHECK_MASK) !=
		       MD1_PROT_CHECK_MASK) {
			count++;
			if (count > 1000)
				break;
		}
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | MD1_SRAM_PDN);
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MD1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK))
			;
#endif

		/* TINFO="MD_EXT_BUCK_ISO[0]=1"*/
		spm_write(MD_EXT_BUCK_ISO, spm_read(MD_EXT_BUCK_ISO) | (0x1 << 0));
		/* TINFO="Finish to turn off MD1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MD1" */
		/* TINFO="MD_EXT_BUCK_ISO[0]=0"*/
		spm_write(MD_EXT_BUCK_ISO, spm_read(MD_EXT_BUCK_ISO) & ~(0x1 << 0));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MD1_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK))
			;
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~(0x1 << 8));
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) & ~MD1_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & MD1_PROT_CHECK_MASK)
			;
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MD1" */
	}
	return err;
}

static int spm_mtcmos_ctrl_mdsys2(int state)
{
	int err = 0;
	int count = 0;
	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off C2K" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) | C2K_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & C2K_PROT_MASK) != C2K_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}
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

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & C2K_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & C2K_PWR_STA_MASK))
			;
#endif

		/* TINFO="Finish to turn off C2K" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on C2K" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & C2K_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & C2K_PWR_STA_MASK))
			;
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(C2K_PWR_CON, spm_read(C2K_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1,
			  spm_read(INFRA_TOPAXI_PROTECTEN_1) & ~C2K_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1_1) & C2K_PROT_MASK)
			;
		/* TINFO="Finish to turn on C2K" */
	}
	return err;
}

int spm_mtcmos_ctrl_mdsys_intf_infra2(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
#if 1
		if ((spm_read(PWR_STATUS) & MD1_PWR_STA_MASK)
		    || (spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK))
			return 0;
		if ((spm_read(PWR_STATUS) & C2K_PWR_STA_MASK)
		    || (spm_read(PWR_STATUS_2ND) & C2K_PWR_STA_MASK))
			return 0;
#endif
		/* TINFO="Start to turn off MDSYS_INTF_INFRA" */
		/* TINFO="Set bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) | MDSYS_INTF_INFRA_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MDSYS_INTF_INFRA_PROT_MASK) !=
		       MDSYS_INTF_INFRA_PROT_MASK) {
			count++;
			if (count > 1000)
				break;
		}
#else
		spm_topaxi_protect(MDSYS_INTF_INFRA_PROT_MASK, 1);
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON,
			  spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON,
			  spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON,
			  spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MDSYS_INTF_INFRA_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & MDSYS_INTF_INFRA_PWR_STA_MASK))
			;
#endif

		/* TINFO="Finish to turn off MDSYS_INTF_INFRA" */
	} else {		/* STA_POWER_ON */
#if 0
		if ((spm_read(PWR_STATUS) & MDSYS_INTF_INFRA_PWR_STA_MASK)
		    || (spm_read(PWR_STATUS_2ND) & MDSYS_INTF_INFRA_PWR_STA_MASK))
			return;
#endif
		/* TINFO="Start to turn on MDSYS_INTF_INFRA" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON,
			  spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MDSYS_INTF_INFRA_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & MDSYS_INTF_INFRA_PWR_STA_MASK))
			;
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON,
			  spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MDSYS_INTF_INFRA_PWR_CON, spm_read(MDSYS_INTF_INFRA_PWR_CON) | PWR_RST_B);
		/* TINFO="Release bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) & ~MDSYS_INTF_INFRA_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & MDSYS_INTF_INFRA_PROT_MASK)
			;
#else
		spm_topaxi_protect(MDSYS_INTF_INFRA_PROT_MASK, 0);
#endif
		/* TINFO="Finish to turn on MDSYS_INTF_INFRA" */
	}
	return err;
}

int spm_mtcmos_ctrl_dis(int state)
{
	int err = 0;
	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DIS" */
		/* TINFO="Set bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN, spm_read(INFRA_TOPAXI_PROTECTEN) | DIS_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & DIS_PROT_MASK) != DIS_PROT_MASK)
			;
#else
		spm_topaxi_protect(DIS_PROT_MASK, 1);
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | DIS_SRAM_PDN);
		/* TINFO="Wait until DIS_SRAM_PDN_ACK = 1" */
		while (!(spm_read(DIS_PWR_CON) & DIS_SRAM_PDN_ACK))
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DIS_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & DIS_PWR_STA_MASK))
			;
#endif

		/* TINFO="Finish to turn off DIS" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on DIS" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & DIS_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & DIS_PWR_STA_MASK))
			;
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~(0x1 << 8));
		/* TINFO="Wait until DIS_SRAM_PDN_ACK = 0" */
		while (spm_read(DIS_PWR_CON) & DIS_SRAM_PDN_ACK)
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
		/* TINFO="Release bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) & ~DIS_PROT_MASK);
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & DIS_PROT_MASK)
			;
#else
		spm_topaxi_protect(DIS_PROT_MASK, 0);
#endif
		/* TINFO="Finish to turn on DIS" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg2(int state)
{
	int err = 0;
	int count = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG" */
		/* TINFO="Set bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN, spm_read(INFRA_TOPAXI_PROTECTEN) | MFG_PROT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTSTA1) & MFG_PROT_MASK) != MFG_PROT_MASK) {
			count++;
			if (count > 1000) {
				pr_err("mfgx: , TOPAXI = 0x%x\n",
				       spm_read(INFRA_TOPAXI_PROTECTSTA1));
				break;
			}
		}
#else
		spm_topaxi_protect(MFG_PROT_MASK, 1);
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		count = 0;
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | MFG_SRAM_PDN);
		/* TINFO="Wait until MFG_SRAM_PDN_ACK = 1" */
		while (!(spm_read(MFG_PWR_CON) & MFG_SRAM_PDN_ACK)) {
			count++;
			if (count > 10000) {
				pr_err("mfgx: , MFG_PWR_CON = 0x%08x\n", spm_read(MFG_PWR_CON));
				/*BUG();*/
			}

		}
		/* Need f_fmfg_core_ck for SRAM PDN delay IP. */
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		count = 0;
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			count++;
			if (count > 10000) {
				pr_err("mfgx: , PWR_STATUS = 0x%08x, 0x%08x\n",
				       spm_read(PWR_STATUS), spm_read(PWR_STATUS_2ND));
				/*BUG();*/
			}
		}
#endif

		/* TINFO="Finish to turn off MFG" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on MFG" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MFG_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			count++;

			if (count > 10000) {
				pr_err("mfgo1: PWR_STATUS = 0x%08x, 0x%08x\n",
				       spm_read(PWR_STATUS), spm_read(PWR_STATUS_2ND));
				pr_err("mfgo1: MFG_PWR_CON = 0x%08x\n", spm_read(MFG_PWR_CON));
				/*BUG();*/
			}
		}
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~(0x1 << 8));
		count = 0;
		/* TINFO="Wait until MFG_SRAM_PDN_ACK = 0" */
		while (spm_read(MFG_PWR_CON) & MFG_SRAM_PDN_ACK) {
			/* Need f_fmfg_core_ck for SRAM PDN delay IP. */
			count++;
			if (count > 10000) {
				pr_err("mfgo2: PWR_STATUS = 0x%08x, 0x%08x\n",
				       spm_read(PWR_STATUS), spm_read(PWR_STATUS_2ND));
				pr_err("mfgo2: MFG_PWR_CON = 0x%08x\n", spm_read(MFG_PWR_CON));
				/*BUG();*/
			}
		}
		/* TINFO="Release bus protect" */
#if 0
		spm_write(INFRA_TOPAXI_PROTECTEN,
			  spm_read(INFRA_TOPAXI_PROTECTEN) & ~MFG_PROT_MASK);
		count = 0;
		while (spm_read(INFRA_TOPAXI_PROTECTSTA1) & MFG_PROT_MASK) {
			count++;
			if (count > 2000) {
				pr_err("mfgo: , TOPAXI = 0x%x\n",
				       spm_read(INFRA_TOPAXI_PROTECTSTA1));
				BUG();
			}
		}
#else
		spm_topaxi_protect(MFG_PROT_MASK, 0);
#endif
		/* TINFO="Finish to turn on MFG" */
	}

	return err;
}

int spm_mtcmos_ctrl_isp2(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off ISP" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | ISP_SRAM_PDN);
		/* TINFO="Wait until ISP_SRAM_PDN_ACK = 1" */
		while (!(spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK))
			;
			/* Need hf_fmm_ck for SRAM PDN delay IP. */
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

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & ISP_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & ISP_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Finish to turn off ISP" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on ISP" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & ISP_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & ISP_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~(0x1 << 8));
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~(0x1 << 9));
		/* TINFO="Wait until ISP_SRAM_PDN_ACK = 0" */
		while (spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK)
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
		/* TINFO="Finish to turn on ISP" */
	}
	return err;
}

int spm_mtcmos_ctrl_ifr(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off IFR" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | IFR_SRAM_PDN);
		/* TINFO="Wait until IFR_SRAM_PDN_ACK = 1" */
		while (!(spm_read(IFR_PWR_CON) & IFR_SRAM_PDN_ACK))
			;
		/* SRAM PDN delay IP clock is 26MHz. Print SRAM control and ACK for debug. */
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & IFR_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & IFR_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Finish to turn off IFR" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on IFR" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & IFR_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & IFR_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~(0x1 << 8));
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~(0x1 << 9));
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~(0x1 << 10));
		spm_write(IFR_PWR_CON, spm_read(IFR_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Wait until IFR_SRAM_PDN_ACK = 0" */
		while (spm_read(IFR_PWR_CON) & IFR_SRAM_PDN_ACK)
			;
		/* SRAM PDN delay IP clock is 26MHz. Print SRAM control and ACK for debug. */
		/* TINFO="Finish to turn on IFR" */
	}
	return err;
}

int spm_mtcmos_ctrl_vde(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VDE" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | VDE_SRAM_PDN);
		/* TINFO="Wait until VDE_SRAM_PDN_ACK = 1" */
		while (!(spm_read(VDE_PWR_CON) & VDE_SRAM_PDN_ACK))
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
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

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VDE_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & VDE_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Finish to turn off VDE" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on VDE" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & VDE_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & VDE_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(VDE_PWR_CON, spm_read(VDE_PWR_CON) & ~(0x1 << 8));
		/* TINFO="Wait until VDE_SRAM_PDN_ACK = 0" */
		while (spm_read(VDE_PWR_CON) & VDE_SRAM_PDN_ACK)
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
		/* TINFO="Finish to turn on VDE" */
	}
	return err;
}

int spm_mtcmos_ctrl_ven(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VEN" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | VEN_SRAM_PDN);
		/* TINFO="Wait until VEN_SRAM_PDN_ACK = 1" */
		while (!(spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK))
			;
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
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

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VEN_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & VEN_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Finish to turn off VEN" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on VEN" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & VEN_PWR_STA_MASK)
			|| !(spm_read(PWR_STATUS_2ND) & VEN_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 8));
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 9));
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 10));
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Wait until VEN_SRAM_PDN_ACK = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK)
			;
		/* Need hf_fmm_ck for SRAM PDN delay IP. */
		/* TINFO="Finish to turn on VEN" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg_async(int state)
{
	int err = 0;
	int count = 0;
	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG_ASYNC" */
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_ASYNC_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND) & MFG_ASYNC_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			count++;
			if (count > 2000) {
				pr_err("mfgAx: , PWR_STATUS = 0x%08x, 0x%08x\n",
				       spm_read(PWR_STATUS), spm_read(PWR_STATUS_2ND));
				BUG();
			}
		}
#endif

		/* TINFO="Finish to turn off MFG_ASYNC" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on MFG_ASYNC" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & MFG_ASYNC_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & MFG_ASYNC_PWR_STA_MASK)) {
			/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
			count++;
			if (count > 2000) {
				pr_err("mfgAo: , PWR_STATUS = 0x%08x, 0x%08x\n",
				       spm_read(PWR_STATUS), spm_read(PWR_STATUS_2ND));
				BUG();
			}
		}
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_RST_B);
		/* TINFO="Finish to turn on MFG_ASYNC" */
	}
	return err;
}

int spm_mtcmos_ctrl_audio(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off AUDIO" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | AUDIO_SRAM_PDN);
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK = 1" */
		while (!(spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK))
			;
		/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & AUDIO_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & AUDIO_PWR_STA_MASK))
				;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */

#endif

		/* TINFO="Finish to turn off AUDIO" */
	} else {		/* STA_POWER_ON */
		/* TINFO="Start to turn on AUDIO" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_ON_2ND);

#ifndef IGNORE_PWR_ACK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (!(spm_read(PWR_STATUS) & AUDIO_PWR_STA_MASK)
		       || !(spm_read(PWR_STATUS_2ND) & AUDIO_PWR_STA_MASK))
			;
/* No logic between pwr_on and pwr_ack. Print SRAM / MTCMOS control and PWR_ACK for debug. */
#endif

		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 8));
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 9));
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 10));
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 11));
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK = 0" */
		while (spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK)
			;
		/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
		/* TINFO="Finish to turn on AUDIO" */
	}
	return err;
}
#if 0
static void set_bus_protect(int en, uint32_t mask, unsigned long expired)
{
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: en=%d, mask=%u, expired=%lu: S\n", __func__,
		 en, mask, expired);
#endif /* MT_CCF_DEBUG */
	if (!mask)
		return;

	if (en) {
		clk_setl(mask, INFRA_TOPAXI_PROTECTEN);

#if !DUMMY_REG_TEST
		while ((clk_readl(INFRA_TOPAXI_PROTECTSTA1) & mask) != mask) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
#endif /* !DUMMY_REG_TEST */
	} else {
		clk_clrl(mask, INFRA_TOPAXI_PROTECTEN);

#if !DUMMY_REG_TEST
		while (clk_readl(INFRA_TOPAXI_PROTECTSTA1) & mask) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
#endif /* !DUMMY_REG_TEST */
	}
}

static int spm_mtcmos_power_off_general_locked(struct subsys *sys,
		int wait_power_ack, int ext_pwr_delay)
{
	unsigned long expired = jiffies + HZ / 10;
	void __iomem *ctl_addr = sys->ctl_addr;

/*	pr_debug("[CCF] %s: sys=%s, wait_power_ack=%d, ext_pwr_delay=%d\n",
		 __func__, sys->name, wait_power_ack, ext_pwr_delay);*/

	/* BUS_PROTECT */
	if (sys->bus_prot_mask)
		set_bus_protect(1, sys->bus_prot_mask, expired);

	/* SRAM_PDN */
	clk_setl(sys->sram_pdn_bits, ctl_addr);

	/* wait until SRAM_PDN_ACK all 1 */
#if !DUMMY_REG_TEST
	if (sys->sram_pdn_ack_bits) {
		while (((clk_readl(ctl_addr) & sys->sram_pdn_ack_bits) != sys->sram_pdn_ack_bits)) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
	}
#endif /* !DUMMY_REG_TEST */

	clk_setl(PWR_ISO_BIT, ctl_addr);
	clk_clrl(PWR_RST_B_BIT, ctl_addr);
	clk_setl(PWR_CLK_DIS_BIT, ctl_addr);

	clk_clrl(PWR_ON_BIT, ctl_addr);
	clk_clrl(PWR_ON_2ND_BIT, ctl_addr);

	/* extra delay after power off */
	if (ext_pwr_delay > 0)
		udelay(ext_pwr_delay);

	if (wait_power_ack) {
		/* wait until PWR_ACK = 0 */
#if !DUMMY_REG_TEST
		while ((clk_readl(PWR_STATUS) & sys->sta_mask)
			|| (clk_readl(PWR_STATUS_2ND) & sys->sta_mask)) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
#endif /* !DUMMY_REG_TEST */
	}

	return 0;
}

static int spm_mtcmos_power_on_general_locked(
		struct subsys *sys, int wait_power_ack, int ext_pwr_delay)
{
	unsigned long expired = jiffies + HZ / 10;
	void __iomem *ctl_addr = sys->ctl_addr;

/*	pr_debug("[CCF] %s: sys=%s, wait_power_ack=%d, ext_pwr_delay=%d\n",
		 __func__, sys->name, wait_power_ack, ext_pwr_delay);*/

	clk_setl(PWR_ON_BIT, ctl_addr);
	clk_setl(PWR_ON_2ND_BIT, ctl_addr);

	/* extra delay after power on */
	if (ext_pwr_delay > 0)
		udelay(ext_pwr_delay);

	if (wait_power_ack) {
		/* wait until PWR_ACK = 1 */
#if !DUMMY_REG_TEST
		while (!(clk_readl(PWR_STATUS) & sys->sta_mask)
			|| !(clk_readl(PWR_STATUS_2ND) & sys->sta_mask)) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
#endif /* !DUMMY_REG_TEST */
	}

	clk_clrl(PWR_CLK_DIS_BIT, ctl_addr);
	clk_clrl(PWR_ISO_BIT, ctl_addr);
	clk_setl(PWR_RST_B_BIT, ctl_addr);

	/* SRAM_PDN */
	clk_clrl(sys->sram_pdn_bits, ctl_addr);

	/* wait until SRAM_PDN_ACK all 0 */
#if !DUMMY_REG_TEST
	if (sys->sram_pdn_ack_bits) {
		while (sys->sram_pdn_ack_bits && (clk_readl(ctl_addr) & sys->sram_pdn_ack_bits)) {
			if (time_after(jiffies, expired)) {
				WARN_ON(1);
				break;
			}
		}
	}
#endif /* !DUMMY_REG_TEST */

	/* BUS_PROTECT */
	if (sys->bus_prot_mask)
		set_bus_protect(0, sys->bus_prot_mask, expired);

	return 0;
}
#endif
/*static int general_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_power_on_general_locked(sys, 1, 0);
}

static int general_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_power_off_general_locked(sys, 1, 0);
}*/

static int MD1_sys_enable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_ON);
	return spm_mtcmos_ctrl_mdsys1(STA_POWER_ON);
}
static int C2K_sys_enable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_ON);
	return spm_mtcmos_ctrl_mdsys2(STA_POWER_ON);
}
static int CONN_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_connsys(STA_POWER_ON);
}
static int MFG_sys_enable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mfg_async(STA_POWER_ON);
	return spm_mtcmos_ctrl_mfg2(STA_POWER_ON);
}
static int DIS_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_dis(STA_POWER_ON);
}
static int ISP_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_isp2(STA_POWER_ON);
}
static int VDE_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_vde(STA_POWER_ON);
}
static int VEN_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_ven(STA_POWER_ON);
}

static int AUD_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_audio(STA_POWER_ON);
}
#if 0
static int IFR_sys_enable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_ifr(STA_POWER_ON);
}
#endif
static int MD1_sys_disable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mdsys1(STA_POWER_DOWN);
	return spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_DOWN);
}
static int C2K_sys_disable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mdsys2(STA_POWER_DOWN);
	return spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_DOWN);
}
static int CONN_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_connsys(STA_POWER_DOWN);
}
static int MFG_sys_disable_op(struct subsys *sys)
{
	spm_mtcmos_ctrl_mfg2(STA_POWER_DOWN);
	return spm_mtcmos_ctrl_mfg_async(STA_POWER_DOWN);
	/*return 0;*/
}
static int DIS_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_dis(STA_POWER_DOWN);
	/*return 0;*/
}
static int ISP_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_isp2(STA_POWER_DOWN);
}
static int VDE_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_vde(STA_POWER_DOWN);
}
static int VEN_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_ven(STA_POWER_DOWN);
}

static int AUD_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_audio(STA_POWER_DOWN);
}
#if 0
static int IFR_sys_disable_op(struct subsys *sys)
{
	return spm_mtcmos_ctrl_ifr(STA_POWER_DOWN);
}
#endif
static int sys_get_state_op(struct subsys *sys)
{
	unsigned int sta = clk_readl(PWR_STATUS);
	unsigned int sta_s = clk_readl(PWR_STATUS_2ND);

	return (sta & sys->sta_mask) && (sta_s & sys->sta_mask);
}

/*static struct subsys_ops general_sys_ops = {
	.enable = general_sys_enable_op,
	.disable = general_sys_disable_op,
	.get_state = sys_get_state_op,
};*/

static struct subsys_ops MD1_sys_ops = {
	.enable = MD1_sys_enable_op,
	.disable = MD1_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops C2K_sys_ops = {
	.enable = C2K_sys_enable_op,
	.disable = C2K_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops CONN_sys_ops = {
	.enable = CONN_sys_enable_op,
	.disable = CONN_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG_sys_ops = {
	.enable = MFG_sys_enable_op,
	.disable = MFG_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops DIS_sys_ops = {
	.enable = DIS_sys_enable_op,
	.disable = DIS_sys_disable_op,
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
static struct subsys_ops AUD_sys_ops = {
	.enable = AUD_sys_enable_op,
	.disable = AUD_sys_disable_op,
	.get_state = sys_get_state_op,
};
/*static struct subsys_ops IFR_sys_ops = {
	.enable = IFR_sys_enable_op,
	.disable = IFR_sys_disable_op,
	.get_state = sys_get_state_op,
};*/


static int subsys_is_on(enum subsys_id id)
{
	int r;
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!sys);

	r = sys->ops->get_state(sys);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s:%d, sys=%s, id=%d\n", __func__, r, sys->name, id);
#endif /* MT_CCF_DEBUG */

	return r;
}

static int enable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!sys);

#if 0				/*MT_CCF_BRINGUP */
/*	pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_MD1:
#ifdef VLTE_SUPPORT /* Workaround is handled by ccci */
	/* mt_set_gpio_out(GPIO_LTE_VSRAM_EXT_POWER_EN_PIN,1);
	pmic_config_interface(0x04D6, 0x1, 0x1, 0);
	udelay(200); */
#endif /* VLTE_SUPPORT */
		spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_ON);
		spm_mtcmos_ctrl_mdsys1(STA_POWER_ON);
		break;
	case SYS_MD2:
		spm_mtcmos_ctrl_mdsys_intf_infra2(STA_POWER_ON);
		spm_mtcmos_ctrl_mdsys2(STA_POWER_ON);
		break;
	case SYS_CONN:
		spm_mtcmos_ctrl_connsys(STA_POWER_ON);
		break;
	default:
		break;
	}
	return 0;
#endif /* MT_CCF_BRINGUP */

	mtk_clk_lock(flags);

#if CHECK_PWR_ST
	if (sys->ops->get_state(sys) == SUBSYS_PWR_ON) {
		mtk_clk_unlock(flags);
		return 0;
	}
#endif /* CHECK_PWR_ST */

	r = sys->ops->enable(sys);
	WARN_ON(r);

	mtk_clk_unlock(flags);

	if (g_pgcb && g_pgcb->after_on)
		g_pgcb->after_on(id);

	return r;
}

static int disable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	BUG_ON(!sys);

#if 0				/*MT_CCF_BRINGUP */
/*	pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_DIS:
		return 0;
	default:
		break;
	}
#endif /* MT_CCF_BRINGUP */

	/* TODO: check all clocks related to this subsys are off */
	/* could be power off or not */

	if (g_pgcb && g_pgcb->before_off)
		g_pgcb->before_off(id);

	mtk_clk_lock(flags);

#if CHECK_PWR_ST
	if (sys->ops->get_state(sys) == SUBSYS_PWR_DOWN) {
		mtk_clk_unlock(flags);
		return 0;
	}
#endif /* CHECK_PWR_ST */

	r = sys->ops->disable(sys);
	WARN_ON(r);

	mtk_clk_unlock(flags);

	return r;
}

/*
 * power_gate
 */

struct mt_power_gate {
	struct clk_hw	hw;
	struct clk	*pre_clk;
	enum subsys_id	pd_id;
};

#define to_power_gate(_hw) container_of(_hw, struct mt_power_gate, hw)

static int pg_enable(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, pd_id=%u\n", __func__,
		 __clk_get_name(hw->clk), pg->pd_id);
#endif /* MT_CCF_DEBUG */

	return enable_subsys(pg->pd_id);
}

static void pg_disable(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: sys=%s, pd_id=%u\n", __func__,
		 __clk_get_name(hw->clk), pg->pd_id);
#endif /* MT_CCF_DEBUG */

	disable_subsys(pg->pd_id);
}

static int pg_is_enabled(struct clk_hw *hw)
{
	struct mt_power_gate *pg = to_power_gate(hw);
#if 0				/*MT_CCF_BRINGUP */
	return 1;
#endif /* MT_CCF_BRINGUP */

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
#endif /* MT_CCF_DEBUG */

	if (pg->pre_clk) {
		r = clk_prepare_enable(pg->pre_clk);
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
#endif /* MT_CCF_DEBUG */

	pg_disable(hw);

	if (pg->pre_clk)
		clk_disable_unprepare(pg->pre_clk);
}

static const struct clk_ops mt_power_gate_ops = {
	.prepare	= pg_prepare,
	.unprepare	= pg_unprepare,
	.is_enabled	= pg_is_enabled,
};

struct clk *mt_clk_register_power_gate(
		const char *name,
		const char *parent_name,
		struct clk *pre_clk,
		enum subsys_id pd_id)
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
	pg->pd_id = pd_id;
	pg->hw.init = &init;

	clk = clk_register(NULL, &pg->hw);
	if (IS_ERR(clk))
		kfree(pg);

	return clk;
}

#define pg_md1			"pg_md1"
#define pg_md2			"pg_md2"
#define pg_conn			"pg_conn"
#define pg_dis			"pg_dis"
#define pg_mfg			"pg_mfg"
#define pg_isp			"pg_isp"
#define pg_vde			"pg_vde"
#define pg_ven			"pg_ven"
#define pg_aud			"pg_aud"
#define pg_ifr			"pg_ifr"

/* FIXME: set correct value: S */
#define md_sel			"md_sel"
#define conn_sel			"conn_sel"
#define mm_sel			"mm_sel"
#define vdec_sel		"vdec_sel"
#define venc_sel		"venc_sel"
#define mfg_sel			"mfg_sel"
/* FIXME: set correct value: E */

struct mtk_power_gate {
	int id;
	const char *name;
	const char *parent_name;
	const char *pre_clk_name;
	enum subsys_id pd_id;
};

#define PGATE(_id, _name, _parent, _pre_clk, _pd_id) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.pre_clk_name = _pre_clk,		\
		.pd_id = _pd_id,			\
	}

/* FIXME: all values needed to be verified */
struct mtk_power_gate scp_clks[] __initdata = {
	PGATE(SCP_SYS_MD1, pg_md1, NULL, NULL, SYS_MD1),
	PGATE(SCP_SYS_MD2, pg_md2, NULL, NULL, SYS_MD2),
	PGATE(SCP_SYS_CONN, pg_conn, NULL, NULL, SYS_CONN),
	PGATE(SCP_SYS_DIS, pg_dis, NULL, mm_sel, SYS_DIS),
	PGATE(SCP_SYS_MFG, pg_mfg, NULL, mfg_sel, SYS_MFG),
	PGATE(SCP_SYS_ISP, pg_isp, NULL, NULL, SYS_ISP),
	PGATE(SCP_SYS_VDE, pg_vde, NULL, vdec_sel, SYS_VDE),
	PGATE(SCP_SYS_VEN, pg_ven, NULL, NULL, SYS_VEN),
	PGATE(SCP_SYS_AUD, pg_aud, NULL, NULL, SYS_AUD),
};

static void __init init_clk_scpsys(
		void __iomem *infracfg_reg,
		void __iomem *spm_reg,
		void __iomem *infra_reg,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;
	struct clk *pre_clk;

	infracfg_base = infracfg_reg;
	spm_base = spm_reg;
	infra_base = infra_reg;

	syss[SYS_MD1].ctl_addr = MD1_PWR_CON;
	syss[SYS_MD2].ctl_addr = C2K_PWR_CON;
	syss[SYS_CONN].ctl_addr = CONN_PWR_CON;
	syss[SYS_DIS].ctl_addr = DIS_PWR_CON;
	syss[SYS_MFG].ctl_addr = MFG_PWR_CON;
	syss[SYS_ISP].ctl_addr = ISP_PWR_CON;
	syss[SYS_VDE].ctl_addr = VDE_PWR_CON;
	syss[SYS_VEN].ctl_addr = VEN_PWR_CON;

	for (i = 0; i < ARRAY_SIZE(scp_clks); i++) {
		struct mtk_power_gate *pg = &scp_clks[i];

		pre_clk = pg->pre_clk_name ?
				__clk_lookup(pg->pre_clk_name) : NULL;

		clk = mt_clk_register_power_gate(pg->name, pg->parent_name,
				pre_clk, pg->pd_id);

		if (IS_ERR(clk)) {
			pr_err("[CCF] %s: Failed to register clk %s: %ld\n",
					__func__, pg->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[pg->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] %s: pgate %3d: %s\n", __func__, i, pg->name);
#endif /* MT_CCF_DEBUG */
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
	int r;
	void __iomem *infra_reg;
	struct device_node *node2;

	node2 = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
	if (!node2)
		pr_err("[infracfg] find node failed\n");
	infra_reg = of_iomap(node, 0);
	if (!infra_reg)
		pr_err("[infracfg] base failed\n");

	infracfg_reg = get_reg(node, 0);
	spm_reg = get_reg(node, 1);

	if (!infracfg_reg || !spm_reg) {
		pr_err("clk-pg-mt6755: missing reg\n");
		return;
	}

/*	pr_debug("[CCF] %s: sys: %s, reg: 0x%p, 0x%p\n",
		__func__, node->name, infracfg_reg, spm_reg);*/

	clk_data = alloc_clk_data(SCP_NR_SYSS);

	init_clk_scpsys(infracfg_reg, spm_reg, infra_reg, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("[CCF] %s:could not register clock provide\n", __func__);

	/* subsys init: per modem owner request, disable modem power first */
	disable_subsys(SYS_MD1);
	disable_subsys(SYS_MD2);
	spm_mtcmos_ctrl_mfg_async(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg2(STA_POWER_ON);
}
CLK_OF_DECLARE(mtk_pg_regs, "mediatek,mt6755-scpsys", mt_scpsys_init);
void subsys_if_on(void)
{
		unsigned int sta = spm_read(PWR_STATUS);
		unsigned int sta_s = spm_read(PWR_STATUS_2ND);

		if ((sta & (1U << 0)) && (sta_s & (1U << 0)))
			pr_err("suspend warning: SYS_MD1 is on!!!\n");

		if ((sta & (1U << 1)) && (sta_s & (1U << 1)))
			pr_err("suspend warning: SYS_CONN is on!!!\n");

		if ((sta & (1U << 3)) && (sta_s & (1U << 3)))
			pr_err("suspend warning: SYS_DIS is on!!!\n");

		if ((sta & (1U << 4)) && (sta_s & (1U << 4)))
			pr_err("suspend warning: SYS_MFG is on!!!\n");

		if ((sta & (1U << 5)) && (sta_s & (1U << 5)))
			pr_err("suspend warning: SYS_ISP is on!!!\n");

		if ((sta & (1U << 7)) && (sta_s & (1U << 7)))
			pr_err("suspend warning: SYS_VDE is on!!!\n");

		if ((sta & (1U << 21)) && (sta_s & (1U << 21)))
			pr_err("suspend warning: SYS_VEN is on!!!\n");

		if ((sta & (1U << 24)) && (sta_s & (1U << 24)))
			pr_err("suspend warning: SYS_AUD is on!!!\n");

		if ((sta & (1U << 23)) && (sta_s & (1U << 23)))
			pr_err("suspend warning: SYS_MFG_ASYNC is on!!!\n");

		if ((sta & (1U << 28)) && (sta_s & (1U << 28)))
			pr_err("suspend warning: SYS_C2K is on!!!\n");
}

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
		__clk_get_enable_count(c),
		__clk_get_rate(c),
		p ? __clk_get_name(p) : "");

		clk_put(c);
}

static int test_pg_dump_state_all(struct seq_file *s, void *v)
{
	static const char * const clks[] = {
		pg_md1,
		pg_md2,
		pg_conn,
		pg_dis,
		pg_mfg,
		pg_isp,
		pg_vde,
		pg_ven,
	};

	int i;

/*	pr_debug("\n");*/
	for (i = 0; i < ARRAY_SIZE(clks); i++)
		dump_pg_state(clks[i], s);

	return 0;
}

static struct {
	const char	*name;
	struct clk	*clk;
} g_clks[] = {
	{.name = pg_md1},
	{.name = pg_vde},
	{.name = pg_ven},
	{.name = pg_mfg},
};

static int test_pg_1(struct seq_file *s, void *v)
{
	int i;

/*	pr_debug("\n");*/

	for (i = 0; i < ARRAY_SIZE(g_clks); i++) {
		g_clks[i].clk = __clk_lookup(g_clks[i].name);
		if (IS_ERR_OR_NULL(g_clks[i].clk)) {
			seq_printf(s, "clk_get(%s): NULL\n",
				g_clks[i].name);
			continue;
		}

		clk_prepare_enable(g_clks[i].clk);
		seq_printf(s, "clk_prepare_enable(%s)\n",
			__clk_get_name(g_clks[i].clk));
	}

	return 0;
}

static int test_pg_2(struct seq_file *s, void *v)
{
	int i;

/*	pr_debug("\n");*/

	for (i = 0; i < ARRAY_SIZE(g_clks); i++) {
		if (IS_ERR_OR_NULL(g_clks[i].clk)) {
			seq_printf(s, "(%s).clk: NULL\n",
				g_clks[i].name);
			continue;
		}

		seq_printf(s, "clk_disable_unprepare(%s)\n",
			__clk_get_name(g_clks[i].clk));
		clk_disable_unprepare(g_clks[i].clk);
		clk_put(g_clks[i].clk);
	}

	return 0;
}

static int test_pg_show(struct seq_file *s, void *v)
{
	static const struct {
		int (*fn)(struct seq_file *, void *);
		const char	*cmd;
	} cmds[] = {
		{.cmd = "dump_regs",	.fn = test_pg_dump_regs},
		{.cmd = "dump_state",	.fn = test_pg_dump_state_all},
		{.cmd = "1",		.fn = test_pg_1},
		{.cmd = "2",		.fn = test_pg_2},
	};

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

static ssize_t test_pg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
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
	.owner		= THIS_MODULE,
	.open		= test_pg_open,
	.read		= seq_read,
	.write		= test_pg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
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

#endif /* CLK_DEBUG */
