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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_freqhopping.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/dma.h>
#include <mt-plat/sync_write.h>
#include <mt_spm_sleep.h>
#include <mt_spm_reg.h>

#include "mt_dramc.h"

#ifdef CONFIG_OF_RESERVED_MEM
#define DRAM_R0_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r0_dummy_read"
#define DRAM_R1_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r1_dummy_read"
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif
void __iomem *DRAMCAO_CHA_BASE_ADDR;
void __iomem *DRAMCAO_CHB_BASE_ADDR;
void __iomem *DRAMCNAO_CHA_BASE_ADDR;
void __iomem *DRAMCNAO_CHB_BASE_ADDR;
void __iomem *DDRPHY_BASE_ADDR;
void __iomem *TOPCKGEN_BASE_ADDR;
void __iomem *INFRACFG_AO_BASE_ADDR;
void __iomem *SLEEP_BASE_ADDR;
#define DRAM_RSV_SIZE 0x1000

static DEFINE_MUTEX(dram_dfs_mutex);
int highest_dram_data_rate = 0;
unsigned char No_DummyRead = 0;
unsigned int DRAM_TYPE = 0;

/*extern bool spm_vcorefs_is_dvfs_in_porgress(void);*/
#define Reg_Sync_Writel(addr, val)   writel(val, IOMEM(addr))
#define Reg_Readl(addr) readl(IOMEM(addr))
static unsigned int dram_rank_num;
phys_addr_t dram_rank0_addr, dram_rank1_addr;


struct dram_info *g_dram_info_dummy_read = NULL;

