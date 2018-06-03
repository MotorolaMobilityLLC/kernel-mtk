/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __MCSI_B_TRACER_V1_H__
#define __MCSI_B_TRACER_V1_H__

#define get_bit_at(reg, pos) (((reg) >> (pos)) & 1)

#define CFG_CT_CTRL		0x0000
#define CFG_ERR_FLAG		0x0004
#define CFG_INT_STS		0x0008
#define CFG_INT_EN		0x000c
#define CFG_ERR_FLAG2		0x0020
#define CFG_WDTO_ERR_FLAG	0x0024

#define CFG_TRACE_CTRL		0x7000
#define CFG_TRACE_STS		0X7004
#define CFG_TRIGGER_CTRL0	0x7008
#define CFG_TRIGGER_CTRL1	0x700c
#define CFG_TRACE_TRI_LOW_ADDR	0x7010
#define CFG_TRACE_TRI_HIGH_ADDR 0x7014
#define CFG_TRACE_TRI_LOW_MASK	0x7018
#define CFG_TRACE_TRI_HIGH_MASK 0x701c
#define CFG_TRACE_TRI_OTHER_MATCH_FIELD	0x7020
#define CFG_TRACE_TRI_OTHER_MATCH_MASK	0x7024
#define CFG_TRACE_FILTER_LOW_ADDR  0x7030
#define CFG_TRACE_FILTER_HIGH_ADDR 0x7034
#define CFG_TRACE_FILTER_LOW_MASK  0x7038
#define CFG_TRACE_FILTER_HIGH_MASK 0x703C
#define CFG_TRACE_TRI_INT_STS	0x7040
#define CFG_TRACE_FILTER_LOW_CONTENT_FILED	0x7050
#define CFG_TRACE_FILTER_HIGH_CONTENT_FILED	0x7054
#define CFG_TRACE_FILTER_LOW_CONTENT_MASK	0x7058
#define CFG_TRACE_FILTER_HIGH_CONTENT_MASK	0x705c
#define	CFG_DBG_RD_CTRL		0x7100
#define CFG_DBG_RD_DATA		0x7104
#define CFG_DBG_INTF_HANG_CHK	0x7108

/* macro for CFG_CT_CTRL */
#define INT_EN			(1 << 3)

/* macro for CFG_INT_EN */
#define INTF_ERR		(1 << 22)
#define COH_ERR			(1 << 21)
#define IMPRECISE_ERR		(1 << 20)
#define TRACE_DONE		(1 << 19)
#define CCNT_OVERFLOW		(1 << 16)

/* macro for CFG_TRACE_CTRL */
#define TRACE_EN		(1 << 0)
#define VALID_XACT_ONLY		(1 << 18)
#define DUMP_DBG_BY_WD_TO	(1 << 19)

#define TP_SEL_SHIFT		(1)
#define TP_NUM_SHIFT		(4)

/* macro for CFG_TRACE_STS */
#define ATB_IDLE_SHIFT		(5)
#define AXI_IDLE_SHIFT		(1)
#define TRACE_STS_DONE		(1 << 0)

/* macro for CFG_TRIGGER_CTRL0 */
#define TRIGGER_MODE_SHIFT	(0)
#define TRIGGER_LINK_SHIFT	(4)
#define TRIGGER_TRI0_SEL_SHIFT	(16)
#define TRIGGER_TRI1_SEL_SHIFT	(20)
#define TRIGGER_TRI2_SEL_SHIFT	(24)
#define TRIGGER_TRI3_SEL_SHIFT	(28)

/* macro for CFG_TRIGGER_CTRL1 */
#define TRIGGER_OR_SHIFT	(12)
#define TRIGGER_AND0_SHIFT	(16)
#define TRIGGER_AND1_SHIFT	(20)
#define TRIGGER_AND2_SHIFT	(24)
#define TRIGGER_AND3_SHIFT	(28)



/* ETB registers, "CoreSight Components TRM", 9.3 */
#define ETB_DEPTH		0x04
#define ETB_STATUS		0x0c
#define ETB_READMEM		0x10
#define ETB_READADDR		0x14
#define ETB_WRITEADDR		0x18
#define ETB_TRIGGERCOUNT	0x1c
#define ETB_CTRL		0x20
#define ETB_FFSR		0x300
#define ETB_FFCR		0x304
#define ETB_LAR			0xfb0
#define ETB_LSR			0xfb4

#define DEM_ATB_CLK		0x70

#define REPLICATOR1_BASE	0x7000
#define REPLICATOR_LAR		0xfb0
#define REPLICATOR_IDFILTER0	0x0
#define REPLICATOR_IDFILTER1	0x4

#define FUNNEL_CTRL_REG		0x0
#define FUNNEL_LOCKACCESS	0xfb0

/* macros for ETB_FCCR */
#define FCCR_FLUSH_ON_TRIGGER	(1 << 5)
#define FCCR_STOP_FLUSH		(1 << 12)

#define CORESIGHT_LAR		0xfb0
#define CORESIGHT_UNLOCK        0xc5acce55

static inline void CS_LOCK(void __iomem *addr)
{
	do {
		/* Wait for things to settle */
		mb();
		writel_relaxed(0x0, addr + CORESIGHT_LAR);
	} while (0);
}

static inline void CS_UNLOCK(void __iomem *addr)
{
	do {
		writel_relaxed(CORESIGHT_UNLOCK, addr + CORESIGHT_LAR);
		/* Make sure everyone has seen this */
		mb();
	} while (0);
}

/* platform-dependent variables and functions */
extern char *tri_sel_to_type[];
unsigned int check_mcsi_bus_hang(void *base);

#endif /* end of __MCSI_B_TRACER_V1_H__ */

