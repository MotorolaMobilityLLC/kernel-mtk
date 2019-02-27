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

#ifndef __CMDQ_ENGINE_H__
#define __CMDQ_ENGINE_H__

enum CMDQ_ENG_ENUM {
	/* ISP */
	CMDQ_ENG_WPEI = 0,
	CMDQ_ENG_WPEO,			/* 1 */
	CMDQ_ENG_WPEI2,			/* 2 */
	CMDQ_ENG_WPEO2,			/* 3 */
	CMDQ_ENG_ISP_IMGI,		/* 4 */
	CMDQ_ENG_ISP_IMGO,		/* 5 */
	CMDQ_ENG_ISP_IMG2O,		/* 6 */

	/* IPU */
	CMDQ_ENG_IPUI,			/* 7 */
	CMDQ_ENG_IPUO,			/* 8 */

	/* MDP */
	CMDQ_ENG_MDP_CAMIN,		/* 9 */
	CMDQ_ENG_MDP_CAMIN2,		/* 10 */
	CMDQ_ENG_MDP_RDMA0,		/* 11 */
	CMDQ_ENG_MDP_RDMA1,		/* 12 */
	CMDQ_ENG_MDP_HDR0,		/* 13 */
	CMDQ_ENG_MDP_AAL0,		/* 14 */
	CMDQ_ENG_MDP_RSZ0,		/* 15 */
	CMDQ_ENG_MDP_RSZ1,		/* 16 */
	CMDQ_ENG_MDP_TDSHP0,		/* 17 */
	CMDQ_ENG_MDP_COLOR0,		/* 18 */
	CMDQ_ENG_MDP_PATH0_SOUT,	/* 19 */
	CMDQ_ENG_MDP_PATH1_SOUT,	/* 20 */
	CMDQ_ENG_MDP_WROT0,		/* 21 */
	CMDQ_ENG_MDP_WROT1,		/* 22 */

	/* JPEG & VENC */
	CMDQ_ENG_JPEG_ENC,		/* 23 */
	CMDQ_ENG_VIDEO_ENC,		/* 24 */
	CMDQ_ENG_JPEG_DEC,		/* 25 */
	CMDQ_ENG_JPEG_REMDC,		/* 26 */

	/* DISP */
	CMDQ_ENG_DISP_UFOE,		/* 27 */
	CMDQ_ENG_DISP_AAL,		/* 28 */
	CMDQ_ENG_DISP_COLOR0,		/* 29 */
	CMDQ_ENG_DISP_RDMA0,		/* 30 */
	CMDQ_ENG_DISP_RDMA1,		/* 31 */
	CMDQ_ENG_DISP_RDMA2,		/* 32 */
	CMDQ_ENG_DISP_WDMA0,		/* 33 */
	CMDQ_ENG_DISP_WDMA1,		/* 34 */
	CMDQ_ENG_DISP_OVL0,		/* 35 */
	CMDQ_ENG_DISP_OVL1,		/* 36 */
	CMDQ_ENG_DISP_OVL2,		/* 37 */
	CMDQ_ENG_DISP_GAMMA,		/* 38 */
	CMDQ_ENG_DISP_DSI0_VDO,		/* 39 */
	CMDQ_ENG_DISP_DSI0_CMD,		/* 40 */
	CMDQ_ENG_DISP_DSI0,		/* 41 */
	CMDQ_ENG_DISP_2L_OVL0,		/* 42 */
	CMDQ_ENG_DISP_2L_OVL1,		/* 43 */
	CMDQ_ENG_DISP_2L_OVL2,		/* 44 */

	/* ISP */
	CMDQ_ENG_DPE,			/* 45 */
	CMDQ_ENG_RSC,			/* 46 */
	CMDQ_ENG_GEPF,			/* 47 */
	CMDQ_ENG_EAF,			/* 48 */
	CMDQ_ENG_OWE,			/* 49 */
	CMDQ_ENG_MFB,			/* 50 */
	CMDQ_ENG_FDVT,			/* 51 */

	/* temp: CMDQ internal usage */
	CMDQ_ENG_CMDQ,			/* 52 */
	CMDQ_ENG_DISP_MUTEX,		/* 53 */
	CMDQ_ENG_MMSYS_CONFIG,		/* 54 */

