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

#define EMI_MPUA		(0x0160)
#define EMI_MPUB		(0x0168)
#define EMI_MPUC		(0x0170)
#define EMI_MPUD		(0x0178)
#define EMI_MPUE		(0x0180)
#define EMI_MPUF		(0x0188)
#define EMI_MPUG		(0x0190)
#define EMI_MPUH		(0x0198)
#define EMI_MPUA2		(0x0260)
#define EMI_MPUB2		(0x0268)
#define EMI_MPUC2		(0x0270)
#define EMI_MPUD2		(0x0278)
#define EMI_MPUE2		(0x0280)
#define EMI_MPUF2		(0x0288)
#define EMI_MPUG2		(0x0290)
#define EMI_MPUH2		(0x0298)
#define EMI_MPUA3		(0x0360)
#define EMI_MPUB3		(0x0368)
#define EMI_MPUC3		(0x0370)
#define EMI_MPUD3		(0x0378)
#define EMI_MPUE3		(0x0380)
#define EMI_MPUF3		(0x0388)
#define EMI_MPUG3		(0x0390)
#define EMI_MPUH3		(0x0398)

#define EMI_MPUI		   (0x01A0)
#define EMI_MPUI_2ND	 (0x01A4)
#define EMI_MPUJ		   (0x01A8)
#define EMI_MPUJ_2ND	 (0x01AC)
#define EMI_MPUK		   (0x01B0)
#define EMI_MPUK_2ND	 (0x01B4)
#define EMI_MPUL		   (0x01B8)
#define EMI_MPUL_2ND	 (0x01BC)
#define EMI_MPUI2				(0x02A0)
#define EMI_MPUI2_2ND	 (0x02A4)
#define EMI_MPUJ2		   (0x02A8)
#define EMI_MPUJ2_2ND	 (0x02AC)
#define EMI_MPUK2		   (0x02B0)
#define EMI_MPUK2_2ND	 (0x02B4)
#define EMI_MPUL2		   (0x02B8)
#define EMI_MPUL2_2ND	 (0x02BC)
#define EMI_MPUI3		   (0x03A0)
#define EMI_MPUI3_2ND	 (0x03A4)
#define EMI_MPUJ3		   (0x03A8)
#define EMI_MPUJ3_2ND	 (0x03AC)
#define EMI_MPUK3		   (0x03B0)
#define EMI_MPUK3_2ND	 (0x03B4)
#define EMI_MPUL3		   (0x03B8)
#define EMI_MPUL3_2ND	 (0x03BC)

#define EMI_MPUM        (0x01C0)
#define EMI_MPUN        (0x01C8)
#define EMI_MPUO        (0x01D0)
#define EMI_MPUU        (0x0200)
#define EMI_MPUM2        (0x02C0)
#define EMI_MPUN2        (0x02C8)
#define EMI_MPUO2        (0x02D0)
#define EMI_MPUU2        (0x0300)

#define EMI_MPUV        (0x0208)
#define EMI_MPUW		(0x0210)
#define EMI_MPUX        (0x0218)
#define EMI_MPU_START   (0x0160)
#define EMI_MPU_END     (0x03BC)

#define EMI_CONA		(EMI_BASE_ADDR + 0x0000)
#define EMI_CONH		(EMI_BASE_ADDR + 0x0038)

#define EMI_MPUP		(EMI_BASE_ADDR+0x01D8)
#define EMI_MPUQ		(EMI_BASE_ADDR+0x01E0)
#define EMI_MPUR		(EMI_BASE_ADDR+0x01E8)
#define EMI_MPUS		(EMI_BASE_ADDR+0x01F0)
#define EMI_MPUT		(EMI_BASE_ADDR+0x01F8)
#define EMI_MPUY		(EMI_BASE_ADDR+0x0220)

#define EMI_MPUP2		(EMI_BASE_ADDR+0x02D8)
#define EMI_MPUQ2		(EMI_BASE_ADDR+0x02E0)
#define EMI_MPUR2		(EMI_BASE_ADDR+0x02E8)
#define EMI_MPUY2		(EMI_BASE_ADDR+0x0320)

#define EMI_WP_ADR       (EMI_BASE_ADDR + 0x5E0)
#define EMI_WP_CTRL      (EMI_BASE_ADDR + 0x5E8)
#define EMI_CHKER        (EMI_BASE_ADDR + 0x5F0)
#define EMI_CHKER_TYPE   (EMI_BASE_ADDR + 0x5F4)
#define EMI_CHKER_ADR    (EMI_BASE_ADDR + 0x5F8)

#define NO_PROTECTION 0
#define SEC_RW 1
#define SEC_RW_NSEC_R 2
#define SEC_RW_NSEC_W 3
#define SEC_R_NSEC_R 4
#define FORBIDDEN 5
#define SEC_R_NSEC_RW 6

