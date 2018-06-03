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
#include "clk-mt6763-pg.h"

#include <dt-bindings/clock/mt6763-clk.h>

/*#define TOPAXI_PROTECT_LOCK*/

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

#define mt_reg_sync_writel(v, a) \
	do { \
		__raw_writel((v), IOMEM(a)); \
		mb(); } \
while (0)
#define spm_read(addr)			__raw_readl(IOMEM(addr))
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_readl(addr)			__raw_readl(IOMEM(addr))

/*MM Bus*/
#ifdef CONFIG_OF
void __iomem *clk_mmsys_config_base;
void __iomem *clk_imgsys_base;
void __iomem *clk_vdec_gcon_base;
void __iomem *clk_venc_gcon_base;
void __iomem *clk_camsys_base;
#endif

#if 0
#define MM_CG_CLR0 (clk_mmsys_config_base + 0x108)
#define IMG_CG_CLR	(clk_imgsys_base + 0x0008)
#define VDEC_CKEN_SET	(clk_vdec_gcon_base + 0x0000)
#define VDEC_GALS_CFG (clk_vdec_gcon_base + 0x0168)
#define VENC_CG_SET	(clk_venc_gcon_base + 0x0004)
#endif
#define CAMSYS_CG_STA	(clk_camsys_base + 0x0000)
#define CAMSYS_CG_CLR	(clk_camsys_base + 0x0008)


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
static struct subsys_ops CONN_sys_ops;
static struct subsys_ops DIS_sys_ops;
static struct subsys_ops MFG_sys_ops;
static struct subsys_ops ISP_sys_ops;
static struct subsys_ops VEN_sys_ops;
static struct subsys_ops MFG_ASYNC_sys_ops;
static struct subsys_ops AUDIO_sys_ops;
static struct subsys_ops CAM_sys_ops;
static struct subsys_ops MFG_CORE1_sys_ops;
static struct subsys_ops MFG_CORE0_sys_ops;


static void __iomem *infracfg_base;/*infracfg_ao*/
static void __iomem *spm_base;
static void __iomem *infra_base;/*infracfg*/
static void __iomem *ckgen_base;/*ckgen*/
static void __iomem *smi_larb2_base;
static void __iomem *smi_common_base;

#define INFRACFG_REG(offset)		(infracfg_base + offset)
#define SPM_REG(offset)			(spm_base + offset)
#define INFRA_REG(offset)	(infra_base + offset)
#define CKGEN_REG(offset)	(ckgen_base + offset)
#define SMI_LARB2_REG(offset)	(smi_larb2_base + offset)
#define SMI_COMMON_REG(offset)	(smi_common_base + offset)
/**************************************
 * for non-CPU MTCMOS
 **************************************/
#ifdef TOPAXI_PROTECT_LOCK
static DEFINE_SPINLOCK(spm_noncpu_lock);
#define spm_mtcmos_noncpu_lock(flags)   spin_lock_irqsave(&spm_noncpu_lock, flags)
#define spm_mtcmos_noncpu_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_lock, flags)

static DEFINE_SPINLOCK(spm_noncpu_md_lock);
#define spm_mtcmos_noncpu_md_lock(flags)   spin_lock_irqsave(&spm_noncpu_md_lock, flags)
#define spm_mtcmos_noncpu_md_unlock(flags) spin_unlock_irqrestore(&spm_noncpu_md_lock, flags)
#endif

#if 1
#define POWERON_CONFIG_EN			SPM_REG(0x0000)
#define PWR_STATUS       SPM_REG(0x0180)
#define PWR_STATUS_2ND       SPM_REG(0x0184)
#define MD_EXTRA_PWR_CON       SPM_REG(0x0398)
#define MD1_PWR_CON       SPM_REG(0x0320)
#define MD_SRAM_ISO_CON     SPM_REG(0x0394)
#define CONN_PWR_CON       SPM_REG(0x032C)
#define DPY_PWR_CON       SPM_REG(0x031C)
#define DIS_PWR_CON       SPM_REG(0x030C)
#define MFG_PWR_CON       SPM_REG(0x0338)
#define ISP_PWR_CON       SPM_REG(0x0308)
#define VEN_PWR_CON       SPM_REG(0x0304)
#define MFG_ASYNC_PWR_CON     SPM_REG(0x0334)
#define AUDIO_PWR_CON      SPM_REG(0x314)
#define CAM_PWR_CON       SPM_REG(0x344)
#define MFG_CORE1_PWR_CON       SPM_REG(0x340)
#define MFG_CORE0_PWR_CON       SPM_REG(0x33C)
#define MBIST_EFUSE_REPAIR_ACK_STA       SPM_REG(0x3D0)

#define INFRA_TOPAXI_SI0_CTL	INFRACFG_REG(0x0200)
#define INFRA_TOPAXI_PROTECTEN		INFRACFG_REG(0x0220)
#define INFRA_TOPAXI_PROTECTSTA0	INFRACFG_REG(0x0224)
#define INFRA_TOPAXI_PROTECTEN_STA1	INFRACFG_REG(0x0228)
#define INFRA_TOPAXI_PROTECTEN_1   INFRACFG_REG(0x0250)
#define INFRA_TOPAXI_PROTECTSTA0_1 INFRACFG_REG(0x0254)
#define INFRA_TOPAXI_PROTECTEN_STA1_1 INFRACFG_REG(0x0258)

#define INFRA_TOPAXI_PROTECTEN_SET	INFRACFG_REG(0x02A0)
#define INFRA_TOPAXI_PROTECTEN_CLR		INFRACFG_REG(0x02A4)
#define INFRA_TOPAXI_PROTECTEN_1_SET	INFRACFG_REG(0x02A8)
#define INFRA_TOPAXI_PROTECTEN_1_CLR	INFRACFG_REG(0x02AC)



/* INFRACFG */
#define INFRA_TOPAXI_SI0_STA		INFRA_REG(0x0000)
#define INFRA_BUS_IDLE_STA5	INFRA_REG(0x0190)
/* SMI LARB */
#define SMI_LARB_STAT	SMI_LARB2_REG(0x0000)
#define SMI_LARB2_SLP_CON    SMI_LARB2_REG(0x000C)
/* SMI COMMON */
#define SMI_COMMON_SMI_CLAMP_SET    SMI_COMMON_REG(0x03C4)
#define SMI_COMMON_SMI_CLAMP_CLR    SMI_COMMON_REG(0x03C8)