static int __init dt_scan_dram_info(unsigned long node,
const char *uname, int depth, void *data)
{
	char *type = (char *)of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg, *endp;
	unsigned long l;

	/* We are scanning "memory" nodes only */
	if (type == NULL) {
		/*
		* The longtrail doesn't have a device_type on the
		* /memory node, so look for the node called /memory@0.
		*/
		if (depth != 1 || strcmp(uname, "memory@0") != 0)
			return 0;
	} else if (strcmp(type, "memory") != 0)
		return 0;

	reg = (const __be32 *)of_get_flat_dt_prop(node,
	(const char *)"reg", (int *)&l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	if (node) {
		g_dram_info_dummy_read =
		(struct dram_info *)of_get_flat_dt_prop(node,
		"orig_dram_info", NULL);
		if (g_dram_info_dummy_read == NULL) {
			No_DummyRead = 1;
			return 0; }

		pr_err("[DRAMC] dram info dram rank number = %d\n",
		g_dram_info_dummy_read->rank_num);

		if (dram_rank_num == SINGLE_RANK) {
			g_dram_info_dummy_read->rank_info[0].start = dram_rank0_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
		} else if (dram_rank_num == DUAL_RANK) {
			g_dram_info_dummy_read->rank_info[0].start = dram_rank0_addr;
			g_dram_info_dummy_read->rank_info[1].start = dram_rank1_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
			pr_err("[DRAMC] dram info dram rank1 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[1].start);
		}
	  else
		pr_err("[DRAMC] dram info dram rank number incorrect !!!\n");
	}

	return node;
}

void *mt_dramc_cha_base_get(void)
{
	return DRAMCAO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_cha_base_get);

void *mt_dramc_chb_base_get(void)
{
	return DRAMCAO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_chb_base_get);

void *mt_dramc_nao_cha_base_get(void)
{
	return DRAMCNAO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_cha_base_get);

void *mt_dramc_nao_chb_base_get(void)
{
	return DRAMCNAO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_chb_base_get);

void *mt_ddrphy_base_get(void)
{
	return DDRPHY_BASE_ADDR;
}
EXPORT_SYMBOL(mt_ddrphy_base_get);

#ifdef CONFIG_MTK_DRAMC_PASR
int enter_pasr_dpd_config(unsigned char segment_rank0,
			   unsigned char segment_rank1)
{
	unsigned int rank_pasr_segment[2];
	/*unsigned int dramc0_spcmd, dramc0_pd_ctrl;
	//unsigned int dramc0_pd_ctrl_2, dramc0_padctl4, dramc0_1E8; */
	unsigned int i, cnt = 1000;
	unsigned int u4value_1E4 = 0;
	unsigned int u4value_1E8 = 0;
	unsigned int u4value_E4 = 0;
	unsigned int u4value_F4 = 0;

	/* request SPM HW SEMAPHORE to avoid race condition */
	cnt = 100;
	do {
		if (cnt < 100)
			udelay(10);

		if (cnt-- == 0) {
			pr_warn("[DRAMC0] can NOT get SPM HW SEMAPHORE!\n");
			return -1; }

		writel(0x1, PDEF_SPM_AP_SEMAPHORE);

		} while ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x0);

	/* pr_warn("[DRAMC0] get SPM HW SEMAPHORE!\n"); */
	rank_pasr_segment[0] = segment_rank0 & 0xFF;	/* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF;	/* for rank1 */
	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n",
	rank_pasr_segment[0], rank_pasr_segment[1]);

	/*Channel-A*/
	u4value_E4 = readl(PDEF_DRAMC0_CHA_REG_0E4);
	u4value_F4 = readl(PDEF_DRAMC0_CHA_REG_0F4);
	u4value_1E8 = readl(PDEF_DRAMC0_CHA_REG_1E8);
	u4value_1E4 = readl(PDEF_DRAMC0_CHA_REG_1E4);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E8) | 0x04000000,
	PDEF_DRAMC0_CHA_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xF7FFFFFF,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_0E4) | 0x00000004,
	PDEF_DRAMC0_CHA_REG_0E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHA_REG_0F4);

	for (i = 0; i < 2; i++) {
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011),
		PDEF_DRAMC0_CHA_REG_088);
		writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
		PDEF_DRAMC0_CHA_REG_1E4);

		cnt = 1000;
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] CHA PASR MRW fail!\n");

				/* release SEMAPHORE to avoid race condition */
				writel(0x1, PDEF_SPM_AP_SEMAPHORE);
				if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1)
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
				else
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n");

				return -1; }
			udelay(1);
			} while ((readl(PDEF_DRAMCNAO_CHA_REG_3B8)
			& 0x00000001) == 0x0);

		writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
		PDEF_DRAMC0_CHA_REG_1E4);
	}

	writel(u4value_E4, PDEF_DRAMC0_CHA_REG_0E4);
	writel(u4value_F4, PDEF_DRAMC0_CHA_REG_0F4);
	writel(u4value_1E4, PDEF_DRAMC0_CHA_REG_1E4);
	writel(u4value_1E8, PDEF_DRAMC0_CHA_REG_1E8);
	writel(0, PDEF_DRAMC0_CHA_REG_088);

	/*Channel-B*/
	u4value_E4 = readl(PDEF_DRAMC0_CHB_REG_0E4);
	u4value_F4 = readl(PDEF_DRAMC0_CHB_REG_0F4);
	u4value_1E8 = readl(PDEF_DRAMC0_CHB_REG_1E8);
	u4value_1E4 = readl(PDEF_DRAMC0_CHB_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E8) | 0x04000000,
	PDEF_DRAMC0_CHB_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xF7FFFFFF,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /*flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_0E4) | 0x00000004,
	PDEF_DRAMC0_CHB_REG_0E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHB_REG_0F4);

	for (i = 0; i < 2; i++) {
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011),
		PDEF_DRAMC0_CHB_REG_088);
		writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
		PDEF_DRAMC0_CHB_REG_1E4);

		cnt = 1000;
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] CHB PASR MRW fail!\n");

				/* release SEMAPHORE to avoid race condition */
				writel(0x1, PDEF_SPM_AP_SEMAPHORE);
				if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1)
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
				else
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n");

				return -1; }
			udelay(1);
			} while ((readl(PDEF_DRAMCNAO_CHB_REG_3B8)
			& 0x00000001) == 0x0);
		writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
		PDEF_DRAMC0_CHB_REG_1E4);
	}

	writel(u4value_E4, PDEF_DRAMC0_CHB_REG_0E4);
	writel(u4value_F4, PDEF_DRAMC0_CHB_REG_0F4);
	writel(u4value_1E4, PDEF_DRAMC0_CHB_REG_1E4);
	writel(u4value_1E8, PDEF_DRAMC0_CHB_REG_1E8);
	writel(0, PDEF_DRAMC0_CHB_REG_088);

	/* pr_warn("[DRAMC0] enter PASR!\n"); */
	/* release SPM HW SEMAPHORE to avoid race condition */
	writel(0x1, PDEF_SPM_AP_SEMAPHORE);
	if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1)
		pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");

	return 0;
}

