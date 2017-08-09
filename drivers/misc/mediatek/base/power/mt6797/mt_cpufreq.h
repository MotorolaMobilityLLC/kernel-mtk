/*
 * @file mt_cpufreq.h
 * @brief CPU DVFS driver interface
 */

#ifndef __MT_CPUFREQ_H__
#define __MT_CPUFREQ_H__
#include "mt_typedefs.h"
#ifdef __cplusplus
extern "C" {
#endif

#if 1
/* ARMPLL ADDR */
#define BASE_MCUMIXEDSYS 0x1001A000
/* LL */
#define ARMPLLCAXPLL0_CON0	(BASE_MCUMIXEDSYS+0x200)
#define ARMPLLCAXPLL0_CON1	(BASE_MCUMIXEDSYS+0x204)
#define ARMPLLCAXPLL0_CON2	(BASE_MCUMIXEDSYS+0x208)
/* L */
#define ARMPLLCAXPLL1_CON0	(BASE_MCUMIXEDSYS+0x210)
#define ARMPLLCAXPLL1_CON1	(BASE_MCUMIXEDSYS+0x214)
#define ARMPLLCAXPLL1_CON2	(BASE_MCUMIXEDSYS+0x218)
/* CCI */
#define ARMPLLCAXPLL2_CON0	(BASE_MCUMIXEDSYS+0x220)
#define ARMPLLCAXPLL2_CON1	(BASE_MCUMIXEDSYS+0x224)
#define ARMPLLCAXPLL2_CON2	(BASE_MCUMIXEDSYS+0x228)
/* Backup */
#define ARMPLLCAXPLL3_CON0	(BASE_MCUMIXEDSYS+0x230)
#define ARMPLLCAXPLL3_CON1	(BASE_MCUMIXEDSYS+0x234)
#define ARMPLLCAXPLL3_CON2	(BASE_MCUMIXEDSYS+0x238)
/* B */
#define BASE_CA15M_CONFIG 0x10222000
# if 0
#define ARMPLL_CON0		(BASE_CA15M_CONFIG+0x4A0)
#define ARMPLL_CON1		(BASE_CA15M_CONFIG+0x4A4)
#define ARMPLL_CON2		(BASE_CA15M_CONFIG+0x4A8)
#endif
/* 7:4 enable, 3:0 sel*/
#define SRAMLDO			(BASE_CA15M_CONFIG+0x2B0)
/* ARMPLL DIV */
#define ARMPLLDIV_MUXSEL	(BASE_MCUMIXEDSYS+0x270)
#define ARMPLLDIV_CKDIV		(BASE_MCUMIXEDSYS+0x274)
/* L VSRAM */
#define CPULDO_CTRL_BASE	(0x10001000)
#define CPULDO_CTRL_0		(CPULDO_CTRL_BASE+0xF98)
/* 3:0, 4, 11:8, 12, 19:16, 20, 27:24, 28*/
#define CPULDO_CTRL_1		(CPULDO_CTRL_BASE+0xF9C)
/* 3:0, 4, 11:8, 12, 19:16, 20, 27:24, 28*/
#define CPULDO_CTRL_2		(CPULDO_CTRL_BASE+0xFA0)

#endif

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LL,
	MT_CPU_DVFS_L,
	MT_CPU_DVFS_B,
	MT_CPU_DVFS_CCI,

	NR_MT_CPU_DVFS,
};
/* 3 => MAIN */
enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_MAINPLL = 2,
	TOP_CKMUXSEL_UNIVPLL = 3,

	NR_TOP_CKMUXSEL,
};

typedef void (*cpuVoltsampler_func) (enum mt_cpu_dvfs_id, unsigned int mv);

/* PMIC */
extern int is_ext_buck_sw_ready(void);
extern int is_ext_buck_exist(void);
extern void mt6311_set_vdvfs11_vosel(kal_uint8 val);
extern void mt6311_set_vdvfs11_vosel_on(kal_uint8 val);
extern void mt6311_set_vdvfs11_vosel_ctrl(kal_uint8 val);
extern kal_uint32 mt6311_read_byte(kal_uint8 cmd, kal_uint8 *returnData);
extern kal_uint32 mt6311_get_chip_id(void);

extern u32 get_devinfo_with_index(u32 index);
extern void (*cpufreq_freq_check)(enum mt_cpu_dvfs_id id);

/* Freq Meter API */
#ifdef __KERNEL__
extern unsigned int mt_get_cpu_freq(void);
#endif

/* PMIC WRAP */
#ifdef CONFIG_OF
extern void __iomem *pwrap_base;
#define PWRAP_BASE_ADDR     ((unsigned long)pwrap_base)
#endif

/* #ifdef CONFIG_CPU_DVFS_AEE_RR_REC */
#if 1
/* SRAM debugging*/
extern void aee_rr_rec_cpu_dvfs_vproc_big(u8 val);
extern void aee_rr_rec_cpu_dvfs_vproc_little(u8 val);
extern void aee_rr_rec_cpu_dvfs_oppidx(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_oppidx(void);
extern void aee_rr_rec_cpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_status(void);
#endif

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
				  int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_enable_by_ptpod(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_disable_by_ptpod(enum mt_cpu_dvfs_id id);

/* PBM */
extern unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id);

/* PPM */
extern unsigned int mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id);

/* DCM */
#ifdef DCM_ENABLE
extern int sync_dcm_set_cci_freq(unsigned int cci_mhz);
extern int sync_dcm_set_mp0_freq(unsigned int mp0_mhz);
extern int sync_dcm_set_mp1_freq(unsigned int mp1_mhz);
extern int sync_dcm_set_mp2_freq(unsigned int mp2_mhz);
#endif

/* Generic */
extern int mt_cpufreq_state_set(int enabled);
extern int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
extern bool mt_cpufreq_earlysuspend_status_get(void);
extern unsigned int mt_get_bigcpu_freq(void);
extern unsigned int mt_get_cpu_freq(void);

#ifdef __cplusplus
}
#endif
#endif
