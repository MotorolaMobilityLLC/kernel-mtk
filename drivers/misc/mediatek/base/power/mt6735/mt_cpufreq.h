/**
 * @file mt_cpufreq.h
 * @brief CPU DVFS driver interface
 */

#ifndef __MT_CPUFREQ_H__
#define __MT_CPUFREQ_H__

#ifdef __cplusplus
extern "C" {
#endif

/* configs */
#ifdef CONFIG_ARCH_MT6753
#ifndef __KERNEL__
#include "mt6311.h"
#else
#include "../../../power/mt6735/mt6311.h"
#endif
#define CONFIG_CPU_DVFS_HAS_EXTBUCK		1	/* external PMIC related access */
#endif

#if 0
#define CONFIG_CPU_DVFS_PERFORMANCE_TEST     1       /* fix at max freq for perf test */
#define CONFIG_CPU_DVFS_FFTT_TEST            1       /* FF TT SS volt test */
#endif

#ifdef __KERNEL__
#if 0
#define CONFIG_CPU_DVFS_TURBO_MODE           1       /* turbo max freq when CPU core <= 2*/
#endif
#define CONFIG_CPU_DVFS_POWER_THROTTLING	1	/* power throttling features */
#endif

#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_CPU_DVFS_AEE_RR_REC		1	/* AEE SRAM debugging */
#endif

/*=============================================================*/
/* Include files */
/*=============================================================*/

/* system includes */

/* project includes */

/* local includes */

/* forward references */
extern void __iomem *pwrap_base;
#define PMIC_WRAP_BASE		(pwrap_base)

#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
/* #include "mach/mt_typedefs.h" */

extern int is_ext_buck_sw_ready(void);
extern int is_ext_buck_exist(void);
extern void mt6311_set_vdvfs11_vosel(unsigned char val);
extern void mt6311_set_vdvfs11_vosel_on(unsigned char val);
extern void mt6311_set_vdvfs11_vosel_ctrl(unsigned char val);
extern unsigned int mt6311_read_byte(unsigned char cmd, unsigned char *returnData);
extern void mt6311_set_buck_test_mode(unsigned char val);
extern unsigned int mt6311_get_chip_id(void);
#endif

extern u32 get_devinfo_with_index(u32 index);

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
extern void aee_rr_rec_cpu_dvfs_vproc_big(u8 val);
extern void aee_rr_rec_cpu_dvfs_vproc_little(u8 val);
extern void aee_rr_rec_cpu_dvfs_oppidx(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_oppidx(void);
extern void aee_rr_rec_cpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_status(void);
#endif


/*=============================================================*/
/* Macro definition */
/*=============================================================*/

/*=============================================================*/
/* Type definition */
/*=============================================================*/

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LITTLE,
	NR_MT_CPU_DVFS,
};

enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,	/* i.e. reg setting */
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_MAINPLL = 2,

	NR_TOP_CKMUXSEL,
};

/*
 * PMIC_WRAP
 */

/* Phase */
enum pmic_wrap_phase_id {
	PMIC_WRAP_PHASE_NORMAL,
	PMIC_WRAP_PHASE_SUSPEND,
	PMIC_WRAP_PHASE_DEEPIDLE,

	NR_PMIC_WRAP_PHASE,
};

/* IDX mapping */
#ifdef CONFIG_ARCH_MT6735M
enum {
	IDX_NM_VCORE_TRANS4,		/* 0 *//* PMIC_WRAP_PHASE_NORMAL */
	IDX_NM_VCORE_TRANS3,		/* 1 */
	IDX_NM_VCORE_HPM,		/* 2 */
	IDX_NM_VCORE_TRANS2,		/* 3 */
	IDX_NM_VCORE_TRANS1,		/* 4 */
	IDX_NM_VCORE_LPM,		/* 5 */
	IDX_NM_VCORE_UHPM,		/* 6 */
	IDX_NM_VRF18_0_PWR_ON,		/* 7 */
	IDX_NM_NOT_USED,		/* 8 */
	IDX_NM_VRF18_0_SHUTDOWN,	/* 9 */

	NR_IDX_NM,
};
#else	/* !CONFIG_ARCH_MT6735M */
enum {
	IDX_NM_NOT_USED1,		/* 0 *//* PMIC_WRAP_PHASE_NORMAL */
	IDX_NM_NOT_USED2,		/* 1 */
	IDX_NM_VCORE_HPM,		/* 2 */
	IDX_NM_VCORE_TRANS2,		/* 3 */
	IDX_NM_VCORE_TRANS1,		/* 4 */
	IDX_NM_VCORE_LPM,		/* 5 */
	IDX_NM_VRF18_0_SHUTDOWN,	/* 6 */
	IDX_NM_VRF18_0_PWR_ON,		/* 7 */

	NR_IDX_NM,
};
#endif

