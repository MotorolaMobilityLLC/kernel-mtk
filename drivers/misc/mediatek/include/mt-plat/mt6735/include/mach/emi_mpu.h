#ifndef __MT_EMI_MPU_H
#define __MT_EMI_MPU_H

#define EMI_MPUA		(EMI_BASE_ADDR+0x0160)
#define EMI_MPUB		(EMI_BASE_ADDR+0x0168)
#define EMI_MPUC		(EMI_BASE_ADDR+0x0170)
#define EMI_MPUD		(EMI_BASE_ADDR+0x0178)
#define EMI_MPUE		(EMI_BASE_ADDR+0x0180)
#define EMI_MPUF		(EMI_BASE_ADDR+0x0188)
#define EMI_MPUG		(EMI_BASE_ADDR+0x0190)
#define EMI_MPUH		(EMI_BASE_ADDR+0x0198)
#define EMI_MPUI		(EMI_BASE_ADDR+0x01A0)
#define EMI_MPUI_2ND	(EMI_BASE_ADDR+0x01A4)
#define EMI_MPUJ		(EMI_BASE_ADDR+0x01A8)
#define EMI_MPUJ_2ND	(EMI_BASE_ADDR+0x01AC)
#define EMI_MPUK		(EMI_BASE_ADDR+0x01B0)
#define EMI_MPUK_2ND	(EMI_BASE_ADDR+0x01B4)
#define EMI_MPUL		(EMI_BASE_ADDR+0x01B8)
#define EMI_MPUL_2ND	(EMI_BASE_ADDR+0x01BC)
#define EMI_MPUM		(EMI_BASE_ADDR+0x01C0)
#define EMI_MPUN		(EMI_BASE_ADDR+0x01C8)
#define EMI_MPUO		(EMI_BASE_ADDR+0x01D0)
#define EMI_MPUP		(EMI_BASE_ADDR+0x01D8)
#define EMI_MPUQ		(EMI_BASE_ADDR+0x01E0)
#define EMI_MPUR		(EMI_BASE_ADDR+0x01E8)
#define EMI_MPUS		(EMI_BASE_ADDR+0x01F0)
#define EMI_MPUT		(EMI_BASE_ADDR+0x01F8)
#define EMI_MPUU		(EMI_BASE_ADDR+0x0200)
#define EMI_MPUY		(EMI_BASE_ADDR+0x0220)
#define EMI_MPUA2		(EMI_BASE_ADDR+0x0260)
#define EMI_MPUB2		(EMI_BASE_ADDR+0x0268)
#define EMI_MPUC2		(EMI_BASE_ADDR+0x0270)
#define EMI_MPUD2		(EMI_BASE_ADDR+0x0278)
#define EMI_MPUE2		(EMI_BASE_ADDR+0x0280)
#define EMI_MPUF2		(EMI_BASE_ADDR+0x0288)
#define EMI_MPUG2		(EMI_BASE_ADDR+0x0290)
#define EMI_MPUH2		(EMI_BASE_ADDR+0x0298)
#define EMI_MPUI2		(EMI_BASE_ADDR+0x02A0)
#define EMI_MPUI2_2ND	(EMI_BASE_ADDR+0x02A4)
#define EMI_MPUJ2		(EMI_BASE_ADDR+0x02A8)
#define EMI_MPUJ2_2ND	(EMI_BASE_ADDR+0x02AC)
#define EMI_MPUK2		(EMI_BASE_ADDR+0x02B0)
#define EMI_MPUK2_2ND	(EMI_BASE_ADDR+0x02B4)
#define EMI_MPUL2		(EMI_BASE_ADDR+0x02B8)
#define EMI_MPUL2_2ND	(EMI_BASE_ADDR+0x02BC)
#define EMI_MPUM2		(EMI_BASE_ADDR+0x02C0)
#define EMI_MPUN2		(EMI_BASE_ADDR+0x02C8)
#define EMI_MPUO2		(EMI_BASE_ADDR+0x02D0)
#define EMI_MPUP2		(EMI_BASE_ADDR+0x02D8)
#define EMI_MPUQ2		(EMI_BASE_ADDR+0x02E0)
#define EMI_MPUR2		(EMI_BASE_ADDR+0x02E8)
#define EMI_MPUU2		(EMI_BASE_ADDR+0x0300)
#define EMI_MPUY2		(EMI_BASE_ADDR+0x0320)
#define EMI_WP_ADR    (EMI_BASE_ADDR+0x5E0)
#define EMI_WP_CTRL   (EMI_BASE_ADDR+0x5E8)
#define EMI_CHKER        (EMI_BASE_ADDR+0x5F0)
#define EMI_CHKER_TYPE    (EMI_BASE_ADDR+0x5F4)
#define EMI_CHKER_ADR   (EMI_BASE_ADDR+0x5F8)

