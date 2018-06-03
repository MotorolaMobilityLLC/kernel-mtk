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

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <aee.h>
#include <linux/timer.h>
/* #include <asm/system.h> */
#include <asm-generic/irq_regs.h>
/* #include <asm/mach/map.h> */
#include <sync_write.h>
/*#include <mach/irqs.h>*/
#include <asm/cacheflush.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/fb.h>
#include <linux/debugfs.h>
#include <m4u.h>
#include "mtk_smi.h"

#include "smi_common.h"
#include "smi_reg.h"
#include "smi_debug.h"
#include "smi_configuration.h"
#include "smi_public.h"
#ifdef CONFIG_MTK_M4U
#include "m4u.h"
#endif

#define SMI_LOG_TAG "smi"

#if !defined(CONFIG_MTK_CLKMGR)
#define SMI_INTERNAL_CCF_SUPPORT
#endif

#if !defined(SMI_INTERNAL_CCF_SUPPORT)
#include <mach/mt_clkmgr.h>
#endif

#if IS_ENABLED(CONFIG_MACH_MT6771)
unsigned long long smi_latest_mdelay_sec;
unsigned long smi_latest_mdelay_nsec;
unsigned int smi_latest_mdelay_larb;
#endif

/* Debug Function */
static void smi_dump_format(unsigned long base, unsigned int from, unsigned int to);
static void smi_dumpper(int output_gce_buffer, unsigned long *offset, unsigned long base,
	int reg_number, bool zero_dump)
{
	int size = 0;
	int length = 0;
	int max_size = 128;
	char buffer[max_size + 1];
	int i = 0;
	int j = 0;

	for (i = 0; i < reg_number; i += j) {
		length = 0;
		max_size = 128;
		for (j = 0; i + j < reg_number; j++) {
			if (zero_dump == false && M4U_ReadReg32(base, offset[i + j]) == 0)
				continue;
			else if (zero_dump == true && base == 0) /* offset */
				size = snprintf(buffer + length, max_size + 1, " 0x%lx,", offset[i + j]);
			else if (base != 0) /* offset + value */
				size = snprintf(buffer + length, max_size + 1, " 0x%lx=0x%x,",
					offset[i + j], M4U_ReadReg32(base, offset[i + j]));

			if (size < 0 || max_size < size) {
				snprintf(buffer + length, max_size + 1, " ");
				break;
			}
			length = length + size;
			max_size = max_size - size;
		}
		SMIMSG3(output_gce_buffer, "%s\n", buffer);
	}
}

static void smi_dumpRegDebugMsg(int output_gce_buffer)
{
	SMIMSG3(output_gce_buffer, "========== SMI common register dump offset ==========\n");
	smi_dumpper(output_gce_buffer, smi_common_debug_offset, 0,
		    SMI_COMMON_DEBUG_OFFSET_NUM, true);

	SMIMSG3(output_gce_buffer, "========== SMI larb register dump offset ==========\n");
	smi_dumpper(output_gce_buffer, smi_larb_debug_offset[0], 0,
	    smi_larb_debug_offset_num[0], true);
}

void smi_dumpCommonDebugMsg(int output_gce_buffer)
{
	unsigned long u4Base = 0;
	/* No verify API in CCF, assume clk is always on */
	unsigned int smiCommonClkEnabled = 1;

	u4Base = get_common_base_addr();
	smiCommonClkEnabled = smi_clk_get_ref_count(SMI_COMMON_REG_INDX);
	/* SMI COMMON dump */
	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG3(output_gce_buffer, "Doesn't support reg dump for SMI common\n");
		return;
	} else if (smiCommonClkEnabled == 0) {
		SMIMSG3(output_gce_buffer, "========== SMI common clock is disable ==========\n");
		return;
	}

	SMIMSG3(output_gce_buffer, "========== SMI common reg dump, CLK: %d ==========\n", smiCommonClkEnabled);

	smi_dumpper(output_gce_buffer, smi_common_debug_offset, u4Base,
		    SMI_COMMON_DEBUG_OFFSET_NUM, false);
}

void smi_dumpLarbDebugMsg(unsigned int u4Index, int output_gce_buffer)
{
	unsigned long u4Base = 0;
	/* No verify API in CCF, assume clk is always on */
	unsigned int larbClkEnabled = 1;

	u4Base = get_larb_base_addr(u4Index);
	larbClkEnabled = smi_clk_get_ref_count(u4Index);
	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG3(output_gce_buffer, "Doesn't support reg dump for Larb%d\n", u4Index);
		return;
	} else if (larbClkEnabled == 0) {
		SMIMSG3(output_gce_buffer, "========== SMI LARB%d clock is disable ==========\n", u4Index);
		return;
	}

	SMIMSG3(output_gce_buffer, "========== SMI LARB%d reg dump, CLK: %d ==========\n",
		u4Index, larbClkEnabled);

	smi_dumpper(output_gce_buffer, smi_larb_debug_offset[u4Index], u4Base,
		    smi_larb_debug_offset_num[u4Index], false);
}

