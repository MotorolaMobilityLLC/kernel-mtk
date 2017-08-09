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

#ifndef _DDP_REG_H_
#define _DDP_REG_H_
#include <mt-plat/sync_write.h>
/*#include <mt-plat/mt_typedefs.h>*/
/* #include <mach/mt_reg_base.h> */
#include <linux/types.h>
#include "display_recorder.h"
#include "cmdq_record.h"
#include "cmdq_core.h"
#include "ddp_hal.h"

/* //////////////////////////////////// macro /////////////////////////////////////////// */

#define ENABLE_CLK_MGR

/* field definition */
/* ------------------------------------------------------------- */
/* MIPITX */
typedef struct {
	unsigned RG_DSI_LDOCORE_EN:1;
	unsigned RG_DSI_CKG_LDOOUT_EN:1;
	unsigned RG_DSI_BCLK_SEL:2;
	unsigned RG_DSI_LD_IDX_SEL:3;
	unsigned rsv_7:1;
	unsigned RG_DSI_PHYCLK_SEL:2;
	unsigned RG_DSI_DSICLK_FREQ_SEL:1;
	unsigned RG_DSI_LPTX_CLMP_EN:1;
	unsigned rsv_12:20;
} MIPITX_DSI_CON_REG, *PMIPITX_DSI_CON_REG;

typedef struct {
	unsigned RG_DSI_LNTC_LDOOUT_EN:1;
	unsigned RG_DSI_LNTC_LOOPBACK_EN:1;
	unsigned RG_DSI_LNTC_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNTC_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNTC_LPTX_IMINUS:1;
	unsigned RG_DSI_LNTC_LPCD_IPLUS:1;
	unsigned RG_DSI_LNTC_LPCD_IMINUS:1;
	unsigned rsv_7:1;
	unsigned RG_DSI_LNTC_RT_CODE:4;
	unsigned rsv_12:20;
} MIPITX_DSI_CLOCK_LANE_REG, *PMIPITX_DSI_CLOCK_LANE_REG;

typedef struct {
	unsigned RG_DSI_LNT0_LDOOUT_EN:1;
	unsigned RG_DSI_LNT0_CKLANE_EN:1;
	unsigned RG_DSI_LNT0_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT0_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT0_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT0_LPCD_IPLUS:1;
	unsigned RG_DSI_LNT0_LPCD_IMINUS:1;
	unsigned rsv_7:1;
	unsigned RG_DSI_LNT0_RT_CODE:4;
	unsigned rsv_11:20;
} MIPITX_DSI_DATA_LANE0_REG, *PMIPITX_DSI_DATA_LANE0_REG;


typedef struct {
	unsigned RG_DSI_LNT1_LDOOUT_EN:1;
	unsigned RG_DSI_LNT1_CKLANE_EN:1;
	unsigned RG_DSI_LNT1_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT1_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT1_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT1_LPCD_IPLUS:1;
	unsigned RG_DSI_LNT1_LPCD_IMINUS:1;
	unsigned rsv_7:1;
	unsigned RG_DSI_LNT1_RT_CODE:4;
	unsigned rsv_11:20;
} MIPITX_DSI_DATA_LANE1_REG, *PMIPITX_DSI_DATA_LANE1_REG;


typedef struct {
	unsigned RG_DSI_LNT2_LDOOUT_EN:1;
	unsigned RG_DSI_LNT2_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT2_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT2_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT2_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT2_LPCD_IPLUS:1;
	unsigned RG_DSI_LNT2_LPCD_IMINUS:1;
	unsigned rsv_7:1;
	unsigned RG_DSI_LNT2_RT_CODE:4;
	unsigned rsv_11:20;
} MIPITX_DSI_DATA_LANE2_REG, *PMIPITX_DSI_DATA_LANE2_REG;

typedef struct {
	unsigned RG_DSI_LNT3_LDOOUT_EN:1;
	unsigned RG_DSI_LNT3_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT3_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT3_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT3_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT3_LPCD_IPLUS:1;
	unsigned RG_DSI_LNT3_LPCD_IMINUS:1;
	unsigned rsv_7:1;
	unsigned RG_DSI_LNT3_RT_CODE:4;
	unsigned rsv_11:20;
} MIPITX_DSI_DATA_LANE3_REG, *PMIPITX_DSI_DATA_LANE3_REG;

typedef struct {
	unsigned RG_DSI_LNT_INTR_EN:1;
	unsigned RG_DSI_LNT_HS_BIAS_EN:1;
	unsigned RG_DSI_LNT_IMP_CAL_EN:1;
	unsigned RG_DSI_LNT_TESTMODE_EN:1;
	unsigned RG_DSI_LNT_IMP_CAL_CODE:4;
	unsigned RG_DSI_LNT_AIO_SEL:3;
	unsigned RG_DSI_PAD_TIE_LOW_EN:1;
	unsigned rsv_12:1;
	unsigned RG_DSI_PRESERVE:3;
	unsigned rsv_16:16;
} MIPITX_DSI_TOP_CON_REG, *PMIPITX_DSI_TOP_CON_REG;


typedef struct {
	unsigned RG_DSI_BG_CORE_EN:1;
	unsigned RG_DSI_BG_CKEN:1;
	unsigned RG_DSI_BG_DIV:2;
	unsigned RG_DSI_BG_FAST_CHARGE:1;
	unsigned RG_DSI_V12_SEL:3;
	unsigned RG_DSI_V10_SEL:3;
	unsigned RG_DSI_V072_SEL:3;
	unsigned RG_DSI_V04_SEL:3;
	unsigned RG_DSI_V032_SEL:3;
	unsigned RG_DSI_V02_SEL:3;
	unsigned rsv_23:1;
	unsigned RG_DSI_BG_R1_TRIM:4;
	unsigned RG_DSI_BG_R2_TRIM:4;
} MIPITX_DSI_BG_CON_REG, *PMIPITX_DSI_BG_CON_REG;


typedef struct {
	unsigned RG_DSI0_MPPLL_PLL_EN:1;
	unsigned RG_DSI0_MPPLL_PREDIV:2;
	unsigned RG_DSI0_MPPLL_TXDIV0:2;
	unsigned RG_DSI0_MPPLL_TXDIV1:2;
	unsigned RG_DSI0_MPPLL_POSDIV:3;
	unsigned RG_DSI0_MPPLL_MONVC_EN:1;
	unsigned RG_DSI0_MPPLL_MONREF_EN:1;
	unsigned RG_DSI0_MPPLL_VDO_EN:1;
	unsigned rsv_13:19;
} MIPITX_DSI_PLL_CON0_REG, *PMIPITX_DSI_PLL_CON0_REG;


typedef struct {
	unsigned RG_DSI0_MPPLL_SDM_FRA_EN:1;
	unsigned RG_DSI0_MPPLL_SDM_SSC_PH_INIT:1;
	unsigned RG_DSI0_MPPLL_SDM_SSC_EN:1;
	unsigned rsv_3:13;
	unsigned RG_DSI0_MPPLL_SDM_SSC_PRD:16;
} MIPITX_DSI_PLL_CON1_REG, *PMIPITX_DSI_PLL_CON1_REG;

typedef struct {
	unsigned RG_DSI0_MPPLL_SDM_PCW_0_7:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_8_15:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_16_23:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_H:7;
	unsigned rsv_31:1;
} MIPITX_DSI_PLL_CON2_REG, *PMIPITX_DSI_PLL_CON2_REG;


typedef struct {
	unsigned RG_DSI0_MPPLL_SDM_SSC_DELTA1:16;
	unsigned RG_DSI0_MPPLL_SDM_SSC_DELTA:16;
} MIPITX_DSI_PLL_CON3_REG, *PMIPITX_DSI_PLL_CON3_REG;


typedef struct {
	unsigned RG_DSI0_MPPLL_SDM_PCW_CHG:1;
	unsigned rsv_1:31;
} MIPITX_DSI_PLL_CHG_REG, *PMIPITX_DSI_PLL_CHG_REG;


typedef struct {
	unsigned RG_MPPLL_TST_EN:1;
	unsigned RG_MPPLL_TSTCK_EN:1;
	unsigned RG_MPPLL_TSTSEL:2;
	unsigned RG_MPPLL_S2QDIV:2;
	unsigned RG_MPPLL_PLLOUT_EN:1;
	unsigned RG_MPPLL_PRESERVE:5;
	unsigned rsv_12:20;
} MIPITX_DSI_PLL_TOP_REG, *PMIPITX_DSI_PLL_TOP_REG;


typedef struct {
	unsigned DA_DSI_MPPLL_SDM_PWR_ON:1;
	unsigned DA_DSI_MPPLL_SDM_ISO_EN:1;
	unsigned rsv_2:6;
	unsigned AD_DSI0_MPPLL_SDM_PWR_ACK:1;
	unsigned rsv_9:23;
} MIPITX_DSI_PLL_PWR_REG, *PMIPITX_DSI_PLL_PWR_REG;


typedef struct {
	unsigned RGS_DSI_LNT_IMP_CAL_OUTPUT:1;
	unsigned rsv_1:31;
} MIPITX_DSI_RGS_REG, *PMIPITX_DSI_RGS_REG;


typedef struct {
	unsigned RG_DSI0_GPI0_EN:1;
	unsigned RG_DSI0_GPI1_EN:1;
	unsigned RG_DSI0_GPI2_EN:1;
	unsigned RG_DSI0_GPI3_EN:1;
	unsigned RG_DSI0_GPI4_EN:1;
	unsigned RG_DSI0_GPI5_EN:1;
	unsigned RG_DSI0_GPI6_EN:1;
	unsigned RG_DSI0_GPI7_EN:1;
	unsigned RG_DSI0_GPI8_EN:1;
	unsigned RG_DSI0_GPI9_EN:1;
	unsigned RG_DSI0_GPI_SMT_EN:1;
	unsigned RG_DSI0_GPI_DRIVE_EN:1;
	unsigned rsv_12:20;
} MIPITX_DSI_GPI_EN_REG, *PMIPITX_DSI_GPI_EN_REG;

typedef struct {
	unsigned RG_DSI0_GPI0_PD:1;
	unsigned RG_DSI0_GPI1_PD:1;
	unsigned RG_DSI0_GPI2_PD:1;
	unsigned RG_DSI0_GPI3_PD:1;
	unsigned RG_DSI0_GPI4_PD:1;
	unsigned RG_DSI0_GPI5_PD:1;
	unsigned RG_DSI0_GPI6_PD:1;
	unsigned RG_DSI0_GPI7_PD:1;
	unsigned RG_DSI0_GPI8_PD:1;
	unsigned RG_DSI0_GPI9_PD:1;
	unsigned rsv_10:6;
	unsigned RG_DSI0_GPI0_PU:1;
	unsigned RG_DSI0_GPI1_PU:1;
	unsigned RG_DSI0_GPI2_PU:1;
	unsigned RG_DSI0_GPI3_PU:1;
	unsigned RG_DSI0_GPI4_PU:1;
	unsigned RG_DSI0_GPI5_PU:1;
	unsigned RG_DSI0_GPI6_PU:1;
	unsigned RG_DSI0_GPI7_PU:1;
	unsigned RG_DSI0_GPI8_PU:1;
	unsigned RG_DSI0_GPI9_PU:1;
	unsigned rsv_26:6;
} MIPITX_DSI_GPI_PULL_REG, *PMIPITX_DSI_GPI_PULL_REG;


typedef struct {
	unsigned MIPI_TX_PHY0_SEL:3;
	unsigned rsv_3:1;
	unsigned MIPI_TX_PHY1_SEL:3;
	unsigned rsv_7:1;
	unsigned MIPI_TX_PHY2_SEL:3;
	unsigned rsv_11:1;
	unsigned MIPI_TX_PHY3_SEL:3;
	unsigned rsv_15:1;
	unsigned MIPI_TX_PHYC_SEL:3;
	unsigned rsv_19:1;
	unsigned MIPI_TX_LPRX_SEL:3;
	unsigned rsv_23:9;
} MIPITX_DSI_PHY_SEL_REG, *PMIPITX_DSI_PHY_SEL_REG;


typedef struct {
	unsigned SW_CTRL_EN:1;
	unsigned rsv_1:31;
} MIPITX_DSI_SW_CTRL_REG, *PMIPITX_DSI_SW_CTRL_REG;


typedef struct {
	unsigned SW_LNTC_LPTX_PRE_OE:1;
	unsigned SW_LNTC_LPTX_OE:1;
	unsigned SW_LNTC_LPTX_DP:1;
	unsigned SW_LNTC_LPTX_DN:1;
	unsigned SW_LNTC_HSTX_PRE_OE:1;
	unsigned SW_LNTC_HSTX_OE:1;
	unsigned SW_LNTC_HSTX_RDY:1;
	unsigned SW_LNTC_LPRX_EN:1;
	unsigned SW_LNTC_HSTX_DATA:8;
	unsigned rsv_16:16;
} MIPITX_DSI_SW_CTRL_CON0_REG, *PMIPITX_DSI_SW_CTRL_CON0_REG;


typedef struct {
	unsigned SW_LNT0_LPTX_PRE_OE:1;
	unsigned SW_LNT0_LPTX_OE:1;
	unsigned SW_LNT0_LPTX_DP:1;
	unsigned SW_LNT0_LPTX_DN:1;
	unsigned SW_LNT0_HSTX_PRE_OE:1;
	unsigned SW_LNT0_HSTX_OE:1;
	unsigned SW_LNT0_HSTX_RDY:1;
	unsigned SW_LNT0_LPRX_EN:1;
	unsigned SW_LNT1_LPTX_PRE_OE:1;
	unsigned SW_LNT1_LPTX_OE:1;
	unsigned SW_LNT1_LPTX_DP:1;
	unsigned SW_LNT1_LPTX_DN:1;
	unsigned SW_LNT1_HSTX_PRE_OE:1;
	unsigned SW_LNT1_HSTX_OE:1;
	unsigned SW_LNT1_HSTX_RDY:1;
	unsigned SW_LNT1_LPRX_EN:1;
	unsigned SW_LNT2_LPTX_PRE_OE:1;
	unsigned SW_LNT2_LPTX_OE:1;
	unsigned SW_LNT2_LPTX_DP:1;
	unsigned SW_LNT2_LPTX_DN:1;
	unsigned SW_LNT2_HSTX_PRE_OE:1;
	unsigned SW_LNT2_HSTX_OE:1;
	unsigned SW_LNT2_HSTX_RDY:1;
	unsigned SW_LNT2_LPRX_EN:1;
	unsigned SW_LNT3_LPTX_PRE_OE:1;
	unsigned SW_LNT3_LPTX_OE:1;
	unsigned SW_LNT3_LPTX_DP:1;
	unsigned SW_LNT3_LPTX_DN:1;
	unsigned SW_LNT3_HSTX_PRE_OE:1;
	unsigned SW_LNT3_HSTX_OE:1;
	unsigned SW_LNT3_HSTX_RDY:1;
	unsigned SW_LNT3_LPRX_EN:1;
} MIPITX_DSI_SW_CTRL_CON1_REG, *PMIPITX_DSI_SW_CTRL_CON1_REG;

typedef struct {
	unsigned SW_LNT_HSTX_DATA:8;
	unsigned rsv_8:24;
} MIPITX_DSI_SW_CTRL_CON2_REG, *PMIPITX_DSI_SW_CTRL_CON2_REG;

typedef struct {
	unsigned MIPI_TX_DBG_SEL:4;
	unsigned MIPI_TX_DBG_OUT_EN:1;
	unsigned MIPI_TX_GPIO_MODE_EN:1;
	unsigned MIPI_TX_APB_ASYNC_CNT_EN:1;
	unsigned rsv_7:25;
} MIPITX_DSI_DBG_CON_REG, *PMIPITX_DSI_DBG_CON_REG;

typedef struct {
	unsigned MIPI_TX_APB_ASYNC_ERR:1;
	unsigned MIPI_TX_APB_ASYNC_ERR_ADDR:10;
	unsigned rsv_11:21;
} MIPITX_DSI_APB_ASYNC_STA_REG, *PMIPITX_DSI_APB_ASYNC_STA_REG;


/* field definition */
/* ------------------------------------------------------------- */
/* DSI */

typedef struct {
	unsigned DSI_START:1;
	unsigned rsv_1:1;
	unsigned SLEEPOUT_START:1;
	unsigned rsv_3:1;
	unsigned SKEWCAL_START:1;
	unsigned rsv_5:11;
	unsigned VM_CMD_START:1;
	unsigned rsv_17:15;
} DSI_START_REG, *PDSI_START_REG;


typedef struct {
	unsigned rsv_0:1;
	unsigned BUF_UNDERRUN:1;
	unsigned rsv_2:2;
	unsigned ESC_ENTRY_ERR:1;
	unsigned ESC_SYNC_ERR:1;
	unsigned CTRL_ERR:1;
	unsigned CONTENT_ERR:1;
	unsigned rsv_8:24;
} DSI_STATUS_REG, *PDSI_STATUS_REG;


typedef struct {
	unsigned RD_RDY:1;
	unsigned CMD_DONE:1;
	unsigned TE_RDY:1;
	unsigned VM_DONE:1;
	unsigned FRAME_DONE_INT_EN:1;
	unsigned VM_CMD_DONE:1;
	unsigned SLEEPOUT_DONE:1;
	unsigned TE_TIMEOUT_INT_EN:1;
	unsigned VM_VBP_STR_INT_EN:1;
	unsigned VM_VACT_STR_INT_EN:1;
	unsigned VM_VFP_STR_INT_EN:1;
	unsigned SKEWCAL_DONE_INT_EN:1;
	unsigned rsv_12:20;
} DSI_INT_ENABLE_REG, *PDSI_INT_ENABLE_REG;


typedef struct {
	unsigned RD_RDY:1;
	unsigned CMD_DONE:1;
	unsigned TE_RDY:1;
	unsigned VM_DONE:1;
	unsigned FRAME_DONE_INT_EN:1;
	unsigned VM_CMD_DONE:1;
	unsigned SLEEPOUT_DONE:1;
	unsigned TE_TIMEOUT_INT_EN:1;
	unsigned VM_VBP_STR_INT_EN:1;
	unsigned VM_VACT_STR_INT_EN:1;
	unsigned VM_VFP_STR_INT_EN:1;
	unsigned SKEWCAL_DONE_INT_EN:1;
	unsigned rsv_12:19;
	unsigned BUSY:1;
} DSI_INT_STATUS_REG, *PDSI_INT_STATUS_REG;


typedef struct {
	unsigned DSI_RESET:1;
	unsigned rsv_1:1;
	unsigned DPHY_RESET:1;
	unsigned rsv_3:1;
	unsigned DSI_DUAL_EN:1;
	unsigned rsv_5:27;
} DSI_COM_CTRL_REG, *PDSI_COM_CTRL_REG;


typedef enum {
	DSI_CMD_MODE = 0,
	DSI_SYNC_PULSE_VDO_MODE = 1,
	DSI_SYNC_EVENT_VDO_MODE = 2,
	DSI_BURST_VDO_MODE = 3
} DSI_MODE_CTRL;


typedef struct {
	unsigned MODE:2;
	unsigned rsv_2:14;
	unsigned FRM_MODE:1;
	unsigned MIX_MODE:1;
	unsigned V2C_SWITCH_ON:1;
	unsigned C2V_SWITCH_ON:1;
	unsigned SLEEP_MODE:1;
	unsigned rsv_21:11;
} DSI_MODE_CTRL_REG, *PDSI_MODE_CTRL_REG;


typedef enum {
	ONE_LANE = 1,
	TWO_LANE = 2,
	THREE_LANE = 3,
	FOUR_LANE = 4
} DSI_LANE_NUM;


typedef struct {
	unsigned VC_NUM:2;
	unsigned LANE_NUM:4;
	unsigned DIS_EOT:1;
	unsigned BLLP_EN:1;
	unsigned TE_FREERUN:1;
	unsigned EXT_TE_EN:1;
	unsigned EXT_TE_EDGE:1;
	unsigned TE_AUTO_SYNC:1;
	unsigned MAX_RTN_SIZE:4;
	unsigned HSTX_CKLP_EN:1;
	unsigned TYPE1_BTA_SEL:1;
	unsigned TE_WITH_CMD_EN:1;
	unsigned TE_TIMEOUT_CHK_EN:1;
	unsigned EXT_TE_TIME_VM:4;
	unsigned rsv_24:8;
} DSI_TXRX_CTRL_REG, *PDSI_TXRX_CTRL_REG;


typedef enum {
	PACKED_PS_16BIT_RGB565 = 0,
	LOOSELY_PS_18BIT_RGB666 = 1,
	PACKED_PS_24BIT_RGB888 = 2,
	PACKED_PS_18BIT_RGB666 = 3
} DSI_PS_TYPE;


typedef struct {
	unsigned DSI_PS_WC:14;
	unsigned rsv_14:2;
	unsigned DSI_PS_SEL:2;
	unsigned rsv_18:6;
	unsigned RGB_SWAP:1;
	unsigned BYTE_SWAP:1;
	unsigned rsv_26:6;
} DSI_PSCTRL_REG, *PDSI_PSCTRL_REG;


typedef struct {
	unsigned VSA_NL:12;
	unsigned rsv_12:20;
} DSI_VSA_NL_REG, *PDSI_VSA_NL_REG;


typedef struct {
	unsigned VBP_NL:12;
	unsigned rsv_12:20;
} DSI_VBP_NL_REG, *PDSI_VBP_NL_REG;


typedef struct {
	unsigned VFP_NL:12;
	unsigned rsv_12:20;
} DSI_VFP_NL_REG, *PDSI_VFP_NL_REG;


typedef struct {
	unsigned VACT_NL:12;
	unsigned rsv_12:20;
} DSI_VACT_NL_REG, *PDSI_VACT_NL_REG;

typedef struct {
	unsigned LFR_MODE:2;
	unsigned LFR_TYPE:2;
	unsigned LFR_EN:1;
	unsigned LFR_UPDATE:1;
	unsigned LFR_VSE_DIS:1;
	unsigned rsv_7:1;
	unsigned LFR_SKIP_NUM:6;
	unsigned rsv_14:18;
} DSI_LFR_CON_REG, *PDSI_LFR_CON_REG;


