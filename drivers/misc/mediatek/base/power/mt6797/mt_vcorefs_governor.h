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
#define FDDR_S2_KHZ		800000

#define FAXI_S0_KHZ		156000
#define FAXI_S1_KHZ		136500

enum vcore_trans {
	TRANS1,
	TRANS2,
	TRANS3,
	TRANS4,
	TRANS5,
	TRANS6,
	TRANS7,
	NUM_TRANS,
};

enum dvfs_kicker {
	KIR_MM = 0,
	KIR_PERF,
	KIR_REESPI,	/* From KERNEL */
	KIR_TEESPI,	/* From SPI1 */
	KIR_SCP,	/* For debug */
	KIR_SYSFS,
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
	OPP_0 = 0,	/* 1.0/1600/1700 or 1.05/1866 */
	OPP_1,		/* 0.9/1270 */
	OPP_2,		/* 0.9/800 */
	NUM_OPP,
};

#define OPPI_PERF OPP_0
#define OPPI_LOW_PWR OPP_1
#define OPPI_ULTRA_LOW_PWR OPP_2
#define OPPI_UNREQ OPP_OFF

enum dvfs_timer {
	RANGE_0 = 0,
	RANGE_1 = 50,
	RANGE_2 = 75,
	RANGE_3 = 100,
	RANGE_4 = 125,
	RANGE_5 = 150,
	RANGE_6 = 200,
	NUM_RANGE = 7,
};

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
	int axi_khz;
};

extern int kicker_table[LAST_KICKER];

/* Governor extern API */
extern bool is_vcorefs_feature_enable(void);
extern bool is_vcorefs_dvfs_enable(void);
extern bool vcorefs_get_screen_on_state(void);
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
bool vcorefs_screen_on_lock_dpidle(void);
bool vcorefs_screen_on_lock_suspend(void);
bool vcorefs_screen_on_lock_sodi(void);
void vcorefs_go_to_vcore_dvfs(void);
bool vcorefs_sodi_rekick_lock(void);

/* AXI API */
extern unsigned int ckgen_meter(int ID);

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
