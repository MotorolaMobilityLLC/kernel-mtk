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
#include <linux/types.h>
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
#include <asm/setup.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#endif

#include <mt-plat/mt_io.h>
#include <mt-plat/dma.h>
#include <mt-plat/sync_write.h>
/* #include <mach/mt_reg_base.h> */
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif /* #ifdef CONFIG_MTK_CLKMGR */

#include "mt_dramc.h"

/* #include <mach/mt_typedefs.h> */
/* #include <mach/memory.h> */
/* #include <mach/mt_sleep.h> */
/* #include <mach/mt_dcm.h> */
/* #include <mach/mt_ccci_common.h> */
/* #include <mach/mt_vcore_dvfs.h> */

#ifdef FREQ_HOPPING_TEST
#include <mach/mt_freqhopping.h>
#endif

#ifdef VCORE1_ADJ_TEST
#include <mt-plat/upmu_common.h>
#include <mach/upmu_hw.h>
#endif

#ifdef CONFIG_OF_RESERVED_MEM
#define DRAM_R0_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r0_dummy_read"
#define DRAM_R1_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r1_dummy_read"
#define DRAM_RSV_SIZE 0x1000
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif

static void __iomem *CQDMA_BASE_ADDR;
static void __iomem *DRAMCAO_BASE_ADDR;
static void __iomem *DDRPHY_BASE_ADDR;
static void __iomem *DRAMCNAO_BASE_ADDR;

volatile unsigned char *dst_array_v;
volatile unsigned int dst_array_p;
static char dfs_dummy_buffer[BUFF_LEN] __aligned(PAGE_SIZE);
int init_done = 0;
static DEFINE_MUTEX(dram_dfs_mutex);
int org_dram_data_rate = 0;
static unsigned int dram_rank_num;
phys_addr_t dram_rank0_addr, dram_rank1_addr;


/* #include <mach/emi_bwl.h> */
#define DRAMC_LPDDR2    (0x1e0)
#define DRAMC_PADCTL4   (0x0e4)
#define DRAMC_ACTIM1    (0x1e8)

#ifndef CONFIG_MTK_CLKMGR
struct clk *clk_cqdma;
#endif /* #ifndef CONFIG_MTK_CLKMGR */

int get_ddr_type(void)
{
	unsigned int value;

	value = ucDram_Register_Read(DRAMC_LPDDR2);
	if ((value >> 28) & 0x1) {	/* check LPDDR2_EN */
		return TYPE_LPDDR2;
	}

	value = ucDram_Register_Read(DRAMC_PADCTL4);
	if ((value >> 7) & 0x1) {	/* check DDR3_EN */
		return TYPE_PCDDR3;
	}

	value = ucDram_Register_Read(DRAMC_ACTIM1);
	if ((value >> 28) & 0x1) {	/* check LPDDR3_EN */
		return TYPE_LPDDR3;
	}

	return TYPE_DDR1;
}

/*----------------------------------------------------------------------------------------*/
/* Sampler function for profiling purpose                                                 */
/*----------------------------------------------------------------------------------------*/
typedef void (*dram_sampler_func) (unsigned int);
static dram_sampler_func g_pDramFreqSampler;

void mt_dramfreq_setfreq_registerCB(dram_sampler_func pCB)
{
	g_pDramFreqSampler = pCB;
}
EXPORT_SYMBOL(mt_dramfreq_setfreq_registerCB);

void mt_dramfreq_setfreq_notify(unsigned int freq)
{
	if (NULL != g_pDramFreqSampler)
		g_pDramFreqSampler(freq);
}

static int check_dramc_base_addr(void)
{
	if ((!DRAMCAO_BASE_ADDR) || (!DDRPHY_BASE_ADDR) || (!DRAMCNAO_BASE_ADDR))
		return -1;
	else
		return 0;
}

static int check_cqdma_base_addr(void)
{
	if (!CQDMA_BASE_ADDR)
		return -1;
	else
		return 0;
}

void *mt_dramc_base_get(void)
{
	return DRAMCAO_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_base_get);

void *mt_dramc_nao_base_get(void)
{
	return DRAMCNAO_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_base_get);

void *mt_ddrphy_base_get(void)
{
	return DDRPHY_BASE_ADDR;
}
EXPORT_SYMBOL(mt_ddrphy_base_get);

#ifdef VCORE1_ADJ_TEST
void pmic_voltage_read(unsigned int nAdjust)
{
	int ret_val = 0;
	unsigned int OldVcore1 = 0;
	unsigned int OldVmem = 0, OldVmem1 = 0;

	pr_warn("[PMIC]pmic_voltage_read : \r\n");

	ret_val = pmic_read_interface(MT6328_VCORE1_CON11, &OldVcore1, 0x7F, 0);
	ret_val = pmic_read_interface(MT6328_SLDO_ANA_CON0, &OldVmem, 0x7F, 0);
	ret_val = pmic_read_interface(MT6328_SLDO_ANA_CON1, &OldVmem1, 0x0F, 8);

	pr_warn("[Vcore] MT6328_VCORE1_CON11=0x%x,\r\n[Vmem] MT6328_SLDO_ANA_CON0/1=0x%x 0x%x\r\n",
		OldVcore1, OldVmem, OldVmem1);
}