int exit_pasr_dpd_config(void)
{
	int ret;

	ret = enter_pasr_dpd_config(0, 0);

	return ret;
}
#else
int enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1)
{
	return 0;
}

int exit_pasr_dpd_config(void)
{
	return 0;
}
#endif

#define MEM_TEST_SIZE 0x2000
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
int Binning_DRAM_complex_mem_test(void)
{
	unsigned char *MEM8_BASE;
	unsigned short *MEM16_BASE;
	unsigned int *MEM32_BASE;
	unsigned int *MEM_BASE;
	unsigned long mem_ptr;
	unsigned char pattern8;
	unsigned short pattern16;
	unsigned int i, j, size, pattern32;
	unsigned int value;
	unsigned int len = MEM_TEST_SIZE;
	void *ptr;
	int ret = 1;

	ptr = vmalloc(MEM_TEST_SIZE);

	if (!ptr) {
		/*pr_err("fail to vmalloc\n");*/
		/*ASSERT(0);*/
		ret = -24;
		goto fail;
	}

	MEM8_BASE = (unsigned char *)ptr;
	MEM16_BASE = (unsigned short *)ptr;
	MEM32_BASE = (unsigned int *)ptr;
	MEM_BASE = (unsigned int *)ptr;
	/*pr_warn("Test DRAM start address 0x%lx\n", (unsigned long)ptr);*/
	pr_warn("Test DRAM start address %p\n", ptr);
	pr_warn("Test DRAM SIZE 0x%x\n", MEM_TEST_SIZE);
	size = len >> 2;

	/* === Verify the tied bits (tied high) === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0;

	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0) {
			vfree(ptr);
			/* return -1; */
			ret = -1;
			goto fail;
		} else
			MEM32_BASE[i] = 0xffffffff;
	}

	/* === Verify the tied bits (tied low) === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			/* return -2; */
			ret = -2;
			goto fail;
		} else
			MEM32_BASE[i] = 0x00;
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = 0; i < len; i++)
		MEM8_BASE[i] = pattern8++;
	pattern8 = 0x00;
	for (i = 0; i < len; i++) {
		if (MEM8_BASE[i] != pattern8++) {
			vfree(ptr);
			/* return -3; */
			ret = -3;
			goto fail;
		}
	}

	/* === Verify pattern 2 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = j = 0; i < len; i += 2, j++) {
		if (MEM8_BASE[i] == pattern8)
			MEM16_BASE[j] = pattern8;
		if (MEM16_BASE[j] != pattern8) {
			vfree(ptr);
			/* return -4; */
			ret = -4;
			goto fail;
		}
		pattern8 += 2;
	}

	/* === Verify pattern 3 (0x00~0xffff) === */
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++)
		MEM16_BASE[i] = pattern16++;
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++) {
		if (MEM16_BASE[i] != pattern16++) {
			vfree(ptr);
			/* return -5; */
			ret = -5;
			goto fail;
		}
	}

	/* === Verify pattern 4 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++)
		MEM32_BASE[i] = pattern32++;
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++) {
		if (MEM32_BASE[i] != pattern32++) {
			vfree(ptr);
			/* return -6; */
			ret = -6;
			goto fail;
		}
	}

	/* === Pattern 5: Filling memory range with 0x44332211 === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0x44332211;

	/* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x44332211) {
			vfree(ptr);
			/* return -7; */
			ret = -7;
			goto fail;
		} else {
			MEM32_BASE[i] = 0xa5a5a5a5;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 0h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a5a5) {
			vfree(ptr);
			/* return -8; */
			ret = -8;
			goto fail;
		} else {
			MEM8_BASE[i * 4] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 2h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a500) {
			vfree(ptr);
			/* return -9; */
			ret = -9;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 2] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 1h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa500a500) {
			vfree(ptr);
			/* return -10; */
			ret = -10;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 1] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 3h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5000000) {
			vfree(ptr);
			/* return -11; */
			ret = -11;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 3] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 1h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x00000000) {
			vfree(ptr);
			/* return -12; */
			ret = -12;
			goto fail;
		} else {
			MEM16_BASE[i * 2 + 1] = 0xffff;
		}
	}

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 0h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffff0000) {
			vfree(ptr);
			/* return -13; */
			ret = -13;
			goto fail;
		} else {
			MEM16_BASE[i * 2] = 0xffff;
		}
	}
    /*===  Read Check === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			/* return -14; */
			ret = -14;
			goto fail;
		}
	}

    /************************************************
    * Additional verification
    ************************************************/
	/* === stage 1 => write 0 === */

	for (i = 0; i < size; i++)
		MEM_BASE[i] = PATTERN1;

	/* === stage 2 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];

		if (value != PATTERN1) {
			vfree(ptr);
			/* return -15; */
			ret = -15;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 3 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			/* return -16; */
			ret = -16;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 4 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			/* return -17; */
			ret = -17;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 5 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			/* return -18; */
			ret = -18;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 6 => read 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			/* return -19; */
			ret = -19;
			goto fail;
		}
	}

	/* === 1/2/4-byte combination test === */
	mem_ptr = (unsigned long)MEM_BASE;
	while (mem_ptr < ((unsigned long)MEM_BASE + (size << 2))) {
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
	}
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != 0x12345678) {
			vfree(ptr);
			/* return -20; */
			ret = -20;
			goto fail;
		}
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	MEM8_BASE[0] = pattern8;
	for (i = 0; i < size * 4; i++) {
		unsigned char waddr8, raddr8;

		waddr8 = i + 1;
		raddr8 = i;
		if (i < size * 4 - 1)
			MEM8_BASE[waddr8] = pattern8 + 1;
		if (MEM8_BASE[raddr8] != pattern8) {
			vfree(ptr);
			/* return -21; */
			ret = -21;
			goto fail;
		}
		pattern8++;
	}

	/* === Verify pattern 2 (0x00~0xffff) === */
	pattern16 = 0x00;
	MEM16_BASE[0] = pattern16;
	for (i = 0; i < size * 2; i++) {
		if (i < size * 2 - 1)
			MEM16_BASE[i + 1] = pattern16 + 1;
		if (MEM16_BASE[i] != pattern16) {
			vfree(ptr);
			/* return -22; */
			ret = -22;
			goto fail;
		}
		pattern16++;
	}
	/* === Verify pattern 3 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	MEM32_BASE[0] = pattern32;
	for (i = 0; i < size; i++) {
		if (i < size - 1)
			MEM32_BASE[i + 1] = pattern32 + 1;
		if (MEM32_BASE[i] != pattern32) {
			vfree(ptr);
			/* return -23; */
			ret = -23;
			goto fail;
		}
		pattern32++;
	}
	pr_warn("complex R/W mem test pass\n");