#define CLK_MODE	CKGEN_REG(0x0000)
#endif

#define  SPM_PROJECT_CODE    0xB16

/* Define MBIST EFUSE control */

#define INFRA_AO_EFUSE_S2P_RX_DONE_BIT   (0x1 << 3)
#define INFRA_PWR_EFUSE_S2P_RX_DONE_BIT  (0x1 << 2)
#define CAM_EFUSE_S2P_RX_DONE_BIT        (0x1 << 1)
#define MFG_EFUSE_S2P_RX_DONE_BIT        (0x1 << 0)

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
#define MD1_PROT_BIT_MASK                ((0x1 << 3) \
					  |(0x1 << 4) \
					  |(0x1 << 7))
#define MD1_PROT_BIT_ACK_MASK            ((0x1 << 3) \
					  |(0x1 << 4) \
					  |(0x1 << 7))
#define MD1_PROT_BIT_2ND_MASK            ((0x1 << 6))
#define MD1_PROT_BIT_ACK_2ND_MASK        ((0x1 << 6))
#define CONN_PROT_BIT_MASK               ((0x1 << 13) \
					  |(0x1 << 14))
#define CONN_PROT_BIT_ACK_MASK           ((0x1 << 13) \
					  |(0x1 << 14))
#define DIS_PROT_BIT_MASK                ((0x1 << 1) \
					  |(0x1 << 2))
#define DIS_PROT_BIT_ACK_MASK            ((0x1 << 1) \
					  |(0x1 << 2))
#define MFG_PROT_BIT_MASK                ((0x1 << 19) \
					  |(0x1 << 20) \
					  |(0x1 << 21))
#define MFG_PROT_BIT_ACK_MASK            (0x1 << 21)
#define CTRL_UPDATE_BIT                  ((0x1 << 24))
#define WR_RD_OT_BUSY_BIT                ((0x1 << 15) \
					  |(0x1 << 14))
#define WR_OT_BUSY_BIT                   ((0x1 << 15))
#define RD_OT_BUSY_BIT                   ((0x1 << 14))
#define MFG_PROT_BIT_2ND_MASK            ((0x1 << 21) \
					  |(0x1 << 22))
#define MFG_PROT_BIT_ACK_2ND_MASK        ((0x1 << 21) \
					  |(0x1 << 22))

/* Define MTCMOS Power Status Mask */

#define MD1_PWR_STA_MASK                 (0x1 << 0)
#define CONN_PWR_STA_MASK                (0x1 << 1)
#define DIS_PWR_STA_MASK                 (0x1 << 3)
#define MFG_PWR_STA_MASK                 (0x1 << 4)
#define ISP_PWR_STA_MASK                 (0x1 << 5)
#define VEN_PWR_STA_MASK                 (0x1 << 21)
#define MFG_ASYNC_PWR_STA_MASK           (0x1 << 23)
#define AUDIO_PWR_STA_MASK               (0x1 << 24)
#define CAM_PWR_STA_MASK                 (0x1 << 27)
#define MFG_CORE1_PWR_STA_MASK           (0x1 << 30)
#define MFG_CORE0_PWR_STA_MASK           (0x1 << 31)

/* Define Non-CPU SRAM Mask */
#define MD1_SRAM_PDN                     (0x1 << 8)
#define MD1_SRAM_PDN_ACK                 (0x0 << 12)
#define MD1_SRAM_PDN_ACK_BIT0            (0x1 << 12)
#define DIS_SRAM_PDN                     (0x1 << 8)
#define DIS_SRAM_PDN_ACK                 (0x1 << 12)
#define DIS_SRAM_PDN_ACK_BIT0            (0x1 << 12)
#define MFG_SRAM_PDN                     (0x3 << 8)
#define MFG_SRAM_PDN_ACK                 (0x3 << 16)
#define MFG_SRAM_PDN_ACK_BIT0            (0x1 << 16)
#define MFG_SRAM_PDN_ACK_BIT1            (0x1 << 17)
#define ISP_SRAM_PDN                     (0x3 << 8)
#define ISP_SRAM_PDN_ACK                 (0x3 << 12)
#define ISP_SRAM_PDN_ACK_BIT0            (0x1 << 12)
#define ISP_SRAM_PDN_ACK_BIT1            (0x1 << 13)
#define VEN_SRAM_PDN                     (0xF << 8)
#define VEN_SRAM_PDN_ACK                 (0xF << 12)
#define VEN_SRAM_PDN_ACK_BIT0            (0x1 << 12)
#define VEN_SRAM_PDN_ACK_BIT1            (0x1 << 13)
#define VEN_SRAM_PDN_ACK_BIT2            (0x1 << 14)
#define VEN_SRAM_PDN_ACK_BIT3            (0x1 << 15)
#define AUDIO_SRAM_PDN                   (0xF << 8)
#define AUDIO_SRAM_PDN_ACK               (0xF << 12)
#define AUDIO_SRAM_PDN_ACK_BIT0          (0x1 << 12)
#define AUDIO_SRAM_PDN_ACK_BIT1          (0x1 << 13)
#define AUDIO_SRAM_PDN_ACK_BIT2          (0x1 << 14)
#define AUDIO_SRAM_PDN_ACK_BIT3          (0x1 << 15)
#define CAM_SRAM_PDN                     (0x3 << 8)
#define CAM_SRAM_PDN_ACK                 (0x3 << 12)
#define CAM_SRAM_PDN_ACK_BIT0            (0x1 << 12)
#define CAM_SRAM_PDN_ACK_BIT1            (0x1 << 13)
#define MFG_CORE1_SRAM_PDN               (0x1 << 5)
#define MFG_CORE1_SRAM_PDN_ACK           (0x1 << 6)
#define MFG_CORE1_SRAM_PDN_ACK_BIT0      (0x1 << 6)
#define MFG_CORE0_SRAM_PDN               (0x1 << 5)
#define MFG_CORE0_SRAM_PDN_ACK           (0x1 << 6)
#define MFG_CORE0_SRAM_PDN_ACK_BIT0      (0x1 << 6)



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
	[SYS_CONN] = {
		      .name = __stringify(SYS_CONN),
		      .sta_mask = CONN_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		      .sram_pdn_bits = 0,
		      .sram_pdn_ack_bits = 0,
		      .bus_prot_mask = 0,
		      .ops = &CONN_sys_ops,
		      },
	[SYS_DIS] = {
		     .name = __stringify(SYS_DIS),
		     .sta_mask = DIS_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &DIS_sys_ops,
		     },
	[SYS_MFG] = {
		     .name = __stringify(SYS_MFG),
		     .sta_mask = MFG_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &MFG_sys_ops,
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
	[SYS_VEN] = {
		     .name = __stringify(SYS_VEN),
		     .sta_mask = VEN_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
		     .sram_pdn_bits = 0,
		     .sram_pdn_ack_bits = 0,
		     .bus_prot_mask = 0,
		     .ops = &VEN_sys_ops,
		     },
	[SYS_MFG_ASYNC] = {
			   .name = __stringify(SYS_MFG_ASYNC),
			   .sta_mask = MFG_ASYNC_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &MFG_ASYNC_sys_ops,
			   },
	[SYS_AUDIO] = {
		       .name = __stringify(SYS_AUDIO),
		       .sta_mask = AUDIO_PWR_STA_MASK,
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
	[SYS_MFG_CORE1] = {
			   .name = __stringify(SYS_MFG_CORE1),
			   .sta_mask = MFG_CORE1_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &MFG_CORE1_sys_ops,
			   },
	[SYS_MFG_CORE0] = {
			   .name = __stringify(SYS_MFG_CORE0),
			   .sta_mask = MFG_CORE0_PWR_STA_MASK,
		     /* .ctl_addr = NULL,  */
			   .sram_pdn_bits = 0,
			   .sram_pdn_ack_bits = 0,
			   .bus_prot_mask = 0,
			   .ops = &MFG_CORE0_sys_ops,
			   },
};

static struct pg_callbacks *g_pgcb;

struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb)
{
	struct pg_callbacks *old_pgcb = g_pgcb;

	g_pgcb = pgcb;
	return old_pgcb;
}

