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

/* Registers define */
#define PDEF_DRAMC0_CHA_REG_0E4	IOMEM((DRAMC_AO_CHA_BASE_ADDR + 0x00e4))
#define PDEF_DRAMC0_CHA_REG_010	IOMEM((DRAMC_AO_CHA_BASE_ADDR + 0x0010))
#define PDEF_SPM_AP_SEMAPHORE	IOMEM((SLEEP_BASE_ADDR + 0x428))

/* Define */
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

#define SW_ZQCS
#define SW_TX_TRACKING
#ifdef SW_TX_TRACKING
/* #define MIX_MODE */
#define DRAMC_AO_RKCFG		(dramc_ao_chx_base+0x034)
#define DRAMC_AO_PD_CTRL	(dramc_ao_chx_base+0x038)
#define DRAMC_AO_MRS		(dramc_ao_chx_base+0x05C)
#define DRAMC_AO_SPCMD		(dramc_ao_chx_base+0x060)
#define DRAMC_AO_SPCMDCTRL	(dramc_ao_chx_base+0x064)
#define DRAMC_AO_SLP4_TESTMODE	(dramc_ao_chx_base+0x0C4)
#define DRAMC_AO_DQSOSCR	(dramc_ao_chx_base+0x0C8)
#define DRAMC_AO_SHUSTATUS	(dramc_ao_chx_base+0x0E4)
#define DRAMC_AO_CKECTRL	(dramc_ao_chx_base+0x85C)
#define DRAMC_AO_DQSOSC_PRD	(dramc_ao_chx_base+0x868)
#define DRAMC_AO_SHU1RK0_PI	(dramc_ao_chx_base+0xA0C)
#define DRAMC_AO_SHU1RK0_DQSOSC	(dramc_ao_chx_base+0xA10)
#define DRAMC_AO_SHU1RK1_PI     (dramc_ao_chx_base+0xB0C)
#define DRAMC_AO_SHU1RK1_DQSOSC (dramc_ao_chx_base+0xB10)
#define DRAMC_NAO_MISC_STATUSA	(dramc_nao_chx_base+0x80)
#define DRAMC_NAO_SPCMDRESP	(dramc_nao_chx_base+0x88)
#define DRAMC_NAO_MRR_STATUS	(dramc_nao_chx_base+0x8C)
#define DDRPHY_SHU1_R0_B0_DQ7	(ddrphy_chx_base+0xE1C)
#define DDRPHY_SHU1_R0_B1_DQ7	(ddrphy_chx_base+0xE6C)
#define DDRPHY_SHU1_R1_B0_DQ7	(ddrphy_chx_base+0xF1C)
#define DDRPHY_SHU1_R1_B1_DQ7	(ddrphy_chx_base+0xF6C)

typedef enum {
	TX_DONE = 0,
	TX_TIMEOUT_MRR_ENABLE,
	TX_TIMEOUT_MRR_DISABLE,
	TX_TIMEOUT_DQSOSC,
	TX_TIMEOUT_DDRPHY,
	TX_FAIL_DATA_RATE,
	TX_FAIL_VARIATION
} tx_result;
#endif

#define LAST_DRAMC
#ifdef LAST_DRAMC
#define LAST_DRAMC_IP_BASED

#define LAST_DRAMC_SRAM_SIZE		(20)
#define DRAMC_STORAGE_API_ERR_OFFSET	(8)

#define STORAGE_READ_API_MASK		(0xf)
#define ERR_PL_UPDATED			(0x4)

void *mt_emi_base_get(void);
unsigned int mt_dramc_chn_get(unsigned int emi_cona);
unsigned int mt_dramc_chp_get(unsigned int emi_cona);
phys_addr_t mt_dramc_rankbase_get(unsigned int rank);
unsigned int mt_dramc_ta_support_ranks(void);
phys_addr_t mt_dramc_ta_reserve_addr(unsigned int rank);
#endif

/* Sysfs config */
#define APDMA_TEST
/*#define READ_DRAM_TEMP_TEST*/
/*#define APDMAREG_DUMP*/
#define PHASE_NUMBER		3
#define DRAM_BASE		(0x40000000ULL)
#define BUFF_LEN		0x100
#define IOREMAP_ALIGMENT	0x1000
#define Delay_magic_num		0x295
/*We use GPT to measurement how many clk pass in 100us*/

/* DRAM HQA config */
/*#define DRAM_HQA*/
#ifdef DRAM_HQA

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define MAX_VDRAM2 1190625 /*Vddq*/
#define MAX_VDRAM1 1309375 /*Vdram*/
#define MAX_VCORE  1196875
#endif

/*#define HQA_LPDDR3*/
#define HQA_LPDDR4
/*#define HQA_LPDDR4X*/

/*#define HVCORE_HVDRAM*/
/*#define NVCORE_NVDRAM*/
/*#define LVCORE_LVDRAM*/
/*#define HVCORE_LVDRAM*/
/*#define LVCORE_HVDRAM*/

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define VCORE_LV_LPM 656000	/*0.656V*/
#define VCORE_NV_LPM 700000	/*0.700V*/
#define VCORE_HV_LPM 744000	/*0.744V*/
#define VCORE_LV_HPM 756000	/*0.756V*/
#define VCORE_NV_HPM 800000	/*0.800V*/
#define VCORE_HV_HPM 844000	/*0.844V*/
#else
#define VCORE_LV_LPM 0x09
#define VCORE_NV_LPM 0x10
#define VCORE_HV_LPM 0x17
#define VCORE_LV_HPM 0x19
#define VCORE_NV_HPM 0x20
#define VCORE_HV_HPM 0x27
#endif