typedef struct {
	unsigned LFR_SKIP_CNT:6;
	unsigned rsv_6:2;
	unsigned LFR_SKIP_STA:1;
	unsigned rsv_9:23;
} DSI_LFR_STA_REG, *PDSI_LFR_STA_REG;


typedef struct {
	unsigned HSA_WC:12;
	unsigned rsv_12:20;
} DSI_HSA_WC_REG, *PDSI_HSA_WC_REG;


typedef struct {
	unsigned HBP_WC:12;
	unsigned rsv_12:20;
} DSI_HBP_WC_REG, *PDSI_HBP_WC_REG;


typedef struct {
	unsigned HFP_WC:12;
	unsigned rsv_12:20;
} DSI_HFP_WC_REG, *PDSI_HFP_WC_REG;

typedef struct {
	unsigned BLLP_WC:12;
	unsigned rsv_12:20;
} DSI_BLLP_WC_REG, *PDSI_BLLP_WC_REG;

typedef struct {
	unsigned CMDQ_SIZE:8;
	unsigned rsv_8:24;
} DSI_CMDQ_CTRL_REG, *PDSI_CMDQ_CTRL_REG;
typedef struct {
	unsigned rsv_0:2;
	unsigned HSTX_CKLP_WC:14;
	unsigned HSTX_CKLP_WC_AUTO:1;
	unsigned rsv_17:15;
} DSI_HSTX_CKLP_REG, *PDSI_HSTX_CKLP_REG;

typedef struct {
	unsigned HSTX_CKLP_WC_AUTO_RESULT:16;
	unsigned rsv_16:16;
} DSI_HSTX_CKLP_WC_AUTO_RESULT_REG, *PDSI_HSTX_CKLP_WC_AUTO_RESULT_REG;

typedef struct {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
} DSI_RX_DATA_REG, *PDSI_RX_DATA_REG;


typedef struct {
	unsigned DSI_RACK:1;
	unsigned DSI_RACK_BYPASS:1;
	unsigned rsv2:30;
} DSI_RACK_REG, *PDSI_RACK_REG;


typedef struct {
	unsigned TRIG0:1;	/* remote rst */
	unsigned TRIG1:1;	/* TE */
	unsigned TRIG2:1;	/* ack */
	unsigned TRIG3:1;	/* rsv */
	unsigned RX_ULPS:1;
	unsigned DIRECTION:1;
	unsigned RX_LPDT:1;
	unsigned rsv7:1;
	unsigned RX_POINTER:4;
	unsigned rsv12:20;
} DSI_TRIG_STA_REG, *PDSI_TRIG_STA_REG;


typedef struct {
	unsigned RWMEM_CONTI:16;
	unsigned rsv16:16;
} DSI_MEM_CONTI_REG, *PDSI_MEM_CONTI_REG;


typedef struct {
	unsigned FRM_BC:21;
	unsigned rsv21:11;
} DSI_FRM_BC_REG, *PDSI_FRM_BC_REG;

typedef struct {
	unsigned _3D_MODE:2;
	unsigned _3D_FMT:2;
	unsigned _3D_VSYNC:1;
	unsigned _3D_LR:1;
	unsigned _3D_EN:1;
	unsigned rsv08:25;
} DSI_3D_CON_REG, *PDSI_3D_CON_REG;

typedef struct {
	unsigned UPLS_WAKEUP_PRD:16;
	unsigned SKEWCALL_PRD:16;
} DSI_TIME_CON0_REG, *PDSI_TIME_CON0_REG;

typedef struct {
	unsigned TE_WAKEUP_PRD:16;
	unsigned PREFETCH_TIME:15;
	unsigned PREFETCH_EN:1;
} DSI_TIME_CON1_REG, *PDSI_TIME_CON1_REG;


typedef struct {
	unsigned LC_HSTX_CK_PAT:32;
} DSI_PHY_LCPAT_REG, *PDSI_PHY_LCPAT_REG;

typedef struct {
	unsigned LC_HS_TX_EN:1;
	unsigned LC_ULPM_EN:1;
	unsigned LC_WAKEUP_EN:1;
	unsigned rsv3:29;
} DSI_PHY_LCCON_REG, *PDSI_PHY_LCCON_REG;


typedef struct {
	unsigned L0_RM_TRIG_EN:1;
	unsigned L0_ULPM_EN:1;
	unsigned L0_WAKEUP_EN:1;
	unsigned Lx_ULPM_AS_L0:1;
	unsigned L0_RX_FILTER_EN:1;
	unsigned rsv3:27;
} DSI_PHY_LD0CON_REG, *PDSI_PHY_LD0CON_REG;


typedef struct {
	unsigned HS_SYNC_CODE:8;
	unsigned HS_SYNC_CODE2:8;
	unsigned HS_SKEWCAL_PAT:8;
	unsigned HS_DB_SYNC_EN:1;
	unsigned rsv25:7;
} DSI_PHY_SYNCON_REG, *PDSI_PHY_SYNCON_REG;


typedef struct {
	unsigned char LPX;
	unsigned char HS_PRPR;
	unsigned char HS_ZERO;
	unsigned char HS_TRAIL;
} DSI_PHY_TIMCON0_REG, *PDSI_PHY_TIMCON0_REG;


typedef struct {
	unsigned char TA_GO;
	unsigned char TA_SURE;
	unsigned char TA_GET;
	unsigned char DA_HS_EXIT;
} DSI_PHY_TIMCON1_REG, *PDSI_PHY_TIMCON1_REG;


typedef struct {
	unsigned char CONT_DET;
	unsigned char DA_HS_SYNC;
	unsigned char CLK_ZERO;
	unsigned char CLK_TRAIL;
} DSI_PHY_TIMCON2_REG, *PDSI_PHY_TIMCON2_REG;


typedef struct {
	unsigned char CLK_HS_PRPR;
	unsigned char CLK_HS_POST;
	unsigned char CLK_HS_EXIT;
	unsigned rsv24:8;
} DSI_PHY_TIMCON3_REG, *PDSI_PHY_TIMCON3_REG;


typedef struct {
	unsigned ULPS_WAKEUP:20;
	unsigned rsv20:12;
} DSI_PHY_TIMCON4_REG, *PDSI_PHY_TIMCON4_REG;

typedef struct {
	unsigned VM_CMD_EN:1;
	unsigned LONG_PKT:1;
	unsigned TIME_SEL:1;
	unsigned TS_VSA_EN:1;
	unsigned TS_VBP_EN:1;
	unsigned TS_VFP_EN:1;
	unsigned rsv6:2;
	unsigned CM_DATA_ID:8;
	unsigned CM_DATA_0:8;
	unsigned CM_DATA_1:8;
} DSI_VM_CMD_CON_REG, *PDSI_VM_CMD_CON_REG;


typedef struct {
	DSI_PHY_TIMCON0_REG CTRL0;
	DSI_PHY_TIMCON1_REG CTRL1;
	DSI_PHY_TIMCON2_REG CTRL2;
	DSI_PHY_TIMCON3_REG CTRL3;
} DSI_PHY_TIMCON_REG, *PDSI_PHY_TIMCON_REG;


typedef struct {
	unsigned PKT_CHECK_SUM:16;
	unsigned ACC_CHECK_SUM:16;
} DSI_CKSM_OUT_REG, *PDSI_CKSM_OUT_REG;


typedef struct {
	unsigned DPHY_CTL_STATE_C:9;
	unsigned rsv9:7;
	unsigned DPHY_HS_TX_STATE_C:5;
	unsigned rsv21:11;
} DSI_STATE_DBG0_REG, *PDSI_STATE_DBG0_REG;


typedef struct {
	unsigned CTL_STATE_C:15;
	unsigned rsv15:1;
	unsigned HS_TX_STATE_0:5;
	unsigned rsv21:3;
	unsigned ESC_STATE_0:8;
} DSI_STATE_DBG1_REG, *PDSI_STATE_DBG1_REG;


typedef struct {
	unsigned RX_ESC_STATE:10;
	unsigned rsv10:6;
	unsigned TA_T2R_STATE:5;
	unsigned rsv21:3;
	unsigned TA_R2T_STATE:5;
	unsigned rsv29:3;
} DSI_STATE_DBG2_REG, *PDSI_STATE_DBG2_REG;


typedef struct {
	unsigned CTL_STATE_1:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_1:5;
	unsigned rsv13:3;
	unsigned CTL_STATE_2:5;
	unsigned rsv21:3;
	unsigned HS_TX_STATE_2:5;
	unsigned rsv29:3;
} DSI_STATE_DBG3_REG, *PDSI_STATE_DBG3_REG;


typedef struct {
	unsigned CTL_STATE_3:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_3:5;
	unsigned rsv13:19;
} DSI_STATE_DBG4_REG, *PDSI_STATE_DBG4_REG;


typedef struct {
	unsigned TIMER_COUNTER:16;
	unsigned TIMER_BUSY:1;
	unsigned rsv17:11;
	unsigned WAKEUP_STATE:4;
} DSI_STATE_DBG5_REG, *PDSI_STATE_DBG5_REG;


typedef struct {
	unsigned CMTRL_STATE:14;
	unsigned rsv14:2;
	unsigned CMDQ_STATE:8;
	unsigned rsv24:8;
} DSI_STATE_DBG6_REG, *PDSI_STATE_DBG6_REG;


typedef struct {
	unsigned VMCTL_STATE:11;
	unsigned rsv11:1;
	unsigned VFP_PERIOD:1;
	unsigned VACT_PERIOD:1;
	unsigned VBP_PERIOD:1;
	unsigned VSA_PERIOD:1;
	unsigned rsv16:16;
} DSI_STATE_DBG7_REG, *PDSI_STATE_DBG7_REG;


typedef struct {
	unsigned WORD_COUNTER:14;
	unsigned rsv14:18;
} DSI_STATE_DBG8_REG, *PDSI_STATE_DBG8_REG;


typedef struct {
	unsigned LINE_COUNTER:22;
	unsigned rsv22:10;
} DSI_STATE_DBG9_REG, *PDSI_STATE_DBG9_REG;


typedef struct {
	unsigned DEBUG_OUT_SEL:5;
	unsigned rsv5:3;
	unsigned CHKSUM_REC_EN:1;
	unsigned rsv9:23;
} DSI_DEBUG_SEL_REG, *PDSI_DEBUG_SEL_REG;


typedef struct {
	unsigned BIST_MODE:1;
	unsigned BIST_ENABLE:1;
	unsigned BIST_FIX_PATTERN:1;
	unsigned BIST_SPC_PATTERN:1;
	unsigned BIST_HS_FREE:1;
	unsigned rsv_05:1;
	unsigned SELF_PAT_MODE:1;
	unsigned rsv_07:1;
	unsigned BIST_LANE_NUM:4;
	unsigned rsv12:4;
	unsigned BIST_TIMING:8;
	unsigned rsv24:8;
} DSI_BIST_CON_REG, *PDSI_BIST_CON_REG;



typedef struct {
	DSI_START_REG DSI_START;	/* 0000 */
	DSI_STATUS_REG DSI_STA;	/* 0004 */
	DSI_INT_ENABLE_REG DSI_INTEN;	/* 0008 */
	DSI_INT_STATUS_REG DSI_INTSTA;	/* 000C */
	DSI_COM_CTRL_REG DSI_COM_CTRL;	/* 0010 */
	DSI_MODE_CTRL_REG DSI_MODE_CTRL;	/* 0014 */
	DSI_TXRX_CTRL_REG DSI_TXRX_CTRL;	/* 0018 */
	DSI_PSCTRL_REG DSI_PSCTRL;	/* 001C */
	DSI_VSA_NL_REG DSI_VSA_NL;	/* 0020 */
	DSI_VBP_NL_REG DSI_VBP_NL;	/* 0024 */
	DSI_VFP_NL_REG DSI_VFP_NL;	/* 0028 */
	DSI_VACT_NL_REG DSI_VACT_NL;	/* 002C */
	DSI_LFR_CON_REG DSI_LFR_CON;	/* 0030 */
	DSI_LFR_STA_REG DSI_LFR_STA;	/* 0034 */
	uint32_t rsv_38[6];	/* 0038..004C */
	DSI_HSA_WC_REG DSI_HSA_WC;	/* 0050 */
	DSI_HBP_WC_REG DSI_HBP_WC;	/* 0054 */
	DSI_HFP_WC_REG DSI_HFP_WC;	/* 0058 */
	DSI_BLLP_WC_REG DSI_BLLP_WC;	/* 005C */
	DSI_CMDQ_CTRL_REG DSI_CMDQ_SIZE;	/* 0060 */
	DSI_HSTX_CKLP_REG DSI_HSTX_CKL_WC;	/* 0064 */
	DSI_HSTX_CKLP_WC_AUTO_RESULT_REG DSI_HSTX_CKL_WC_AUTO_RESULT;	/* 0068 */
	uint32_t rsv_006C[2];	/* 006c..0070 */
	DSI_RX_DATA_REG DSI_RX_DATA0;	/* 0074 */
	DSI_RX_DATA_REG DSI_RX_DATA1;	/* 0078 */
	DSI_RX_DATA_REG DSI_RX_DATA2;	/* 007c */
	DSI_RX_DATA_REG DSI_RX_DATA3;	/* 0080 */
	DSI_RACK_REG DSI_RACK;	/* 0084 */
	DSI_TRIG_STA_REG DSI_TRIG_STA;	/* 0088 */
	uint32_t rsv_008C;	/* 008C */
	DSI_MEM_CONTI_REG DSI_MEM_CONTI;	/* 0090 */
	DSI_FRM_BC_REG DSI_FRM_BC;	/* 0094 */
	DSI_3D_CON_REG DSI_3D_CON;	/* 0098 */
	uint32_t rsv_009C;	/* 009c */
	DSI_TIME_CON0_REG DSI_TIME_CON0;	/* 00A0 */
	DSI_TIME_CON1_REG DSI_TIME_CON1;	/* 00A4 */

	uint32_t rsv_00A8[22];	/* 0A8..0FC */
	uint32_t DSI_PHY_PCPAT;	/* 00100 */

	DSI_PHY_LCCON_REG DSI_PHY_LCCON;	/* 0104 */
	DSI_PHY_LD0CON_REG DSI_PHY_LD0CON;	/* 0108 */
	DSI_PHY_SYNCON_REG DSI_PHY_SYNCON;	/* 010C */
	DSI_PHY_TIMCON0_REG DSI_PHY_TIMECON0;	/* 0110 */
	DSI_PHY_TIMCON1_REG DSI_PHY_TIMECON1;	/* 0114 */
	DSI_PHY_TIMCON2_REG DSI_PHY_TIMECON2;	/* 0118 */
	DSI_PHY_TIMCON3_REG DSI_PHY_TIMECON3;	/* 011C */
	uint32_t rsv_0120[4];	/* 0120..012c */
	DSI_VM_CMD_CON_REG DSI_VM_CMD_CON;	/* 0130 */
	uint32_t DSI_VM_CMD_DATA0;	/* 0134 */
	uint32_t DSI_VM_CMD_DATA4;	/* 0138 */
	uint32_t DSI_VM_CMD_DATA8;	/* 013C */
	uint32_t DSI_VM_CMD_DATAC;	/* 0140 */
	DSI_CKSM_OUT_REG DSI_CKSM_OUT;	/* 0144 */
	DSI_STATE_DBG0_REG DSI_STATE_DBG0;	/* 0148 */
	DSI_STATE_DBG1_REG DSI_STATE_DBG1;	/* 014C */
	DSI_STATE_DBG2_REG DSI_STATE_DBG2;	/* 0150 */
	DSI_STATE_DBG3_REG DSI_STATE_DBG3;	/* 0154 */
	DSI_STATE_DBG4_REG DSI_STATE_DBG4;	/* 0158 */
	DSI_STATE_DBG5_REG DSI_STATE_DBG5;	/* 015C */
	DSI_STATE_DBG6_REG DSI_STATE_DBG6;	/* 0160 */
	DSI_STATE_DBG7_REG DSI_STATE_DBG7;	/* 0164 */
	DSI_STATE_DBG8_REG DSI_STATE_DBG8;	/* 0168 */
	DSI_STATE_DBG9_REG DSI_STATE_DBG9;	/* 016C */
	DSI_DEBUG_SEL_REG DSI_DEBUG_SEL;	/* 0170 */
	uint32_t rsv174;		/* 0174 */
	uint32_t DSI_BIST_PATTERN;	/* 0178 */
	DSI_BIST_CON_REG DSI_BIST_CON;	/* 017C */
	uint32_t DSI_VM_CMD_DATA10;	/* 00180 */
	uint32_t DSI_VM_CMD_DATA14;	/* 00184 */
	uint32_t DSI_VM_CMD_DATA18;	/* 00188 */
	uint32_t DSI_VM_CMD_DATA1C;	/* 0018C */
} volatile DSI_REGS, *PDSI_REGS;

/* 0~1 TYPE ,2 BTA,3 HS, 4 CL,5 TE,6~7 RESV, 8~15 DATA_ID,16~23 DATA_0,24~31 DATA_1 */
typedef struct {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
} DSI_CMDQ, *PDSI_CMDQ;

typedef struct {
	DSI_CMDQ data[128];
} DSI_CMDQ_REGS, *PDSI_CMDQ_REGS;

typedef struct {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
} DSI_VM_CMDQ, *PDSI_VM_CMDQ;

typedef struct {
	DSI_VM_CMDQ data[4];
} DSI_VM_CMDQ_REGS, *PDSI_VM_CMDQ_REGS;

typedef struct {
	MIPITX_DSI_CON_REG MIPITX_DSI_CON;	/* 0000 */
	MIPITX_DSI_CLOCK_LANE_REG MIPITX_DSI_CLOCK_LANE;	/* 0004 */
	MIPITX_DSI_DATA_LANE0_REG MIPITX_DSI_DATA_LANE0;	/* 0008 */
	MIPITX_DSI_DATA_LANE1_REG MIPITX_DSI_DATA_LANE1;	/* 000C */
	MIPITX_DSI_DATA_LANE2_REG MIPITX_DSI_DATA_LANE2;	/* 0010 */
	MIPITX_DSI_DATA_LANE3_REG MIPITX_DSI_DATA_LANE3;	/* 0014 */
	uint32_t rsv_18[10];	/* 0018..003C */

	MIPITX_DSI_TOP_CON_REG MIPITX_DSI_TOP_CON;	/* 0040 */
	MIPITX_DSI_BG_CON_REG MIPITX_DSI_BG_CON;	/* 0044 */
	uint32_t rsv_48[2];	/* 0048..004C */
	MIPITX_DSI_PLL_CON0_REG MIPITX_DSI_PLL_CON0;	/* 0050 */
	MIPITX_DSI_PLL_CON1_REG MIPITX_DSI_PLL_CON1;	/* 0054 */
	MIPITX_DSI_PLL_CON2_REG MIPITX_DSI_PLL_CON2;	/* 0058 */
	MIPITX_DSI_PLL_CON3_REG MIPITX_DSI_PLL_CON3;	/* 005C */
	MIPITX_DSI_PLL_CHG_REG MIPITX_DSI_PLL_CHG;	/* 0060 */
	MIPITX_DSI_PLL_TOP_REG MIPITX_DSI_PLL_TOP;	/* 0064 */
	MIPITX_DSI_PLL_PWR_REG MIPITX_DSI_PLL_PWR;	/* 0068 */
	uint32_t rsv_6C;		/* 006C */
	MIPITX_DSI_RGS_REG MIPITX_DSI_RGS;	/* 0070 */
	MIPITX_DSI_GPI_EN_REG MIPITX_DSI_GPI_EN;	/* 0074 */
	MIPITX_DSI_GPI_PULL_REG MIPITX_DSI_GPI_PULL;	/* 0078 */
	MIPITX_DSI_PHY_SEL_REG MIPITX_DSI_PHY_SEL;	/* 007C */

	MIPITX_DSI_SW_CTRL_REG MIPITX_DSI_SW_CTRL_EN;	/* 0080 */
	MIPITX_DSI_SW_CTRL_CON0_REG MIPITX_DSI_SW_CTRL_CON0;	/* 0084 */
	MIPITX_DSI_SW_CTRL_CON1_REG MIPITX_DSI_SW_CTRL_CON1;	/* 0088 */
	MIPITX_DSI_SW_CTRL_CON2_REG MIPITX_DSI_SW_CTRL_CON2;	/* 008C */
	MIPITX_DSI_DBG_CON_REG MIPITX_DSI_DBG_CON;	/* 0090 */
	uint32_t MIPITX_DSI_DBG_OUT;	/* 0084 */
	MIPITX_DSI_APB_ASYNC_STA_REG MIPITX_DSI_APB_ASYNC_STA;	/* 0098 */

} volatile DSI_PHY_REGS, *PDSI_PHY_REGS;

/*
#ifndef BUILD_LK

STATIC_ASSERT(0x0050 == offsetof(DSI_PHY_REGS, MIPITX_DSI_PLL_CON0));
STATIC_ASSERT(0x0070 == offsetof(DSI_PHY_REGS, MIPITX_DSI_RGS));
STATIC_ASSERT(0x0080 == offsetof(DSI_PHY_REGS, MIPITX_DSI_SW_CTRL_EN));
STATIC_ASSERT(0x0090 == offsetof(DSI_PHY_REGS, MIPITX_DSI_DBG_CON));

STATIC_ASSERT(0x002C == offsetof(DSI_REGS, DSI_VACT_NL));
STATIC_ASSERT(0x0104 == offsetof(DSI_REGS, DSI_PHY_LCCON));
STATIC_ASSERT(0x011C == offsetof(DSI_REGS, DSI_PHY_TIMECON3));
STATIC_ASSERT(0x017C == offsetof(DSI_REGS, DSI_BIST_CON));
STATIC_ASSERT(0x0100 == offsetof(DSI_REGS, DSI_PHY_PCPAT));

STATIC_ASSERT((uint32_t)0x0098 == offsetof(DSI_REGS, DSI_3D_CON));
#endif
*/
/* mipi and dsi's extern variable */
extern volatile unsigned long dispsys_reg[DISP_REG_NUM];
extern unsigned long ddp_reg_pa_base[DISP_REG_NUM];
extern volatile unsigned long mipi_tx_reg;
extern volatile unsigned long dsi_reg_va;

