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

#ifndef __HW_BREAKPOINT_64_H
#define __HW_BREAKPOINT_64_H
#include <asm/io.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/hw_watchpoint.h>

#define MAX_NR_WATCH_POINT 4
struct wp_trace_context_t {
	void __iomem **debug_regs;
	struct wp_event wp_events[MAX_NR_WATCH_POINT];
	unsigned int wp_nr;
	unsigned int bp_nr;
	unsigned long id_aaa64dfr0_el1;
	unsigned int nr_dbg;
};

#define  WATCHPOINT_TEST_SUIT

struct dbgreg_set {
	unsigned long regs[28];
};
#define SDSR            regs[22]
#define DBGVCR          regs[21]
#define DBGWCR3         regs[20]
#define DBGWCR2         regs[19]
#define DBGWCR1         regs[18]
#define DBGWCR0         regs[17]
#define DBGWVR3         regs[16]
#define DBGWVR2         regs[15]
#define DBGWVR1         regs[14]
#define DBGWVR0         regs[13]
#define DBGBCR5         regs[12]
#define DBGBCR4         regs[11]
#define DBGBCR3         regs[10]
#define DBGBCR2         regs[9]
#define DBGBCR1         regs[8]
#define DBGBCR0         regs[7]
#define DBGBVR5         regs[6]
#define DBGBVR4         regs[5]
#define DBGBVR3		regs[4]
#define DBGBVR2		regs[3]
#define DBGBVR1		regs[2]
#define DBGBVR0		regs[1]
#define MDSCR_EL1	regs[0]

#define DBGWVR 0x800
#define DBGWCR 0x808
#define DBGBVR 0x400
#define DBGBCR 0x408
#define EDSCR  0x088

#define EDLAR 0xFB0
#define EDLSR 0xFB4
#define OSLAR_EL1 0x300

#define UNLOCK_KEY 0xC5ACCE55
#define HDE (1 << 14)
#define MDBGEN (1 << 15)
#define KDE (0x1 << 13)
#define DBGWCR_VAL 0x000001E7
 /**/
#define WP_EN (1 << 0)
#define LSC_LDR (1 << 3)
#define LSC_STR (2 << 3)
#define LSC_ALL (3 << 3)
/*
	#define WATCHPOINT_TEST_SUIT
*/
#define ARM_DBG_READ(N, M, OP2, VAL) {\
	asm volatile("mrc p14, 0, %0, " #N "," #M ", " #OP2 : "=r" (VAL));\
}
#define ARM_DBG_WRITE(N, M, OP2, VAL) {\
	asm volatile("mcr p14, 0, %0, " #N "," #M ", " #OP2 : : "r" (VAL));\
}
/* This is bit [29:27] in ESR which indicate watchpoint exception occurred */
#define DBG_ESR_EVT_HWWP    0x2
#define dbg_mem_read(addr)		readl(IOMEM(addr))
#define dbg_mem_read_64(addr)		readq(IOMEM(addr))
#define dbg_mem_write(addr, val)    mt_reg_sync_writel(val, addr)
#define dbg_reg_copy(offset, base_to, base_from)   \
	dbg_mem_write((base_to) + (offset), dbg_mem_read((base_from) + (offset)))
#define dbg_mem_write_64(addr, val)    mt_reg_sync_writeq(val, addr)
#define dbg_reg_copy_64(offset, base_to, base_from)   \
	dbg_mem_write_64((base_to) + (offset), dbg_mem_read_64((base_from) + (offset)))
static inline void cs_cpu_write(void __iomem *addr_base, u32 offset, u32 wdata)
{
	/* TINFO="Write addr %h, with data %h", addr_base+offset, wdata */
	mt_reg_sync_writel(wdata, addr_base + offset);
}

static inline unsigned int cs_cpu_read(const void __iomem *addr_base, u32 offset)
{
	u32 actual;

	/* TINFO="Read addr %h, with data %h", addr_base+offset, actual */
	actual = readl(addr_base + offset);

	return actual;
}

static inline void cs_cpu_write_64(void __iomem *addr_base, u64 offset, u64 wdata)
{
	/* TINFO="Write addr %h, with data %h", addr_base+offset, wdata */
	mt_reg_sync_writeq(wdata, addr_base + offset);
}

static inline unsigned long cs_cpu_read_64(const void __iomem *addr_base, u64 offset)
{
	u64 actual;

	/* TINFO="Read addr %h, with data %h", addr_base+offset, actual */
	actual = readq(addr_base + offset);

	return actual;
}


void smp_read_dbgsdsr_callback(void *info);
void smp_write_dbgsdsr_callback(void *info);
void smp_read_MDSCR_EL1_callback(void *info);
void smp_write_MDSCR_EL1_callback(void *info);
void smp_read_OSLSR_EL1_callback(void *info);
int register_wp_context(struct wp_trace_context_t **wp_tracer_context);
void __iomem *get_wp_base(void);

extern unsigned read_clusterid(void);

#endif				/* !__HW_BREAKPOINT_64_H */