#ifdef TOPAXI_PROTECT_LOCK
#define _TOPAXI_TIMEOUT_CNT_ 20000
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
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & (mask_value)) != (mask_value)) {
			count++;
			if (count > _TOPAXI_TIMEOUT_CNT_)
				break;
		}
	} else {
		spm_write(INFRA_TOPAXI_PROTECTEN, spm_read(INFRA_TOPAXI_PROTECTEN) & ~(mask_value));
		while (spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & (mask_value)) {
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
		pr_err("INFRA_TOPAXI_PROTECTEN = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN));
		pr_err("INFRA_TOPAXI_PROTECTSTA0 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA0));
		pr_err("INFRA_TOPAXI_PROTECTEN_STA1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN_STA1));
		/*pr_err("INFRA_BUS_IDLE_STA5 = 0x%x\n", clk_readl(INFRA_BUS_IDLE_STA5));*/
		/*dump_emi_MM();*/
		/*dump_emi_latency();*/
		WARN_ON(1);
	}

	return 0;
}
#if 0
void __iomem *infra_md_base;
#define MDSYS_INTF_PERMON0	(infra_md_base + 0x0F70)
#define MDSYS_INTF_PERMON8	(infra_md_base + 0x0F90)

void mdsys_intf_mon_dump(void)
{
	int i = 0;
	unsigned int temp;
	/*mon init*/
	spm_write(MDSYS_INTF_PERMON0, 0x81000042);

	for (i = 0; i < 7; i++) {
		temp = spm_read(MDSYS_INTF_PERMON0)&0xFF07FFFF;
		temp |= (0x18 + i) << 19;
		spm_write(MDSYS_INTF_PERMON0, temp);
		pr_err("mon8 = 0x%x (mon0=0x%x)\n", spm_read(MDSYS_INTF_PERMON8), temp);
	}
}

static void iomap(void)
{
	struct device_node *node;

/*infra md*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,infra_md");
	if (!node)
		pr_err("[INFRA_MD] find node failed\n");
	infra_md_base = of_iomap(node, 0);
	if (!infra_md_base)
		pr_err("[INFRA_MD] base failed\n");
}
#endif

int spm_topaxi_protect_1(unsigned int mask_value, int en)
{
	unsigned long flags;
	int count = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;

	do_gettimeofday(&tm_s);
	spm_mtcmos_noncpu_md_lock(flags);

	if (en == 1) {
		spm_write(INFRA_TOPAXI_PROTECTEN_1, spm_read(INFRA_TOPAXI_PROTECTEN_1) | (mask_value));
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_1) & (mask_value)) != (mask_value)) {
			count++;
			if (count > _TOPAXI_TIMEOUT_CNT_)
				break;
		}
	} else {
		spm_write(INFRA_TOPAXI_PROTECTEN_1, spm_read(INFRA_TOPAXI_PROTECTEN_1) & ~(mask_value));
		while (spm_read(INFRA_TOPAXI_PROTECTEN_STA1_1) & (mask_value)) {
			count++;
			if (count > _TOPAXI_TIMEOUT_CNT_)
				break;
		}
	}

	spm_mtcmos_noncpu_md_unlock(flags);

	if (count > _TOPAXI_TIMEOUT_CNT_) {
		do_gettimeofday(&tm_e);
		tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000000 + (tm_e.tv_usec - tm_s.tv_usec);
		pr_err("TOPAXI_1 Bus Protect Timeout Error (%d us)(%d) !!\n", tm_val, count);
		pr_err("INFRA_TOPAXI_PROTECTEN_1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN_1));
		pr_err("INFRA_TOPAXI_PROTECTSTA0_1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTSTA0_1));
		pr_err("INFRA_TOPAXI_PROTECTSTA1_1 = 0x%x\n", clk_readl(INFRA_TOPAXI_PROTECTEN_STA1_1));
		/*mdsys_intf_mon_dump();*/
		/*dump_emi_MM();*/
		/*dump_emi_latency();*/
		WARN_ON(1);
	}

	return 0;
}
#endif

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

int cam_mtcmos_patch(int on)
{
	int larb2_cg = 0;
	if (on) {
		/* do something */
		/* do something */
	} else {
		if (clk_readl(CAMSYS_CG_STA) & 0x0001)
			larb2_cg = 1;
		else
			larb2_cg = 0;
		clk_writel(CAMSYS_CG_CLR, 0x0001);
	}
	return larb2_cg;
}

/* auto-gen begin*/
int spm_mtcmos_ctrl_md1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MD1" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MD1_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & MD1_PROT_BIT_ACK_MASK) != MD1_PROT_BIT_ACK_MASK) {
			/*  */
			/*  */
		}
#endif
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MD1_PROT_BIT_2ND_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_1) & MD1_PROT_BIT_ACK_2ND_MASK)
			!= MD1_PROT_BIT_ACK_2ND_MASK) {
			/*  */
			/*  */
		}