#if IS_ENABLED(CONFIG_MACH_MT6771)
void smi_dump_mmsys(int output_gce_buffer)
{
	unsigned long u4Base = (unsigned long)mmsys_config_reg;
	unsigned int smiCommonClkEnabled = 1;

	smiCommonClkEnabled = smi_clk_get_ref_count(SMI_COMMON_REG_INDX);
	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG3(output_gce_buffer, "Doesn't support reg dump for mmsys\n");
		return;
	} else if (smiCommonClkEnabled == 0) {
		SMIMSG3(output_gce_buffer, "========== mmsys mtcmos is disable ==========\n");
		return;
	}

	SMIMSG3(output_gce_buffer, "========== mmsys reg dump, CLK: %d ==========\n", smiCommonClkEnabled);
	smi_dumpper(output_gce_buffer, smi_mmsys_debug_offset, u4Base,
		    SMI_MMSYS_DEBUG_OFFSET_NUM, false);

	SMIMSG3(output_gce_buffer, "latest mdelay kernel time:[%5llu.%06lu], smi larb:%d\n",
		smi_latest_mdelay_sec, smi_latest_mdelay_nsec / 1000, smi_latest_mdelay_larb);
}
#endif

void smi_dumpLarb(unsigned int index)
{
	unsigned long u4Base;

	u4Base = get_larb_base_addr(index);

	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG2("Doesn't support reg dump for Larb%d\n", index);
	} else {
		SMIMSG2("===SMI LARB%d reg dump base 0x%lx===\n", index, u4Base);
		smi_dump_format(u4Base, 0, 0x434);
		smi_dump_format(u4Base, 0xF00, 0xF0C);
	}
}

void smi_dumpCommon(void)
{
	SMIMSG2("===SMI COMMON reg dump base 0x%lx===\n", get_common_base_addr());

	smi_dump_format(get_common_base_addr(), 0x1A0, 0x444);
}

static void smi_dump_format(unsigned long base, unsigned int from, unsigned int to)
{
	int i, j, left;
	unsigned int value[8];

	for (i = from; i <= to; i += 32) {
		for (j = 0; j < 8; j++)
			value[j] = M4U_ReadReg32(base, i + j * 4);

		SMIMSG2("%8x %x %x %x %x %x %x %x %x\n", i, value[0], value[1],
			value[2], value[3], value[4], value[5], value[6], value[7]);
	}

	left = ((from - to) / 4 + 1) % 8;

	if (left) {
		memset(value, 0, 8 * sizeof(unsigned int));

		for (j = 0; j < left; j++)
			value[j] = M4U_ReadReg32(base, i - 32 + j * 4);

		SMIMSG2("%8x %x %x %x %x %x %x %x %x\n", i - 32 + j * 4, value[0],
			value[1], value[2], value[3], value[4], value[5], value[6], value[7]);
	}
}

void smi_dumpDebugMsg(void)
{
	unsigned int u4Index;

	/* SMI COMMON dump, 0 stands for not pass log to CMDQ error dumping messages */
	smi_dumpCommonDebugMsg(0);

	/* dump all SMI LARB */
	/* SMI Larb dump, 0 stands for not pass log to CMDQ error dumping messages */
	for (u4Index = 0; u4Index < SMI_LARB_NUM; u4Index++)
		smi_dumpLarbDebugMsg(u4Index, 0);

	/* dump m4u register status */
	for (u4Index = 0; u4Index < SMI_LARB_NUM; u4Index++)
		smi_dump_larb_m4u_register(u4Index);
}

int smi_debug_bus_hanging_detect(unsigned int larbs, int show_dump)
{
	return smi_debug_bus_hanging_detect_ext(larbs, show_dump, 0);
}

static int get_status_code(int smi_larb_clk_status, int smi_larb_busy_count,
			   int smi_common_busy_count, int max_count)
{
	int status_code = 0;

	if (smi_larb_clk_status != 0) {
		if (smi_larb_busy_count == max_count) {	/* The larb is always busy */
			if (smi_common_busy_count == max_count)	/* smi common is always busy */
				status_code = 1;
			else if (smi_common_busy_count == 0)	/* smi common is always idle */
				status_code = 2;
			else
				status_code = 5;	/* smi common is sometimes busy and idle */
		} else if (smi_larb_busy_count == 0) {	/* The larb is always idle */
			if (smi_common_busy_count == max_count)	/* smi common is always busy */
				status_code = 3;
			else if (smi_common_busy_count == 0)	/* smi common is always idle */
				status_code = 4;
			else
				status_code = 6;	/* smi common is sometimes busy and idle */
		} else {	/* sometime the larb is busy */
			if (smi_common_busy_count == max_count)	/* smi common is always busy */
				status_code = 7;
			else if (smi_common_busy_count == 0)	/* smi common is always idle */
				status_code = 8;
			else
				status_code = 9;	/* smi common is sometimes busy and idle */
		}
	} else {
		status_code = 10;
	}
	return status_code;
}

