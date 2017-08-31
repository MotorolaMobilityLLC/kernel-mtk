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

#ifndef __MT_EMI_MPU_H
#define __MT_EMI_MPU_H

/* Olympus EMI */
#define EMI_MPUD0_ST            (EMI_BASE_ADDR + 0x160)     /* violation status domain */
#define EMI_MPUD_ST(domain)     (EMI_MPUD0_ST + (4*domain)) /* violation status domain */
#define EMI_MPUD0_ST2           (EMI_BASE_ADDR + 0x200)     /* violation status domain */
#define EMI_MPUD_ST2(domain)    (EMI_MPUD0_ST2 + (4*domain))/* violation status domain */
#define EMI_MPUS                (EMI_BASE_ADDR + 0x01F0)    /* Memory protect unit control registers S */
#define EMI_MPUT                (EMI_BASE_ADDR + 0x01F8)    /* Memory protect unit control registers S */
#define EMI_MPUT_2ND            (EMI_BASE_ADDR + 0x01FC)    /* Memory protect unit control registers S */

#define EMI_MPU_CTRL            (0x0)
#define EMI_MPU_DBG             (0x4)
#define EMI_MPU_SA0             (0x100)
#define EMI_MPU_EA0             (0x200)
#define EMI_MPU_SA(region)      (EMI_MPU_SA0 + (region*4))
#define EMI_MPU_EA(region)      (EMI_MPU_EA0 + (region*4))
#define EMI_MPU_APC0            (0x300)
#define EMI_MPU_APC(region, domain)  (EMI_MPU_APC0 + (region*4) + ((domain/8)*0x100))
#define EMI_MPU_CTRL_D0         (0x800)
#define EMI_MPU_CTRL_D(domain)  (EMI_MPU_CTRL_D0 + (domain*4))
#define EMI_RG_MASK_D0          (0x900)
#define EMI_RG_MASK_D(domain)   (EMI_RG_MASK_D0 + (domain*4))
#define EMI_MPU_START           (0x0)
#define EMI_MPU_END             (0x980)

#define EMI_WP_ADR          (EMI_BASE_ADDR + 0x5E0)
#define EMI_WP_ADR_2ND      (EMI_BASE_ADDR + 0x5E4)
#define EMI_WP_CTRL         (EMI_BASE_ADDR + 0x5E8)
#define EMI_CHKER           (EMI_BASE_ADDR + 0x5F0)
#define EMI_CHKER_TYPE      (EMI_BASE_ADDR + 0x5F4)
#define EMI_CHKER_ADR       (EMI_BASE_ADDR + 0x5F8)
#define EMI_CHKER_ADR_2ND   (EMI_BASE_ADDR + 0x5FC)
#define EMI_CONA            (EMI_BASE_ADDR + 0x000)
#define EMI_CONH            (EMI_BASE_ADDR + 0x038)
#define EMI_CH_STA          (INFRA_BASE_ADDR + 0x254)

#define NO_PROTECTION   0
#define SEC_RW          1
#define SEC_RW_NSEC_R   2
#define SEC_RW_NSEC_W   3
#define SEC_R_NSEC_R    4
#define FORBIDDEN       5
#define SEC_R_NSEC_RW   6

#define EN_MPU_STR "ON"
#define DIS_MPU_STR "OFF"

#define EN_WP_STR "ON"
#define DIS_WP_STR "OFF"


/*EMI memory protection align 64K*/
#define EMI_MPU_ALIGNMENT 0x10000
#define OOR_VIO 0x00000200

#define MAX_CHANNELS  (4)
#define MAX_RANKS     (2)

#define EMI_MPU_REGION_NUMBER	(32)
#define EMI_MPU_DOMAIN_NUMBER	(32)

enum{		/* (M0) APMCU CH1 */
		MID_APMCU_CH1_0,  MID_APMCU_CH1_1,  MID_APMCU_CH1_2,
		MID_APMCU_CH1_3,  MID_APMCU_CH1_4,  MID_APMCU_CH1_5,
		MID_APMCU_CH1_6,  MID_APMCU_CH1_7,  MID_APMCU_CH1_8,
		MID_APMCU_CH1_9,  MID_APMCU_CH1_10, MID_APMCU_CH1_11,
		MID_APMCU_CH1_12, MID_APMCU_CH1_13, MID_APMCU_CH1_14,
		MID_APMCU_CH1_15, MID_APMCU_CH1_16, MID_APMCU_CH1_17,
		MID_APMCU_CH1_18, MID_APMCU_CH1_19, MID_APMCU_CH1_20,
		MID_APMCU_CH1_21, MID_APMCU_CH1_22, MID_APMCU_CH1_23,
		MID_APMCU_CH1_24, MID_APMCU_CH1_25, MID_APMCU_CH1_26,
		MID_APMCU_CH1_27, MID_APMCU_CH1_28,