#define EMI_CONA    (EMI_BASE_ADDR+0x00)
#define EMI_CONH    (EMI_BASE_ADDR+0x38)

#define EMI_MPU_START   (0x0160)
#define EMI_MPU_END     (0x03B8)
#define EMI_MPU_START_OFFSET EMI_MPUA
#define EMI_MPU_END_OFFSET   EMI_MPUY2

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
	/* apmcu */
	MST_ID_APMCU_0, MST_ID_APMCU_1, MST_ID_APMCU_2,
	MST_ID_APMCU_3, MST_ID_APMCU_4, MST_ID_APMCU_5,
	MST_ID_APMCU_6, MST_ID_APMCU_7, MST_ID_APMCU_8,
	MST_ID_APMCU_9, MST_ID_APMCU_10,
#if defined(CONFIG_ARCH_MT6753)
	MST_ID_APMCU_11, MST_ID_APMCU_12, MST_ID_APMCU_13,
	MST_ID_APMCU_14, MST_ID_APMCU_15, MST_ID_APMCU_16,
	MST_ID_APMCU_17, MST_ID_APMCU_18, MST_ID_APMCU_19,
	MST_ID_APMCU_20, MST_ID_APMCU_21,
#endif
	/* MM */
	MST_ID_MM_0, MST_ID_MM_1, MST_ID_MM_2, MST_ID_MM_3, MST_ID_MM_4,
	/* Periperal */
	MST_ID_PERI_0, MST_ID_PERI_1, MST_ID_PERI_2,
	MST_ID_PERI_3, MST_ID_PERI_4,	MST_ID_PERI_5,
	MST_ID_PERI_6, MST_ID_PERI_7, MST_ID_PERI_8,
	MST_ID_PERI_9, MST_ID_PERI_10, MST_ID_PERI_11,
	MST_ID_PERI_12, MST_ID_PERI_13, MST_ID_PERI_14,
	MST_ID_PERI_15, MST_ID_PERI_16, MST_ID_PERI_17,
	MST_ID_PERI_18, MST_ID_PERI_19,
	/* Modem */
	MST_ID_MDMCU_0, MST_ID_MDMCU_1, MST_ID_MDMCU_2, MST_ID_MDMCU_3,
	MST_ID_C2KMCU_0, MST_ID_C2KMCU_1, MST_ID_C2KMCU_2,
	/* Modem HW (2G/3G) */
	MST_ID_MDHW_0, MST_ID_MDHW_1, MST_ID_MDHW_2, MST_ID_MDHW_3,
	MST_ID_MDHW_4, MST_ID_MDHW_5,	MST_ID_MDHW_6, MST_ID_MDHW_7,
	MST_ID_MDHW_8, MST_ID_MDHW_9,
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
	MASTER_APMCU = 0,
	MASTER_APMCU_2 = 1,
	MASTER_MM,
	MASTER_MDMCU,
	MASTER_MDHW,
	MASTER_MM_2,
	MASTER_PERI,
	MASTER_GPU,
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

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
#define SET_ACCESS_PERMISSON(d7, d6, d5, d4, d3, d2, d1, d0) \
((((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0)) |  \
((((d7) << 9) | ((d6) << 6) | ((d5) << 3) | (d4)) << 16))
#else /* MT6735M */
#define SET_ACCESS_PERMISSON(d3, d2, d1, d0) \
(((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))
#endif

extern int emi_mpu_set_region_protection(unsigned int start_addr,
unsigned int end_addr, int region, unsigned int access_permission);
extern int emi_mpu_notifier_register(int master, emi_mpu_notifier notifider);
extern void acquire_dram_setting(struct basic_dram_setting *pasrdpd);
extern void emi_wp_get_status(void);
extern void mt_emi_reg_write(unsigned int data, unsigned int offset);
extern unsigned int mt_emi_reg_read(unsigned int offset);
extern void mt_emi_reg_base_set(void *base);
extern void *mt_emi_reg_base_get(void);
extern phys_addr_t get_max_DRAM_size(void);
extern void __iomem *EMI_BASE_ADDR;
#endif  /* !__MT_EMI_MPU_H */