#endif
		/* TINFO="MD_EXTRA_PWR_CON[0]=1"*/
		spm_write(MD_EXTRA_PWR_CON, spm_read(MD_EXTRA_PWR_CON) | (0x1 << 0));
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ISO);
		/* TINFO="MD_SRAM_ISO_CON[0]=0"*/
		spm_write(MD_SRAM_ISO_CON, spm_read(MD_SRAM_ISO_CON) & ~(0x1 << 0));
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | MD1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MD1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MD1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MD1" */
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MD1_PWR_STA_MASK) != MD1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MD1_PWR_STA_MASK) != MD1_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~(0x1 << 8));
		/* TINFO="MD_SRAM_ISO_CON[0]=1"*/
		spm_write(MD_SRAM_ISO_CON, spm_read(MD_SRAM_ISO_CON) | (0x1 << 0));
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MD1_PWR_CON, spm_read(MD1_PWR_CON) | PWR_RST_B);
		/* TINFO="MD_EXTRA_PWR_CON[0]=0"*/
		spm_write(MD_EXTRA_PWR_CON, spm_read(MD_EXTRA_PWR_CON) & ~(0x1 << 0));
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, MD1_PROT_BIT_2ND_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MD1_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on MD1" */
	}
	return err;
}

int spm_mtcmos_ctrl_conn(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off CONN" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, CONN_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & CONN_PROT_BIT_ACK_MASK) != CONN_PROT_BIT_ACK_MASK) {
			/*  */
			/*  */
		}
#endif
#ifndef IGNORE_MTCMOS_CHECK
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
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & CONN_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & CONN_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off CONN" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on CONN" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & CONN_PWR_STA_MASK) != CONN_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & CONN_PWR_STA_MASK) != CONN_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(CONN_PWR_CON, spm_read(CONN_PWR_CON) | PWR_RST_B);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, CONN_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on CONN" */
	}
	return err;
}

int spm_mtcmos_ctrl_dis(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off DIS" */
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, DIS_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & DIS_PROT_BIT_ACK_MASK) != DIS_PROT_BIT_ACK_MASK) {
			/*  */
			/*  */
		}
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | DIS_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DIS_SRAM_PDN_ACK = 1" */
		while ((spm_read(DIS_PWR_CON) & DIS_SRAM_PDN_ACK) != DIS_SRAM_PDN_ACK) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
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
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & DIS_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off DIS" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on DIS" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & DIS_PWR_STA_MASK) != DIS_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & DIS_PWR_STA_MASK) != DIS_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(DIS_PWR_CON, spm_read(DIS_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until DIS_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(DIS_PWR_CON) & DIS_SRAM_PDN_ACK_BIT0) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, DIS_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Finish to turn on DIS" */
	}
	return err;
}

void mfg_way_en(int way_en)
{
	int retry = 0;

	if (way_en) {
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MFG_PROT_BIT_2ND_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, MFG_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Extra set after releasing bus protect of MFG (way_en enable)" */
		/* *((UINT32P)(0x10001200)) |= (0x1<<7);   set bit_7=1 */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) | (0x1 << 7));
	} else {
		/* TINFO="Start to turn off MFG" */
		/* TINFO="Extra set before setting bus protect of MFG (way_en disable)" */
		/* *((UINT32P)(0x10001200)) &= (0xffffff7f);   set bit_7=0 */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) & (~(0x1 << 7)));
		/* TINFO="Extra polling before setting bus protect of MFG*/
		/* : (topaxi_si0_ctrl_updated==1) or (wr_ot_busy==rd_ot_busy==0)" */
		while (((spm_read(INFRA_TOPAXI_SI0_STA) & CTRL_UPDATE_BIT) != CTRL_UPDATE_BIT) &&
			!((spm_read(INFRA_TOPAXI_SI0_STA) | ~(WR_RD_OT_BUSY_BIT)) == ~(WR_RD_OT_BUSY_BIT))) {
		}
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MFG_PROT_BIT_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_1) & MFG_PROT_BIT_ACK_MASK) != MFG_PROT_BIT_ACK_MASK) {
			/**/
			/**/
		}
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MFG_PROT_BIT_2ND_MASK);
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & MFG_PROT_BIT_ACK_2ND_MASK)
			!= MFG_PROT_BIT_ACK_2ND_MASK) {
			retry++;
			if (retry > 100) /*check mfg idle status[20:19]*/
				pr_debug("%s:%08x\n", __func__, spm_read(INFRA_TOPAXI_PROTECTSTA0_1));
		}
	}
}

int spm_mtcmos_ctrl_mfg(int state)
{
	int err = 0;


	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG" */
#if 0
		/* TINFO="Extra set before setting bus protect of MFG (way_en disable)" */
		/* *((UINT32P)(0x10001200)) &= (0xffffff7f);   set bit_7=0 */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) & (~(0x1 << 7)));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Extra polling before setting bus protect of MFG*/
		/* : (topaxi_si0_ctrl_updated==1) or (wr_ot_busy==rd_ot_busy==0)" */
		while (((spm_read(INFRA_TOPAXI_SI0_STA) & CTRL_UPDATE_BIT) != CTRL_UPDATE_BIT) &&
			!((spm_read(INFRA_TOPAXI_SI0_STA) | ~(WR_RD_OT_BUSY_BIT)) == ~(WR_RD_OT_BUSY_BIT))) {
			/*  */
			/*  */
		}
#endif
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_SET, MFG_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_1) & MFG_PROT_BIT_ACK_MASK) != MFG_PROT_BIT_ACK_MASK) {
			/*  */
			/*  */
		}
#endif
		/* TINFO="Set bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_SET, MFG_PROT_BIT_2ND_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1) & MFG_PROT_BIT_ACK_2ND_MASK)
			!= MFG_PROT_BIT_ACK_2ND_MASK) {
			retry++;
			if (retry > 100) /*check mfg idle status[20:19]*/
				pr_debug("%s:%08x\n", __func__, spm_read(INFRA_TOPAXI_PROTECTSTA0_1));
		}
