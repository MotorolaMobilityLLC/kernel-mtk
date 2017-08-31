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

#ifndef __M4U_PORT_PRIV_H__
#define __M4U_PORT_PRIV_H__

static const char *const gM4U_SMILARB[] = {
	"mediatek,smi_larb0", "mediatek,smi_larb1", "mediatek,smi_larb2", "mediatek,smi_larb3",
	"mediatek,smi_larb4", "mediatek,smi_larb5", "mediatek,smi_larb6", "mediatek,smi_larb7",
	"mediatek,smi_larb8"
};

#define M4U0_PORT_INIT(name, slave, larb_inx, larb_id, port)  {\
		name, 0, slave, larb_inx, port, (((larb_id)<<7)|((port)<<2)), 1\
}

struct m4u_port_t gM4uPort[] = {
	M4U0_PORT_INIT("DISP_OVL0", 0, 0, 0, 0),
	M4U0_PORT_INIT("DISP_2L_OVL0_LARB0", 0, 0, 0, 1),
	M4U0_PORT_INIT("DISP_RDMA0", 0, 0, 0, 2),
	M4U0_PORT_INIT("DISP_WDMA0", 0, 0, 0, 3),
	M4U0_PORT_INIT("MDP_RDMA0", 0, 0, 0, 4),
	M4U0_PORT_INIT("MDP_WDMA0", 0, 0, 0, 5),
	M4U0_PORT_INIT("MDP_WROT0", 0, 0, 0, 6),
	M4U0_PORT_INIT("DISP_FAKE0", 0, 0, 0, 7),

	M4U0_PORT_INIT("DISP_OVL1", 0, 1, 1, 0),
	M4U0_PORT_INIT("DISP_2L_OVL1", 0, 1, 1, 1),
	M4U0_PORT_INIT("DISP_RDMA1", 0, 1, 1, 2),
	M4U0_PORT_INIT("DISP_RDMA2", 0, 1, 1, 3),
	M4U0_PORT_INIT("DISP_WDMA1", 0, 1, 1, 4),
	M4U0_PORT_INIT("DISP_OD_R", 0, 1, 1, 5),
	M4U0_PORT_INIT("DISP_OD_W", 0, 1, 1, 6),
	M4U0_PORT_INIT("DISP_2L_OVL0_LARB1", 0, 1, 1, 7),
	M4U0_PORT_INIT("MDP_RDMA1", 0, 1, 1, 8),
	M4U0_PORT_INIT("MDP_WROT1", 0, 1, 1, 9),
	M4U0_PORT_INIT("DISP_FAKE1", 0, 1, 1, 10),

	M4U0_PORT_INIT("VDEC_MC", 0, 4, 2, 0),
	M4U0_PORT_INIT("VDEC_PP", 0, 4, 2, 1),
	M4U0_PORT_INIT("VDEC_UFO", 0, 4, 2, 2),
	M4U0_PORT_INIT("VDEC_VLD", 0, 4, 2, 3),
	M4U0_PORT_INIT("VDEC_VLD2", 0, 4, 2, 4),
	M4U0_PORT_INIT("VDEC_AVC_MV", 0, 4, 2, 5),
	M4U0_PORT_INIT("VDEC_PRED_RD", 0, 4, 2, 6),
	M4U0_PORT_INIT("VDEC_PRED_WR", 0, 4, 2, 7),
	M4U0_PORT_INIT("VDEC_PPWRAP", 0, 4, 2, 8),
	M4U0_PORT_INIT("VDEC_TILE", 0, 4, 2, 9),

	M4U0_PORT_INIT("CAM_IMGI", 0, 5, 3, 0),
	M4U0_PORT_INIT("CAM_IMG2O", 0, 5, 3, 1),
	M4U0_PORT_INIT("CAM_IMG3O", 0, 5, 3, 2),
	M4U0_PORT_INIT("CAM_VIPI", 0, 5, 3, 3),
	M4U0_PORT_INIT("CAM_LCEI", 0, 5, 3, 4),
	M4U0_PORT_INIT("CAM_WPEA_RDMA0", 0, 5, 3, 5),
	M4U0_PORT_INIT("CAM_WPEA_RDMA1", 0, 5, 3, 6),
	M4U0_PORT_INIT("CAM_WPEA_WDMA", 0, 5, 3, 7),
	M4U0_PORT_INIT("CAM_FD_RP", 0, 5, 3, 8),
	M4U0_PORT_INIT("CAM_FD_WR", 0, 5, 3, 9),
	M4U0_PORT_INIT("CAM_FD_RB", 0, 5, 3, 10),
	M4U0_PORT_INIT("CAM_WPEB_RDMA0", 0, 5, 3, 11),
	M4U0_PORT_INIT("CAM_WPEB_RDMA1", 0, 5, 3, 12),
	M4U0_PORT_INIT("CAM_WPEB_WDMA", 0, 5, 3, 13),
	M4U0_PORT_INIT("CAM_EAF_RDMA0", 0, 5, 3, 14),
	M4U0_PORT_INIT("CAM_EAF_RDMA1", 0, 5, 3, 15),
	M4U0_PORT_INIT("CAM_EAF_WDMA", 0, 5, 3, 16),
	M4U0_PORT_INIT("CAM_DPE_RDMA", 0, 5, 3, 17),
	M4U0_PORT_INIT("CAM_DPE_WDMA", 0, 5, 3, 18),
	M4U0_PORT_INIT("CAM_RSC_RDMA0", 0, 5, 3, 19),
	M4U0_PORT_INIT("CAM_RSC_WDMA", 0, 5, 3, 20),

