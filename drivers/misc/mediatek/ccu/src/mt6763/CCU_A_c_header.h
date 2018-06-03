#ifndef __CCU_A_REGS_H__
#define __CCU_A_REGS_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------- CCU_A Bit Field Definitions ------------------- */

#define PACKING
typedef unsigned int FIELD;

typedef PACKING union
{
	PACKING struct
	{
		FIELD RDMA_SOFT_RST_ST          : 1;
		FIELD WDMA_SOFT_RST_ST          : 1;
		FIELD rsv_2                     : 6;
		FIELD RDMA_SOFT_RST             : 1;
		FIELD WDMA_SOFT_RST             : 1;
		FIELD rsv_10                    : 6;
		FIELD CCU_HW_RST               : 1;
		FIELD ARBITER_HW_RST            : 1;
		FIELD S2T_A_HW_RST              : 1;
		FIELD T2S_A_HW_RST              : 1;
		FIELD T2S_B_HW_RST              : 1;
		FIELD RDMA_HW_RST               : 1;
		FIELD WDMA_HW_RST               : 1;
		FIELD AHB2GMC_HW_RST            : 1;
		FIELD rsv_24                    : 8;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_RESET, *PCCU_A_REG_RESET;

typedef PACKING union
{
	PACKING struct
	{
		FIELD rsv_0                     : 1;
		FIELD S2T_A_START               : 1;
		FIELD T2S_A_START               : 1;
		FIELD RDMA_START                : 1;
		FIELD WDMA_START                : 1;
		FIELD rsv_5                     : 27;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_START_TRIG, *PCCU_A_REG_START_TRIG;

typedef PACKING union
{
	PACKING struct
	{
		FIELD S2T_A_FINISH              : 1;
		FIELD T2S_A_FINISH              : 1;
		FIELD T2S_B_FINISH              : 1;
		FIELD rsv_3                     : 29;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_BANK_INCR, *PCCU_A_REG_BANK_INCR;

typedef PACKING union
{
	PACKING struct
	{
		FIELD RDMA_ENABLE               : 1;
		FIELD WDMA_ENABLE               : 1;
		FIELD T2S_B_ENABLE              : 1;
		FIELD CCU_PRINNER               : 1;
		FIELD DB_LOAD_DISABLE           : 1;
		FIELD CCU_DL_PATH_SEL           : 1;
		FIELD CCU_TILE_SOURCE           : 1;
		FIELD INT_CLR_MODE              : 1;
		FIELD H2G_GID                   : 2;
		FIELD H2G_GULTRA_ENABLE         : 1;
		FIELD H2G_EARLY_RESP            : 1;
		FIELD CCU_AHB_BASE             : 4;
		FIELD OCD_JTAG_BYPASS           : 1;
		FIELD DAP_DIS                   : 1;
		FIELD rsv_18                    : 2;
		FIELD CROP_EN                   : 1;
		FIELD CCUI_DCM_DIS              : 1;
		FIELD CCUO_DCM_DIS              : 1;
		FIELD SRAM_DCM_DIS              : 1;
		FIELD rsv_24                    : 8;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CTRL, *PCCU_A_REG_CTRL;

typedef PACKING union
{
	PACKING struct
	{
		FIELD T2S_A_BANK_DONE           : 1;
		FIELD T2S_A_FRAME_DONE          : 1;
		FIELD rsv_2                     : 30;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_T2S_A_DONE_ST, *PCCU_A_REG_T2S_A_DONE_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD T2S_B_BANK_DONE           : 1;
		FIELD T2S_B_FRAME_DONE          : 1;
		FIELD rsv_2                     : 30;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_T2S_B_DONE_ST, *PCCU_A_REG_T2S_B_DONE_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD S2T_A_BANK_DONE           : 1;
		FIELD S2T_A_FRAME_DONE          : 1;
		FIELD rsv_2                     : 30;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_S2T_A_DONE_ST, *PCCU_A_REG_S2T_A_DONE_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD WDMA_DONE_ST              : 1;
		FIELD rsv_1                     : 31;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_WDMA_DONE_ST, *PCCU_A_REG_WDMA_DONE_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_HALT                 : 1;
		FIELD CCU_GATED                : 1;
		FIELD rsv_2                     : 30;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_ST, *PCCU_A_REG_CCU_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INT_EN               : 8;
		FIELD rsv_8                     : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INT_EN, *PCCU_A_REG_CCU_INT_EN;

typedef PACKING union
{
	PACKING struct
	{
		FIELD INT_CCU                  : 1;
		FIELD rsv_1                     : 31;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INT, *PCCU_A_REG_CCU_INT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD INT_CTL_CCU              : 1;
		FIELD rsv_1                     : 31;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CTL_CCU_INT, *PCCU_A_REG_CTL_CCU_INT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCUI_DCM_ST               : 1;
		FIELD rsv_1                     : 3;
		FIELD CCUO_DCM_ST               : 1;
		FIELD rsv_5                     : 3;
		FIELD SRAM_DCM_ST               : 1;
		FIELD rsv_9                     : 23;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DCM_ST, *PCCU_A_REG_DCM_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD DMA_ERR_INT               : 1;
		FIELD rsv_1                     : 7;
		FIELD DMA_REQ_ST                : 4;
		FIELD rsv_12                    : 4;
		FIELD DMA_RDY_ST                : 1;
		FIELD rsv_17                    : 15;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DMA_ERR_ST, *PCCU_A_REG_DMA_ERR_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD DMA_DEBUG                 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DMA_DEBUG, *PCCU_A_REG_DMA_DEBUG;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_EINTC_MASK            : 8;
		FIELD rsv_8                     : 8;
		FIELD CCU_EINTC_MODE            : 1;
		FIELD rsv_17                    : 15;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_MASK, *PCCU_A_REG_EINTC_MASK;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_EINTC_CLR             : 8;
		FIELD rsv_8                     : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_CLR, *PCCU_A_REG_EINTC_CLR;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_EINTC_ST              : 8;
		FIELD rsv_8                     : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_ST, *PCCU_A_REG_EINTC_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_EINTC_RAW_ST          : 8;
		FIELD rsv_8                     : 8;
		FIELD CCU_EINTC_TRIG_ST         : 1;
		FIELD rsv_17                    : 15;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_MISC, *PCCU_A_REG_EINTC_MISC;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_CROP_X_SIZE           : 16;
		FIELD CCU_CROP_Y_SIZE           : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_SIZE, *PCCU_A_REG_CROP_SIZE;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_CROP_START_X          : 16;
		FIELD CCU_CROP_START_Y          : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_START, *PCCU_A_REG_CROP_START;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_CROP_END_X            : 16;
		FIELD CCU_CROP_END_Y            : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_END, *PCCU_A_REG_CROP_END;

typedef PACKING union
{
	PACKING struct
	{
		FIELD T2S_B_OVERFLOW            : 1;
		FIELD rsv_1                     : 3;
		FIELD T2S_A_INCF                : 1;
		FIELD S2T_A_INCF                : 1;
		FIELD T2S_B_INCF                : 1;
		FIELD rsv_7                     : 25;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_ERR_ST, *PCCU_A_REG_ERR_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_CCU_PC               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_PC, *PCCU_A_REG_CCU_PC;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_SPARE                 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_TOP_SPARE, *PCCU_A_REG_TOP_SPARE;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO0                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO00, *PCCU_A_REG_CCU_INFO00;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO1                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO01, *PCCU_A_REG_CCU_INFO01;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO2                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO02, *PCCU_A_REG_CCU_INFO02;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO3                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO03, *PCCU_A_REG_CCU_INFO03;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO4                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO04, *PCCU_A_REG_CCU_INFO04;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO5                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO05, *PCCU_A_REG_CCU_INFO05;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO6                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO06, *PCCU_A_REG_CCU_INFO06;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO7                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO07, *PCCU_A_REG_CCU_INFO07;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO8                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO08, *PCCU_A_REG_CCU_INFO08;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO9                : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO09, *PCCU_A_REG_CCU_INFO09;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO10               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO10, *PCCU_A_REG_CCU_INFO10;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO11               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO11, *PCCU_A_REG_CCU_INFO11;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO12               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO12, *PCCU_A_REG_CCU_INFO12;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO13               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO13, *PCCU_A_REG_CCU_INFO13;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO14               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO14, *PCCU_A_REG_CCU_INFO14;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO15               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO15, *PCCU_A_REG_CCU_INFO15;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO16               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO16, *PCCU_A_REG_CCU_INFO16;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO17               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO17, *PCCU_A_REG_CCU_INFO17;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO18               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO18, *PCCU_A_REG_CCU_INFO18;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO19               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO19, *PCCU_A_REG_CCU_INFO19;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO20               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO20, *PCCU_A_REG_CCU_INFO20;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO21               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO21, *PCCU_A_REG_CCU_INFO21;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO22               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO22, *PCCU_A_REG_CCU_INFO22;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO23               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO23, *PCCU_A_REG_CCU_INFO23;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO24               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO24, *PCCU_A_REG_CCU_INFO24;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO25               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO25, *PCCU_A_REG_CCU_INFO25;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO26               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO26, *PCCU_A_REG_CCU_INFO26;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO27               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO27, *PCCU_A_REG_CCU_INFO27;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO28               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO28, *PCCU_A_REG_CCU_INFO28;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO29               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO29, *PCCU_A_REG_CCU_INFO29;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO30               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO30, *PCCU_A_REG_CCU_INFO30;

typedef PACKING union
{
	PACKING struct
	{
		FIELD CCU_INFO31               : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO31, *PCCU_A_REG_CCU_INFO31;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AAO_A_INT_PRD             : 16;
		FIELD AAO_B_INT_PRD             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AAO_INT_PERIOD, *PCCU_A_REG_AAO_INT_PERIOD;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AAO_A_INT_CNT             : 16;
		FIELD AAO_B_INT_CNT             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AAO_INT_CNT, *PCCU_A_REG_AAO_INT_CNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AFO_A_INT_PRD             : 16;
		FIELD AFO_B_INT_PRD             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AFO_INT_PERIOD, *PCCU_A_REG_AFO_INT_PERIOD;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AFO_A_INT_CNT             : 16;
		FIELD AFO_B_INT_CNT             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AFO_INT_CNT, *PCCU_A_REG_AFO_INT_CNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD PSO_A_INT_PRD             : 16;
		FIELD PSO_B_INT_PRD             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_PSO_INT_PERIOD, *PCCU_A_REG_PSO_INT_PERIOD;

typedef PACKING union
{
	PACKING struct
	{
		FIELD PSO_A_INT_CNT             : 16;
		FIELD PSO_B_INT_CNT             : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_PSO_INT_CNT, *PCCU_A_REG_PSO_INT_CNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AAO_A_FRAME_BCNT          : 16;
		FIELD AAO_B_FRAME_BCNT          : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AAO_FRAME_BCNT, *PCCU_A_REG_AAO_FRAME_BCNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AAO_A_BANK_INT_ST         : 1;
		FIELD AAO_A_FRAME_INT_ST        : 1;
		FIELD AAO_B_BANK_INT_ST         : 1;
		FIELD AAO_B_FRAME_INT_ST        : 1;
		FIELD rsv_4                     : 28;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AAO_INT_ST, *PCCU_A_REG_AAO_INT_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AFO_A_FRAME_BCNT          : 16;
		FIELD AFO_B_FRAME_BCNT          : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AFO_FRAME_BCNT, *PCCU_A_REG_AFO_FRAME_BCNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD AFO_A_BANK_INT_ST         : 1;
		FIELD AFO_A_FRAME_INT_ST        : 1;
		FIELD AFO_B_BANK_INT_ST         : 1;
		FIELD AFO_B_FRAME_INT_ST        : 1;
		FIELD rsv_4                     : 28;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_AFO_INT_ST, *PCCU_A_REG_AFO_INT_ST;

typedef PACKING union
{
	PACKING struct
	{
		FIELD PSO_A_FRAME_BCNT          : 16;
		FIELD PSO_B_FRAME_BCNT          : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_PSO_FRAME_BCNT, *PCCU_A_REG_PSO_FRAME_BCNT;

typedef PACKING union
{
	PACKING struct
	{
		FIELD PSO_A_BANK_INT_ST         : 1;
		FIELD PSO_A_FRAME_INT_ST        : 1;
		FIELD PSO_B_BANK_INT_ST         : 1;
		FIELD PSO_B_FRAME_INT_ST        : 1;
		FIELD rsv_4                     : 28;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_PSO_INT_ST, *PCCU_A_REG_PSO_INT_ST;

/* ----------------- CCU_A  Grouping Definitions -------------------*/
/* ----------------- CCU_A Register Definition -------------------*/
typedef volatile PACKING struct
{
	CCU_A_REG_RESET                 RESET;            /* 1000*/
	CCU_A_REG_START_TRIG            START_TRIG;       /* 1004*/
	CCU_A_REG_BANK_INCR             BANK_INCR;        /* 1008*/
	CCU_A_REG_CTRL                  CTRL;             /* 100C*/
	CCU_A_REG_T2S_A_DONE_ST         T2S_A_DONE_ST;    /* 1010*/
	CCU_A_REG_T2S_B_DONE_ST         T2S_B_DONE_ST;    /* 1014*/
	CCU_A_REG_S2T_A_DONE_ST         S2T_A_DONE_ST;    /* 1018*/
	CCU_A_REG_WDMA_DONE_ST          WDMA_DONE_ST;     /* 101C*/
	UINT32                          rsv_1020[4];      /* 1020..102C*/
	CCU_A_REG_CCU_ST               DONE_ST;          /* 1030*/
	CCU_A_REG_CCU_INT_EN           CCU_INT_EN;      /* 1034*/
	CCU_A_REG_CCU_INT              CCU_INT;         /* 1038*/
	CCU_A_REG_CTL_CCU_INT          CTL_CCU_INT;     /* 103C*/
	CCU_A_REG_DCM_ST                DCM_ST;           /* 1040*/
	CCU_A_REG_DMA_ERR_ST            DMA_ERR_ST;       /* 1044*/
	CCU_A_REG_DMA_DEBUG             DMA_DEBUG;        /* 1048*/
	UINT32                          rsv_104C;         /* 104C*/
	CCU_A_REG_EINTC_MASK            EINTC_MASK;       /* 1050*/
	CCU_A_REG_EINTC_CLR             EINTC_CLR;        /* 1054*/
	CCU_A_REG_EINTC_ST              EINTC_ST;         /* 1058*/
	CCU_A_REG_EINTC_MISC            EINTC_MISC;       /* 105C*/
	CCU_A_REG_CROP_SIZE             CROP_SIZE;        /* 1060*/
	CCU_A_REG_CROP_START            CROP_START;       /* 1064*/
	CCU_A_REG_CROP_END              CROP_END;         /* 1068*/
	CCU_A_REG_ERR_ST                ERR_ST;           /* 106C*/
	CCU_A_REG_CCU_PC               CCU_PC;          /* 1070*/
	CCU_A_REG_TOP_SPARE             TOP_SPARE;        /* 1074*/
	UINT32                          rsv_1078[2];      /* 1078..107C*/
	CCU_A_REG_CCU_INFO00           CCU_INFO00;      /* 1080*/
	CCU_A_REG_CCU_INFO01           CCU_INFO01;      /* 1084*/
	CCU_A_REG_CCU_INFO02           CCU_INFO02;      /* 1088*/
	CCU_A_REG_CCU_INFO03           CCU_INFO03;      /* 108C*/
	CCU_A_REG_CCU_INFO04           CCU_INFO04;      /* 1090*/
	CCU_A_REG_CCU_INFO05           CCU_INFO05;      /* 1094*/
	CCU_A_REG_CCU_INFO06           CCU_INFO06;      /* 1098*/
	CCU_A_REG_CCU_INFO07           CCU_INFO07;      /* 109C*/
	CCU_A_REG_CCU_INFO08           CCU_INFO08;      /* 10A0*/
	CCU_A_REG_CCU_INFO09           CCU_INFO09;      /* 10A4*/
	CCU_A_REG_CCU_INFO10           CCU_INFO10;      /* 10A8*/
	CCU_A_REG_CCU_INFO11           CCU_INFO11;      /* 10AC*/
	CCU_A_REG_CCU_INFO12           CCU_INFO12;      /* 10B0*/
	CCU_A_REG_CCU_INFO13           CCU_INFO13;      /* 10B4*/
	CCU_A_REG_CCU_INFO14           CCU_INFO14;      /* 10B8*/
	CCU_A_REG_CCU_INFO15           CCU_INFO15;      /* 10BC*/
	CCU_A_REG_CCU_INFO16           CCU_INFO16;      /* 10C0*/
	CCU_A_REG_CCU_INFO17           CCU_INFO17;      /* 10C4*/
	CCU_A_REG_CCU_INFO18           CCU_INFO18;      /* 10C8*/
	CCU_A_REG_CCU_INFO19           CCU_INFO19;      /* 10CC*/
	CCU_A_REG_CCU_INFO20           CCU_INFO20;      /* 10D0*/
	CCU_A_REG_CCU_INFO21           CCU_INFO21;      /* 10D4*/
	CCU_A_REG_CCU_INFO22           CCU_INFO22;      /* 10D8*/
	CCU_A_REG_CCU_INFO23           CCU_INFO23;      /* 10DC*/
	CCU_A_REG_CCU_INFO24           CCU_INFO24;      /* 10E0*/
	CCU_A_REG_CCU_INFO25           CCU_INFO25;      /* 10E4*/
	CCU_A_REG_CCU_INFO26           CCU_INFO26;      /* 10E8*/
	CCU_A_REG_CCU_INFO27           CCU_INFO27;      /* 10EC*/
	CCU_A_REG_CCU_INFO28           CCU_INFO28;      /* 10F0*/
	CCU_A_REG_CCU_INFO29           CCU_INFO29;      /* 10F4*/
	CCU_A_REG_CCU_INFO30           CCU_INFO30;      /* 10F8*/
	CCU_A_REG_CCU_INFO31           CCU_INFO31;      /* 10FC*/
	CCU_A_REG_AAO_INT_PERIOD        AAO_INT_PERIOD;   /* 1100*/
	CCU_A_REG_AAO_INT_CNT           AAO_INT_CNT;      /* 1104*/
	CCU_A_REG_AFO_INT_PERIOD        AFO_INT_PERIOD;   /* 1108*/
	CCU_A_REG_AFO_INT_CNT           AFO_INT_CNT;      /* 110C*/
	CCU_A_REG_PSO_INT_PERIOD        PSO_INT_PERIOD;   /* 1110*/
	CCU_A_REG_PSO_INT_CNT           PSO_INT_CNT;      /* 1114*/
	UINT32                          rsv_1118[2];      /* 1118..111C*/
	CCU_A_REG_AAO_FRAME_BCNT        AAO_FRAME_BCNT;   /* 1120*/
	CCU_A_REG_AAO_INT_ST            AAO_INT_ST;       /* 1124*/
	CCU_A_REG_AFO_FRAME_BCNT        AFO_FRAME_BCNT;   /* 1128*/
	CCU_A_REG_AFO_INT_ST            AFO_INT_ST;       /* 112C*/
	CCU_A_REG_PSO_FRAME_BCNT        PSO_FRAME_BCNT;   /* 1130*/
	CCU_A_REG_PSO_INT_ST            PSO_INT_ST;       /* 1134*/
	UINT32                          rsv_1138[14257];  /* 1138..EFF8*/
	UINT8                           rsv_EFFC;         /* EFFC*/
	UINT16                          rsv_EFFD;         /* EFFD*/
	UINT8                           rsv_F0FF;         /* F0FF*/
	UINT32                          rsv_F100[959];    /* F100..FFF8*/
	UINT8                           rsv_FFFC;         /* FFFC*/
	UINT16                          rsv_FFFD;         /* FFFD*/
} CCU_A_REGS, *PCCU_A_REGS;

#ifdef __cplusplus
}
#endif

#endif /* __CCU_A_REGS_H__*/