#endif
#endif
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | MFG_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG_PWR_CON) & MFG_SRAM_PDN_ACK) != MFG_SRAM_PDN_ACK) {
				/* Need f_fmfg_core_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
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
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MFG" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG_PWR_STA_MASK) != MFG_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG_PWR_STA_MASK) != MFG_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) | PWR_RST_B);

		/* TINFO="Wait until mfg_efuse_s2p_rx_done = 1" */
		while ((spm_read(MBIST_EFUSE_REPAIR_ACK_STA) & MFG_EFUSE_S2P_RX_DONE_BIT)
			!= MFG_EFUSE_S2P_RX_DONE_BIT) {
				/* Need PWR_RST to trigger efuse s2p. */
				/*  */
		}

		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG_PWR_CON) & MFG_SRAM_PDN_ACK_BIT0) {
				/* Need f_fmfg_core_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(MFG_PWR_CON, spm_read(MFG_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(MFG_PWR_CON) & MFG_SRAM_PDN_ACK_BIT1) {
				/* Need f_fmfg_core_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif

#if 0
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_CLR, MFG_PROT_BIT_2ND_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Release bus protect" */
		spm_write(INFRA_TOPAXI_PROTECTEN_1_CLR, MFG_PROT_BIT_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		/* Note that this protect ack check after releasing protect has been ignored */
#endif
		/* TINFO="Extra set after releasing bus protect of MFG (way_en enable)" */
		/* *((UINT32P)(0x10001200)) |= (0x1<<7);   set bit_7=1 */
		spm_write(INFRA_TOPAXI_SI0_CTL, spm_read(INFRA_TOPAXI_SI0_CTL) | (0x1 << 7));
#endif
		/* TINFO="Finish to turn on MFG" */
	}
	return err;
}

int spm_mtcmos_ctrl_isp(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off ISP" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) | ISP_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until ISP_SRAM_PDN_ACK = 1" */
		while ((spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK) != ISP_SRAM_PDN_ACK) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
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
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(ISP_PWR_CON, spm_read(ISP_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until ISP_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(ISP_PWR_CON) & ISP_SRAM_PDN_ACK_BIT1) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		/* TINFO="Finish to turn on ISP" */
	}
	return err;
}

int spm_mtcmos_ctrl_ven(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VEN" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) | VEN_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK = 1" */
		while ((spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK) != VEN_SRAM_PDN_ACK) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
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
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
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
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
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
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT1) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT2) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(VEN_PWR_CON, spm_read(VEN_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VEN_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(VEN_PWR_CON) & VEN_SRAM_PDN_ACK_BIT3) {
				/* Need hf_fmm_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		/* TINFO="Finish to turn on VEN" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg_async(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG_ASYNC" */
#ifndef IGNORE_MTCMOS_CHECK
#endif
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
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_ASYNC_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG_ASYNC_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MFG_ASYNC" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG_ASYNC" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG_ASYNC_PWR_STA_MASK) != MFG_ASYNC_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG_ASYNC_PWR_STA_MASK) != MFG_ASYNC_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_ASYNC_PWR_CON, spm_read(MFG_ASYNC_PWR_CON) | PWR_RST_B);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on MFG_ASYNC" */
	}
	return err;
}

int spm_mtcmos_ctrl_audio(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off AUDIO" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | AUDIO_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK = 1" */
		while ((spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK) != AUDIO_SRAM_PDN_ACK) {
				/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
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
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & AUDIO_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & AUDIO_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off AUDIO" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on AUDIO" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & AUDIO_PWR_STA_MASK) != AUDIO_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & AUDIO_PWR_STA_MASK) != AUDIO_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK_BIT0) {
				/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK_BIT1) {
				/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 10));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK_BIT2 = 0" */
		while (spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK_BIT2) {
				/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		spm_write(AUDIO_PWR_CON, spm_read(AUDIO_PWR_CON) & ~(0x1 << 11));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until AUDIO_SRAM_PDN_ACK_BIT3 = 0" */
		while (spm_read(AUDIO_PWR_CON) & AUDIO_SRAM_PDN_ACK_BIT3) {
				/* Need f_f26M_aud_ck for SRAM PDN delay IP. */
				/*  */
		}
#endif
		/* TINFO="Finish to turn on AUDIO" */
	}
	return err;
}

int spm_mtcmos_ctrl_cam(int state)
{
	int err = 0;
	int retry = 0;
	int larb2_cg = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		larb2_cg = cam_mtcmos_patch(state);
		/* TINFO="Start to turn off CAM" */
		/* TINFO="Set bus protect(SMI_LARB2_SLP_CON)" */
		spm_write(SMI_LARB2_SLP_CON, spm_read(SMI_LARB2_SLP_CON) | 0x1);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(SMI_LARB2_SLP_CON) & 0x10000) != 0x10000) {
			retry++;
			if (retry > 100) {
				pr_debug("CAMSYS_CG_STA = 0x%x, larb2_cg = %d\n", clk_readl(CAMSYS_CG_STA), larb2_cg);
				pr_debug("SMI_LARB_STAT = 0x%x\n", clk_readl(SMI_LARB_STAT));
				pr_debug("SMI_LARB2_SLP_CON = 0x%x\n", clk_readl(SMI_LARB2_SLP_CON));
			}
			/**/
		}
#endif
		/* TINFO="Enable SMI COMM request mask" */
		spm_write(SMI_COMMON_SMI_CLAMP_SET, spm_read(SMI_COMMON_SMI_CLAMP_SET) | (0x1 << 2));
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | CAM_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK = 1" */
		while ((spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK) != CAM_SRAM_PDN_ACK) {
				/*  */
				/*  */
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
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
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
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) | PWR_RST_B);

		/* TINFO="Wait until cam_efuse_s2p_rx_done = 1" */
		while ((spm_read(MBIST_EFUSE_REPAIR_ACK_STA) & CAM_EFUSE_S2P_RX_DONE_BIT)
			!= CAM_EFUSE_S2P_RX_DONE_BIT) {
				/* Need PWR_RST to trigger efuse s2p. */
				/*  */
		}

		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~(0x1 << 8));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK_BIT0) {
				/*  */
				/*  */
		}
#endif
		spm_write(CAM_PWR_CON, spm_read(CAM_PWR_CON) & ~(0x1 << 9));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until CAM_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(CAM_PWR_CON) & CAM_SRAM_PDN_ACK_BIT1) {
				/*  */
				/*  */
		}