		/* (M1) APMCU CH2 */
		MID_APMCU_CH2_0,  MID_APMCU_CH2_1,  MID_APMCU_CH2_2,
		MID_APMCU_CH2_3,  MID_APMCU_CH2_4,  MID_APMCU_CH2_5,
		MID_APMCU_CH2_6,  MID_APMCU_CH2_7,  MID_APMCU_CH2_8,
		MID_APMCU_CH2_9,  MID_APMCU_CH2_10, MID_APMCU_CH2_11,
		MID_APMCU_CH2_12, MID_APMCU_CH2_13, MID_APMCU_CH2_14,
		MID_APMCU_CH2_15, MID_APMCU_CH2_16, MID_APMCU_CH2_17,
		MID_APMCU_CH2_18, MID_APMCU_CH2_19, MID_APMCU_CH2_20,
		MID_APMCU_CH2_21, MID_APMCU_CH2_22, MID_APMCU_CH2_23,
		MID_APMCU_CH2_24, MID_APMCU_CH2_25, MID_APMCU_CH2_26,
		MID_APMCU_CH2_27, MID_APMCU_CH2_28,

		/* (M2) MM CH1 */
		MID_MM_CH1_0, MID_MM_CH1_1, MID_MM_CH1_2,
		MID_MM_CH1_3, MID_MM_CH1_4, MID_MM_CH1_5,
		MID_MM_CH1_6, MID_MM_CH1_7, MID_MM_CH1_8,
		MID_MM_CH1_9,

		/* (M3) MDMCU */
		MID_MDMCU_0, MID_MDMCU_1, MID_MDMCU_2,
		MID_MDMCU_3, MID_MDMCU_4,

		/* (M3) C2K */
		MID_C2K_0, MID_C2K_1, MID_C2K_2,
		MID_C2K_3,

		/* (M4) Infra/Peri */
		MID_INFRA_0,  MID_INFRA_1,  MID_INFRA_2,
		MID_INFRA_3,  MID_INFRA_4,  MID_INFRA_5,
		MID_INFRA_6,  MID_INFRA_7,  MID_INFRA_8,
		MID_INFRA_9,  MID_INFRA_10, MID_INFRA_11,
		MID_INFRA_12, MID_INFRA_13, MID_INFRA_14,
		MID_INFRA_15, MID_INFRA_16, MID_INFRA_17,
		MID_INFRA_18, MID_INFRA_19, MID_INFRA_20,
		MID_INFRA_21, MID_INFRA_22, MID_INFRA_23,
		MID_INFRA_24, MID_INFRA_25, MID_INFRA_26,
		MID_INFRA_27, MID_INFRA_28, MID_INFRA_29,
		MID_INFRA_30, MID_INFRA_31, MID_INFRA_32,
		MID_INFRA_33, MID_INFRA_34, MID_INFRA_35,

		/* (M4) MDDMA */
		MID_MDDMA_0,  MID_MDDMA_1,  MID_MDDMA_2,
		MID_MDDMA_3,  MID_MDDMA_4,  MID_MDDMA_5,
		MID_MDDMA_6,  MID_MDDMA_7,  MID_MDDMA_8,
		MID_MDDMA_9,  MID_MDDMA_10, MID_MDDMA_11,
		MID_MDDMA_12, MID_MDDMA_13, MID_MDDMA_14,
		MID_MDDMA_15, MID_MDDMA_16,

		/* (M4) L1SYS */
		MID_L1SYS_0,  MID_L1SYS_1,  MID_L1SYS_2,
		MID_L1SYS_3,  MID_L1SYS_4,  MID_L1SYS_5,
		MID_L1SYS_6,  MID_L1SYS_7,  MID_L1SYS_8,
		MID_L1SYS_9,  MID_L1SYS_10, MID_L1SYS_11,
		MID_L1SYS_12, MID_L1SYS_13, MID_L1SYS_14,
		MID_L1SYS_15, MID_L1SYS_16, MID_L1SYS_17,
		MID_L1SYS_18, MID_L1SYS_19, MID_L1SYS_20,
		MID_L1SYS_21, MID_L1SYS_22, MID_L1SYS_23,
		MID_L1SYS_24,

