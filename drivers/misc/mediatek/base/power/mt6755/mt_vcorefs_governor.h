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
	pr_err(VCPREFS_TAG"[CRTT]"fmt, ##args)
#define vcorefs_err(fmt, args...)	\
	pr_err(VCPREFS_TAG"[ERR]"fmt, ##args)
#define vcorefs_warn(fmt, args...)	\
	pr_warn(VCPREFS_TAG"[WARN]"fmt, ##args)
#define vcorefs_info(fmt, args...)	\
	pr_warn(VCPREFS_TAG""fmt, ##args)
#define vcorefs_debug(fmt, args...)	\
	pr_debug(VCPREFS_TAG""fmt, ##args)

#define DBG_MSG_ENABLE (1U << 31)
#define vcorefs_debug_mask(type, fmt, args...)	\
	do {							\
		if (vcorefs_log_mask & DBG_MSG_ENABLE)		\
			vcorefs_info(fmt, ##args);		\
		else if (vcorefs_log_mask & (1U << type))	\
			vcorefs_info(fmt, ##args);		\
	} while (0)

struct kicker_config {
	int kicker;
	int opp;
	int dvfs_opp;
};


#define VCORE_1_P_00_UV		1000000
#define VCORE_0_P_90_UV		900000

#define FDDR_S0_KHZ		1866000
#define FDDR_S1_KHZ		1333000

/* Vcore 1.0 <=> trans1 <=> trans2 <=> Vcore 0.9 (SPM control) */
enum vcore_trans {
	TRANS1,
	TRANS2,
	NUM_TRANS
};

enum dvfs_kicker {
	KIR_MM_16MCAM,
	KIR_MM_WFD,
	KIR_MM_MHL,
	KIR_OVL,
	KIR_SDIO,
	KIR_PERF,
	KIR_SYSFS,
	KIR_SYSFS_N,
	KIR_GPU,
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
	MD_Paging,
	MD_Position,
	MD_Cell_Search,
	MD_Cell_Manage,
	MD_DISABLE_SCREEN_CHANGE = 16,
};

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
};

extern unsigned int vcorefs_log_mask;

extern int kicker_table[LAST_KICKER];

extern void __iomem *vcorefs_sram_base;

#define VCOREFS_SRAM_BASE		vcorefs_sram_base	/* map 0x0011cf80 */
/* SDIO parameters */
#define VCOREFS_SRAM_AUTOK_REMARK       (VCOREFS_SRAM_BASE)
#define VCOREFS_SRAM_SDIO_HPM_PARA      (VCOREFS_SRAM_BASE + 0x04)
#define VCOREFS_SRAM_SDIO_LPM_PARA      (VCOREFS_SRAM_BASE + 0x2C)
/* VcoreFS debug */
#define VCOREFS_SRAM_DVS_UP_COUNT		(VCOREFS_SRAM_BASE + 0x54)
#define VCOREFS_SRAM_DFS_UP_COUNT		(VCOREFS_SRAM_BASE + 0x58)
#define VCOREFS_SRAM_DVS_DOWN_COUNT		(VCOREFS_SRAM_BASE + 0x5c)
#define VCOREFS_SRAM_DFS_DOWN_COUNT		(VCOREFS_SRAM_BASE + 0x60)
#define VCOREFS_SRAM_DVFS_UP_LATENCY	(VCOREFS_SRAM_BASE + 0x64)
#define VCOREFS_SRAM_DVFS_DOWN_LATENCY	(VCOREFS_SRAM_BASE + 0x68)
#define VCOREFS_SRAM_DVFS_LATENCY_SPEC  (VCOREFS_SRAM_BASE + 0x6c)
#define VCOREFS_SRAM_DRAM_GATING_CHECK  (VCOREFS_SRAM_BASE + 0x70)

#define VALID_AUTOK_REMARK_VAL 0x55AA55AA

/* 1T@32K = 30.5us, 1ms is about 32 T */
#define DVFS_LATENCY_MAX 32	/* about 1 msc */



/*
 * User API
 */
extern void vcorefs_set_cpu_dvfs_req(u32 value);

/*
 * Framework API
 */
extern int vcorefs_late_init_dvfs(void);
extern int kick_dvfs_by_opp_index(struct kicker_config *krconf);
extern bool is_vcorefs_feature_enable(void);
extern int vcorefs_get_num_opp(void);
extern int vcorefs_get_curr_vcore(void);
extern int vcorefs_get_curr_ddr(void);
extern int vcorefs_get_vcore_by_steps(u32);
extern int vcorefs_get_ddr_by_steps(unsigned int steps);
extern char *vcorefs_get_dvfs_info(char *p);
extern char *vcorefs_get_opp_table_info(char *p);
extern void vcorefs_update_opp_table(char *cmd, int val);
extern int vcorefs_output_kicker_id(char *name);

/* Manager extern API */
extern int governor_debug_store(const char *);

extern int vcorefs_enable_dvs(bool enable);
extern int vcorefs_enable_dfs(bool enable);
extern int vcorefs_enable_debug_isr(bool enable);

extern int get_kicker_group_opp(int kicker, int group_id);
extern char *get_kicker_name(int id);

/* EMIBW API */
extern int vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);
extern int vcorefs_set_total_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);
extern int vcorefs_enable_perform_bw(bool enable);
extern int vcorefs_enable_total_bw(bool enable);

/* screen size */
extern int primary_display_get_width(void);
extern int primary_display_get_height(void);
extern int primary_display_get_virtual_width(void);
extern int primary_display_get_virtual_height(void);

/* AutoK related API */
extern void governor_autok_manager(void);
extern bool governor_autok_check(int kicker, int opp);
extern bool governor_autok_lock_check(int kicker, int opp);

extern void vcorefs_set_sram_data(int index, u32 data);
extern u32 vcorefs_get_sram_data(int index);

extern void aee_rr_rec_vcore_dvfs_opp(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_opp(void);
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);

/* GPU kicker init opp API */
extern int vcorefs_gpu_get_init_opp(void);
extern void  vcorefs_gpu_set_init_opp(int opp);
extern bool vcorefs_request_init_opp(int kicker, int opp);

#endif				/* _MT_VCOREFS_GOVERNOR_H */
