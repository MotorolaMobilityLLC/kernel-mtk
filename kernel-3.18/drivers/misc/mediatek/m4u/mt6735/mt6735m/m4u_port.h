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

#ifndef __M4U_PORT_D2_H__
#define __M4U_PORT_D2_H__

/* ==================================== */
/* about portid */
/* ==================================== */

enum {
	M4U_PORT_DISP_OVL0                ,
	M4U_PORT_DISP_OVL1                ,
	M4U_PORT_DISP_RDMA0               ,
	M4U_PORT_DISP_WDMA0               ,
	M4U_PORT_MDP_RDMA                 ,
	M4U_PORT_MDP_WDMA                 ,
	M4U_PORT_MDP_WROT                 ,
	M4u_PORT_DISP_FAKE                ,

	M4U_PORT_HW_VDEC_MC_EXT           ,
	M4U_PORT_HW_VDEC_PP_EXT           ,
	M4U_PORT_HW_VDEC_AVC_MV_EXT       ,
	M4U_PORT_HW_VDEC_PRED_RD_EXT      ,
	M4U_PORT_HW_VDEC_PRED_WR_EXT      ,
	M4U_PORT_HW_VDEC_VLD_EXT          ,
	M4U_PORT_HW_VDEC_PPWRAP_EXT       ,

	M4U_PORT_IMGO                     ,
	M4U_PORT_IMGO2O                   ,
	M4U_PORT_LSCI                     ,
	M4U_PORT_VENC_BSDMA_VDEC_POST0    ,
	M4U_PORT_JPGENC_RDMA              ,
	M4U_PORT_CAM_IMGI                 ,
	M4U_PORT_CAM_ESFKO                ,
	M4U_PORT_CAM_AAO                  ,
	M4U_PORT_JPGENC_BSDMA             ,
	M4U_PORT_VENC_MVQP                ,
	M4U_PORT_VENC_MC                  ,
	M4U_PORT_VENC_CDMA_VDEC_CDMA      ,
	M4U_PORT_VENC_REC_VDEC_WDMA       ,

	M4U_PORT_UNKNOWN                  ,
};
#define M4U_PORT_NR M4U_PORT_UNKNOWN

#endif