fail:
	vfree(ptr);
	return ret;
}

unsigned int ucDram_Register_Read(unsigned int u4reg_addr)
{
	return 0; /* for mbw dummy use */
}

/************************************************
* input parameter:
*************************************************/
unsigned int get_dram_data_rate_from_reg(void)
{
	unsigned char REF_CLK = 52;
	unsigned int u2real_freq = 0;
	unsigned char mpdiv_shu_sel = 0, pll_shu_sel = 0;
	unsigned int u4MPDIV_IN_SEL = 0;
	unsigned char u1MPDIV_Sel = 0;
	unsigned int u4DDRPHY_PLL12_Addr[3] = {0x42c, 0x448, 0x9c4};
	unsigned int u4DDRPHY_PLL1_Addr[3] =  {0x400, 0x640, 0xa40};
	unsigned int u4DDRPHY_PLL3_Addr[3] =  {0x408, 0x644, 0xa44};

	if (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028)) & (1<<17))
		return 0;

		mpdiv_shu_sel = (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028))>>8) & 0x7;
		pll_shu_sel = readl(IOMEM(DDRPHY_BASE_ADDR + 0x63c)) & 0x3;

	u4MPDIV_IN_SEL =
	readl(IOMEM(DDRPHY_BASE_ADDR +
	(u4DDRPHY_PLL12_Addr[mpdiv_shu_sel]))) & (1 << 17);
	u1MPDIV_Sel =
	(readl(IOMEM(DDRPHY_BASE_ADDR +
	(u4DDRPHY_PLL12_Addr[mpdiv_shu_sel]))) >> 10) & 0x3;

	if (u4MPDIV_IN_SEL == 0) {/* PHYPLL */
		/* FREQ_LCPLL = = FREQ_XTAL*(RG_*_RPHYPLL_FBKDIV+1)
		*2^(RG_*_RPHYPLL_FBKSEL)/2^(RG_*_RPHYPLL_PREDIV) */
		unsigned int u4DDRPHY_PLL1 =
		readl(IOMEM(DDRPHY_BASE_ADDR +
		(u4DDRPHY_PLL1_Addr[0])));
		unsigned int u4FBKDIV = (u4DDRPHY_PLL1>>24) & 0x7F;
		unsigned char u1FBKSEL = (u4DDRPHY_PLL1>>20) & 0x3;
		unsigned char u1PREDIV = (u4DDRPHY_PLL1>>18) & 0x3;

		if (u1PREDIV == 3)
			u1PREDIV = 2;
		u2real_freq = (unsigned int)
		(((u4FBKDIV+1)*REF_CLK)<<u1FBKSEL)>>(u1PREDIV);
		} else {
			/* Freq(VCO clock) = FREQ_XTAL*(RG_RCLRPLL_SDM_PCW)
			/2^(RG_*_RCLRPLL_PREDIV)/2^(RG_*_RCLRPLL_POSDIV) */
			/* Freq(DRAM clock)= Freq(VCO clock)/4 */
			unsigned int u4DDRPHY_PLL3 =
			readl(IOMEM(DDRPHY_BASE_ADDR +
			(u4DDRPHY_PLL3_Addr[pll_shu_sel])));
			unsigned int u4FBKDIV = (u4DDRPHY_PLL3>>24) & 0x7F;
			unsigned char u1PREDIV = (u4DDRPHY_PLL3>>18) & 0x3;
			unsigned char u1POSDIV = (u4DDRPHY_PLL3>>16) & 0x3;
			unsigned char u1FBKSEL =
			(readl(IOMEM(DDRPHY_BASE_ADDR + 0x43c)) >> 12) & 0x1;

			u2real_freq = (unsigned int)
			((((u4FBKDIV)*REF_CLK) << u1FBKSEL >> u1PREDIV)
			>> (u1POSDIV)) >> 2;
		}

	u2real_freq = u2real_freq >> u1MPDIV_Sel;
	/* pr_err("[DRAMC] GetPhyFrequency: %d\n", u2real_freq); */
	return u2real_freq;
}