#endif
		/* TINFO="Disable SMI COMM request mask" */
		spm_write(SMI_COMMON_SMI_CLAMP_CLR, spm_read(SMI_COMMON_SMI_CLAMP_CLR) | (0x1 << 2));
		/* TINFO="Finish to turn on CAM" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg_core1(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG_CORE1" */
		mfg_way_en(0);
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | MFG_CORE1_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_CORE1_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG_CORE1_PWR_CON) & MFG_CORE1_SRAM_PDN_ACK) != MFG_CORE1_SRAM_PDN_ACK) {
				/*  */
				/*  */
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_CORE1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG_CORE1_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MFG_CORE1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG_CORE1" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG_CORE1_PWR_STA_MASK) != MFG_CORE1_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG_CORE1_PWR_STA_MASK) != MFG_CORE1_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG_CORE1_PWR_CON, spm_read(MFG_CORE1_PWR_CON) & ~(0x1 << 5));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_CORE1_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG_CORE1_PWR_CON) & MFG_CORE1_SRAM_PDN_ACK_BIT0) {
				/*  */
				/*  */
		}
#endif
		mfg_way_en(1);
		/* TINFO="Finish to turn on MFG_CORE1" */
	}
	return err;
}

int spm_mtcmos_ctrl_mfg_core0(int state)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	/*spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0));*/

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MFG_CORE0" */
		/* TINFO="Set SRAM_PDN = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | MFG_CORE0_SRAM_PDN);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_CORE0_SRAM_PDN_ACK = 1" */
		while ((spm_read(MFG_CORE0_PWR_CON) & MFG_CORE0_SRAM_PDN_ACK) != MFG_CORE0_SRAM_PDN_ACK) {
				/*  */
				/*  */
		}
#endif
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & MFG_CORE0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & MFG_CORE0_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/*Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Finish to turn off MFG_CORE0" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on MFG_CORE0" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & MFG_CORE0_PWR_STA_MASK) != MFG_CORE0_PWR_STA_MASK)
		       || ((spm_read(PWR_STATUS_2ND) & MFG_CORE0_PWR_STA_MASK) != MFG_CORE0_PWR_STA_MASK)) {
				/* No logic between pwr_on and pwr_ack.*/
				/* Print SRAM / MTCMOS control and PWR_ACK for debug. */
		}
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) | PWR_RST_B);
		/* TINFO="Set SRAM_PDN = 0" */
		spm_write(MFG_CORE0_PWR_CON, spm_read(MFG_CORE0_PWR_CON) & ~(0x1 << 5));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until MFG_CORE0_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(MFG_CORE0_PWR_CON) & MFG_CORE0_SRAM_PDN_ACK_BIT0) {
				/*  */
				/*  */
		}
#endif
		/* TINFO="Finish to turn on MFG_CORE0" */
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