/* DTS will assign reigister address dynamically, so can not define to 0x1000 */
/* #define DISP_INDEX_OFFSET 0x1000 */

#define DISP_RDMA_INDEX_OFFSET  (dispsys_reg[DISP_REG_RDMA1] - dispsys_reg[DISP_REG_RDMA0])
#define DISP_WDMA_INDEX_OFFSET  (dispsys_reg[DISP_REG_WDMA1] - dispsys_reg[DISP_REG_WDMA0])

#define DDP_REG_BASE_MMSYS_CONFIG  dispsys_reg[DISP_REG_CONFIG]
#define DDP_REG_BASE_DISP_OVL0     dispsys_reg[DISP_REG_OVL0]
#define DDP_REG_BASE_DISP_OVL1     dispsys_reg[DISP_REG_OVL1]
#define DDP_REG_BASE_DISP_RDMA0    dispsys_reg[DISP_REG_RDMA0]
#define DDP_REG_BASE_DISP_RDMA1    dispsys_reg[DISP_REG_RDMA1]
#define DDP_REG_BASE_DISP_WDMA0    dispsys_reg[DISP_REG_WDMA0]
#define DDP_REG_BASE_DISP_WDMA1    dispsys_reg[DISP_REG_WDMA1]
#define DDP_REG_BASE_DISP_COLOR0   dispsys_reg[DISP_REG_COLOR]
#define DDP_REG_BASE_DISP_COLOR1   0
#define DDP_REG_BASE_DISP_AAL      dispsys_reg[DISP_REG_AAL]
#define DDP_REG_BASE_DISP_GAMMA    dispsys_reg[DISP_REG_GAMMA]
#define DDP_REG_BASE_DISP_MERGE    0
#define DDP_REG_BASE_DISP_SPLIT0   0
#define DDP_REG_BASE_DISP_SPLIT1   0
#define DDP_REG_BASE_DISP_UFOE     0
#define DDP_REG_BASE_DSI0          dispsys_reg[DISP_REG_DSI0]
#define DDP_REG_BASE_DSI1          0
#define DDP_REG_BASE_DPI           dispsys_reg[DISP_REG_DPI0]
#define DDP_REG_BASE_DISP_PWM0     dispsys_reg[DISP_REG_PWM]
#define DDP_REG_BASE_DISP_PWM1     0
#define DDP_REG_BASE_MM_MUTEX      dispsys_reg[DISP_REG_MUTEX]
#define DDP_REG_BASE_SMI_LARB0     dispsys_reg[DISP_REG_SMI_LARB0]
#define DDP_REG_BASE_SMI_COMMON    dispsys_reg[DISP_REG_SMI_COMMON]
#define DDP_REG_BASE_DISP_OD       0
#define DDP_REG_BASE_DISP_CCORR    dispsys_reg[DISP_REG_CCORR]
#define DDP_REG_BASE_DISP_DITHER   dispsys_reg[DISP_REG_DITHER]
#define DDP_REG_BASE_DISP_OVL0_2L  dispsys_reg[DISP_REG_OVL0_2L]
#define DDP_REG_BASE_DISP_OVL1_2L  dispsys_reg[DISP_REG_OVL1_2L]

#define MIPI_TX_REG_BASE			(mipi_tx_reg)

#define DISPSYS_REG_ADDR_MIN            (DISPSYS_CONFIG_BASE)
#define DISPSYS_REG_ADDR_MAX            (DDP_REG_BASE_DISP_OVL1_2L)

#define DISPSYS_CONFIG_BASE			DDP_REG_BASE_MMSYS_CONFIG
#define DISPSYS_OVL0_BASE		        DDP_REG_BASE_DISP_OVL0
#define DISPSYS_OVL1_BASE		        DDP_REG_BASE_DISP_OVL1
#define DISPSYS_RDMA0_BASE		        DDP_REG_BASE_DISP_RDMA0
#define DISPSYS_RDMA1_BASE		        DDP_REG_BASE_DISP_RDMA1
#define DISPSYS_WDMA0_BASE		        DDP_REG_BASE_DISP_WDMA0
#define DISPSYS_WDMA1_BASE		        DDP_REG_BASE_DISP_WDMA1
#define DISPSYS_COLOR0_BASE		        DDP_REG_BASE_DISP_COLOR0
#define DISPSYS_COLOR1_BASE		        DDP_REG_BASE_DISP_COLOR1
#define DISPSYS_AAL_BASE		        DDP_REG_BASE_DISP_AAL
#define DISPSYS_GAMMA_BASE		        DDP_REG_BASE_DISP_GAMMA
#define DISPSYS_MERGE_BASE			DDP_REG_BASE_DISP_MERGE
#define DISPSYS_SPLIT0_BASE		        DDP_REG_BASE_DISP_SPLIT0
#define DISPSYS_SPLIT1_BASE		        DDP_REG_BASE_DISP_SPLIT1
#define DISPSYS_UFOE_BASE		        DDP_REG_BASE_DISP_UFOE
#define DISPSYS_DSI0_BASE		        DDP_REG_BASE_DSI0
#define DISPSYS_DITHER_BASE		        DDP_REG_BASE_DISP_DITHER
#define DISPSYS_DSI1_BASE		        DDP_REG_BASE_DSI1
#define DISPSYS_DPI_BASE			DDP_REG_BASE_DPI
#define DISPSYS_PWM0_BASE		        DDP_REG_BASE_DISP_PWM0
#define DISPSYS_PWM1_BASE		        DDP_REG_BASE_DISP_PWM1
#define DISPSYS_MUTEX_BASE			DDP_REG_BASE_MM_MUTEX
#define DISPSYS_SMI_LARB0_BASE		    DDP_REG_BASE_SMI_LARB0
#define DISPSYS_SMI_COMMON_BASE		    DDP_REG_BASE_SMI_COMMON
#define DISPSYS_OD_BASE                 DDP_REG_BASE_DISP_OD
#define DISPSYS_CCORR_BASE		        DDP_REG_BASE_DISP_CCORR


#define MIPITX_BASE					    MIPI_TX_REG_BASE

#ifdef INREG32
#undef INREG32
#define INREG32(x)          (__raw_readl((volatile unsigned long*)(x)))
#endif

/* --------------------------------------------------------------------------- */
/* Register Field Access */
/* --------------------------------------------------------------------------- */

#define READ_REGISTER_UINT32(reg)	(*(volatile uint32_t *const)(reg))
#define INREG32(x)	READ_REGISTER_UINT32((uint32_t *)((void *)(x)))
#define DRV_Reg32(addr)	INREG32(addr)
#define WRITE_REGISTER_UINT32(reg, val)	((*(volatile uint32_t *const)(reg)) = (val))
#define OUTREG32(x, y)	WRITE_REGISTER_UINT32((uint32_t *)((void *)(x)), (uint32_t)(y))
#define AS_UINT32(x)	(*(uint32_t *)((void *)x))

#define REG_FLD(width, shift) \
	((unsigned int)((((width) & 0xFF) << 16) | ((shift) & 0xFF)))

#define REG_FLD_MSB_LSB(msb, lsb) REG_FLD((msb) - (lsb) + 1, (lsb))

#define REG_FLD_WIDTH(field) \
	((unsigned int)(((field) >> 16) & 0xFF))

#define REG_FLD_SHIFT(field) \
	((unsigned int)((field) & 0xFF))

#define REG_FLD_MASK(field) \
	((unsigned int)((1ULL << REG_FLD_WIDTH(field)) - 1) << REG_FLD_SHIFT(field))

#define REG_FLD_VAL(field, val) \
	(((val) << REG_FLD_SHIFT(field)) & REG_FLD_MASK(field))

#define REG_FLD_VAL_GET(field, regval) \
	(((regval) & REG_FLD_MASK(field)) >> REG_FLD_SHIFT(field))


#define DISP_REG_GET(reg32) __raw_readl((volatile unsigned int*)(reg32))
#define DISP_REG_GET_FIELD(field, reg32) \
	REG_FLD_VAL_GET(field, __raw_readl((volatile unsigned int*)(reg32)))

/* polling register until masked bit is 1 */
#define DDP_REG_POLLING(reg32, mask) \
	do { \
		while (!((DISP_REG_GET(reg32))&mask))\
			; \
	} while (0)

/* Polling register until masked bit is 0 */
#define DDP_REG_POLLING_NEG(reg32, mask) \
	do { \
		while ((DISP_REG_GET(reg32))&mask)\
			; \
	} while (0)

#define DISP_CPU_REG_SET(reg32, val) \
	do {\
		if (0) \
			dprec_reg_op(NULL, reg32, val, 0x00000000);\
		mt_reg_sync_writel(val, (volatile unsigned long*)(reg32));\
	} while (0)
#define DISP_CPU_REG_SET_FIELD(field, reg32, val)  \
	do {                                \
		if (0)\
			dprec_reg_op(NULL, (unsigned long)(reg32), val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		mt_reg_sync_writel(\
			((unsigned int)(*(volatile unsigned long *)(reg32))&~REG_FLD_MASK(field))|REG_FLD_VAL((field),\
			(val)),\
			(volatile unsigned long *)reg32);  \
	} while (0)

/* after apply device tree va/pa is not mapped by a fixed offset */
static inline unsigned long disp_addr_convert(unsigned long va)
{
	unsigned int i = 0;
	for (i = 0; i < DISP_REG_NUM; i++) {
		if (dispsys_reg[i] == (va & (~0xfffl)))
			return ddp_reg_pa_base[i] + (va & 0xfffl);
	}
	pr_err("DDP/can not find reg addr for va=0x%lx!\n", va);
	BUG();
	return 0;
}


#define DISP_REG_MASK(handle, reg32, val, mask)	\
	do { \
		dprec_reg_op((void *)handle, (unsigned long)reg32, val, mask);\
		 if (handle == NULL) \
			mt_reg_sync_writel((unsigned int)(INREG32(reg32)&~(mask))|(val), \
				(volatile unsigned long *)(reg32));\
		 else \
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, mask); \
	} while (0)

#define DISP_REG_SET(handle, reg32, val) \
	do { \
		if (handle == NULL) { \
			mt_reg_sync_writel(val, (volatile unsigned long*)(reg32));\
		} else { \
			dprec_reg_op((void *)handle, (unsigned long)(reg32), val, 0x00000000);\
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, ~0); \
		}  \
	} while (0)


