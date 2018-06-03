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
#define EMI_MPU_END             (0x91C)

#define EMI_WP_ADR          (EMI_BASE_ADDR + 0x5E0)
#define EMI_WP_ADR_2ND      (EMI_BASE_ADDR + 0x5E4)
#define EMI_WP_CTRL         (EMI_BASE_ADDR + 0x5E8)
#define EMI_CHKER           (EMI_BASE_ADDR + 0x5F0)
#define EMI_CHKER_TYPE      (EMI_BASE_ADDR + 0x5F4)
#define EMI_CHKER_ADR       (EMI_BASE_ADDR + 0x5F8)
#define EMI_CHKER_ADR_2ND   (EMI_BASE_ADDR + 0x5FC)
#define EMI_CONA            (EMI_BASE_ADDR + 0x000)
#define EMI_CONH            (EMI_BASE_ADDR + 0x038)

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

#define MAX_CHANNELS  (2)
#define MAX_RANKS     (2)

#define EMI_MPU_REGION_NUMBER	(24)
#define EMI_MPU_DOMAIN_NUMBER	(8)

enum{    /* apmcu */
		MST_ID_APMCU_0, MST_ID_APMCU_1, MST_ID_APMCU_2,
		MST_ID_APMCU_3, MST_ID_APMCU_4, MST_ID_APMCU_5,
		MST_ID_APMCU_6, MST_ID_APMCU_7, MST_ID_APMCU_8,
		MST_ID_APMCU_9,  MST_ID_APMCU_10, MST_ID_APMCU_11,
		MST_ID_APMCU_12, MST_ID_APMCU_13, MST_ID_APMCU_14,
		MST_ID_APMCU_15, MST_ID_APMCU_16,

		/* MM */
		MST_ID_MM_0, MST_ID_MM_1, MST_ID_MM_2,
		MST_ID_MM_3, MST_ID_MM_4,

		/* INFRA */
		MST_ID_INFRA_0, MST_ID_INFRA_1, MST_ID_INFRA_2,
		MST_ID_INFRA_3, MST_ID_INFRA_4, MST_ID_INFRA_5,
		MST_ID_INFRA_6, MST_ID_INFRA_7, MST_ID_INFRA_8,
		MST_ID_INFRA_9,  MST_ID_INFRA_10, MST_ID_INFRA_11,
		MST_ID_INFRA_12, MST_ID_INFRA_13, MST_ID_INFRA_14,
		MST_ID_INFRA_15, MST_ID_INFRA_16, MST_ID_INFRA_17,
		MST_ID_INFRA_18, MST_ID_INFRA_19,

		/* Modem (EMI port 3)*/
		MST_ID_MD_IA_0, MST_ID_MD_IA_1,
		MST_ID_MD_USIP_0, MST_ID_MD_USIP_1, MST_ID_MD_USIP_2,
		MST_ID_MD_USIP_3, MST_ID_MD_USIP_4, MST_ID_MD_USIP_5,

		/* Modem (EMI port 4) */
		MST_ID_MD_RXBRP_0, MST_ID_MD_RXBRP_1, MST_ID_MD_RXBRP_2,
		MST_ID_MD_RXBRP_3, MST_ID_MD_RXBRP_4, MST_ID_MD_RXBRP_5,
		MST_ID_MD_RXBRP_6,
		MST_ID_MD_Bigram_0, MST_ID_MD_Bigram_1,
		MST_ID_MD_MD2G_0,
		MST_ID_MD_TXBRP_0, MST_ID_MD_TXBRP_1,
		MST_ID_MD_TXDFE_0, MST_ID_MD_TXDFE_1,
		MST_ID_MD_RXDFESYS_0, MST_ID_MD_RXDFESYS_1, MST_ID_MD_RXDFESYS_2,
		MST_ID_MD_CS_0, MST_ID_MD_CS_1, MST_ID_MD_CS_2,
		MST_ID_MD_mml2_0, MST_ID_MD_mml2_1, MST_ID_MD_mml2_2, MST_ID_MD_mml2_3,
		MST_ID_MD_mdinfra_0, MST_ID_MD_mdinfra_1, MST_ID_MD_mdinfra_2,
		MST_ID_MD_mdinfra_3, MST_ID_MD_mdinfra_4,
		MST_ID_MD_MDPERI_0, MST_ID_MD_MDPERI_1,

		/* MFG  */
		MST_ID_MFG_0,

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
	MASTER_MDHW,
	MASTER_MM_CH2,
	MASTER_MFG,
	MASTER_PERI,
	MASTER_ALL = 8
};

/* Basic DRAM setting */
struct basic_dram_setting {
	/* Number of channels */
	unsigned channel_nr;
	/* Per-channel information */
	struct {
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
extern int emi_mpu_get_violation_port(void);
extern phys_addr_t get_max_DRAM_size(void);
/* extern void __iomem *EMI_BASE_ADDR; */
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern int mt_emi_mpu_set_region_protection(unsigned long long start,
unsigned long long end, unsigned int region_permission);
extern void dump_emi_latency(void);
extern void dump_emi_MM(void);
#endif  /* !__MT_EMI_MPU_H */
