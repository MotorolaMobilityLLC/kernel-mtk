/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef _MT_EEM_
#define _MT_EEM_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <mt-plat/sync_write.h>
#endif

#define EN_EEM_OD (1) /* enable/disable EEM-OD (SW) */
/* #define EEM_DVT_TEST */

/**
 * 1: Select VCORE_AO ptpod detector
 * 0: Select VCORE_PDN ptpod detector
 */

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
	EEM_CTRL_2L = 0,
	EEM_CTRL_L = 1,
	EEM_CTRL_CCI = 2,
	EEM_CTRL_GPU = 3,
	NR_EEM_CTRL,
};

enum eem_det_id {
	EEM_DET_2L	= EEM_CTRL_2L,
	EEM_DET_L = EEM_CTRL_L,
	EEM_DET_CCI	= EEM_CTRL_CCI,
	EEM_DET_GPU	= EEM_CTRL_GPU,
	NR_EEM_DET, /* 3 */
};

enum eem_vcore_id {
	VCORE_VOLT_0,
	VCORE_VOLT_1,
	VCORE_VOLT_2
};

enum mt_eem_cpu_id {
	MT_EEM_CPU_2L,
	MT_EEM_CPU_L,
	NR_MT_EEM_CPU
};


/* Global variable for SW EFUSE*/
/* TODO: FIXME #include "devinfo.h" */
extern u32 get_devinfo_with_index(u32 index);

#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_EEM_AEE_RR_REC 1
#endif

#ifdef CONFIG_EEM_AEE_RR_REC
enum eem_state {
	EEM_CPU_2_LITTLE_IS_SET_VOLT = 0, /* 2L */
	EEM_CPU_LITTLE_IS_SET_VOLT,		/* B */
	EEM_CCI_IS_SET_VOLT,			/* CCI */
	EEM_GPU_IS_SET_VOLT,            /* GPU */
};

extern void aee_rr_rec_ptp_60(u32 val);
extern void aee_rr_rec_ptp_64(u32 val);
extern void aee_rr_rec_ptp_68(u32 val);
extern void aee_rr_rec_ptp_6C(u32 val);
extern void aee_rr_rec_ptp_78(u32 val);
extern void aee_rr_rec_ptp_7C(u32 val);
extern void aee_rr_rec_ptp_80(u32 val);
extern void aee_rr_rec_ptp_84(u32 val);
extern void aee_rr_rec_ptp_88(u32 val);
extern void aee_rr_rec_ptp_8C(u32 val);
extern void aee_rr_rec_ptp_9C(u32 val);
extern void aee_rr_rec_ptp_A0(u32 val);
extern void aee_rr_rec_ptp_vboot(u64 val);
extern void aee_rr_rec_ptp_cpu_big_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_big_volt_1(u64 val);
extern void aee_rr_rec_ptp_cpu_big_volt_2(u64 val);
extern void aee_rr_rec_ptp_cpu_big_volt_3(u64 val);
extern void aee_rr_rec_ptp_gpu_volt(u64 val);
extern void aee_rr_rec_ptp_gpu_volt_1(u64 val);
extern void aee_rr_rec_ptp_gpu_volt_2(u64 val);
extern void aee_rr_rec_ptp_gpu_volt_3(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt_1(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt_2(u64 val);
extern void aee_rr_rec_ptp_cpu_little_volt_3(u64 val);
extern void aee_rr_rec_ptp_cpu_2_little_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_2_little_volt_1(u64 val);
extern void aee_rr_rec_ptp_cpu_2_little_volt_2(u64 val);
extern void aee_rr_rec_ptp_cpu_2_little_volt_3(u64 val);
extern void aee_rr_rec_ptp_cpu_cci_volt(u64 val);
extern void aee_rr_rec_ptp_cpu_cci_volt_1(u64 val);
extern void aee_rr_rec_ptp_cpu_cci_volt_2(u64 val);
extern void aee_rr_rec_ptp_cpu_cci_volt_3(u64 val);
extern void aee_rr_rec_ptp_temp(u64 val);
extern void aee_rr_rec_ptp_status(u8 val);
extern void aee_rr_rec_eem_pi_offset(u8 val);

extern void aee_rr_rec_ptp_e0(u32 val);
extern void aee_rr_rec_ptp_e1(u32 val);
extern void aee_rr_rec_ptp_e2(u32 val);
extern void aee_rr_rec_ptp_e3(u32 val);
extern void aee_rr_rec_ptp_e4(u32 val);
extern void aee_rr_rec_ptp_e5(u32 val);
extern void aee_rr_rec_ptp_e6(u32 val);
extern void aee_rr_rec_ptp_e7(u32 val);
extern void aee_rr_rec_ptp_e8(u32 val);
extern u64 aee_rr_curr_ptp_vboot(void);
extern u64 aee_rr_curr_ptp_cpu_big_volt(void);
extern u64 aee_rr_curr_ptp_cpu_big_volt_1(void);
extern u64 aee_rr_curr_ptp_cpu_big_volt_2(void);
extern u64 aee_rr_curr_ptp_cpu_big_volt_3(void);
extern u64 aee_rr_curr_ptp_gpu_volt(void);
extern u64 aee_rr_curr_ptp_gpu_volt_1(void);
extern u64 aee_rr_curr_ptp_gpu_volt_2(void);
extern u64 aee_rr_curr_ptp_gpu_volt_3(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt_1(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt_2(void);
extern u64 aee_rr_curr_ptp_cpu_little_volt_3(void);
extern u64 aee_rr_curr_ptp_cpu_2_little_volt(void);
extern u64 aee_rr_curr_ptp_cpu_2_little_volt_1(void);
extern u64 aee_rr_curr_ptp_cpu_2_little_volt_2(void);
extern u64 aee_rr_curr_ptp_cpu_2_little_volt_3(void);
extern u64 aee_rr_curr_ptp_cpu_cci_volt(void);
extern u64 aee_rr_curr_ptp_cpu_cci_volt_1(void);
extern u64 aee_rr_curr_ptp_cpu_cci_volt_2(void);
extern u64 aee_rr_curr_ptp_cpu_cci_volt_3(void);
extern u64 aee_rr_curr_ptp_temp(void);
extern u8 aee_rr_curr_ptp_status(void);
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
extern int pmic_force_vgpu_pwm(bool enable);
#ifdef EEM_DVT_TEST
extern void otp_fake_temp_test(void);
#endif

#if defined(__MTK_SLT_)
/* extern int mt_ptp_idle_can_enter(void); */
extern unsigned int ptp_init01_ptp(int id);
extern int ptp_isr(void);
#endif


#endif