/************************************************
* input parameter:
*************************************************/
unsigned int get_dram_data_rate(void)
{
	unsigned int MEMPLL_FOUT = 0;

	return 0; /* will sync code from SA later */
	MEMPLL_FOUT = get_dram_data_rate_from_reg() << 1;

	/* DVFS owner to request provide a spec. frequency,
	not real frequency */
	if (MEMPLL_FOUT == 1820)
		MEMPLL_FOUT = 1866;

	return MEMPLL_FOUT;
}

unsigned int read_dram_temperature(unsigned char channel)
{
	unsigned int value = 0;

	if (channel == CHANNEL_A) {
		value =
		(readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
	}	else if (channel == CHANNEL_B) {
		value =
		(readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
		}

	return value;
}

unsigned int get_shuffle_status(void)
{
	return 0; /* readl(PDEF_DRAMC0_CHA_REG_010) & 0x6; */
	/* HPM = 0, LPM = 1, ULPM = 2; 0x100040e4[2:1] */

}

int get_ddr_type(void)
{
	return DRAM_TYPE;

}
int dram_steps_freq(unsigned int step)
{
	int freq = -1;

	switch (step) {
	case 0:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 1866;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X))
			freq = 3200;
		break;
	case 1:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 1333;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X))
			freq = 2667;
		break;
	case 2:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 933;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X))
			freq = 1600;
		break;
	default:
		return -1;
	}
	return freq;
}

int dram_can_support_fh(void)
{
	if (No_DummyRead)
		return 0;
	else
		return 1;
}

#ifdef CONFIG_OF_RESERVED_MEM
int dram_dummy_read_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;

	if (strstr(DRAM_R0_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			pr_err("[DRAMC] Can NOT reserve memory for Rank0\n");
			No_DummyRead = 1;
			return 0;
		}
		dram_rank0_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank0_addr = %pa, size = 0x%x\n",
				&dram_rank0_addr, rsize);
	}

	if (strstr(DRAM_R1_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			pr_err("[DRAMC] Can NOT reserve memory for Rank1\n");
			No_DummyRead = 1;
			return 0;
		}
		dram_rank1_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank1_addr = %pa, size = 0x%x\n",
				&dram_rank1_addr, rsize);
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(dram_reserve_r0_dummy_read_init,
DRAM_R0_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(dram_reserve_r1_dummy_read_init,
DRAM_R1_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
#endif
static ssize_t complex_mem_test_show(struct device_driver *driver, char *buf)
{
	int ret;

	ret = Binning_DRAM_complex_mem_test();
	if (ret > 0)
		return snprintf(buf, PAGE_SIZE, "MEM Test all pass\n");
	else
		return snprintf(buf, PAGE_SIZE, "MEM TEST failed %d\n", ret);
}

static ssize_t complex_mem_test_store(struct device_driver *driver,
const char *buf, size_t count)
{
	/*snprintf(buf, "do nothing\n");*/
	return count;
}

static ssize_t read_dram_data_rate_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM data rate = %d\n",
	get_dram_data_rate());
}