void pmic_Vcore_adjust(int nAdjust)
{
	switch (nAdjust) {
	case 0:		/* HV 1280MHz */
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x6B, 0x7F, 0);*/    /* 1.265V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x65, 0x7F, 0);	/* 1.230V */
		break;
	case 1:		/* HV 938MHz */
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x59, 0x7F, 0);*/    /* 1.155V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x54, 0x7F, 0);	/* 1.124V */
		break;
	case 2:		/* NV 1280MHz */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		break;
	case 3:		/* NV 938MHz */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x48, 0x7F, 0);	/* 1.05V */
		break;
	case 4:		/* LV 1280MHz */
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x46, 0x7F, 0);*/    /* 1.035V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x4B, 0x7F, 0);	/* 1.070V */
		break;
	case 5:		/* LV 938MHz */
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x38, 0x7F, 0);*/    /* 0.945V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x3C, 0x7F, 0);	/* 0.976V */
		break;
	default:
		pr_warn("Enter incorrect config number !!!\n");
		break;
	}

}

void pmic_Vmem_adjust(int nAdjust)
{
	switch (nAdjust) {
	case 0:
		pmic_config_interface(MT6328_SLDO_ANA_CON0, 0, 0x3, 0);	/* 1.24V */
		break;
	case 1:
		pmic_config_interface(MT6328_SLDO_ANA_CON0, 1, 0x3, 0);	/* 1.39V */
		break;
	case 2:
		pmic_config_interface(MT6328_SLDO_ANA_CON0, 2, 0x3, 0);	/* 1.54V */
		break;
	default:
		pmic_config_interface(MT6328_SLDO_ANA_CON0, 0, 0x3, 0);	/* 1.24V */
		break;
	}
}

void pmic_Vmem_Cal_adjust(int nAdjust)
{
	switch (nAdjust) {
	case 0:
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x0D, 0xF, 8);	/* +0.06V */
		break;
	case 1:
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V */
		break;
	case 2:
		/*pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x04, 0xF, 8);*/    /* -0.08V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x05, 0xF, 8);	/* -1.00V */
		break;
	default:
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x00, 0xF, 8);	/* 1.24V */
		break;
	}
}

void pmic_HQA_NoSSC_Voltage_adjust(int nAdjust)
{
	switch (nAdjust) {
	case 0:		/* Vm = 1.18V & Vc = 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x03, 0xF, 8);	/* -0.06V (1.18V) */
		pr_warn("========== Vm = 1.18V & Vc = 1.15V ==========\r\n");
		break;
	case 1:		/* Vm = 1.26V & Vc = 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x0F, 0xF, 8);	/* +0.02V (1.26V) */
		pr_warn("========== Vm = 1.26V & Vc = 1.15V ==========\r\n");
		break;
	case 2:		/* Vm = 1.22V & Vc = 1.11V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x52, 0x7F, 0);	/* 1.11V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x52, 0x7F, 0);	/* 1.11V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== Vm = 1.22V & Vc = 1.11V ==========\r\n");
		break;
	case 3:		/* Vm = 1.22V & Vc = 1.19V */
		pmic_config_interface(MT6328_VCORE1_CON11, 0x5F, 0x7F, 0);	/* 1.19V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x5F, 0x7F, 0);	/* 1.19V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== Vm = 1.22V & Vc = 1.19V ==========\r\n");
		break;
	case 4:		/*NV*/
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== NV ==========\r\n");
		break;
	default:	/*NV*/
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== NV ==========\r\n");
		break;
	}
}

void pmic_HQA_Voltage_adjust(int nAdjust)
{
	switch (nAdjust) {
	case 0:		/*HVcHVm*/
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x6B, 0x7F, 0);*/    /* 1.265V */
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x71, 0x7F, 0);	/* 1.310V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x71, 0x7F, 0);	/* 1.310V */
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x65, 0x7F, 0);	/* 1.230V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x65, 0x7F, 0);	/* 1.230V */
#endif
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x0D, 0xF, 8);	/* +0.06V (1.3V) */
		pr_warn("========== HVcHVm ==========\r\n");
		break;
	case 1:		/*HVcLVm*/
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x6B, 0x7F, 0);*/    /* 1.265V */
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x71, 0x7F, 0);	/* 1.310V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x71, 0x7F, 0);	/* 1.310V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x04, 0xF, 8);	/* -0.08V (1.16V) */
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x65, 0x7F, 0);	/* 1.230V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x65, 0x7F, 0);	/* 1.230V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x05, 0xF, 8);	/* -1.00V (1.14V) */
#endif
		pr_warn("========== HVcLVm ==========\r\n");
		break;
	case 2:		/*LVcHVm*/
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x46, 0x7F, 0);*/    /* 1.035V */
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x54, 0x7F, 0);	/* 1.125V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x54, 0x7F, 0);	/* 1.125V */
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x4B, 0x7F, 0);	/* 1.070V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x4B, 0x7F, 0);	/* 1.070V */
#endif
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x0D, 0xF, 8);	/* +0.06V (1.3V) */
		pr_warn("========== LVcHVm ==========\r\n");
		break;
	case 3:		/*LVcLVm*/
		/*pmic_config_interface(MT6328_VCORE1_CON11, 0x46, 0x7F, 0);*/    /* 1.035V */
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x54, 0x7F, 0);	/* 1.125V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x54, 0x7F, 0);	/* 1.125V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x04, 0xF, 8);	/* -0.08V (1.16V)*/
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x4B, 0x7F, 0);	/* 1.070V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x4B, 0x7F, 0);	/* 1.070V */
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x05, 0xF, 8);	/* -1.00V (1.14V) */
#endif
		pr_warn("========== LVcLVm ==========\r\n");
		break;
	case 4:		/*NV*/
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x68, 0x7F, 0);	/* 1.25V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x68, 0x7F, 0);	/* 1.25V */
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
#endif
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== NV ==========\r\n");
		break;
	default:	/*NV*/
