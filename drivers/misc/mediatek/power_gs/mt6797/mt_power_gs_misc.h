extern struct proc_dir_entry *mt_power_gs_dir;


/* MDP, DISP, and SMI are in the DIS subsys */
/*Base address = 0x12000000 */
const unsigned int gs_mdp_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x200AC28, 0x00000001, 0x00000000,	/* DISP_COLOR_CK_ON */
	0x2009110, 0x00000100, 0x00000000,	/* MDP_TDSHP_CFG */
	0x2001000, 0x00000010, 0x00000010,	/* EN */
	0x2002000, 0x00000010, 0x00000010,	/* EN */
	0x2006008, 0x80000000, 0x80000000,	/* WDMA_EN */
	0x200707C, 0x00010000, 0x00010000,	/* VIDO_ROT_EN */
	0x200807C, 0x00010000, 0x00010000,	/* VIDO_ROT_EN */
	0x2001028, 0xFFFFFFFF, 0x00010371,	/* GMCIF_CON */
	0x2001240, 0xFFFFFFFF, 0x07000024,	/* DMABUF_CON_0 */
	0x2001250, 0xFFFFFFFF, 0x03000013,	/* DMABUF_CON_1 */
	0x2001260, 0xFFFFFFFF, 0x03000013,	/* DMABUF_CON_2 */
	0x2007008, 0xFFFFFFFF, 0x01802000,	/* VIDO_MAIN_BUF_SIZE */
	0x2007004, 0xFFFFFFFF, 0x3F000000	/* VIDO_DMA_PERF */
};
const unsigned int *gs_mdp_array_ptr = gs_mdp_array;
unsigned int gs_mdp_array_len = 39;

/*Base address = 0x12000000 */
const unsigned int gs_disp_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x200B2A0, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM0 */
	0x200B2A4, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM1 */
	0x200C2A0, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM0 */
	0x200C2A4, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM1 */
	0x200D2A0, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM0 */
	0x200D2A4, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM1 */
	0x200E2A0, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM0 */
	0x200E2A4, 0xFFFFFFFF, 0x00000000,	/* OVL_FUNC_DCM1 */
	0x200F0B4, 0x00000FFF, 0x00000000,	/* DISP_RDMA_INTERNAL_CG_CON */
	0x20100B4, 0x00000FFF, 0x00000000,	/* DISP_RDMA_INTERNAL_CG_CON */
	0x2013C28, 0x00000001, 0x00000000,	/* DISP_COLOR_CK_ON */
	0x201E0E8, 0x000000FF, 0x00000000,	/* DPI_INTERNAL_CG_CON */
	0x2015020, 0x00000010, 0x00000000,	/* DISP_AAL_CFG */
	0x2014020, 0x00000100, 0x00000100,	/* DISP_CCORR_CFG */
	0x2011008, 0x80000000, 0x80000000,	/* WDMA_EN */
	0x2012008, 0x80000000, 0x80000000,	/* WDMA_EN */
	0x2018020, 0x00000100, 0x00000000	/* DISP_DITHER_CFG */
};
const unsigned int *gs_disp_array_ptr = gs_disp_array;
unsigned int gs_disp_array_len = 51;

/*Base address = 0x12000000*/
const unsigned int gs_smi_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x2020010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB0 */
	0x4010010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB1 */
	0x8001010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB2 */
	0x5001010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB3 */
	0x0002010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB4 */
	0x2021010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB5 */
	0x3001010, 0x0000FFF0, 0x000007F0,	/* SMI_LARB6 */
	0x2022300, 0x00003FFF, 0x00000403,	/* SMI_DCM */
	0x2020100, 0xFFFFFFFF, 0x0000000B,	/* disp_ovl0 */
	0x2020104, 0xFFFFFFFF, 0x0000000B,	/* disp_ovl0_2l */
	0x2020108, 0xFFFFFFFF, 0x0000000B,	/* disp_rdma0 */
	0x202010C, 0xFFFFFFFF, 0x00000004,	/* disp_wdma0 */
	0x2020110, 0xFFFFFFFF, 0x00000005,	/* mdp_rdma0 */
	0x2020114, 0xFFFFFFFF, 0x00000004,	/* mdp_wdma0 */
	0x2020118, 0xFFFFFFFF, 0x00000002,	/* mdp_wrot0 */
	0x2020200, 0xFFFFFFFF, 0x0000001C,	/* disp_ovl0 */
	0x2020204, 0xFFFFFFFF, 0x00000005,	/* disp_ovl0_2l */
	0x2020208, 0xFFFFFFFF, 0x0000001F,	/* disp_rdma0 */
	0x202020C, 0xFFFFFFFF, 0x0000000A,
	0x2020210, 0xFFFFFFFF, 0x00000005,
	0x2020214, 0xFFFFFFFF, 0x00000001,
	0x2020218, 0xFFFFFFFF, 0x00000005
};
const unsigned int *gs_smi_array_ptr = gs_smi_array;
unsigned int gs_smi_array_len = 66;


