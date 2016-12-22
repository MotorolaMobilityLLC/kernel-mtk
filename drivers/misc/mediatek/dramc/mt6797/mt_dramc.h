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
#define PDEF_DRAMC0_CHA_REG_008	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x0008))
#define PDEF_DRAMC0_CHA_REG_088	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x0088))
#define PDEF_DRAMC0_CHA_REG_0E4	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x00e4))
#define PDEF_DRAMC0_CHA_REG_0F4	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x00f4))
#define PDEF_DRAMC0_CHA_REG_110	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x0110))
#define PDEF_DRAMC0_CHA_REG_138	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x0138))
#define PDEF_DRAMC0_CHA_REG_1DC	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x01dc))
#define PDEF_DRAMC0_CHA_REG_1E4	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x01e4))
#define PDEF_DRAMC0_CHA_REG_1E8	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x01e8))
#define PDEF_DRAMC0_CHA_REG_1EC	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x01ec))
#define PDEF_DRAMC0_CHA_REG_3B8	IOMEM((DRAMCAO_CHA_BASE_ADDR + 0x03B8))

#define PDEF_DRAMC0_CHB_REG_008	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x0008))
#define PDEF_DRAMC0_CHB_REG_088	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x0088))
#define PDEF_DRAMC0_CHB_REG_0E4	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x00e4))
#define PDEF_DRAMC0_CHB_REG_0F4	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x00f4))
#define PDEF_DRAMC0_CHB_REG_110	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x0110))
#define PDEF_DRAMC0_CHB_REG_138	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x0138))
#define PDEF_DRAMC0_CHB_REG_1DC	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x01dc))
#define PDEF_DRAMC0_CHB_REG_1E4	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x01e4))
#define PDEF_DRAMC0_CHB_REG_1E8	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x01e8))
#define PDEF_DRAMC0_CHB_REG_1EC	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x01ec))
#define PDEF_DRAMC0_CHB_REG_3B8	IOMEM((DRAMCAO_CHB_BASE_ADDR + 0x03B8))

#define PDEF_DRAMCNAO_CHA_REG_3B8	IOMEM((DRAMCNAO_CHA_BASE_ADDR + 0x03B8))
#define PDEF_DRAMCNAO_CHB_REG_3B8	IOMEM((DRAMCNAO_CHB_BASE_ADDR + 0x03B8))

#define PDEF_SPM_PASR_DPD_0	IOMEM((SLEEP_BASE_ADDR + 0x0630))
#define PDEF_SPM_PASR_DPD_3	IOMEM((SLEEP_BASE_ADDR + 0x063C))
#define PDEF_SPM_AP_SEMAPHORE	IOMEM((SLEEP_BASE_ADDR + 0x0428))
/* #define _CLK_CFG_0_SET		IOMEM((TOPCKGEN_BASE_ADDR + 0x0040)) */

/*=========================
Define
=========================*/
#define DUAL_FREQ_HIGH		900
#define DUAL_FREQ_LOW		650
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
/*#define DRAM_HQA*/

#ifdef DRAM_HQA
/*#define HVcore1*/		/*Vcore1=1.10, Vdram=1.3,  Vio18=1.8*/
/*#define NV*/			/*Vcore1=1.00, Vdram=1.22, Vio18=1.8*/
/*#define LVcore1*/		/*Vcore1=0.90, Vdram=1.16, Vio18=1.8*/
#define HVcore1_LVdram		/*Vcore1=1.10, Vdram=1.16, Vio18=1.8*/
/*#define LVcore1_HVdram*/	/*Vcore1=0.90, Vdram=1.3,  Vio18=1.8*/

#define RG_VDRAM_VOSEL_1p2V			(0x5 << 8)	/*1.2V*/
#define VDRAM_ANA_CON0_SUB40mV	0x4
#define VDRAM_ANA_CON0_ADD20mV	0x1e
#define VDRAM_ANA_CON0_ADD100mV	0X16

#define Vdram_HV (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_ADD100mV) /*1.30V*/
#define Vdram_NV (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_ADD20mV)  /*1.22V*/
#define Vdram_LV (RG_VDRAM_VOSEL_1p2V | VDRAM_ANA_CON0_SUB40mV)  /*1.16V*/

#define Vcore1_HV	0x48	/*1.05V*/
#define Vcore1_NV	0x40	/*1.00V*/
#define Vcore1_LV	0x38	/*0.95V*/

#define Vio18_HV	0x28	/*1.9V*/
#define Vio18_NV	0x20	/*1.8V*/
#define Vio18_LV	0x18	/*1.7V*/

#endif

/*=========================
DRAMC API config
=========================*/
extern unsigned int DMA_TIMES_RECORDER;
extern phys_addr_t get_max_DRAM_size(void);
/*void get_mempll_table_info(u32 *high_addr, u32 *low_addr, u32 *num);*/
unsigned int get_dram_data_rate(int freq_sel);
unsigned int read_dram_temperature(unsigned char channel);
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
int enter_pasr_dpd_config(unsigned char segment_rank0,
unsigned char segment_rank1);
int exit_pasr_dpd_config(void);

enum DDRTYPE {
	TYPE_DDR1 = 1,
	TYPE_LPDDR2,
	TYPE_LPDDR3,
	TYPE_PCDDR3
};

enum {
	DRAM_OK = 0,
	DRAM_FAIL
}; /* DRAM status type */

enum RANKNUM {
	SINGLE_RANK = 1,
	DUAL_RANK,
};
enum {
	CHANNEL_A = 0,
	CHANNEL_B,
	CHANNEL_MAX
}; /* DRAM_CHANNEL_T */
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