#define EN_MPU_STR "ON"
#define DIS_MPU_STR "OFF"

#define EN_WP_STR "ON"
#define DIS_WP_STR "OFF"


/*EMI memory protection align 64K*/
#define EMI_MPU_ALIGNMENT 0x10000
#define OOR_VIO 0x00000200

#define MAX_CHANNELS	(2)
#define MAX_RANKS	(2)


enum{    /* apmcu_ch1 */
		MST_ID_APMCU_CH1_0, MST_ID_APMCU_CH1_1,
		MST_ID_APMCU_CH1_2, MST_ID_APMCU_CH1_3, MST_ID_APMCU_CH1_4,
		MST_ID_APMCU_CH1_5, MST_ID_APMCU_CH1_6,
		MST_ID_APMCU_CH1_7, MST_ID_APMCU_CH1_8, MST_ID_APMCU_CH1_9,
		MST_ID_APMCU_CH1_10, MST_ID_APMCU_CH1_11,
		MST_ID_APMCU_CH1_12, MST_ID_APMCU_CH1_13, MST_ID_APMCU_CH1_14,
		MST_ID_APMCU_CH1_15, MST_ID_APMCU_CH1_16,
		MST_ID_APMCU_CH1_17, MST_ID_APMCU_CH1_18, MST_ID_APMCU_CH1_19,
		MST_ID_APMCU_CH1_20, MST_ID_APMCU_CH1_21,
		MST_ID_APMCU_CH1_22, MST_ID_APMCU_CH1_23, MST_ID_APMCU_CH1_24,
		MST_ID_APMCU_CH1_25, MST_ID_APMCU_CH1_26,
		MST_ID_APMCU_CH1_27, MST_ID_APMCU_CH1_28, MST_ID_APMCU_CH1_29,
		MST_ID_APMCU_CH1_30, MST_ID_APMCU_CH1_31,
		MST_ID_APMCU_CH1_32, MST_ID_APMCU_CH1_33, MST_ID_APMCU_CH1_34,
		MST_ID_APMCU_CH1_35, MST_ID_APMCU_CH1_36,
		MST_ID_APMCU_CH1_37, MST_ID_APMCU_CH1_38, MST_ID_APMCU_CH1_39,
		MST_ID_APMCU_CH1_40, MST_ID_APMCU_CH1_41,
		MST_ID_APMCU_CH1_42, MST_ID_APMCU_CH1_43, MST_ID_APMCU_CH1_44,
		MST_ID_APMCU_CH1_45, MST_ID_APMCU_CH1_46, MST_ID_APMCU_CH1_47,
		MST_ID_APMCU_CH1_48, MST_ID_APMCU_CH1_49, MST_ID_APMCU_CH1_50,

		/* apmcu_ch2 */
		MST_ID_APMCU_CH2_0, MST_ID_APMCU_CH2_1,
		MST_ID_APMCU_CH2_2, MST_ID_APMCU_CH2_3, MST_ID_APMCU_CH2_4,
		MST_ID_APMCU_CH2_5, MST_ID_APMCU_CH2_6,
		MST_ID_APMCU_CH2_7, MST_ID_APMCU_CH2_8, MST_ID_APMCU_CH2_9,
		MST_ID_APMCU_CH2_10, MST_ID_APMCU_CH2_11,
		MST_ID_APMCU_CH2_12, MST_ID_APMCU_CH2_13, MST_ID_APMCU_CH2_14,
		MST_ID_APMCU_CH2_15, MST_ID_APMCU_CH2_16,
		MST_ID_APMCU_CH2_17, MST_ID_APMCU_CH2_18, MST_ID_APMCU_CH2_19,
		MST_ID_APMCU_CH2_20, MST_ID_APMCU_CH2_21,
		MST_ID_APMCU_CH2_22, MST_ID_APMCU_CH2_23, MST_ID_APMCU_CH2_24,
		MST_ID_APMCU_CH2_25, MST_ID_APMCU_CH2_26,
		MST_ID_APMCU_CH2_27, MST_ID_APMCU_CH2_28, MST_ID_APMCU_CH2_29,
		MST_ID_APMCU_CH2_30, MST_ID_APMCU_CH2_31,
		MST_ID_APMCU_CH2_32, MST_ID_APMCU_CH2_33, MST_ID_APMCU_CH2_34,
		MST_ID_APMCU_CH2_35, MST_ID_APMCU_CH2_36,
		MST_ID_APMCU_CH2_37, MST_ID_APMCU_CH2_38, MST_ID_APMCU_CH2_39,
		MST_ID_APMCU_CH2_40, MST_ID_APMCU_CH2_41,
		MST_ID_APMCU_CH2_42, MST_ID_APMCU_CH2_43, MST_ID_APMCU_CH2_44,
		MST_ID_APMCU_CH2_45, MST_ID_APMCU_CH2_46, MST_ID_APMCU_CH2_47,
		MST_ID_APMCU_CH2_48, MST_ID_APMCU_CH2_49, MST_ID_APMCU_CH2_50,