static ssize_t read_dram_data_rate_store(struct device_driver *driver,
const char *buf, size_t count)
{
	return count;
}


DRIVER_ATTR(emi_clk_mem_test, 0664,
complex_mem_test_show, complex_mem_test_store);

DRIVER_ATTR(read_dram_data_rate, 0664,
read_dram_data_rate_show, read_dram_data_rate_store);

/*DRIVER_ATTR(dram_dfs, 0664, dram_dfs_show, dram_dfs_store);*/

static int dram_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("[DRAMC0] module probe.\n");

	return ret;
}

static int dram_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dram_of_ids[] = {
	{.compatible = "mediatek,dramc",},
	{}
};
#endif

static struct platform_driver dram_test_drv = {
	.probe = dram_probe,
	.remove = dram_remove,
	.driver = {
		.name = "emi_clk_test",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = dram_of_ids,
#endif
		},
};

static int dram_dt_init(void)
{
	int ret = 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc");
	if (node) {
		DRAMCAO_CHA_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_CHA_BASE_ADDR @ %p\n",
		DRAMCAO_CHA_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMC0 CHA compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_conf_b");
	if (node) {
		DRAMCAO_CHB_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_CHB_BASE_ADDR @ %p\n",
		DRAMCAO_CHB_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMC0 CHB compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,ddrphy");
	if (node) {
		DDRPHY_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DDRPHY_BASE_ADDR @ %p\n", DDRPHY_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DDRPHY compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_nao");
	if (node) {
		DRAMCNAO_CHA_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_CHA_BASE_ADDR @ %p\n",
		DRAMCNAO_CHA_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMCNAO CHA compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_b_nao");
	if (node) {
		DRAMCNAO_CHB_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_CHB_BASE_ADDR @ %p\n",
		DRAMCNAO_CHB_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMCNAO CHB compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (node) {
		TOPCKGEN_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get TOPCKGEN_BASE_ADDR @ %p\n",
		TOPCKGEN_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find TOPCKGEN compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (node) {
		INFRACFG_AO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get INFRACFG_AO_BASE_ADDR @ %p\n",
		INFRACFG_AO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find INFRACFG_AO compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (node) {
		SLEEP_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get SLEEP_BASE_ADDR @ %p\n",
		INFRACFG_AO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find SLEEP_BASE_ADDR compatible node\n");
		return -1;
	}

	if (of_scan_flat_dt(dt_scan_dram_info, NULL) > 0) {
		pr_err("[DRAMC]find dt_scan_dram_info\n");
	} else {
		pr_err("[DRAMC]can't find dt_scan_dram_info\n");
		return -1;
	}

	return ret;
}

/* int __init dram_test_init(void) */
static int __init dram_test_init(void)
{
	int ret = 0;

	ret = dram_dt_init();
	if (ret) {
		pr_warn("[DRAMC] Device Tree Init Fail\n");
		return ret;
	}

	ret = platform_driver_register(&dram_test_drv);
	if (ret) {
		pr_warn("fail to create dram_test platform driver\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}

	DRAM_TYPE = readl(PDEF_DRAMC0_CHA_REG_010) & 0xC00;
	pr_err("[DRAMC Driver] dram type =%d\n", DRAM_TYPE);

	pr_err("[DRAMC Driver] Dram Data Rate = %d\n", get_dram_data_rate());

	if (dram_can_support_fh())
		pr_err("[DRAMC Driver] dram can support DFS\n");
	else
		pr_err("[DRAMC Driver] dram can not support DFS\n");

	return ret;
}

static void __exit dram_test_exit(void)
{
	platform_driver_unregister(&dram_test_drv);
}

postcore_initcall(dram_test_init);
module_exit(dram_test_exit);

MODULE_DESCRIPTION("MediaTek DRAMC Driver v0.1");