	/* Dummy Engine */
	CMDQ_ENG_MDP_RSZ2,		/* 55 */
	CMDQ_ENG_MDP_TDSHP1,		/* 56 */
	CMDQ_ENG_MDP_WDMA,		/* 57 */

	CMDQ_ENG_DISP_MERGE,		/* 58 */
	CMDQ_ENG_DISP_SPLIT0,		/* 59 */
	CMDQ_ENG_DISP_SPLIT1,		/* 60 */
	CMDQ_ENG_DISP_DSI1_VDO,		/* 61 */
	CMDQ_ENG_DISP_DSI1_CMD,		/* 62 */
	CMDQ_ENG_DISP_DSI1,		/* 63 */


	CMDQ_MAX_ENGINE_COUNT	/* ALWAYS keep at the end */
};

#define CMDQ_ENG_WPE_GROUP_BITS	((1LL << CMDQ_ENG_WPEI) |	\
				 (1LL << CMDQ_ENG_WPEO) |	\
				 (1LL << CMDQ_ENG_WPEI2) |	\
				 (1LL << CMDQ_ENG_WPEO2))

#define CMDQ_ENG_IPU_GROUP_BITS	((1LL << CMDQ_ENG_IPUI) |	\
				 (1LL << CMDQ_ENG_IPUO))

#define CMDQ_ENG_ISP_GROUP_BITS	((1LL << CMDQ_ENG_ISP_IMGI) |	\
				 (1LL << CMDQ_ENG_ISP_IMGO) |	\
				 (1LL << CMDQ_ENG_ISP_IMG2O))

#define CMDQ_ENG_MDP_GROUP_BITS	((1LL << CMDQ_ENG_MDP_CAMIN) |	\
				 (1LL << CMDQ_ENG_MDP_CAMIN2) |	\
				 (1LL << CMDQ_ENG_MDP_RDMA0) |	\
				 (1LL << CMDQ_ENG_MDP_RDMA1) |	\
				 (1LL << CMDQ_ENG_MDP_AAL0) |	\
				 (1LL << CMDQ_ENG_MDP_HDR0) |	\
				 (1LL << CMDQ_ENG_MDP_RSZ0) |	\
				 (1LL << CMDQ_ENG_MDP_RSZ1) |	\
				 (1LL << CMDQ_ENG_MDP_RSZ2) |	\
				 (1LL << CMDQ_ENG_MDP_TDSHP0) |	\
				 (1LL << CMDQ_ENG_MDP_TDSHP1) |	\
				 (1LL << CMDQ_ENG_MDP_COLOR0) |	\
				 (1LL << CMDQ_ENG_MDP_WROT0) |	\
				 (1LL << CMDQ_ENG_MDP_WROT1))

#define CMDQ_ENG_DISP_GROUP_BITS	\
				((1LL << CMDQ_ENG_DISP_UFOE) |		\
				(1LL << CMDQ_ENG_DISP_AAL) |		\
				(1LL << CMDQ_ENG_DISP_COLOR0) |	\
				(1LL << CMDQ_ENG_DISP_RDMA0) |		\
				(1LL << CMDQ_ENG_DISP_RDMA1) |		\
				(1LL << CMDQ_ENG_DISP_WDMA0) |		\
				(1LL << CMDQ_ENG_DISP_WDMA1) |		\
				(1LL << CMDQ_ENG_DISP_OVL0) |		\
				(1LL << CMDQ_ENG_DISP_OVL1) |		\
				(1LL << CMDQ_ENG_DISP_OVL2) |		\
				(1LL << CMDQ_ENG_DISP_2L_OVL0) |	\
				(1LL << CMDQ_ENG_DISP_2L_OVL1) |	\
				(1LL << CMDQ_ENG_DISP_2L_OVL2) |	\
				(1LL << CMDQ_ENG_DISP_GAMMA) |		\
				(1LL << CMDQ_ENG_DISP_MERGE) |		\
				(1LL << CMDQ_ENG_DISP_SPLIT0) |	\
				(1LL << CMDQ_ENG_DISP_SPLIT1) |	\
				(1LL << CMDQ_ENG_DISP_DSI0_VDO) |	\
				(1LL << CMDQ_ENG_DISP_DSI1_VDO) |	\
				(1LL << CMDQ_ENG_DISP_DSI0_CMD) |	\
				(1LL << CMDQ_ENG_DISP_DSI1_CMD) |	\
				(1LL << CMDQ_ENG_DISP_DSI0) |		\
				(1LL << CMDQ_ENG_DISP_DSI1))