		/* MM ch1 */
		MST_ID_MM_CH1_0, MST_ID_MM_CH1_1, MST_ID_MM_CH1_2,
		MST_ID_MM_CH1_3, MST_ID_MM_CH1_4, MST_ID_MM_CH1_5,
		MST_ID_MM_CH1_6, MST_ID_MM_CH1_7, MST_ID_MM_CH1_8,

		/* Modem */
		MST_ID_MDMCU_0, MST_ID_MDMCU_1, MST_ID_MDMCU_2, MST_ID_MDMCU_3,
		MST_ID_MD_L1MCU_0, MST_ID_C2KMCU_0, MST_ID_C2KMCU_1,
		MST_ID_C2KMCU_2,

		/* Modem HW (2G/3G) */
		MST_ID_MDHW_0, MST_ID_MDHW_1, MST_ID_MDHW_2,
		MST_ID_MDHW_3, MST_ID_MDHW_4,
		MST_ID_MDHW_5, MST_ID_MDHW_6, MST_ID_MDHW_7,
		MST_ID_MDHW_8, MST_ID_MDHW_9,	MST_ID_MDHW_10,
		MST_ID_MDHW_11, MST_ID_MDHW_12,	MST_ID_MDHW_13,
		MST_ID_MDHW_14, MST_ID_MDHW_15,	MST_ID_MDHW_16,
		MST_ID_MDHW_17, MST_ID_MDHW_18,	MST_ID_MDHW_19,
		MST_ID_MDHW_20, MST_ID_MDHW_21,	MST_ID_MDHW_22,
		MST_ID_MDHW_23, MST_ID_MDHW_24,	MST_ID_MDHW_25,
		MST_ID_MDHW_26, MST_ID_MDHW_27,	MST_ID_MDHW_28,
		MST_ID_MDHW_29, MST_ID_MDHW_30,	MST_ID_MDHW_31,
		MST_ID_MDHW_32, MST_ID_MDHW_33,	MST_ID_MDHW_34,
		MST_ID_MDHW_35, MST_ID_MDHW_36,	MST_ID_MDHW_37,
		MST_ID_MDHW_38,

		/* MM ch2 */
		MST_ID_MM_CH2_0, MST_ID_MM_CH2_1, MST_ID_MM_CH2_2,
		MST_ID_MM_CH2_3, MST_ID_MM_CH2_4, MST_ID_MM_CH2_5,
		MST_ID_MM_CH2_6, MST_ID_MM_CH2_7, MST_ID_MM_CH2_8,

		/* Periperal */
		MST_ID_PERI_0, MST_ID_PERI_1,
		MST_ID_PERI_2, MST_ID_PERI_3, MST_ID_PERI_4,
		MST_ID_PERI_5, MST_ID_PERI_6,
		MST_ID_PERI_7, MST_ID_PERI_8, MST_ID_PERI_9,
		MST_ID_PERI_10, MST_ID_PERI_11,
		MST_ID_PERI_12, MST_ID_PERI_13, MST_ID_PERI_14,
		MST_ID_PERI_15, MST_ID_PERI_16, MST_ID_PERI_17,
		MST_ID_PERI_18, MST_ID_PERI_19,	MST_ID_PERI_20,
		MST_ID_PERI_21, MST_ID_PERI_22,

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

#define EMI_WP_RANGE					0x0000003F
#define EMI_WP_AXI_ID                 0x0000FFFF
#define EMI_WP_RW_MONITOR       0x000000C0
#define EMI_WP_RW_DISABLE        0x00000300
#define WP_BOTH_READ_WRITE      3

#define EMI_WP_RW_MONITOR_SHIFT    6
#define EMI_WP_RW_DISABLE_SHIFT    8
#define EMI_WP_SLVERR_SHIFT     10
#define EMI_WP_INT_SHIFT     15
#define EMI_WP_ENABLE_SHIFT     19
#define EMI_WP_VIO_CLR_SHIFT    24


enum {
	MASTER_APMCU_CH1 = 0,
	MASTER_APMCU_CH2 = 1,
	MASTER_MM_CH1 = 2,
	MASTER_MDMCU,
	MASTER_MDHW,
	MASTER_MM_CH2,
	MASTER_PERI,
	MASTER_MFG,
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
extern void __iomem *EMI_BASE_ADDR;
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern int mt_emi_mpu_set_region_protection(unsigned long long start,
unsigned long long end, unsigned int region_permission);
#endif  /* !__MT_EMI_MPU_H */