#if defined(HQA_LPDDR3)
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define VDRAM_LV 1160000	/*1.16V*/
#define VDRAM_NV 1220000	/*1.22V*/
#define VDRAM_HV 1300000	/*1.30V*/
#else
#define VDRAM_LV 0x504
#define VDRAM_NV 0x51E
#define VDRAM_HV 0x617
#endif
#elif defined(HQA_LPDDR4) || defined(HQA_LPDDR4X)
#define VDRAM_LV 1060000
#define VDRAM_NV 1100000
#define VDRAM_HV 1170000
#endif

#define VDDQ_LV 570000
#define VDDQ_NV 600000
#define VDDQ_HV 650000

#if defined(HVCORE_HVDRAM)
#define HQA_VCORE_LPM	VCORE_HV_LPM
#define HQA_VCORE_HPM	VCORE_HV_HPM
#define HQA_VDRAM	VDRAM_HV
#define HQA_VDDQ	VDDQ_HV
#elif defined(NVCORE_NVDRAM)
#define HQA_VCORE_LPM	VCORE_NV_LPM
#define HQA_VCORE_HPM	VCORE_NV_HPM
#define HQA_VDRAM	VDRAM_NV
#define HQA_VDDQ	VDDQ_NV
#elif defined(LVCORE_LVDRAM)
#define HQA_VCORE_LPM	VCORE_LV_LPM
#define HQA_VCORE_HPM	VCORE_LV_HPM
#define HQA_VDRAM	VDRAM_LV
#define HQA_VDDQ	VDDQ_LV
#elif defined(HVCORE_LVDRAM)
#define HQA_VCORE_LPM	VCORE_HV_LPM
#define HQA_VCORE_HPM	VCORE_HV_HPM
#define HQA_VDRAM	VDRAM_LV
#define HQA_VDDQ	VDDQ_LV
#elif defined(LVCORE_HVDRAM)
#define HQA_VCORE_LPM	VCORE_LV_LPM
#define HQA_VCORE_HPM	VCORE_LV_HPM
#define HQA_VDRAM	VDRAM_HV
#define HQA_VDDQ	VDDQ_HV
#endif

#endif /*end #ifdef DRAM_HQA*/

#ifdef CONFIG_MTK_MEMORY_LOWPOWER
extern int __init acquire_buffer_from_memory_lowpower(phys_addr_t *addr);
#else
static inline int acquire_buffer_from_memory_lowpower(phys_addr_t *addr) { return -3; }
#endif

/* DRAMC API config */
extern unsigned int DMA_TIMES_RECORDER;

void *mt_dramc_chn_base_get(int channel);
void *mt_dramc_nao_chn_base_get(int channel);
void *mt_ddrphy_chn_base_get(int channel);
void *mt_ddrphy_nao_chn_base_get(int channel);

/*void get_mempll_table_info(u32 *high_addr, u32 *low_addr, u32 *num);*/
unsigned int get_dram_data_rate(void);
unsigned int read_dram_temperature(unsigned char channel);
/*void sync_hw_gating_value(void);*/
/*unsigned int is_one_pll_mode(void);*/
int dram_steps_freq(unsigned int step);
unsigned int get_shuffle_status(void);
int get_ddr_type(void);
int get_emi_ch_num(void);
int dram_can_support_fh(void);
extern void *mt_dramc_base_get(void);
extern void *mt_dramc_nao_base_get(void);
extern void *mt_ddrphy_base_get(void);
unsigned int ucDram_Register_Read(unsigned int u4reg_addr);
unsigned int lpDram_Register_Read(unsigned int Reg_base, unsigned int Offset);
void ucDram_Register_Write(unsigned int u4reg_addr, unsigned int u4reg_value);
void dram_HQA_adjust_voltage(void);
int enter_pasr_dpd_config(unsigned char segment_rank0,
unsigned char segment_rank1);
int exit_pasr_dpd_config(void);
void del_zqcs_timer(void);
void add_zqcs_timer(void);

enum DDRTYPE {
	TYPE_LPDDR3 = 1,
	TYPE_LPDDR4,
	TYPE_LPDDR4X
};

#ifdef MIX_MODE
#define PDEF_DRAMC0_CHA_REG_01C	IOMEM((DRAMC_AO_CHA_BASE_ADDR + 0x001c))
enum DRAM_MODE {
	NORMAL_MODE = 0,
	BYTE_MODE,
	R0_NORMAL_R1_BYTE,
	R0_BYTE_R1_NORMAL
};
enum RANK_MODE {
	RANK_NORMAL = 0,
	RANK_BYTE
};
#else
enum DRAM_MODE {
	NORMAL_MODE = 0,
	BYTE_MODE
};
#endif
enum {
	DRAM_OK = 0,
	DRAM_FAIL
}; /* DRAM status type */

enum {
	DRAMC_NAO_CHA = 0,
	DRAMC_NAO_CHB,
	DRAMC_AO_CHA,
	DRAMC_AO_CHB,
	PHY_NAO_CHA,
	PHY_NAO_CHB,
	PHY_AO_CHA,
	PHY_AO_CHB
}; /* RegBase */

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
