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
	CMDQ_ENG_ISP_IMGI = 0,
	CMDQ_ENG_ISP_IMGO,	/* 1 */
	CMDQ_ENG_ISP_IMG2O,	/* 2 */

	/* MDP */
	CMDQ_ENG_MDP_CAMIN,	/* 3 */
	CMDQ_ENG_MDP_RDMA0,	/* 4 */
	CMDQ_ENG_MDP_RSZ0,	/* 5 */
	CMDQ_ENG_MDP_RSZ1,	/* 6 */
	CMDQ_ENG_MDP_TDSHP0,	/* 7 */
	CMDQ_ENG_MDP_COLOR0,	/* 8 */
	CMDQ_ENG_MDP_PATH0_SOUT,	/* 9 */
	CMDQ_ENG_MDP_PATH1_SOUT,	/* 10 */
	CMDQ_ENG_MDP_WROT0,	/* 11 */
	CMDQ_ENG_MDP_WDMA,	/* 12 */

	/* JPEG & VENC */
	CMDQ_ENG_JPEG_ENC,	/* 14 */
	CMDQ_ENG_VIDEO_ENC,	/* 15 */
	CMDQ_ENG_JPEG_DEC,	/* 16 */
	CMDQ_ENG_JPEG_REMDC,	/* 17 */

	/* DISP */
	CMDQ_ENG_DISP_UFOE,	/* 18 */
	CMDQ_ENG_DISP_AAL,	/* 19 */
	CMDQ_ENG_DISP_COLOR0,	/* 20 */
	CMDQ_ENG_DISP_RDMA0,	/* 21 */
	CMDQ_ENG_DISP_RDMA1,	/* 22 */
	CMDQ_ENG_DISP_WDMA0,	/* 23 */
	CMDQ_ENG_DISP_WDMA1,	/* 24 */
	CMDQ_ENG_DISP_OVL0,	/* 25 */
	CMDQ_ENG_DISP_OVL1,	/* 26 */
	CMDQ_ENG_DISP_OVL2,	/* 27 */
	CMDQ_ENG_DISP_GAMMA,	/* 28 */
	CMDQ_ENG_DISP_DSI0_VDO,	/* 29 */
	CMDQ_ENG_DISP_DSI0_CMD,	/* 30 */
	CMDQ_ENG_DISP_DSI0,	/* 31 */
	CMDQ_ENG_DISP_DPI,	/* 32 */
	CMDQ_ENG_DISP_2L_OVL0,	/* 33 */
	CMDQ_ENG_DISP_2L_OVL1,	/* 34 */
	CMDQ_ENG_DISP_2L_OVL2,	/* 35 */

	/* DPE */
	CMDQ_ENG_DPE,	/* 36 */

	/* temp: CMDQ internal usage */
	CMDQ_ENG_CMDQ,
	CMDQ_ENG_DISP_MUTEX,
	CMDQ_ENG_MMSYS_CONFIG,

	/* Dummy Engine */
	CMDQ_ENG_MDP_TDSHP1,
	CMDQ_ENG_MDP_MOUT0,
	CMDQ_ENG_MDP_MOUT1,
	CMDQ_ENG_MDP_RDMA1,
	CMDQ_ENG_MDP_RSZ2,
	CMDQ_ENG_MDP_WROT1,

	CMDQ_ENG_DISP_COLOR1,
	CMDQ_ENG_DISP_RDMA2,
	CMDQ_ENG_DISP_MERGE,
	CMDQ_ENG_DISP_SPLIT0,
	CMDQ_ENG_DISP_SPLIT1,
	CMDQ_ENG_DISP_DSI1_VDO,
	CMDQ_ENG_DISP_DSI1_CMD,
	CMDQ_ENG_DISP_DSI1,

	CMDQ_MAX_ENGINE_COUNT	/* ALWAYS keep at the end */
};

#endif				/* __CMDQ_ENGINE_H__ */