int smi_debug_bus_hanging_detect_ext(unsigned short larbs, int show_dump, int output_gce_buffer)
{
	return smi_debug_bus_hanging_detect_ext2(larbs, show_dump, output_gce_buffer, 0);
}

int smi_debug_bus_hanging_detect_ext2(unsigned short larbs, int show_dump,
	int output_gce_buffer, int enable_m4u_reg_dump)
{
/* output_gce_buffer = 1, write log into kernel log and CMDQ buffer. */
/* dual_buffer = 0, write log into kernel log only */
/* call m4u dump API when enable_m4u_reg_dump = 1 */

	int i = 0;
	int dump_time = 0;
	int status_code = 0;
	int max_count = 5;
	/* Keep the dump result */
	unsigned int u4Index = 0;
	unsigned long u4Base = 0;
	unsigned char smi_common_busy_count = 0;
	unsigned char smi_larb_busy_count[SMI_LARB_NUM] = { 0 };

	/* dump resister and save resgister status */
	if (show_dump)
		smi_dumpRegDebugMsg(output_gce_buffer);

	for (dump_time = 0; dump_time < max_count; dump_time++) {
		u4Base = get_common_base_addr();
		/* check smi common busy register */
		if (u4Base != SMI_ERROR_ADDR && smi_clk_get_ref_count(SMI_COMMON_REG_INDX) != 0) {
			if ((M4U_ReadReg32(u4Base, 0x440) & (1 << 0)) == 0)
				smi_common_busy_count++;
			if (show_dump != 0)
				smi_dumpCommonDebugMsg(output_gce_buffer);
		}

		for (u4Index = 0; u4Index < SMI_LARB_NUM; u4Index++) {
			u4Base = get_larb_base_addr(u4Index);
			/* check smi larb busy register */
			if (u4Base != SMI_ERROR_ADDR && smi_clk_get_ref_count(u4Index) != 0) {
				if (M4U_ReadReg32(u4Base, 0x0) != 0)
					smi_larb_busy_count[u4Index]++;
				if (show_dump != 0) {
					smi_dumpLarbDebugMsg(u4Index, output_gce_buffer);
					smi_dump_larb_m4u_register(u4Index);
				}
			}
		}
#if IS_ENABLED(CONFIG_MACH_MT6771)
		if (show_dump)
			smi_dump_mmsys(output_gce_buffer);
#endif
	}
	/* Show the checked result */
	for (i = 0; i < SMI_LARB_NUM; i++) {	/* Check each larb */
		if (SMI_DGB_LARB_SELECT(larbs, i)) {
			/* larb i has been selected */
			/* Get status code */
			status_code = get_status_code(smi_clk_get_ref_count(i), smi_larb_busy_count[i],
					smi_common_busy_count, max_count);

			/* Send the debug message according to the final result */
			if (status_code == 10) { /* larb i clock is disable */
				SMIMSG3(output_gce_buffer, "Larb%d clk is disbable, status=%d ==> no need to check\n",
					i, status_code);
				continue;
			}
			SMIMSG3(output_gce_buffer,
				"Larb%d Busy=%d/%d, SMI Common Busy=%d/%d, status=%d ==> Check engine's state first\n",
				i, smi_larb_busy_count[i], max_count, smi_common_busy_count, max_count, status_code);
		}
	}

#ifdef CONFIG_MTK_M4U
	if (enable_m4u_reg_dump) {
		SMIMSG("call m4u API for m4u register dump\n");
		m4u_dump_reg_for_smi_hang_issue();
	}
#endif
	return 0;
}
void smi_dump_clk_status(void)
{
	int i = 0;

	for (i = 0 ; i < SMI_CLK_CNT ; ++i)
		SMIMSG("CLK status of %s is 0x%x", smi_clk_info[i].name,
		M4U_ReadReg32(smi_clk_info[i].base_addr, smi_clk_info[i].offset));
}

void smi_dump_larb_m4u_register(int larb)
{
	unsigned long u4Base = 0;
	unsigned int larbClkEnabled = 0;

	u4Base = get_larb_base_addr(larb);
	larbClkEnabled = smi_clk_get_ref_count(larb);

	if (u4Base == SMI_ERROR_ADDR) {
		SMIMSG("Doesn't support reg dump for Larb%d\n", larb);
		return;
	} else if (larbClkEnabled == 0) {
		SMIMSG("========== SMI LARB%d clock is disable ==========\n", larb);
		return;
	}

#if defined(SMI_MMU_V1)
	SMIMSG("LARB%d m4u register:0x%x\n", larb, M4U_ReadReg32(u4Base, 0xfc0));
#else
	SMIMSG("========== LARB%d m4u secure register ==========\n", larb);
	smi_dumpper(0, smi_m4u_secure_offset, u4Base, SMI_MAX_PORT_NUM, false);
	SMIMSG("========== LARB%d m4u non-secure register ==========\n", larb);
	smi_dumpper(0, smi_m4u_non_secure_offset, u4Base, SMI_MAX_PORT_NUM, false);
#endif
}
