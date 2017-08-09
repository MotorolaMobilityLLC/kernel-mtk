#include "m4u_platform.h"
#include "m4u_priv.h"
#include "m4u_hw.h"

unsigned int gM4UTagCount[]  = {32};

const char *gM4U_SMILARB[] = {
	"mediatek,SMI_LARB0", "mediatek,SMI_LARB1", "mediatek,SMI_LARB2"};

M4U_RANGE_DES_T gM4u0_seq[M4U0_SEQ_NR] = {{0} };
M4U_RANGE_DES_T *gM4USeq[] = {gM4u0_seq};

#define M4U0_PORT_INIT(slave, larb, port)  (0, slave, larb, port, (((larb)<<7)|((port)<<2)), 1)

m4u_port_t gM4uPort[] = {
	{ "DISP_OVL0_PORT0",            M4U0_PORT_INIT(0, 0,  0),    },
	{ "DISP_OVL0_PORT1",            M4U0_PORT_INIT(0, 0,  1),    },
	{ "DISP_RDMA0",                 M4U0_PORT_INIT(0, 0,  2),    },
	{ "DISP_WDMA0",                 M4U0_PORT_INIT(0, 0,  3),    },
	{ "MDP_RDMA",                   M4U0_PORT_INIT(0, 0,  4),    },
	{ "MDP_WDMA",                   M4U0_PORT_INIT(0, 0,  5),    },
	{ "MDP_WROT",                   M4U0_PORT_INIT(0, 0,  6),    },
	{ "DISP_FAKE",                  M4U0_PORT_INIT(0, 0,  7),    },

	{ "VDEC_MC",                    M4U0_PORT_INIT(0, 1,  0),    },
	{ "VDEC_PP",                    M4U0_PORT_INIT(0, 1,  1),    },
	{ "VDEC_AVC_MV",                M4U0_PORT_INIT(0, 1,  2),    },
	{ "VDEC_PRED_RD",               M4U0_PORT_INIT(0, 1,  3),    },
	{ "VDEC_PRED_WR",               M4U0_PORT_INIT(0, 1,  4),    },
	{ "VDEC_VLD",                   M4U0_PORT_INIT(0, 1,  5),    },
	{ "VDEC_PPWRAP",                M4U0_PORT_INIT(0, 1,  6),    },

	{ "IMGO",                       M4U0_PORT_INIT(0, 2,  0),    },
	{ "IMGO2O",                     M4U0_PORT_INIT(0, 2,  1),    },
	{ "LSCI",                       M4U0_PORT_INIT(0, 2,  2),    },
	{ "VENC_BSDMA_VDEC_POST0",      M4U0_PORT_INIT(0, 2,  3),    },
	{ "JPGENC_RDMA",                M4U0_PORT_INIT(0, 2,  4),    },
	{ "CAM_IMGI",                   M4U0_PORT_INIT(0, 2,  5),    },
	{ "CAM_ESFKO",                  M4U0_PORT_INIT(0, 2,  6),    },
	{ "CAM_AAO",                    M4U0_PORT_INIT(0, 2,  7),    },
	{ "JPGENC_BSDMA",               M4U0_PORT_INIT(0, 2,  8),    },
	{ "VENC_MVQP",                  M4U0_PORT_INIT(0, 2,  9),    },
	{ "VENC_MC",                    M4U0_PORT_INIT(0, 2,  10),    },
	{ "VENC_CDMA_VDEC_CDMA",        M4U0_PORT_INIT(0, 2,  11),    },
	{ "VENC_REC_VDEC_WDMA",         M4U0_PORT_INIT(0, 2,  12),    },

	{ "UNKNOWN",                     M4U0_PORT_INIT(0, 4,  0),    },
};
