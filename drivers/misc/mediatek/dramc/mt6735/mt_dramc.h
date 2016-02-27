#ifndef __DRAMC_H__
#define __DRAMC_H__

#define DUAL_FREQ_DIFF_RLWL	/* If defined, need to set MR2 in dramcinit. */

#define DMA_SRC             IOMEM((CQDMA_BASE_ADDR + 0x001C))
#define DMA_DST             IOMEM((CQDMA_BASE_ADDR + 0x0020))
#define DMA_LEN1            IOMEM((CQDMA_BASE_ADDR + 0x0024))
#define DMA_GSEC_EN         IOMEM((CQDMA_BASE_ADDR + 0x0058))
#define DMA_INT_EN          IOMEM((CQDMA_BASE_ADDR + 0x0004))
#define DMA_CON             IOMEM((CQDMA_BASE_ADDR + 0x0018))
#define DMA_START           IOMEM((CQDMA_BASE_ADDR + 0x0008))
#define DMA_INT_FLAG        IOMEM((CQDMA_BASE_ADDR + 0x0000))
#define PDEF_DRAMC0_REG_088 IOMEM((DRAMCAO_BASE_ADDR + 0x0088))
#define PDEF_DRAMC0_REG_0E4 IOMEM((DRAMCAO_BASE_ADDR + 0x00E4))
#define PDEF_DRAMC0_REG_1DC IOMEM((DRAMCAO_BASE_ADDR + 0x01DC))
#define PDEF_DRAMC0_REG_1E4 IOMEM((DRAMCAO_BASE_ADDR + 0x01E4))
#define PDEF_DRAMC0_REG_3B8 IOMEM((DRAMCNAO_BASE_ADDR + 0x03B8))
#define DMA_GDMA_LEN_MAX_MASK	(0x000FFFFF)
#define DMA_GSEC_EN_BIT		(0x00000001)
#define DMA_INT_EN_BIT		(0x00000001)
#define DMA_INT_FLAG_CLR_BIT	(0x00000000)

#define DMA_LEN2           IOMEM((CQDMA_BASE_ADDR + 0x0028))
#define DMA_SRC2          IOMEM((CQDMA_BASE_ADDR + 0x002C))
#define DMA_CON_WPEN            0x00008000


#if 0
#define CHA_DRAMCAO_BASE	0xF0004000
#define CHA_DDRPHY_BASE		0xF000F000
#define CHA_DRAMCNAO_BASE	0xF020E000
#define INFRA_BASE		0xF0001000
#define MEM_DCM_CTRL		(INFRA_BASE + 0x078)
#define DFS_MEM_DCM_CTRL	(INFRA_BASE + 0x07c)
#define CLK_CFG_0_CLR		(0xF0000048)
#define CLK_CFG_0_SET		(0xF0000044)
#define CLK_CFG_UPDATE		(0xF0000004)
#define PCM_INI_PWRON0_REG	(0xF0006010)
#endif

#define LPDDR3_MODE_REG_2_LOW		0x00140002	/*  RL6 WL3. */
#define LPDDR2_MODE_REG_2_LOW		0x00040002	/*  RL6 WL3. */

#ifdef DDR_1866
#define LPDDR3_MODE_REG_2		0x001C0002
#else
#define LPDDR3_MODE_REG_2		0x001A0002
#endif
#define LPDDR2_MODE_REG_2		0x00060002

#define DRAMC_REG_MRS 0x088
#define DRAMC_REG_PADCTL4 0x0e4
#define DRAMC_REG_LPDDR2_3 0x1e0
#define DRAMC_REG_SPCMD 0x1e4
#define DRAMC_REG_ACTIM1 0x1e8
#define DRAMC_REG_RRRATE_CTL 0x1f4
#define DRAMC_REG_MRR_CTL 0x1fc
#define DRAMC_REG_SPCMDRESP 0x3b8
/* #define READ_DRAM_TEMP_TEST */
#define READ_DRAM_FREQ_TEST

#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
/*Config*/
#define APDMA_TEST
/* #define APDMAREG_DUMP */
#define FREQ_HOPPING_TEST
#define VCORE1_ADJ_TEST
/* #define DUMP_DDR_RESERVED_MODE */
#define PHASE_NUMBER 3
#define DRAM_BASE             (0x40000000ULL)
#define BUFF_LEN   0x100
#define IOREMAP_ALIGMENT 0x1000
#define Delay_magic_num 0x295;	/* We use GPT to measurement how many clk pass in 100us */

/* #define HQA_HVcHVm */
/* #define HQA_HVcLVm */
/* #define HQA_LVcHVm */
/* #define HQA_LVcLVm */
/* #define HQA_NV */

extern unsigned int DMA_TIMES_RECORDER;
extern phys_addr_t get_max_DRAM_size(void);

int DFS_APDMA_Enable(void);
int DFS_APDMA_Init(void);
int DFS_APDMA_END(void);
int DFS_APDMA_early_init(void);
void DFS_APDMA_dummy_read_preinit(void);
void DFS_APDMA_dummy_read_deinit(void);
void dma_dummy_read_for_vcorefs(int loops);
void get_mempll_table_info(u32 *high_addr, u32 *low_addr, u32 *num);
int get_dram_data_rate(void);
unsigned int read_dram_temperature(void);
void sync_hw_gating_value(void);
int dram_can_support_fh(void);
int dram_fh_steps_freq(unsigned int step);
#if defined(CONFIG_ARCH_MT6753)
int enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1);
int exit_pasr_dpd_config(void);
#endif
#ifdef FREQ_HOPPING_TEST
int dram_do_dfs_by_fh(unsigned int freq);
#endif
#ifdef VCORE1_ADJ_TEST
extern void pmic_voltage_read(unsigned int nAdjust);
extern void pmic_Vcore_adjust(int nAdjust);
extern void pmic_Vmem_adjust(int nAdjust);
extern void pmic_Vmem_Cal_adjust(int nAdjust);
extern void pmic_HQA_NoSSC_Voltage_adjust(int nAdjust);
extern void pmic_HQA_Voltage_adjust(int nAdjust);
#endif
#ifdef DUMP_DDR_RESERVED_MODE
extern void dump_DMA_Reserved_AREA(void);
#endif
extern u32 get_devinfo_with_index(u32 index);
unsigned int ucDram_Register_Read(unsigned long u4reg_addr);
void ucDram_Register_Write(unsigned long u4reg_addr, unsigned int u4reg_value);
extern void *mt_dramc_base_get(void);
extern void *mt_dramc_nao_base_get(void);
extern void *mt_ddrphy_base_get(void);

enum DDRTYPE {
	TYPE_DDR1 = 1,
	TYPE_LPDDR2,
	TYPE_LPDDR3,
	TYPE_PCDDR3,
};
enum RANKNUM {
	SINGLE_RANK = 1,
	DUAL_RANK,
};

/************************** Common Macro *********************/
#define delay_a_while(count)	\
	do {						\
		register unsigned int delay;		\
		asm volatile ("mov %0, %1\n\t"		\
				"1:\n\t"		\
				"subs %0, %0, #1\n\t"	\
				"bne 1b\n\t"		\
				: "+r" (delay)		\
				: "r" (count)		\
				: "cc");		\
	} while (0)

#define mcDELAY_US(x)           delay_a_while((U32) (x*1000*10))

#endif	 /*__WDT_HW_H__*/