#if defined(CONFIG_ARCH_MT6753)
		pmic_config_interface(MT6328_VCORE1_CON11, 0x68, 0x7F, 0);	/* 1.25V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x68, 0x7F, 0);	/* 1.25V */
#else
		pmic_config_interface(MT6328_VCORE1_CON11, 0x58, 0x7F, 0);	/* 1.15V */
		pmic_config_interface(MT6328_VCORE1_CON12, 0x58, 0x7F, 0);	/* 1.15V */
#endif
		pmic_config_interface(MT6328_SLDO_ANA_CON1, 0x01, 0xF, 8);	/* -0.02V (1.22V) */
		pr_warn("========== NV ==========\r\n");
		break;
	}
}
#endif

#if defined(CONFIG_ARCH_MT6753)
int enter_pasr_dpd_config(unsigned char segment_rank0,
			   unsigned char segment_rank1)
{
	/* for D-3, support run time MRW */
	unsigned int rank_pasr_segment[2];
	unsigned int dramc0_spcmd, dramc0_pd_ctrl, dramc0_padctl4;
	unsigned int i, cnt = 1000;

	rank_pasr_segment[0] = segment_rank0 & 0xFF;	/* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF;	/* for rank1 */
	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n", rank_pasr_segment[0], rank_pasr_segment[1]);

	/* backup original data */
	dramc0_spcmd = readl(PDEF_DRAMC0_REG_1E4);
	dramc0_pd_ctrl = readl(PDEF_DRAMC0_REG_1DC);
	dramc0_padctl4 = readl(PDEF_DRAMC0_REG_0E4);

	/* Set MIOCKCTRLOFF(0x1dc[26])=1: not stop to DRAM clock! */
	writel(dramc0_pd_ctrl | 0x04000000, PDEF_DRAMC0_REG_1DC);

	/* fix CKE */
	writel(dramc0_padctl4 | 0x00000004, PDEF_DRAMC0_REG_0E4);

	udelay(1);

	for (i = 0; i < 2; i++) {
		/* set MRS settings include rank number, segment information and MRR17 */
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011), PDEF_DRAMC0_REG_088);
		/* Mode register write command enable */
		writel(0x00000001, PDEF_DRAMC0_REG_1E4);

		/* wait MRW command response */
		/* wait >1us */
		/* gpt_busy_wait_us(1); */
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] PASR MRW fail!\n");
				return -1;
			}
			udelay(1);
		} while ((readl(PDEF_DRAMC0_REG_3B8) & 0x00000001) == 0x0);
		mb();	/* make sure the DRAM have been read */

		/* Mode register write command disable */
		writel(0x0, PDEF_DRAMC0_REG_1E4);
	}

	/* release fix CKE */
	writel(dramc0_padctl4, PDEF_DRAMC0_REG_0E4);

	/* recover Set MIOCKCTRLOFF(0x1dc[26]) */
	/* Set MIOCKCTRLOFF(0x1DC[26])=0: stop to DRAM clock */
	writel(dramc0_pd_ctrl, PDEF_DRAMC0_REG_1DC);

	/* writel(0x00000004, PDEF_DRAMC0_REG_088); */
	pr_warn("[DRAMC0] PASR offset 0x88 = 0x%x\n", readl(PDEF_DRAMC0_REG_088));
	writel(dramc0_spcmd, PDEF_DRAMC0_REG_1E4);

	return 0;
#if 0
	if (segment_rank1 == 0xFF) {	/*all segments of rank1 are not reserved -> rank1 enter DPD*/
		slp_dpd_en(1);
	}
#endif

	/*slp_pasr_en(1, segment_rank0 | (segment_rank1 << 8));*/
}

