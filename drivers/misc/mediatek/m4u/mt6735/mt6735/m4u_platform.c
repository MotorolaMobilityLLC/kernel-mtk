#include "m4u_platform.h"
#include "m4u_priv.h"
#include "m4u_hw.h"

unsigned int gM4UTagCount[]  = {32};

const char *gM4U_SMILARB[] = {
	"mediatek,SMI_LARB0", "mediatek,SMI_LARB1", "mediatek,SMI_LARB2", "mediatek,SMI_LARB3"};

M4U_RANGE_DES_T gM4u0_seq[M4U0_SEQ_NR] = {{0} };
M4U_RANGE_DES_T *gM4USeq[] = {gM4u0_seq};

#define M4U0_PORT_INIT(slave, larb, port)  (0, slave, larb, port, (((larb)<<7)|((port)<<2)), 1)

m4u_port_t gM4uPort[] = {
	{ "DISP_OVL0",                  M4U0_PORT_INIT(0, 0,  0),    },
	{ "DISP_RDMA0",                 M4U0_PORT_INIT(0, 0,  1),    },
	{ "DISP_WDMA0",                 M4U0_PORT_INIT(0, 0,  2),    },
	{ "DISP_RDMA1",                 M4U0_PORT_INIT(0, 0,  3),    },
	{ "MDP_RDMA",                   M4U0_PORT_INIT(0, 0,  4),    },
	{ "MDP_WDMA",                   M4U0_PORT_INIT(0, 0,  5),    },
	{ "MDP_WROT",                   M4U0_PORT_INIT(0, 0,  6),    },

	{ "VDEC_MC",                    M4U0_PORT_INIT(0, 1,  0),    },
	{ "VDEC_PP",                    M4U0_PORT_INIT(0, 1,  1),    },
	{ "VDEC_AVC_MV",                M4U0_PORT_INIT(0, 1,  2),    },
	{ "VDEC_PRED_RD",               M4U0_PORT_INIT(0, 1,  3),    },
	{ "VDEC_PRED_WR",               M4U0_PORT_INIT(0, 1,  4),    },
	{ "VDEC_VLD",                   M4U0_PORT_INIT(0, 1,  5),    },
	{ "VDEC_PPWRAP",                M4U0_PORT_INIT(0, 1,  6),    },

	{ "CAM_IMGO",                   M4U0_PORT_INIT(0, 2,  0),    },
	{ "CAM_RRZO",                   M4U0_PORT_INIT(0, 2,  1),    },
	{ "CAM_AAO",                    M4U0_PORT_INIT(0, 2,  2),    },
	{ "CAM_LCSO",                   M4U0_PORT_INIT(0, 2,  3),    },
	{ "CAM_ESFKO",                  M4U0_PORT_INIT(0, 2,  4),    },
	{ "CAM_IMGO_S",                 M4U0_PORT_INIT(0, 2,  5),    },
	{ "CAM_LSCI",                   M4U0_PORT_INIT(0, 2,  6),    },
	{ "CAM_LSCI_D",                 M4U0_PORT_INIT(0, 2,  7),    },
	{ "CAM_BPCI",                   M4U0_PORT_INIT(0, 2,  8),    },
	{ "CAM_BPCI_D",                 M4U0_PORT_INIT(0, 2,  9),    },
	{ "CAM_UFDI",                   M4U0_PORT_INIT(0, 2,  10),    },
	{ "CAM_IMGI",                   M4U0_PORT_INIT(0, 2,  11),    },
	{ "CAM_IMG2O",                  M4U0_PORT_INIT(0, 2,  12),    },
	{ "CAM_IMG3O",                  M4U0_PORT_INIT(0, 2,  13),    },
	{ "CAM_VIPI",                   M4U0_PORT_INIT(0, 2,  14),    },
	{ "CAM_VIP2I",                  M4U0_PORT_INIT(0, 2,  15),    },
	{ "CAM_VIP3I",                  M4U0_PORT_INIT(0, 2,  16),    },
	{ "CAM_LCEI",                   M4U0_PORT_INIT(0, 2,  17),    },
	{ "CAM_RB",                     M4U0_PORT_INIT(0, 2,  18),    },
	{ "CAM_RP",                     M4U0_PORT_INIT(0, 2,  19),    },
	{ "CAM_WR",                     M4U0_PORT_INIT(0, 2,  20),    },

	{ "VENC_RCPU",                  M4U0_PORT_INIT(0, 3,  0),    },
	{ "VENC_REC",                   M4U0_PORT_INIT(0, 3,  1),    },
	{ "VENC_BSDMA",                 M4U0_PORT_INIT(0, 3,  2),    },
	{ "VENC_SV_COMV",               M4U0_PORT_INIT(0, 3,  3),    },
	{ "VENC_RD_COMV",               M4U0_PORT_INIT(0, 3,  4),    },
	{ "JPGENC_RDMA",                M4U0_PORT_INIT(0, 3,  5),    },
	{ "JPGENC_BSDMA",               M4U0_PORT_INIT(0, 3,  6),    },
	{ "JPGDEC_WDMA",                M4U0_PORT_INIT(0, 3,  7),    },
	{ "JPGDEC_BSDMA",               M4U0_PORT_INIT(0, 3,  8),    },
	{ "VENC_CUR_LUMA",              M4U0_PORT_INIT(0, 3,  9),    },
	{ "VENC_CUR_CHROMA",            M4U0_PORT_INIT(0, 3,  10),    },
	{ "VENC_REF_LUMA",              M4U0_PORT_INIT(0, 3,  11),    },
	{ "VENC_REF_CHROMA",            M4U0_PORT_INIT(0, 3,  12),    },

	{ "UNKNOWN",                     M4U0_PORT_INIT(0, 4,  0),    },
};
