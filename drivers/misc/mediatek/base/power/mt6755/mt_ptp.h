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

#ifndef _MT_PTP_
#define _MT_PTP_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <mt-plat/sync_write.h>
#endif

#define EN_EEM_OD (1) /* enable/disable EEM-OD (SW) */

/**
 * 1: Select VCORE_AO ptpod detector
 * 0: Select VCORE_PDN ptpod detector
 */

/*
#define PERI_VCORE_EEM_CON0	(PERICFG_BASE + 0x408)
#define VCORE_PTPODSEL		0:0
#define SEL_VCORE_AO		1
#define SEL_VCORE_PDN		0
*/

/* Thermal Register Definition */

/* EEM Structure */
typedef struct {
	unsigned int ADC_CALI_EN;
	unsigned int PTPINITEN;
	unsigned int PTPMONEN;

	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCCONFIG;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int AGEDELTA;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int MTDES;
	unsigned int MTS;
	unsigned int BTS;

	unsigned char FREQPCT0;
	unsigned char FREQPCT1;
	unsigned char FREQPCT2;
	unsigned char FREQPCT3;
	unsigned char FREQPCT4;
	unsigned char FREQPCT5;
	unsigned char FREQPCT6;
	unsigned char FREQPCT7;

	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;

	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;
} PTP_INIT_T;


enum eem_ctrl_id {
	EEM_CTRL_LITTLE = 0,
	EEM_CTRL_GPU = 1,
	EEM_CTRL_BIG = 2,
	/* EEM_CTRL_SOC	= 3, */
	NR_EEM_CTRL,
};

enum eem_det_id {
	EEM_DET_LITTLE	= EEM_CTRL_LITTLE,
	EEM_DET_GPU	= EEM_CTRL_GPU,
	EEM_DET_BIG	= EEM_CTRL_BIG,
	/* EEM_DET_SOC		= EEM_CTRL_SOC, */
	NR_EEM_DET, /* 3 */
};

enum mt_eem_cpu_id {
	MT_EEM_CPU_LITTLE,
	MT_EEM_CPU_BIG,

	NR_MT_EEM_CPU,
};


/* Global variable for SW EFUSE*/
/* TODO: FIXME #include "devinfo.h" */
extern u32 get_devinfo_with_index(u32 index);

#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_EEM_AEE_RR_REC 1
#endif

#ifdef CONFIG_EEM_AEE_RR_REC
enum eem_state {
	EEM_CPU_LITTLE_IS_SET_VOLT = 0, /* L */
	EEM_GPU_IS_SET_VOLT,            /* G */
	EEM_CPU_BIG_IS_SET_VOLT,        /* B */
};

extern void aee_rr_rec_ptp_cpu_little_volt(u64 val);
extern void aee_rr_rec_ptp_gpu_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_big_volt(u64 val);
extern void aee_rr_rec_ptp_temp(u64 val);
extern void aee_rr_rec_ptp_status(u8 val);
extern void aee_rr_rec_eem_pi_offset(u8 val);

extern u64 aee_rr_curr_ptp_cpu_little_volt(void);
extern u64 aee_rr_curr_ptp_gpu_volt(void);
extern u64 aee_rr_curr_ptp_cpu_big_volt(void);
extern u64 aee_rr_curr_ptp_temp(void);
extern u8 aee_rr_curr_ptp_status(void);
extern u8 aee_rr_curr_eem_pi_offset(void);
#endif


/* EEM Extern Function */
/* extern unsigned int mt_eem_get_level(void); */
extern void mt_ptp_lock(unsigned long *flags);
extern void mt_ptp_unlock(unsigned long *flags);
extern void eem_init02(const char *str);
extern int mt_eem_status(enum eem_det_id id);
extern int get_ptpod_status(void);
extern int is_have_550(void);
extern unsigned int get_vcore_ptp_volt(int uv);
extern void eem_set_pi_offset(enum eem_ctrl_id id, int step);
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
extern unsigned int get_turbo_status(void);
#endif

#if defined(__MTK_SLT_)
/* extern int mt_ptp_idle_can_enter(void); */
extern void ptp_init01_ptp(int id);
extern int ptp_isr(void);
#endif


#endif