int exit_pasr_dpd_config(void)
{
	int ret;
	/*slp_dpd_en(0);*/
	/*slp_pasr_en(0, 0);*/
	ret = enter_pasr_dpd_config(0, 0);

	return ret;
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

	ptr = vmalloc(PAGE_SIZE * 2);
	MEM8_BASE = (unsigned char *)ptr;
	MEM16_BASE = (unsigned short *)ptr;
	MEM32_BASE = (unsigned int *)ptr;
	MEM_BASE = (unsigned int *)ptr;
	pr_warn("Test DRAM start address 0x%lx\n", (unsigned long)ptr);
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

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
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

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
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

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
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

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
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

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
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

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
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

unsigned int ucDram_Register_Read(unsigned long u4reg_addr)
{
	unsigned int pu4reg_value;

	if (check_dramc_base_addr() == -1) {
		pr_err("[DRAMC] Access-R DRAMC base address is NULL!!!\n");
		/* ASSERT(0); */ /* need porting*/
	}

	pu4reg_value = (*(volatile unsigned int *)(DRAMCAO_BASE_ADDR + (u4reg_addr))) |
					(*(volatile unsigned int *)(DDRPHY_BASE_ADDR + (u4reg_addr))) |
					(*(volatile unsigned int *)(DRAMCNAO_BASE_ADDR + (u4reg_addr)));
	return pu4reg_value;
}

void ucDram_Register_Write(unsigned long u4reg_addr, unsigned int u4reg_value)
{
	if (check_dramc_base_addr() == -1) {
		pr_err("[DRAMC] Access-W DRAMC base address is NULL!!!\n");
		/* ASSERT(0); */ /* need porting*/
	}

	(*(volatile unsigned int *)(DRAMCAO_BASE_ADDR + (u4reg_addr))) = u4reg_value;
	(*(volatile unsigned int *)(DDRPHY_BASE_ADDR + (u4reg_addr))) = u4reg_value;
	(*(volatile unsigned int *)(DRAMCNAO_BASE_ADDR + (u4reg_addr))) = u4reg_value;
	mb();	/* make sure the DRAM have been read */
}

unsigned int dram_support_1600_freq(void)
{
	unsigned int spbin = ((get_devinfo_with_index(5) & (1<<20)) && (get_devinfo_with_index(5) & (1<<21)) &&
				(get_devinfo_with_index(5) & (1<<22))) ? 1 : 0;	/* MT6753 or MT6753T */
	unsigned int ddr1466 = (get_devinfo_with_index(15) & (1<<8)) ? 1 : 0;	/* DDR1466 or DDR1600 */
	unsigned result = 0;

	if (spbin == 1) {
		if (ddr1466 == 1)
			result = 0;
		else
			result = 1;
	} else
		result = 0;

	return result;
}

int dram_fh_steps_freq(unsigned int step)
{
	int freq;

#if defined(CONFIG_ARCH_MT6735)
	unsigned int d1plus = ((get_devinfo_with_index(47) & (1<<31)) && !(get_devinfo_with_index(47) & (1<<29)) &&
				!(get_devinfo_with_index(47) & (1<<28))) ? 1 : 0;
	unsigned int ddr_type = get_ddr_type();

#if defined(CONFIG_MTK_EFUSE_DOWNGRADE)
	d1plus = 0;
#endif

	switch (step) {
	case 0:
		if (d1plus == 1)
			freq = 1466;
		else
			if (ddr_type == TYPE_LPDDR2)
				freq = 1066;
			else
				freq = 1280;
		break;
	case 1:
		if (d1plus == 1)
			freq = 1313;
		else
			freq = 938;
		break;
	default:
		return -1;
	}
#elif defined(CONFIG_ARCH_MT6735M)
	unsigned int d2plus = ((get_devinfo_with_index(47) & (1<<31)) &&
				(get_devinfo_with_index(47) & (1<<29))) ? 1 : 0;

#if defined(CONFIG_MTK_EFUSE_DOWNGRADE)
	d2plus = 0;
#endif

	switch (step) {
	case 0:
		if (d2plus == 1)
			freq = 1280;
		else
			freq = 1066;
		break;
	case 1:
		if (d2plus == 1)
			freq = 938;
		else
			freq = 800;
		break;
	default:
		return -1;
	}
#elif defined(CONFIG_ARCH_MT6753)
	unsigned int temp = dram_support_1600_freq();

	switch (step) {
	case 0:
		/* need porting to eFuse check for MT6753T (1600) */
		if (temp == 1)
			freq = 1600;
		else
			freq = 1466;
		break;
	case 1:
		freq = 1313;
		break;
	default:
		return -1;
	}
#else
	pr_warn("DRAM FH steps not define\n");
#endif

	return freq;
}

int dram_can_support_fh(void)
{
	unsigned int value;
	int ret = 0;

	value = ucDram_Register_Read(0xf4);
	/*value = (0x1 <<15);
	pr_debug("dummy regs 0x%x\n", value);
	pr_debug("check 0x%x\n",(0x1 <<15));*/

	if (value & (0x1 << 15))
		/*pr_debug("DFS could be enabled\n");*/
		ret = 1;
	else
		/*pr_debug("DFS could NOT be enabled\n");*/
		ret = 0;

	return ret;
}

#ifdef FREQ_HOPPING_TEST
int dram_do_dfs_by_fh(unsigned int freq)
{
	int detect_fh = dram_can_support_fh();
	unsigned int target_dds;
#if defined(CONFIG_ARCH_MT6735)
	unsigned int ddr_type = get_ddr_type();
#endif

	if ((check_cqdma_base_addr() == -1) || (check_dramc_base_addr() == -1)) {
		pr_err("[DRAMC] DFS DRAMC/CQDMA base addr fail!");
		return -1;
	}

	if (detect_fh == 0)
		return -1;

	switch (freq) {
#if defined(CONFIG_ARCH_MT6735)
	case 938000:
		if (ddr_type == TYPE_LPDDR2)
			target_dds = 0x10cefe;	/* ///< 938Mbps for LPDDR2 */
		else
			target_dds = 0xe38fe;	/* ///< 938Mbps for LPDDR3 */
		break;
	case 1066000:
		target_dds = 0x131A2D;	/* ///< 1066Mbps */
		break;
	case 1280000:
		target_dds = 0x136885;	/* ///< 1280Mbps */
		break;
	case 1313000:
		target_dds = 0x127C92;	/* ///< 1313Mbps */
		break;
	case 1466000:
		target_dds = 0x14A40B;	/* ///< 1466Mbps */
		break;
#elif defined(CONFIG_ARCH_MT6735M)
	case 800000:
		target_dds = 0xe55ee;	/* ///< 800Mbps */
		break;
	case 938000:
		target_dds = 0xe38fe;	/* ///< 938Mbps for LPDDR3 */
		break;
	case 1066000:
		target_dds = 0x131A2D;	/* ///< 1066Mbps */
		break;
	case 1280000:
		target_dds = 0x136885;	/* ///< 1280Mbps */
		break;
#elif defined(CONFIG_ARCH_MT6753)
	case 1313000:
		target_dds = 0x127C92;	/* ///< 1313Mbps */
		break;
	case 1466000:
		target_dds = 0x14A40B;	/* ///< 1466Mbps */
		break;
	case 1600000:
		target_dds = 0x168708;	/* ///< 1600Mbps */
		break;
#endif
	default:
		pr_err("[DRAMC] Input incorrect freq for DFS fail!\n");
		return -1;
	}

	/*pr_warn("dram_do_dfs_by_fh to 0x%x ddr_type = %d\n", target_dds, ddr_type);*/
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_INFRA_GCE, "CQDMA");
#else
	if (clk_prepare_enable(clk_cqdma))
		pr_err("enable CQDMA clk fail!\n");
#endif /* #ifdef CONFIG_MTK_CLKMGR */

	mt_dfs_mempll(target_dds);

#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_INFRA_GCE, "CQDMA");
#else
	clk_disable_unprepare(clk_cqdma);
#endif /* #ifdef CONFIG_MTK_CLKMGR */

	/*Notify profiling engine */
	mt_dramfreq_setfreq_notify(freq);

	return 0;
}
#endif

