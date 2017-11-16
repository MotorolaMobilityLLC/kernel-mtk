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

static const char * const gM4U_SMILARB[] = { "mediatek,SMI_LARB0", "mediatek,SMI_LARB1" };

#define M4U0_PORT_INIT(name, slave, larb, port)  {\
		name, 0, slave, larb, port, (((larb)<<7)|((port)<<2)), 1\
}

m4u_port_t gM4uPort[] = {
	M4U0_PORT_INIT("DISP_OVL0", 0, 0, 0),
	M4U0_PORT_INIT("DISP_RDMA0", 0, 0, 1),
	M4U0_PORT_INIT("DISP_WDMA0", 0, 0, 2),
	M4U0_PORT_INIT("MDP_RDMA", 0, 0, 3),
	M4U0_PORT_INIT("MDP_WDMA", 0, 0, 4),
	M4U0_PORT_INIT("MDP_WROT", 0, 0, 5),

	M4U0_PORT_INIT("CAM_IMGO", 0, 1, 0),
	M4U0_PORT_INIT("CAM_IMG2O", 0, 1, 1),
	M4U0_PORT_INIT("CAM_LSCI", 0, 1, 2),
	M4U0_PORT_INIT("VENC_BSDMA_VDEC_POST0", 0, 1, 3),
	M4U0_PORT_INIT("CAM_IMGI", 0, 1, 4),
	M4U0_PORT_INIT("CAM_ESFKO", 0, 1, 5),
	M4U0_PORT_INIT("CAM_AAO", 0, 1, 6),
	M4U0_PORT_INIT("VENC_MVQP", 0, 1, 7),
	M4U0_PORT_INIT("VENC_MC", 0, 1, 8),
	M4U0_PORT_INIT("VENC_CDMA_VDEC_CDMA", 0, 1, 9),
	M4U0_PORT_INIT("VENC_REC_VDEC_WDMA", 0, 1, 10),
};

#endif
