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

#define EMI_MPUA (0x0160)
#define EMI_MPUB (0x0168)
#define EMI_MPUC (0x0170)
#define EMI_MPUD (0x0178)
#define EMI_MPUE (0x0180)
#define EMI_MPUF (0x0188)
#define EMI_MPUG (0x0190)
#define EMI_MPUH (0x0198)
#define EMI_MPUI (0x01A0)
#define EMI_MPUJ (0x01A8)
#define EMI_MPUK (0x01B0)
#define EMI_MPUL (0x01B8)
#define EMI_MPUM (0x01C0)
#define EMI_MPUN (0x01C8)
#define EMI_MPUO (0x01D0)
#define EMI_MPUP (0x01D8)
#define EMI_MPUQ (0x01E0)
#define EMI_MPUR (0x01E8)
#define EMI_MPUS (0x01F0)
#define EMI_MPUT (0x01F8)
#define EMI_MPUU (0x0200)
#define EMI_MPUY (0x0220)

#define EMI_WP_ADR     (0x5E0)
#define EMI_WP_CTRL    (0x5E8)
#define EMI_CHKER      (0x5F0)
#define EMI_CHKER_TYPE (0x5F4)
#define EMI_CHKER_ADR  (0x5F8)

#define EMI_MPU_START_OFFSET EMI_MPUA
#define EMI_MPU_END_OFFSET   EMI_MPUY

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

enum {
	/* apmcu write */
	MST_ID_APMCU_W0,
	MST_ID_APMCU_W1,
	MST_ID_APMCU_W2,
	MST_ID_APMCU_W3,
	MST_ID_APMCU_W4,

	/* apmcu read */
	MST_ID_APMCU_R0,
	MST_ID_APMCU_R1,
	MST_ID_APMCU_R2,
	MST_ID_APMCU_R3,
	MST_ID_APMCU_R4,
	MST_ID_APMCU_R5,
	MST_ID_APMCU_R6,
	MST_ID_APMCU_R7,
	MST_ID_APMCU_R8,
	MST_ID_APMCU_R9,
	MST_ID_APMCU_R10,
	MST_ID_APMCU_R11,
	MST_ID_APMCU_R12,
	MST_ID_APMCU_R13,

	/* MM */
	MST_ID_MM_0,
	MST_ID_MM_1,
	MST_ID_MM_2,
	MST_ID_MM_3,
	MST_ID_MM_4,
	MST_ID_MM_5,
	MST_ID_MM_6,

	/* Periperal */
	MST_ID_PERI_0,
	MST_ID_PERI_1,
	MST_ID_PERI_2,
	MST_ID_PERI_3,
	MST_ID_PERI_4,
	MST_ID_PERI_5,
	MST_ID_PERI_6,
	MST_ID_PERI_7,
	MST_ID_PERI_8,
	MST_ID_PERI_9,
	MST_ID_PERI_10,

	/* MDMCU  */
	MST_ID_MDMCU_0,
	/* MDHW  */
	MST_ID_MDHW_0,

	/* MFG  */
	MST_ID_MFG_0,

	/* CONN  */
	MST_ID_CONN_0,

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

#define EMI_WP_RANGE       0x0000003F
#define EMI_WP_AXI_ID      0x0000FFFF
#define EMI_WP_RW_MONITOR  0x000000C0
#define EMI_WP_RW_DISABLE  0x00000300
#define WP_BOTH_READ_WRITE 3

#define EMI_WP_RW_MONITOR_SHIFT    6
#define EMI_WP_RW_DISABLE_SHIFT    8
#define EMI_WP_SLVERR_SHIFT     10
#define EMI_WP_INT_SHIFT     15
#define EMI_WP_ENABLE_SHIFT     19
#define EMI_WP_VIO_CLR_SHIFT    24

enum {
	MONITOR_PORT_APMCU = 0,
	MONITOR_PORT_MM,
	MONITOR_PORT_PERI,
	MONITOR_PORT_MDMCU,
	MONITOR_PORT_MDHW,
	MONITOR_PORT_MFG,
	MONITOR_PORT_CONN,
	MONITOR_PORT_ALL
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
#define SET_ACCESS_PERMISSON(d3, d2, d1, d0) \
	(((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))

extern int emi_mpu_set_region_protection(unsigned int start_addr,
					 unsigned int end_addr,
					 int region,
					 unsigned int access_permission);
extern int emi_mpu_notifier_register(int master, emi_mpu_notifier notifider);
extern void acquire_dram_setting(struct basic_dram_setting *pasrdpd);
extern void emi_wp_get_status(void);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_base_set(void *base);
extern void *mt_emi_reg_base_get(void);
extern phys_addr_t get_max_DRAM_size(void);
#endif  /* !__MT_EMI_MPU_H */