bool pasr_is_valid(void)
{
	unsigned int ddr_type = 0;

	ddr_type = get_ddr_type();
	/* Following DDR types can support PASR */
	if ((ddr_type == TYPE_LPDDR3) || (ddr_type == TYPE_LPDDR2))
		return true;

	return false;
}

/*-------------------------------------------------------------------------*/
/** Round_Operation
 *  Round operation of A/B
 *  @param  A
 *  @param  B
 *  @retval round(A/B)
 */
/*-------------------------------------------------------------------------*/
unsigned int Round_Operation(unsigned int A, unsigned int B)
{
	unsigned int temp;

	if (B == 0)
		return 0xffffffff;

	temp = A / B;

	if ((A - temp * B) >= ((temp + 1) * B - A))
		return temp++;
	else
		return temp;
}

int get_dram_data_rate(void)
{
	unsigned int MEMPLL1_DIV, MEMPLL1_NCPO, MEMPLL1_FOUT;
	unsigned int MEMPLL2_FOUT, MEMPLL2_FBSEL, MEMPLL2_FBDIV;
	unsigned int MEMPLL2_M4PDIV;

	MEMPLL1_DIV = (ucDram_Register_Read(0x0604) & (0x0000007f << 25)) >> 25;
	MEMPLL1_NCPO = (ucDram_Register_Read(0x0624) & (0x7fffffff << 1)) >> 1;
	MEMPLL2_FBSEL = (ucDram_Register_Read(0x0608) & (0x00000003 << 10)) >> 10;
	MEMPLL2_FBSEL = 1 << MEMPLL2_FBSEL;
	MEMPLL2_FBDIV = (ucDram_Register_Read(0x0618) & (0x0000007f << 2)) >> 2;
	MEMPLL2_M4PDIV = (ucDram_Register_Read(0x060c) & (0x00000003 << 10)) >> 10;
	MEMPLL2_M4PDIV = 1 << (MEMPLL2_M4PDIV + 1);

	/*  1PLL:  26*MEMPLL1_NCPO/MEMPLL1_DIV*MEMPLL2_FBSEL*MEMPLL2_FBDIV/2^24 */
	/*  3PLL:  26*MEMPLL1_NCPO/MEMPLL1_DIV*MEMPLL2_M4PDIV*MEMPLL2_FBDIV*2/2^24 */

	MEMPLL1_FOUT = (MEMPLL1_NCPO / MEMPLL1_DIV) * 26;
	if ((ucDram_Register_Read(0x0640) & 0x3) == 3) {
		/*  1PLL */
		MEMPLL2_FOUT = (((MEMPLL1_FOUT * MEMPLL2_FBSEL) >> 12) * MEMPLL2_FBDIV) >> 12;
	} else {
		/*  2 or 3 PLL */
		MEMPLL2_FOUT = (((MEMPLL1_FOUT * MEMPLL2_M4PDIV * 2) >> 12) * MEMPLL2_FBDIV) >> 12;
	}

	/* pr_warn("MEMPLL1_DIV=%d, MEMPLL1_NCPO=%d, MEMPLL2_FBSEL=%d, MEMPLL2_FBDIV=%d\n",
			MEMPLL1_DIV, MEMPLL1_NCPO, MEMPLL2_FBSEL, MEMPLL2_FBDIV); */
	/* pr_warn("MEMPLL2_M4PDIV=%d, MEMPLL1_FOUT=%d, MEMPLL2_FOUT=%d\n",
			MEMPLL2_M4PDIV, MEMPLL1_FOUT, MEMPLL2_FOUT); */

	/*  workaround (Darren) */
	MEMPLL2_FOUT++;

	switch (MEMPLL2_FOUT) {
	case 800:
	case 938:
	case 1066:
	case 1280:
	case 1313:
	case 1466:
	case 1600:
		break;

	default:
		pr_warn("[DRAMC] MemFreq region is incorrect MEMPLL2_FOUT=%d\n", MEMPLL2_FOUT);
		/* return -1; */
	}

	return MEMPLL2_FOUT;
}

