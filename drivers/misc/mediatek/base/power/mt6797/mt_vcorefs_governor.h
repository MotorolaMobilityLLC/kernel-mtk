#ifndef _MT_VCOREFS_GOVERNOR_H
#define _MT_VCOREFS_GOVERNOR_H

#undef VCPREFS_TAG
#define VCPREFS_TAG "[VcoreFS]"

#define vcorefs_crit(fmt, args...)	\
	pr_crit(VCPREFS_TAG""fmt, ##args)
#define vcorefs_err(fmt, args...)	\
	pr_err(VCPREFS_TAG""fmt, ##args)
#define vcorefs_warn(fmt, args...)	\
	pr_warn(VCPREFS_TAG""fmt, ##args)
#define vcorefs_debug(fmt, args...)	\
	pr_debug(VCPREFS_TAG""fmt, ##args)

/* log_mask[15:0]: show nothing, log_mask[16:31]: show only on MobileLog */
#define vcorefs_crit_mask(fmt, args...)				\
do {								\
	if (pwrctrl->log_mask & (1U << kicker))			\
		;						\
	else if ((pwrctrl->log_mask >> 16) & (1U << kicker))	\
		vcorefs_debug(fmt, ##args);			\
	else							\
		vcorefs_crit(fmt, ##args);			\
} while (0)

struct kicker_config {
	int kicker;
	int opp;
	int dvfs_opp;
};

#define VCORE_1_P_00_UV		1000000
#define VCORE_0_P_90_UV		900000

#define FDDR_S0_KHZ		1600000
#define FDDR_S1_KHZ		1270000

#define FAXI_S0_KHZ		158000
#define FAXI_S1_KHZ		138000

/* Vcore 1.0 <=> trans1 <=> trans2 <=> Vcore 0.9 (SPM control) */
enum vcore_trans {
	TRANS1,
	TRANS2,
	NUM_TRANS
};

enum dvfs_kicker {
	KIR_MM = 0,
	KIR_SYSFS,
	KIR_MAX,
	NUM_KICKER,

	/* internal kicker */
	KIR_LATE_INIT,
	KIR_SYSFSX,
	KIR_AUTOK_EMMC,
	KIR_AUTOK_SDIO,
	KIR_AUTOK_SD,
	LAST_KICKER,
};

#define KIR_AUTOK KIR_AUTOK_SDIO

enum dvfs_opp {
	OPP_OFF = -1,
	OPP_0 = 0,
	OPP_1,
	NUM_OPP
};

#define OPPI_PERF OPP_0
#define OPPI_LOW_PWR OPP_1
#define OPPI_UNREQ OPP_OFF

enum md_status {
	MD_CAT6_CA_DATALINK = 0,
	MD_NON_CA_DATALINK,
	MD_PAGING,
	MD_POSITION,
	MD_CELL_SEARCH,
	MD_CCELL_MANAGEMENT,
	MD_DISABLE_SCREEN_CHANGE = 16,
};

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
	int axi_khz;
};

extern int kicker_table[NUM_KICKER];

extern void __iomem *vcorefs_sram_base;

#define VCOREFS_SRAM_BASE		vcorefs_sram_base	/* map 0x0011cf80 */

#define VCOREFS_SRAM_AUTOK_REMARK       (VCOREFS_SRAM_BASE)
#define VCOREFS_SRAM_SDIO_HPM_PARA      (VCOREFS_SRAM_BASE + 0x04)
#define VCOREFS_SRAM_SDIO_LPM_PARA      (VCOREFS_SRAM_BASE + 0x2C)

#define VCOREFS_SRAM_DVS_UP_COUNT	(VCOREFS_SRAM_BASE + 0x54)
#define VCOREFS_SRAM_DFS_UP_COUNT	(VCOREFS_SRAM_BASE + 0x58)
#define VCOREFS_SRAM_DVS_DOWN_COUNT	(VCOREFS_SRAM_BASE + 0x5c)
#define VCOREFS_SRAM_DFS_DOWN_COUNT	(VCOREFS_SRAM_BASE + 0x60)
#define VCOREFS_SRAM_DVFS_UP_LATENCY	(VCOREFS_SRAM_BASE + 0x64)
#define VCOREFS_SRAM_DVFS_DOWN_LATENCY	(VCOREFS_SRAM_BASE + 0x68)
#define VCOREFS_SRAM_DVFS_LATENCY_SPEC  (VCOREFS_SRAM_BASE + 0x6c)

/* 1T@32K = 30.5us, 1ms is about 32 T */
#define DVFS_LATENCY_MAX 32	/* about 1 msc */

/* Governor extern API */
extern bool is_vcorefs_feature_enable(void);
extern int vcorefs_get_num_opp(void);
extern int vcorefs_get_curr_opp(void);
extern int vcorefs_get_prev_opp(void);
extern int vcorefs_get_curr_vcore(void);
extern int vcorefs_get_curr_ddr(void);
extern int vcorefs_get_vcore_by_steps(u32);
extern int vcorefs_get_ddr_by_steps(unsigned int steps);
extern void vcorefs_update_opp_table(char *cmd, int val);
extern char *governor_get_kicker_name(int id);
extern char *vcorefs_get_opp_table_info(char *p);
extern int vcorefs_output_kicker_id(char *name);
extern int governor_debug_store(const char *);
extern int vcorefs_late_init_dvfs(void);
extern int kick_dvfs_by_opp_index(struct kicker_config *krconf);
extern char *governor_get_dvfs_info(char *p);

/* EMIBW API */
extern int vcorefs_enable_perform_bw(bool enable);
extern int vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);

/* AutoK related API */
extern void governor_autok_manager(void);
extern bool governor_autok_check(int kicker);
extern bool governor_autok_lock_check(int kicker, int opp);

/* sram debug info */
extern int vcorefs_enable_debug_isr(bool enable);

#endif	/* _MT_VCOREFS_GOVERNOR_H */
