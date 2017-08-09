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

/* Uses for DVFS Request */
#define vcorefs_crit_mask(log_mask, kicker, fmt, args...)	\
do {								\
	if (log_mask & (1U << kicker))				\
		;						\
	else							\
		vcorefs_crit(fmt, ##args);			\
} while (0)

struct kicker_config {
	int kicker;
	int opp;
	int dvfs_opp;
};

#define VCORE_0_P_80_UV		800000
#define VCORE_0_P_70_UV		700000

#define FDDR_S0_KHZ		3200000
#define FDDR_S1_KHZ		2667000
#define FDDR_S2_KHZ		1600000


enum dvfs_kicker {
	KIR_MM = 0,
	KIR_MM_NON_FORCE,
	KIR_AUDIO,
	KIR_PERF,
	KIR_SYSFS,
	KIR_SYSFS_N,
	NUM_KICKER,

	/* internal kicker */
	KIR_LATE_INIT,
	KIR_SYSFSX,
	KIR_AUTOK_EMMC,
	KIR_AUTOK_SDIO,
	KIR_AUTOK_SD,
	LAST_KICKER,
};

extern int kicker_table[LAST_KICKER];

enum dvfs_opp {
	OPP_OFF = -1,
	OPP_0 = 0,	/* HPM */
	OPP_1,		/* LPM */
	OPP_2,		/* ULPM */
	NUM_OPP,
};

#define OPPI_PERF OPP_0
#define OPPI_LOW_PWR OPP_1
#define OPPI_ULTRA_LOW_PWR OPP_2
#define OPPI_UNREQ OPP_OFF

enum dvfs_kicker_group {
	KIR_GROUP_HPM = 0,
	KIR_GROUP_HPM_NON_FORCE,
	KIR_GROUP_FIX,
	NUM_KIR_GROUP,
};

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
	int axi_khz;
};

#define VCOREFS_SRAM_BASE		vcorefs_sram_base	/* map 0x0011cf80 */
/* VcoreFS debug */
#define VCOREFS_SRAM_DVFS_UP_COUNT		(VCOREFS_SRAM_BASE + 0x54)
#define VCOREFS_SRAM_DVFS_DOWN_COUNT		(VCOREFS_SRAM_BASE + 0x58)
#define VCOREFS_SRAM_DVFS2_UP_COUNT		(VCOREFS_SRAM_BASE + 0x5c)
#define VCOREFS_SRAM_DVFS2_DOWN_COUNT		(VCOREFS_SRAM_BASE + 0x60)
#define VCOREFS_SRAM_DVFS_UP_TIME	(VCOREFS_SRAM_BASE + 0x64)
#define VCOREFS_SRAM_DVFS_DOWN_TIME	(VCOREFS_SRAM_BASE + 0x68)
#define VCOREFS_SRAM_DVFS2_UP_TIME	(VCOREFS_SRAM_BASE + 0x6c)
#define VCOREFS_SRAM_DVFS2_DOWN_TIME	(VCOREFS_SRAM_BASE + 0x70)
#define VCOREFS_SRAM_EMI_BLOCK_TIME	(VCOREFS_SRAM_BASE + 0x74)

/* Governor extern API */
extern bool is_vcorefs_feature_enable(void);
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
extern int vcorefs_get_dvfs_kicker_group(int kicker);
extern int governor_debug_store(const char *);
extern int vcorefs_late_init_dvfs(void);
extern int kick_dvfs_by_opp_index(struct kicker_config *krconf);
extern char *governor_get_dvfs_info(char *p);
void vcorefs_go_to_vcore_dvfs(void);
bool vcorefs_sodi_rekick_lock(void);

/* AXI API */
extern unsigned int ckgen_meter(int ID);

/* EMIBW API */
extern int vcorefs_enable_perform_bw(bool enable);
extern int vcorefs_set_perform_bw_threshold_v2(u32 ulpm_threshold, u32 lpm_threshold, u32 hpm_threshold);
extern int vcorefs_enable_total_bw(bool enable);
extern int vcorefs_set_total_bw_threshold_v2(u32 ulpm_threshold, u32 lpm_threshold, u32 hpm_threshold);

/* AutoK related API */
extern void governor_autok_manager(void);
extern bool governor_autok_check(int kicker);
extern bool governor_autok_lock_check(int kicker, int opp);


#endif	/* _MT_VCOREFS_GOVERNOR_H */
