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

#ifndef __DRAMC_H__
#define __DRAMC_H__

/*=========================
Registers define
=========================*/
#define PDEF_DRAMC0_REG_088	IOMEM((DRAMCAO_BASE_ADDR + 0x0088))
#define PDEF_DRAMC0_REG_0E4	IOMEM((DRAMCAO_BASE_ADDR + 0x00e4))
#define PDEF_DRAMC0_REG_0F4	IOMEM((DRAMCAO_BASE_ADDR + 0x00f4))
#define PDEF_DRAMC0_REG_110	IOMEM((DRAMCAO_BASE_ADDR + 0x0110))
#define PDEF_DRAMC0_REG_138	IOMEM((DRAMCAO_BASE_ADDR + 0x0138))
#define PDEF_DRAMC0_REG_1DC	IOMEM((DRAMCAO_BASE_ADDR + 0x01dc))
#define PDEF_DRAMC0_REG_1E4	IOMEM((DRAMCAO_BASE_ADDR + 0x01e4))
#define PDEF_DRAMC0_REG_1E8	IOMEM((DRAMCAO_BASE_ADDR + 0x01e8))
#define PDEF_DRAMC0_REG_1EC	IOMEM((DRAMCAO_BASE_ADDR + 0x01ec))
#define PDEF_DRAMC0_REG_3B8	IOMEM((DRAMCNAO_BASE_ADDR + 0x03B8))
#define _CLK_CFG_0_SET		IOMEM((TOPCKGEN_BASE_ADDR + 0x0040))

/*=========================
Define
=========================*/
#define DUAL_FREQ_HIGH_J	832
#define DUAL_FREQ_LOW_J		650
#define DUAL_FREQ_HIGH_R	898
#define DUAL_FREQ_LOW_R		650
#define DATA_RATE_THRESHOLD	15
#define MPLL_CON0_OFFSET	0x280
#define MPLL_CON1_OFFSET	0x284
#define MEMPLL5_OFFSET		0x614
#define DRAMC_ACTIM1		(0x1e8)
#define TB_DRAM_SPEED
#define DUAL_FREQ_DIFF_RLWL	/* If defined, need to set MR2 in dramcinit.*/
#define DMA_GDMA_LEN_MAX_MASK	(0x000FFFFF)
#define DMA_GSEC_EN_BIT		(0x00000001)
#define DMA_INT_EN_BIT		(0x00000001)
#define DMA_INT_FLAG_CLR_BIT	(0x00000000)
#define LPDDR3_MODE_REG_2_LOW	0x00140002              /*RL6 WL3.*/
#define LPDDR2_MODE_REG_2_LOW	0x00040002              /*RL6 WL3.*/

#ifdef DDR_1866
#define LPDDR3_MODE_REG_2	0x001C0002
#else
#define LPDDR3_MODE_REG_2	0x001A0002
#endif
#define LPDDR2_MODE_REG_2	0x00060002

#define DRAMC_REG_MRS		0x088
#define DRAMC_REG_PADCTL4	0x0e4
#define DRAMC_REG_LPDDR2_3	0x1e0
#define DRAMC_REG_SPCMD		0x1e4
#define DRAMC_REG_ACTIM1	0x1e8
#define DRAMC_REG_RRRATE_CTL	0x1f4
#define DRAMC_REG_MRR_CTL	0x1fc
#define DRAMC_REG_SPCMDRESP	0x3b8
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5

/*=========================
Sysfs config
=========================*/
#define APDMA_TEST
/*#define READ_DRAM_TEMP_TEST*/
/*#define APDMAREG_DUMP*/
#define PHASE_NUMBER		3
#define DRAM_BASE		(0x40000000ULL)
#define BUFF_LEN		0x100
#define IOREMAP_ALIGMENT	0x1000
#define Delay_magic_num		0x295
/*We use GPT to measurement how many clk pass in 100us*/

/*=========================
DRAM HQA Config
=========================*/
/* Vdram :for jade minus PMIC change to MT6353*/
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
#define Vdram_REG MT6353_SLDO_ANA_CON5
#define Vcore_REG_SW MT6353_BUCK_VCORE_VOL_CON1
#define Vcore_REG_HW MT6353_BUCK_VCORE_VOL_CON2
#define RG_VDRAM_VOSEL_1p24V	(0x0<<8)/*1.24V*/
#define VDRAM_ANA_CON0_SUB20mV	0x1
#define VDRAM_ANA_CON0_SUB80mV	0x4
#define VDRAM_ANA_CON0_ADD60mV	0Xd

