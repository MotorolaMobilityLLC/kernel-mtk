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

#define MTK_SIP_KERNEL_IDVFS_READ						0x8200035F
#define MTK_SIP_KERNEL_IDVFS_WRITE						0x8200035E
#endif

/*
 * bit operation
 */
#define BIT_IDVFS(bit)		(1U << (bit))

#define MSB_IDVFS(range)	(1 ? range)
#define LSB_IDVFS(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r: Range in the form of MSB:LSB
 */
#define BITMASK_IDVFS(r) \
	(((unsigned) -1 >> (31 - MSB_IDVFS(r))) & ~((1U << LSB_IDVFS(r)) - 1))

#define GET_BITS_VAL_IDVFS(_bits_, _val_) \
	(((_val_) & (BITMASK_IDVFS(_bits_))) >> ((0) ? _bits_))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS_IDVFS(r, val)	((val << LSB_IDVFS(r)) & BITMASK_IDVFS(r))

/* dfine for IDVFS register service */
#ifdef __KERNEL__
#define idvfs_read(addr) \
		mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_READ, addr, 0, 0)
#define idvfs_write(addr, val) \
		mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_WRITE, addr, val, 0)
#else
#define idvfs_read(addr) \
		ptp3_reg_read(addr)
#define idvfs_write(addr, val) \
		ptp3_reg_write(addr, val)
#endif

#define idvfs_read_field(addr, range) \
		GET_BITS_VAL_IDVFS(range, idvfs_read(addr))
#define idvfs_write_field(addr, range, val) \
		idvfs_write(addr, (idvfs_read(addr) & ~(BITMASK_IDVFS(range))) | BITS_IDVFS(range, val))

/* ATF only support */
#ifdef __KERNEL__
#define SEC_BIGIDVFSENABLE(FMax, Vproc, Vsram) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE, FMax, Vproc, Vsram)
#define SEC_BIGIDVFSDISABLE() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE, 0, 0, 0)
#define SEC_BIGIDVFSPLLSETFREQ(Freq) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ, Freq, 0, 0)
#define SEC_BIGIDVFSSRAMLDOSET(mv_x100) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET, mv_x100, 0, 0)
#else
#define SEC_BIGIDVFSENABLE(FMax, Vproc_x100, Vsram_x100) \
			API_BIGIDVFSENABLE(FMax, Vproc_x100, Vsram_x100)
#define SEC_BIGIDVFSDISABLE() \
			API_BIGIDVFSDISABLE()
#define SEC_BIGIDVFSPLLSETFREQ(Freq) \
			API_BIGPLLSETFREQ(Freq)
#define SEC_BIGIDVFSSRAMLDOSET(mv_x100) \
			BigSRAMLDOSet(mv_x100)
#endif

/* ATF remove, add at Android code base, remove later due to build code issue */
#define SEC_BIGIDVFSFREQ(FreqPct) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSFREQ, FreqPct, 0, 0)
#define SEC_BIGIDVFSCHANNEL(Channel, EnDis) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSCHANNEL, Channel, EnDis, 0)
#define SEC_BIGIDVFSSWAVG(Length, EnDis) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVG, Length, EnDis, 0)
#define SEC_BIGIDVFSSWAVGSTATUS() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWAVGSTATUS, 0, 0, 0)
#define SEC_BIGIDVFSPURESWMODE(Func, Para) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSWMODE, Func, Para, 0) /* option */
#define SEC_BIGIDVFSSLOWMODE(pllus, voltus) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSLOWMODE, pllus, voltus, 0) /* option */
#define SEC_BIGIDVFSPLLDISABLE() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLDISABLE, 0, 0, 0)
#define SEC_BIGIDVFSPLLGETFREQ() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETFREQ, 0, 0, 0)
#define SEC_BIGIDVFSPLLSETPOSDIV(div) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPOSDIV, div, 0, 0)
#define SEC_BIGIDVFSPLLGETPOSDIV() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPOSDIV, 0, 0, 0)
#define SEC_BIGIDVFSPLLSETPCW(pcw) \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETPCW, pcw, 0, 0)
#define SEC_BIGIDVFSPLLGETPCW() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLGETPCW, 0, 0, 0)
#define SEC_BIGIDVFSSRAMLDOGET() \
			mt_secure_call_idvfs(MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOGET, 0, 0, 0)

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
	struct CHANNEL_STATUS *channel;
};

/* IDVFS Extern Function */

#ifdef CONFIG_ARM64
/* IDVFS_EXTERN int get_idvfs_status(void); */
/* IDVFS_EXTERN unsigned int get_vcore_ptp_volt(int uv); */
#endif /* CONFIG_ARM64 */

/* #undef IDVFS_EXTERN */

/* iDVFS function */
/* extern int BigiDVFSEnable(unsigned int Fmax, unsigned int cur_vproc_mv_x100, unsigned int cur_vsram_mv_x100); */
/* extern int BigiDVFSDisable(void); */
extern int BigiDVFSEnable_hp(void); /* chg for hot plug */
extern int BigiDVFSDisable_hp(void); /* chg for hot plug */
extern int BigiDVFSChannel(unsigned int Channelm, unsigned int EnDis);
extern int BigIDVFSFreq(unsigned int Freqpct_x100);
extern int BigiDVFSSWAvg(unsigned int Length, unsigned int EnDis);
extern int BigiDVFSSWAvgStatus(void);

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