unsigned int DRAM_MRR(int MRR_num)
{
	unsigned int MRR_value = 0x0;
	unsigned int u4value;

	/*  set DQ bit 0, 1, 2, 3, 4, 5, 6, 7 pinmux for LPDDR3 */
	ucDram_Register_Write(DRAMC_REG_RRRATE_CTL, 0x13121110);
	ucDram_Register_Write(DRAMC_REG_MRR_CTL, 0x17161514);

	ucDram_Register_Write(DRAMC_REG_MRS, MRR_num);
	ucDram_Register_Write(DRAMC_REG_SPCMD, ucDram_Register_Read(DRAMC_REG_SPCMD) | 0x00000002);
	/* udelay(1); */
	while ((ucDram_Register_Read(DRAMC_REG_SPCMDRESP) & 0x02) == 0)
		;
	ucDram_Register_Write(DRAMC_REG_SPCMD, ucDram_Register_Read(DRAMC_REG_SPCMD) & 0xFFFFFFFD);

	u4value = ucDram_Register_Read(DRAMC_REG_SPCMDRESP);
	MRR_value = (u4value >> 20) & 0xFF;

	return MRR_value;
}

unsigned int read_dram_temperature(void)
{
	unsigned int value;

	value = DRAM_MRR(4) & 0x7;
	return value;
}

#if 0
volatile int shuffer_done;

void dram_dfs_ipi_handler(int id, void *data, unsigned int len)
{
	shuffer_done = 1;
}
#endif


#if 0
__attribute__ ((__section__(".sram.func"))) void uart_print(unsigned char ch)
{
	int i;

	for (i = 0; i < 5; i++)
		(*(volatile unsigned int *)(0xF1003000)) = ch;
}
#endif

#ifdef FREQ_HOPPING_TEST
static ssize_t freq_hopping_test_show(struct device_driver *driver, char *buf)
{
	int dfs_ability = 0;
	unsigned int temp = dram_support_1600_freq();

	dfs_ability = dram_can_support_fh();

	if (dfs_ability == 0)
		return snprintf(buf, PAGE_SIZE, "DRAM DFS can not be enabled, 1600 = %d\n", temp);
	else
		return snprintf(buf, PAGE_SIZE, "DRAM DFS can be enabled, 1600 = %d\n", temp);
}

static ssize_t freq_hopping_test_store(struct device_driver *driver,
				       const char *buf, size_t count)
{
	unsigned int freq;

	if (kstrtouint(buf, 10, &freq))
		return -EINVAL;

	pr_warn("[DRAM DFS] freqency hopping to %dKHz\n", freq);
	dram_do_dfs_by_fh(freq);

	return count;
}
#endif

static void disable_dram_fh(void)
{
	unsigned int value;

	value = ucDram_Register_Read(0xf4);
	value &= ~(0x1 << 15);
	ucDram_Register_Write(0xf4, value);
}

static int __init dt_scan_dram_info(unsigned long node, const char *uname, int depth, void *data)
{
	char *type = (char *)of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg, *endp;
	const struct dram_info *dram_info = NULL;
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

	reg = (const __be32 *)of_get_flat_dt_prop(node, (const char *)"reg", (int *)&l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));

	if (node) {
		/* orig_dram_info */
		dram_info = (const struct dram_info *)of_get_flat_dt_prop(node, "orig_dram_info", NULL);
		if (dram_info == NULL) {
			disable_dram_fh();
			return 0;
		}

		if ((dram_rank_num == 0) || ((dram_rank_num != dram_info->rank_num) && (dram_rank_num != 0))) {
			/* OTA, Rsv fail and 1GB simluate to 512MB */
			dram_rank_num = dram_info->rank_num;

			if (dram_rank0_addr == 0) /* Rsv fail */
				dram_rank0_addr = dram_info->rank_info[0].start;

			if (dram_rank_num == SINGLE_RANK)
				pr_warn("[DFS]  enable (dram base): (%pa)\n", &dram_rank0_addr);
			else {
				if (dram_rank1_addr == 0) /* Rsv fail */
					dram_rank1_addr = dram_info->rank_info[1].start;
				pr_warn("[DFS]  enable (dram base, dram rank1 base): (%pa,%pa)\n",
									&dram_rank0_addr, &dram_rank1_addr);
			}
		} else
			pr_warn("[DFS]  rsv memory from LK node\n");
	}

	return node;
}

int DFS_APDMA_early_init(void)
{
	if (init_done == 0) {
		dst_array_p = __pa(dfs_dummy_buffer);
		pr_warn("[DFS]dfs_dummy_buffer va: 0x%p, dst_pa: 0x%llx, size: %d\n",
				(void *)dfs_dummy_buffer, (unsigned long long)dst_array_p,
				BUFF_LEN);
#ifdef APDMAREG_DUMP
		src_array_v =
				ioremap(rounddown(src_array_p, IOREMAP_ALIGMENT),
			    IOREMAP_ALIGMENT << 1) + IOREMAP_ALIGMENT -
				(BUFF_LEN >> 1);
		dst_array_v = src_array_v + BUFF_LEN;
#endif
		/* memset(src_array_v, 0x6a6a6a6a, BUFF_LEN); */

		init_done = 1;
	}

	return 1;
}

int DFS_APDMA_Init(void)
{
	writel(((~DMA_GSEC_EN_BIT) & readl(DMA_GSEC_EN)), DMA_GSEC_EN);
	return 1;
}