/* IMG is in the ISP subsys */
/*Base address = 0x12000000, IMG*/
const unsigned int gs_img_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x8000480, 0x7FFFFFFF, 0x00000000,	/* CTL_RAW_DCM_DIS */
	0x8000484, 0x000007FF, 0x00000000,	/* CTL_DMA_DCM_DIS */
	0x8000488, 0x00000001, 0x00000000,	/* CTL_TOP_DCM_DIS */
	0x3022090, 0xFFFFFFFF, 0x00000000,	/* CTL_YUV_DCM_DIS */
	0x3022094, 0x00000007, 0x00000000,	/* CTL_YUV2_DCM_DIS */
	0x3022098, 0x0003FFFF, 0x00000000,	/* CTL_RGB_DCM_DIS */
	0x302209C, 0x0001FFFF, 0x00000000,	/* CTL_DMA_DCM_DIS */
	0x30220A0, 0x00000001, 0x00000000	/* CTL_TOP_DCM_DIS */
};
const unsigned int *gs_img_array_ptr = gs_img_array;
unsigned int gs_img_array_len = 24;


/* VENC is in the VEN subsys */
/*Base address = 0x12000000*/
const unsigned int gs_venc_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x50020EC, 0xFFFFFFFF, 0x80000001	/* VENC */
};
const unsigned int *gs_venc_array_ptr = gs_venc_array;
unsigned int gs_venc_array_len = 3;


/* VDEC is in the VDE subsys */
/*Base address = 0x12000000*/
const unsigned int gs_vdec_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x4000018, 0x00000001, 0x00000000	/* VDEC_DCM_CON */
};
const unsigned int *gs_vdec_array_ptr = gs_vdec_array;
unsigned int gs_vdec_array_len = 3;


/* Infra is always on */
/*Base address = 0x10000000*/
const unsigned int gs_infra_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x01070, 0xFFFFFFFF, 0x40780603,	/* infra_bus_dcm */
	0x01074, 0xFFFFFFFF, 0xB0380603,	/* peri_bus_dcm */
	0x01078, 0xFFFFFFFF, 0x00000000,	/* MEM_DCM_CTRL */
	0x0107C, 0xFFFFFFFF, 0x00000000		/* DFS_MEM_DCM_CTRL */
};
const unsigned int *gs_infra_array_ptr = gs_infra_array;
unsigned int gs_infra_array_len = 12;


/* ClockTop is always on */
/*Base address = 0x10000000*/
const unsigned int gs_clockTop_array[] = {
	/* Offset     Mask        Golden Setting Value */
	0x01090, 0xFFFFFFFF, 0xFFBFFFBC,	/* CG0 */
	0x01094, 0xFFFFFFFF, 0x7FFFFFFF,	/* CG1 */
	0x010B0, 0xFFFFFFFF, 0xFFFFF7FF,	/* CG2 */
	0x00220, 0xFFFFFFFF, 0x00000000,	/* fqmtr_en (topckgen) */
	0x00104, 0xFFFFFFFF, 0xFFFF0000,	/* fqmtr_en (topckgen) */
	0x00108, 0xFFFFFFFF, 0xFFFF0000,	/* fqmtr_en (topckgen) */
	0x01500, 0xFFFFFFFF, 0x00000001,	/* top debug mux */
	0x01504, 0xFFFFFFFF, 0x00000001,	/* top debug mux */
	0x01098, 0xFFFFFFFF, 0x00004C70,	/* top debug mux */
	0x11000, 0xFFFFFFFF, 0x030C0000	/* top debug mux */
};
const unsigned int *gs_clockTop_array_ptr = gs_clockTop_array;
unsigned int gs_clockTop_array_len = 30;
