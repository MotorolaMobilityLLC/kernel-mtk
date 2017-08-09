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
/*
 * @file    mt_idvfs.c
 * @brief   Driver for CPU iDVFS define
 *
 */
#ifndef _MT_IDVFS_H
#define _MT_IDVFS_H

#ifdef __KERNEL__
#ifdef __MT_IDVFS_C__
#include "mach/mt_secure_api.h"
#endif
#else
/* for ATF function include */
#include "mt_idvfs_api.h"
#endif

/*
#ifdef __MT_IDVFS_C__
	#define IDVFS_EXTERN
#else
	#define IDVFS_EXTERN extern
#endif
*/

#ifdef __KERNEL__
#ifdef __MT_IDVFS_C__
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define mt_secure_call_idvfs	mt_secure_call
#else
/* This is workaround for idvfs use  */
static noinline int mt_secure_call_idvfs(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile ("smc    #0\n" : "+r" (reg0) :
		"r"(reg1), "r"(reg2), "r"(reg3));

	ret = (int)reg0;
	return ret;
}
#endif
#endif
#endif

/* define for iDVFS service */
#ifdef CONFIG_ARM64

#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE				0xC20003B0
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE			0xC20003B1
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSFREQ				0xC20003B2
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSCHANNEL			0xC20003B3
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVG				0xC20003B4
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVGSTATUS		0xC20003B5
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWMODE				0xC20003B6
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSLOWMODE			0xC20003B7
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ			0xC20003B8
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLDISABLE			0xC20003B9	/* add */
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETFREQ			0xC20003BA
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPOSDIV		0xC20003BB
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPOSDIV		0xC20003BC
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPCW			0xC20003BD
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPCW			0xC20003BE
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET			0xC20003BF
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOGET			0xC20003C0
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWREQ			0xC20003C1

#define MTK_SIP_KERNEL_IDVFS_READ						0xC200035F
#define MTK_SIP_KERNEL_IDVFS_WRITE						0xC200035E

#else

#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE				0x820003B0
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE			0x820003B1
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSFREQ				0x820003B2
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSCHANNEL			0x820003B3
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVG				0x820003B4
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVGSTATUS		0x820003B5
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWMODE				0x820003B6
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSLOWMODE			0x820003B7
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ			0x820003B8
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLDISABLE			0x820003B9
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETFREQ			0x820003BA
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPOSDIV		0x820003BB
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPOSDIV		0x820003BC
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPCW			0x820003BD
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPCW			0x820003BE
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET			0x820003BF
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOGET			0x820003C0
#define MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWREQ			0x820003C1

#define MTK_SIP_KERNEL_IDVFS_READ						0x8200035F
#define MTK_SIP_KERNEL_IDVFS_WRITE						0x8200035E
#endif

/* ATF only support */
#ifdef __KERNEL__
#define SEC_BIGIDVFSENABLE(idvfs_ctrl, Vproc, Vsram) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE, idvfs_ctrl, Vproc, Vsram)
#define SEC_BIGIDVFSDISABLE() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE, 0, 0, 0)
#define SEC_BIGIDVFSPLLSETFREQ(Freq) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ, Freq, 0, 0)
#define SEC_BIGIDVFSSRAMLDOSET(mv_x100) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET, mv_x100, 0, 0)
#define SEC_BIGIDVFS_READ(addr) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_READ, addr, 0, 0)
#define SEC_BIGIDVFS_WRITE(addr, val) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_WRITE, addr, val, 0)
#define SEC_BIGIDVFS_SWREQ(swreq_reg) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWREQ, swreq_reg, 0, 0)
#else
#define SEC_BIGIDVFSENABLE(idvfs_ctrl, Vproc_x100, Vsram_x100) \
			API_BIGIDVFSENABLE(idvfs_ctrl, Vproc_x100, Vsram_x100)
#define SEC_BIGIDVFSDISABLE() \
			API_BIGIDVFSDISABLE()
#define SEC_BIGIDVFSPLLSETFREQ(Freq) \
			API_BIGPLLSETFREQ(Freq)
#define SEC_BIGIDVFSSRAMLDOSET(mv_x100) \
			BigSRAMLDOSet(mv_x100)
#define SEC_BIGIDVFS_READ(addr) \
			ptp3_reg_read(addr)
#define SEC_BIGIDVFS_WRITE(addr, val) \
			ptp3_reg_write(addr, val)
#define SEC_BIGIDVFS_SWREQ(swreq_reg) \
			API_BIGIDVFSSWREQ(swreq_reg)
#endif

/* Channel type*/
enum idvfs_channel {
	/* 0 */
	IDVFS_CHANNEL_SWP,
	/* 2 */
	IDVFS_CHANNEL_OCP,
	/* 1 */ /* nth */
	IDVFS_CHANNEL_OTP,

	IDVFS_CHANNEL_NP,
};

/* IDVFS Structure */
struct CHANNEL_STATUS {
	const char *name;
	unsigned short ch_number;
	unsigned short status;
	unsigned short percentage;
};