int DFS_APDMA_Enable(void)
{
#ifdef APDMAREG_DUMP
	int i;
#endif

	while (readl(DMA_START) & 0x1)
		;

	if (dram_rank_num == DUAL_RANK) {
		writel(dram_rank0_addr, DMA_SRC);
		writel(dram_rank1_addr, DMA_SRC2);
		writel(dst_array_p, DMA_DST);
		writel((BUFF_LEN >> 1), DMA_LEN1);
		writel((BUFF_LEN >> 1), DMA_LEN2);
		writel((DMA_CON_BURST_8BEAT | DMA_CON_WPEN), DMA_CON);
	} else if (dram_rank_num == SINGLE_RANK) {
		writel(dram_rank0_addr, DMA_SRC);
		writel(dst_array_p, DMA_DST);
		writel(BUFF_LEN, DMA_LEN1);
		writel(DMA_CON_BURST_8BEAT, DMA_CON);
	} else {
		pr_warn("[DFS] error rank number = %x\n", dram_rank_num);
		return 1;
	}


#ifdef APDMAREG_DUMP
	pr_debug("src_p=0x%x, dst_p=0x%x, src_v=0x%x, dst_v=0x%x, len=%d\n",
	       src_array_p, dst_array_p, (unsigned int)src_array_v,
	       (unsigned int)dst_array_v, BUFF_LEN);
	for (i = 0; i < 0x60; i += 4) {
		pr_debug("[Before]addr:0x%x, value:%x\n",
		       (unsigned int)(DMA_BASE + i),
		       *((volatile int *)(DMA_BASE + i)));
	}

#ifdef APDMA_TEST
	for (i = 0; i < BUFF_LEN / sizeof(unsigned int); i++) {
		dst_array_v[i] = 0;
		src_array_v[i] = i;
	}
#endif
#endif

	mt_reg_sync_writel(0x1, DMA_START);

#ifdef APDMAREG_DUMP
	for (i = 0; i < 0x60; i += 4) {
		pr_debug("[AFTER]addr:0x%x, value:%x\n",
		       (unsigned int)(DMA_BASE + i),
		       *((volatile int *)(DMA_BASE + i)));
	}

#ifdef APDMA_TEST
	for (i = 0; i < BUFF_LEN / sizeof(unsigned int); i++) {
		if (dst_array_v[i] != src_array_v[i]) {
			pr_debug("DMA ERROR at Address %x\n (i=%d, value=0x%x(should be 0x%x))",
				(unsigned int)&dst_array_v[i], i, dst_array_v[i],
				src_array_v[i]);
			ASSERT(0);
		}
	}
	pr_debug("Channe0 DFS DMA TEST PASS\n");
#endif
#endif
	return 1;
}

int DFS_APDMA_END(void)
{
	while (readl(DMA_START))
		;
	return 1;
}

void DFS_APDMA_dummy_read_preinit(void)
{
	DFS_APDMA_early_init();
}

void DFS_APDMA_dummy_read_deinit(void)
{
}

void dma_dummy_read_for_vcorefs(int loops)
{
	int i, count;
	unsigned long long start_time, end_time, duration;

	DFS_APDMA_early_init();
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_INFRA_GCE, "CQDMA");
#else
	if (clk_prepare_enable(clk_cqdma))
		pr_err("enable CQDMA clk fail!\n");
#endif /* #ifdef CONFIG_MTK_CLKMGR */


	for (i = 0; i < loops; i++) {
		count = 0;
		start_time = sched_clock();
		do {
			DFS_APDMA_Enable();
			DFS_APDMA_END();
			end_time = sched_clock();
			duration = end_time - start_time;
			count++;
		} while (duration < 4000L);
		/* pr_warn("[DMA_dummy_read[%d], duration=%lld, count = %d\n", duration, count); */
	}
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_INFRA_GCE, "CQDMA");
#else
	clk_disable_unprepare(clk_cqdma);
#endif /* #ifdef CONFIG_MTK_CLKMGR */
}

#ifdef CONFIG_OF_RESERVED_MEM
int dram_dummy_read_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;

	if (strstr(DRAM_R0_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE)
			return 0;
		dram_rank0_addr = rptr;
		dram_rank_num++;
		pr_debug("[dram_dummy_read_reserve_mem_of_init] dram_rank0_addr = %pa, size = 0x%x\n",
				&dram_rank0_addr, rsize);
	}

	if (strstr(DRAM_R1_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE)
			return 0;
		dram_rank1_addr = rptr;
		dram_rank_num++;
		pr_debug("[dram_dummy_read_reserve_mem_of_init] dram_rank1_addr = %pa, size = 0x%x\n",
				&dram_rank1_addr, rsize);
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(dram_reserve_r0_dummy_read_init, DRAM_R0_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(dram_reserve_r1_dummy_read_init, DRAM_R1_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
#endif

unsigned int is_one_pll_mode(void)
{
	int data;
	unsigned  int ret = 0;

	data = *(volatile unsigned int *)(0xF0004000 + (0x00a << 2));
	if (data & 0x10000)
		/* print("It is 1-PLL mode (value = 0x%x)\n", data); */
		ret = 1;
	else
		/* print("It is 3-PLL mode (value = 0x%x)\n", data); */
		ret = 0;

	return ret;
}

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
	return count;
}

#ifdef APDMA_TEST
static ssize_t DFS_APDMA_TEST_show(struct device_driver *driver, char *buf)
{
	dma_dummy_read_for_vcorefs(7);

	if (dram_rank_num == DUAL_RANK)
		return snprintf(buf, PAGE_SIZE, "DFS APDMA Dummy Read Address src1:%pa src2:%pa\n",
				&dram_rank0_addr, &dram_rank1_addr);
	else if (dram_rank_num == SINGLE_RANK)
		return snprintf(buf, PAGE_SIZE, "DFS APDMA Dummy Read Address src1:%pa\n", &dram_rank0_addr);
	else
		return snprintf(buf, PAGE_SIZE, "DFS APDMA Dummy Read rank number incorrect = %d !!!\n", dram_rank_num);
}

static ssize_t DFS_APDMA_TEST_store(struct device_driver *driver,
				    const char *buf, size_t count)
{
	return count;
}
#endif

#ifdef READ_DRAM_TEMP_TEST
static ssize_t read_dram_temp_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM MR4 = 0x%x\n", read_dram_temperature());
}

static ssize_t read_dram_temp_store(struct device_driver *driver,
				    const char *buf, size_t count)
{
	return count;
}
#endif

#ifdef READ_DRAM_FREQ_TEST
static ssize_t read_dram_data_rate_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM data rate = %d\n", get_dram_data_rate());
}