#define Vdram_NV_default (RG_VDRAM_VOSEL_1p24V | VDRAM_ANA_CON0_SUB20mV)/*1.22V*/
#define Vdram_HV (RG_VDRAM_VOSEL_1p24V | VDRAM_ANA_CON0_ADD60mV)/*1.30V*/
#define Vdram_NV Vdram_NV_default
#define Vdram_LV (RG_VDRAM_VOSEL_1p24V | VDRAM_ANA_CON0_SUB80mV)/*1.16V*/

#else /*#if defined (CONFIG_MTK_PMIC_CHIP_MT6353)*/
#define Vdram_REG MT6351_VDRAM_ANA_CON0
#define Vcore_REG_SW MT6351_BUCK_VCORE_CON4
#define Vcore_REG_HW MT6351_BUCK_VCORE_CON5
#define RG_VDRAM_VOSEL_1p2V			(0x5 << 8)	/*1.2V*/
#define VDRAM_ANA_CON0_ADD20mV	0x1e
#define VDRAM_ANA_CON0_SUB40mV	0x4
#define VDRAM_ANA_CON0_ADD100mV	0X16

#define Vdram_NV_default (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_ADD20mV)/*1.22V*/
#define Vdram_HV (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_ADD100mV) /*1.30V*/
#define Vdram_NV Vdram_NV_default
#define Vdram_LV (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_SUB40mV)  /*1.16V*/

#endif/*#if defined (CONFIG_MTK_PMIC_CHIP_MT6353)*/


/*#define DRAM_HQA*/
#ifdef DRAM_HQA
#define HVcore1		/*Vcore1=1.10, Vdram=1.3,  Vio18=1.8*/
/*#define NV*/			/*Vcore1=1.00, Vdram=1.22, Vio18=1.8*/
/*#define LVcore1*/		/*Vcore1=0.90, Vdram=1.16, Vio18=1.8*/
/*#define HVcore1_LVdram*/	/*Vcore1=1.10, Vdram=1.16, Vio18=1.8*/
/*#define LVcore1_HVdram*/	/*Vcore1=0.90, Vdram=1.3,  Vio18=1.8*/

/*Vcore :for DRAM HQA*/
#ifdef ONEPLL_TEST
#define Vcore1_HV 0x38 /*0.95V*/
#define Vcore1_NV 0x30 /*0.90V*/
#define Vcore1_LV 0x28 /*0.85V*/
#else
#define Vcore1_HV	0x48	/*1.05V*/
#define Vcore1_NV	0x40	/*1.00V*/
#define Vcore1_LV	0x38	/*0.95V*/
#endif /*end #ifdef ONEPLL_TEST*/
#endif /*end #ifdef DRAM_HQA*/

/*=========================
DRAMC API config
=========================*/
extern unsigned int DMA_TIMES_RECORDER;
extern phys_addr_t get_max_DRAM_size(void);
/*int DFS_APDMA_Enable(void);*/
/*int DFS_APDMA_Init(void);*/
int DFS_APDMA_early_init(void);
void dma_dummy_read_for_vcorefs(int loops);
/*void get_mempll_table_info(u32 *high_addr, u32 *low_addr, u32 *num);*/
unsigned int get_dram_data_rate(void);
/*unsigned int read_dram_temperature(void);*/
/*void sync_hw_gating_value(void);*/
/*unsigned int is_one_pll_mode(void);*/
int dram_steps_freq(unsigned int step);
int dram_can_support_fh(void);
void spm_dpd_init(void);
void spm_dpd_dram_init(void);
unsigned int support_4GB_mode(void);
extern void *mt_dramc_base_get(void);
extern void *mt_dramc_nao_base_get(void);
extern void *mt_ddrphy_base_get(void);
unsigned int ucDram_Register_Read(unsigned int u4reg_addr);
void ucDram_Register_Write(unsigned int u4reg_addr, unsigned int u4reg_value);
void dram_HQA_adjust_voltage(void);
int enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1);
int exit_pasr_dpd_config(void);

enum DDRTYPE {
	TYPE_DDR1 = 1,
	TYPE_LPDDR2,
	TYPE_LPDDR3,
	TYPE_PCDDR3
};

/************************** Common Macro *********************/
#define delay_a_while(count) \
	do {                                    \
		register unsigned int delay;    \
		asm volatile ("mov %0, %1\n\t"\
			"1:\n\t"                                \
			"subs %0, %0, #1\n\t"   \
			"bne 1b\n\t"    \
			: "+r" (delay)  \
			: "r" (count)   \
			: "cc");                \
	} while (0)

#define mcDELAY_US(x)           delay_a_while((U32) (x*1000*10))

#endif   /*__WDT_HW_H__*/
