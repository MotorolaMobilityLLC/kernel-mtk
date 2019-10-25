/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __CCU_A_REGS_H__
#define __CCU_A_REGS_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REG_BASE_C_MODULE
/* ----------------- CCU_A Bit Field Definitions -------------------*/

#define PACKING
typedef unsigned int FIELD;

typedef PACKING union {
	PACKING struct {
		FIELD RDMA_SOFT_RST_ST : 1;
		FIELD WDMA_SOFT_RST_ST : 1;
		FIELD rsv_2 : 6;
		FIELD RDMA_SOFT_RST : 1;
		FIELD WDMA_SOFT_RST : 1;
		FIELD rsv_10 : 6;
		FIELD CCU_HW_RST : 1;
		FIELD ARBITER_HW_RST : 1;
		FIELD S2T_A_HW_RST : 1;
		FIELD T2S_A_HW_RST : 1;
		FIELD S2T_B_HW_RST : 1;
		FIELD T2S_B_HW_RST : 1;
		FIELD RDMA_HW_RST : 1;
		FIELD WDMA_HW_RST : 1;
		FIELD AHB2GMC_HW_RST : 1;
		FIELD rsv_25 : 7;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_RESET, *PCCU_A_REG_RESET;

typedef PACKING union {
	PACKING struct {
		FIELD rsv_0 : 1;
		FIELD S2T_A_START : 1;
		FIELD T2S_A_START : 1;
		FIELD RDMA_START : 1;
		FIELD WDMA_START : 1;
		FIELD rsv_5 : 27;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_START_TRIG, *PCCU_A_REG_START_TRIG;

typedef PACKING union {
	PACKING struct {
		FIELD S2T_A_FINISH : 1;
		FIELD T2S_A_FINISH : 1;
		FIELD S2T_B_FINISH : 1;
		FIELD T2S_B_FINISH : 1;
		FIELD rsv_4 : 28;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_BANK_INCR, *PCCU_A_REG_BANK_INCR;

typedef PACKING union {
	PACKING struct {
		FIELD S2T_B_BANK_FULL : 1;
		FIELD T2S_B_BANK_FULL : 1;
		FIELD rsv_2 : 6;
		FIELD S2T_B_BANK_ID : 3;
		FIELD T2S_B_BANK_ID : 3;
		FIELD rsv_14 : 2;
		FIELD CCU_HALT : 1;
		FIELD CCU_GATED : 1;
		FIELD rsv_18 : 6;
		FIELD T2S_A_BANK_DONE : 1;
		FIELD T2S_A_FRAME_DONE : 1;
		FIELD S2T_A_BANK_DONE : 1;
		FIELD S2T_A_FRAME_DONE : 1;
		FIELD T2S_B_BANK_DONE : 1;
		FIELD T2S_B_FRAME_DONE : 1;
		FIELD S2T_B_BANK_DONE : 1;
		FIELD S2T_B_FRAME_DONE : 1;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DONE_ST, *PCCU_A_REG_DONE_ST;

typedef PACKING union {
	PACKING struct {
		FIELD RDMA_ENABLE : 1;
		FIELD WDMA_ENABLE : 1;
		FIELD S2T_A_ENABLE : 1;
		FIELD T2S_A_ENABLE : 1;
		FIELD S2T_B_ENABLE : 1;
		FIELD T2S_B_ENABLE : 1;
		FIELD CCU_PRINNER : 1;
		FIELD CCU_TILE_SOURCE : 1;
		FIELD H2G_GID : 2;
		FIELD H2G_GULTRA_ENABLE : 1;
		FIELD H2G_EARLY_RESP : 1;
		FIELD rsv_12 : 4;
		FIELD CCU_AHB_BASE : 4;
		FIELD OCD_JTAG_BYPASS : 1;
		FIELD rsv_21 : 3;
		FIELD OCD_EN : 1;
		FIELD rsv_25 : 3;
		FIELD DB_LOAD_DISABLE : 1;
		FIELD STR_SEL : 1;
		FIELD DL_PATH_SEL : 1;
		FIELD CROP_EN : 1;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CTRL, *PCCU_A_REG_CTRL;

typedef PACKING union {
	PACKING struct {
		FIELD INT_CCU : 1;
		FIELD rsv_1 : 31;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INT, *PCCU_A_REG_CCU_INT;

typedef PACKING union {
	PACKING struct {
		FIELD INT_CTL_CCU : 1;
		FIELD rsv_1 : 31;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CTL_CCU_INT, *PCCU_A_REG_CTL_CCU_INT;

typedef PACKING union {
	PACKING struct {
		FIELD CCUI_DCM_DIS : 1;
		FIELD rsv_1 : 3;
		FIELD CCUO_DCM_DIS : 1;
		FIELD rsv_5 : 3;
		FIELD SRAM_DCM_DIS : 1;
		FIELD rsv_9 : 23;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DCM_DIS, *PCCU_A_REG_DCM_DIS;

typedef PACKING union {
	PACKING struct {
		FIELD CCUI_DCM_ST : 1;
		FIELD rsv_1 : 3;
		FIELD CCUO_DCM_ST : 1;
		FIELD rsv_5 : 3;
		FIELD SRAM_DCM_ST : 1;
		FIELD rsv_9 : 23;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DCM_ST, *PCCU_A_REG_DCM_ST;

typedef PACKING union {
	PACKING struct {
		FIELD DMA_ERR_INT : 1;
		FIELD rsv_1 : 3;
		FIELD DMA_REQ_ST : 1;
		FIELD DMA_RDY_ST : 1;
		FIELD rsv_6 : 26;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DMA_ERR_ST, *PCCU_A_REG_DMA_ERR_ST;

typedef PACKING union {
	PACKING struct {
		FIELD DMA_DEBUG : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DMA_DEBUG, *PCCU_A_REG_DMA_DEBUG;

typedef PACKING union {
	PACKING struct {
		FIELD ISP_INT_MASK : 1;
		FIELD rsv_1 : 3;
		FIELD RDMA_INT_MASK : 1;
		FIELD rsv_5 : 3;
		FIELD WDMA_INT_MASK : 1;
		FIELD rsv_9 : 3;
		FIELD S2T_BANK_INT_MASK : 1;
		FIELD rsv_13 : 3;
		FIELD T2S_BANK_INT_MASK : 1;
		FIELD rsv_17 : 3;
		FIELD S2T_TILE_INT_MASK : 1;
		FIELD rsv_21 : 3;
		FIELD T2S_TILE_INT_MASK : 1;
		FIELD rsv_25 : 3;
		FIELD APMCU_INT_MASK : 1;
		FIELD rsv_29 : 3;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_INT_MASK, *PCCU_A_REG_INT_MASK;

typedef PACKING union {
	PACKING struct {
		FIELD A2S_EN : 1;
		FIELD rsv_1 : 3;
		FIELD S2T_A_EN : 1;
		FIELD rsv_5 : 3;
		FIELD T2S_A_EN : 1;
		FIELD rsv_9 : 3;
		FIELD CCU_EN : 1;
		FIELD rsv_13 : 19;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_PARB_ENABLE, *PCCU_A_REG_PARB_ENABLE;

typedef PACKING union {
	PACKING struct {
		FIELD A2S_EN : 1;
		FIELD rsv_1 : 3;
		FIELD S2T_A_EN : 1;
		FIELD rsv_5 : 3;
		FIELD T2S_A_EN : 1;
		FIELD rsv_9 : 3;
		FIELD S2T_B_EN : 1;
		FIELD rsv_13 : 3;
		FIELD T2S_B_EN : 1;
		FIELD rsv_17 : 3;
		FIELD CCU_EN : 1;
		FIELD rsv_21 : 11;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DARB_ENABLE, *PCCU_A_REG_DARB_ENABLE;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_SPARE : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_TOP_SPARE, *PCCU_A_REG_TOP_SPARE;

typedef PACKING union {
	PACKING struct {
		FIELD RDMA_DONE_ST : 1;
		FIELD WDMA_DONE_ST : 1;
		FIELD rsv_2 : 30;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_DMA_ST, *PCCU_A_REG_DMA_ST;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_EINTC_MASK : 8;
		FIELD rsv_8 : 8;
		FIELD CCU_EINTC_MODE : 1;
		FIELD rsv_17 : 15;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_MASK, *PCCU_A_REG_EINTC_MASK;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_EINTC_CLR : 8;
		FIELD rsv_8 : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_CLR, *PCCU_A_REG_EINTC_CLR;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_EINTC_ST : 8;
		FIELD rsv_8 : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_ST, *PCCU_A_REG_EINTC_ST;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_EINTC_RAW_ST : 8;
		FIELD rsv_8 : 8;
		FIELD CCU_EINTC_TRIG_ST : 1;
		FIELD rsv_17 : 15;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_EINTC_MISC, *PCCU_A_REG_EINTC_MISC;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_CROP_X_SIZE : 16;
		FIELD CCU_CROP_Y_SIZE : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_SIZE, *PCCU_A_REG_CROP_SIZE;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_CROP_START_X : 16;
		FIELD CCU_CROP_START_Y : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_START, *PCCU_A_REG_CROP_START;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_CROP_END_X : 16;
		FIELD CCU_CROP_END_Y : 16;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CROP_END, *PCCU_A_REG_CROP_END;

typedef PACKING union {
	PACKING struct {
		FIELD T2S_B_OVERFLOW : 1;
		FIELD rsv_1 : 3;
		FIELD T2S_A_INCF : 1;
		FIELD S2T_A_INCF : 1;
		FIELD T2S_B_INCF : 1;
		FIELD S2T_B_INCF : 1;
		FIELD rsv_8 : 24;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_ERR_ST, *PCCU_A_REG_ERR_ST;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO0 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO00, *PCCU_A_REG_CCU_INFO00;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO1 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO01, *PCCU_A_REG_CCU_INFO01;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO2 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO02, *PCCU_A_REG_CCU_INFO02;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO3 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO03, *PCCU_A_REG_CCU_INFO03;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO4 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO04, *PCCU_A_REG_CCU_INFO04;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO5 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO05, *PCCU_A_REG_CCU_INFO05;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO6 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO06, *PCCU_A_REG_CCU_INFO06;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO7 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO07, *PCCU_A_REG_CCU_INFO07;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO8 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO08, *PCCU_A_REG_CCU_INFO08;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO9 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO09, *PCCU_A_REG_CCU_INFO09;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO10 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO10, *PCCU_A_REG_CCU_INFO10;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO11 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO11, *PCCU_A_REG_CCU_INFO11;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO12 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO12, *PCCU_A_REG_CCU_INFO12;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO13 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO13, *PCCU_A_REG_CCU_INFO13;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO14 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO14, *PCCU_A_REG_CCU_INFO14;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO15 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO15, *PCCU_A_REG_CCU_INFO15;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO16 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO16, *PCCU_A_REG_CCU_INFO16;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO17 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO17, *PCCU_A_REG_CCU_INFO17;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO18 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO18, *PCCU_A_REG_CCU_INFO18;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO19 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO19, *PCCU_A_REG_CCU_INFO19;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO20 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO20, *PCCU_A_REG_CCU_INFO20;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO21 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO21, *PCCU_A_REG_CCU_INFO21;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO22 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO22, *PCCU_A_REG_CCU_INFO22;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO23 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO23, *PCCU_A_REG_CCU_INFO23;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO24 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO24, *PCCU_A_REG_CCU_INFO24;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO25 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO25, *PCCU_A_REG_CCU_INFO25;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO26 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO26, *PCCU_A_REG_CCU_INFO26;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO27 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO27, *PCCU_A_REG_CCU_INFO27;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO28 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO28, *PCCU_A_REG_CCU_INFO28;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO29 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO29, *PCCU_A_REG_CCU_INFO29;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO30 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO30, *PCCU_A_REG_CCU_INFO30;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_INFO31 : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_INFO31, *PCCU_A_REG_CCU_INFO31;

typedef PACKING union {
	PACKING struct {
		FIELD CCU_CCU_PC : 32;
	} Bits;
	UINT32 Raw;
} CCU_A_REG_CCU_PC, *PCCU_A_REG_CCU_PC;

/* ----------------- CCU_A  Grouping Definitions -------------------*/
/* ----------------- CCU_A Register Definition -------------------*/
typedef volatile PACKING struct {
	CCU_A_REG_RESET RESET;		   /* 7400*/
	CCU_A_REG_START_TRIG START_TRIG;   /* 7404*/
	CCU_A_REG_BANK_INCR BANK_INCR;     /* 7408*/
	CCU_A_REG_DONE_ST DONE_ST;	 /* 740C*/
	CCU_A_REG_CTRL CTRL;		   /* 7410*/
	CCU_A_REG_CCU_INT CCU_INT;	 /* 7414*/
	CCU_A_REG_CTL_CCU_INT CTL_CCU_INT; /* 7418*/
	CCU_A_REG_DCM_DIS DCM_DIS;	 /* 741C*/
	CCU_A_REG_DCM_ST DCM_ST;	   /* 7420*/
	CCU_A_REG_DMA_ERR_ST DMA_ERR_ST;   /* 7424*/
	CCU_A_REG_DMA_DEBUG DMA_DEBUG;     /* 7428*/
	CCU_A_REG_INT_MASK INT_MASK;       /* 742C*/
	CCU_A_REG_PARB_ENABLE PARB_ENABLE; /* 7430*/
	CCU_A_REG_DARB_ENABLE DARB_ENABLE; /* 7434*/
	CCU_A_REG_TOP_SPARE TOP_SPARE;     /* 7438*/
	CCU_A_REG_DMA_ST DMA_ST;	   /* 743C*/
	CCU_A_REG_EINTC_MASK EINTC_MASK;   /* 7440*/
	CCU_A_REG_EINTC_CLR EINTC_CLR;     /* 7444*/
	CCU_A_REG_EINTC_ST EINTC_ST;       /* 7448*/
	CCU_A_REG_EINTC_MISC EINTC_MISC;   /* 744C*/
	CCU_A_REG_CROP_SIZE CROP_SIZE;     /* 7450*/
	CCU_A_REG_CROP_START CROP_START;   /* 7454*/
	CCU_A_REG_CROP_END CROP_END;       /* 7458*/
	CCU_A_REG_ERR_ST ERR_ST;	   /* 745C*/
	CCU_A_REG_CCU_INFO00 CCU_INFO00;   /* 7460*/
	CCU_A_REG_CCU_INFO01 CCU_INFO01;   /* 7464*/
	CCU_A_REG_CCU_INFO02 CCU_INFO02;   /* 7468*/
	CCU_A_REG_CCU_INFO03 CCU_INFO03;   /* 746C*/
	CCU_A_REG_CCU_INFO04 CCU_INFO04;   /* 7470*/
	CCU_A_REG_CCU_INFO05 CCU_INFO05;   /* 7474*/
	CCU_A_REG_CCU_INFO06 CCU_INFO06;   /* 7478*/
	CCU_A_REG_CCU_INFO07 CCU_INFO07;   /* 747C*/
	CCU_A_REG_CCU_INFO08 CCU_INFO08;   /* 7480*/
	CCU_A_REG_CCU_INFO09 CCU_INFO09;   /* 7484*/
	CCU_A_REG_CCU_INFO10 CCU_INFO10;   /* 7488*/
	CCU_A_REG_CCU_INFO11 CCU_INFO11;   /* 748C*/
	CCU_A_REG_CCU_INFO12 CCU_INFO12;   /* 7490*/
	CCU_A_REG_CCU_INFO13 CCU_INFO13;   /* 7494*/
	CCU_A_REG_CCU_INFO14 CCU_INFO14;   /* 7498*/
	CCU_A_REG_CCU_INFO15 CCU_INFO15;   /* 749C*/
	CCU_A_REG_CCU_INFO16 CCU_INFO16;   /* 74A0*/
	CCU_A_REG_CCU_INFO17 CCU_INFO17;   /* 74A4*/
	CCU_A_REG_CCU_INFO18 CCU_INFO18;   /* 74A8*/
	CCU_A_REG_CCU_INFO19 CCU_INFO19;   /* 74AC*/
	CCU_A_REG_CCU_INFO20 CCU_INFO20;   /* 74B0*/
	CCU_A_REG_CCU_INFO21 CCU_INFO21;   /* 74B4*/
	CCU_A_REG_CCU_INFO22 CCU_INFO22;   /* 74B8*/
	CCU_A_REG_CCU_INFO23 CCU_INFO23;   /* 74BC*/
	CCU_A_REG_CCU_INFO24 CCU_INFO24;   /* 74C0*/
	CCU_A_REG_CCU_INFO25 CCU_INFO25;   /* 74C4*/
	CCU_A_REG_CCU_INFO26 CCU_INFO26;   /* 74C8*/
	CCU_A_REG_CCU_INFO27 CCU_INFO27;   /* 74CC*/
	CCU_A_REG_CCU_INFO28 CCU_INFO28;   /* 74D0*/
	CCU_A_REG_CCU_INFO29 CCU_INFO29;   /* 74D4*/
	CCU_A_REG_CCU_INFO30 CCU_INFO30;   /* 74D8*/
	CCU_A_REG_CCU_INFO31 CCU_INFO31;   /* 74DC*/
	CCU_A_REG_CCU_PC CCU_PC;	   /* 74E0*/
} CCU_A_REGS, *PCCU_A_REGS;

/* ---------- CCU_A Enum Definitions      ----------*/
/* ---------- CCU_A C Macro Definitions   ----------*/
/*extern PCCU_A_REGS g_CCU_A_BASE;*/

#define CCU_A_BASE (g_CCU_A_BASE)

#define CCU_A_RESET INREG32(&CCU_A_BASE->RESET)		    /* 7400*/
#define CCU_A_START_TRIG INREG32(&CCU_A_BASE->START_TRIG)   /* 7404*/
#define CCU_A_BANK_INCR INREG32(&CCU_A_BASE->BANK_INCR)     /* 7408*/
#define CCU_A_DONE_ST INREG32(&CCU_A_BASE->DONE_ST)	 /* 740C*/
#define CCU_A_CTRL INREG32(&CCU_A_BASE->CTRL)		    /* 7410*/
#define CCU_A_CCU_INT INREG32(&CCU_A_BASE->CCU_INT)	 /* 7414*/
#define CCU_A_CTL_CCU_INT INREG32(&CCU_A_BASE->CTL_CCU_INT) /* 7418*/
#define CCU_A_DCM_DIS INREG32(&CCU_A_BASE->DCM_DIS)	 /* 741C*/
#define CCU_A_DCM_ST INREG32(&CCU_A_BASE->DCM_ST)	   /* 7420*/
#define CCU_A_DMA_ERR_ST INREG32(&CCU_A_BASE->DMA_ERR_ST)   /* 7424*/
#define CCU_A_DMA_DEBUG INREG32(&CCU_A_BASE->DMA_DEBUG)     /* 7428*/
#define CCU_A_INT_MASK INREG32(&CCU_A_BASE->INT_MASK)       /* 742C*/
#define CCU_A_PARB_ENABLE INREG32(&CCU_A_BASE->PARB_ENABLE) /* 7430*/
#define CCU_A_DARB_ENABLE INREG32(&CCU_A_BASE->DARB_ENABLE) /* 7434*/
#define CCU_A_TOP_SPARE INREG32(&CCU_A_BASE->TOP_SPARE)     /* 7438*/
#define CCU_A_DMA_ST INREG32(&CCU_A_BASE->DMA_ST)	   /* 743C*/
#define CCU_A_EINTC_MASK INREG32(&CCU_A_BASE->EINTC_MASK)   /* 7440*/
#define CCU_A_EINTC_CLR INREG32(&CCU_A_BASE->EINTC_CLR)     /* 7444*/
#define CCU_A_EINTC_ST INREG32(&CCU_A_BASE->EINTC_ST)       /* 7448*/
#define CCU_A_EINTC_MISC INREG32(&CCU_A_BASE->EINTC_MISC)   /* 744C*/
#define CCU_A_CROP_SIZE INREG32(&CCU_A_BASE->CROP_SIZE)     /* 7450*/
#define CCU_A_CROP_START INREG32(&CCU_A_BASE->CROP_START)   /* 7454*/
#define CCU_A_CROP_END INREG32(&CCU_A_BASE->CROP_END)       /* 7458*/
#define CCU_A_ERR_ST INREG32(&CCU_A_BASE->ERR_ST)	   /* 745C*/
#define CCU_A_CCU_INFO00 INREG32(&CCU_A_BASE->CCU_INFO00)   /* 7460*/
#define CCU_A_CCU_INFO01 INREG32(&CCU_A_BASE->CCU_INFO01)   /* 7464*/
#define CCU_A_CCU_INFO02 INREG32(&CCU_A_BASE->CCU_INFO02)   /* 7468*/
#define CCU_A_CCU_INFO03 INREG32(&CCU_A_BASE->CCU_INFO03)   /* 746C*/
#define CCU_A_CCU_INFO04 INREG32(&CCU_A_BASE->CCU_INFO04)   /* 7470*/
#define CCU_A_CCU_INFO05 INREG32(&CCU_A_BASE->CCU_INFO05)   /* 7474*/
#define CCU_A_CCU_INFO06 INREG32(&CCU_A_BASE->CCU_INFO06)   /* 7478*/
#define CCU_A_CCU_INFO07 INREG32(&CCU_A_BASE->CCU_INFO07)   /* 747C*/
#define CCU_A_CCU_INFO08 INREG32(&CCU_A_BASE->CCU_INFO08)   /* 7480*/
#define CCU_A_CCU_INFO09 INREG32(&CCU_A_BASE->CCU_INFO09)   /* 7484*/
#define CCU_A_CCU_INFO10 INREG32(&CCU_A_BASE->CCU_INFO10)   /* 7488*/
#define CCU_A_CCU_INFO11 INREG32(&CCU_A_BASE->CCU_INFO11)   /* 748C*/
#define CCU_A_CCU_INFO12 INREG32(&CCU_A_BASE->CCU_INFO12)   /* 7490*/
#define CCU_A_CCU_INFO13 INREG32(&CCU_A_BASE->CCU_INFO13)   /* 7494*/
#define CCU_A_CCU_INFO14 INREG32(&CCU_A_BASE->CCU_INFO14)   /* 7498*/
#define CCU_A_CCU_INFO15 INREG32(&CCU_A_BASE->CCU_INFO15)   /* 749C*/
#define CCU_A_CCU_INFO16 INREG32(&CCU_A_BASE->CCU_INFO16)   /* 74A0*/
#define CCU_A_CCU_INFO17 INREG32(&CCU_A_BASE->CCU_INFO17)   /* 74A4*/
#define CCU_A_CCU_INFO18 INREG32(&CCU_A_BASE->CCU_INFO18)   /* 74A8*/
#define CCU_A_CCU_INFO19 INREG32(&CCU_A_BASE->CCU_INFO19)   /* 74AC*/
#define CCU_A_CCU_INFO20 INREG32(&CCU_A_BASE->CCU_INFO20)   /* 74B0*/
#define CCU_A_CCU_INFO21 INREG32(&CCU_A_BASE->CCU_INFO21)   /* 74B4*/
#define CCU_A_CCU_INFO22 INREG32(&CCU_A_BASE->CCU_INFO22)   /* 74B8*/
#define CCU_A_CCU_INFO23 INREG32(&CCU_A_BASE->CCU_INFO23)   /* 74BC*/
#define CCU_A_CCU_INFO24 INREG32(&CCU_A_BASE->CCU_INFO24)   /* 74C0*/
#define CCU_A_CCU_INFO25 INREG32(&CCU_A_BASE->CCU_INFO25)   /* 74C4*/
#define CCU_A_CCU_INFO26 INREG32(&CCU_A_BASE->CCU_INFO26)   /* 74C8*/
#define CCU_A_CCU_INFO27 INREG32(&CCU_A_BASE->CCU_INFO27)   /* 74CC*/
#define CCU_A_CCU_INFO28 INREG32(&CCU_A_BASE->CCU_INFO28)   /* 74D0*/
#define CCU_A_CCU_INFO29 INREG32(&CCU_A_BASE->CCU_INFO29)   /* 74D4*/
#define CCU_A_CCU_INFO30 INREG32(&CCU_A_BASE->CCU_INFO30)   /* 74D8*/
#define CCU_A_CCU_INFO31 INREG32(&CCU_A_BASE->CCU_INFO31)   /* 74DC*/
#define CCU_A_CCU_PC INREG32(&CCU_A_BASE->CCU_PC)	   /* 74E0*/

#endif

#define RESET_FLD_AHB2GMC_HW_RST REG_FLD(1, 24)
#define RESET_FLD_WDMA_HW_RST REG_FLD(1, 23)
#define RESET_FLD_RDMA_HW_RST REG_FLD(1, 22)
#define RESET_FLD_T2S_B_HW_RST REG_FLD(1, 21)
#define RESET_FLD_S2T_B_HW_RST REG_FLD(1, 20)
#define RESET_FLD_T2S_A_HW_RST REG_FLD(1, 19)
#define RESET_FLD_S2T_A_HW_RST REG_FLD(1, 18)
#define RESET_FLD_ARBITER_HW_RST REG_FLD(1, 17)
#define RESET_FLD_CCU_HW_RST REG_FLD(1, 16)
#define RESET_FLD_WDMA_SOFT_RST REG_FLD(1, 9)
#define RESET_FLD_RDMA_SOFT_RST REG_FLD(1, 8)
#define RESET_FLD_WDMA_SOFT_RST_ST REG_FLD(1, 1)
#define RESET_FLD_RDMA_SOFT_RST_ST REG_FLD(1, 0)

#define START_TRIG_FLD_WDMA_START REG_FLD(1, 4)
#define START_TRIG_FLD_RDMA_START REG_FLD(1, 3)
#define START_TRIG_FLD_T2S_A_START REG_FLD(1, 2)
#define START_TRIG_FLD_S2T_A_START REG_FLD(1, 1)

#define BANK_INCR_FLD_T2S_B_FINISH REG_FLD(1, 3)
#define BANK_INCR_FLD_S2T_B_FINISH REG_FLD(1, 2)
#define BANK_INCR_FLD_T2S_A_FINISH REG_FLD(1, 1)
#define BANK_INCR_FLD_S2T_A_FINISH REG_FLD(1, 0)

#define DONE_ST_FLD_S2T_B_FRAME_DONE REG_FLD(1, 31)
#define DONE_ST_FLD_S2T_B_BANK_DONE REG_FLD(1, 30)
#define DONE_ST_FLD_T2S_B_FRAME_DONE REG_FLD(1, 29)
#define DONE_ST_FLD_T2S_B_BANK_DONE REG_FLD(1, 28)
#define DONE_ST_FLD_S2T_A_FRAME_DONE REG_FLD(1, 27)
#define DONE_ST_FLD_S2T_A_BANK_DONE REG_FLD(1, 26)
#define DONE_ST_FLD_T2S_A_FRAME_DONE REG_FLD(1, 25)
#define DONE_ST_FLD_T2S_A_BANK_DONE REG_FLD(1, 24)
#define DONE_ST_FLD_CCU_GATED REG_FLD(1, 17)
#define DONE_ST_FLD_CCU_HALT REG_FLD(1, 16)
#define DONE_ST_FLD_T2S_B_BANK_ID REG_FLD(3, 11)
#define DONE_ST_FLD_S2T_B_BANK_ID REG_FLD(3, 8)
#define DONE_ST_FLD_T2S_B_BANK_FULL REG_FLD(1, 1)
#define DONE_ST_FLD_S2T_B_BANK_FULL REG_FLD(1, 0)

#define CTRL_FLD_CROP_EN REG_FLD(1, 31)
#define CTRL_FLD_DL_PATH_SEL REG_FLD(1, 30)
#define CTRL_FLD_STR_SEL REG_FLD(1, 29)
#define CTRL_FLD_DB_LOAD_DISABLE REG_FLD(1, 28)
#define CTRL_FLD_OCD_EN REG_FLD(1, 24)
#define CTRL_FLD_OCD_JTAG_BYPASS REG_FLD(1, 20)
#define CTRL_FLD_CCU_AHB_BASE REG_FLD(4, 16)
#define CTRL_FLD_H2G_EARLY_RESP REG_FLD(1, 11)
#define CTRL_FLD_H2G_GULTRA_ENABLE REG_FLD(1, 10)
#define CTRL_FLD_H2G_GID REG_FLD(2, 8)
#define CTRL_FLD_CCU_TILE_SOURCE REG_FLD(1, 7)
#define CTRL_FLD_CCU_PRINNER REG_FLD(1, 6)
#define CTRL_FLD_T2S_B_ENABLE REG_FLD(1, 5)
#define CTRL_FLD_S2T_B_ENABLE REG_FLD(1, 4)
#define CTRL_FLD_T2S_A_ENABLE REG_FLD(1, 3)
#define CTRL_FLD_S2T_A_ENABLE REG_FLD(1, 2)
#define CTRL_FLD_WDMA_ENABLE REG_FLD(1, 1)
#define CTRL_FLD_RDMA_ENABLE REG_FLD(1, 0)

#define CCU_INT_FLD_INT_CCU REG_FLD(1, 0)

#define CTL_CCU_INT_FLD_INT_CTL_CCU REG_FLD(1, 0)

#define DCM_DIS_FLD_SRAM_DCM_DIS REG_FLD(1, 8)
#define DCM_DIS_FLD_CCUO_DCM_DIS REG_FLD(1, 4)
#define DCM_DIS_FLD_CCUI_DCM_DIS REG_FLD(1, 0)

#define DCM_ST_FLD_SRAM_DCM_ST REG_FLD(1, 8)
#define DCM_ST_FLD_CCUO_DCM_ST REG_FLD(1, 4)
#define DCM_ST_FLD_CCUI_DCM_ST REG_FLD(1, 0)

#define DMA_ERR_ST_FLD_DMA_RDY_ST REG_FLD(1, 5)
#define DMA_ERR_ST_FLD_DMA_REQ_ST REG_FLD(1, 4)
#define DMA_ERR_ST_FLD_DMA_ERR_INT REG_FLD(1, 0)

#define DMA_DEBUG_FLD_DMA_DEBUG REG_FLD(32, 0)

#define INT_MASK_FLD_APMCU_INT_MASK REG_FLD(1, 28)
#define INT_MASK_FLD_T2S_TILE_INT_MASK REG_FLD(1, 24)
#define INT_MASK_FLD_S2T_TILE_INT_MASK REG_FLD(1, 20)
#define INT_MASK_FLD_T2S_BANK_INT_MASK REG_FLD(1, 16)
#define INT_MASK_FLD_S2T_BANK_INT_MASK REG_FLD(1, 12)
#define INT_MASK_FLD_WDMA_INT_MASK REG_FLD(1, 8)
#define INT_MASK_FLD_RDMA_INT_MASK REG_FLD(1, 4)
#define INT_MASK_FLD_ISP_INT_MASK REG_FLD(1, 0)

#define PARB_ENABLE_FLD_CCU_EN REG_FLD(1, 12)
#define PARB_ENABLE_FLD_T2S_A_EN REG_FLD(1, 8)
#define PARB_ENABLE_FLD_S2T_A_EN REG_FLD(1, 4)
#define PARB_ENABLE_FLD_A2S_EN REG_FLD(1, 0)

#define DARB_ENABLE_FLD_CCU_EN REG_FLD(1, 20)
#define DARB_ENABLE_FLD_T2S_B_EN REG_FLD(1, 16)
#define DARB_ENABLE_FLD_S2T_B_EN REG_FLD(1, 12)
#define DARB_ENABLE_FLD_T2S_A_EN REG_FLD(1, 8)
#define DARB_ENABLE_FLD_S2T_A_EN REG_FLD(1, 4)
#define DARB_ENABLE_FLD_A2S_EN REG_FLD(1, 0)

#define TOP_SPARE_FLD_CCU_SPARE REG_FLD(32, 0)

#define DMA_ST_FLD_WDMA_DONE_ST REG_FLD(1, 1)
#define DMA_ST_FLD_RDMA_DONE_ST REG_FLD(1, 0)

#define EINTC_MASK_FLD_CCU_EINTC_MODE REG_FLD(1, 16)
#define EINTC_MASK_FLD_CCU_EINTC_MASK REG_FLD(8, 0)

#define EINTC_CLR_FLD_CCU_EINTC_CLR REG_FLD(8, 0)

#define EINTC_ST_FLD_CCU_EINTC_ST REG_FLD(8, 0)

#define EINTC_MISC_FLD_CCU_EINTC_TRIG_ST REG_FLD(1, 16)
#define EINTC_MISC_FLD_CCU_EINTC_RAW_ST REG_FLD(8, 0)

#define CROP_SIZE_FLD_CCU_CROP_Y_SIZE REG_FLD(16, 16)
#define CROP_SIZE_FLD_CCU_CROP_X_SIZE REG_FLD(16, 0)

#define CROP_START_FLD_CCU_CROP_START_Y REG_FLD(16, 16)
#define CROP_START_FLD_CCU_CROP_START_X REG_FLD(16, 0)

#define CROP_END_FLD_CCU_CROP_END_Y REG_FLD(16, 16)
#define CROP_END_FLD_CCU_CROP_END_X REG_FLD(16, 0)

#define ERR_ST_FLD_S2T_B_INCF REG_FLD(1, 7)
#define ERR_ST_FLD_T2S_B_INCF REG_FLD(1, 6)
#define ERR_ST_FLD_S2T_A_INCF REG_FLD(1, 5)
#define ERR_ST_FLD_T2S_A_INCF REG_FLD(1, 4)
#define ERR_ST_FLD_T2S_B_OVERFLOW REG_FLD(1, 0)

#define CCU_INFO00_FLD_CCU_INFO0 REG_FLD(32, 0)

#define CCU_INFO01_FLD_CCU_INFO1 REG_FLD(32, 0)

#define CCU_INFO02_FLD_CCU_INFO2 REG_FLD(32, 0)

#define CCU_INFO03_FLD_CCU_INFO3 REG_FLD(32, 0)

#define CCU_INFO04_FLD_CCU_INFO4 REG_FLD(32, 0)

#define CCU_INFO05_FLD_CCU_INFO5 REG_FLD(32, 0)

#define CCU_INFO06_FLD_CCU_INFO6 REG_FLD(32, 0)

#define CCU_INFO07_FLD_CCU_INFO7 REG_FLD(32, 0)

#define CCU_INFO08_FLD_CCU_INFO8 REG_FLD(32, 0)

#define CCU_INFO09_FLD_CCU_INFO9 REG_FLD(32, 0)

#define CCU_INFO10_FLD_CCU_INFO10 REG_FLD(32, 0)

#define CCU_INFO11_FLD_CCU_INFO11 REG_FLD(32, 0)

#define CCU_INFO12_FLD_CCU_INFO12 REG_FLD(32, 0)

#define CCU_INFO13_FLD_CCU_INFO13 REG_FLD(32, 0)

#define CCU_INFO14_FLD_CCU_INFO14 REG_FLD(32, 0)

#define CCU_INFO15_FLD_CCU_INFO15 REG_FLD(32, 0)

#define CCU_INFO16_FLD_CCU_INFO16 REG_FLD(32, 0)

#define CCU_INFO17_FLD_CCU_INFO17 REG_FLD(32, 0)

#define CCU_INFO18_FLD_CCU_INFO18 REG_FLD(32, 0)

#define CCU_INFO19_FLD_CCU_INFO19 REG_FLD(32, 0)

#define CCU_INFO20_FLD_CCU_INFO20 REG_FLD(32, 0)

#define CCU_INFO21_FLD_CCU_INFO21 REG_FLD(32, 0)

#define CCU_INFO22_FLD_CCU_INFO22 REG_FLD(32, 0)

#define CCU_INFO23_FLD_CCU_INFO23 REG_FLD(32, 0)

#define CCU_INFO24_FLD_CCU_INFO24 REG_FLD(32, 0)

#define CCU_INFO25_FLD_CCU_INFO25 REG_FLD(32, 0)

#define CCU_INFO26_FLD_CCU_INFO26 REG_FLD(32, 0)

#define CCU_INFO27_FLD_CCU_INFO27 REG_FLD(32, 0)

#define CCU_INFO28_FLD_CCU_INFO28 REG_FLD(32, 0)

#define CCU_INFO29_FLD_CCU_INFO29 REG_FLD(32, 0)

#define CCU_INFO30_FLD_CCU_INFO30 REG_FLD(32, 0)

#define CCU_INFO31_FLD_CCU_INFO31 REG_FLD(32, 0)

#define CCU_PC_FLD_CCU_CCU_PC REG_FLD(32, 0)

#define RESET_GET_AHB2GMC_HW_RST(reg32)                                        \
	REG_FLD_GET(RESET_FLD_AHB2GMC_HW_RST, (reg32))
#define RESET_GET_WDMA_HW_RST(reg32) REG_FLD_GET(RESET_FLD_WDMA_HW_RST, (reg32))
#define RESET_GET_RDMA_HW_RST(reg32) REG_FLD_GET(RESET_FLD_RDMA_HW_RST, (reg32))
#define RESET_GET_T2S_B_HW_RST(reg32)                                          \
	REG_FLD_GET(RESET_FLD_T2S_B_HW_RST, (reg32))
#define RESET_GET_S2T_B_HW_RST(reg32)                                          \
	REG_FLD_GET(RESET_FLD_S2T_B_HW_RST, (reg32))
#define RESET_GET_T2S_A_HW_RST(reg32)                                          \
	REG_FLD_GET(RESET_FLD_T2S_A_HW_RST, (reg32))
#define RESET_GET_S2T_A_HW_RST(reg32)                                          \
	REG_FLD_GET(RESET_FLD_S2T_A_HW_RST, (reg32))
#define RESET_GET_ARBITER_HW_RST(reg32)                                        \
	REG_FLD_GET(RESET_FLD_ARBITER_HW_RST, (reg32))
#define RESET_GET_CCU_HW_RST(reg32) REG_FLD_GET(RESET_FLD_CCU_HW_RST, (reg32))
#define RESET_GET_WDMA_SOFT_RST(reg32)                                         \
	REG_FLD_GET(RESET_FLD_WDMA_SOFT_RST, (reg32))
#define RESET_GET_RDMA_SOFT_RST(reg32)                                         \
	REG_FLD_GET(RESET_FLD_RDMA_SOFT_RST, (reg32))
#define RESET_GET_WDMA_SOFT_RST_ST(reg32)                                      \
	REG_FLD_GET(RESET_FLD_WDMA_SOFT_RST_ST, (reg32))
#define RESET_GET_RDMA_SOFT_RST_ST(reg32)                                      \
	REG_FLD_GET(RESET_FLD_RDMA_SOFT_RST_ST, (reg32))

#define START_TRIG_GET_WDMA_START(reg32)                                       \
	REG_FLD_GET(START_TRIG_FLD_WDMA_START, (reg32))
#define START_TRIG_GET_RDMA_START(reg32)                                       \
	REG_FLD_GET(START_TRIG_FLD_RDMA_START, (reg32))
#define START_TRIG_GET_T2S_A_START(reg32)                                      \
	REG_FLD_GET(START_TRIG_FLD_T2S_A_START, (reg32))
#define START_TRIG_GET_S2T_A_START(reg32)                                      \
	REG_FLD_GET(START_TRIG_FLD_S2T_A_START, (reg32))

#define BANK_INCR_GET_T2S_B_FINISH(reg32)                                      \
	REG_FLD_GET(BANK_INCR_FLD_T2S_B_FINISH, (reg32))
#define BANK_INCR_GET_S2T_B_FINISH(reg32)                                      \
	REG_FLD_GET(BANK_INCR_FLD_S2T_B_FINISH, (reg32))
#define BANK_INCR_GET_T2S_A_FINISH(reg32)                                      \
	REG_FLD_GET(BANK_INCR_FLD_T2S_A_FINISH, (reg32))
#define BANK_INCR_GET_S2T_A_FINISH(reg32)                                      \
	REG_FLD_GET(BANK_INCR_FLD_S2T_A_FINISH, (reg32))

#define DONE_ST_GET_S2T_B_FRAME_DONE(reg32)                                    \
	REG_FLD_GET(DONE_ST_FLD_S2T_B_FRAME_DONE, (reg32))
#define DONE_ST_GET_S2T_B_BANK_DONE(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_S2T_B_BANK_DONE, (reg32))
#define DONE_ST_GET_T2S_B_FRAME_DONE(reg32)                                    \
	REG_FLD_GET(DONE_ST_FLD_T2S_B_FRAME_DONE, (reg32))
#define DONE_ST_GET_T2S_B_BANK_DONE(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_T2S_B_BANK_DONE, (reg32))
#define DONE_ST_GET_S2T_A_FRAME_DONE(reg32)                                    \
	REG_FLD_GET(DONE_ST_FLD_S2T_A_FRAME_DONE, (reg32))
#define DONE_ST_GET_S2T_A_BANK_DONE(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_S2T_A_BANK_DONE, (reg32))
#define DONE_ST_GET_T2S_A_FRAME_DONE(reg32)                                    \
	REG_FLD_GET(DONE_ST_FLD_T2S_A_FRAME_DONE, (reg32))
#define DONE_ST_GET_T2S_A_BANK_DONE(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_T2S_A_BANK_DONE, (reg32))
#define DONE_ST_GET_CCU_GATED(reg32) REG_FLD_GET(DONE_ST_FLD_CCU_GATED, (reg32))
#define DONE_ST_GET_CCU_HALT(reg32) REG_FLD_GET(DONE_ST_FLD_CCU_HALT, (reg32))
#define DONE_ST_GET_T2S_B_BANK_ID(reg32)                                       \
	REG_FLD_GET(DONE_ST_FLD_T2S_B_BANK_ID, (reg32))
#define DONE_ST_GET_S2T_B_BANK_ID(reg32)                                       \
	REG_FLD_GET(DONE_ST_FLD_S2T_B_BANK_ID, (reg32))
#define DONE_ST_GET_T2S_B_BANK_FULL(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_T2S_B_BANK_FULL, (reg32))
#define DONE_ST_GET_S2T_B_BANK_FULL(reg32)                                     \
	REG_FLD_GET(DONE_ST_FLD_S2T_B_BANK_FULL, (reg32))

#define CTRL_GET_CROP_EN(reg32) REG_FLD_GET(CTRL_FLD_CROP_EN, (reg32))
#define CTRL_GET_DL_PATH_SEL(reg32) REG_FLD_GET(CTRL_FLD_DL_PATH_SEL, (reg32))
#define CTRL_GET_STR_SEL(reg32) REG_FLD_GET(CTRL_FLD_STR_SEL, (reg32))
#define CTRL_GET_DB_LOAD_DISABLE(reg32)                                        \
	REG_FLD_GET(CTRL_FLD_DB_LOAD_DISABLE, (reg32))
#define CTRL_GET_OCD_EN(reg32) REG_FLD_GET(CTRL_FLD_OCD_EN, (reg32))
#define CTRL_GET_OCD_JTAG_BYPASS(reg32)                                        \
	REG_FLD_GET(CTRL_FLD_OCD_JTAG_BYPASS, (reg32))
#define CTRL_GET_CCU_AHB_BASE(reg32) REG_FLD_GET(CTRL_FLD_CCU_AHB_BASE, (reg32))
#define CTRL_GET_H2G_EARLY_RESP(reg32)                                         \
	REG_FLD_GET(CTRL_FLD_H2G_EARLY_RESP, (reg32))
#define CTRL_GET_H2G_GULTRA_ENABLE(reg32)                                      \
	REG_FLD_GET(CTRL_FLD_H2G_GULTRA_ENABLE, (reg32))
#define CTRL_GET_H2G_GID(reg32) REG_FLD_GET(CTRL_FLD_H2G_GID, (reg32))
#define CTRL_GET_CCU_TILE_SOURCE(reg32)                                        \
	REG_FLD_GET(CTRL_FLD_CCU_TILE_SOURCE, (reg32))
#define CTRL_GET_CCU_PRINNER(reg32) REG_FLD_GET(CTRL_FLD_CCU_PRINNER, (reg32))
#define CTRL_GET_T2S_B_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_T2S_B_ENABLE, (reg32))
#define CTRL_GET_S2T_B_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_S2T_B_ENABLE, (reg32))
#define CTRL_GET_T2S_A_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_T2S_A_ENABLE, (reg32))
#define CTRL_GET_S2T_A_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_S2T_A_ENABLE, (reg32))
#define CTRL_GET_WDMA_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_WDMA_ENABLE, (reg32))
#define CTRL_GET_RDMA_ENABLE(reg32) REG_FLD_GET(CTRL_FLD_RDMA_ENABLE, (reg32))

#define CCU_INT_GET_INT_CCU(reg32) REG_FLD_GET(CCU_INT_FLD_INT_CCU, (reg32))

#define CTL_CCU_INT_GET_INT_CTL_CCU(reg32)                                     \
	REG_FLD_GET(CTL_CCU_INT_FLD_INT_CTL_CCU, (reg32))

#define DCM_DIS_GET_SRAM_DCM_DIS(reg32)                                        \
	REG_FLD_GET(DCM_DIS_FLD_SRAM_DCM_DIS, (reg32))
#define DCM_DIS_GET_CCUO_DCM_DIS(reg32)                                        \
	REG_FLD_GET(DCM_DIS_FLD_CCUO_DCM_DIS, (reg32))
#define DCM_DIS_GET_CCUI_DCM_DIS(reg32)                                        \
	REG_FLD_GET(DCM_DIS_FLD_CCUI_DCM_DIS, (reg32))

#define DCM_ST_GET_SRAM_DCM_ST(reg32)                                          \
	REG_FLD_GET(DCM_ST_FLD_SRAM_DCM_ST, (reg32))
#define DCM_ST_GET_CCUO_DCM_ST(reg32)                                          \
	REG_FLD_GET(DCM_ST_FLD_CCUO_DCM_ST, (reg32))
#define DCM_ST_GET_CCUI_DCM_ST(reg32)                                          \
	REG_FLD_GET(DCM_ST_FLD_CCUI_DCM_ST, (reg32))

#define DMA_ERR_ST_GET_DMA_RDY_ST(reg32)                                       \
	REG_FLD_GET(DMA_ERR_ST_FLD_DMA_RDY_ST, (reg32))
#define DMA_ERR_ST_GET_DMA_REQ_ST(reg32)                                       \
	REG_FLD_GET(DMA_ERR_ST_FLD_DMA_REQ_ST, (reg32))
#define DMA_ERR_ST_GET_DMA_ERR_INT(reg32)                                      \
	REG_FLD_GET(DMA_ERR_ST_FLD_DMA_ERR_INT, (reg32))

#define DMA_DEBUG_GET_DMA_DEBUG(reg32)                                         \
	REG_FLD_GET(DMA_DEBUG_FLD_DMA_DEBUG, (reg32))

#define INT_MASK_GET_APMCU_INT_MASK(reg32)                                     \
	REG_FLD_GET(INT_MASK_FLD_APMCU_INT_MASK, (reg32))
#define INT_MASK_GET_T2S_TILE_INT_MASK(reg32)                                  \
	REG_FLD_GET(INT_MASK_FLD_T2S_TILE_INT_MASK, (reg32))
#define INT_MASK_GET_S2T_TILE_INT_MASK(reg32)                                  \
	REG_FLD_GET(INT_MASK_FLD_S2T_TILE_INT_MASK, (reg32))
#define INT_MASK_GET_T2S_BANK_INT_MASK(reg32)                                  \
	REG_FLD_GET(INT_MASK_FLD_T2S_BANK_INT_MASK, (reg32))
#define INT_MASK_GET_S2T_BANK_INT_MASK(reg32)                                  \
	REG_FLD_GET(INT_MASK_FLD_S2T_BANK_INT_MASK, (reg32))
#define INT_MASK_GET_WDMA_INT_MASK(reg32)                                      \
	REG_FLD_GET(INT_MASK_FLD_WDMA_INT_MASK, (reg32))
#define INT_MASK_GET_RDMA_INT_MASK(reg32)                                      \
	REG_FLD_GET(INT_MASK_FLD_RDMA_INT_MASK, (reg32))
#define INT_MASK_GET_ISP_INT_MASK(reg32)                                       \
	REG_FLD_GET(INT_MASK_FLD_ISP_INT_MASK, (reg32))

#define PARB_ENABLE_GET_CCU_EN(reg32)                                          \
	REG_FLD_GET(PARB_ENABLE_FLD_CCU_EN, (reg32))
#define PARB_ENABLE_GET_T2S_A_EN(reg32)                                        \
	REG_FLD_GET(PARB_ENABLE_FLD_T2S_A_EN, (reg32))
#define PARB_ENABLE_GET_S2T_A_EN(reg32)                                        \
	REG_FLD_GET(PARB_ENABLE_FLD_S2T_A_EN, (reg32))
#define PARB_ENABLE_GET_A2S_EN(reg32)                                          \
	REG_FLD_GET(PARB_ENABLE_FLD_A2S_EN, (reg32))

#define DARB_ENABLE_GET_CCU_EN(reg32)                                          \
	REG_FLD_GET(DARB_ENABLE_FLD_CCU_EN, (reg32))
#define DARB_ENABLE_GET_T2S_B_EN(reg32)                                        \
	REG_FLD_GET(DARB_ENABLE_FLD_T2S_B_EN, (reg32))
#define DARB_ENABLE_GET_S2T_B_EN(reg32)                                        \
	REG_FLD_GET(DARB_ENABLE_FLD_S2T_B_EN, (reg32))
#define DARB_ENABLE_GET_T2S_A_EN(reg32)                                        \
	REG_FLD_GET(DARB_ENABLE_FLD_T2S_A_EN, (reg32))
#define DARB_ENABLE_GET_S2T_A_EN(reg32)                                        \
	REG_FLD_GET(DARB_ENABLE_FLD_S2T_A_EN, (reg32))
#define DARB_ENABLE_GET_A2S_EN(reg32)                                          \
	REG_FLD_GET(DARB_ENABLE_FLD_A2S_EN, (reg32))

#define TOP_SPARE_GET_CCU_SPARE(reg32)                                         \
	REG_FLD_GET(TOP_SPARE_FLD_CCU_SPARE, (reg32))

#define DMA_ST_GET_WDMA_DONE_ST(reg32)                                         \
	REG_FLD_GET(DMA_ST_FLD_WDMA_DONE_ST, (reg32))
#define DMA_ST_GET_RDMA_DONE_ST(reg32)                                         \
	REG_FLD_GET(DMA_ST_FLD_RDMA_DONE_ST, (reg32))

#define EINTC_MASK_GET_CCU_EINTC_MODE(reg32)                                   \
	REG_FLD_GET(EINTC_MASK_FLD_CCU_EINTC_MODE, (reg32))
#define EINTC_MASK_GET_CCU_EINTC_MASK(reg32)                                   \
	REG_FLD_GET(EINTC_MASK_FLD_CCU_EINTC_MASK, (reg32))

#define EINTC_CLR_GET_CCU_EINTC_CLR(reg32)                                     \
	REG_FLD_GET(EINTC_CLR_FLD_CCU_EINTC_CLR, (reg32))

#define EINTC_ST_GET_CCU_EINTC_ST(reg32)                                       \
	REG_FLD_GET(EINTC_ST_FLD_CCU_EINTC_ST, (reg32))

#define EINTC_MISC_GET_CCU_EINTC_TRIG_ST(reg32)                                \
	REG_FLD_GET(EINTC_MISC_FLD_CCU_EINTC_TRIG_ST, (reg32))
#define EINTC_MISC_GET_CCU_EINTC_RAW_ST(reg32)                                 \
	REG_FLD_GET(EINTC_MISC_FLD_CCU_EINTC_RAW_ST, (reg32))

#define CROP_SIZE_GET_CCU_CROP_Y_SIZE(reg32)                                   \
	REG_FLD_GET(CROP_SIZE_FLD_CCU_CROP_Y_SIZE, (reg32))
#define CROP_SIZE_GET_CCU_CROP_X_SIZE(reg32)                                   \
	REG_FLD_GET(CROP_SIZE_FLD_CCU_CROP_X_SIZE, (reg32))

#define CROP_START_GET_CCU_CROP_START_Y(reg32)                                 \
	REG_FLD_GET(CROP_START_FLD_CCU_CROP_START_Y, (reg32))
#define CROP_START_GET_CCU_CROP_START_X(reg32)                                 \
	REG_FLD_GET(CROP_START_FLD_CCU_CROP_START_X, (reg32))

#define CROP_END_GET_CCU_CROP_END_Y(reg32)                                     \
	REG_FLD_GET(CROP_END_FLD_CCU_CROP_END_Y, (reg32))
#define CROP_END_GET_CCU_CROP_END_X(reg32)                                     \
	REG_FLD_GET(CROP_END_FLD_CCU_CROP_END_X, (reg32))

#define ERR_ST_GET_S2T_B_INCF(reg32) REG_FLD_GET(ERR_ST_FLD_S2T_B_INCF, (reg32))
#define ERR_ST_GET_T2S_B_INCF(reg32) REG_FLD_GET(ERR_ST_FLD_T2S_B_INCF, (reg32))
#define ERR_ST_GET_S2T_A_INCF(reg32) REG_FLD_GET(ERR_ST_FLD_S2T_A_INCF, (reg32))
#define ERR_ST_GET_T2S_A_INCF(reg32) REG_FLD_GET(ERR_ST_FLD_T2S_A_INCF, (reg32))
#define ERR_ST_GET_T2S_B_OVERFLOW(reg32)                                       \
	REG_FLD_GET(ERR_ST_FLD_T2S_B_OVERFLOW, (reg32))

#define CCU_INFO00_GET_CCU_INFO0(reg32)                                        \
	REG_FLD_GET(CCU_INFO00_FLD_CCU_INFO0, (reg32))

#define CCU_INFO01_GET_CCU_INFO1(reg32)                                        \
	REG_FLD_GET(CCU_INFO01_FLD_CCU_INFO1, (reg32))

#define CCU_INFO02_GET_CCU_INFO2(reg32)                                        \
	REG_FLD_GET(CCU_INFO02_FLD_CCU_INFO2, (reg32))

#define CCU_INFO03_GET_CCU_INFO3(reg32)                                        \
	REG_FLD_GET(CCU_INFO03_FLD_CCU_INFO3, (reg32))

#define CCU_INFO04_GET_CCU_INFO4(reg32)                                        \
	REG_FLD_GET(CCU_INFO04_FLD_CCU_INFO4, (reg32))

#define CCU_INFO05_GET_CCU_INFO5(reg32)                                        \
	REG_FLD_GET(CCU_INFO05_FLD_CCU_INFO5, (reg32))

#define CCU_INFO06_GET_CCU_INFO6(reg32)                                        \
	REG_FLD_GET(CCU_INFO06_FLD_CCU_INFO6, (reg32))

#define CCU_INFO07_GET_CCU_INFO7(reg32)                                        \
	REG_FLD_GET(CCU_INFO07_FLD_CCU_INFO7, (reg32))

#define CCU_INFO08_GET_CCU_INFO8(reg32)                                        \
	REG_FLD_GET(CCU_INFO08_FLD_CCU_INFO8, (reg32))

#define CCU_INFO09_GET_CCU_INFO9(reg32)                                        \
	REG_FLD_GET(CCU_INFO09_FLD_CCU_INFO9, (reg32))

#define CCU_INFO10_GET_CCU_INFO10(reg32)                                       \
	REG_FLD_GET(CCU_INFO10_FLD_CCU_INFO10, (reg32))

#define CCU_INFO11_GET_CCU_INFO11(reg32)                                       \
	REG_FLD_GET(CCU_INFO11_FLD_CCU_INFO11, (reg32))

#define CCU_INFO12_GET_CCU_INFO12(reg32)                                       \
	REG_FLD_GET(CCU_INFO12_FLD_CCU_INFO12, (reg32))

#define CCU_INFO13_GET_CCU_INFO13(reg32)                                       \
	REG_FLD_GET(CCU_INFO13_FLD_CCU_INFO13, (reg32))

#define CCU_INFO14_GET_CCU_INFO14(reg32)                                       \
	REG_FLD_GET(CCU_INFO14_FLD_CCU_INFO14, (reg32))

#define CCU_INFO15_GET_CCU_INFO15(reg32)                                       \
	REG_FLD_GET(CCU_INFO15_FLD_CCU_INFO15, (reg32))

#define CCU_INFO16_GET_CCU_INFO16(reg32)                                       \
	REG_FLD_GET(CCU_INFO16_FLD_CCU_INFO16, (reg32))

#define CCU_INFO17_GET_CCU_INFO17(reg32)                                       \
	REG_FLD_GET(CCU_INFO17_FLD_CCU_INFO17, (reg32))

#define CCU_INFO18_GET_CCU_INFO18(reg32)                                       \
	REG_FLD_GET(CCU_INFO18_FLD_CCU_INFO18, (reg32))

#define CCU_INFO19_GET_CCU_INFO19(reg32)                                       \
	REG_FLD_GET(CCU_INFO19_FLD_CCU_INFO19, (reg32))

#define CCU_INFO20_GET_CCU_INFO20(reg32)                                       \
	REG_FLD_GET(CCU_INFO20_FLD_CCU_INFO20, (reg32))

#define CCU_INFO21_GET_CCU_INFO21(reg32)                                       \
	REG_FLD_GET(CCU_INFO21_FLD_CCU_INFO21, (reg32))

#define CCU_INFO22_GET_CCU_INFO22(reg32)                                       \
	REG_FLD_GET(CCU_INFO22_FLD_CCU_INFO22, (reg32))

#define CCU_INFO23_GET_CCU_INFO23(reg32)                                       \
	REG_FLD_GET(CCU_INFO23_FLD_CCU_INFO23, (reg32))

#define CCU_INFO24_GET_CCU_INFO24(reg32)                                       \
	REG_FLD_GET(CCU_INFO24_FLD_CCU_INFO24, (reg32))

#define CCU_INFO25_GET_CCU_INFO25(reg32)                                       \
	REG_FLD_GET(CCU_INFO25_FLD_CCU_INFO25, (reg32))

#define CCU_INFO26_GET_CCU_INFO26(reg32)                                       \
	REG_FLD_GET(CCU_INFO26_FLD_CCU_INFO26, (reg32))

#define CCU_INFO27_GET_CCU_INFO27(reg32)                                       \
	REG_FLD_GET(CCU_INFO27_FLD_CCU_INFO27, (reg32))

#define CCU_INFO28_GET_CCU_INFO28(reg32)                                       \
	REG_FLD_GET(CCU_INFO28_FLD_CCU_INFO28, (reg32))

#define CCU_INFO29_GET_CCU_INFO29(reg32)                                       \
	REG_FLD_GET(CCU_INFO29_FLD_CCU_INFO29, (reg32))

#define CCU_INFO30_GET_CCU_INFO30(reg32)                                       \
	REG_FLD_GET(CCU_INFO30_FLD_CCU_INFO30, (reg32))

#define CCU_INFO31_GET_CCU_INFO31(reg32)                                       \
	REG_FLD_GET(CCU_INFO31_FLD_CCU_INFO31, (reg32))

#define CCU_PC_GET_CCU_CCU_PC(reg32) REG_FLD_GET(CCU_PC_FLD_CCU_CCU_PC, (reg32))

#define RESET_SET_AHB2GMC_HW_RST(reg32, val)                                   \
	REG_FLD_SET(RESET_FLD_AHB2GMC_HW_RST, (reg32), (val))
#define RESET_SET_WDMA_HW_RST(reg32, val)                                      \
	REG_FLD_SET(RESET_FLD_WDMA_HW_RST, (reg32), (val))
#define RESET_SET_RDMA_HW_RST(reg32, val)                                      \
	REG_FLD_SET(RESET_FLD_RDMA_HW_RST, (reg32), (val))
#define RESET_SET_T2S_B_HW_RST(reg32, val)                                     \
	REG_FLD_SET(RESET_FLD_T2S_B_HW_RST, (reg32), (val))
#define RESET_SET_S2T_B_HW_RST(reg32, val)                                     \
	REG_FLD_SET(RESET_FLD_S2T_B_HW_RST, (reg32), (val))
#define RESET_SET_T2S_A_HW_RST(reg32, val)                                     \
	REG_FLD_SET(RESET_FLD_T2S_A_HW_RST, (reg32), (val))
#define RESET_SET_S2T_A_HW_RST(reg32, val)                                     \
	REG_FLD_SET(RESET_FLD_S2T_A_HW_RST, (reg32), (val))
#define RESET_SET_ARBITER_HW_RST(reg32, val)                                   \
	REG_FLD_SET(RESET_FLD_ARBITER_HW_RST, (reg32), (val))
#define RESET_SET_CCU_HW_RST(reg32, val)                                       \
	REG_FLD_SET(RESET_FLD_CCU_HW_RST, (reg32), (val))
#define RESET_SET_WDMA_SOFT_RST(reg32, val)                                    \
	REG_FLD_SET(RESET_FLD_WDMA_SOFT_RST, (reg32), (val))
#define RESET_SET_RDMA_SOFT_RST(reg32, val)                                    \
	REG_FLD_SET(RESET_FLD_RDMA_SOFT_RST, (reg32), (val))
#define RESET_SET_WDMA_SOFT_RST_ST(reg32, val)                                 \
	REG_FLD_SET(RESET_FLD_WDMA_SOFT_RST_ST, (reg32), (val))
#define RESET_SET_RDMA_SOFT_RST_ST(reg32, val)                                 \
	REG_FLD_SET(RESET_FLD_RDMA_SOFT_RST_ST, (reg32), (val))

#define START_TRIG_SET_WDMA_START(reg32, val)                                  \
	REG_FLD_SET(START_TRIG_FLD_WDMA_START, (reg32), (val))
#define START_TRIG_SET_RDMA_START(reg32, val)                                  \
	REG_FLD_SET(START_TRIG_FLD_RDMA_START, (reg32), (val))
#define START_TRIG_SET_T2S_A_START(reg32, val)                                 \
	REG_FLD_SET(START_TRIG_FLD_T2S_A_START, (reg32), (val))
#define START_TRIG_SET_S2T_A_START(reg32, val)                                 \
	REG_FLD_SET(START_TRIG_FLD_S2T_A_START, (reg32), (val))

#define BANK_INCR_SET_T2S_B_FINISH(reg32, val)                                 \
	REG_FLD_SET(BANK_INCR_FLD_T2S_B_FINISH, (reg32), (val))
#define BANK_INCR_SET_S2T_B_FINISH(reg32, val)                                 \
	REG_FLD_SET(BANK_INCR_FLD_S2T_B_FINISH, (reg32), (val))
#define BANK_INCR_SET_T2S_A_FINISH(reg32, val)                                 \
	REG_FLD_SET(BANK_INCR_FLD_T2S_A_FINISH, (reg32), (val))
#define BANK_INCR_SET_S2T_A_FINISH(reg32, val)                                 \
	REG_FLD_SET(BANK_INCR_FLD_S2T_A_FINISH, (reg32), (val))

#define DONE_ST_SET_S2T_B_FRAME_DONE(reg32, val)                               \
	REG_FLD_SET(DONE_ST_FLD_S2T_B_FRAME_DONE, (reg32), (val))
#define DONE_ST_SET_S2T_B_BANK_DONE(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_S2T_B_BANK_DONE, (reg32), (val))
#define DONE_ST_SET_T2S_B_FRAME_DONE(reg32, val)                               \
	REG_FLD_SET(DONE_ST_FLD_T2S_B_FRAME_DONE, (reg32), (val))
#define DONE_ST_SET_T2S_B_BANK_DONE(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_T2S_B_BANK_DONE, (reg32), (val))
#define DONE_ST_SET_S2T_A_FRAME_DONE(reg32, val)                               \
	REG_FLD_SET(DONE_ST_FLD_S2T_A_FRAME_DONE, (reg32), (val))
#define DONE_ST_SET_S2T_A_BANK_DONE(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_S2T_A_BANK_DONE, (reg32), (val))
#define DONE_ST_SET_T2S_A_FRAME_DONE(reg32, val)                               \
	REG_FLD_SET(DONE_ST_FLD_T2S_A_FRAME_DONE, (reg32), (val))
#define DONE_ST_SET_T2S_A_BANK_DONE(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_T2S_A_BANK_DONE, (reg32), (val))
#define DONE_ST_SET_CCU_GATED(reg32, val)                                      \
	REG_FLD_SET(DONE_ST_FLD_CCU_GATED, (reg32), (val))
#define DONE_ST_SET_CCU_HALT(reg32, val)                                       \
	REG_FLD_SET(DONE_ST_FLD_CCU_HALT, (reg32), (val))
#define DONE_ST_SET_T2S_B_BANK_ID(reg32, val)                                  \
	REG_FLD_SET(DONE_ST_FLD_T2S_B_BANK_ID, (reg32), (val))
#define DONE_ST_SET_S2T_B_BANK_ID(reg32, val)                                  \
	REG_FLD_SET(DONE_ST_FLD_S2T_B_BANK_ID, (reg32), (val))
#define DONE_ST_SET_T2S_B_BANK_FULL(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_T2S_B_BANK_FULL, (reg32), (val))
#define DONE_ST_SET_S2T_B_BANK_FULL(reg32, val)                                \
	REG_FLD_SET(DONE_ST_FLD_S2T_B_BANK_FULL, (reg32), (val))

#define CTRL_SET_CROP_EN(reg32, val)                                           \
	REG_FLD_SET(CTRL_FLD_CROP_EN, (reg32), (val))
#define CTRL_SET_DL_PATH_SEL(reg32, val)                                       \
	REG_FLD_SET(CTRL_FLD_DL_PATH_SEL, (reg32), (val))
#define CTRL_SET_STR_SEL(reg32, val)                                           \
	REG_FLD_SET(CTRL_FLD_STR_SEL, (reg32), (val))
#define CTRL_SET_DB_LOAD_DISABLE(reg32, val)                                   \
	REG_FLD_SET(CTRL_FLD_DB_LOAD_DISABLE, (reg32), (val))
#define CTRL_SET_OCD_EN(reg32, val) REG_FLD_SET(CTRL_FLD_OCD_EN, (reg32), (val))
#define CTRL_SET_OCD_JTAG_BYPASS(reg32, val)                                   \
	REG_FLD_SET(CTRL_FLD_OCD_JTAG_BYPASS, (reg32), (val))
#define CTRL_SET_CCU_AHB_BASE(reg32, val)                                      \
	REG_FLD_SET(CTRL_FLD_CCU_AHB_BASE, (reg32), (val))
#define CTRL_SET_H2G_EARLY_RESP(reg32, val)                                    \
	REG_FLD_SET(CTRL_FLD_H2G_EARLY_RESP, (reg32), (val))
#define CTRL_SET_H2G_GULTRA_ENABLE(reg32, val)                                 \
	REG_FLD_SET(CTRL_FLD_H2G_GULTRA_ENABLE, (reg32), (val))
#define CTRL_SET_H2G_GID(reg32, val)                                           \
	REG_FLD_SET(CTRL_FLD_H2G_GID, (reg32), (val))
#define CTRL_SET_CCU_TILE_SOURCE(reg32, val)                                   \
	REG_FLD_SET(CTRL_FLD_CCU_TILE_SOURCE, (reg32), (val))
#define CTRL_SET_CCU_PRINNER(reg32, val)                                       \
	REG_FLD_SET(CTRL_FLD_CCU_PRINNER, (reg32), (val))
#define CTRL_SET_T2S_B_ENABLE(reg32, val)                                      \
	REG_FLD_SET(CTRL_FLD_T2S_B_ENABLE, (reg32), (val))
#define CTRL_SET_S2T_B_ENABLE(reg32, val)                                      \
	REG_FLD_SET(CTRL_FLD_S2T_B_ENABLE, (reg32), (val))
#define CTRL_SET_T2S_A_ENABLE(reg32, val)                                      \
	REG_FLD_SET(CTRL_FLD_T2S_A_ENABLE, (reg32), (val))
#define CTRL_SET_S2T_A_ENABLE(reg32, val)                                      \
	REG_FLD_SET(CTRL_FLD_S2T_A_ENABLE, (reg32), (val))
#define CTRL_SET_WDMA_ENABLE(reg32, val)                                       \
	REG_FLD_SET(CTRL_FLD_WDMA_ENABLE, (reg32), (val))
#define CTRL_SET_RDMA_ENABLE(reg32, val)                                       \
	REG_FLD_SET(CTRL_FLD_RDMA_ENABLE, (reg32), (val))

#define CCU_INT_SET_INT_CCU(reg32, val)                                        \
	REG_FLD_SET(CCU_INT_FLD_INT_CCU, (reg32), (val))

#define CTL_CCU_INT_SET_INT_CTL_CCU(reg32, val)                                \
	REG_FLD_SET(CTL_CCU_INT_FLD_INT_CTL_CCU, (reg32), (val))

#define DCM_DIS_SET_SRAM_DCM_DIS(reg32, val)                                   \
	REG_FLD_SET(DCM_DIS_FLD_SRAM_DCM_DIS, (reg32), (val))
#define DCM_DIS_SET_CCUO_DCM_DIS(reg32, val)                                   \
	REG_FLD_SET(DCM_DIS_FLD_CCUO_DCM_DIS, (reg32), (val))
#define DCM_DIS_SET_CCUI_DCM_DIS(reg32, val)                                   \
	REG_FLD_SET(DCM_DIS_FLD_CCUI_DCM_DIS, (reg32), (val))

#define DCM_ST_SET_SRAM_DCM_ST(reg32, val)                                     \
	REG_FLD_SET(DCM_ST_FLD_SRAM_DCM_ST, (reg32), (val))
#define DCM_ST_SET_CCUO_DCM_ST(reg32, val)                                     \
	REG_FLD_SET(DCM_ST_FLD_CCUO_DCM_ST, (reg32), (val))
#define DCM_ST_SET_CCUI_DCM_ST(reg32, val)                                     \
	REG_FLD_SET(DCM_ST_FLD_CCUI_DCM_ST, (reg32), (val))

#define DMA_ERR_ST_SET_DMA_RDY_ST(reg32, val)                                  \
	REG_FLD_SET(DMA_ERR_ST_FLD_DMA_RDY_ST, (reg32), (val))
#define DMA_ERR_ST_SET_DMA_REQ_ST(reg32, val)                                  \
	REG_FLD_SET(DMA_ERR_ST_FLD_DMA_REQ_ST, (reg32), (val))
#define DMA_ERR_ST_SET_DMA_ERR_INT(reg32, val)                                 \
	REG_FLD_SET(DMA_ERR_ST_FLD_DMA_ERR_INT, (reg32), (val))

#define DMA_DEBUG_SET_DMA_DEBUG(reg32, val)                                    \
	REG_FLD_SET(DMA_DEBUG_FLD_DMA_DEBUG, (reg32), (val))

#define INT_MASK_SET_APMCU_INT_MASK(reg32, val)                                \
	REG_FLD_SET(INT_MASK_FLD_APMCU_INT_MASK, (reg32), (val))
#define INT_MASK_SET_T2S_TILE_INT_MASK(reg32, val)                             \
	REG_FLD_SET(INT_MASK_FLD_T2S_TILE_INT_MASK, (reg32), (val))
#define INT_MASK_SET_S2T_TILE_INT_MASK(reg32, val)                             \
	REG_FLD_SET(INT_MASK_FLD_S2T_TILE_INT_MASK, (reg32), (val))
#define INT_MASK_SET_T2S_BANK_INT_MASK(reg32, val)                             \
	REG_FLD_SET(INT_MASK_FLD_T2S_BANK_INT_MASK, (reg32), (val))
#define INT_MASK_SET_S2T_BANK_INT_MASK(reg32, val)                             \
	REG_FLD_SET(INT_MASK_FLD_S2T_BANK_INT_MASK, (reg32), (val))
#define INT_MASK_SET_WDMA_INT_MASK(reg32, val)                                 \
	REG_FLD_SET(INT_MASK_FLD_WDMA_INT_MASK, (reg32), (val))
#define INT_MASK_SET_RDMA_INT_MASK(reg32, val)                                 \
	REG_FLD_SET(INT_MASK_FLD_RDMA_INT_MASK, (reg32), (val))
#define INT_MASK_SET_ISP_INT_MASK(reg32, val)                                  \
	REG_FLD_SET(INT_MASK_FLD_ISP_INT_MASK, (reg32), (val))

#define PARB_ENABLE_SET_CCU_EN(reg32, val)                                     \
	REG_FLD_SET(PARB_ENABLE_FLD_CCU_EN, (reg32), (val))
#define PARB_ENABLE_SET_T2S_A_EN(reg32, val)                                   \
	REG_FLD_SET(PARB_ENABLE_FLD_T2S_A_EN, (reg32), (val))
#define PARB_ENABLE_SET_S2T_A_EN(reg32, val)                                   \
	REG_FLD_SET(PARB_ENABLE_FLD_S2T_A_EN, (reg32), (val))
#define PARB_ENABLE_SET_A2S_EN(reg32, val)                                     \
	REG_FLD_SET(PARB_ENABLE_FLD_A2S_EN, (reg32), (val))

#define DARB_ENABLE_SET_CCU_EN(reg32, val)                                     \
	REG_FLD_SET(DARB_ENABLE_FLD_CCU_EN, (reg32), (val))
#define DARB_ENABLE_SET_T2S_B_EN(reg32, val)                                   \
	REG_FLD_SET(DARB_ENABLE_FLD_T2S_B_EN, (reg32), (val))
#define DARB_ENABLE_SET_S2T_B_EN(reg32, val)                                   \
	REG_FLD_SET(DARB_ENABLE_FLD_S2T_B_EN, (reg32), (val))
#define DARB_ENABLE_SET_T2S_A_EN(reg32, val)                                   \
	REG_FLD_SET(DARB_ENABLE_FLD_T2S_A_EN, (reg32), (val))
#define DARB_ENABLE_SET_S2T_A_EN(reg32, val)                                   \
	REG_FLD_SET(DARB_ENABLE_FLD_S2T_A_EN, (reg32), (val))
#define DARB_ENABLE_SET_A2S_EN(reg32, val)                                     \
	REG_FLD_SET(DARB_ENABLE_FLD_A2S_EN, (reg32), (val))

#define TOP_SPARE_SET_CCU_SPARE(reg32, val)                                    \
	REG_FLD_SET(TOP_SPARE_FLD_CCU_SPARE, (reg32), (val))

#define DMA_ST_SET_WDMA_DONE_ST(reg32, val)                                    \
	REG_FLD_SET(DMA_ST_FLD_WDMA_DONE_ST, (reg32), (val))
#define DMA_ST_SET_RDMA_DONE_ST(reg32, val)                                    \
	REG_FLD_SET(DMA_ST_FLD_RDMA_DONE_ST, (reg32), (val))

#define EINTC_MASK_SET_CCU_EINTC_MODE(reg32, val)                              \
	REG_FLD_SET(EINTC_MASK_FLD_CCU_EINTC_MODE, (reg32), (val))
#define EINTC_MASK_SET_CCU_EINTC_MASK(reg32, val)                              \
	REG_FLD_SET(EINTC_MASK_FLD_CCU_EINTC_MASK, (reg32), (val))

#define EINTC_CLR_SET_CCU_EINTC_CLR(reg32, val)                                \
	REG_FLD_SET(EINTC_CLR_FLD_CCU_EINTC_CLR, (reg32), (val))

#define EINTC_ST_SET_CCU_EINTC_ST(reg32, val)                                  \
	REG_FLD_SET(EINTC_ST_FLD_CCU_EINTC_ST, (reg32), (val))

#define EINTC_MISC_SET_CCU_EINTC_TRIG_ST(reg32, val)                           \
	REG_FLD_SET(EINTC_MISC_FLD_CCU_EINTC_TRIG_ST, (reg32), (val))
#define EINTC_MISC_SET_CCU_EINTC_RAW_ST(reg32, val)                            \
	REG_FLD_SET(EINTC_MISC_FLD_CCU_EINTC_RAW_ST, (reg32), (val))

#define CROP_SIZE_SET_CCU_CROP_Y_SIZE(reg32, val)                              \
	REG_FLD_SET(CROP_SIZE_FLD_CCU_CROP_Y_SIZE, (reg32), (val))
#define CROP_SIZE_SET_CCU_CROP_X_SIZE(reg32, val)                              \
	REG_FLD_SET(CROP_SIZE_FLD_CCU_CROP_X_SIZE, (reg32), (val))

#define CROP_START_SET_CCU_CROP_START_Y(reg32, val)                            \
	REG_FLD_SET(CROP_START_FLD_CCU_CROP_START_Y, (reg32), (val))
#define CROP_START_SET_CCU_CROP_START_X(reg32, val)                            \
	REG_FLD_SET(CROP_START_FLD_CCU_CROP_START_X, (reg32), (val))

#define CROP_END_SET_CCU_CROP_END_Y(reg32, val)                                \
	REG_FLD_SET(CROP_END_FLD_CCU_CROP_END_Y, (reg32), (val))
#define CROP_END_SET_CCU_CROP_END_X(reg32, val)                                \
	REG_FLD_SET(CROP_END_FLD_CCU_CROP_END_X, (reg32), (val))

#define ERR_ST_SET_S2T_B_INCF(reg32, val)                                      \
	REG_FLD_SET(ERR_ST_FLD_S2T_B_INCF, (reg32), (val))
#define ERR_ST_SET_T2S_B_INCF(reg32, val)                                      \
	REG_FLD_SET(ERR_ST_FLD_T2S_B_INCF, (reg32), (val))
#define ERR_ST_SET_S2T_A_INCF(reg32, val)                                      \
	REG_FLD_SET(ERR_ST_FLD_S2T_A_INCF, (reg32), (val))
#define ERR_ST_SET_T2S_A_INCF(reg32, val)                                      \
	REG_FLD_SET(ERR_ST_FLD_T2S_A_INCF, (reg32), (val))
#define ERR_ST_SET_T2S_B_OVERFLOW(reg32, val)                                  \
	REG_FLD_SET(ERR_ST_FLD_T2S_B_OVERFLOW, (reg32), (val))

#define CCU_INFO00_SET_CCU_INFO0(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO00_FLD_CCU_INFO0, (reg32), (val))

#define CCU_INFO01_SET_CCU_INFO1(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO01_FLD_CCU_INFO1, (reg32), (val))

#define CCU_INFO02_SET_CCU_INFO2(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO02_FLD_CCU_INFO2, (reg32), (val))

#define CCU_INFO03_SET_CCU_INFO3(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO03_FLD_CCU_INFO3, (reg32), (val))

#define CCU_INFO04_SET_CCU_INFO4(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO04_FLD_CCU_INFO4, (reg32), (val))

#define CCU_INFO05_SET_CCU_INFO5(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO05_FLD_CCU_INFO5, (reg32), (val))

#define CCU_INFO06_SET_CCU_INFO6(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO06_FLD_CCU_INFO6, (reg32), (val))

#define CCU_INFO07_SET_CCU_INFO7(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO07_FLD_CCU_INFO7, (reg32), (val))

#define CCU_INFO08_SET_CCU_INFO8(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO08_FLD_CCU_INFO8, (reg32), (val))

#define CCU_INFO09_SET_CCU_INFO9(reg32, val)                                   \
	REG_FLD_SET(CCU_INFO09_FLD_CCU_INFO9, (reg32), (val))

#define CCU_INFO10_SET_CCU_INFO10(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO10_FLD_CCU_INFO10, (reg32), (val))

#define CCU_INFO11_SET_CCU_INFO11(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO11_FLD_CCU_INFO11, (reg32), (val))

#define CCU_INFO12_SET_CCU_INFO12(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO12_FLD_CCU_INFO12, (reg32), (val))

#define CCU_INFO13_SET_CCU_INFO13(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO13_FLD_CCU_INFO13, (reg32), (val))

#define CCU_INFO14_SET_CCU_INFO14(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO14_FLD_CCU_INFO14, (reg32), (val))

#define CCU_INFO15_SET_CCU_INFO15(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO15_FLD_CCU_INFO15, (reg32), (val))

#define CCU_INFO16_SET_CCU_INFO16(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO16_FLD_CCU_INFO16, (reg32), (val))

#define CCU_INFO17_SET_CCU_INFO17(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO17_FLD_CCU_INFO17, (reg32), (val))

#define CCU_INFO18_SET_CCU_INFO18(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO18_FLD_CCU_INFO18, (reg32), (val))

#define CCU_INFO19_SET_CCU_INFO19(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO19_FLD_CCU_INFO19, (reg32), (val))

#define CCU_INFO20_SET_CCU_INFO20(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO20_FLD_CCU_INFO20, (reg32), (val))

#define CCU_INFO21_SET_CCU_INFO21(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO21_FLD_CCU_INFO21, (reg32), (val))

#define CCU_INFO22_SET_CCU_INFO22(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO22_FLD_CCU_INFO22, (reg32), (val))

#define CCU_INFO23_SET_CCU_INFO23(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO23_FLD_CCU_INFO23, (reg32), (val))

#define CCU_INFO24_SET_CCU_INFO24(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO24_FLD_CCU_INFO24, (reg32), (val))

#define CCU_INFO25_SET_CCU_INFO25(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO25_FLD_CCU_INFO25, (reg32), (val))

#define CCU_INFO26_SET_CCU_INFO26(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO26_FLD_CCU_INFO26, (reg32), (val))

#define CCU_INFO27_SET_CCU_INFO27(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO27_FLD_CCU_INFO27, (reg32), (val))

#define CCU_INFO28_SET_CCU_INFO28(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO28_FLD_CCU_INFO28, (reg32), (val))

#define CCU_INFO29_SET_CCU_INFO29(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO29_FLD_CCU_INFO29, (reg32), (val))

#define CCU_INFO30_SET_CCU_INFO30(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO30_FLD_CCU_INFO30, (reg32), (val))

#define CCU_INFO31_SET_CCU_INFO31(reg32, val)                                  \
	REG_FLD_SET(CCU_INFO31_FLD_CCU_INFO31, (reg32), (val))

#define CCU_PC_SET_CCU_CCU_PC(reg32, val)                                      \
	REG_FLD_SET(CCU_PC_FLD_CCU_CCU_PC, (reg32), (val))

#define RESET_VAL_AHB2GMC_HW_RST(val)                                          \
	REG_FLD_VAL(RESET_FLD_AHB2GMC_HW_RST, (val))
#define RESET_VAL_WDMA_HW_RST(val) REG_FLD_VAL(RESET_FLD_WDMA_HW_RST, (val))
#define RESET_VAL_RDMA_HW_RST(val) REG_FLD_VAL(RESET_FLD_RDMA_HW_RST, (val))
#define RESET_VAL_T2S_B_HW_RST(val) REG_FLD_VAL(RESET_FLD_T2S_B_HW_RST, (val))
#define RESET_VAL_S2T_B_HW_RST(val) REG_FLD_VAL(RESET_FLD_S2T_B_HW_RST, (val))
#define RESET_VAL_T2S_A_HW_RST(val) REG_FLD_VAL(RESET_FLD_T2S_A_HW_RST, (val))
#define RESET_VAL_S2T_A_HW_RST(val) REG_FLD_VAL(RESET_FLD_S2T_A_HW_RST, (val))
#define RESET_VAL_ARBITER_HW_RST(val)                                          \
	REG_FLD_VAL(RESET_FLD_ARBITER_HW_RST, (val))
#define RESET_VAL_CCU_HW_RST(val) REG_FLD_VAL(RESET_FLD_CCU_HW_RST, (val))
#define RESET_VAL_WDMA_SOFT_RST(val) REG_FLD_VAL(RESET_FLD_WDMA_SOFT_RST, (val))
#define RESET_VAL_RDMA_SOFT_RST(val) REG_FLD_VAL(RESET_FLD_RDMA_SOFT_RST, (val))
#define RESET_VAL_WDMA_SOFT_RST_ST(val)                                        \
	REG_FLD_VAL(RESET_FLD_WDMA_SOFT_RST_ST, (val))
#define RESET_VAL_RDMA_SOFT_RST_ST(val)                                        \
	REG_FLD_VAL(RESET_FLD_RDMA_SOFT_RST_ST, (val))

#define START_TRIG_VAL_WDMA_START(val)                                         \
	REG_FLD_VAL(START_TRIG_FLD_WDMA_START, (val))
#define START_TRIG_VAL_RDMA_START(val)                                         \
	REG_FLD_VAL(START_TRIG_FLD_RDMA_START, (val))
#define START_TRIG_VAL_T2S_A_START(val)                                        \
	REG_FLD_VAL(START_TRIG_FLD_T2S_A_START, (val))
#define START_TRIG_VAL_S2T_A_START(val)                                        \
	REG_FLD_VAL(START_TRIG_FLD_S2T_A_START, (val))

#define BANK_INCR_VAL_T2S_B_FINISH(val)                                        \
	REG_FLD_VAL(BANK_INCR_FLD_T2S_B_FINISH, (val))
#define BANK_INCR_VAL_S2T_B_FINISH(val)                                        \
	REG_FLD_VAL(BANK_INCR_FLD_S2T_B_FINISH, (val))
#define BANK_INCR_VAL_T2S_A_FINISH(val)                                        \
	REG_FLD_VAL(BANK_INCR_FLD_T2S_A_FINISH, (val))
#define BANK_INCR_VAL_S2T_A_FINISH(val)                                        \
	REG_FLD_VAL(BANK_INCR_FLD_S2T_A_FINISH, (val))

#define DONE_ST_VAL_S2T_B_FRAME_DONE(val)                                      \
	REG_FLD_VAL(DONE_ST_FLD_S2T_B_FRAME_DONE, (val))
#define DONE_ST_VAL_S2T_B_BANK_DONE(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_S2T_B_BANK_DONE, (val))
#define DONE_ST_VAL_T2S_B_FRAME_DONE(val)                                      \
	REG_FLD_VAL(DONE_ST_FLD_T2S_B_FRAME_DONE, (val))
#define DONE_ST_VAL_T2S_B_BANK_DONE(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_T2S_B_BANK_DONE, (val))
#define DONE_ST_VAL_S2T_A_FRAME_DONE(val)                                      \
	REG_FLD_VAL(DONE_ST_FLD_S2T_A_FRAME_DONE, (val))
#define DONE_ST_VAL_S2T_A_BANK_DONE(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_S2T_A_BANK_DONE, (val))
#define DONE_ST_VAL_T2S_A_FRAME_DONE(val)                                      \
	REG_FLD_VAL(DONE_ST_FLD_T2S_A_FRAME_DONE, (val))
#define DONE_ST_VAL_T2S_A_BANK_DONE(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_T2S_A_BANK_DONE, (val))
#define DONE_ST_VAL_CCU_GATED(val) REG_FLD_VAL(DONE_ST_FLD_CCU_GATED, (val))
#define DONE_ST_VAL_CCU_HALT(val) REG_FLD_VAL(DONE_ST_FLD_CCU_HALT, (val))
#define DONE_ST_VAL_T2S_B_BANK_ID(val)                                         \
	REG_FLD_VAL(DONE_ST_FLD_T2S_B_BANK_ID, (val))
#define DONE_ST_VAL_S2T_B_BANK_ID(val)                                         \
	REG_FLD_VAL(DONE_ST_FLD_S2T_B_BANK_ID, (val))
#define DONE_ST_VAL_T2S_B_BANK_FULL(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_T2S_B_BANK_FULL, (val))
#define DONE_ST_VAL_S2T_B_BANK_FULL(val)                                       \
	REG_FLD_VAL(DONE_ST_FLD_S2T_B_BANK_FULL, (val))

#define CTRL_VAL_CROP_EN(val) REG_FLD_VAL(CTRL_FLD_CROP_EN, (val))
#define CTRL_VAL_DL_PATH_SEL(val) REG_FLD_VAL(CTRL_FLD_DL_PATH_SEL, (val))
#define CTRL_VAL_STR_SEL(val) REG_FLD_VAL(CTRL_FLD_STR_SEL, (val))
#define CTRL_VAL_DB_LOAD_DISABLE(val)                                          \
	REG_FLD_VAL(CTRL_FLD_DB_LOAD_DISABLE, (val))
#define CTRL_VAL_OCD_EN(val) REG_FLD_VAL(CTRL_FLD_OCD_EN, (val))
#define CTRL_VAL_OCD_JTAG_BYPASS(val)                                          \
	REG_FLD_VAL(CTRL_FLD_OCD_JTAG_BYPASS, (val))
#define CTRL_VAL_CCU_AHB_BASE(val) REG_FLD_VAL(CTRL_FLD_CCU_AHB_BASE, (val))
#define CTRL_VAL_H2G_EARLY_RESP(val) REG_FLD_VAL(CTRL_FLD_H2G_EARLY_RESP, (val))
#define CTRL_VAL_H2G_GULTRA_ENABLE(val)                                        \
	REG_FLD_VAL(CTRL_FLD_H2G_GULTRA_ENABLE, (val))
#define CTRL_VAL_H2G_GID(val) REG_FLD_VAL(CTRL_FLD_H2G_GID, (val))
#define CTRL_VAL_CCU_TILE_SOURCE(val)                                          \
	REG_FLD_VAL(CTRL_FLD_CCU_TILE_SOURCE, (val))
#define CTRL_VAL_CCU_PRINNER(val) REG_FLD_VAL(CTRL_FLD_CCU_PRINNER, (val))
#define CTRL_VAL_T2S_B_ENABLE(val) REG_FLD_VAL(CTRL_FLD_T2S_B_ENABLE, (val))
#define CTRL_VAL_S2T_B_ENABLE(val) REG_FLD_VAL(CTRL_FLD_S2T_B_ENABLE, (val))
#define CTRL_VAL_T2S_A_ENABLE(val) REG_FLD_VAL(CTRL_FLD_T2S_A_ENABLE, (val))
#define CTRL_VAL_S2T_A_ENABLE(val) REG_FLD_VAL(CTRL_FLD_S2T_A_ENABLE, (val))
#define CTRL_VAL_WDMA_ENABLE(val) REG_FLD_VAL(CTRL_FLD_WDMA_ENABLE, (val))
#define CTRL_VAL_RDMA_ENABLE(val) REG_FLD_VAL(CTRL_FLD_RDMA_ENABLE, (val))

#define CCU_INT_VAL_INT_CCU(val) REG_FLD_VAL(CCU_INT_FLD_INT_CCU, (val))

#define CTL_CCU_INT_VAL_INT_CTL_CCU(val)                                       \
	REG_FLD_VAL(CTL_CCU_INT_FLD_INT_CTL_CCU, (val))

#define DCM_DIS_VAL_SRAM_DCM_DIS(val)                                          \
	REG_FLD_VAL(DCM_DIS_FLD_SRAM_DCM_DIS, (val))
#define DCM_DIS_VAL_CCUO_DCM_DIS(val)                                          \
	REG_FLD_VAL(DCM_DIS_FLD_CCUO_DCM_DIS, (val))
#define DCM_DIS_VAL_CCUI_DCM_DIS(val)                                          \
	REG_FLD_VAL(DCM_DIS_FLD_CCUI_DCM_DIS, (val))

#define DCM_ST_VAL_SRAM_DCM_ST(val) REG_FLD_VAL(DCM_ST_FLD_SRAM_DCM_ST, (val))
#define DCM_ST_VAL_CCUO_DCM_ST(val) REG_FLD_VAL(DCM_ST_FLD_CCUO_DCM_ST, (val))
#define DCM_ST_VAL_CCUI_DCM_ST(val) REG_FLD_VAL(DCM_ST_FLD_CCUI_DCM_ST, (val))

#define DMA_ERR_ST_VAL_DMA_RDY_ST(val)                                         \
	REG_FLD_VAL(DMA_ERR_ST_FLD_DMA_RDY_ST, (val))
#define DMA_ERR_ST_VAL_DMA_REQ_ST(val)                                         \
	REG_FLD_VAL(DMA_ERR_ST_FLD_DMA_REQ_ST, (val))
#define DMA_ERR_ST_VAL_DMA_ERR_INT(val)                                        \
	REG_FLD_VAL(DMA_ERR_ST_FLD_DMA_ERR_INT, (val))

#define DMA_DEBUG_VAL_DMA_DEBUG(val) REG_FLD_VAL(DMA_DEBUG_FLD_DMA_DEBUG, (val))

#define INT_MASK_VAL_APMCU_INT_MASK(val)                                       \
	REG_FLD_VAL(INT_MASK_FLD_APMCU_INT_MASK, (val))
#define INT_MASK_VAL_T2S_TILE_INT_MASK(val)                                    \
	REG_FLD_VAL(INT_MASK_FLD_T2S_TILE_INT_MASK, (val))
#define INT_MASK_VAL_S2T_TILE_INT_MASK(val)                                    \
	REG_FLD_VAL(INT_MASK_FLD_S2T_TILE_INT_MASK, (val))
#define INT_MASK_VAL_T2S_BANK_INT_MASK(val)                                    \
	REG_FLD_VAL(INT_MASK_FLD_T2S_BANK_INT_MASK, (val))
#define INT_MASK_VAL_S2T_BANK_INT_MASK(val)                                    \
	REG_FLD_VAL(INT_MASK_FLD_S2T_BANK_INT_MASK, (val))
#define INT_MASK_VAL_WDMA_INT_MASK(val)                                        \
	REG_FLD_VAL(INT_MASK_FLD_WDMA_INT_MASK, (val))
#define INT_MASK_VAL_RDMA_INT_MASK(val)                                        \
	REG_FLD_VAL(INT_MASK_FLD_RDMA_INT_MASK, (val))
#define INT_MASK_VAL_ISP_INT_MASK(val)                                         \
	REG_FLD_VAL(INT_MASK_FLD_ISP_INT_MASK, (val))

#define PARB_ENABLE_VAL_CCU_EN(val) REG_FLD_VAL(PARB_ENABLE_FLD_CCU_EN, (val))
#define PARB_ENABLE_VAL_T2S_A_EN(val)                                          \
	REG_FLD_VAL(PARB_ENABLE_FLD_T2S_A_EN, (val))
#define PARB_ENABLE_VAL_S2T_A_EN(val)                                          \
	REG_FLD_VAL(PARB_ENABLE_FLD_S2T_A_EN, (val))
#define PARB_ENABLE_VAL_A2S_EN(val) REG_FLD_VAL(PARB_ENABLE_FLD_A2S_EN, (val))

#define DARB_ENABLE_VAL_CCU_EN(val) REG_FLD_VAL(DARB_ENABLE_FLD_CCU_EN, (val))
#define DARB_ENABLE_VAL_T2S_B_EN(val)                                          \
	REG_FLD_VAL(DARB_ENABLE_FLD_T2S_B_EN, (val))
#define DARB_ENABLE_VAL_S2T_B_EN(val)                                          \
	REG_FLD_VAL(DARB_ENABLE_FLD_S2T_B_EN, (val))
#define DARB_ENABLE_VAL_T2S_A_EN(val)                                          \
	REG_FLD_VAL(DARB_ENABLE_FLD_T2S_A_EN, (val))
#define DARB_ENABLE_VAL_S2T_A_EN(val)                                          \
	REG_FLD_VAL(DARB_ENABLE_FLD_S2T_A_EN, (val))
#define DARB_ENABLE_VAL_A2S_EN(val) REG_FLD_VAL(DARB_ENABLE_FLD_A2S_EN, (val))

#define TOP_SPARE_VAL_CCU_SPARE(val) REG_FLD_VAL(TOP_SPARE_FLD_CCU_SPARE, (val))

#define DMA_ST_VAL_WDMA_DONE_ST(val) REG_FLD_VAL(DMA_ST_FLD_WDMA_DONE_ST, (val))
#define DMA_ST_VAL_RDMA_DONE_ST(val) REG_FLD_VAL(DMA_ST_FLD_RDMA_DONE_ST, (val))

#define EINTC_MASK_VAL_CCU_EINTC_MODE(val)                                     \
	REG_FLD_VAL(EINTC_MASK_FLD_CCU_EINTC_MODE, (val))
#define EINTC_MASK_VAL_CCU_EINTC_MASK(val)                                     \
	REG_FLD_VAL(EINTC_MASK_FLD_CCU_EINTC_MASK, (val))

#define EINTC_CLR_VAL_CCU_EINTC_CLR(val)                                       \
	REG_FLD_VAL(EINTC_CLR_FLD_CCU_EINTC_CLR, (val))

#define EINTC_ST_VAL_CCU_EINTC_ST(val)                                         \
	REG_FLD_VAL(EINTC_ST_FLD_CCU_EINTC_ST, (val))

#define EINTC_MISC_VAL_CCU_EINTC_TRIG_ST(val)                                  \
	REG_FLD_VAL(EINTC_MISC_FLD_CCU_EINTC_TRIG_ST, (val))
#define EINTC_MISC_VAL_CCU_EINTC_RAW_ST(val)                                   \
	REG_FLD_VAL(EINTC_MISC_FLD_CCU_EINTC_RAW_ST, (val))

#define CROP_SIZE_VAL_CCU_CROP_Y_SIZE(val)                                     \
	REG_FLD_VAL(CROP_SIZE_FLD_CCU_CROP_Y_SIZE, (val))
#define CROP_SIZE_VAL_CCU_CROP_X_SIZE(val)                                     \
	REG_FLD_VAL(CROP_SIZE_FLD_CCU_CROP_X_SIZE, (val))

#define CROP_START_VAL_CCU_CROP_START_Y(val)                                   \
	REG_FLD_VAL(CROP_START_FLD_CCU_CROP_START_Y, (val))
#define CROP_START_VAL_CCU_CROP_START_X(val)                                   \
	REG_FLD_VAL(CROP_START_FLD_CCU_CROP_START_X, (val))

#define CROP_END_VAL_CCU_CROP_END_Y(val)                                       \
	REG_FLD_VAL(CROP_END_FLD_CCU_CROP_END_Y, (val))
#define CROP_END_VAL_CCU_CROP_END_X(val)                                       \
	REG_FLD_VAL(CROP_END_FLD_CCU_CROP_END_X, (val))

#define ERR_ST_VAL_S2T_B_INCF(val) REG_FLD_VAL(ERR_ST_FLD_S2T_B_INCF, (val))
#define ERR_ST_VAL_T2S_B_INCF(val) REG_FLD_VAL(ERR_ST_FLD_T2S_B_INCF, (val))
#define ERR_ST_VAL_S2T_A_INCF(val) REG_FLD_VAL(ERR_ST_FLD_S2T_A_INCF, (val))
#define ERR_ST_VAL_T2S_A_INCF(val) REG_FLD_VAL(ERR_ST_FLD_T2S_A_INCF, (val))
#define ERR_ST_VAL_T2S_B_OVERFLOW(val)                                         \
	REG_FLD_VAL(ERR_ST_FLD_T2S_B_OVERFLOW, (val))

#define CCU_INFO00_VAL_CCU_INFO0(val)                                          \
	REG_FLD_VAL(CCU_INFO00_FLD_CCU_INFO0, (val))

#define CCU_INFO01_VAL_CCU_INFO1(val)                                          \
	REG_FLD_VAL(CCU_INFO01_FLD_CCU_INFO1, (val))

#define CCU_INFO02_VAL_CCU_INFO2(val)                                          \
	REG_FLD_VAL(CCU_INFO02_FLD_CCU_INFO2, (val))

#define CCU_INFO03_VAL_CCU_INFO3(val)                                          \
	REG_FLD_VAL(CCU_INFO03_FLD_CCU_INFO3, (val))

#define CCU_INFO04_VAL_CCU_INFO4(val)                                          \
	REG_FLD_VAL(CCU_INFO04_FLD_CCU_INFO4, (val))

#define CCU_INFO05_VAL_CCU_INFO5(val)                                          \
	REG_FLD_VAL(CCU_INFO05_FLD_CCU_INFO5, (val))

#define CCU_INFO06_VAL_CCU_INFO6(val)                                          \
	REG_FLD_VAL(CCU_INFO06_FLD_CCU_INFO6, (val))

#define CCU_INFO07_VAL_CCU_INFO7(val)                                          \
	REG_FLD_VAL(CCU_INFO07_FLD_CCU_INFO7, (val))

#define CCU_INFO08_VAL_CCU_INFO8(val)                                          \
	REG_FLD_VAL(CCU_INFO08_FLD_CCU_INFO8, (val))

#define CCU_INFO09_VAL_CCU_INFO9(val)                                          \
	REG_FLD_VAL(CCU_INFO09_FLD_CCU_INFO9, (val))

#define CCU_INFO10_VAL_CCU_INFO10(val)                                         \
	REG_FLD_VAL(CCU_INFO10_FLD_CCU_INFO10, (val))

#define CCU_INFO11_VAL_CCU_INFO11(val)                                         \
	REG_FLD_VAL(CCU_INFO11_FLD_CCU_INFO11, (val))

#define CCU_INFO12_VAL_CCU_INFO12(val)                                         \
	REG_FLD_VAL(CCU_INFO12_FLD_CCU_INFO12, (val))

#define CCU_INFO13_VAL_CCU_INFO13(val)                                         \
	REG_FLD_VAL(CCU_INFO13_FLD_CCU_INFO13, (val))

#define CCU_INFO14_VAL_CCU_INFO14(val)                                         \
	REG_FLD_VAL(CCU_INFO14_FLD_CCU_INFO14, (val))

#define CCU_INFO15_VAL_CCU_INFO15(val)                                         \
	REG_FLD_VAL(CCU_INFO15_FLD_CCU_INFO15, (val))

#define CCU_INFO16_VAL_CCU_INFO16(val)                                         \
	REG_FLD_VAL(CCU_INFO16_FLD_CCU_INFO16, (val))

#define CCU_INFO17_VAL_CCU_INFO17(val)                                         \
	REG_FLD_VAL(CCU_INFO17_FLD_CCU_INFO17, (val))

#define CCU_INFO18_VAL_CCU_INFO18(val)                                         \
	REG_FLD_VAL(CCU_INFO18_FLD_CCU_INFO18, (val))

#define CCU_INFO19_VAL_CCU_INFO19(val)                                         \
	REG_FLD_VAL(CCU_INFO19_FLD_CCU_INFO19, (val))

#define CCU_INFO20_VAL_CCU_INFO20(val)                                         \
	REG_FLD_VAL(CCU_INFO20_FLD_CCU_INFO20, (val))

#define CCU_INFO21_VAL_CCU_INFO21(val)                                         \
	REG_FLD_VAL(CCU_INFO21_FLD_CCU_INFO21, (val))

#define CCU_INFO22_VAL_CCU_INFO22(val)                                         \
	REG_FLD_VAL(CCU_INFO22_FLD_CCU_INFO22, (val))

#define CCU_INFO23_VAL_CCU_INFO23(val)                                         \
	REG_FLD_VAL(CCU_INFO23_FLD_CCU_INFO23, (val))

#define CCU_INFO24_VAL_CCU_INFO24(val)                                         \
	REG_FLD_VAL(CCU_INFO24_FLD_CCU_INFO24, (val))

#define CCU_INFO25_VAL_CCU_INFO25(val)                                         \
	REG_FLD_VAL(CCU_INFO25_FLD_CCU_INFO25, (val))

#define CCU_INFO26_VAL_CCU_INFO26(val)                                         \
	REG_FLD_VAL(CCU_INFO26_FLD_CCU_INFO26, (val))

#define CCU_INFO27_VAL_CCU_INFO27(val)                                         \
	REG_FLD_VAL(CCU_INFO27_FLD_CCU_INFO27, (val))

#define CCU_INFO28_VAL_CCU_INFO28(val)                                         \
	REG_FLD_VAL(CCU_INFO28_FLD_CCU_INFO28, (val))

#define CCU_INFO29_VAL_CCU_INFO29(val)                                         \
	REG_FLD_VAL(CCU_INFO29_FLD_CCU_INFO29, (val))

#define CCU_INFO30_VAL_CCU_INFO30(val)                                         \
	REG_FLD_VAL(CCU_INFO30_FLD_CCU_INFO30, (val))

#define CCU_INFO31_VAL_CCU_INFO31(val)                                         \
	REG_FLD_VAL(CCU_INFO31_FLD_CCU_INFO31, (val))

#define CCU_PC_VAL_CCU_CCU_PC(val) REG_FLD_VAL(CCU_PC_FLD_CCU_CCU_PC, (val))

#ifdef __cplusplus
}
#endif
#endif /* __CCU_A_REGS_H__*/