static ssize_t read_dram_data_rate_store(struct device_driver *driver,
					 const char *buf, size_t count)
{
	return count;
}
#endif

DRIVER_ATTR(emi_clk_mem_test, 0664, complex_mem_test_show,
	    complex_mem_test_store);

#ifdef APDMA_TEST
DRIVER_ATTR(dram_dummy_read_test, 0664, DFS_APDMA_TEST_show,
	    DFS_APDMA_TEST_store);
#endif

#ifdef READ_DRAM_TEMP_TEST
DRIVER_ATTR(read_dram_temp_test, 0664, read_dram_temp_show,
	    read_dram_temp_store);
#endif

#ifdef READ_DRAM_FREQ_TEST
DRIVER_ATTR(read_dram_data_rate, 0664, read_dram_data_rate_show,
	    read_dram_data_rate_store);
#endif

#ifdef FREQ_HOPPING_TEST
DRIVER_ATTR(freq_hopping_test, 0664, freq_hopping_test_show,
	    freq_hopping_test_store);
#endif

static int dram_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("[DRAMC0] module probe.\n");

#ifndef CONFIG_MTK_CLKMGR
	clk_cqdma = devm_clk_get(&pdev->dev, "infra-cqdma");
	if (IS_ERR(clk_cqdma)) {
		pr_err("[DRAMC0] can not get CQDMA clock fail!\n");
		return PTR_ERR(clk_cqdma);
	}
#endif /* #ifndef CONFIG_MTK_CLKMGR */

	return ret;
}

static int dram_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dram_of_ids[] = {
	{.compatible = "mediatek,mt6735-dramc",},
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

	/* DTS version */
	node = of_find_compatible_node(NULL, NULL, "mediatek,cqdma");
	if (node) {
		CQDMA_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get CQDMA_BASE_ADDR @ %p\n", CQDMA_BASE_ADDR);
	} else {
		pr_warn("[DRAMC]can't find compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-dramc");
	if (node) {
		DRAMCAO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_BASE_ADDR @ %p\n", DRAMCAO_BASE_ADDR);
	} else {
		pr_warn("[DRAMC]can't find DRAMC0 compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-ddrphy");
	if (node) {
		DDRPHY_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DDRPHY_BASE_ADDR @ %p\n", DDRPHY_BASE_ADDR);
	} else {
		pr_warn("[DRAMC]can't find DDRPHY compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-dramc_nao");
	if (node) {
		DRAMCNAO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_BASE_ADDR @ %p\n", DRAMCNAO_BASE_ADDR);
	} else {
		pr_warn("[DRAMC]can't find DRAMCNAO compatible node\n");
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
	unsigned int tmp;

	/* unsigned char * dst = &__ssram_text; */
	/* unsigned char * src = &_sram_start; */

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

	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}
#ifdef APDMA_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_dram_dummy_read_test);
	if (ret) {
		pr_warn("fail to create the DFS sysfs files\n");
		return ret;
	}
#endif

#ifdef READ_DRAM_TEMP_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_read_dram_temp_test);
	if (ret) {
		pr_warn("fail to create the read dram temp sysfs files\n");
		return ret;
	}
#endif

#ifdef READ_DRAM_FREQ_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}
#endif

#ifdef FREQ_HOPPING_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_freq_hopping_test);
	if (ret) {
		pr_warn("fail to create the read dram temp sysfs files\n");
		return ret;
	}
#endif

	org_dram_data_rate = get_dram_data_rate();
	pr_warn("[DRAMC Driver] Dram Data Rate = %d\n", org_dram_data_rate);
	if (org_dram_data_rate != dram_fh_steps_freq(0)) {
		tmp = ucDram_Register_Read(0xf4);
		tmp &= ~(0x1 << 15);
		ucDram_Register_Write(0xf4, tmp);
		pr_warn("[DRAMC Driver] (boot DRAM freq != DVFS DRAM freq) !!!\n");
	}

	if (dram_can_support_fh())
		pr_warn("[DRAMC Driver] dram can support Frequency Hopping\n");
	else
		pr_warn("[DRAMC Driver] dram can not support Frequency Hopping\n");

	return ret;
}

static void __exit dram_test_exit(void)
{
	platform_driver_unregister(&dram_test_drv);
}

postcore_initcall(dram_test_init);
module_exit(dram_test_exit);

MODULE_DESCRIPTION("MediaTek DRAMC Driver v0.1");