#define DISP_REG_SET_FIELD(handle, field, reg32, val)  \
	do {  \
		if (handle == NULL) { \
			mt_reg_sync_writel(\
			(__raw_readl((volatile unsigned long *)(reg32)) & ~REG_FLD_MASK(field))|REG_FLD_VAL((field),\
			(val)),\
			(volatile unsigned long *)(reg32));  \
		} else { \
			dprec_reg_op(handle, (unsigned long)(reg32), val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
			cmdqRecWrite(handle, disp_addr_convert((unsigned long)(reg32)),\
					val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		} \
	} while (0)

#define DISP_REG_CMDQ_POLLING(handle, reg32, val, mask) \
	do { \
		if (handle == NULL) { \
			while (!((DISP_REG_GET(reg32))&val))\
				; \
		} else { \
			dprec_reg_op(handle, (unsigned long)(reg32), val, 0x00000000);\
			cmdqRecPoll(handle, disp_addr_convert((unsigned long)(reg32)), val, mask); \
		}  \
	} while (0)

#define DISP_REG_BACKUP(handle, hSlot, idx, reg32) \
	do { \
		if (handle != NULL) \
			if (hSlot) \
				cmdqRecBackupRegisterToSlot(handle,\
							hSlot, idx, disp_addr_convert((unsigned long)reg32));\
	} while (0)

/* field definition */
/* ------------------------------------------------------------- */
/* AAL */
#define DISP_AAL_EN                             (DISPSYS_AAL_BASE + 0x000)
#define DISP_AAL_RESET                          (DISPSYS_AAL_BASE + 0x004)
#define DISP_AAL_INTEN                          (DISPSYS_AAL_BASE + 0x008)
#define DISP_AAL_INTSTA                         (DISPSYS_AAL_BASE + 0x00c)
#define DISP_AAL_STATUS                         (DISPSYS_AAL_BASE + 0x010)
#define DISP_AAL_CFG                            (DISPSYS_AAL_BASE + 0x020)
#define DISP_AAL_IN_CNT                         (DISPSYS_AAL_BASE + 0x024)
#define DISP_AAL_OUT_CNT                        (DISPSYS_AAL_BASE + 0x028)
#define DISP_AAL_CHKSUM                         (DISPSYS_AAL_BASE + 0x02c)
#define DISP_AAL_SIZE                           (DISPSYS_AAL_BASE + 0x030)
#define DISP_AAL_DUMMY_REG                      (DISPSYS_AAL_BASE + 0x0c0)
#define DISP_AAL_MAX_HIST_CONFIG_00             (DISPSYS_AAL_BASE + 0x204)
#define DISP_AAL_CABC_00                        (DISPSYS_AAL_BASE + 0x20c)
#define DISP_AAL_CABC_02                        (DISPSYS_AAL_BASE + 0x214)
#define DISP_AAL_CABC_04                        (DISPSYS_AAL_BASE + 0x21c)
#define DISP_AAL_STATUS_00                      (DISPSYS_AAL_BASE + 0x224)
/* 00 ~ 32: max histogram */
#define DISP_AAL_STATUS_32                      (DISPSYS_AAL_BASE + 0x2a4)
/* bit 8: dre_gain_force_en */
#define DISP_AAL_DRE_GAIN_FILTER_00             (DISPSYS_AAL_BASE + 0x354)
#define DISP_AAL_DRE_FLT_FORCE(idx)             (DISPSYS_AAL_BASE + 0x358 + (idx) * 4)
#define DISP_AAL_DRE_CRV_CAL_00                 (DISPSYS_AAL_BASE + 0x344)
#define DISP_AAL_DRE_MAPPING_00                 (DISPSYS_AAL_BASE + 0x3b0)
#define DISP_AAL_CABC_GAINLMT_TBL(idx)          (DISPSYS_AAL_BASE + 0x40c + (idx) * 4)


#define DISP_PWM_EN_OFF                         (0x00)
#define DISP_PWM_COMMIT_OFF                     (0x08)
#define DISP_PWM_CON_0_OFF                      (0x10)
#define DISP_PWM_CON_1_OFF                      (0x14)


/* field definition */
/* ------------------------------------------------------------- */
/* DISP OD */
#define DISP_OD_EN                              (DISPSYS_OD_BASE + 0x000)
#define DISP_OD_RESET                           (DISPSYS_OD_BASE + 0x004)
#define DISP_OD_INTEN                           (DISPSYS_OD_BASE + 0x008)
#define DISP_OD_INTS                            (DISPSYS_OD_BASE + 0x00C)
#define DISP_OD_STATUS                          (DISPSYS_OD_BASE + 0x010)
#define DISP_OD_CFG                             (DISPSYS_OD_BASE + 0x020)
#define DISP_OD_INPUT_COUNT                     (DISPSYS_OD_BASE + 0x024)
#define DISP_OD_OUTPUT_COUNT                    (DISPSYS_OD_BASE + 0x028)
#define DISP_OD_CHKS_UM                         (DISPSYS_OD_BASE + 0x02c)
#define DISP_OD_SIZE                            (DISPSYS_OD_BASE + 0x030)
#define DISP_OD_HSYNC_WIDTH                     (DISPSYS_OD_BASE + 0x040)
#define DISP_OD_VSYNC_WIDTH                     (DISPSYS_OD_BASE + 0x044)
#define DISP_OD_MISC                            (DISPSYS_OD_BASE + 0x048)
/* field definition */
/* ------------------------------------------------------------- */
/* COLOR */
#define CFG_MAIN_FLD_M_REG_RESET                 REG_FLD(1, 31)
#define CFG_MAIN_FLD_M_DISP_RESET                REG_FLD(1, 30)
#define CFG_MAIN_FLD_COLOR_DBUF_EN               REG_FLD(1, 29)
#define CFG_MAIN_FLD_C_PP_CM_DBG_SEL             REG_FLD(4, 16)
#define CFG_MAIN_FLD_SEQ_SEL                     REG_FLD(1, 13)
#define CFG_MAIN_FLD_ALLBP                       REG_FLD(1, 7)
#define CFG_MAIN_FLD_HEBP                        REG_FLD(1, 4)
#define CFG_MAIN_FLD_SEBP                        REG_FLD(1, 3)
#define CFG_MAIN_FLD_YEBP                        REG_FLD(1, 2)
#define CFG_MAIN_FLD_P2CBP                       REG_FLD(1, 1)
#define CFG_MAIN_FLD_C2PBP                       REG_FLD(1, 0)
#define START_FLD_DISP_COLOR_START               REG_FLD(1, 0)

#define DISP_COLOR_CFG_MAIN             (DISPSYS_COLOR0_BASE+0x400)
#define DISP_COLOR_PXL_CNT_MAIN         (DISPSYS_COLOR0_BASE+0x404)
#define DISP_COLOR_LINE_CNT_MAIN        (DISPSYS_COLOR0_BASE+0x408)
#define DISP_COLOR_WIN_X_MAIN           (DISPSYS_COLOR0_BASE+0x40C)
#define DISP_COLOR_WIN_Y_MAIN           (DISPSYS_COLOR0_BASE+0x410)
#define DISP_COLOR_TIMING_DETECTION_0   (DISPSYS_COLOR0_BASE+0x418)
#define DISP_COLOR_TIMING_DETECTION_1   (DISPSYS_COLOR0_BASE+0x41c)
#define DISP_COLOR_DBG_CFG_MAIN         (DISPSYS_COLOR0_BASE+0x420)
#define DISP_COLOR_C_BOOST_MAIN         (DISPSYS_COLOR0_BASE+0x428)
#define DISP_COLOR_C_BOOST_MAIN_2       (DISPSYS_COLOR0_BASE+0x42C)
#define DISP_COLOR_LUMA_ADJ             (DISPSYS_COLOR0_BASE+0x430)
#define DISP_COLOR_G_PIC_ADJ_MAIN_1     (DISPSYS_COLOR0_BASE+0x434)
#define DISP_COLOR_G_PIC_ADJ_MAIN_2     (DISPSYS_COLOR0_BASE+0x438)
#define DISP_COLOR_POS_MAIN             (DISPSYS_COLOR0_BASE+0x484)
#define DISP_COLOR_INK_DATA_MAIN        (DISPSYS_COLOR0_BASE+0x488)
#define DISP_COLOR_INK_DATA_MAIN_CR     (DISPSYS_COLOR0_BASE+0x48c)
#define DISP_COLOR_CAP_IN_DATA_MAIN     (DISPSYS_COLOR0_BASE+0x490)
#define DISP_COLOR_CAP_IN_DATA_MAIN_CR  (DISPSYS_COLOR0_BASE+0x494)
#define DISP_COLOR_CAP_OUT_DATA_MAIN    (DISPSYS_COLOR0_BASE+0x498)
#define DISP_COLOR_CAP_OUT_DATA_MAIN_CR (DISPSYS_COLOR0_BASE+0x49c)
#define DISP_COLOR_Y_SLOPE_1_0_MAIN     (DISPSYS_COLOR0_BASE+0x4A0)
#define DISP_COLOR_LOCAL_HUE_CD_0       (DISPSYS_COLOR0_BASE+0x620)
#define DISP_COLOR_TWO_D_WINDOW_1       (DISPSYS_COLOR0_BASE+0x740)
#define DISP_COLOR_TWO_D_W1_RESULT      (DISPSYS_COLOR0_BASE+0x74C)
#define DISP_COLOR_SAT_HIST_X_CFG_MAIN  (DISPSYS_COLOR0_BASE+0x768)
#define DISP_COLOR_SAT_HIST_Y_CFG_MAIN  (DISPSYS_COLOR0_BASE+0x76c)
#define DISP_COLOR_BWS_2                (DISPSYS_COLOR0_BASE+0x79c)
#define DISP_COLOR_CRC_0                (DISPSYS_COLOR0_BASE+0x7e0)
#define DISP_COLOR_PART_SAT_GAIN1_0     (DISPSYS_COLOR0_BASE+0x7FC)
#define DISP_COLOR_PART_SAT_GAIN1_1     (DISPSYS_COLOR0_BASE+0x800)
#define DISP_COLOR_PART_SAT_GAIN1_2     (DISPSYS_COLOR0_BASE+0x804)
#define DISP_COLOR_PART_SAT_GAIN1_3     (DISPSYS_COLOR0_BASE+0x808)
#define DISP_COLOR_PART_SAT_GAIN1_4     (DISPSYS_COLOR0_BASE+0x80C)
#define DISP_COLOR_PART_SAT_GAIN2_0     (DISPSYS_COLOR0_BASE+0x810)
#define DISP_COLOR_PART_SAT_GAIN2_1     (DISPSYS_COLOR0_BASE+0x814)
#define DISP_COLOR_PART_SAT_GAIN2_2     (DISPSYS_COLOR0_BASE+0x818)
#define DISP_COLOR_PART_SAT_GAIN2_3	    (DISPSYS_COLOR0_BASE+0x81C)
#define DISP_COLOR_PART_SAT_GAIN2_4     (DISPSYS_COLOR0_BASE+0x820)
#define DISP_COLOR_PART_SAT_GAIN3_0     (DISPSYS_COLOR0_BASE+0x824)
#define DISP_COLOR_PART_SAT_GAIN3_1     (DISPSYS_COLOR0_BASE+0x828)
#define DISP_COLOR_PART_SAT_GAIN3_2     (DISPSYS_COLOR0_BASE+0x82C)
#define DISP_COLOR_PART_SAT_GAIN3_3     (DISPSYS_COLOR0_BASE+0x830)
#define DISP_COLOR_PART_SAT_GAIN3_4     (DISPSYS_COLOR0_BASE+0x834)
#define DISP_COLOR_PART_SAT_POINT1_0    (DISPSYS_COLOR0_BASE+0x838)
#define DISP_COLOR_PART_SAT_POINT1_1    (DISPSYS_COLOR0_BASE+0x83C)
#define DISP_COLOR_PART_SAT_POINT1_2    (DISPSYS_COLOR0_BASE+0x840)
#define DISP_COLOR_PART_SAT_POINT1_3    (DISPSYS_COLOR0_BASE+0x844)
#define DISP_COLOR_PART_SAT_POINT1_4    (DISPSYS_COLOR0_BASE+0x848)
#define DISP_COLOR_PART_SAT_POINT2_0    (DISPSYS_COLOR0_BASE+0x84C)
#define DISP_COLOR_PART_SAT_POINT2_1    (DISPSYS_COLOR0_BASE+0x850)
#define DISP_COLOR_PART_SAT_POINT2_2    (DISPSYS_COLOR0_BASE+0x854)
#define DISP_COLOR_PART_SAT_POINT2_3    (DISPSYS_COLOR0_BASE+0x858)
#define DISP_COLOR_PART_SAT_POINT2_4    (DISPSYS_COLOR0_BASE+0x85C)

#define DISP_COLOR_START                (DISPSYS_COLOR0_BASE+0xC00)
#define DISP_COLOR_INTEN                (DISPSYS_COLOR0_BASE+0xC04)
#define DISP_COLOR_OUT_SEL              (DISPSYS_COLOR0_BASE+0xC08)
#define DISP_COLOR_FRAME_DONE_DEL       (DISPSYS_COLOR0_BASE+0xC10)
#define DISP_COLOR_CRC                  (DISPSYS_COLOR0_BASE+0xC14)
#define DISP_COLOR_SW_SCRATCH           (DISPSYS_COLOR0_BASE+0xC18)
#define DISP_COLOR_CK_ON                (DISPSYS_COLOR0_BASE+0xC28)
#define DISP_COLOR_INTERNAL_IP_WIDTH    (DISPSYS_COLOR0_BASE+0xC50)
#define DISP_COLOR_INTERNAL_IP_HEIGHT   (DISPSYS_COLOR0_BASE+0xC54)
#define DISP_COLOR_CM1_EN               (DISPSYS_COLOR0_BASE+0xC60)
#define DISP_COLOR_CM2_EN               (DISPSYS_COLOR0_BASE+0xCA0)
#define DISP_COLOR_R0_CRC               (DISPSYS_COLOR0_BASE+0xCF0)

/* field definition */
/* ------------------------------------------------------------- */
/* Config */
#define DISP_REG_CONFIG_MMSYS_INTEN				(DISPSYS_CONFIG_BASE + 0x0)
#define DISP_REG_CONFIG_MMSYS_INTSTA				(DISPSYS_CONFIG_BASE + 0x4)
#define DISP_REG_CONFIG_MFG_APB_TX_CON					(DISPSYS_CONFIG_BASE + 0x8)
#define DISP_REG_CONFIG_PWM_APB_ERR_ADDR				(DISPSYS_CONFIG_BASE + 0xc)
#define DISP_REG_CONFIG_ISP_MOUT_EN				(DISPSYS_CONFIG_BASE + 0x01c)
#define DISP_REG_CONFIG_MDP_RDMA0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x020)
#define DISP_REG_CONFIG_MDP_PRZ0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x024)
#define DISP_REG_CONFIG_MDP_PRZ1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x028)
#define DISP_REG_CONFIG_MDP_TDSHP0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x02C)
#define DISP_REG_CONFIG_DISP_OVL0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x030)
#define DISP_REG_CONFIG_DISP_OVL1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x034)
#define DISP_REG_CONFIG_DISP_DITHER_MOUT_EN                     (DISPSYS_CONFIG_BASE + 0x038)
#define DISP_REG_CONFIG_DISP_UFOE_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x03C)
#define DISP_REG_CONFIG_MMSYS_MOUT_RST				(DISPSYS_CONFIG_BASE + 0x040)
#define DISP_REG_CONFIG_MDP_PRZ0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x044)
#define DISP_REG_CONFIG_MDP_PRZ1_SEL_IN				(DISPSYS_CONFIG_BASE + 0x048)
#define DISP_REG_CONFIG_MDP_TDSHP0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x04C)
#define DISP_REG_CONFIG_MDP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x050)
#define DISP_REG_CONFIG_MDP_WROT0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x054)
#define DISP_REG_CONFIG_DISP_COLOR0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x058)
#define DISP_REG_CONFIG_DISP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x05C)
#define DISP_REG_CONFIG_DISP_UFOE_SEL_IN			(DISPSYS_CONFIG_BASE + 0x060)
#define DISP_REG_CONFIG_DSI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x064)
#define DISP_REG_CONFIG_DPI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x068)
#define DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN                  (DISPSYS_CONFIG_BASE + 0x06C)
#define DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x070)
#define DISP_REG_CONFIG_DISP_OVL0_SOUT_SEL_IN                   (DISPSYS_CONFIG_BASE + 0x074)
#define DISP_REG_CONFIG_DISP_OVL0_SEL_IN		        (DISPSYS_CONFIG_BASE + 0x078)
#define DISP_REG_CONFIG_DISP_OVL1_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x07C)

#define DISP_REG_CONFIG_MMSYS_MISC				(DISPSYS_CONFIG_BASE + 0x0F0)
#define DISP_REG_CONFIG_MMSYS_CG_CON0				(DISPSYS_CONFIG_BASE + 0x100)
#define DISP_REG_CONFIG_MMSYS_CG_SET0				(DISPSYS_CONFIG_BASE + 0x104)
#define DISP_REG_CONFIG_MMSYS_CG_CLR0				(DISPSYS_CONFIG_BASE + 0x108)
#define DISP_REG_CONFIG_MMSYS_CG_CON1				(DISPSYS_CONFIG_BASE + 0x110)
#define DISP_REG_CONFIG_MMSYS_CG_SET1				(DISPSYS_CONFIG_BASE + 0x114)
#define DISP_REG_CONFIG_MMSYS_CG_CLR1				(DISPSYS_CONFIG_BASE + 0x118)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0			(DISPSYS_CONFIG_BASE + 0x120)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET0                   (DISPSYS_CONFIG_BASE + 0x124)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR0                   (DISPSYS_CONFIG_BASE + 0x128)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS1                       (DISPSYS_CONFIG_BASE + 0x130)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET1                   (DISPSYS_CONFIG_BASE + 0x134)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR1                   (DISPSYS_CONFIG_BASE + 0x138)
#define DISP_REG_CONFIG_MMSYS_SW0_RST_B				(DISPSYS_CONFIG_BASE + 0x140)
#define DISP_REG_CONFIG_MMSYS_SW1_RST_B				(DISPSYS_CONFIG_BASE + 0x144)
#define DISP_REG_CONFIG_MMSYS_LCM_RST_B				(DISPSYS_CONFIG_BASE + 0x150)
#define DISP_REG_CONFIG_MM_VDEC_SLICE_CFG                       (DISPSYS_CONFIG_BASE + 0x164)
#define DISP_REG_CONFIG_SMI_N21MUX_CFG_WR			(DISPSYS_CONFIG_BASE + 0x168)
#define DISP_REG_CONFIG_SMI_N21MUX_CFG_RD			(DISPSYS_CONFIG_BASE + 0x16c)
#define DISP_REG_CONFIG_ELA2GMC_BASE_ADDR			(DISPSYS_CONFIG_BASE + 0x170)
#define DISP_REG_CONFIG_ELA2GMC_BASE_ADDR_END                   (DISPSYS_CONFIG_BASE + 0x174)
#define DISP_REG_CONFIG_ELA2GMC_FINAL_ADDR			(DISPSYS_CONFIG_BASE + 0x178)
#define DISP_REG_CONFIG_ELA2GMC_STATUS				(DISPSYS_CONFIG_BASE + 0x17c)
#define DISP_REG_CONFIG_LARB4_AXI_ASIF_CFG_WD                   (DISPSYS_CONFIG_BASE + 0x180)
#define DISP_REG_CONFIG_LARB4_AXI_ASIF_CFG_RD                   (DISPSYS_CONFIG_BASE + 0x184)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_EN				(DISPSYS_CONFIG_BASE + 0x200)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RST			(DISPSYS_CONFIG_BASE + 0x204)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON0				(DISPSYS_CONFIG_BASE + 0x208)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON1				(DISPSYS_CONFIG_BASE + 0x20c)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RD_ADDR		    (DISPSYS_CONFIG_BASE + 0x210)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_WR_ADDR			    (DISPSYS_CONFIG_BASE + 0x214)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_STATE				(DISPSYS_CONFIG_BASE + 0x218)
#define DISP_REG_CONFIG_MMSYS_MBIST_CON				(DISPSYS_CONFIG_BASE + 0x800)
#define DISP_REG_CONFIG_MMSYS_MBIST_DONE			(DISPSYS_CONFIG_BASE + 0x804)
#define DISP_REG_CONFIG_MMSYS_MBIST_HOLDB			(DISPSYS_CONFIG_BASE + 0x808)
#define DISP_REG_CONFIG_MMSYS_MBIST_MODE			(DISPSYS_CONFIG_BASE + 0x80c)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL0			(DISPSYS_CONFIG_BASE + 0x810)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL1			(DISPSYS_CONFIG_BASE + 0x814)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL2			(DISPSYS_CONFIG_BASE + 0x818)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL0				(DISPSYS_CONFIG_BASE + 0x820)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL1				(DISPSYS_CONFIG_BASE + 0x824)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL0			(DISPSYS_CONFIG_BASE + 0x830)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL1			(DISPSYS_CONFIG_BASE + 0x834)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL2			(DISPSYS_CONFIG_BASE + 0x838)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL3			(DISPSYS_CONFIG_BASE + 0x83c)
#define DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL			(DISPSYS_CONFIG_BASE + 0x880)
#define DISP_REG_CONFIG_MMSYS_DUMMY0				(DISPSYS_CONFIG_BASE + 0x890)
#define DISP_REG_CONFIG_MMSYS_DUMMY1				(DISPSYS_CONFIG_BASE + 0x894)
#define DISP_REG_CONFIG_MMSYS_DUMMY2				(DISPSYS_CONFIG_BASE + 0x898)
#define DISP_REG_CONFIG_MMSYS_DUMMY3				(DISPSYS_CONFIG_BASE + 0x89C)

#define DISP_REG_CONFIG_DISP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8a0)
#define DISP_REG_CONFIG_DISP_DL_VALID_1				(DISPSYS_CONFIG_BASE + 0x8a4)
#define DISP_REG_CONFIG_DISP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8a8)
#define DISP_REG_CONFIG_DISP_DL_READY_1				(DISPSYS_CONFIG_BASE + 0x8ac)

#define DISP_REG_CONFIG_MDP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8b0)
#define DISP_REG_CONFIG_MDP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8b4)
#define DISP_REG_CONFIG_SMI_LARB0_GREQ				(DISPSYS_CONFIG_BASE + 0x8d0)

#define DISP_REG_CONFIG_C08					        (DISPSYS_CONFIG_BASE + 0xc08)
#define DISP_REG_CLK_CFG_0_MM_CLK                               (DISPSYS_CONFIG_BASE+0x40)
#define DISP_REG_CLK_CFG_0_CLR                                  (DISPSYS_CONFIG_BASE+0x48)
#define DISP_REG_CLK_CFG_1_CLR                                  (DISPSYS_CONFIG_BASE+0x58)
#define DISP_REG_CLK_CFG_6_DPI                                  (DISPSYS_CONFIG_BASE+0xA0)
#define DISP_REG_CLK_CFG_6_CLR                                  (DISPSYS_CONFIG_BASE+0xA8)
/* #define DISP_REG_VENCPLL_CON0                                        0xf0209260  // bit0 1 */

#define MMSYS_INTEN_FLD_MMSYS_INTEN				REG_FLD(8, 0)
#define MMSYS_INSTA_FLD_MMSYS_INSTA				REG_FLD(2, 0)
#define MFG_APB_TX_CON_FLD_MFG_APB_COUNTER_EN				REG_FLD(1, 31)
#define MFG_APB_TX_CON_FLD_MFG_APB_ERR_ADDR				REG_FLD(16, 0)
#define PWM_APB_ERR_ADDR_FLD_PWM0_APB_ERR_ADDR					REG_FLD(12, 0)
#define ISP_MOUT_EN_FLD_ISP_MOUT_EN				REG_FLD(2, 0)
#define MDP_RDMA0_MOUT_EN_FLD_MDP_RDMA0_MOUT_EN				REG_FLD(2, 0)
#define MDP_PRZ0_MOUT_EN_FLD_MDP_PRZ0_MOUT_EN			REG_FLD(3, 0)
#define MDP_PRZ1_MOUT_EN_FLD_MDP_PRZ1_MOUT_EN			REG_FLD(3, 0)
#define MDP_TDSHP0_MOUT_EN_FLD_MDP_TDSHP0_MOUT_EN		    REG_FLD(2, 0)
#define DISP_OVL0_MOUT_EN_FLD_DISP_OVL0_MOUT_EN                 REG_FLD(2, 0)
#define DISP_OVL1_MOUT_EN_FLD_DISP_OVL1_MOUT_EN					REG_FLD(2, 0)
#define DISP_DITHER_MOUT_EN_FLD_DISP_DITHER_MOUT_EN             REG_FLD(3, 0)
#define DISP_UFOE_MOUT_EN_FLD_DISP_UFOE_MOUT_EN					REG_FLD(3, 0)
#define MMSYS_MOUT_RST_FLD_MMSYS_MOUT_RST				REG_FLD(9, 0)
#define MDP_PRZ0_SEL_IN_FLD_MDP_PRZ0_SEL_IN			REG_FLD(1, 0)
#define MDP_PRZ1_SEL_IN_FLD_MDP_PRZ1_SEL_IN			REG_FLD(2, 0)
#define MDP_TDSHP0_SEL_IN_FLD_MDP_TDSHP0_SEL_IN                 REG_FLD(1, 0)
#define MDP_WDMA0_SEL_IN_FLD_MDP_WDMA0_SEL_IN				REG_FLD(2, 0)
#define MDP_WROT0_SEL_IN_FLD_MDP_WROT0_SEL_IN			REG_FLD(2, 0)
#define DISP_COLOR0_SEL_IN_FLD_DISP_COLOR0_SEL_IN               REG_FLD(1, 0)
#define DISP_WDMA0_SEL_IN_FLD_DISP_WDMA0_SEL_IN			REG_FLD(2, 0)
#define DISP_UFO_SEL_IN_FLD_DISP_UFO_SEL_IN			REG_FLD(1, 0)
#define DSI0_SEL_IN_FLD_DSI0_SEL_IN				REG_FLD(1, 0)
#define DPI0_SEL_IN_FLD_DPI0_SEL_IN                             REG_FLD(1, 0)
#define DISP_RDMA0_SOUT_SEL_IN_FLD_DISP_RDMA0_SOUT_SEL_IN       REG_FLD(1, 0)
#define DISP_RDMA1_SOUT_SEL_IN_FLD_DISP_RDMA1_SOUT_SEL_IN       REG_FLD(1, 0)
#define MMSYS_MISC_FLD_SMI_LARB0_TEST_MODE					    REG_FLD(1, 0)
#define MMSYS_CG_CON0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_SET0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CON1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_SET1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR1_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS0_FLD_DCM_DIS0                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS1_FLD_DCM_DIS1                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET0_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR0_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_SW0_RST_B_FLD_SW0_RST_B                           REG_FLD(32, 0)
#define MMSYS_SW1_RST_B_FLD_SW1_RST_B                           REG_FLD(32, 0)
#define MMSYS_LCM_RST_B_FLD_LCM_RST_B                           REG_FLD(1, 0)
#define MM_VDEC_SLICE_CFG_FLD_MM_VDEC_SLICE_CFG_WD              REG_FLD(3, 0)
#define MM_VDEC_SLICE_CFG_FLD_MM_VDEC_SLICE_CFG_RD              REG_FLD(4, 4)
#define SMI_N21MUX_CFG_WR_FLD_SMI_N21MUX_CFG_WR                 REG_FLD(32, 0)
#define SMI_N21MUX_CFG_RD_FLD_SMI_N21MUX_CFG_RD                 REG_FLD(32, 0)
#define ELA2GMC_BASE_ADDR_FLD_ELA2GMC_BASE_ADDR			REG_FLD(32, 0)
#define ELA2GMC_BASE_ADDR_END_FLD_ELA2GMC_BASE_ADDR_END		    REG_FLD(32, 0)
#define ELA2GMC_FINAL_ADDR_FLD_ELA2GMC_FINAL_ADDR		REG_FLD(32, 0)
#define ELA2GMC_STATUS_FLD_ELA2GMC_STATUS				REG_FLD(1, 0)
#define LARB4_AXI_ASIF_CFG_WD_FLD_LARB4_AXI_ASIF_CFG_WD         REG_FLD(32, 0)
#define LARB4_AXI_ASIF_CFG_RD_FLD_LARB4_AXI_ASIF_CFG_RD         REG_FLD(32, 0)
#define DISP_FAKE_ENG_EN_FLD_DFE_START				REG_FLD(1, 1)
#define DISP_FAKE_ENG_EN_FLD_DFE_MUX_SEL			REG_FLD(1, 0)
#define DISP_FAKE_ENG_RST_FLD_DISP_FAKE_ENG_RST				REG_FLD(1, 0)
#define DISP_FAKE_ENG_CON0_FLD_DFE_DRE_EN			REG_FLD(1, 23)
#define DISP_FAKE_ENG_CON0_FLD_DFE_LOOP_MODE				REG_FLD(1, 22)
#define DISP_FAKE_ENG_CON0_FLD_DFE_TEST_LEN			REG_FLD(20, 0)
#define DISP_FAKE_ENG_CON1_FLD_DFE_WR_DIS			REG_FLD(1, 11)
#define DISP_FAKE_ENG_CON1_FLD_DFE_RD_DIS			REG_FLD(1, 10)
#define DISP_FAKE_ENG_CON1_FLD_DFE_SLOW_DOWN				REG_FLD(10, 0)
#define DISP_FAKE_ENG_RD_ADDR_FLD_DISP_FAKE_ENG_RD_ADDR			REG_FLD(32, 0)
#define DISP_FAKE_ENG_WR_ADDR_FLD_DISP_FAKE_ENG_WR_ADDR			REG_FLD(32, 0)
#define DISP_FAKE_ENG_STATE_FLD_DFE_RD_ST			REG_FLD(3, 12)
#define DISP_FAKE_ENG_STATE_FLD_DFE_WR_ST			REG_FLD(4, 8)
#define DISP_FAKE_ENG_STATE_FLD_DFE_BUSY			REG_FLD(1, 0)

#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_BACKGROUND			REG_FLD(3, 16)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_RSTB			REG_FLD(1, 15)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_SCANOUT_SEL			REG_FLD(4, 8)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_DEBUG				REG_FLD(1, 7)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_FAILOUT_SEL			REG_FLD(6, 0)
#define MMSYS_MBIST_DONE_FLD_MMSYS_MBIST_DONE				REG_FLD(14, 0)
#define MMSYS_MBIST_HOLDB_FLD_MMSYS_MBIST_HOLDB			REG_FLD(14, 0)
#define MMSYS_MBIST_MODE_FLD_MMSYS_MBIST_MODE				REG_FLD(14, 0)
#define MMSYS_MBIST_FAIL0_FLD_MMSYS_MBIST_FAIL0				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL1_FLD_MMSYS_MBIST_FAIL1				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL2_FLD_MMSYS_MBIST_FAIL2				REG_FLD(24, 0)
#define MMSYS_MBIST_BSEL0_FLD_MDP_TDSHP_MBIST_BSEL			REG_FLD(16, 0)
#define MMSYS_MBIST_BSEL0_FLD_MDP_RDMA_MBIST_BSEL		REG_FLD(8, 16)
#define MMSYS_MBIST_BSEL0_FLD_MDP_WROT_MBIST_BSEL		REG_FLD(8, 24)
#define MMSYS_MBIST_BSEL1_FLD_MDP_RSZ0_MBIST_BSEL			REG_FLD(12, 0)
#define MMSYS_MBIST_BSEL1_FLD_MDP_RSZ1_MBIST_BSEL			REG_FLD(12, 12)
#define MMSYS_MBIST_BSEL1_FLD_DISP_UFOE_MBIST_BSEL		REG_FLD(6, 24)
#define MMSYS_MEM_DELSEL0_FLD_MDP_RSZ_MEM_DELSEL                REG_FLD(16, 0)
#define MMSYS_MEM_DELSEL0_FLD_MDP_RDMA_MEM_DELSEL               REG_FLD(14, 16)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WROT_MEM_DELSEL				REG_FLD(22, 0)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WDMA_MEM_DELSEL               REG_FLD(6, 22)
#define MMSYS_MEM_DELSEL1_FLD_MDP_TDSHP_MEM_DELSEL              REG_FLD(4, 28)


#define MMSYS_MEM_DELSEL2_FLD_DISP_OVL_MEM_DELSEL               REG_FLD(4, 0)
#define MMSYS_MEM_DELSEL2_FLD_DISP_RDMA_MEM_DELSEL              REG_FLD(4, 4)
#define MMSYS_MEM_DELSEL2_FLD_DISP_WDMA_MEM_DELSEL              REG_FLD(6, 8)
#define MMSYS_MEM_DELSEL2_FLD_DISP_GAMM_MEM_DELSEL              REG_FLD(4, 14)
#define MMSYS_MEM_DELSEL2_FLD_DISP_UFOE_MEM_DELSEL              REG_FLD(12, 18)

#define MMSYS_MEM_DELSEL3_FLD_DSI_MEM_DELSEL                    REG_FLD(4, 0)

#define MMSYS_DEBUG_OUT_SEL_FLD_MMSYS_DEBUG_OUT_SEL             REG_FLD(5, 0)
#define MMSYS_DUMMY0_FLD_MMSYS_DUMMY0                           REG_FLD(32, 0)
#define MMSYS_DUMMY1_FLD_MMSYS_DUMMY1                           REG_FLD(32, 0)
#define MMSYS_DUMMY2_FLD_MMSYS_DUMMY2                           REG_FLD(32, 0)
#define MMSYS_DUMMY3_FLD_MMSYS_DUMMY3                           REG_FLD(32, 0)
#define DISP_DL_VALID_0_FLD_DISP_DL_VALID_0                     REG_FLD(32, 0)
#define DISP_DL_VALID_1_FLD_DISP_DL_VALID_1                     REG_FLD(32, 0)
#define DISP_DL_READY_0_FLD_DISP_DL_READY_0                     REG_FLD(32, 0)
#define DISP_DL_READY_1_FLD_DISP_DL_READY_1                     REG_FLD(32, 0)
#define MDP_DL_VALID_0_FLD_MDP_DL_VALID_0                       REG_FLD(32, 0)
#define MDP_DL_READY_0_FLD_MDP_DL_READY_0                       REG_FLD(32, 0)
#define SMI_LARB0_GREQ_FLD_SMI_LARB0_GREQ                       REG_FLD(14, 0)
/* field definition */
/* ------------------------------------------------------------- */
/* DPI */
#define DISP_REG_DPI_EN								(DISPSYS_DPI_BASE + 0x000)
#define DISP_REG_DPI_RST							(DISPSYS_DPI_BASE + 0x004)
#define DISP_REG_DPI_INTEN							(DISPSYS_DPI_BASE + 0x008)
#define DISP_REG_DPI_INSTA							(DISPSYS_DPI_BASE + 0x00C)
#define DISP_REG_DPI_CON							(DISPSYS_DPI_BASE + 0x010)
#define DISP_REG_DPI_OUTPUT_SETTING					(DISPSYS_DPI_BASE + 0x014)
#define DISP_REG_DPI_SIZE							(DISPSYS_DPI_BASE + 0x018)
#define DISP_REG_DPI_DDR_SETTING					(DISPSYS_DPI_BASE + 0x01c)
#define DISP_REG_DPI_TGEN_HWIDTH					(DISPSYS_DPI_BASE + 0x020)
#define DISP_REG_DPI_TGEN_HPORCH					(DISPSYS_DPI_BASE + 0x024)
#define DISP_REG_DPI_TGEN_VWIDTH					(DISPSYS_DPI_BASE + 0x028)
#define DISP_REG_DPI_TGEN_VPORCH					(DISPSYS_DPI_BASE + 0x02C)
#define DISP_REG_DPI_BG_HCNTL						(DISPSYS_DPI_BASE + 0x030)
#define DISP_REG_DPI_BG_VCNTL						(DISPSYS_DPI_BASE + 0x034)
#define DISP_REG_DPI_BG_COLOR						(DISPSYS_DPI_BASE + 0x038)
#define DISP_REG_DPI_FIFO_CTL						(DISPSYS_DPI_BASE + 0x03C)
#define DISP_REG_DPI_STATUS							(DISPSYS_DPI_BASE + 0x040)
#define DISP_REG_DPI_TMODE							(DISPSYS_DPI_BASE + 0x044)
#define DISP_REG_DPI_CHKSUM							(DISPSYS_DPI_BASE + 0x048)
#define DISP_REG_DPI_DUMMY							(DISPSYS_DPI_BASE + 0x050)
#define DISP_REG_DPI_TGEN_VWIDTH_LEVEN				(DISPSYS_DPI_BASE + 0x068)
#define DISP_REG_DPI_TGEN_VPORCH_LEVEN				(DISPSYS_DPI_BASE + 0x06c)
#define DISP_REG_DPI_TGEN_VWIDTH_RODD				(DISPSYS_DPI_BASE + 0x070)
#define DISP_REG_DPI_TGEN_VPORCH_RODD				(DISPSYS_DPI_BASE + 0x074)
#define DISP_REG_DPI_TGEN_VWIDTH_REVEN				(DISPSYS_DPI_BASE + 0x078)
#define DISP_REG_DPI_TGEN_VPORCH_REVEN				(DISPSYS_DPI_BASE + 0x07c)
#define DISP_REG_DPI_ESAV_VTIM_L					(DISPSYS_DPI_BASE + 0x080)
#define DISP_REG_DPI_ESAV_VTIM_R					(DISPSYS_DPI_BASE + 0x084)
#define DISP_REG_DPI_CLPF_SETTING					(DISPSYS_DPI_BASE + 0x08c)
#define DISP_REG_DPI_Y_LIMIT						(DISPSYS_DPI_BASE + 0x090)
#define DISP_REG_DPI_C_LIMIT						(DISPSYS_DPI_BASE + 0x094)
#define DISP_REG_DPI_YUV422_SETTING					(DISPSYS_DPI_BASE + 0x098)
#define DISP_REG_DPI_EMBSYNC_SETTING				(DISPSYS_DPI_BASE + 0x09c)
#define DISP_REG_DPI_ESAV_CODE_SET0					(DISPSYS_DPI_BASE + 0x0a8)
#define DISP_REG_DPI_ESAV_CODE_SET1					(DISPSYS_DPI_BASE + 0x0ac)
#define DISP_REG_DPI_BLANK_CODE_SET					(DISPSYS_DPI_BASE + 0x0b0)
#define DISP_REG_DPI_MATRIX_SET						(DISPSYS_DPI_BASE + 0x0b4)

#define EN_FLD_EN							REG_FLD(1, 0)
#define RST_FLD_RST						REG_FLD(1, 0)
#define INTEN_FLD_INT_UNDERFLOW_EN				REG_FLD(1, 2)
#define INTEN_FLD_INT_VDE_EN					REG_FLD(1, 1)
#define INTEN_FLD_INT_VSYNC_EN					REG_FLD(1, 0)
#define INSTA_FLD_INTSTA_UNDERFLOW_EN				REG_FLD(1, 2)
#define INSTA_FLD_INTSTA_VDE_EN					REG_FLD(1, 1)
#define INSTA_FLD_INTSTA_VSYNC_EN				REG_FLD(1, 0)
#define CON_FLD_IN_RB_SWAP					REG_FLD(1, 1)
#define CON_FLD_BG_ENABLE						REG_FLD(1, 0)
#define OUTPUT_SETTING_FLD_EDGE_SEL					        REG_FLD(1, 17)
#define OUTPUT_SETTING_FLD_OEN_OFF				REG_FLD(1, 16)
#define OUTPUT_SETTING_FLD_CK_POL				REG_FLD(1, 15)
#define OUTPUT_SETTING_FLD_VSYNC_POL				REG_FLD(1, 14)
#define OUTPUT_SETTING_FLD_HSYNC_POL				REG_FLD(1, 13)
#define OUTPUT_SETTING_FLD_DE_POL				REG_FLD(1, 12)
#define OUTPUT_SETTING_FLD_VS_MASK				REG_FLD(1, 10)
#define OUTPUT_SETTING_FLD_HS_MASK				REG_FLD(1, 9)
#define OUTPUT_SETTING_FLD_DE_MASK				REG_FLD(1, 8)
#define OUTPUT_SETTING_FLD_R_MASK				REG_FLD(1, 6)
#define OUTPUT_SETTING_FLD_G_MASK				REG_FLD(1, 5)
#define OUTPUT_SETTING_FLD_B_MASK				REG_FLD(1, 4)
#define OUTPUT_SETTING_FLD_BIT_SWAP			REG_FLD(1, 3)
#define OUTPUT_SETTING_FLD_CH_SWAP					REG_FLD(3, 0)
#define DPI_SIZE_FLD_HSIZE							REG_FLD(11, 16)
#define DPI_SIZE_FLD_VSIZE							REG_FLD(11, 0)
#define TGEN_HWIDTH_FLD_HPW							REG_FLD(12, 0)
#define TGEN_HPORCH_FLD_HFP							REG_FLD(12, 16)
#define TGEN_HPORCH_FLD_HBP							REG_FLD(12, 0)
#define TGEN_VWIDTH_FLD_VPW							REG_FLD(8, 0)
#define TGEN_VPORCH_FLD_VFP							REG_FLD(8, 16)
#define TGEN_VPORCH_FLD_VBP							REG_FLD(8, 0)
#define BG_HCNTL_FLD_BG_LEFT						REG_FLD(11, 16)
#define BG_HCNTL_FLD_BG_RIGHT						REG_FLD(11, 0)
#define BG_VCNTL_FLD_BG_TOP							REG_FLD(11, 16)
#define BG_VCNTL_FLD_BG_BOT							REG_FLD(11, 0)
#define BG_COLOR_FLD_BG_R							REG_FLD(8, 16)
#define BG_COLOR_FLD_BG_G							REG_FLD(8, 8)
#define BG_COLOR_FLD_BG_B							REG_FLD(8, 0)
#define FIFO_CTL_FLD_FIFO_RST_SEL					REG_FLD(1, 8)
#define FIFO_CTL_FLD_FIFO_VALID_SET					REG_FLD(5, 0)
#define STATUS_FLD_OUTEN						REG_FLD(1, 17)
#define STATUS_FLD_DPI_BUSY							REG_FLD(1, 16)
#define STATUS_FLD_V_COUNTER						REG_FLD(13, 0)
#define TMODE_FLD_DPI_OEN_ON						REG_FLD(1, 0)
#define CHKSUM_FLD_DPI_CHKSUM_EN				REG_FLD(1, 31)
#define CHKSUM_FLD_DPI_CHKSUM_READY				    REG_FLD(1, 30)
#define CHKSUM_FLD_DPI_CHKSUM						REG_FLD(24, 0)
#define PATTERN_FLD_PAT_R_MAN							REG_FLD(8, 24)
#define PATTERN_FLD_PAT_G_MAN						    REG_FLD(8, 16)
#define PATTERN_FLD_PAT_B_MAN							REG_FLD(8, 8)
#define PATTERN_FLD_PAT_SEL							REG_FLD(3, 4)
#define PATTERN_FLD_PAT_EN						REG_FLD(1, 0)
/* field definition */
/* ------------------------------------------------------------- */
/* CCORR */
#define DISP_REG_CCORR_EN                                    (DISPSYS_CCORR_BASE + 0x000)
#define DISP_REG_CCORR_RESET                                 (DISPSYS_CCORR_BASE + 0x004)
#define DISP_REG_CCORR_INTEN                                 (DISPSYS_CCORR_BASE + 0x008)
#define DISP_REG_CCORR_INTSTA                                (DISPSYS_CCORR_BASE + 0x00c)
#define DISP_REG_CCORR_STATUS                                (DISPSYS_CCORR_BASE + 0x010)
#define DISP_REG_CCORR_CFG                                   (DISPSYS_CCORR_BASE + 0x020)
#define DISP_REG_CCORR_IN_CNT                                (DISPSYS_CCORR_BASE + 0x024)
#define DISP_REG_CCORR_OUT_CNT                               (DISPSYS_CCORR_BASE + 0x028)
#define DISP_REG_CCORR_CHKSUM                                (DISPSYS_CCORR_BASE + 0x02c)
#define DISP_REG_CCORR_SIZE                                  (DISPSYS_CCORR_BASE + 0x030)
#define DISP_REG_CCORR_COEF_0                                (DISPSYS_CCORR_BASE + 0x080)
#define DISP_REG_CCORR_DUMMY_REG                             (DISPSYS_CCORR_BASE + 0x0c0)

#define CCORR_SIZE_FLD_HSIZE                                  REG_FLD(13, 16)
#define CCORR_SIZE_FLD_VSIZE                                  REG_FLD(13, 0)
#define CCORR_CFG_FLD_CHKSUM_SEL                              REG_FLD(3, 29)
#define CCORR_CFG_FLD_CHKSUM_EN                               REG_FLD(1, 28)
#define CCORR_CFG_FLD_CCORR_GAMMA_OFF                         REG_FLD(1, 2)
#define CCORR_CFG_FLD_CCORR_ENGINE_EN                         REG_FLD(1, 1)
/* field definition */
/* ------------------------------------------------------------- */
/* GAMMA */
#define DISP_REG_GAMMA_EN								(DISPSYS_GAMMA_BASE + 0x000)
#define DISP_REG_GAMMA_RESET							(DISPSYS_GAMMA_BASE + 0x004)
#define DISP_REG_GAMMA_INTEN							(DISPSYS_GAMMA_BASE + 0x008)
#define DISP_REG_GAMMA_INTSTA							(DISPSYS_GAMMA_BASE + 0x00c)
#define DISP_REG_GAMMA_STATUS						    (DISPSYS_GAMMA_BASE + 0x010)
#define DISP_REG_GAMMA_CFG							    (DISPSYS_GAMMA_BASE + 0x020)
#define DISP_REG_GAMMA_INPUT_COUNT					    (DISPSYS_GAMMA_BASE + 0x024)
#define DISP_REG_GAMMA_OUTPUT_COUNT					    (DISPSYS_GAMMA_BASE + 0x028)
#define DISP_REG_GAMMA_CHKSUM						    (DISPSYS_GAMMA_BASE + 0x02c)
#define DISP_REG_GAMMA_SIZE							    (DISPSYS_GAMMA_BASE + 0x030)
#define DISP_REG_GAMMA_DUMMY_REG					    (DISPSYS_GAMMA_BASE + 0x0c0)
#define DISP_REG_GAMMA_LUT							    (DISPSYS_GAMMA_BASE + 0x700)

#define EN_FLD_GAMMA_EN                         REG_FLD(1, 0)
#define RESET_FLD_GAMMA_RESET                   REG_FLD(1, 0)
#define INTEN_FLD_OF_END_INT_EN                 REG_FLD(1, 1)
#define INTEN_FLD_IF_END_INT_EN                 REG_FLD(1, 0)
#define INTSTA_FLD_OF_END_INT                   REG_FLD(1, 1)
#define INTSTA_FLD_IF_END_INT                   REG_FLD(1, 0)
#define STATUS_FLD_IN_VALID				REG_FLD(1, 7)
#define STATUS_FLD_IN_READY				REG_FLD(1, 6)
#define STATUS_FLD_OUT_VALID	                REG_FLD(1, 5)
#define STATUS_FLD_OUT_READY	                REG_FLD(1, 4)
#define STATUS_FLD_OF_UNFINISH	                REG_FLD(1, 1)
#define STATUS_FLD_IF_UNFINISH	                REG_FLD(1, 0)
#define CFG_FLD_CHKSUM_SEL				REG_FLD(2, 29)
#define CFG_FLD_CHKSUM_EN				REG_FLD(1, 28)
#define CFG_FLD_CCORR_GAMMA_OFF                 REG_FLD(1, 5)
#define CFG_FLD_CCORR_EN			            REG_FLD(1, 4)
#define CFG_FLD_DITHER_EN			            REG_FLD(1, 2)
#define CFG_FLD_GAMMA_LUT_EN			REG_FLD(1, 1)
#define CFG_FLD_RELAY_MODE				REG_FLD(1, 0)
#define INPUT_COUNT_FLD_INP_LINE_CNT			REG_FLD(13, 16)
#define INPUT_COUNT_FLD_INP_PIX_CNT			REG_FLD(13, 0)
#define OUTPUT_COUNT_FLD_OUTP_LINE_CNT			REG_FLD(13, 16)
#define OUTPUT_COUNT_FLD_OUTP_PIX_CNT			REG_FLD(13, 0)
#define CHKSUM_FLD_CHKSUM						REG_FLD(30, 0)
#define SIZE_FLD_HSIZE							REG_FLD(13, 16)
#define SIZE_FLD_VSIZE							REG_FLD(13, 0)
#define CCORR_0_FLD_CCORR_C00					REG_FLD(12, 16)
#define CCORR_0_FLD_CCORR_C01					REG_FLD(12, 0)
#define CCORR_1_FLD_CCORR_C02					REG_FLD(12, 16)
#define CCORR_1_FLD_CCORR_C10					REG_FLD(12, 0)
#define CCORR_2_FLD_CCORR_C11					REG_FLD(12, 16)
#define CCORR_2_FLD_CCORR_C12					REG_FLD(12, 0)
#define CCORR_3_FLD_CCORR_C20					REG_FLD(12, 16)
#define CCORR_3_FLD_CCORR_C21					REG_FLD(12, 0)
#define CCORR_4_FLD_CCORR_C22					REG_FLD(12, 16)
#define DUMMY_REG_FLD_DUMMY_REG					REG_FLD(32, 0)
#define DITHER_0_FLD_CRC_CLR					REG_FLD(1, 24)
#define DITHER_0_FLD_CRC_START					REG_FLD(1, 20)
#define DITHER_0_FLD_CRC_CEN					REG_FLD(1, 16)
#define DITHER_0_FLD_FRAME_DONE_DEL				REG_FLD(8, 8)
#define DITHER_0_FLD_OUT_SEL					REG_FLD(1, 4)
#define DITHER_5_FLD_W_DEMO						REG_FLD(16, 0)
#define DITHER_6_FLD_WRAP_MODE					REG_FLD(1, 16)
#define DITHER_6_FLD_LEFT_EN					REG_FLD(2, 14)
#define DITHER_6_FLD_FPHASE_R					REG_FLD(1, 13)
#define DITHER_6_FLD_FPHASE_EN					REG_FLD(1, 12)
#define DITHER_6_FLD_FPHASE						REG_FLD(6, 4)
#define DITHER_6_FLD_ROUND_EN					REG_FLD(1, 3)
#define DITHER_6_FLD_RDITHER_EN					REG_FLD(1, 2)
#define DITHER_6_FLD_LFSR_EN					REG_FLD(1, 1)
#define DITHER_6_FLD_EDITHER_EN					REG_FLD(1, 0)
#define DITHER_7_FLD_DRMOD_B					REG_FLD(2, 8)
#define DITHER_7_FLD_DRMOD_G					REG_FLD(2, 4)
#define DITHER_7_FLD_DRMOD_R					REG_FLD(2, 0)
#define GAMMA_DITHER_8_FLD_INK_DATA_R			REG_FLD(12, 16)
#define DITHER_8_FLD_INK						REG_FLD(1, 0)
#define GAMMA_DITHER_9_FLD_INK_DATA_B			REG_FLD(12, 16)
#define GAMMA_DITHER_9_FLD_INK_DATA_G			REG_FLD(12, 0)
#define DITHER_10_FLD_FPHASE_BIT				REG_FLD(3, 8)
#define DITHER_10_FLD_FPHASE_SEL				REG_FLD(2, 4)
#define DITHER_10_FLD_FPHASE_CTRL				REG_FLD(2, 0)
#define DITHER_11_FLD_SUB_B						REG_FLD(2, 12)
#define DITHER_11_FLD_SUB_G						REG_FLD(2, 8)
#define DITHER_11_FLD_SUB_R						REG_FLD(2, 4)
#define DITHER_11_FLD_SUBPIX_EN					REG_FLD(1, 0)
#define DITHER_12_FLD_H_ACTIVE					REG_FLD(16, 16)
#define DITHER_12_FLD_TABLE_EN					REG_FLD(2, 4)
#define DITHER_12_FLD_LSB_OFF					REG_FLD(1, 0)
#define DITHER_13_FLD_RSHIFT_B					REG_FLD(3, 8)
#define DITHER_13_FLD_RSHIFT_G					REG_FLD(3, 4)
#define DITHER_13_FLD_RSHIFT_R					REG_FLD(3, 0)
#define DITHER_14_FLD_DEBUG_MODE				REG_FLD(2, 8)
#define DITHER_14_FLD_DIFF_SHIFT				REG_FLD(3, 4)
#define DITHER_14_FLD_TESTPIN_EN				REG_FLD(1, 0)
#define DITHER_15_FLD_LSB_ERR_SHIFT_R			REG_FLD(3, 28)
#define DITHER_15_FLD_LSB_OVFLW_BIT_R			REG_FLD(3, 24)
#define DITHER_15_FLD_LSB_ADD_LSHIFT_R			REG_FLD(3, 20)
#define DITHER_15_FLD_LSB_INPUT_RSHIFT_R		REG_FLD(3, 16)
#define DITHER_15_FLD_LSB_NEW_BIT_MODE			REG_FLD(1, 0)
#define DITHER_16_FLD_LSB_ERR_SHIFT_B			REG_FLD(3, 28)
#define DITHER_16_FLD_OVFLW_BIT_B				REG_FLD(3, 24)
#define DITHER_16_FLD_ADD_LSHIFT_B				REG_FLD(3, 20)
#define DITHER_16_FLD_INPUT_RSHIFT_B			REG_FLD(3, 16)
#define DITHER_16_FLD_LSB_ERR_SHIFT_G			REG_FLD(3, 12)
#define DITHER_16_FLD_OVFLW_BIT_G				REG_FLD(3, 8)
#define DITHER_16_FLD_ADD_LSHIFT_G				REG_FLD(3, 4)
#define DITHER_16_FLD_INPUT_RSHIFT_G			REG_FLD(3, 0)
#define DITHER_17_FLD_CRC_RDY					REG_FLD(1, 16)
#define DITHER_17_FLD_CRC_OUT					REG_FLD(16, 0)
#define LUT_FLD_GAMMA_LUT_R						REG_FLD(10, 20)
#define LUT_FLD_GAMMA_LUT_G						REG_FLD(10, 10)
#define LUT_FLD_GAMMA_LUT_B						REG_FLD(10, 0)

/* ------------------------------------------------------------- */
/* Dither */
#define DISP_REG_DITHER_EN                                        (DISPSYS_DITHER_BASE + 0x000)
#define DISP_REG_DITHER_RESET                                     (DISPSYS_DITHER_BASE + 0x004)
#define DISP_REG_DITHER_INTEN                                     (DISPSYS_DITHER_BASE + 0x008)
#define DISP_REG_DITHER_INTSTA                                    (DISPSYS_DITHER_BASE + 0x00c)
#define DISP_REG_DITHER_STATUS                                    (DISPSYS_DITHER_BASE + 0x010)
#define DISP_REG_DITHER_CFG                                       (DISPSYS_DITHER_BASE + 0x020)
#define DISP_REG_DITHER_IN_CNT                                    (DISPSYS_DITHER_BASE + 0x024)
#define DISP_REG_DITHER_OUT_CNT                                   (DISPSYS_DITHER_BASE + 0x028)
#define DISP_REG_DITHER_CHKSUM                                    (DISPSYS_DITHER_BASE + 0x02c)
#define DISP_REG_DITHER_SIZE                                      (DISPSYS_DITHER_BASE + 0x030)
#define DISP_REG_DITHER_DUMMY_REG                                 (DISPSYS_DITHER_BASE + 0x0c0)
#define DISP_REG_DITHER_0                                         (DISPSYS_DITHER_BASE + 0x100)
#define DISP_REG_DITHER_5                                         (DISPSYS_DITHER_BASE + 0x114)
#define DISP_REG_DITHER_6                                         (DISPSYS_DITHER_BASE + 0x118)
#define DISP_REG_DITHER_7                                         (DISPSYS_DITHER_BASE + 0x11c)
#define DISP_REG_DITHER_8                                         (DISPSYS_DITHER_BASE + 0x120)
#define DISP_REG_DITHER_9                                         (DISPSYS_DITHER_BASE + 0x124)
#define DISP_REG_DITHER_10                                        (DISPSYS_DITHER_BASE + 0x128)
#define DISP_REG_DITHER_11                                        (DISPSYS_DITHER_BASE + 0x12c)
#define DISP_REG_DITHER_12                                        (DISPSYS_DITHER_BASE + 0x130)
#define DISP_REG_DITHER_13                                        (DISPSYS_DITHER_BASE + 0x134)
#define DISP_REG_DITHER_14                                        (DISPSYS_DITHER_BASE + 0x138)
#define DISP_REG_DITHER_15                                        (DISPSYS_DITHER_BASE + 0x13c)
#define DISP_REG_DITHER_16                                        (DISPSYS_DITHER_BASE + 0x140)
#define DISP_REG_DITHER_17                                        (DISPSYS_DITHER_BASE + 0x144)

#define DITHER_CFG_FLD_CHKSUM_SEL			REG_FLD(2, 29)
#define DITHER_CFG_FLD_CHKSUM_EN				REG_FLD(1, 28)
#define DITHER_CFG_FLD_DITHER_ENGINE_EN				REG_FLD(1, 1)
#define DITHER_CFG_FLD_RELAY_MODE				REG_FLD(1, 0)
#define DITHER_SIZE_FLD_HSIZE				REG_FLD(13, 16)
#define DITHER_SIZE_FLD_VSIZE					REG_FLD(13, 0)

/* ------------------------------------------------------------- */
/* MERGE */
#define DISP_REG_MERGE_ENABLE						(DISPSYS_MERGE_BASE + 0x000)
#define DISP_REG_MERGE_SW_RESET						(DISPSYS_MERGE_BASE + 0x004)
#define DISP_REG_MERGE_DEBUG						(DISPSYS_MERGE_BASE + 0x008)

#define ENABLE_FLD_MERGE_EN						REG_FLD(1, 0)
#define SW_RESET_FLD_MERGE_SW_RST						REG_FLD(1, 0)
#define DEBUG_FLD_MERGE_FSM						REG_FLD(3, 29)
#define DEBUG_FLD_OUT_PIXEL_CNT						REG_FLD(24, 0)

/* ------------------------------------------------------------- */
/* MUTEX */
#define DISP_OVL_SEPARATE_MUTEX_ID (DISP_MUTEX_DDP_LAST+1)	/* other disp will not see mutex 4 */
#define DISP_REG_CONFIG_MUTEX_INTEN					(DISPSYS_MUTEX_BASE + 0x000)
#define DISP_REG_CONFIG_MUTEX_INTSTA                                (DISPSYS_MUTEX_BASE + 0x004)
#define DISP_REG_CONFIG_MUTEX0_EN					(DISPSYS_MUTEX_BASE + 0x020)
#define DISP_REG_CONFIG_MUTEX0_RST					(DISPSYS_MUTEX_BASE + 0x028)
#define DISP_REG_CONFIG_MUTEX0_MOD					(DISPSYS_MUTEX_BASE + 0x02C)
#define DISP_REG_CONFIG_MUTEX0_SOF					(DISPSYS_MUTEX_BASE + 0x030)
#define DISP_REG_CONFIG_MUTEX1_EN					(DISPSYS_MUTEX_BASE + 0x040)
#define DISP_REG_CONFIG_MUTEX1_RST					(DISPSYS_MUTEX_BASE + 0x048)
#define DISP_REG_CONFIG_MUTEX1_MOD					(DISPSYS_MUTEX_BASE + 0x04C)
#define DISP_REG_CONFIG_MUTEX1_SOF					(DISPSYS_MUTEX_BASE + 0x050)
#define DISP_REG_CONFIG_MUTEX2_EN					(DISPSYS_MUTEX_BASE + 0x060)
#define DISP_REG_CONFIG_MUTEX2_RST					(DISPSYS_MUTEX_BASE + 0x068)
#define DISP_REG_CONFIG_MUTEX2_MOD					(DISPSYS_MUTEX_BASE + 0x06C)
#define DISP_REG_CONFIG_MUTEX2_SOF					(DISPSYS_MUTEX_BASE + 0x070)
#define DISP_REG_CONFIG_MUTEX3_EN					(DISPSYS_MUTEX_BASE + 0x080)
#define DISP_REG_CONFIG_MUTEX3_RST					(DISPSYS_MUTEX_BASE + 0x088)
#define DISP_REG_CONFIG_MUTEX3_MOD					(DISPSYS_MUTEX_BASE + 0x08C)
#define DISP_REG_CONFIG_MUTEX3_SOF					(DISPSYS_MUTEX_BASE + 0x090)
#define DISP_REG_CONFIG_MUTEX4_EN					(DISPSYS_MUTEX_BASE + 0x0A0)
#define DISP_REG_CONFIG_MUTEX4_RST					(DISPSYS_MUTEX_BASE + 0x0A8)
#define DISP_REG_CONFIG_MUTEX4_MOD					(DISPSYS_MUTEX_BASE + 0x0AC)
#define DISP_REG_CONFIG_MUTEX4_SOF					(DISPSYS_MUTEX_BASE + 0x0B0)
#define DISP_REG_CONFIG_MUTEX5_EN					(DISPSYS_MUTEX_BASE + 0x0C0)
#define DISP_REG_CONFIG_MUTEX5_RST					(DISPSYS_MUTEX_BASE + 0x0C8)
#define DISP_REG_CONFIG_MUTEX5_MOD					(DISPSYS_MUTEX_BASE + 0x0CC)
#define DISP_REG_CONFIG_MUTEX5_SOF					(DISPSYS_MUTEX_BASE + 0x0D0)
#define DISP_REG_CONFIG_DEBUG_OUT_SEL				    (DISPSYS_MUTEX_BASE + 0x200)

#define DISP_REG_CONFIG_MUTEX_EN(n)	(DISP_REG_CONFIG_MUTEX0_EN + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_RST(n)	(DISP_REG_CONFIG_MUTEX0_RST + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_MOD(n)	(DISP_REG_CONFIG_MUTEX0_MOD + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_SOF(n)	(DISP_REG_CONFIG_MUTEX0_SOF + (0x20 * (n)))
#define SOF_FLD_MUTEX0_SOF		REG_FLD(3, 0)
#define SOF_FLD_MUTEX0_SOF_TIMING	REG_FLD(2, 3)
#define SOF_FLD_MUTEX0_SOF_WAIT		REG_FLD(1, 5)
#define SOF_FLD_MUTEX0_EOF		REG_FLD(3, 6)
#define SOF_FLD_MUTEX0_FOF_TIMING	REG_FLD(2, 9)
#define SOF_FLD_MUTEX0_EOF_WAIT		REG_FLD(1, 11)

#define SOF_VAL_MUTEX0_SOF_SINGLE_MODE			(0)
#define SOF_VAL_MUTEX0_SOF_FROM_DSI0			(1)
#define SOF_VAL_MUTEX0_SOF_FROM_DPI			(2)
#define SOF_VAL_MUTEX0_EOF_SINGLE_MODE			(0)
#define SOF_VAL_MUTEX0_EOF_FROM_DSI0			(1)
#define SOF_VAL_MUTEX0_EOF_FROM_DPI			(2)



#define INTEN_FLD_MUTEX_INTEN                               REG_FLD(20, 0)
#define INTSTA_FLD_MUTEX_INTSTA                             REG_FLD(20, 0)
#define EN_FLD_MUTEX0_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX0_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD						REG_FLD(21, 0)



#define EN_FLD_MUTEX1_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX1_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX1_MOD						REG_FLD(21, 0)
#define SOF_FLD_MUTEX1_SOF					REG_FLD(3, 0)
#define SOF_FLD_MUTEX1_SOF_TIMING				    REG_FLD(2, 3)
#define SOF_FLD_MUTEX1_SOF_WAIT					        REG_FLD(1, 5)
#define SOF_FLD_MUTEX1_EOF					            REG_FLD(3, 6)
#define SOF_FLD_MUTEX1_FOF_TIMING				    REG_FLD(2, 9)
#define SOF_FLD_MUTEX1_EOF_WAIT					        REG_FLD(1, 11)



#define EN_FLD_MUTEX2_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX2_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX2_MOD					REG_FLD(21, 0)
#define SOF_FLD_MUTEX2_SOF						REG_FLD(3, 0)
#define SOF_FLD_MUTEX2_SOF_TIMING					    REG_FLD(2, 3)
#define SOF_FLD_MUTEX2_SOF_WAIT					        REG_FLD(1, 5)
#define SOF_FLD_MUTEX2_EOF					            REG_FLD(3, 6)
#define SOF_FLD_MUTEX2_FOF_TIMING				    REG_FLD(2, 9)
#define SOF_FLD_MUTEX2_EOF_WAIT					        REG_FLD(1, 11)



#define EN_FLD_MUTEX3_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX3_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX3_MOD							REG_FLD(21, 0)
#define SOF_FLD_MUTEX3_SOF						REG_FLD(3, 0)
#define SOF_FLD_MUTEX3_SOF_TIMING					    REG_FLD(2, 3)
#define SOF_FLD_MUTEX3_SOF_WAIT					        REG_FLD(1, 5)
#define SOF_FLD_MUTEX3_EOF					            REG_FLD(3, 6)
#define SOF_FLD_MUTEX3_FOF_TIMING				    REG_FLD(2, 9)
#define SOF_FLD_MUTEX3_EOF_WAIT					        REG_FLD(1, 11)


#define EN_FLD_MUTEX4_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX4_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX4_MOD							REG_FLD(21, 0)
#define SOF_FLD_MUTEX4_SOF						REG_FLD(3, 0)
#define SOF_FLD_MUTEX4_SOF_TIMING					    REG_FLD(2, 3)
#define SOF_FLD_MUTEX4_SOF_WAIT					        REG_FLD(1, 5)
#define SOF_FLD_MUTEX4_EOF					            REG_FLD(3, 6)
#define SOF_FLD_MUTEX4_FOF_TIMING				    REG_FLD(2, 9)
#define SOF_FLD_MUTEX4_EOF_WAIT					        REG_FLD(1, 11)


#define EN_FLD_MUTEX5_EN						REG_FLD(1, 0)
#define RST_FLD_MUTEX5_RST						REG_FLD(1, 0)
#define MOD_FLD_MUTEX5_MOD							REG_FLD(21, 0)
#define SOF_FLD_MUTEX5_SOF						REG_FLD(3, 0)
#define SOF_FLD_MUTEX5_SOF_TIMING					    REG_FLD(2, 3)
#define SOF_FLD_MUTEX5_SOF_WAIT					        REG_FLD(1, 5)
#define SOF_FLD_MUTEX5_EOF					            REG_FLD(3, 6)
#define SOF_FLD_MUTEX5_FOF_TIMING				    REG_FLD(2, 9)
#define SOF_FLD_MUTEX5_EOF_WAIT					        REG_FLD(1, 11)


#define DEBUG_OUT_SEL_FLD_DEBUG_OUT_SEL				REG_FLD(2, 0)

/* ------------------------------------------------------------- */
/* OD */
#define DISP_REG_OD_EN           (DISPSYS_OD_BASE+0x000)
#define DISP_REG_OD_RESET        (DISPSYS_OD_BASE+0x004)
#define DISP_REG_OD_INTEN        (DISPSYS_OD_BASE+0x008)
#define DISP_REG_OD_INTSTA       (DISPSYS_OD_BASE+0x00C)
#define DISP_REG_OD_STATUS       (DISPSYS_OD_BASE+0x010)
#define DISP_REG_OD_CFG          (DISPSYS_OD_BASE+0x020)
#define DISP_REG_OD_INPUT_COUNT	 (DISPSYS_OD_BASE+0x024)
#define DISP_REG_OD_OUTPUT_COUNT (DISPSYS_OD_BASE+0x028)
#define DISP_REG_OD_CHKSUM       (DISPSYS_OD_BASE+0x02C)
#define DISP_REG_OD_SIZE	     (DISPSYS_OD_BASE+0x030)
#define DISP_REG_OD_HSYNC_WIDTH  (DISPSYS_OD_BASE+0x040)
#define DISP_REG_OD_VSYNC_WIDTH	 (DISPSYS_OD_BASE+0x044)
#define DISP_REG_OD_MISC         (DISPSYS_OD_BASE+0x048)
#define DISP_REG_OD_DUMMY_REG    (DISPSYS_OD_BASE+0x0C0)
#define DISP_REG_OD_DITHER_0	   (DISPSYS_OD_BASE+0x100)
#define DISP_REG_OD_DITHER_5     (DISPSYS_OD_BASE+0x114)
#define DISP_REG_OD_DITHER_6     (DISPSYS_OD_BASE+0x118)
#define DISP_REG_OD_DITHER_7	   (DISPSYS_OD_BASE+0x11C)
#define DISP_REG_OD_DITHER_8	   (DISPSYS_OD_BASE+0x120)
#define DISP_REG_OD_DITHER_9	   (DISPSYS_OD_BASE+0x124)
#define DISP_REG_OD_DITHER_10	   (DISPSYS_OD_BASE+0x128)
#define DISP_REG_OD_DITHER_11	   (DISPSYS_OD_BASE+0x12C)
#define DISP_REG_OD_DITHER_12	   (DISPSYS_OD_BASE+0x130)
#define DISP_REG_OD_DITHER_13	   (DISPSYS_OD_BASE+0x134)
#define DISP_REG_OD_DITHER_14	   (DISPSYS_OD_BASE+0x138)
#define DISP_REG_OD_DITHER_15	   (DISPSYS_OD_BASE+0x13C)
#define DISP_REG_OD_DITHER_16	   (DISPSYS_OD_BASE+0x140)
#define DISP_REG_OD_DITHER_17    (DISPSYS_OD_BASE+0x144)

/* ------------------------------------------------------------- */
/* OVL */
#define DISP_REG_OVL_STA						(0x000UL)
#define DISP_REG_OVL_INTEN						(0x004UL)
#define DISP_REG_OVL_INTSTA						(0x008UL)
#define DISP_REG_OVL_EN							(0x00CUL)
#define DISP_REG_OVL_TRIG						(0x010UL)
#define DISP_REG_OVL_RST						(0x014UL)
#define DISP_REG_OVL_ROI_SIZE						(0x020UL)
#define DISP_REG_OVL_DATAPATH_CON				    (0x024UL)
#define DISP_REG_OVL_ROI_BGCLR						(0x028UL)
#define DISP_REG_OVL_SRC_CON						(0x02CUL)
#define DISP_REG_OVL_L0_CON						(0x030UL)
#define DISP_REG_OVL_L0_SRCKEY						(0x034UL)
#define DISP_REG_OVL_L0_SRC_SIZE					(0x038UL)
#define DISP_REG_OVL_L0_OFFSET						(0x03CUL)
#define DISP_REG_OVL_L0_ADDR						(0xf40UL)
#define DISP_REG_OVL_L0_PITCH						(0x044UL)
#define DISP_REG_OVL_L0_TILE						(0x048UL)
#define DISP_REG_OVL_L0_CLIP						(0x04CUL)
#define DISP_REG_OVL_L1_CON						(0x050UL)
#define DISP_REG_OVL_L1_SRCKEY						(0x054UL)
#define DISP_REG_OVL_L1_SRC_SIZE					(0x058UL)
#define DISP_REG_OVL_L1_OFFSET						(0x05CUL)
#define DISP_REG_OVL_L1_ADDR						(0xf60UL)
#define DISP_REG_OVL_L1_PITCH						(0x064UL)
#define DISP_REG_OVL_L1_TILE						(0x068UL)
#define DISP_REG_OVL_L1_CLIP						(0x06CUL)
#define DISP_REG_OVL_L2_CON						(0x070UL)
#define DISP_REG_OVL_L2_SRCKEY						(0x074UL)
#define DISP_REG_OVL_L2_SRC_SIZE					(0x078UL)
#define DISP_REG_OVL_L2_OFFSET						(0x07CUL)
#define DISP_REG_OVL_L2_ADDR						(0xf80UL)
#define DISP_REG_OVL_L2_PITCH						(0x084UL)
#define DISP_REG_OVL_L2_TILE						(0x088UL)
#define DISP_REG_OVL_L2_CLIP						(0x08CUL)
#define DISP_REG_OVL_L3_CON						(0x090UL)
#define DISP_REG_OVL_L3_SRCKEY						(0x094UL)
#define DISP_REG_OVL_L3_SRC_SIZE					(0x098UL)
#define DISP_REG_OVL_L3_OFFSET						(0x09CUL)
#define DISP_REG_OVL_L3_ADDR						(0xfA0UL)
#define DISP_REG_OVL_L3_PITCH						(0x0A4UL)
#define DISP_REG_OVL_L3_TILE						(0x0A8UL)
#define DISP_REG_OVL_L3_CLIP						(0x0ACUL)
#define DISP_REG_OVL_RDMA0_CTRL						(0x0C0UL)
#define DISP_REG_OVL_RDMA0_MEM_GMC_SETTING		            (0x0C8UL)
	#define FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD			REG_FLD_MSB_LSB(7, 0)
	#define FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD		REG_FLD_MSB_LSB(15, 8)
	#define FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD_HIGH_OFS		REG_FLD_MSB_LSB(23, 16)
	#define FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD_HIGH_OFS	REG_FLD_MSB_LSB(31, 24)

#define DISP_REG_OVL_RDMA0_MEM_SLOW_CON			            (0x0CCUL)
#define DISP_REG_OVL_RDMA0_FIFO_CTRL				    (0x0D0UL)
	#define FLD_OVL_RDMA_FIFO_THRD		REG_FLD_MSB_LSB(9, 0)
	#define FLD_OVL_RDMA_FIFO_SIZE		REG_FLD_MSB_LSB(25, 16)
	#define FLD_OVL_RDMA_FIFO_UND_EN	REG_FLD_MSB_LSB(31, 31)
#define DISP_REG_OVL_RDMA1_CTRL						(0x0E0UL)
#define DISP_REG_OVL_RDMA1_MEM_GMC_SETTING		            (0x0E8UL)
#define DISP_REG_OVL_RDMA1_MEM_SLOW_CON			        (0x0ECUL)
#define DISP_REG_OVL_RDMA1_FIFO_CTRL					(0x0F0UL)
#define DISP_REG_OVL_RDMA2_CTRL						(0x100UL)
#define DISP_REG_OVL_RDMA2_MEM_GMC_SETTING		                (0x108UL)
#define DISP_REG_OVL_RDMA2_MEM_SLOW_CON			            (0x10CUL)
#define DISP_REG_OVL_RDMA2_FIFO_CTRL				    (0x110UL)
#define DISP_REG_OVL_RDMA3_CTRL						(0x120UL)
#define DISP_REG_OVL_RDMA3_MEM_GMC_SETTING		                (0x128UL)
#define DISP_REG_OVL_RDMA3_MEM_SLOW_CON				        (0x12CUL)
#define DISP_REG_OVL_RDMA3_FIFO_CTRL				    (0x130UL)
#define DISP_REG_OVL_L0_Y2R_PARA_R0				    (0x134UL)
#define DISP_REG_OVL_L0_Y2R_PARA_R1				    (0x138UL)
#define DISP_REG_OVL_L0_Y2R_PARA_G0				    (0x13CUL)
#define DISP_REG_OVL_L0_Y2R_PARA_G1				    (0x140UL)
#define DISP_REG_OVL_L0_Y2R_PARA_B0				    (0x144UL)
#define DISP_REG_OVL_L0_Y2R_PARA_B1				    (0x148UL)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0			        (0x14CUL)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1			        (0x150UL)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0			        (0x154UL)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1			        (0x158UL)
#define DISP_REG_OVL_L1_Y2R_PARA_R0				    (0x15CUL)
#define DISP_REG_OVL_L1_Y2R_PARA_R1				    (0x160UL)
#define DISP_REG_OVL_L1_Y2R_PARA_G0				    (0x164UL)
#define DISP_REG_OVL_L1_Y2R_PARA_G1				    (0x168UL)
#define DISP_REG_OVL_L1_Y2R_PARA_B0				    (0x16CUL)
#define DISP_REG_OVL_L1_Y2R_PARA_B1				    (0x170UL)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0			        (0x174UL)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1			        (0x178UL)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0			        (0x17CUL)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1			        (0x180UL)
#define DISP_REG_OVL_L2_Y2R_PARA_R0				    (0x184UL)
#define DISP_REG_OVL_L2_Y2R_PARA_R1				    (0x188UL)
#define DISP_REG_OVL_L2_Y2R_PARA_G0				    (0x18CUL)
#define DISP_REG_OVL_L2_Y2R_PARA_G1				    (0x190UL)
#define DISP_REG_OVL_L2_Y2R_PARA_B0				    (0x194UL)
#define DISP_REG_OVL_L2_Y2R_PARA_B1				    (0x198UL)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0			        (0x19CUL)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1			        (0x1A0UL)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0			        (0x1A4UL)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1			        (0x1A8UL)
#define DISP_REG_OVL_L3_Y2R_PARA_R0				    (0x1ACUL)
#define DISP_REG_OVL_L3_Y2R_PARA_R1				    (0x1B0UL)
#define DISP_REG_OVL_L3_Y2R_PARA_G0				    (0x1B4UL)
#define DISP_REG_OVL_L3_Y2R_PARA_G1				    (0x1B8UL)
#define DISP_REG_OVL_L3_Y2R_PARA_B0				    (0x1BCUL)
#define DISP_REG_OVL_L3_Y2R_PARA_B1				    (0x1C0UL)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0			        (0x1C4UL)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1			        (0x1C8UL)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0			    (0x1CCUL)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1			        (0x1D0UL)
#define DISP_REG_OVL_DEBUG_MON_SEL				    (0x1D4UL)
#define DISP_REG_OVL_RDMA0_MEM_GMC_S2		                        (0x1E0UL)
#define DISP_REG_OVL_RDMA1_MEM_GMC_S2		                        (0x1E4UL)
#define DISP_REG_OVL_RDMA2_MEM_GMC_S2		                        (0x1E8UL)
#define DISP_REG_OVL_RDMA3_MEM_GMC_S2		                        (0x1ECUL)
#define DISP_REG_OVL_RDMA_GREQ_NUM					(0x1F8UL)
	#define FLD_OVL_RDMA_GREQ_LAYER0_GREQ_NUM	REG_FLD_MSB_LSB(2, 0)
	#define FLD_OVL_RDMA_GREQ_LAYER1_GREQ_NUM	REG_FLD_MSB_LSB(6, 4)
	#define FLD_OVL_RDMA_GREQ_LAYER2_GREQ_NUM	REG_FLD_MSB_LSB(10, 8)
	#define FLD_OVL_RDMA_GREQ_LAYER3_GREQ_NUM	REG_FLD_MSB_LSB(14, 12)
	#define FLD_OVL_RDMA_GREQ_OSTD_GREQ_NUM		REG_FLD_MSB_LSB(23, 16)
	#define FLD_OVL_RDMA_GREQ_GREQ_DIS_CNT		REG_FLD_MSB_LSB(26, 24)
	#define FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_PREULTRA	REG_FLD_MSB_LSB(28, 28)
	#define FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_ULTRA	REG_FLD_MSB_LSB(29, 29)

#define DISP_REG_OVL_RDMA_GREQ_URG_NUM					(0x1FCUL)
	#define FLD_OVL_RDMA_GREQ_LAYER0_GREQ_URG_NUM		REG_FLD_MSB_LSB(2, 0)
	#define FLD_OVL_RDMA_GREQ_LAYER1_GREQ_URG_NUM		REG_FLD_MSB_LSB(6, 4)
	#define FLD_OVL_RDMA_GREQ_LAYER2_GREQ_URG_NUM		REG_FLD_MSB_LSB(10, 8)
	#define FLD_OVL_RDMA_GREQ_LAYER3_GREQ_URG_NUM		REG_FLD_MSB_LSB(14, 12)
	#define FLD_OVL_RDMA_GREQ_ARG_GREQ_URG_TH		REG_FLD_MSB_LSB(25, 16)
	#define FLD_OVL_RDMA_GREQ_ARG_URG_BIAS			REG_FLD_MSB_LSB(28, 28)

#define DISP_REG_OVL_DUMMY_REG						(0x200UL)
#define DISP_REG_OVL_RDMA_ULTRA_SRC					(0x20CUL)
	#define FLD_OVL_RDMA_PREULTRA_BUF_SRC		REG_FLD_MSB_LSB(1, 0)
	#define FLD_OVL_RDMA_PREULTRA_SMI_SRC		REG_FLD_MSB_LSB(3, 2)
	#define FLD_OVL_RDMA_PREULTRA_ROI_END_SRC	REG_FLD_MSB_LSB(5, 4)
	#define FLD_OVL_RDMA_PREULTRA_RDMA_SRC		REG_FLD_MSB_LSB(7, 6)
	#define FLD_OVL_RDMA_ULTRA_BUF_SRC		REG_FLD_MSB_LSB(9, 8)
	#define FLD_OVL_RDMA_ULTRA_SMI_SRC		REG_FLD_MSB_LSB(11, 10)
	#define FLD_OVL_RDMA_ULTRA_ROI_END_SRC		REG_FLD_MSB_LSB(13, 12)
	#define FLD_OVL_RDMA_ULTRA_RDMA_SRC		REG_FLD_MSB_LSB(15, 14)

#define DISP_REG_OVL_RDMAn_BUF_LOW(layer)			(0x210UL + ((layer)<<2))
	#define FLD_OVL_RDMA_BUF_LOW_ULTRA_TH		REG_FLD_MSB_LSB(9, 0)
	#define FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH	REG_FLD_MSB_LSB(21, 12)

#define DISP_REG_OVL_SMI_DBG                                        (0x230UL)
#define DISP_REG_OVL_GREQ_LAYER_CNT                                 (0x234UL)
#define DISP_REG_OVL_FLOW_CTRL_DBG				    (0x240UL)
#define DISP_REG_OVL_ADDCON_DBG					    (0x244UL)
#define DISP_REG_OVL_RDMA0_DBG						(0x24CUL)
#define DISP_REG_OVL_RDMA1_DBG						(0x250UL)
#define DISP_REG_OVL_RDMA2_DBG						(0x254UL)
#define DISP_REG_OVL_RDMA3_DBG						(0x258UL)
#define DISP_REG_OVL_L0_CLR					    (0x25cUL)
#define DISP_REG_OVL_L1_CLR					    (0x260UL)
#define DISP_REG_OVL_L2_CLR					    (0x264UL)
#define DISP_REG_OVL_L3_CLR					    (0x268UL)
#define DISP_REG_OVL_LC_CLR					    (0x26CUL)
#define DISP_REG_OVL_CRC                                            (0x270UL)
#define DISP_REG_OVL_LC_CON                                         (0x280UL)
#define DISP_REG_OVL_LC_SRCKEY						(0x284UL)
#define DISP_REG_OVL_LC_SRC_SIZE					(0x288UL)
#define DISP_REG_OVL_LC_OFFSET						(0x28CUL)
#define DISP_REG_OVL_LC_SRC_SEL                                     (0x290UL)
#define DISP_REG_OVL_FUNC_DCM0					(0x2A0UL)
#define DISP_REG_OVL_FUNC_DCM1					(0x2A4UL)
#define DISP_REG_OVL_SECURE                                         (0x2C0UL)

#define STA_FLD_RDMA3_IDLE                              REG_FLD(1, 4)
#define STA_FLD_RDMA2_IDLE                              REG_FLD(1, 3)
#define STA_FLD_RDMA1_IDLE                              REG_FLD(1, 2)
#define STA_FLD_RDMA0_IDLE                              REG_FLD(1, 1)
#define STA_FLD_RUN                                     REG_FLD(1, 0)
#define INTEN_FLD_ABNORMAL_SOF							REG_FLD(1, 13)
#define INTEN_FLD_RDMA3_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 12)
#define INTEN_FLD_RDMA2_FIFO_UNDERFLOW_INTEN		    REG_FLD(1, 11)
#define INTEN_FLD_RDMA1_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 10)
#define INTEN_FLD_RDMA0_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 9)
#define INTEN_FLD_RDMA3_EOF_ABNORMAL_INTEN              REG_FLD(1, 8)
#define INTEN_FLD_RDMA2_EOF_ABNORMAL_INTEN		REG_FLD(1, 7)
#define INTEN_FLD_RDMA1_EOF_ABNORMAL_INTEN		    REG_FLD(1, 6)
#define INTEN_FLD_RDMA0_EOF_ABNORMAL_INTEN		REG_FLD(1, 5)
#define INTEN_FLD_FME_HWRST_DONE_INTEN			REG_FLD(1, 4)
#define INTEN_FLD_FME_SWRST_DONE_INTEN			REG_FLD(1, 3)
#define INTEN_FLD_FME_UND_INTEN						REG_FLD(1, 2)
#define INTEN_FLD_FME_CPL_INTEN						REG_FLD(1, 1)
#define INTEN_FLD_REG_CMT_INTEN						REG_FLD(1, 0)
#define INTSTA_FLD_RDMA3_FIFO_UNDERFLOW_INTSTA		REG_FLD(1, 12)
#define INTSTA_FLD_RDMA2_FIFO_UNDERFLOW_INTSTA		REG_FLD(1, 11)
#define INTSTA_FLD_RDMA1_FIFO_UNDERFLOW_INTSTA		REG_FLD(1, 10)
#define INTSTA_FLD_RDMA0_FIFO_UNDERFLOW_INTSTA		REG_FLD(1, 9)
#define INTSTA_FLD_RDMA3_EOF_ABNORMAL_INTSTA		REG_FLD(1, 8)
#define INTSTA_FLD_RDMA2_EOF_ABNORMAL_INTSTA		REG_FLD(1, 7)
#define INTSTA_FLD_RDMA1_EOF_ABNORMAL_INTSTA		REG_FLD(1, 6)
#define INTSTA_FLD_RDMA0_EOF_ABNORMAL_INTSTA		REG_FLD(1, 5)
#define INTSTA_FLD_FME_HWRST_DONE_INTSTA		REG_FLD(1, 4)
#define INTSTA_FLD_FME_SWRST_DONE_INTSTA		REG_FLD(1, 3)
#define INTSTA_FLD_FME_UND_INTSTA				REG_FLD(1, 2)
#define INTSTA_FLD_FME_CPL_INTSTA				REG_FLD(1, 1)
#define INTSTA_FLD_REG_CMT_INTSTA				REG_FLD(1, 0)
#define EN_FLD_EN									REG_FLD(1, 0)
#define TRIG_FLD_SW_TRIG							REG_FLD(1, 0)
#define RST_FLD_RST									REG_FLD(1, 0)
#define ROI_SIZE_FLD_ROI_H							REG_FLD(12, 16)
#define ROI_SIZE_FLD_ROI_W							REG_FLD(12, 0)
#define DATAPATH_CON_FLD_LAYER_GREQ_NUM				REG_FLD(3, 29)
#define DATAPATH_CON_FLD_RDMA3_OUT_SEL				REG_FLD(1, 23)
#define DATAPATH_CON_FLD_RDMA2_OUT_SEL				REG_FLD(1, 22)
#define DATAPATH_CON_FLD_RDMA1_OUT_SEL				REG_FLD(1, 21)
#define DATAPATH_CON_FLD_RDMA0_OUT_SEL				REG_FLD(1, 20)
#define DATAPATH_CON_FLD_PQ_OUT_SEL				REG_FLD(2, 16)
#define DATAPATH_CON_FLD_OVL_GAMMA_OUT			REG_FLD(1, 15)
#define DATAPATH_CON_FLD_ADOBE_LAYER				REG_FLD(2, 13)
#define DATAPATH_CON_FLD_ADOBE_MODE				REG_FLD(1, 12)
#define DATAPATH_CON_FLD_L3_GPU_MODE				REG_FLD(1, 11)
#define DATAPATH_CON_FLD_L2_GPU_MODE				REG_FLD(1, 10)
#define DATAPATH_CON_FLD_L1_GPU_MODE				REG_FLD(1, 9)
#define DATAPATH_CON_FLD_L0_GPU_MODE				REG_FLD(1, 8)
#define DATAPATH_CON_FLD_BGCLR_IN_SEL                   REG_FLD(1, 2)
#define DATAPATH_CON_FLD_RANDOM_BGCLR_EN		REG_FLD(1, 1)
#define DATAPATH_CON_FLD_LAYER_SMI_ID_EN			REG_FLD(1, 0)
#define ROI_BGCLR_FLD_ALPHA							REG_FLD(8, 24)
#define ROI_BGCLR_FLD_RED							REG_FLD(8, 16)
#define ROI_BGCLR_FLD_GREEN							REG_FLD(8, 8)
#define ROI_BGCLR_FLD_BLUE							REG_FLD(8, 0)
#define SRC_CON_FLD_L3_EN			REG_FLD(1, 3)
#define SRC_CON_FLD_L2_EN			REG_FLD(1, 2)
#define SRC_CON_FLD_L1_EN			REG_FLD(1, 1)
#define SRC_CON_FLD_L0_EN			REG_FLD(1, 0)

#define L_CON_FLD_SKEN				REG_FLD(1, 30)
#define L_CON_FLD_LARC				REG_FLD(2, 28)	/* layer source 0:mem, 1:constant color */
#define L_CON_FLD_RGB_SWAP			REG_FLD(1, 25)
#define L_CON_FLD_BTSW				REG_FLD(1, 24)
#define L_CON_FLD_CLRFMT_MAN		REG_FLD(1, 23)
#define L_CON_FLD_MTX				REG_FLD(4, 16)
#define L_CON_FLD_CFMT				REG_FLD(4, 12)
#define L_CON_FLD_HORI_FLIP	        REG_FLD(1, 10)
#define L_CON_FLD_VIRTICAL_FLIP	    REG_FLD(1, 9)
#define L_CON_FLD_AEN				REG_FLD(1, 8)
#define L_CON_FLD_APHA				REG_FLD(8, 0)

#define L_PITCH_FLD_LSP				REG_FLD(16, 0)
#define L_PITCH_FLD_SUR_ALFA		REG_FLD(16, 16)
#define L_PITCH_FLD_CONST_BLD		REG_FLD(1, 28)

#define ADDCON_DBG_FLD_L3_WIN_HIT		REG_FLD(1, 31)
#define ADDCON_DBG_FLD_L2_WIN_HIT		REG_FLD(1, 30)
#define ADDCON_DBG_FLD_ROI_Y			REG_FLD(13, 16)
#define ADDCON_DBG_FLD_L1_WIN_HIT		REG_FLD(1, 15)
#define ADDCON_DBG_FLD_L0_WIN_HIT		REG_FLD(1, 14)
#define ADDCON_DBG_FLD_ROI_X			REG_FLD(13, 0)

#define RDMA0_DBG_FLD_RDMA0_SMI_GREQ				REG_FLD(1, 29)
#define RDMA0_DBG_FLD_RDMA0_SMI_BUSY				REG_FLD(1, 28)
#define RDMA0_DBG_FLD_RDMA0_OUT_VALID				REG_FLD(1, 29)
#define RDMA0_DBG_FLD_RDMA0_OUT_READY				REG_FLD(1, 28)
#define RDMA0_DBG_FLD_RDMA0_OUT_DATA				REG_FLD(24, 4)
#define RDMA0_DBG_FLD_RDMA0_WRAM_RST_CS				REG_FLD(3, 0)
#define RDMA0_DBG_FLD_RDMA0_LAYER_GREQ                      REG_FLD(1, 29)
#define OVL_L_CLIP_FLD_LEFT				REG_FLD(8, 0)
#define OVL_L_CLIP_FLD_RIGHT			REG_FLD(8, 8)
#define OVL_L_CLIP_FLD_TOP				REG_FLD(8, 16)
#define OVL_L_CLIP_FLD_BOTTOM			REG_FLD(8, 24)
#define RDMA0_CTRL_FLD_RDMA_EN				REG_FLD(1, 0)
#define RDMA0_CTRL_FLD_RMDA_FIFO_USED_SZ	REG_FLD(11, 16)


/* ------------------------------------------------------------- */
/* RDMA */
#define DISP_REG_RDMA_INT_ENABLE				(DISPSYS_RDMA0_BASE+0x000)
#define DISP_REG_RDMA_INT_STATUS				(DISPSYS_RDMA0_BASE+0x004)
#define DISP_REG_RDMA_GLOBAL_CON				(DISPSYS_RDMA0_BASE+0x010)
#define DISP_REG_RDMA_SIZE_CON_0                                (DISPSYS_RDMA0_BASE+0x014)
#define DISP_REG_RDMA_SIZE_CON_1                                (DISPSYS_RDMA0_BASE+0x018)
#define DISP_REG_RDMA_TARGET_LINE                               (DISPSYS_RDMA0_BASE+0x01C)
#define DISP_REG_RDMA_MEM_CON                                   (DISPSYS_RDMA0_BASE+0x024)
#define DISP_REG_RDMA_MEM_SRC_PITCH					(DISPSYS_RDMA0_BASE+0x02C)
#define DISP_REG_RDMA_MEM_GMC_SETTING_0			    (DISPSYS_RDMA0_BASE+0x030)
#define DISP_REG_RDMA_MEM_GMC_SETTING_1			    (DISPSYS_RDMA0_BASE+0x034)
#define DISP_REG_RDMA_MEM_SLOW_CON				(DISPSYS_RDMA0_BASE+0x038)
#define DISP_REG_RDMA_MEM_GMC_SETTING_2			    (DISPSYS_RDMA0_BASE+0x03c)
#define DISP_REG_RDMA_FIFO_CON                                  (DISPSYS_RDMA0_BASE+0x040)
#define DISP_REG_RDMA_FIFO_LOG                                  (DISPSYS_RDMA0_BASE+0x044)
#define DISP_REG_RDMA_C00                                       (DISPSYS_RDMA0_BASE+0x054)
#define DISP_REG_RDMA_C01                                       (DISPSYS_RDMA0_BASE+0x058)
#define DISP_REG_RDMA_C02                                       (DISPSYS_RDMA0_BASE+0x05C)
#define DISP_REG_RDMA_C10                                       (DISPSYS_RDMA0_BASE+0x060)
#define DISP_REG_RDMA_C11                                       (DISPSYS_RDMA0_BASE+0x064)
#define DISP_REG_RDMA_C12                                       (DISPSYS_RDMA0_BASE+0x068)
#define DISP_REG_RDMA_C20                                       (DISPSYS_RDMA0_BASE+0x06C)
#define DISP_REG_RDMA_C21                                       (DISPSYS_RDMA0_BASE+0x070)
#define DISP_REG_RDMA_C22                                       (DISPSYS_RDMA0_BASE+0x074)
#define DISP_REG_RDMA_PRE_ADD_0					(DISPSYS_RDMA0_BASE+0x078)
#define DISP_REG_RDMA_PRE_ADD_1                                 (DISPSYS_RDMA0_BASE+0x07C)
#define DISP_REG_RDMA_PRE_ADD_2                                 (DISPSYS_RDMA0_BASE+0x080)
#define DISP_REG_RDMA_POST_ADD_0				(DISPSYS_RDMA0_BASE+0x084)
#define DISP_REG_RDMA_POST_ADD_1                                (DISPSYS_RDMA0_BASE+0x088)
#define DISP_REG_RDMA_POST_ADD_2                                (DISPSYS_RDMA0_BASE+0x08C)
#define DISP_REG_RDMA_DUMMY                                     (DISPSYS_RDMA0_BASE+0x090)
#define DISP_REG_RDMA_DEBUG_OUT_SEL                             (DISPSYS_RDMA0_BASE+0x094)
#define DISP_REG_RDMA_BG_CON_0                                  (DISPSYS_RDMA0_BASE+0x0a0)
#define DISP_REG_RDMA_BG_CON_1                                  (DISPSYS_RDMA0_BASE+0x0a4)
#define DISP_REG_RDMA_THRESHOLD_FOR_SODI                        (DISPSYS_RDMA0_BASE+0x0a8)
#define DISP_REG_RDMA_THRESHOLD_FOR_DVFS                       (DISPSYS_RDMA0_BASE+0x0ac)
#define DISP_REG_RDMA_SRAM_SEL                                 (DISPSYS_RDMA0_BASE + 0xb0)
#define DISP_RDMA_STALL_CG_CON                                 (DISPSYS_RDMA0_BASE + 0xb4)
#define DISP_REG_RDMA_IN_P_CNT                                  (DISPSYS_RDMA0_BASE+0x0f0)
#define DISP_REG_RDMA_IN_LINE_CNT                               (DISPSYS_RDMA0_BASE+0x0f4)
#define DISP_REG_RDMA_OUT_P_CNT                                 (DISPSYS_RDMA0_BASE+0x0f8)
#define DISP_REG_RDMA_OUT_LINE_CNT                              (DISPSYS_RDMA0_BASE+0x0fc)
#define DISP_REG_RDMA_MEM_START_ADDR				    (DISPSYS_RDMA0_BASE+0xf00)

#define INT_ENABLE_FLD_TARGET_LINE_INT_EN		    REG_FLD(1, 5)
#define INT_ENABLE_FLD_FIFO_UNDERFLOW_INT_EN		        REG_FLD(1, 4)
#define INT_ENABLE_FLD_EOF_ABNORMAL_INT_EN		    REG_FLD(1, 3)
#define INT_ENABLE_FLD_FRAME_END_INT_EN				    REG_FLD(1, 2)
#define INT_ENABLE_FLD_FRAME_START_INT_EN			    REG_FLD(1, 1)
#define INT_ENABLE_FLD_REG_UPDATE_INT_EN			    REG_FLD(1, 0)
#define INT_STATUS_FLD_FIFO_EMPTY_INT_FLAG		        REG_FLD(1, 6)
#define INT_STATUS_FLD_TARGET_LINE_INT_FLAG		        REG_FLD(1, 5)
#define INT_STATUS_FLD_FIFO_UNDERFLOW_INT_FLAG		            REG_FLD(1, 4)
#define INT_STATUS_FLD_EOF_ABNORMAL_INT_FLAG		        REG_FLD(1, 3)
#define INT_STATUS_FLD_FRAME_END_INT_FLAG			    REG_FLD(1, 2)
#define INT_STATUS_FLD_FRAME_START_INT_FLAG		        REG_FLD(1, 1)
#define INT_STATUS_FLD_REG_UPDATE_INT_FLAG		    REG_FLD(1, 0)
#define GLOBAL_CON_FLD_SMI_BUSY					REG_FLD(1, 12)
#define GLOBAL_CON_FLD_RESET_STATE				REG_FLD(3, 8)
#define GLOBAL_CON_FLD_SOFT_RESET				REG_FLD(1, 4)
#define GLOBAL_CON_FLD_MODE_SEL					REG_FLD(1, 1)
#define GLOBAL_CON_FLD_ENGINE_EN				REG_FLD(1, 0)
#define SIZE_CON_0_FLD_MATRIX_INT_MTX_SEL			    REG_FLD(4, 20)
#define SIZE_CON_0_FLD_MATRIX_WIDE_GAMUT_EN		        REG_FLD(1, 18)
#define SIZE_CON_0_FLD_MATRIX_ENABLE				REG_FLD(1, 17)
#define SIZE_CON_0_FLD_MATRIX_EXT_MTX_EN		    REG_FLD(1, 16)
#define SIZE_CON_0_FLD_OUTPUT_FRAME_WIDTH		        REG_FLD(13, 0)
#define SIZE_CON_1_FLD_OUTPUT_FRAME_HEIGHT		        REG_FLD(20, 0)
#define TARGET_LINE_FLD_TARGET_LINE				REG_FLD(20, 0)
#define MEM_CON_FLD_MEM_MODE_HORI_BLOCK_NUM		            REG_FLD(8, 24)
#define MEM_CON_FLD_MEM_MODE_INPUT_COSITE		            REG_FLD(1, 13)
#define MEM_CON_FLD_MEM_MODE_INPUT_UPSAMPLE		        REG_FLD(1, 12)
#define MEM_CON_FLD_MEM_MODE_INPUT_SWAP				        REG_FLD(1, 8)
#define MEM_CON_FLD_MEM_MODE_INPUT_FORMAT		            REG_FLD(4, 4)
#define MEM_CON_FLD_MEM_MODE_TILE_INTERLACE		        REG_FLD(1, 1)
#define MEM_CON_FLD_MEM_MODE_TILE_EN                            REG_FLD(1, 0)
#define MEM_SRC_PITCH_FLD_MEM_MODE_SRC_PITCH		        REG_FLD(16, 0)
/* #define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_HIGH_OFS	    REG_FLD(8, 24)
#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_HIGH_OFS		REG_FLD(8, 16)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_LOW_OFS	    REG_FLD(8, 8)
#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_LOW               REG_FLD(8, 0) */
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_COUNT			REG_FLD(16, 16)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_EN			REG_FLD(1, 0)
/* #define MEM_GMC_SETTING_1_FLD_ISSUE_REQ_THRESHOLD		REG_FLD(8, 0) */
#define FIFO_CON_FLD_FIFO_UNDERFLOW_EN				REG_FLD(1, 31)
#define FIFO_CON_FLD_FIFO_PSEUDO_SIZE				REG_FLD(12, 16)
#define FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD		REG_FLD(12, 0)
#define FIFO_LOG_FLD_RDMA_FIFO_LOG					REG_FLD(10, 0)
#define C00_FLD_DISP_RDMA_C00					REG_FLD(13, 0)
#define C01_FLD_DISP_RDMA_C01                                   REG_FLD(13, 0)
#define C02_FLD_DISP_RDMA_C02					REG_FLD(13, 0)
#define C10_FLD_DISP_RDMA_C10					REG_FLD(13, 0)
#define C11_FLD_DISP_RDMA_C11					REG_FLD(13, 0)
#define C12_FLD_DISP_RDMA_C12					REG_FLD(13, 0)
#define C20_FLD_DISP_RDMA_C20					REG_FLD(13, 0)
#define C21_FLD_DISP_RDMA_C21					REG_FLD(13, 0)
#define C22_FLD_DISP_RDMA_C22					REG_FLD(13, 0)
#define PRE_ADD_0_FLD_DISP_RDMA_PRE_ADD_0				    REG_FLD(9, 0)
#define PRE_ADD_1_FLD_DISP_RDMA_PRE_ADD_1				    REG_FLD(9, 0)
#define PRE_ADD_2_FLD_DISP_RDMA_PRE_ADD_2				    REG_FLD(9, 0)
#define POST_ADD_0_FLD_DISP_RDMA_POST_ADD_0				    REG_FLD(9, 0)
#define POST_ADD_1_FLD_DISP_RDMA_POST_ADD_1			        REG_FLD(9, 0)
#define POST_ADD_2_FLD_DISP_RDMA_POST_ADD_2			        REG_FLD(9, 0)
#define DUMMY_FLD_DISP_RDMA_DUMMY				REG_FLD(32, 0)
#define DEBUG_OUT_SEL_FLD_DISP_RDMA_DEBUG_OUT_SEL	        REG_FLD(4, 0)
#define MEM_START_ADDR_FLD_MEM_MODE_START_ADDR		        REG_FLD(32, 0)
#define RDMA_BG_CON_0_LEFT					REG_FLD(13, 0)
#define RDMA_BG_CON_0_RIGHT					REG_FLD(13, 16)
#define RDMA_BG_CON_1_TOP					REG_FLD(13, 0)
#define RDMA_BG_CON_1_BOTTOM				REG_FLD(13, 16)

/* ------------------------------------------------------------- */
/* SPLIT */
#define DISP_REG_SPLIT_ENABLE								(DISPSYS_SPLIT0_BASE+0x00)
#define DISP_REG_SPLIT_SW_RESET							(DISPSYS_SPLIT0_BASE+0x04)
#define DISP_REG_SPLIT_DEBUG								(DISPSYS_SPLIT0_BASE+0x08)

#define ENABLE_FLD_SPLIT_EN								REG_FLD(1, 0)
#define W_RESET_FLD_SPLIT_SW_RST					REG_FLD(1, 0)
#define DEBUG_FLD_SPLIT_FSM								REG_FLD(3, 29)
#define DEBUG_FLD_IN_PIXEL_CNT								REG_FLD(24, 0)

/* ------------------------------------------------------------- */
/* WDMA */
#define DISP_REG_WDMA_INTEN							(DISPSYS_WDMA0_BASE+0x000)
#define DISP_REG_WDMA_INTSTA							(DISPSYS_WDMA0_BASE+0x004)
#define DISP_REG_WDMA_EN							(DISPSYS_WDMA0_BASE+0x008)
#define DISP_REG_WDMA_RST							(DISPSYS_WDMA0_BASE+0x00C)
#define DISP_REG_WDMA_SMI_CON							(DISPSYS_WDMA0_BASE+0x010)
#define DISP_REG_WDMA_CFG							(DISPSYS_WDMA0_BASE+0x014)
#define DISP_REG_WDMA_SRC_SIZE							(DISPSYS_WDMA0_BASE+0x018)
#define DISP_REG_WDMA_CLIP_SIZE							(DISPSYS_WDMA0_BASE+0x01C)
#define DISP_REG_WDMA_CLIP_COORD					    (DISPSYS_WDMA0_BASE+0x020)
#define DISP_REG_WDMA_DST_W_IN_BYTE					    (DISPSYS_WDMA0_BASE+0x028)
#define DISP_REG_WDMA_ALPHA							(DISPSYS_WDMA0_BASE+0x02C)
#define DISP_REG_WDMA_BUF_CON1							(DISPSYS_WDMA0_BASE+0x038)
#define DISP_REG_WDMA_BUF_CON2							(DISPSYS_WDMA0_BASE+0x03C)
#define DISP_REG_WDMA_C00							(DISPSYS_WDMA0_BASE+0x040)
#define DISP_REG_WDMA_C02							(DISPSYS_WDMA0_BASE+0x044)
#define DISP_REG_WDMA_C10							(DISPSYS_WDMA0_BASE+0x048)
#define DISP_REG_WDMA_C12							(DISPSYS_WDMA0_BASE+0x04C)
#define DISP_REG_WDMA_C20							(DISPSYS_WDMA0_BASE+0x050)
#define DISP_REG_WDMA_C22							(DISPSYS_WDMA0_BASE+0x054)
#define DISP_REG_WDMA_PRE_ADD0							(DISPSYS_WDMA0_BASE+0x058)
#define DISP_REG_WDMA_PRE_ADD2							(DISPSYS_WDMA0_BASE+0x05C)
#define DISP_REG_WDMA_POST_ADD0						    (DISPSYS_WDMA0_BASE+0x060)
#define DISP_REG_WDMA_POST_ADD2						    (DISPSYS_WDMA0_BASE+0x064)
#define DISP_REG_WDMA_DST_UV_PITCH					    (DISPSYS_WDMA0_BASE+0x078)
#define DISP_REG_WDMA_DST_ADDR_OFFSET0				        (DISPSYS_WDMA0_BASE+0x080)
#define DISP_REG_WDMA_DST_ADDR_OFFSET1				        (DISPSYS_WDMA0_BASE+0x084)
#define DISP_REG_WDMA_DST_ADDR_OFFSET2				        (DISPSYS_WDMA0_BASE+0x088)
#define DISP_REG_WDMA_PROC_TRACK_CON_0				        (DISPSYS_WDMA0_BASE+0x090)
	#define FLD_WDMA_PROC_TRACK_CON0_IGNORE_INIT_LEGECY		REG_FLD_MSB_LSB(30, 30)
	#define FLD_WDMA_PROC_TRACK_CON0_STOP_GREQ_EN			REG_FLD_MSB_LSB(29, 29)
	#define FLD_WDMA_PROC_TRACK_CON0_PREULTRA_EN			REG_FLD_MSB_LSB(28, 28)
	#define FLD_WDMA_PROC_TRACK_CON0_TRACK_WND			REG_FLD_MSB_LSB(11, 0)

#define DISP_REG_WDMA_PROC_TRACK_CON_1				        (DISPSYS_WDMA0_BASE+0x094)
	#define FLD_WDMA_PROC_TRACK_CON1_TARGET_CNT			REG_FLD_MSB_LSB(23, 0)

#define DISP_REG_WDMA_PROC_TRACK_CON_2				        (DISPSYS_WDMA0_BASE+0x098)
	#define FLD_WDMA_PROC_TRACK_CON2_STOP_CNT			REG_FLD_MSB_LSB(23, 0)

#define DISP_REG_WDMA_FLOW_CTRL_DBG					    (DISPSYS_WDMA0_BASE+0x0A0)
#define DISP_REG_WDMA_EXEC_DBG							(DISPSYS_WDMA0_BASE+0x0A4)
#define DISP_REG_WDMA_CT_DBG							(DISPSYS_WDMA0_BASE+0x0A8)
#define DISP_REG_WDMA_SMI_TRAFFIC_DBG                                   (DISPSYS_WDMA0_BASE+0x0AC)
#define DISP_REG_WDMA_PROC_TRACK_DBG_0                                  (DISPSYS_WDMA0_BASE+0x0B0)
#define DISP_REG_WDMA_PROC_TRACK_DBG_1                                  (DISPSYS_WDMA0_BASE+0x0B4)
#define DISP_REG_WDMA_DEBUG							(DISPSYS_WDMA0_BASE+0x0B8)
#define DISP_REG_WDMA_DUMMY							(DISPSYS_WDMA0_BASE+0x100)
#define DISP_REG_WDMA_DITHER_0							(DISPSYS_WDMA0_BASE+0xE00)
#define DISP_REG_WDMA_DITHER_5							(DISPSYS_WDMA0_BASE+0xE14)
#define DISP_REG_WDMA_DITHER_6							(DISPSYS_WDMA0_BASE+0xE18)
#define DISP_REG_WDMA_DITHER_7							(DISPSYS_WDMA0_BASE+0xE1C)
#define DISP_REG_WDMA_DITHER_8							(DISPSYS_WDMA0_BASE+0xE20)
#define DISP_REG_WDMA_DITHER_9							(DISPSYS_WDMA0_BASE+0xE24)
#define DISP_REG_WDMA_DITHER_10						    (DISPSYS_WDMA0_BASE+0xE28)
#define DISP_REG_WDMA_DITHER_11						    (DISPSYS_WDMA0_BASE+0xE2C)
#define DISP_REG_WDMA_DITHER_12						(DISPSYS_WDMA0_BASE+0xE30)
#define DISP_REG_WDMA_DITHER_13						(DISPSYS_WDMA0_BASE+0xE34)
#define DISP_REG_WDMA_DITHER_14						(DISPSYS_WDMA0_BASE+0xE38)
#define DISP_REG_WDMA_DITHER_15						(DISPSYS_WDMA0_BASE+0xE3C)
#define DISP_REG_WDMA_DITHER_16						(DISPSYS_WDMA0_BASE+0xE40)
#define DISP_REG_WDMA_DITHER_17						(DISPSYS_WDMA0_BASE+0xE44)
#define DISP_REG_WDMA_DST_ADDR0						(DISPSYS_WDMA0_BASE+0xF00)
#define DISP_REG_WDMA_DST_ADDR1						(DISPSYS_WDMA0_BASE+0xF04)
#define DISP_REG_WDMA_DST_ADDR2						(DISPSYS_WDMA0_BASE+0xF08)

#define INTEN_FLD_FRAME_UNDERRUN						REG_FLD(1, 1)
#define INTEN_FLD_FRAME_COMPLETE						REG_FLD(1, 0)
#define INTSTA_FLD_FRAME_UNDERRUN						REG_FLD(1, 1)
#define INTSTA_FLD_FRAME_COMPLETE						REG_FLD(1, 0)
#define EN_FLD_ENABLE										REG_FLD(1, 0)
#define RST_FLD_SOFT_RESET								REG_FLD(1, 0)
#define SMI_CON_FLD_SMI_V_REPEAT_NUM					REG_FLD(4, 24)
#define SMI_CON_FLD_SMI_U_REPEAT_NUM					REG_FLD(4, 20)
#define SMI_CON_FLD_SMI_Y_REPEAT_NUM					REG_FLD(4, 16)
#define SMI_CON_FLD_SLOW_COUNT							REG_FLD(8, 8)
#define SMI_CON_FLD_SLOW_LEVEL							REG_FLD(3, 5)
#define SMI_CON_FLD_SLOW_ENABLE							REG_FLD(1, 4)
#define SMI_CON_FLD_THRESHOLD							REG_FLD(4, 0)
#define CFG_FLD_DEBUG_SEL								REG_FLD(4, 28)
#define CFG_FLD_INT_MTX_SEL								REG_FLD(4, 24)
#define CFG_FLD_SWAP									REG_FLD(1, 16)
#define CFG_FLD_DNSP_SEL								REG_FLD(1, 15)
#define CFG_FLD_EXT_MTX_EN								REG_FLD(1, 13)
#define CFG_FLD_VERTICAL_AVG								REG_FLD(1, 12)
#define CFG_FLD_CT_EN										REG_FLD(1, 11)
#define CFG_FLD_OUT_FORMAT								REG_FLD(4, 4)
#define SRC_SIZE_FLD_HEIGHT								REG_FLD(14, 16)
#define SRC_SIZE_FLD_WIDTH								REG_FLD(14, 0)
#define CLIP_SIZE_FLD_HEIGHT								REG_FLD(14, 16)
#define CLIP_SIZE_FLD_WIDTH								REG_FLD(14, 0)
#define CLIP_COORD_FLD_Y_COORD							REG_FLD(14, 16)
#define CLIP_COORD_FLD_X_COORD							REG_FLD(14, 0)
#define DST_W_IN_BYTE_FLD_DST_W_IN_BYTE					REG_FLD(16, 0)
#define ALPHA_FLD_A_SEL									REG_FLD(1, 31)
#define ALPHA_FLD_A_VALUE								REG_FLD(8, 0)
#define BUF_CON1_FLD_ULTRA_ENABLE						REG_FLD(1, 31)
#define BUF_CON1_FLD_PRE_ULTRA_ENABLE						REG_FLD(1, 30)
#define BUF_CON1_FLD_FRAME_END_ULTRA					REG_FLD(1, 28)
#define BUF_CON1_FLD_ISSUE_REQ_TH						REG_FLD(9, 16)
#define BUF_CON1_FLD_FIFO_PSEUDO_SIZE					REG_FLD(9, 0)
#define BUF_CON2_FLD_ULTRA_TH_HIGH_OFS					REG_FLD(8, 24)
#define BUF_CON2_FLD_PRE_ULTRA_TH_HIGH_OFS				REG_FLD(8, 16)
#define BUF_CON2_FLD_ULTRA_TH_LOW_OFS					REG_FLD(8, 8)
#define BUF_CON2_FLD_PRE_ULTRA_TH_LOW					REG_FLD(8, 0)
#define C00_FLD_C01										REG_FLD(13, 16)
#define C00_FLD_C00										REG_FLD(13, 0)
#define C02_FLD_C02										REG_FLD(13, 0)
#define C10_FLD_C11										REG_FLD(13, 16)
#define C10_FLD_C10										REG_FLD(13, 0)
#define C12_FLD_C12										REG_FLD(13, 0)
#define C20_FLD_C21										REG_FLD(13, 16)
#define C20_FLD_C20										REG_FLD(13, 0)
#define C22_FLD_C22										REG_FLD(13, 0)
#define PRE_ADD0_FLD_PRE_ADD_1							REG_FLD(9, 16)
#define PRE_ADD0_FLD_PRE_ADD_0							REG_FLD(9, 0)
#define PRE_ADD2_FLD_PRE_ADD_2							REG_FLD(9, 0)
#define POST_ADD0_FLD_POST_ADD_1						REG_FLD(9, 16)
#define POST_ADD0_FLD_POST_ADD_0						REG_FLD(9, 0)
#define POST_ADD2_FLD_POST_ADD_2						REG_FLD(9, 0)
#define DST_UV_PITCH_FLD_UV_DST_W_IN_BYTE			REG_FLD(16, 0)
#define DST_ADDR_OFFSET0_FLD_WDMA_DESTINATION_ADDRESS_OFFSET0	REG_FLD(28, 0)
#define DST_ADDR_OFFSET1_FLD_WDMA_DESTINATION_ADDRESS_OFFSET1	REG_FLD(28, 0)
#define DST_ADDR_OFFSET2_FLD_WDMA_DESTINATION_ADDRESS_OFFSET2	REG_FLD(28, 0)

#define FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL						REG_FLD(10, 0)
#define EXEC_DBG_FLD_WDMA_IN_REQ						REG_FLD(1, 15)
#define EXEC_DBG_FLD_WDMA_IN_ACK						REG_FLD(1, 14)

#define EXEC_DBG_FLD_WDMA_STA_EXEC				REG_FLD(32, 0)
#define CT_DBG_FLD_WDMA_STA_CT					REG_FLD(32, 0)
#define DEBUG_FLD_WDMA_STA_DEBUG				REG_FLD(32, 0)
#define DUMMY_FLD_WDMA_DUMMY					REG_FLD(32, 0)
#define DITHER_0_FLD_CRC_CLR						REG_FLD(1, 24)
#define DITHER_0_FLD_CRC_START					REG_FLD(1, 20)
#define DITHER_0_FLD_CRC_CEN						REG_FLD(1, 16)
#define DITHER_0_FLD_FRAME_DONE_DEL				REG_FLD(8, 8)
#define DITHER_0_FLD_OUT_SEL						REG_FLD(1, 4)
#define DITHER_0_FLD_START						REG_FLD(1, 0)
#define DITHER_5_FLD_W_DEMO						REG_FLD(16, 0)
#define DITHER_6_FLD_WRAP_MODE					REG_FLD(1, 16)
#define DITHER_6_FLD_LEFT_EN						REG_FLD(2, 14)
#define DITHER_6_FLD_FPHASE_R						REG_FLD(1, 13)
#define DITHER_6_FLD_FPHASE_EN					REG_FLD(1, 12)
#define DITHER_6_FLD_FPHASE						REG_FLD(6, 4)
#define DITHER_6_FLD_ROUND_EN					REG_FLD(1, 3)
#define DITHER_6_FLD_RDITHER_EN					REG_FLD(1, 2)
#define DITHER_6_FLD_LFSR_EN						REG_FLD(1, 1)
#define DITHER_6_FLD_EDITHER_EN					REG_FLD(1, 0)
#define DITHER_7_FLD_DRMOD_B						REG_FLD(2, 8)
#define DITHER_7_FLD_DRMOD_G						REG_FLD(2, 4)
#define DITHER_7_FLD_DRMOD_R						REG_FLD(2, 0)
#define DITHER_8_FLD_INK_DATA_R					REG_FLD(10, 16)
#define DITHER_8_FLD_INK						REG_FLD(1, 0)
#define DITHER_9_FLD_INK_DATA_B					 REG_FLD(10, 16)
#define DITHER_9_FLD_INK_DATA_G					REG_FLD(10, 0)
#define DITHER_10_FLD_FPHASE_BIT				REG_FLD(3, 8)
#define DITHER_10_FLD_FPHASE_SEL				REG_FLD(2, 4)
#define DITHER_10_FLD_FPHASE_CTRL				REG_FLD(2, 0)
#define DITHER_11_FLD_SUB_B						REG_FLD(2, 12)
#define DITHER_11_FLD_SUB_G						REG_FLD(2, 8)
#define DITHER_11_FLD_SUB_R						REG_FLD(2, 4)
#define DITHER_11_FLD_SUBPIX_EN					REG_FLD(1, 0)
#define DITHER_12_FLD_H_ACTIVE					REG_FLD(16, 16)
#define DITHER_12_FLD_TABLE_EN					REG_FLD(2, 4)
#define DITHER_12_FLD_LSB_OFF						REG_FLD(1, 0)
#define DITHER_13_FLD_RSHIFT_B						REG_FLD(3, 8)
#define DITHER_13_FLD_RSHIFT_G					REG_FLD(3, 4)
#define DITHER_13_FLD_RSHIFT_R						REG_FLD(3, 0)
#define DITHER_14_FLD_DEBUG_MODE				REG_FLD(2, 8)
#define DITHER_14_FLD_DIFF_SHIFT				REG_FLD(3, 4)
#define DITHER_14_FLD_TESTPIN_EN				REG_FLD(1, 0)
#define DITHER_15_FLD_LSB_ERR_SHIFT_R				REG_FLD(3, 28)
#define DITHER_15_FLD_OVFLW_BIT_R				REG_FLD(3, 24)
#define DITHER_15_FLD_ADD_lSHIFT_R				REG_FLD(3, 20)
#define DITHER_15_FLD_INPUT_RSHIFT_R				REG_FLD(3, 16)
#define DITHER_15_FLD_NEW_BIT_MODE				REG_FLD(1, 0)
#define DITHER_16_FLD_LSB_ERR_SHIFT_B				REG_FLD(3, 28)
#define DITHER_16_FLD_OVFLW_BIT_B				REG_FLD(3, 24)
#define DITHER_16_FLD_ADD_lSHIFT_B				REG_FLD(3, 20)
#define DITHER_16_FLD_INPUT_RSHIFT_B				REG_FLD(3, 16)
#define DITHER_16_FLD_lSB_ERR_SHIFT_G				REG_FLD(3, 12)
#define DITHER_16_FLD_OVFLW_BIT_G				REG_FLD(3, 8)
#define DITHER_16_FLD_ADD_lSHIFT_G				REG_FLD(3, 4)
#define DITHER_16_FLD_INPUT_RSHIFT_G				REG_FLD(3, 0)
#define DITHER_17_FLD_CRC_RDY						REG_FLD(1, 16)
#define DITHER_17_FLD_CRC_OUT						REG_FLD(16, 0)
#define DST_ADDR0_FLD_ADDRESS0					REG_FLD(32, 0)
#define DST_ADDR1_FLD_ADDRESS1					REG_FLD(32, 0)
#define DST_ADDR2_FLD_ADDRESS2					REG_FLD(32, 0)
#endif
