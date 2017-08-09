#ifndef __M4U_PORT_D3_H__
#define __M4U_PORT_D3_H__

/* ==================================== */
/* about portid */
/* ==================================== */

enum {
	M4U_PORT_DISP_OVL0                ,
	M4U_PORT_DISP_RDMA0               ,
	M4U_PORT_DISP_WDMA0               ,
	M4U_PORT_DISP_OVL1                ,
	M4U_PORT_DISP_RDMA1               ,
	M4U_PORT_DISP_OD_R                ,
	M4U_PORT_DISP_OD_W                ,
	M4U_PORT_MDP_RDMA                 ,
	M4U_PORT_MDP_WDMA                 ,
	M4U_PORT_MDP_WROT                 ,

	M4U_PORT_HW_VDEC_MC_EXT           ,
	M4U_PORT_HW_VDEC_PP_EXT           ,
	M4U_PORT_HW_VDEC_AVC_MV_EXT       ,
	M4U_PORT_HW_VDEC_PRED_RD_EXT      ,
	M4U_PORT_HW_VDEC_PRED_WR_EXT      ,
	M4U_PORT_HW_VDEC_VLD_EXT          ,
	M4U_PORT_HW_VDEC_PPWRAP_EXT       ,

	M4U_PORT_IMGO                ,
	M4U_PORT_RRZO                ,
	M4U_PORT_AAO                 ,
	M4U_PORT_LCSO                ,
	M4U_PORT_ESFKO               ,
	M4U_PORT_IMGO_S              ,
	M4U_PORT_LSCI                ,
	M4U_PORT_LSCI_D              ,
	M4U_PORT_BPCI                ,
	M4U_PORT_BPCI_D              ,
	M4U_PORT_UFDI                ,
	M4U_PORT_IMGI                ,
	M4U_PORT_IMG2O               ,
	M4U_PORT_IMG3O               ,
	M4U_PORT_VIPI                ,
	M4U_PORT_VIP2I               ,
	M4U_PORT_VIP3I               ,
	M4U_PORT_LCEI                ,
	M4U_PORT_RB                  ,
	M4U_PORT_RP                  ,
	M4U_PORT_WR                  ,

	M4U_PORT_VENC_RCPU           ,
	M4U_PORT_VENC_REC            ,
	M4U_PORT_VENC_BSDMA          ,
	M4U_PORT_VENC_SV_COMV        ,
	M4U_PORT_VENC_RD_COMV        ,
	M4U_PORT_JPGENC_RDMA         ,
	M4U_PORT_JPGENC_BSDMA        ,
	M4U_PORT_JPGDEC_WDMA         ,
	M4U_PORT_JPGDEC_BSDMA        ,
	M4U_PORT_VENC_CUR_LUMA       ,
	M4U_PORT_VENC_CUR_CHROMA     ,
	M4U_PORT_VENC_REF_LUMA       ,
	M4U_PORT_VENC_REF_CHROMA     ,

	M4U_PORT_UNKNOWN                  ,
};
#define M4U_PORT_NR M4U_PORT_UNKNOWN

#endif