/*
idvfs_status, status machine
0: disable finish
1: enable finish
2: enable start
3: disable start
4: SWREQ start
5: disable and wait SWREQ finish
6: SWREQ finish can into disable

			 4 - > 5 - > 6
			^ |          |
			| v          |
0 - >  2 - > 1 - > 3 < - -
^                  |
|                  v
< - - - - - - - - -
*/

struct  IDVFS_INIT_OPT {
	unsigned char idvfs_status;
	unsigned short freq_max;
	unsigned short freq_min;
	unsigned short freq_cur;
	unsigned short i2c_speed;
	unsigned short ocp_endis;
	unsigned short otp_endis;
	unsigned int idvfs_ctrl_reg;
	unsigned int idvfs_enable_cnt;
	unsigned int idvfs_swreq_cnt;
	struct CHANNEL_STATUS *channel;
};

/* IDVFS Extern Function */

#ifdef CONFIG_ARM64
/* IDVFS_EXTERN int get_idvfs_status(void); */
/* IDVFS_EXTERN unsigned int get_vcore_ptp_volt(int uv); */
#endif /* CONFIG_ARM64 */

extern void aee_rr_rec_idvfs_ctrl_reg(u32 val);
extern void aee_rr_rec_idvfs_enable_cnt(u32 val);
extern void aee_rr_rec_idvfs_swreq_cnt(u32 val);
extern void aee_rr_rec_idvfs_curr_volt(u16 val);
extern void aee_rr_rec_idvfs_sram_ldo(u16 val);
extern void aee_rr_rec_idvfs_swavg_curr_pct_x100(u16 val);
extern void aee_rr_rec_idvfs_swreq_curr_pct_x100(u16 val);
extern void aee_rr_rec_idvfs_swreq_next_pct_x100(u16 val);
extern void aee_rr_rec_idvfs_state_manchine(u8 val);

extern u32 aee_rr_curr_idvfs_ctrl_reg(void);
extern u32 aee_rr_curr_idvfs_enable_cnt(void);
extern u32 aee_rr_curr_idvfs_swreq_cnt(void);
extern u16 aee_rr_curr_idvfs_curr_volt(void);
extern u16 aee_rr_curr_idvfs_sram_ldo(void);
extern u16 aee_rr_curr_idvfs_swavg_curr_pct_x100(void);
extern u16 aee_rr_curr_idvfs_swreq_curr_pct_x100(void);
extern u16 aee_rr_curr_idvfs_swreq_next_pct_x100(void);
extern u8 aee_rr_curr_idvfs_state_manchine(void);

/* #undef IDVFS_EXTERN */
/* iDVFS function */
/* extern int BigiDVFSEnable(unsigned int Fmax, unsigned int cur_vproc_mv_x100, unsigned int cur_vsram_mv_x100); */
/* extern int BigiDVFSDisable(void); */
extern int BigiDVFSEnable_hp(void); /* chg for hot plug */
extern int BigiDVFSDisable_hp(void); /* chg for hot plug */
extern int BigIDVFSFreq(unsigned int Freqpct_x100);
extern int BigiDVFSSWAvg(unsigned int Length, unsigned int EnDis);
extern int BigiDVFSSWAvgStatus(void);

extern int BigiDVFSChannel(unsigned int Channelm, unsigned int EnDis);
extern int BigiDVFSChannelGet(unsigned int Channelm);

extern int BigiDVFSPllSetFreq(unsigned int Freq); /* rang 507 ~ 3000(MHz) */
extern unsigned int BigiDVFSPllGetFreq(void);
extern int BigiDVFSPllDisable(void);

extern int BigiDVFSSRAMLDOSet(unsigned int mVolts_x100); /* range 60000 ~ 120000(mv_x100) */
extern unsigned int BigiDVFSSRAMLDOGet(void);
extern unsigned int BigiDVFSSRAMLDOEFUSE(void);
extern int BigiDVFSSRAMLDODisable(void);

extern int BigiDVFSPOSDIVSet(unsigned int pos_div); /* range 0/1/2 = div 1/2/4 */
extern unsigned int BigiDVFSPOSDIVGet(void);

extern int BigiDVFSPLLSetPCM(unsigned int freq); /* range 1000 ~ 3000(MHz), without pos div value */
extern unsigned int BigiDVFSPLLGetPCW(void);

/* iDVFSAPB */
extern int iDVFSAPB_DA9214_write(unsigned int sw_pAddr, unsigned int sw_pWdata);
extern int iDVFSAPB_init(void); /* it's only for DA9214 PMIC, return 0: 400K, 1:3.4M */

/* temp for PTP1 */
extern void eem_init_det_tmp(void);

/* SW Channel Turbo/Clamp mode, range 30% ~ 116% */
extern int BigIDVFSFreqMaxMin(unsigned int maxpct_x100, unsigned int minpct_x100);

#endif /* _MT_IDVFS_H  */