enum {
	IDX_SP_VPROC_PWR_ON,		/* 0 *//* PMIC_WRAP_PHASE_SUSPEND */
	IDX_SP_VPROC_SHUTDOWN,		/* 1 */
	IDX_SP_VCORE_HPM,		/* 2 */
	IDX_SP_VCORE_TRANS2,		/* 3 */
	IDX_SP_VCORE_TRANS1,		/* 4 */
	IDX_SP_VCORE_LPM,		/* 5 */
	IDX_SP_VSRAM_SHUTDOWN,		/* 6 */
	IDX_SP_VRF18_0_PWR_ON,		/* 7 */
	IDX_SP_VSRAM_PWR_ON,		/* 8 */
	IDX_SP_VRF18_0_SHUTDOWN,	/* 9 */

	NR_IDX_SP,
};
enum {
	IDX_DI_VPROC_NORMAL,		/* 0 *//* PMIC_WRAP_PHASE_DEEPIDLE */
	IDX_DI_VPROC_SLEEP,		/* 1 */
	IDX_DI_VCORE_HPM,		/* 2 */
	IDX_DI_VCORE_TRANS2,		/* 3 */
	IDX_DI_VCORE_TRANS1,		/* 4 */
	IDX_DI_VCORE_LPM,		/* 5 */
	IDX_DI_VSRAM_SLEEP,		/* 6 */
	IDX_DI_VRF18_0_PWR_ON,		/* 7 */
	IDX_DI_VSRAM_NORMAL,		/* 8 */
	IDX_DI_VRF18_0_SHUTDOWN,	/* 9 */
#ifdef CONFIG_ARCH_MT6753
	IDX_DI_VCORE_IDLE_LPM,		/* 10 */
	IDX_DI_VSRAM_SLEEP_FOR_TURBO,	/* 11 */
	IDX_DI_VSRAM_NORMAL_FOR_TURBO,	/* 12 */
#endif

	NR_IDX_DI,
};


typedef void (*cpuVoltsampler_func) (enum mt_cpu_dvfs_id, unsigned int mv);
/*=============================================================*/
/* Global variable definition */
/*=============================================================*/


/*=============================================================*/
/* Global function definition */
/*=============================================================*/

/* PMIC WRAP */
extern void mt_cpufreq_set_pmic_phase(enum pmic_wrap_phase_id phase);
extern void mt_cpufreq_set_pmic_cmd(enum pmic_wrap_phase_id phase, int idx,
						    unsigned int cmd_wdata);
extern void mt_cpufreq_apply_pmic_cmd(int idx);

/* Dormant profiling */
extern unsigned int mt_cpufreq_get_cur_freq(enum mt_cpu_dvfs_id id);

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
						  int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_enable_by_ptpod(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_disable_by_ptpod(enum mt_cpu_dvfs_id id);
extern int mt_cpufreq_set_lte_volt(int pmic_val);

/* Thermal */
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);

/* PBM */
extern void mt_cpufreq_set_power_limit_by_pbm(unsigned int limited_power);
extern unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id);

/* for perfService kernel module */
extern void mt_cpufreq_set_min_freq(enum mt_cpu_dvfs_id id, unsigned int freq);
extern void mt_cpufreq_set_max_freq(enum mt_cpu_dvfs_id id, unsigned int freq);

/* Generic */
/* extern int mt_cpufreq_state_set(int enabled); */
extern int mt_cpufreq_set_cpu_clk_src(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_cpu_clk_src(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
extern bool mt_cpufreq_earlysuspend_status_get(void);

#ifdef CONFIG_CPU_FREQ_GOV_HOTPLUG
extern void mt_cpufreq_set_ramp_down_count_const(enum mt_cpu_dvfs_id id, int count);
#endif

#ifndef __KERNEL__
extern int mt_cpufreq_pdrv_probe(void);
extern int mt_cpufreq_set_opp_volt(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_set_freq(enum mt_cpu_dvfs_id id, int idx);
extern unsigned int dvfs_get_cpu_freq(enum mt_cpu_dvfs_id id);
extern void dvfs_set_cpu_freq_FH(enum mt_cpu_dvfs_id id, int freq);
extern unsigned int dvfs_get_cur_oppidx(enum mt_cpu_dvfs_id id);
extern unsigned int cpu_frequency_output_slt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_get_cur_volt_lte(void);
extern unsigned int dvfs_get_cpu_volt(enum mt_cpu_dvfs_id id);
extern void dvfs_set_cpu_volt(enum mt_cpu_dvfs_id id, int volt);
extern void dvfs_set_gpu_volt(int pmic_val);
extern void dvfs_set_vcore_ao_volt(int pmic_val);
/* extern void dvfs_set_vcore_pdn_volt(int pmic_val); */
extern void dvfs_disable_by_ptpod(int id);
extern void dvfs_enable_by_ptpod(int id);
#endif	/* ! __KERNEL__ */


#ifdef __cplusplus
}
#endif
#endif	/* __MT_CPUFREQ_H__ */