	M4U0_PORT_INIT("CAM_IMGO", 0, 6, 4, 0),
	M4U0_PORT_INIT("CAM_RRZO", 0, 6, 4, 1),
	M4U0_PORT_INIT("CAM_AAO", 0, 6, 4, 2),
	M4U0_PORT_INIT("CAM_AFO", 0, 6, 4, 3),
	M4U0_PORT_INIT("CAM_LSCI0", 0, 6, 4, 4),
	M4U0_PORT_INIT("CAM_LSCI1", 0, 6, 4, 5),
	M4U0_PORT_INIT("CAM_DPO", 0, 6, 4, 6),
	M4U0_PORT_INIT("CAM_BPCI", 0, 6, 4, 7),
	M4U0_PORT_INIT("CAM_LCSO", 0, 6, 4, 8),
	M4U0_PORT_INIT("CAM_RSSO", 0, 6, 4, 9),
	M4U0_PORT_INIT("CAM_UFEO", 0, 6, 4, 10),
	M4U0_PORT_INIT("CAM_SOCO", 0, 6, 4, 11),
	M4U0_PORT_INIT("CAM_SOC1", 0, 6, 4, 12),
	M4U0_PORT_INIT("CAM_SOC2", 0, 6, 4, 13),
	M4U0_PORT_INIT("CAM_CCUI", 0, 6, 4, 14),
	M4U0_PORT_INIT("CAM_CCUO", 0, 6, 4, 15),
	M4U0_PORT_INIT("CAM_CACI", 0, 6, 4, 16),
	M4U0_PORT_INIT("CAM_RAWI", 0, 6, 4, 17),
	M4U0_PORT_INIT("CAM_CCUG", 0, 6, 4, 18),

	M4U0_PORT_INIT("MJC_MV_RD", 0, 8, 5, 0),
	M4U0_PORT_INIT("MJC_MV_WR", 0, 8, 5, 1),
	M4U0_PORT_INIT("MJC_DMA_RD", 0, 8, 5, 2),
	M4U0_PORT_INIT("MJC_DMA_WR", 0, 8, 5, 3),

	M4U0_PORT_INIT("VENC_RCPU", 0, 7, 6, 0),
	M4U0_PORT_INIT("VENC_REC", 0, 7, 6, 1),
	M4U0_PORT_INIT("VENC_BSDMA", 0, 7, 6, 2),
	M4U0_PORT_INIT("VENC_SV_COMV", 0, 7, 6, 3),
	M4U0_PORT_INIT("VENC_SV_SEGID", 0, 7, 6, 4),
	M4U0_PORT_INIT("VENC_RD_COMV", 0, 7, 6, 5),
	M4U0_PORT_INIT("VENC_RD_SEGID", 0, 7, 6, 6),
	M4U0_PORT_INIT("JPGENC_RDMA", 0, 7, 6, 7),
	M4U0_PORT_INIT("JPGENC_BSDMA", 0, 7, 6, 8),
	M4U0_PORT_INIT("JPGDEC_WDMA", 0, 7, 6, 9),
	M4U0_PORT_INIT("JPGDEC_BSDMA", 0, 7, 6, 10),
	M4U0_PORT_INIT("VENC_CUR_LUMA", 0, 7, 6, 11),
	M4U0_PORT_INIT("VENC_CUR_CHROMA", 0, 7, 6, 12),
	M4U0_PORT_INIT("VENC_REF_LUMA", 0, 7, 6, 13),
	M4U0_PORT_INIT("VENC_REF_CHROMA", 0, 7, 6, 14),
	M4U0_PORT_INIT("VENC_NBM_RDMA", 0, 7, 6, 15),
	M4U0_PORT_INIT("VENC_NBM_WDMA", 0, 7, 6, 16),

	M4U0_PORT_INIT("VPU", 0, 0, 7, 0),

	M4U0_PORT_INIT("UNKNOWN", 0, 0, 0, 0)
};

#endif