static int CONN_sys_enable_op(struct subsys *sys)
{
	/*printk("CONN_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_conn(STA_POWER_ON);
}

static int DIS_sys_enable_op(struct subsys *sys)
{
	/*printk("DIS_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_dis(STA_POWER_ON);
}

static int MFG_sys_enable_op(struct subsys *sys)
{
	/*printk("MFG_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg(STA_POWER_ON);
}

static int ISP_sys_enable_op(struct subsys *sys)
{
	/*printk("ISP_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_isp(STA_POWER_ON);
}

static int VEN_sys_enable_op(struct subsys *sys)
{
	/*printk("VEN_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_ven(STA_POWER_ON);
}

static int MFG_ASYNC_sys_enable_op(struct subsys *sys)
{
	/*printk("MFG_ASYNC_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_async(STA_POWER_ON);
}

static int AUDIO_sys_enable_op(struct subsys *sys)
{
	/*printk("AUDIO_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_audio(STA_POWER_ON);
}

static int CAM_sys_enable_op(struct subsys *sys)
{
	/*printk("CAM_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_cam(STA_POWER_ON);
}

static int MFG_CORE1_sys_enable_op(struct subsys *sys)
{
	/*printk("MFG_CORE1_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_core1(STA_POWER_ON);
}

static int MFG_CORE0_sys_enable_op(struct subsys *sys)
{
	/*printk("MFG_CORE0_sys_enable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_core0(STA_POWER_ON);
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

static int CONN_sys_disable_op(struct subsys *sys)
{
	/*printk("CONN_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_conn(STA_POWER_DOWN);
}

static int DIS_sys_disable_op(struct subsys *sys)
{
	/*printk("DIS_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_dis(STA_POWER_DOWN);
}

static int MFG_sys_disable_op(struct subsys *sys)
{
	/*printk("MFG_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
}

static int ISP_sys_disable_op(struct subsys *sys)
{
	/*printk("ISP_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
}

static int VEN_sys_disable_op(struct subsys *sys)
{
	/*printk("VEN_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_ven(STA_POWER_DOWN);
}

static int MFG_ASYNC_sys_disable_op(struct subsys *sys)
{
	/*printk("MFG_ASYNC_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_async(STA_POWER_DOWN);
}

static int AUDIO_sys_disable_op(struct subsys *sys)
{
	/*printk("AUDIO_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_audio(STA_POWER_DOWN);
}

static int CAM_sys_disable_op(struct subsys *sys)
{
	/*printk("CAM_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_cam(STA_POWER_DOWN);
}

static int MFG_CORE1_sys_disable_op(struct subsys *sys)
{
	/*printk("MFG_CORE1_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_core1(STA_POWER_DOWN);
}

static int MFG_CORE0_sys_disable_op(struct subsys *sys)
{
	/*printk("MFG_CORE0_sys_disable_op\r\n"); */
	return spm_mtcmos_ctrl_mfg_core0(STA_POWER_DOWN);
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

static struct subsys_ops CONN_sys_ops = {
	.enable = CONN_sys_enable_op,
	.disable = CONN_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops DIS_sys_ops = {
	.enable = DIS_sys_enable_op,
	.disable = DIS_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG_sys_ops = {
	.enable = MFG_sys_enable_op,
	.disable = MFG_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops ISP_sys_ops = {
	.enable = ISP_sys_enable_op,
	.disable = ISP_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops VEN_sys_ops = {
	.enable = VEN_sys_enable_op,
	.disable = VEN_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG_ASYNC_sys_ops = {
	.enable = MFG_ASYNC_sys_enable_op,
	.disable = MFG_ASYNC_sys_disable_op,
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

static struct subsys_ops MFG_CORE1_sys_ops = {
	.enable = MFG_CORE1_sys_enable_op,
	.disable = MFG_CORE1_sys_disable_op,
	.get_state = sys_get_state_op,
};

static struct subsys_ops MFG_CORE0_sys_ops = {
	.enable = MFG_CORE0_sys_enable_op,
	.disable = MFG_CORE0_sys_disable_op,
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
1,	/*SYS_MD1 = 0,*/
1,	/*SYS_CONN = 1,*/
1,	/*SYS_DIS = 2,*//*can HS, but resume fail*/
1,	/*SYS_MFG = 3,*/
1,	/*SYS_ISP = 4,*/
1,	/*SYS_VEN = 5,*/
1,	/*SYS_MFG_ASYNC = 6,*/
1,	/*SYS_AUDIO = 7,*/
1,	/*SYS_CAM = 8,*//*There is no process hold rtnl lock*/
1,	/*SYS_MFG_CORE1 = 9,*/
1,	/*SYS_MFG_CORE0 = 10,*/
};
#endif
static int enable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	WARN_ON(!sys);

#if MT_CCF_BRINGUP
	/*pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_MD1:
		spm_mtcmos_ctrl_md1(STA_POWER_ON);
		break;
	case SYS_CONN:
		spm_mtcmos_ctrl_conn(STA_POWER_ON);
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

	if (g_pgcb && g_pgcb->after_on)
		g_pgcb->after_on(id);

	return r;
}

static int disable_subsys(enum subsys_id id)
{
	int r;
	unsigned long flags;
	struct subsys *sys = id_to_sys(id);

	WARN_ON(!sys);

#if MT_CCF_BRINGUP
	/*pr_debug("[CCF] %s: sys=%s, id=%d\n", __func__, sys->name, id);*/
	switch (id) {
	case SYS_MD1:
		spm_mtcmos_ctrl_md1(STA_POWER_DOWN);
		break;
	case SYS_CONN:
		spm_mtcmos_ctrl_conn(STA_POWER_DOWN);
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

	if (g_pgcb && g_pgcb->before_off)
		g_pgcb->before_off(id);

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
}

static const struct clk_ops mt_power_gate_ops = {
	.prepare = pg_prepare,
	.unprepare = pg_unprepare,
	.is_enabled = pg_is_enabled,
};

struct clk *mt_clk_register_power_gate(const char *name,
				       const char *parent_name,
				       struct clk *pre_clk, enum subsys_id pd_id)
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

#define pg_md1         "pg_md1"
#define pg_conn        "pg_conn"
#define pg_dis         "pg_dis"
#define pg_mfg         "pg_mfg"
#define pg_isp         "pg_isp"
#define pg_ven         "pg_ven"
#define pg_mfg_async   "pg_mfg_async"
#define pg_audio       "pg_audio"
#define pg_cam   "pg_cam"
#define pg_mfg_core1   "pg_mfg_core1"
#define pg_mfg_core0   "pg_mfg_core0"

#define img_sel	"img_sel"
#define cam_sel	"cam_sel"
#define mm_sel			"mm_sel"
#define f26m_sel	"f26m_sel"
#define mfg_sel	"mfg_sel"
#define infracfg_ao_audio_26m_bclk_ck "infracfg_ao_audio_26m_bclk_ck"
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
	PGATE(SCP_SYS_CONN, pg_conn, NULL, NULL, SYS_CONN),
	PGATE(SCP_SYS_DIS, pg_dis, NULL, mm_sel, SYS_DIS),
	PGATE(SCP_SYS_MFG_ASYNC, pg_mfg_async, NULL, mfg_sel, SYS_MFG_ASYNC),
	PGATE(SCP_SYS_MFG, pg_mfg, pg_mfg_async, NULL, SYS_MFG),
	PGATE(SCP_SYS_ISP, pg_isp, NULL, img_sel, SYS_ISP),
	PGATE(SCP_SYS_VEN, pg_ven, NULL, mm_sel, SYS_VEN),
	PGATE(SCP_SYS_AUDIO, pg_audio, NULL, infracfg_ao_audio_26m_bclk_ck, SYS_AUDIO),
	PGATE(SCP_SYS_CAM, pg_cam, NULL, cam_sel, SYS_CAM),
	PGATE(SCP_SYS_MFG_CORE1, pg_mfg_core1, pg_mfg, NULL, SYS_MFG_CORE1),
	PGATE(SCP_SYS_MFG_CORE0, pg_mfg_core0, pg_mfg, NULL, SYS_MFG_CORE0),
};

static void __init init_clk_scpsys(void __iomem *infracfg_reg,
				   void __iomem *spm_reg,
				   void __iomem *infra_reg,
				   void __iomem *smi_larb2_reg,
				   void __iomem *smi_common_reg,
				   struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;
	struct clk *pre_clk;

	infracfg_base = infracfg_reg;
	spm_base = spm_reg;
	infra_base = infra_reg;
	smi_larb2_base = smi_larb2_reg;
	smi_common_base = smi_common_reg;

	syss[SYS_MD1].ctl_addr = MD1_PWR_CON;
	syss[SYS_CONN].ctl_addr = CONN_PWR_CON;
	syss[SYS_DIS].ctl_addr = DIS_PWR_CON;
	syss[SYS_MFG].ctl_addr = MFG_PWR_CON;
	syss[SYS_ISP].ctl_addr = ISP_PWR_CON;
	syss[SYS_VEN].ctl_addr = VEN_PWR_CON;
	syss[SYS_MFG_ASYNC].ctl_addr = MFG_ASYNC_PWR_CON;
	syss[SYS_AUDIO].ctl_addr = AUDIO_PWR_CON;
	syss[SYS_CAM].ctl_addr = CAM_PWR_CON;
	syss[SYS_MFG_CORE1].ctl_addr = MFG_CORE1_PWR_CON;
	syss[SYS_MFG_CORE0].ctl_addr = MFG_CORE0_PWR_CON;

	for (i = 0; i < ARRAY_SIZE(scp_clks); i++) {
		struct mtk_power_gate *pg = &scp_clks[i];

		pre_clk = pg->pre_clk_name ? __clk_lookup(pg->pre_clk_name) : NULL;

		clk = mt_clk_register_power_gate(pg->name, pg->parent_name, pre_clk, pg->pd_id);

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

#ifdef CONFIG_OF
void iomap_mm(void)
{
	struct device_node *node;

/*mmsys_config*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,mmsys_config");
	if (!node)
		pr_debug("[CLK_MMSYS] find node failed\n");
	clk_mmsys_config_base = of_iomap(node, 0);
	if (!clk_mmsys_config_base)
		pr_debug("[CLK_MMSYS] base failed\n");
/*imgsys*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,imgsys");
	if (!node)
		pr_debug("[CLK_IMGSYS_CONFIG] find node failed\n");
	clk_imgsys_base = of_iomap(node, 0);
	if (!clk_imgsys_base)
		pr_debug("[CLK_IMGSYS_CONFIG] base failed\n");
/*vdec_gcon*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,vdec_gcon");
	if (!node)
		pr_debug("[CLK_VDEC_GCON] find node failed\n");
	clk_vdec_gcon_base = of_iomap(node, 0);
	if (!clk_vdec_gcon_base)
		pr_debug("[CLK_VDEC_GCON] base failed\n");
/*venc_gcon*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,venc_gcon");
	if (!node)
		pr_debug("[CLK_VENC_GCON] find node failed\n");
	clk_venc_gcon_base = of_iomap(node, 0);
	if (!clk_venc_gcon_base)
		pr_debug("[CLK_VENC_GCON] base failed\n");

/*cam*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,camsys");
	if (!node)
		pr_debug("[CLK_CAM] find node failed\n");
	clk_camsys_base = of_iomap(node, 0);
	if (!clk_camsys_base)
		pr_debug("[CLK_CAM] base failed\n");
}
#endif

static void __init mt_scpsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *infracfg_reg;
	void __iomem *spm_reg;
	void __iomem *infra_reg;
	void __iomem *ckgen_reg;
	void __iomem *smi_larb2_reg;
	void __iomem *smi_common_reg;
	int r;

	infracfg_reg = get_reg(node, 0);
	spm_reg = get_reg(node, 1);
	infra_reg = get_reg(node, 2);
	ckgen_reg = get_reg(node, 3);
	smi_larb2_reg = get_reg(node, 4);
	smi_common_reg = get_reg(node, 5);



	if (!infracfg_reg || !spm_reg || !infra_reg  || !ckgen_reg || !smi_larb2_reg || !smi_common_reg) {
		pr_err("clk-pg-mt6763: missing reg\n");
		return;
	}

/*
*   pr_debug("[CCF] %s: sys: %s, reg: 0x%p, 0x%p\n",
*		__func__, node->name, infracfg_reg, spm_reg);
*/

	clk_data = alloc_clk_data(SCP_NR_SYSS);

	init_clk_scpsys(infracfg_reg, spm_reg, infra_reg, smi_larb2_reg, smi_common_reg, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("[CCF] %s:could not register clock provide\n", __func__);

	ckgen_base = ckgen_reg;
	/*MM Bus*/
	iomap_mm();
#if 0/*!MT_CCF_BRINGUP*/
	/* subsys init: per modem owner request, disable modem power first */
	disable_subsys(SYS_MD1);
#else				/*power on all subsys for bring up */
#ifndef CONFIG_FPGA_EARLY_PORTING
	spm_mtcmos_ctrl_mfg_async(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg_core0(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg_core1(STA_POWER_ON);

	spm_mtcmos_ctrl_dis(STA_POWER_ON);
	spm_mtcmos_ctrl_cam(STA_POWER_ON);
	spm_mtcmos_ctrl_ven(STA_POWER_ON);
	spm_mtcmos_ctrl_isp(STA_POWER_ON);

	/*spm_mtcmos_ctrl_md1(STA_POWER_ON);*/
	spm_mtcmos_ctrl_md1(STA_POWER_DOWN);

	spm_mtcmos_ctrl_audio(STA_POWER_ON);
#endif
#endif				/* !MT_CCF_BRINGUP */
}

CLK_OF_DECLARE(mtk_pg_regs, "mediatek,scpsys", mt_scpsys_init);

void subsys_if_on(void)
{
	unsigned int sta = spm_read(PWR_STATUS);
	unsigned int sta_s = spm_read(PWR_STATUS_2ND);

	if ((sta & (1U << 0)) && (sta_s & (1U << 0)))
		pr_debug("suspend warning: SYS_MD1 is on!!!\n");
	if ((sta & (1U << 1)) && (sta_s & (1U << 1)))
		pr_debug("suspend warning: SYS_CONN is on!!!\n");
	if ((sta & (1U << 3)) && (sta_s & (1U << 3)))
		pr_debug("suspend warning: SYS_DIS is on!!!\n");
	if ((sta & (1U << 4)) && (sta_s & (1U << 4)))
		pr_debug("suspend warning: SYS_MFG is on!!!\n");
	if ((sta & (1U << 5)) && (sta_s & (1U << 5)))
		pr_debug("suspend warning: SYS_ISP is on!!!\n");
	if ((sta & (1U << 21)) && (sta_s & (1U << 21)))
		pr_debug("suspend warning: SYS_VEN is on!!!\n");
	if ((sta & (1U << 23)) && (sta_s & (1U << 23)))
		pr_debug("suspend warning: SYS_MFG_ASYNC is on!!!\n");
	if ((sta & (1U << 24)) && (sta_s & (1U << 24)))
		pr_debug("suspend warning: SYS_AUDIO is on!!!\n");
	if ((sta & (1U << 27)) && (sta_s & (1U << 27)))
		pr_debug("suspend warning: SYS_CAM is on!!!\n");
	if ((sta & (1U << 30)) && (sta_s & (1U << 30)))
		pr_debug("suspend warning: SYS_MFG_CORE1 is on!!!\n");
	if ((sta & (1U << 31)) && (sta_s & (1U << 31)))
		pr_debug("suspend warning: SYS_MFG_CORE0 is on!!!\n");
}

#if 1 /*only use for suspend test*/
void mtcmos_force_off(void)
{
	spm_mtcmos_ctrl_mfg_core0(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg_core1(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg_async(STA_POWER_DOWN);

	spm_mtcmos_ctrl_cam(STA_POWER_DOWN);
	spm_mtcmos_ctrl_ven(STA_POWER_DOWN);
	spm_mtcmos_ctrl_isp(STA_POWER_DOWN);
	spm_mtcmos_ctrl_dis(STA_POWER_DOWN);

	spm_mtcmos_ctrl_md1(STA_POWER_DOWN);

	spm_mtcmos_ctrl_audio(STA_POWER_DOWN);

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
		pg_conn,
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