#define CMDQ_ENG_VENC_GROUP_BITS	((1LL << CMDQ_ENG_VIDEO_ENC))

#define CMDQ_ENG_JPEG_GROUP_BITS	((1LL << CMDQ_ENG_JPEG_ENC) |	\
					 (1LL << CMDQ_ENG_JPEG_REMDC) |	\
					 (1LL << CMDQ_ENG_JPEG_DEC))

#define CMDQ_ENG_DPE_GROUP_BITS		(1LL << CMDQ_ENG_DPE)
#define CMDQ_ENG_RSC_GROUP_BITS		(1LL << CMDQ_ENG_RSC)
#define CMDQ_ENG_GEPF_GROUP_BITS	(1LL << CMDQ_ENG_GEPF)
#define CMDQ_ENG_OWE_GROUP_BITS		(1LL << CMDQ_ENG_OWE)
#define CMDQ_ENG_MFB_GROUP_BITS		(1LL << CMDQ_ENG_MFB)
#define CMDQ_ENG_EAF_GROUP_BITS		(1LL << CMDQ_ENG_EAF)
#define CMDQ_ENG_FDVT_GROUP_BITS	(1LL << CMDQ_ENG_FDVT)

#define CMDQ_ENG_ISP_GROUP_FLAG(flag)   ((flag) & (CMDQ_ENG_ISP_GROUP_BITS))
#define CMDQ_ENG_MDP_GROUP_FLAG(flag)   ((flag) & (CMDQ_ENG_MDP_GROUP_BITS))
#define CMDQ_ENG_DISP_GROUP_FLAG(flag)  ((flag) & (CMDQ_ENG_DISP_GROUP_BITS))
#define CMDQ_ENG_JPEG_GROUP_FLAG(flag)  ((flag) & (CMDQ_ENG_JPEG_GROUP_BITS))
#define CMDQ_ENG_VENC_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_VENC_GROUP_BITS))
#define CMDQ_ENG_DPE_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_DPE_GROUP_BITS))
#define CMDQ_ENG_RSC_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_RSC_GROUP_BITS))
#define CMDQ_ENG_GEPF_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_GEPF_GROUP_BITS))
#define CMDQ_ENG_WPE_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_WPE_GROUP_BITS))
#define CMDQ_ENG_OWE_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_OWE_GROUP_BITS))
#define CMDQ_ENG_MFB_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_MFB_GROUP_BITS))
#define CMDQ_ENG_EAF_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_EAF_GROUP_BITS))
#define CMDQ_ENG_FDVT_GROUP_FLAG(flag)	((flag) & (CMDQ_ENG_FDVT_GROUP_BITS))

#define CMDQ_FOREACH_GROUP(ACTION_struct)\
	ACTION_struct(CMDQ_GROUP_ISP, ISP)	\
	ACTION_struct(CMDQ_GROUP_MDP, MDP)	\
	ACTION_struct(CMDQ_GROUP_DISP, DISP)	\
	ACTION_struct(CMDQ_GROUP_JPEG, JPEG)	\
	ACTION_struct(CMDQ_GROUP_VENC, VENC)	\
	ACTION_struct(CMDQ_GROUP_DPE, DPE)	\
	ACTION_struct(CMDQ_GROUP_RSC, RSC)	\
	ACTION_struct(CMDQ_GROUP_GEPF, GEPF)	\
	ACTION_struct(CMDQ_GROUP_WPE, WPE)	\
	ACTION_struct(CMDQ_GROUP_OWE, OWE)	\
	ACTION_struct(CMDQ_GROUP_MFB, MFB)	\


#define MDP_GENERATE_ENUM(_enum, _string) _enum,

enum CMDQ_GROUP_ENUM {
	CMDQ_FOREACH_GROUP(MDP_GENERATE_ENUM)
	CMDQ_MAX_GROUP_COUNT,	/* ALWAYS keep at the end */
};

#endif				/* __CMDQ_ENGINE_H__ */