		/* (M5) MM CH2 */
		MID_MM_CH2_0, MID_MM_CH2_1, MID_MM_CH2_2,
		MID_MM_CH2_3, MID_MM_CH2_4, MID_MM_CH2_5,
		MID_MM_CH2_6, MID_MM_CH2_7, MID_MM_CH2_8,
		MID_MM_CH2_9,

		/* (M6) GPU CH1 */
		MID_GPU_CH1_0,

		/* (M7) GPU CH2 */
		MID_GPU_CH2_0,

		MST_INVALID,
		NR_MST,
};

enum {
	AXI_VIO_ID = 0,
	AXI_ADR_CHK_EN = 16,
	AXI_LOCK_CHK_EN = 17,
	AXI_NON_ALIGN_CHK_EN = 18,
	AXI_NON_ALIGN_CHK_MST = 20,
	AXI_VIO_CLR = 24,
	AXI_VIO_WR = 27,
	AXI_ADR_VIO = 28,
	AXI_LOCK_ISSUE = 29,
	AXI_NON_ALIGN_ISSUE = 30
};
enum {
	D0_CPU_PREI,
	D1_MD1,
	D2_CONN,
	D3_SCP,
	D4_MM,
	D5_MM3,
	D6_MFG,
	D7_MDHW
};
#define EMI_WP_RANGE        0x0000003F
#define EMI_WP_AXI_ID       0x0000FFFF
#define EMI_WP_RW_MONITOR   0x000000C0
#define EMI_WP_RW_DISABLE   0x00000300
#define WP_BOTH_READ_WRITE  3

#define EMI_WP_RW_MONITOR_SHIFT  6
#define EMI_WP_RW_DISABLE_SHIFT  8
#define EMI_WP_SLVERR_SHIFT     10
#define EMI_WP_INT_SHIFT        15
#define EMI_WP_ENABLE_SHIFT     19
#define EMI_WP_VIO_CLR_SHIFT    24

enum {
	MASTER_APMCU_CH1 = 0,
	MASTER_APMCU_CH2 = 1,
	MASTER_MM_CH1 = 2,
	MASTER_MDMCU,
	MASTER_MDDMA,
	MASTER_MM_CH2,
	MASTER_GPU_CH1,
	MASTER_GPU_CH2,
	MASTER_ALL = 8
};

/* Basic DRAM setting */
struct basic_dram_setting {
	/* Number of channels */
	unsigned channel_nr;
	/* Per-channel information */
	struct {
		/*does this ch exist*/
		bool valid_ch;
		/* Per-rank information */
		struct {
			/* Does this rank exist */
			bool valid_rank;
			/* Rank size - (in Gb)*/
			unsigned rank_size;
			/* Number of segments */
			unsigned segment_nr;
		} rank[MAX_RANKS];
	} channel[MAX_CHANNELS];
};

typedef void (*emi_mpu_notifier)(u32 addr, int wr_vio);

#define SET_ACCESS_PERMISSON(d7, d6, d5, d4, d3, d2, d1, d0) \
(((d7) << 21) | ((d6) << 18) | ((d5) << 15) | \
((d4) << 12) | ((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))

extern int emi_mpu_set_region_protection(unsigned long long start_addr,
unsigned long long end_addr, int region, unsigned int access_permission);
#if defined(CONFIG_MTKPASR)
extern void acquire_dram_setting(struct basic_dram_setting *pasrdpd);
#endif
extern void emi_wp_get_status(void);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_base_set(void *base);
extern void *mt_emi_reg_base_get(void);
extern int set_emi_mpu_region_intr_mask(unsigned int region, unsigned int domain);
extern int emi_mpu_get_violation_port(void);
extern phys_addr_t get_max_DRAM_size(void);
extern void __iomem *EMI_BASE_ADDR;
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern int mt_emi_mpu_set_region_protection(unsigned long long start,
unsigned long long end, unsigned int region_permission);
extern void dump_emi_latency(void);
extern unsigned int get_emi_channel_number(void);
extern int lpdma_emi_ch23_get_status(void);
#endif  /* !__MT_EMI_MPU_H */
