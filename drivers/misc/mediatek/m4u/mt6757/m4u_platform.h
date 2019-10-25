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
	"mediatek,smi_larb0", "mediatek,smi_larb1", "mediatek,smi_larb2",
	"mediatek,smi_larb3", "mediatek,smi_larb4", "mediatek,smi_larb5"};

#define M4U0_PORT_INIT(name, slave, larb, port)                                \
	{                                                                      \
		name, 0, slave, larb, port, (((larb) << 7) | ((port) << 2)), 1 \
	}

m4u_port_t gM4uPort[] = {

	M4U0_PORT_INIT("DISP_OVL0", 0, 0, 0),
	M4U0_PORT_INIT("DISP_2L_OVL0_LARB0", 1, 0, 1),
	M4U0_PORT_INIT("DISP_RDMA0", 0, 0, 2),
	M4U0_PORT_INIT("DISP_WDMA0", 0, 0, 3),
	M4U0_PORT_INIT("MDP_RDMA0", 0, 0, 4),
	M4U0_PORT_INIT("MDP_WDMA0", 0, 0, 5),
	M4U0_PORT_INIT("MDP_WROT0", 0, 0, 6),
	M4U0_PORT_INIT("DISP_FAKE_LARB0", 0, 0, 7),

	M4U0_PORT_INIT("VDEC_MC", 0, 1, 0),
	M4U0_PORT_INIT("VDEC_PP", 0, 1, 1),
	M4U0_PORT_INIT("VDEC_VLD", 0, 1, 2),
	M4U0_PORT_INIT("VDEC_AVC_MV", 0, 1, 3),
	M4U0_PORT_INIT("VDEC_PRED_RD", 0, 1, 4),
	M4U0_PORT_INIT("VDEC_PRED_WR", 0, 1, 5),
	M4U0_PORT_INIT("VDEC_PPWRAP", 0, 1, 6),

	M4U0_PORT_INIT("CAM_IMGO", 0, 2, 0),
	M4U0_PORT_INIT("CAM_RRZO", 0, 2, 1),
	M4U0_PORT_INIT("CAM_AAO", 0, 2, 2),
	M4U0_PORT_INIT("CAM_AFO", 0, 2, 3),
	M4U0_PORT_INIT("CAM_LSCI_0", 0, 2, 4),
	M4U0_PORT_INIT("CAM_LSC3I", 0, 2, 5),
	M4U0_PORT_INIT("CAM_RSSO", 0, 2, 6),
	M4U0_PORT_INIT("CAM_SV0", 0, 2, 7),
	M4U0_PORT_INIT("CAM_SV1", 0, 2, 8),
	M4U0_PORT_INIT("CAM_SV2", 0, 2, 9),
	M4U0_PORT_INIT("CAM_LCSO", 0, 2, 10),
	M4U0_PORT_INIT("CAM_UFEO", 0, 2, 11),
	M4U0_PORT_INIT("CAM_BPCI", 0, 2, 12),
	M4U0_PORT_INIT("CAM_PDO", 0, 2, 13),
	M4U0_PORT_INIT("CAM_RAWI", 0, 2, 14),
	M4U0_PORT_INIT("CAM_CCUI", 0, 2, 15),
	M4U0_PORT_INIT("CAM_CCUO", 0, 2, 16),
	M4U0_PORT_INIT("CAM_CCUG", 0, 2, 17),

	M4U0_PORT_INIT("VENC_RCPU", 0, 3, 0),
	M4U0_PORT_INIT("VENC_REC", 0, 3, 1),
	M4U0_PORT_INIT("VENC_BSDMA", 0, 3, 2),
	M4U0_PORT_INIT("VENC_SV_COMV", 0, 3, 3),
	M4U0_PORT_INIT("VENC_RD_COMV", 0, 3, 4),
	M4U0_PORT_INIT("JPGENC_RDMA", 0, 3, 5),
	M4U0_PORT_INIT("JPGENC_BSDMA", 0, 3, 6),
	M4U0_PORT_INIT("JPGDEC_WDMA", 0, 3, 7),
	M4U0_PORT_INIT("JPGDEC_BSDMA", 0, 3, 8),
	M4U0_PORT_INIT("VENC_CUR_LUMA", 0, 3, 9),
	M4U0_PORT_INIT("VENC_CUR_CHROMA", 0, 3, 10),
	M4U0_PORT_INIT("VENC_REF_LUMA", 0, 3, 11),
	M4U0_PORT_INIT("VENC_REF_CHROMA", 0, 3, 12),

	M4U0_PORT_INIT("DISP_OVL1", 0, 4, 0),
	M4U0_PORT_INIT("DISP_2L_OVL1", 0, 4, 1),
	M4U0_PORT_INIT("DISP_RDMA1", 0, 4, 2),
	M4U0_PORT_INIT("DISP_RDMA2", 0, 4, 3),
	M4U0_PORT_INIT("DISP_WDMA1", 0, 4, 4),
	M4U0_PORT_INIT("DISP_OD_R", 0, 4, 5),
	M4U0_PORT_INIT("DISP_OD_W", 0, 4, 6),
	M4U0_PORT_INIT("DISP_2L_OVL0_LARB4", 0, 4, 7),
	M4U0_PORT_INIT("MDP_RDMA1", 0, 4, 8),
	M4U0_PORT_INIT("MDP_WROT1", 0, 4, 9),
	M4U0_PORT_INIT("DISP_FAKE_LARB4", 0, 4, 10),

	M4U0_PORT_INIT("CAM_IMGI", 0, 5, 0),
	M4U0_PORT_INIT("CAM_IMG2O", 0, 5, 1),
	M4U0_PORT_INIT("CAM_IMG3O", 0, 5, 2),
	M4U0_PORT_INIT("CAM_VIPI", 0, 5, 3),
	M4U0_PORT_INIT("CAM_ICEI", 0, 5, 4),
	M4U0_PORT_INIT("CAM_RP", 0, 5, 5),
	M4U0_PORT_INIT("CAM_WR", 0, 5, 6),
	M4U0_PORT_INIT("CAM_RB", 0, 5, 7),
	M4U0_PORT_INIT("CAM_DPE_RDMA", 0, 5, 8),
	M4U0_PORT_INIT("CAM_DPE_WDMA", 0, 5, 9),
	M4U0_PORT_INIT("CAM_RSC_RDMA", 0, 5, 10),
	M4U0_PORT_INIT("CAM_RSC_WDMA", 0, 5, 11),
	M4U0_PORT_INIT("CAM_GEPFI", 0, 5, 12),
	M4U0_PORT_INIT("CAM_GEPFO", 0, 5, 13),

	M4U0_PORT_INIT("UNKNOWN", 0, 0, 0)};

#endif
