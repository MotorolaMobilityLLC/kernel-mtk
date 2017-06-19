/*
 * Copyright (C) 2011 MediaTek, Inc.
 *
 * Author: Holmes Chiou <holmes.chiou@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MT_FREQHOPPING_H__
#define __MT_FREQHOPPING_H__

#define FHTAG "[FH] "

#define FH_MSG_ERROR(fmt, args...)	pr_err(FHTAG fmt, ##args)
#define FH_MSG_NOTICE(fmt, args...)	pr_notice(FHTAG fmt, ##args)
#define FH_MSG_INFO(fmt, args...)	/* pr_info(FHTAG fmt, ##args) */

enum FH_PLL_ID {
	FH_MIN_PLLID = 0,
	FH_ARMCA7_PLLID = FH_MIN_PLLID,
	FH_ARMCA15_PLLID = 1,
	FH_MAIN_PLLID = 2,	/* hf_faxi_ck = mainpll/4 */
	FH_MEM_PLLID = 3,		/* ?? */
	FH_MSDC_PLLID = 4,	/* hf_fmsdc30_1_ck = MSDCPLL/4 */
	FH_MM_PLLID = 5,	/* hf_fmfg_ck = MMPLL(455MHz) */
	FH_VENC_PLLID = 6,	/* hf_fcci400_ck = VENCPLL */
	FH_TVD_PLLID = 7,	/* hf_dpi0_ck = TVDPLL/2 = 297 */
	FH_VCODEC_PLLID = 8,	/* hf_fvdec_ck = VCODECPLL */
	FH_LVDS_PLLID = 9,
	FH_MSDC2_PLLID = 10,
	FH_LVDS2_PLLID = 11,
	FH_MAX_PLLID = FH_LVDS2_PLLID,
	FH_PLL_NUM
};

struct freqhopping_ssc {
	unsigned int freq;
	unsigned int dt;
	unsigned int df;
	unsigned int upbnd;
	unsigned int lowbnd;
	unsigned int dds;
};

extern int mtk_fhctl_enable_ssc_by_id(enum FH_PLL_ID);
extern int mtk_fhctl_disable_ssc_by_id(enum FH_PLL_ID);
extern int mtk_fhctl_hopping_by_id(enum FH_PLL_ID, unsigned int target_vco_frequency);

#endif				/* !__MT_FREQHOPPING_H__ */
