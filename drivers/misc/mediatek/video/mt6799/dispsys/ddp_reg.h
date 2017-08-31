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
#include "mt-plat/sync_write.h"
#include "display_recorder.h"
#include "cmdq_record.h"
#include "cmdq_core.h"
#include "ddp_hal.h"
#include "disp_cmdq.h"

#define UINT32 unsigned int

/* //////////////////////////////////// macro /////////////////////////////////////////// */
#ifndef READ_REGISTER_UINT32
	#define READ_REGISTER_UINT32(reg)       (*(UINT32 * const)(reg))
#endif
#ifndef WRITE_REGISTER_UINT32
	#define WRITE_REGISTER_UINT32(reg, val) ((*(UINT32 * const)(reg)) = (val))
#endif
#ifndef READ_REGISTER_UINT16
	#define READ_REGISTER_UINT16(reg)       ((*(UINT16 * const)(reg)))
#endif
#ifndef WRITE_REGISTER_UINT16
	#define WRITE_REGISTER_UINT16(reg, val) ((*(UINT16 * const)(reg)) = (val))
#endif
#ifndef READ_REGISTER_UINT8
	#define READ_REGISTER_UINT8(reg)        ((*(UINT8 * const)(reg)))
#endif
#ifndef WRITE_REGISTER_UINT8
	#define WRITE_REGISTER_UINT8(reg, val)  ((*(UINT8 * const)(reg)) = (val))
#endif

#define INREG8(x)           READ_REGISTER_UINT8((UINT8 *)((void *)(x)))
#define OUTREG8(x, y)       WRITE_REGISTER_UINT8((UINT8 *)((void *)(x)), (UINT8)(y))
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))
#define MASKREG8(x, y, z)   OUTREG8(x, (INREG8(x)&~(y))|(z))

#define INREG16(x)          READ_REGISTER_UINT16((UINT16 *)((void *)(x)))
#define OUTREG16(x, y)      WRITE_REGISTER_UINT16((UINT16 *)((void *)(x)), (UINT16)(y))
#define SETREG16(x, y)      OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)      OUTREG16(x, INREG16(x)&~(y))
#define MASKREG16(x, y, z)  OUTREG16(x, (INREG16(x)&~(y))|(z))

#define INREG32(x)          READ_REGISTER_UINT32((UINT32 *)((void *)(x)))
#define OUTREG32(x, y)      WRITE_REGISTER_UINT32((UINT32 *)((void *)(x)), (UINT32)(y))
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)  OUTREG32(x, (INREG32(x)&~(y))|(z))

#ifndef ASSERT
	#define ASSERT(expr)	WARN_ON(!(expr))
#endif

#define AS_INT32(x)     (*(INT32 *)((void *)x))
#define AS_INT16(x)     (*(INT16 *)((void *)x))
#define AS_INT8(x)      (*(INT8  *)((void *)x))

#define AS_UINT32(x)    (*(UINT32 *)((void *)x))
#define AS_UINT16(x)    (*(UINT16 *)((void *)x))
#define AS_UINT8(x)     (*(UINT8  *)((void *)x))

#ifndef FALSE
  #define FALSE (0)
#endif

#ifndef TRUE
  #define TRUE  (1)
#endif


#define ENABLE_CLK_MGR

/* field definition */
/* ------------------------------------------------------------- */
/* MIPITX */
struct MIPITX_DSI_IMPENDANCE_0_REG {
	unsigned RG_DSI0_D2P_RT_CODE:5;
	unsigned rsv_5:3;
	unsigned RG_DSI0_D2N_RT_CODE:5;
	unsigned rev_13:3;
	unsigned RG_DSI0_D0P_RT_CODE:5;
	unsigned rev_21:3;
	unsigned RG_DSI0_D0N_RT_CODE:5;
	unsigned rev_29:3;
};


struct MIPITX_DSI_IMPENDANCE_1_REG {
	unsigned RG_DSI0_CKP_RT_CODE:5;
	unsigned rsv_5:3;
	unsigned RG_DSI0_CKN_RT_CODE:5;
	unsigned rsv_13:3;
	unsigned RG_DSI0_D1P_RT_CODE:5;
	unsigned rsv_21:3;
	unsigned RG_DSI0_D1N_RT_CODE:5;
	unsigned rsv_29:3;
};


struct MIPITX_DSI_IMPENDANCE_2_REG {
	unsigned RG_DSI0_D3P_RT_CODE:5;
	unsigned rsv_5:3;
	unsigned RG_DSI0_D3N_RT_CODE:5;
	unsigned rsv_13:3;
	unsigned RG_DSI0_D2_CKMODE_EN:1;
	unsigned RG_DSI0_D0_CKMODE_EN:1;
	unsigned RG_DSI0_CK_CKMODE_EN:1;
	unsigned RG_DSI0_D1_CKMODE_EN:1;
	unsigned RG_DSI0_D3_CKMODE_EN:1;
	unsigned rsv_21:3;
	unsigned RG_DSI0_D2_PN_SWAP_EN:1;
	unsigned RG_DSI0_D0_PN_SWAP_EN:1;
	unsigned RG_DSI0_CK_PN_SWAP_EN:1;
	unsigned RG_DSI0_D1_PN_SWAP_EN:1;
	unsigned RG_DSI0_D3_PN_SWAP_EN:1;
	unsigned rsv_29:3;
};

struct MIPITX_DSI_LANE_CON_REG {
	unsigned RG_DSI0_CPHY_T1DRV_EN:1;
	unsigned RG_DSI0_ANA_CK_SEL:1;
	unsigned RG_DSI0_PHY_CK_SEL:1;
	unsigned RG_DSI0_CPHY_EN:1;
	unsigned RG_DSI0_PHYCK_INV_EN:1;
	unsigned RG_DSI0_PWR04_EN:1;
	unsigned RG_DSI0_BG_LPF_EN:1;
	unsigned RG_DSI0_BG_CORE_EN:1;
	unsigned RG_DSI0_PAD_TIEL_SEL:1;
	unsigned rsv_9:11;
	unsigned RG_DSI0_PAD_D2P_T0A_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D2N_T0B_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D0P_T0C_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D0N_T1A_TIEL_EN:1;
	unsigned RG_DSI0_PAD_CKP_T1B_TIEL_EN:1;
	unsigned RG_DSI0_PAD_CKN_T1C_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D1P_T2A_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D1N_T2B_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D3P_T2C_TIEL_EN:1;
	unsigned RG_DSI0_PAD_D3N_XXX_TIEL_EN:1;
	unsigned rsv_30:2;
};

struct MIPITX_DSI_VOLTAGE_SEL_REG {
	unsigned RG_DSI0_PRD_REF_SEL:6;
	unsigned RG_DSI0_HSTX_LDO_REF_SEL:4;
	unsigned RG_DSI0_V2I_REF_SEL:4;
	unsigned rsv_14:2;
	unsigned RG_DSI0_LPRX_VTH_SEL:3;
	unsigned rsv_19:1;
	unsigned RG_DSI0_LPRX_VTL_SEL:3;
	unsigned rsv_23:1;
	unsigned RG_DSI0_LPCD_VTH_SEL:3;
	unsigned rsv_27:1;
	unsigned RG_DSI0_LPCD_VTL_SEL:3;
	unsigned rsv_31:1;
};


struct MIPITX_DSI_PRESERVED_REG {
	unsigned RG_DSI0_PRESERVE0:16;
	unsigned RG_DSI0_PRESERVE1:16;
};


struct MIPITX_DSI_TEST_CON_REG {
	unsigned RG_DSI0_D2_SER_BISTTOG:1;
	unsigned RG_DSI0_D0_SER_BISTTOG:1;
	unsigned RG_DSI0_CK_SER_BISTTOG:1;
	unsigned RG_DSI0_D1_SER_BISTTOG:1;
	unsigned RG_DSI0_D3_SER_BISTTOG:1;
	unsigned rsv_5:3;
	unsigned RG_DSI0_D2_SER_DIN_SEL:1;
	unsigned RG_DSI0_D0_SER_DIN_SEL:1;
	unsigned RG_DSI0_CK_SER_DIN_SEL:1;
	unsigned RG_DSI0_D1_SER_DIN_SEL:1;
	unsigned RG_DSI0_D3_SER_DIN_SEL:1;
	unsigned rsv_13:3;
	unsigned RG_DSI0_SER_DATA:8;
	unsigned RG_DSI0_MON_BYTECK_EN:1;
	unsigned RG_DSI0_MON_DIGCK_EN:1;
	unsigned RG_DSI0_TEST_DIV:2;
	unsigned RG_DSI0_TEST_SEL:4;
};


struct MIPITX_DSI_PLL_PWR_REG {
	unsigned AD_DSI0_PLL_SDM_PWR_ON:1;
	unsigned AD_DSI0_PLL_SDM_ISO_EN:1;
	unsigned rsv_2:6;
	unsigned DA_DSI0_PLL_SDM_PWR_ACK:1;
	unsigned rsv_9:23;
};


struct MIPITX_DSI_PLL_CON0_REG {
	unsigned RG_DSI0_PLL_SDM_PCW;
};


struct MIPITX_DSI_PLL_CON1_REG {
	unsigned RG_DSI0_PLL_SDM_PCW_CHG:1;
	unsigned rsv_1:3;
	unsigned RG_DSI0_PLL_EN:1;
	unsigned RG_DSI0_PLL_EN_V18:1;
	unsigned rsv_6:2;
	unsigned RG_DSI0_PLL_POSDIV:3;
	unsigned RG_DSI0_PLL_PREDIV:2;
	unsigned RG_DSI0_PLL_SDM_FRA_EN:1;
	unsigned RG_DSI0_PLL_SDM_HREN:1;
	unsigned RG_DSI0_PLL_LVROD_EN:1;
	unsigned RG_DSI0_PLL_BP:1;
	unsigned RG_DSI0_PLL_BR:1;
	unsigned RG_DSI0_PLL_BLP:1;
	unsigned rsv_19:1;
	unsigned RG_DSI0_PLL_RST_DLY:2;
	unsigned rsv_22:10;
};


struct MIPITX_DSI_PLL_CON2_REG {
	unsigned RG_DSI0_PLL_SDM_SSC_PH_INIT:1;
	unsigned RG_DSI0_PLL_SDM_SSC_EN:1;
	unsigned rsv_2:14;
	unsigned RG_DSI0_PLL_SDM_SSC_PRD:16;
};


struct MIPITX_DSI_PLL_CON3_REG {
	unsigned RG_DSI0_PLL_SDM_SSC_DELTA1:16;
	unsigned RG_DSI0_PLL_SDM_SSC_DELTA:16;
};


struct MIPITX_DSI_PLL_CON4_REG {
	unsigned RG_DSI0_PLL_MONCK_EN:1;
	unsigned rsv_1:1;
	unsigned RG_DSI0_PLL_MONVC_EN:2;
	unsigned RG_DSI0_PLL_MONREF_EN:1;
	unsigned RG_DSI0_PLL_BW:3;
	unsigned RG_DSI0_PLL_FS:2;
	unsigned RG_DSI0_PLL_IBIAS:2;
	unsigned RG_DSI0_PLL_ICHP:2;
	unsigned rsv_14:2;
	unsigned RG_DSI0_PLL_RESERVED:16;
};

struct MIPITX_DSI_PHY_SEL0_REG {
	unsigned MIPI_TX_CPHY_EN:1;
	unsigned MIPI_TX_SW_CTRL_EN:1;
	unsigned rsv_2:2;
	unsigned MIPI_TX_PHY2_SEL:4;
	unsigned MIPI_TX_CPHY0BC_SEL:4;
	unsigned MIPI_TX_PHY0_SEL:4;
	unsigned MIPI_TX_PHY1AB_SEL:4;
	unsigned MIPI_TX_PHYC_SEL:4;
	unsigned MIPI_TX_CPHY1CA_SEL:4;
	unsigned MIPI_TX_PHY1_SEL:4;
};


struct MIPITX_DSI_PHY_SEL1_REG {
	unsigned MIPI_TX_PHY2BC_SEL:4;
	unsigned MIPI_TX_PHY3_SEL:4;
	unsigned MIPI_TX_CPHYXXX_SEL:4;
	unsigned MIPI_TX_LPRX0AB_SEL:4;
	unsigned MIPI_TX_LPRX0BC_SEL:4;
	unsigned MIPI_TX_LPRX0CA_SEL:4;
	unsigned MIPI_TX_CPHY0_HS_SEL:2;
	unsigned MIPI_TX_CPHY1_HS_SEL:2;
	unsigned MIPI_TX_CPHY2_HS_SEL:2;
	unsigned rsv_30:2;
};


struct MIPITX_DSI_PHY_SEL2_REG {
	unsigned MIPI_TX_PHY2_HSDATA_SEL:4;
	unsigned MIPI_TX_CPHY0BC_HSDATA_SEL:4;
	unsigned MIPI_TX_PHY0_HSDATA_SEL:4;
	unsigned MIPI_TX_PHY1AB_HSDATA_SEL:4;
	unsigned MIPI_TX_PHYC_HSDATA_SEL:4;
	unsigned MIPI_TX_CPHY1CA_HSDATA_SEL:4;
	unsigned MIPI_TX_PHY1_HSDATA_SEL:4;
	unsigned MIPI_TX_PHY2BC_HSDATA_SEL:4;
};


struct MIPITX_DSI_PHY_SEL3_REG {
	unsigned MIPI_TX_PHY3_HSDATA_SEL:4;
	unsigned rsv_4:4;
	unsigned MIPI_TX_PN_SWAP:5;
	unsigned rsv_13:19;
};


struct MIPITX_DSI_SW_CTRL_CON0_REG {
	unsigned MIPI_TX_SW_LNTC_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNTC_LPTX_OE:1;
	unsigned MIPI_TX_SW_LNTC_LPTX_DP:1;
	unsigned MIPI_TX_SW_LNTC_LPTX_DN:1;
	unsigned MIPI_TX_SW_LNTC_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNTC_HSTX_OE:1;
	unsigned MIPI_TX_SW_LNTC_HSTX_RDY:1;
	unsigned MIPI_TX_SW_LNTC_LPRX_EN:1;
	unsigned MIPI_TX_SW_LNTC_HSTX_DATA:8;
	unsigned MIPI_TX_SW_CPHY1CA_LPRX_EN:1;
	unsigned MIPI_TX_SW_CPHY1CA_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY1CA_LPTX_OE:1;
	unsigned rsv_19:5;
	unsigned MIPI_TX_SW_CPHY1CA_HSTX_DATA:7;
	unsigned rsv_31:1;
};


struct MIPITX_DSI_SW_CTRL_CON1_REG {
	unsigned MIPI_TX_SW_LNT0_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT0_LPTX_OE:1;
	unsigned MIPI_TX_SW_LNT0_LPTX_DP:1;
	unsigned MIPI_TX_SW_LNT0_LPTX_DN:1;
	unsigned MIPI_TX_SW_LNT0_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT0_HSTX_OE:1;
	unsigned MIPI_TX_SW_LNT0_HSTX_RDY:1;
	unsigned MIPI_TX_SW_LNT0_LPRX_EN:1;
	unsigned MIPI_TX_SW_LNT1_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT1_LPTX_OE:1;
	unsigned MIPI_TX_SW_LNT1_LPTX_DP:1;
	unsigned MIPI_TX_SW_LNT1_LPTX_DN:1;
	unsigned MIPI_TX_SW_LNT1_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT1_HSTX_OE:1;
	unsigned MIPI_TX_SW_LNT1_HSTX_RDY:1;
	unsigned MIPI_TX_SW_LNT1_LPRX_EN:1;
	unsigned MIPI_TX_SW_LNT2_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT2_LPTX_OE:1;
	unsigned MIPI_TX_SW_LNT2_LPTX_DP:1;
	unsigned MIPI_TX_SW_LNT2_LPTX_DN:1;
	unsigned MIPI_TX_SW_LNT2_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT2_HSTX_OE:1;
	unsigned MIPI_TX_SW_LNT2_HSTX_RDY:1;
	unsigned MIPI_TX_SW_LNT2_LPRX_EN:1;
	unsigned MIPI_TX_SW_LNT3_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT3_LPTX_OE:1;
	unsigned MIPI_TX_SW_LNT3_LPTX_DP:1;
	unsigned MIPI_TX_SW_LNT3_LPTX_DN:1;
	unsigned MIPI_TX_SW_LNT3_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_LNT3_HSTX_OE:1;
	unsigned MIPI_TX_SW_LNT3_HSTX_RDY:1;
	unsigned MIPI_TX_SW_LNT3_LPRX_EN:1;
};


struct MIPITX_DSI_SW_CTRL_CON2_REG {
	unsigned MIPI_TX_SW_LNTD_HSTX_DATA:8;
	unsigned rsv_8:24;
};


struct MIPITX_DSI_SW_CTRL_CON3_REG {
	unsigned MIPI_TX_SW_CPHY1AB_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY1AB_LPTX_OE:1;
	unsigned MIPI_TX_SW_CPHY1AB_LPRX_EN:1;
	unsigned rsv_3:1;
	unsigned MIPI_TX_SW_CPHY2BC_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY2BC_LPTX_OE:1;
	unsigned MIPI_TX_SW_CPHY2BC_LPRX_EN:1;
	unsigned rsv_7:1;
	unsigned MIPI_TX_SW_CPHY0BC_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY0BC_LPTX_OE:1;
	unsigned MIPI_TX_SW_CPHY0BC_LPRX_EN:1;
	unsigned rsv_11:1;
	unsigned MIPI_TX_SW_CPHYXXX_LPTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHYXXX_LPTX_OE:1;
	unsigned MIPI_TX_SW_CPHYXXX_LPRX_EN:1;
	unsigned rsv_15:1;
	unsigned MIPI_TX_SW_CPHY0_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY0_HSTX_OE:1;
	unsigned MIPI_TX_SW_CPHY0_HSTX_DRDY:1;
	unsigned rsv_19:1;
	unsigned MIPI_TX_SW_CPHY1_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY1_HSTX_OE:1;
	unsigned MIPI_TX_SW_CPHY1_HSTX_DRDY:1;
	unsigned rsv_23:1;
	unsigned MIPI_TX_SW_CPHY2_HSTX_PRE_OE:1;
	unsigned MIPI_TX_SW_CPHY2_HSTX_OE:1;
	unsigned MIPI_TX_SW_CPHY2_HSTX_DRDY:1;
	unsigned rsv_27:5;
};


struct MIPITX_DSI_SW_CTRL_CON4_REG {
	unsigned MIPI_TX_SW_CPHY0BC_1AB_2BC_HSTX_DATA:7;
	unsigned rsv_7:1;
	unsigned MIPI_TX_SW_ANA_CK_EN:1;
	unsigned rsv_9:23;
};


struct MIPITX_DSI_CD_CON_REG {
	unsigned MIPI_TX_CD_CON:10;
	unsigned rsv_10:22;
};


struct MIPITX_DSI_DBG_CON_REG {
	unsigned MIPI_TX_DBG_SEL:4;
	unsigned MIPI_TX_DBG_OUT_EN:1;
	unsigned MIPI_TX_GPIO_MODE_EN:1;
	unsigned MIPI_TX_APB_ASYNC_CNT_EN:1;
	unsigned MIPI_TX_TST_CK_OUT_EN:1;
	unsigned MIPI_TX_TST_CK_OUT_SEL:1;
	unsigned rsv_9:23;
};



struct MIPITX_DSI_DBG_OUT_REG {
	unsigned MIPI_TX_DBG_OUT;
};


struct MIPITX_DSI_APB_ASYNC_STA_REG {
	unsigned MIPI_TX_APB_ASYNC_ERR:1;
	unsigned MIPI_TX_APB_ASYNC_ERR_ADDR:10;
	unsigned rsv_11:21;
};


struct MIPITX_DSI_BIST_CON_REG {
	unsigned DPHY_BIST_MODE:1;
	unsigned DPHY_BIST_ENABLE:1;
	unsigned DPHY_BIST_FIX_PATTERN:1;
	unsigned DPHY_BIST_SPECIFIED_PATTERN:1;
	unsigned DPHY_BIST_HS_FREE:1;
	unsigned rsv_5:2;
	unsigned DPHY_BIST_DESKEW_MODE:1;
	unsigned DPHY_BIST_LANE_NUM:4;
	unsigned rsv_12:4;
	unsigned DPHY_BIST_TIMIING:8;
	unsigned DPHY_BIST_CK_HS_PATTERN:8;
};


struct MIPITX_DSI_BIST_PAT_REG {
	unsigned DPHY_BIST_PATTERN;
};


struct MIPITX_CPHY_BIST_CON0_REG {
	unsigned CPHY_BIST_MODE:1;
	unsigned CPHY_BIST_PROGSEQ_EN:1;
	unsigned CPHY_BIST_ESCAPE_EN:1;
	unsigned CPHY_BIST_START:1;
	unsigned CPHY_BIST_PACKET_TEST_MODE:1;
	unsigned CPHY_BIST_PACKET_TEST_MODE_OUT_EN:1;
	unsigned CPHY_BIST_SETTLE_SKIP_EN:1;
	unsigned CPHY_BIST_PREBEGIN_CNT:8;
	unsigned CPHY_BIST_POST_CNT:8;
	unsigned rsv_24:8;
};


struct MIPITX_CPHY_BIST_CON1_REG {
	unsigned CPHY_BIST_PROGSEQ_SYMBOL_LSB;
};


struct MIPITX_CPHY_BIST_CON2_REG {
	unsigned CPHY_BIST_PROGSEQ_SYMBOL_MSB:10;
	unsigned rsv_10:2;
	unsigned CPHY_BIST_PRB_SEED:18;
	unsigned rsv_30:2;
};


struct MIPITX_CPHY_BIST_CON3_REG {
	unsigned CPHY_BIST_PRB_SEL:3;
	unsigned CPHY_BIST_PRB_FIX:1;
	unsigned CPHY_BIST_INIT_WIRE_STATE:3;
	unsigned CPHY_BIST_WIRE_STATE_FIX:1;
	unsigned rsv_8:24;
};


/* field definition */
/* ------------------------------------------------------------- */
/* DSI */

struct DSI_START_REG {
	unsigned DSI_START:1;
	unsigned rsv_1:1;
	unsigned SLEEPOUT_START:1;
	unsigned rsv_3:1;
	unsigned SKEWCAL_START:1;
	unsigned rsv_5:11;
	unsigned VM_CMD_START:1;
	unsigned rsv_17:15;
};


struct DSI_STATUS_REG {
	unsigned rsv_0:4;
	unsigned ESC_ENTRY_ERR:1;
	unsigned ESC_SYNC_ERR:1;
	unsigned CTRL_ERR:1;
	unsigned CONTENT_ERR:1;
	unsigned rsv_8:24;
};


struct DSI_INT_ENABLE_REG {
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
	unsigned BUFFER_UNDERRUN_INT_EN:1;
	unsigned rsv_13:3;
	unsigned LPRX_RD_RDY_EVENT_EN:1;
	unsigned CMD_DONE_EVENT_EN:1;
	unsigned TE_RDY_EVENT_EN:1;
	unsigned VM_DONE_EVENT_EN:1;
	unsigned FRAME_DONE_EVENT_EN:1;
	unsigned VM_CMD_DONE_EVENT_EN:1;
	unsigned SLEEPOUT_DONE_EVENT_EN:1;
	unsigned TE_TIMEOUT_EVENT_EN:1;
	unsigned VM_VBP_STR_EVENT_EN:1;
	unsigned VM_VACT_STR_EVENT_EN:1;
	unsigned VM_VFP_STR_EVENT_EN:1;
	unsigned SKEWCAL_DONE_EVENT_EN:1;
	unsigned BUFFER_UNDERRUN_EVENT_EN:1;
	unsigned res_29:3;
};


struct DSI_INT_STATUS_REG {
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
	unsigned BUFFER_UNDERRUN_INT_EN:1;
	unsigned rsv_13:18;
	unsigned BUSY:1;
};


struct DSI_COM_CTRL_REG {
	unsigned DSI_RESET:1;
	unsigned rsv_1:1;
	unsigned DPHY_RESET:1;
	unsigned rsv_3:1;
	unsigned DSI_DUAL_EN:1;
	unsigned rsv_5:27;
};


enum DSI_MODE_CTRL {
	DSI_CMD_MODE = 0,
	DSI_SYNC_PULSE_VDO_MODE = 1,
	DSI_SYNC_EVENT_VDO_MODE = 2,
	DSI_BURST_VDO_MODE = 3
};


struct DSI_MODE_CTRL_REG {
	unsigned MODE:2;
	unsigned rsv_2:14;
	unsigned FRM_MODE:1;
	unsigned MIX_MODE:1;
	unsigned V2C_SWITCH_ON:1;
	unsigned C2V_SWITCH_ON:1;
	unsigned SLEEP_MODE:1;
	unsigned rsv_21:11;
};


enum DSI_LANE_NUM {
	ONE_LANE = 1,
	TWO_LANE = 2,
	THREE_LANE = 3,
	FOUR_LANE = 4
};


struct DSI_TXRX_CTRL_REG {
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
	unsigned RGB_PKT_CNT:4;
	unsigned LP_ONLY_VBLK:1;
	unsigned rsv_29:3;
};


enum DSI_PS_TYPE {
	PACKED_PS_16BIT_RGB565 = 0,
	LOOSELY_PS_18BIT_RGB666 = 1,
	PACKED_PS_24BIT_RGB888 = 2,
	PACKED_PS_18BIT_RGB666 = 3
};


struct DSI_PSCTRL_REG {
	unsigned DSI_PS_WC:14;
	unsigned rsv_14:2;
	unsigned DSI_PS_SEL:2;
	unsigned rsv_18:6;
	unsigned RGB_SWAP:1;
	unsigned BYTE_SWAP:1;
	unsigned CUSTOM_HEADER:6;
};


struct DSI_VSA_NL_REG {
	unsigned VSA_NL:12;
	unsigned rsv_12:20;
};


struct DSI_VBP_NL_REG {
	unsigned VBP_NL:12;
	unsigned rsv_12:20;
};


struct DSI_VFP_NL_REG {
	unsigned VFP_NL:12;
	unsigned rsv_12:20;
};


struct DSI_VACT_NL_REG {
	unsigned VACT_NL:15;
	unsigned rsv_15:17;
};

struct DSI_LFR_CON_REG {
	unsigned LFR_MODE:2;
	unsigned LFR_TYPE:2;
	unsigned LFR_EN:1;
	unsigned LFR_UPDATE:1;
	unsigned LFR_VSE_DIS:1;
	unsigned rsv_7:1;
	unsigned LFR_SKIP_NUM:6;
	unsigned rsv_14:18;
};

struct DSI_LFR_STA_REG {
	unsigned LFR_SKIP_CNT:6;
	unsigned rsv_6:2;
	unsigned LFR_SKIP_STA:1;
	unsigned rsv_9:23;
};

struct DSI_SIZE_CON_REG {
	unsigned DSI_WIDTH:15;
	unsigned rsv_15:1;
	unsigned DSI_HEIGHT:15;
	unsigned rsv_31:1;
};

struct DSI_HSA_WC_REG {
	unsigned HSA_WC:12;
	unsigned rsv_12:20;
};


struct DSI_HBP_WC_REG {
	unsigned HBP_WC:12;
	unsigned rsv_12:20;
};


struct DSI_HFP_WC_REG {
	unsigned HFP_WC:12;
	unsigned rsv_12:20;
};

struct DSI_BLLP_WC_REG {
	unsigned BLLP_WC:12;
	unsigned rsv_12:20;
};

struct DSI_CMDQ_CTRL_REG {
	unsigned CMDQ_SIZE:8;
	unsigned rsv_8:24;
};

struct DSI_HSTX_CKLP_REG {
	unsigned rsv_0:2;
	unsigned HSTX_CKLP_WC:14;
	unsigned HSTX_CKLP_WC_AUTO:1;
	unsigned rsv_17:15;
};

struct DSI_HSTX_CKLP_WC_AUTO_RESULT_REG {
	unsigned HSTX_CKLP_WC_AUTO_RESULT:16;
	unsigned rsv_16:16;
};

struct DSI_RX_DATA_REG {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};


struct DSI_RACK_REG {
	unsigned DSI_RACK:1;
	unsigned DSI_RACK_BYPASS:1;
	unsigned rsv2:30;
};


struct DSI_TRIG_STA_REG {
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
};


struct DSI_MEM_CONTI_REG {
	unsigned RWMEM_CONTI:16;
	unsigned rsv16:16;
};


struct DSI_FRM_BC_REG {
	unsigned FRM_BC:21;
	unsigned rsv21:11;
};

struct DSI_3D_CON_REG {
	unsigned _3D_MODE:2;
	unsigned _3D_FMT:2;
	unsigned _3D_VSYNC:1;
	unsigned _3D_LR:1;
	unsigned rsv6:2;
	unsigned _3D_EN:1;
	unsigned rsv08:23;
};

struct DSI_TIME_CON0_REG {
	unsigned UPLS_WAKEUP_PRD:16;
	unsigned SKEWCALL_PRD:16;
};

struct DSI_TIME_CON1_REG {
	unsigned TE_TIMEOUT_PRD:16;
	unsigned PREFETCH_TIME:15;
	unsigned PREFETCH_EN:1;
};


struct DSI_PHY_LCPAT_REG {
	unsigned LC_HSTX_CK_PAT:8;
	unsigned rsv8:24;
};

struct DSI_PHY_LCCON_REG {
	unsigned LC_HS_TX_EN:1;
	unsigned LC_ULPM_EN:1;
	unsigned LC_WAKEUP_EN:1;
	unsigned rsv3:29;
};


struct DSI_PHY_LD0CON_REG {
	unsigned L0_RM_TRIG_EN:1;
	unsigned L0_ULPM_EN:1;
	unsigned L0_WAKEUP_EN:1;
	unsigned Lx_ULPM_AS_L0:1;
	unsigned L0_RX_FILTER_EN:1;
	unsigned rsv3:27;
};


struct DSI_PHY_SYNCON_REG {
	unsigned HS_SYNC_CODE:8;
	unsigned HS_SYNC_CODE2:8;
	unsigned HS_SKEWCAL_PAT:8;
	unsigned HS_DB_SYNC_EN:1;
	unsigned rsv25:7;
};


struct DSI_PHY_TIMCON0_REG {
	unsigned char LPX;
	unsigned char HS_PRPR;
	unsigned char HS_ZERO;
	unsigned char HS_TRAIL;
};


struct DSI_PHY_TIMCON1_REG {
	unsigned char TA_GO;
	unsigned char TA_SURE;
	unsigned char TA_GET;
	unsigned char DA_HS_EXIT;
};


struct DSI_PHY_TIMCON2_REG {
	unsigned char CONT_DET;
	unsigned char DA_HS_SYNC;
	unsigned char CLK_ZERO;
	unsigned char CLK_TRAIL;
};


struct DSI_PHY_TIMCON3_REG {
	unsigned char CLK_HS_PRPR;
	unsigned char CLK_HS_POST;
	unsigned char CLK_HS_EXIT;
	unsigned rsv24:8;
};


struct DSI_CPHY_CON0_REG {
	unsigned CPHY_EN:1;
	unsigned SETTLE_SKIP_EN:1;
	unsigned PROGSEQ_SKIP_EN:1;
	unsigned rsv3:1;
	unsigned CPHY_PROGSEQMSB:10;
	unsigned rsv14:2;
	unsigned CPHY_INIT_STATE:9;
	unsigned rsv25:3;
	unsigned CPHY_CONTI_CLK:4;
};


struct DSI_CPHY_CON1_REG {
	unsigned CPHY_PROGSEQLSB;
};


struct DSI_CPHY_DBG0_REG {
	unsigned CPHYHS_STATE_DA0:9;
	unsigned rsv9:7;
	unsigned CPHYHS_STATE_DA1:9;
	unsigned rsv25:7;
};


struct DSI_CPHY_DBG1_REG {
	unsigned CPHYHS_STATE_DA2:9;
	unsigned rsv9:23;
};


struct DSI_VM_CMD_CON_REG {
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
};


struct DSI_PHY_TIMCON_REG {
	struct DSI_PHY_TIMCON0_REG CTRL0;
	struct DSI_PHY_TIMCON1_REG CTRL1;
	struct DSI_PHY_TIMCON2_REG CTRL2;
	struct DSI_PHY_TIMCON3_REG CTRL3;
};


struct DSI_CKSM_OUT_REG {
	unsigned PKT_CHECK_SUM:16;
	unsigned ACC_CHECK_SUM:16;
};


struct DSI_STATE_DBG0_REG {
	unsigned DPHY_CTL_STATE_C:9;
	unsigned rsv9:7;
	unsigned DPHY_HS_TX_STATE_C:5;
	unsigned rsv21:11;
};


struct DSI_STATE_DBG1_REG {
	unsigned CTL_STATE_C:15;
	unsigned rsv15:1;
	unsigned HS_TX_STATE_0:5;
	unsigned rsv21:3;
	unsigned ESC_STATE_0:8;
};


struct DSI_STATE_DBG2_REG {
	unsigned RX_ESC_STATE:10;
	unsigned rsv10:6;
	unsigned TA_T2R_STATE:5;
	unsigned rsv21:3;
	unsigned TA_R2T_STATE:5;
	unsigned rsv29:3;
};

struct DSI_STATE_DBG3_REG {
	unsigned CTL_STATE_1:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_1:5;
	unsigned rsv13:3;
	unsigned CTL_STATE_2:5;
	unsigned rsv21:3;
	unsigned HS_TX_STATE_2:5;
	unsigned rsv29:3;
};

struct DSI_STATE_DBG4_REG {
	unsigned CTL_STATE_3:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_3:5;
	unsigned rsv13:19;
};

struct DSI_STATE_DBG5_REG {
	unsigned TIMER_COUNTER:16;
	unsigned TIMER_BUSY:1;
	unsigned rsv17:11;
	unsigned WAKEUP_STATE:4;
};


struct DSI_STATE_DBG6_REG {
	unsigned CMTRL_STATE:15;
	unsigned rsv15:2;
	unsigned CMDQ_STATE:7;
	unsigned rsv24:8;
};

struct DSI_STATE_DBG7_REG {
	unsigned VMCTL_STATE:11;
	unsigned rsv11:1;
	unsigned VFP_PERIOD:1;
	unsigned VACT_PERIOD:1;
	unsigned VBP_PERIOD:1;
	unsigned VSA_PERIOD:1;
	unsigned rsv16:16;
};

struct DSI_STATE_DBG8_REG {
	unsigned WORD_COUNTER:14;
	unsigned rsv14:2;
	unsigned PREFETCH_COUNTER:15;
	unsigned rsv31:1;
};

struct DSI_STATE_DBG9_REG {
	unsigned LINE_COUNTER:22;
	unsigned rsv22:10;
};

struct DSI_DEBUG_SEL_REG {
	unsigned DEBUG_OUT_SEL:5;
	unsigned rsv5:3;
	unsigned CHKSUM_REC_EN:1;
	unsigned C2V_START_CON:1;
	unsigned rsv10:4;
	unsigned DYNAMIC_CG_CON:18;
};

struct DSI_STATE_DBG10_REG {
	unsigned LIMIT_W:15;
	unsigned rsv15:1;
	unsigned LIMIT_H:15;
	unsigned rsv31:1;
};


struct DSI_BIST_CON_REG {
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
};


struct DSI_SHADOW_DEBUG_REG {
	unsigned FORCE_COMMIT:1;
	unsigned BYPASS_SHADOW:1;
	unsigned READ_WORKING:1;
	unsigned rsv3:29;
};


struct DSI_SHADOW_STA_REG {
	unsigned VACT_UPDATE_ERR:1;
	unsigned VFP_UPDATE_ERR:1;
	unsigned rsv2:30;
};


struct DSI_REGS {
	struct DSI_START_REG DSI_START;	/* 0000 */
	struct DSI_STATUS_REG DSI_STA;	/* 0004 */
	struct DSI_INT_ENABLE_REG DSI_INTEN;	/* 0008 */
	struct DSI_INT_STATUS_REG DSI_INTSTA;	/* 000C */
	struct DSI_COM_CTRL_REG DSI_COM_CTRL;	/* 0010 */
	struct DSI_MODE_CTRL_REG DSI_MODE_CTRL;	/* 0014 */
	struct DSI_TXRX_CTRL_REG DSI_TXRX_CTRL;	/* 0018 */
	struct DSI_PSCTRL_REG DSI_PSCTRL;	/* 001C */
	struct DSI_VSA_NL_REG DSI_VSA_NL;	/* 0020 */
	struct DSI_VBP_NL_REG DSI_VBP_NL;	/* 0024 */
	struct DSI_VFP_NL_REG DSI_VFP_NL;	/* 0028 */
	struct DSI_VACT_NL_REG DSI_VACT_NL;	/* 002C */
	struct DSI_LFR_CON_REG DSI_LFR_CON;	/* 0030 */
	struct DSI_LFR_STA_REG DSI_LFR_STA;	/* 0034 */
	struct DSI_SIZE_CON_REG DSI_SIZE_CON;	/* 0038 */
	UINT32 rsv_3c[5];	/* 003C..004C */
	struct DSI_HSA_WC_REG DSI_HSA_WC;	/* 0050 */
	struct DSI_HBP_WC_REG DSI_HBP_WC;	/* 0054 */
	struct DSI_HFP_WC_REG DSI_HFP_WC;	/* 0058 */
	struct DSI_BLLP_WC_REG DSI_BLLP_WC;	/* 005C */
	struct DSI_CMDQ_CTRL_REG DSI_CMDQ_SIZE;	/* 0060 */
	struct DSI_HSTX_CKLP_REG DSI_HSTX_CKL_WC;	/* 0064 */
	struct DSI_HSTX_CKLP_WC_AUTO_RESULT_REG DSI_HSTX_CKL_WC_AUTO_RESULT;	/* 0068 */
	UINT32 rsv_006C[2];	/* 006c..0070 */
	struct DSI_RX_DATA_REG DSI_RX_DATA0;	/* 0074 */
	struct DSI_RX_DATA_REG DSI_RX_DATA1;	/* 0078 */
	struct DSI_RX_DATA_REG DSI_RX_DATA2;	/* 007c */
	struct DSI_RX_DATA_REG DSI_RX_DATA3;	/* 0080 */
	struct DSI_RACK_REG DSI_RACK;	/* 0084 */
	struct DSI_TRIG_STA_REG DSI_TRIG_STA;	/* 0088 */
	UINT32 rsv_008C;	/* 008C */
	struct DSI_MEM_CONTI_REG DSI_MEM_CONTI;	/* 0090 */
	struct DSI_FRM_BC_REG DSI_FRM_BC;	/* 0094 */
	struct DSI_3D_CON_REG DSI_3D_CON;	/* 0098 */
	UINT32 rsv_009C;	/* 009c */
	struct DSI_TIME_CON0_REG DSI_TIME_CON0;	/* 00A0 */
	struct DSI_TIME_CON1_REG DSI_TIME_CON1;	/* 00A4 */

	UINT32 rsv_00A8[22];	/* 0A8..0FC */
	UINT32 DSI_PHY_PCPAT;	/* 00100 */

	struct DSI_PHY_LCCON_REG DSI_PHY_LCCON;	/* 0104 */
	struct DSI_PHY_LD0CON_REG DSI_PHY_LD0CON;	/* 0108 */
	struct DSI_PHY_SYNCON_REG DSI_PHY_SYNCON;	/* 010C */
	struct DSI_PHY_TIMCON0_REG DSI_PHY_TIMECON0;	/* 0110 */
	struct DSI_PHY_TIMCON1_REG DSI_PHY_TIMECON1;	/* 0114 */
	struct DSI_PHY_TIMCON2_REG DSI_PHY_TIMECON2;	/* 0118 */
	struct DSI_PHY_TIMCON3_REG DSI_PHY_TIMECON3;	/* 011C */
	struct DSI_CPHY_CON0_REG DSI_CPHY_CON0;	/* 0120 */
	struct DSI_CPHY_CON1_REG DSI_CPHY_CON1;	/* 0124 */
	struct DSI_CPHY_DBG0_REG DSI_CPHY_DBG0;	/* 0128 */
	struct DSI_CPHY_DBG1_REG DSI_CPHY_DBG1;	/* 012C */
	struct DSI_VM_CMD_CON_REG DSI_VM_CMD_CON;	/* 0130 */
	UINT32 DSI_VM_CMD_DATA0;	/* 0134 */
	UINT32 DSI_VM_CMD_DATA4;	/* 0138 */
	UINT32 DSI_VM_CMD_DATA8;	/* 013C */
	UINT32 DSI_VM_CMD_DATAC;	/* 0140 */
	struct DSI_CKSM_OUT_REG DSI_CKSM_OUT;	/* 0144 */
	struct DSI_STATE_DBG0_REG DSI_STATE_DBG0;	/* 0148 */
	struct DSI_STATE_DBG1_REG DSI_STATE_DBG1;	/* 014C */
	struct DSI_STATE_DBG2_REG DSI_STATE_DBG2;	/* 0150 */
	struct DSI_STATE_DBG3_REG DSI_STATE_DBG3;	/* 0154 */
	struct DSI_STATE_DBG4_REG DSI_STATE_DBG4;	/* 0158 */
	struct DSI_STATE_DBG5_REG DSI_STATE_DBG5;	/* 015C */
	struct DSI_STATE_DBG6_REG DSI_STATE_DBG6;	/* 0160 */
	struct DSI_STATE_DBG7_REG DSI_STATE_DBG7;	/* 0164 */
	struct DSI_STATE_DBG8_REG DSI_STATE_DBG8;	/* 0168 */
	struct DSI_STATE_DBG9_REG DSI_STATE_DBG9;	/* 016C */
	struct DSI_DEBUG_SEL_REG DSI_DEBUG_SEL;	/* 0170 */
	struct DSI_STATE_DBG10_REG DSI_STATE_DBG10;	/* 0174 */
	UINT32 DSI_BIST_PATTERN;	/* 0178 */
	struct DSI_BIST_CON_REG DSI_BIST_CON;	/* 017C */
	UINT32 DSI_VM_CMD_DATA10;	/* 00180 */
	UINT32 DSI_VM_CMD_DATA14;	/* 00184 */
	UINT32 DSI_VM_CMD_DATA18;	/* 00188 */
	UINT32 DSI_VM_CMD_DATA1C;	/* 0018C */
	struct DSI_SHADOW_DEBUG_REG DSI_SHADOW_DEBUG;	/* 0190 */
	struct DSI_SHADOW_STA_REG DSI_SHADOW_STA;	/* 0194 */
};

/* 0~1 TYPE ,2 BTA,3 HS, 4 CL,5 TE,6~7 RESV, 8~15 DATA_ID,16~23 DATA_0,24~31 DATA_1 */
struct DSI_CMDQ {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};

struct DSI_CMDQ_REGS {
	struct DSI_CMDQ data[128];
};

struct DSI_VM_CMDQ {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};

struct DSI_VM_CMDQ_REGS {
	struct DSI_VM_CMDQ data[4];
};

struct DSI_PHY_REGS {
	struct MIPITX_DSI_IMPENDANCE_0_REG MIPITX_DSI_IMPENDANCE_0;	/* 0000 */
	struct MIPITX_DSI_IMPENDANCE_1_REG MIPITX_DSI_IMPENDANCE_1;	/* 0004 */
	struct MIPITX_DSI_IMPENDANCE_2_REG MIPITX_DSI_IMPENDANCE_2;	/* 0008 */
	struct MIPITX_DSI_LANE_CON_REG MIPITX_DSI_LANE_CON;		/* 000C */
	struct MIPITX_DSI_VOLTAGE_SEL_REG MIPITX_DSI_VOLTAGE_SEL;	/* 0010 */
	struct MIPITX_DSI_PRESERVED_REG MIPITX_DSI_PRESERVED;		/* 0014 */
	struct MIPITX_DSI_TEST_CON_REG MIPITX_DSI_TEST_CON;		/* 0018 */
	UINT32 rsv_1C[3];	/* 001C..0024 */
	struct MIPITX_DSI_PLL_PWR_REG MIPITX_DSI_PLL_PWR;		/* 0028 */
	struct MIPITX_DSI_PLL_CON0_REG MIPITX_DSI_PLL_CON0;		/* 002C */
	struct MIPITX_DSI_PLL_CON1_REG MIPITX_DSI_PLL_CON1;		/* 0030 */
	struct MIPITX_DSI_PLL_CON2_REG MIPITX_DSI_PLL_CON2;		/* 0034 */
	struct MIPITX_DSI_PLL_CON3_REG MIPITX_DSI_PLL_CON3;		/* 0038 */
	struct MIPITX_DSI_PLL_CON4_REG MIPITX_DSI_PLL_CON4;		/* 003C */
	struct MIPITX_DSI_PHY_SEL0_REG MIPITX_DSI_PHY_SEL0;		/* 0040 */
	struct MIPITX_DSI_PHY_SEL1_REG MIPITX_DSI_PHY_SEL1;		/* 0044 */
	struct MIPITX_DSI_PHY_SEL2_REG MIPITX_DSI_PHY_SEL2;		/* 0048 */
	struct MIPITX_DSI_PHY_SEL3_REG MIPITX_DSI_PHY_SEL3;		/* 004C */
	struct MIPITX_DSI_SW_CTRL_CON0_REG MIPITX_DSI_SW_CTRL_CON0;	/* 0050 */
	struct MIPITX_DSI_SW_CTRL_CON1_REG MIPITX_DSI_SW_CTRL_CON1;	/* 0054 */
	struct MIPITX_DSI_SW_CTRL_CON2_REG MIPITX_DSI_SW_CTRL_CON2;	/* 0058 */
	struct MIPITX_DSI_SW_CTRL_CON3_REG MIPITX_DSI_SW_CTRL_CON3;	/* 005C */
	struct MIPITX_DSI_SW_CTRL_CON4_REG MIPITX_DSI_SW_CTRL_CON4;	/* 0060 */
	UINT32 rsv_64;	/* 0064 */
	struct MIPITX_DSI_CD_CON_REG MIPITX_DSI_CD_CON;		/* 0068 */
	UINT32 rsv_6C;	/* 006C */
	struct MIPITX_DSI_DBG_CON_REG MIPITX_DSI_DBG_CON;		/* 0070 */
	UINT32 MIPITX_DSI_DBG_OUT;				/* 0074 */
	struct MIPITX_DSI_APB_ASYNC_STA_REG MIPITX_DSI_APB_ASYNC_STA;	/* 0078 */
	UINT32 rsv_7C;	/* 007C */
	struct MIPITX_DSI_BIST_CON_REG MIPITX_DSI_BIST_CON;		/* 0080 */
	UINT32 MIPITX_DSI_BIST_PAT;				/* 0084 */
	UINT32 rsv_88[2];	/* 0088..008C */
	struct MIPITX_CPHY_BIST_CON0_REG MIPITX_CPHY_BIST_CON0;	/* 0090 */
	UINT32 MIPITX_CPHY_BIST_CON1;				/* 0094 */
	struct MIPITX_CPHY_BIST_CON2_REG MIPITX_CPHY_BIST_CON2;	/* 0098 */
	struct MIPITX_CPHY_BIST_CON3_REG MIPITX_CPHY_BIST_CON3;	/* 009c */
};


/* mipi and dsi's extern variable */
extern unsigned long dispsys_reg[DISP_REG_NUM];
extern unsigned long ddp_reg_pa_base[DISP_REG_NUM];
extern unsigned long mipi_tx0_reg;
extern unsigned long mipi_tx1_reg;
extern unsigned long dsi_reg_va[2];

/* DTS will assign reigister address dynamically, so can not define to 0x1000 */
/* #define DISP_INDEX_OFFSET 0x1000 */

#define DDP_REG_BASE_MMSYS_CONFIG  dispsys_reg[DISP_REG_CONFIG]
#define DDP_REG_BASE_DISP_OVL0     dispsys_reg[DISP_REG_OVL0]
#define DDP_REG_BASE_DISP_OVL1     dispsys_reg[DISP_REG_OVL1]
#define DDP_REG_BASE_DISP_OVL0_2L  dispsys_reg[DISP_REG_OVL0_2L]
#define DDP_REG_BASE_DISP_OVL1_2L  dispsys_reg[DISP_REG_OVL1_2L]
#define DDP_REG_BASE_DISP_RDMA0    dispsys_reg[DISP_REG_RDMA0]
#define DDP_REG_BASE_DISP_RDMA1    dispsys_reg[DISP_REG_RDMA1]
#define DDP_REG_BASE_DISP_RDMA2    dispsys_reg[DISP_REG_RDMA2]
#define DDP_REG_BASE_DISP_WDMA0    dispsys_reg[DISP_REG_WDMA0]
#define DDP_REG_BASE_DISP_WDMA1    dispsys_reg[DISP_REG_WDMA1]
#define DDP_REG_BASE_DISP_COLOR0   dispsys_reg[DISP_REG_COLOR0]
#define DDP_REG_BASE_DISP_COLOR1   dispsys_reg[DISP_REG_COLOR1]
#define DDP_REG_BASE_DISP_CCORR0   dispsys_reg[DISP_REG_CCORR0]
#define DDP_REG_BASE_DISP_CCORR1   dispsys_reg[DISP_REG_CCORR1]
#define DDP_REG_BASE_DISP_AAL0     dispsys_reg[DISP_REG_AAL0]
#define DDP_REG_BASE_DISP_AAL1     dispsys_reg[DISP_REG_AAL1]
#define DDP_REG_BASE_DISP_GAMMA0   dispsys_reg[DISP_REG_GAMMA0]
#define DDP_REG_BASE_DISP_GAMMA1   dispsys_reg[DISP_REG_GAMMA1]
#define DDP_REG_BASE_DISP_OD       dispsys_reg[DISP_REG_OD]
#define DDP_REG_BASE_DISP_DITHER0  dispsys_reg[DISP_REG_DITHER0]
#define DDP_REG_BASE_DISP_DITHER1  dispsys_reg[DISP_REG_DITHER1]
#define DDP_REG_BASE_DISP_UFOE     dispsys_reg[DISP_REG_UFOE]
#define DDP_REG_BASE_DISP_DSC      dispsys_reg[DISP_REG_DSC]
#define DDP_REG_BASE_DISP_DSC_2ND  dispsys_reg[DISP_REG_DSC_2ND]
#define DDP_REG_BASE_DISP_SPLIT0   dispsys_reg[DISP_REG_SPLIT0]
#define DDP_REG_BASE_DSI0          dispsys_reg[DISP_REG_DSI0]
#define DDP_REG_BASE_DSI1          dispsys_reg[DISP_REG_DSI1]
#define DDP_REG_BASE_DPI           dispsys_reg[DISP_REG_DPI0]
#define DDP_REG_BASE_DISP_PWM0     dispsys_reg[DISP_REG_PWM0]
#define DDP_REG_BASE_DISP_PWM1     dispsys_reg[DISP_REG_PWM1]
#define DDP_REG_BASE_MM_MUTEX      dispsys_reg[DISP_REG_MUTEX]
#define DDP_REG_BASE_DISP_RSZ0     dispsys_reg[DISP_REG_RSZ0]
#define DDP_REG_BASE_DISP_RSZ1     dispsys_reg[DISP_REG_RSZ1]
#define DDP_REG_BASE_SMI_LARB0     dispsys_reg[DISP_REG_SMI_LARB0]
#define DDP_REG_BASE_SMI_LARB1     dispsys_reg[DISP_REG_SMI_LARB1]
#define DDP_REG_BASE_SMI_COMMON    dispsys_reg[DISP_REG_SMI_COMMON]
#define DDP_REG_BASE_CLOCK_MUX     dispsys_reg[DISP_REG_CLOCK_MUX]

#define MIPI_TX0_REG_BASE			(mipi_tx0_reg)
#define MIPI_TX1_REG_BASE			(mipi_tx1_reg)

#define DISPSYS_REG_ADDR_MIN            (DISPSYS_CONFIG_BASE)
#define DISPSYS_REG_ADDR_MAX            (DDP_REG_BASE_DISP_OVL1_2L)

#define DISPSYS_CONFIG_BASE			DDP_REG_BASE_MMSYS_CONFIG
#define DISPSYS_OVL0_BASE		        DDP_REG_BASE_DISP_OVL0
#define DISPSYS_OVL1_BASE		        DDP_REG_BASE_DISP_OVL1
#define DISPSYS_OVL0_2L_BASE		    DDP_REG_BASE_DISP_OVL0_2L
#define DISPSYS_OVL1_2L_BASE		    DDP_REG_BASE_DISP_OVL1_2L
#define DISPSYS_RDMA0_BASE		        DDP_REG_BASE_DISP_RDMA0
#define DISPSYS_RDMA1_BASE		        DDP_REG_BASE_DISP_RDMA1
#define DISPSYS_RDMA2_BASE		        DDP_REG_BASE_DISP_RDMA2
#define DISPSYS_WDMA0_BASE		        DDP_REG_BASE_DISP_WDMA0
#define DISPSYS_WDMA1_BASE		        DDP_REG_BASE_DISP_WDMA1
#define DISPSYS_COLOR0_BASE		        DDP_REG_BASE_DISP_COLOR0
#define DISPSYS_COLOR1_BASE		        DDP_REG_BASE_DISP_COLOR1
#define DISPSYS_CCORR0_BASE		        DDP_REG_BASE_DISP_CCORR0
#define DISPSYS_CCORR1_BASE		        DDP_REG_BASE_DISP_CCORR1
#define DISPSYS_AAL0_BASE		        DDP_REG_BASE_DISP_AAL0
#define DISPSYS_AAL1_BASE		        DDP_REG_BASE_DISP_AAL1
#define DISPSYS_GAMMA0_BASE		        DDP_REG_BASE_DISP_GAMMA0
#define DISPSYS_GAMMA1_BASE		        DDP_REG_BASE_DISP_GAMMA1
#define DISPSYS_OD_BASE                 DDP_REG_BASE_DISP_OD
#define DISPSYS_DITHER0_BASE		    DDP_REG_BASE_DISP_DITHER0
#define DISPSYS_DITHER1_BASE		    DDP_REG_BASE_DISP_DITHER1
#define DISPSYS_UFOE_BASE		        DDP_REG_BASE_DISP_UFOE
#define DISPSYS_DSC_BASE		        DDP_REG_BASE_DISP_DSC
#define DISPSYS_DSC_2ND_BASE			DDP_REG_BASE_DISP_DSC_2ND
#define DISPSYS_SPLIT0_BASE		        DDP_REG_BASE_DISP_SPLIT0
#define DISPSYS_DSI0_BASE		        DDP_REG_BASE_DSI0
#define DISPSYS_DSI1_BASE		        DDP_REG_BASE_DSI1
#define DISPSYS_DPI_BASE				DDP_REG_BASE_DPI
#define DISPSYS_PWM0_BASE		        DDP_REG_BASE_DISP_PWM0
#define DISPSYS_PWM1_BASE		        DDP_REG_BASE_DISP_PWM1
#define DISPSYS_MUTEX_BASE			DDP_REG_BASE_MM_MUTEX
#define DISPSYS_RSZ0_BASE				DDP_REG_BASE_DISP_RSZ0
#define DISPSYS_RSZ1_BASE				DDP_REG_BASE_DISP_RSZ1
#define DISPSYS_SMI_LARB0_BASE		    DDP_REG_BASE_SMI_LARB0
#define DISPSYS_SMI_LARB1_BASE		    DDP_REG_BASE_SMI_LARB1
#define DISPSYS_SMI_COMMON_BASE		    DDP_REG_BASE_SMI_COMMON
#define DISPSYS_CLOCK_MUX_BASE		    DDP_REG_BASE_CLOCK_MUX

#define MIPITX0_BASE					MIPI_TX0_REG_BASE
#define MIPITX1_BASE					MIPI_TX1_REG_BASE

#ifdef INREG32
#undef INREG32
#define INREG32(x)          (__raw_readl((volatile unsigned long*)(x)))
#endif

/* --------------------------------------------------------------------------- */
/* Register Field Access */
/* --------------------------------------------------------------------------- */

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
	do {  \
		unsigned int regval; \
		regval = __raw_readl((volatile unsigned long*)(reg32)); \
		regval	= (regval & ~REG_FLD_MASK(field)) | (REG_FLD_VAL((field), (val))); \
		mt_reg_sync_writel(regval, (reg32));  \
	} while (0)

/* after apply device tree va/pa is not mapped by a fixed offset */
static inline unsigned long disp_addr_convert(unsigned long va)
{
	unsigned int i = 0;
	unsigned long dispsys_va;
	unsigned long dispsys_pa;

	for (i = 0; i < DISP_REG_NUM; i++) {
		dispsys_va = dispsys_reg[i] & (~0xfffl);
		if (dispsys_va == (va & (~0xfffl))) {
			dispsys_pa = ddp_reg_pa_base[i] & (~0xfffl);
			return dispsys_pa + (va & 0xfffl);
		}
	}
	pr_err("DDP/can not find reg addr for va=0x%lx!\n", va);
	ASSERT(0);
	return 0;
}


#define DISP_REG_MASK(handle, reg32, val, mask)	\
	do { \
		 if (handle == NULL) { \
			mt_reg_sync_writel((unsigned int)(INREG32(reg32)&~(mask))|(val), (reg32));\
		 } else { \
			dprec_reg_op(handle, disp_addr_convert((unsigned long)(reg32)), val, mask);\
			disp_cmdq_write_reg(handle, disp_addr_convert((unsigned long)(reg32)), val, mask); \
		 }	\
	} while (0)

#define DISP_REG_SET(handle, reg32, val) \
	do { \
		if (handle == NULL) { \
			mt_reg_sync_writel(val, (volatile unsigned long*)(reg32));\
		} else { \
			dprec_reg_op(handle, disp_addr_convert((unsigned long)(reg32)), val, 0x00000000);\
			disp_cmdq_write_reg(handle, disp_addr_convert((unsigned long)(reg32)), val, ~0); \
		}  \
	} while (0)


#define DISP_REG_SET_FIELD(handle, field, reg32, val)  \
	do {  \
		if (handle == NULL) { \
			unsigned int regval; \
			regval = __raw_readl((volatile unsigned long*)(reg32)); \
			regval  = (regval & ~REG_FLD_MASK(field)) | (REG_FLD_VAL((field), (val))); \
			mt_reg_sync_writel(regval, (reg32));  \
		} else { \
			dprec_reg_op(handle, disp_addr_convert((unsigned long)(reg32)),\
				       (val)<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
			disp_cmdq_write_reg(handle, disp_addr_convert((unsigned long)(reg32)),\
				       (val)<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		} \
	} while (0)

#define DISP_REG_CMDQ_POLLING(handle, reg32, val, mask) \
	do { \
		if (handle == NULL) { \
			while (!((DISP_REG_GET(reg32))&(val)))\
				; \
		} else { \
			dprec_reg_op(handle, disp_addr_convert((unsigned long)(reg32)), val, 0x00000000);\
			disp_cmdq_poll_reg(handle, disp_addr_convert((unsigned long)(reg32)), val, mask); \
		}  \
	} while (0)

#define DISP_REG_BACKUP(handle, hSlot, idx, reg32) \
	do { \
		if (handle != NULL) { \
			if (hSlot) \
				disp_cmdq_read_reg_to_slot(handle, hSlot, idx,\
							    disp_addr_convert((unsigned long)(reg32)));\
		}  \
	} while (0)

/* Helper macros for local command queue */
#define DISP_CMDQ_BEGIN(__cmdq, scenario) \
	do { \
		disp_cmdq_create(scenario, &__cmdq, __func__);\
		disp_cmdq_reset(__cmdq);\
		ddp_insert_config_allow_rec(__cmdq);\
	} while (0)

#define DISP_CMDQ_REG_SET(__cmdq, reg32, val, mask) DISP_REG_MASK(__cmdq, reg32, val, mask)

#define DISP_CMDQ_CONFIG_STREAM_DIRTY(__cmdq) ddp_insert_config_dirty_rec(__cmdq)

#define DISP_CMDQ_END(__cmdq)		\
	do {				\
		disp_cmdq_flush(__cmdq, __func__, __LINE__);	\
		disp_cmdq_destroy(__cmdq, __func__, __LINE__); \
	} while (0)

/********************************/

#include "ddp_reg_ovl.h"

/*******************************/
/* field definition */
/* ------------------------------------------------------------- */
/* AAL */
#define DISP_AAL_EN                             (DISPSYS_AAL0_BASE + 0x000)
#define DISP_AAL_RESET                          (DISPSYS_AAL0_BASE + 0x004)
#define DISP_AAL_INTEN                          (DISPSYS_AAL0_BASE + 0x008)
#define DISP_AAL_INTSTA                         (DISPSYS_AAL0_BASE + 0x00c)
#define DISP_AAL_STATUS                         (DISPSYS_AAL0_BASE + 0x010)
#define DISP_AAL_CFG                            (DISPSYS_AAL0_BASE + 0x020)
#define DISP_AAL_IN_CNT                         (DISPSYS_AAL0_BASE + 0x024)
#define DISP_AAL_OUT_CNT                        (DISPSYS_AAL0_BASE + 0x028)
#define DISP_AAL_CHKSUM                         (DISPSYS_AAL0_BASE + 0x02c)
#define DISP_AAL_SIZE                           (DISPSYS_AAL0_BASE + 0x030)
#define DISP_AAL_DUMMY_REG                      (DISPSYS_AAL0_BASE + 0x0c0)
#define DISP_AAL_SHADOW_CTL                     (DISPSYS_AAL0_BASE + 0x0c4)
#define DISP_AAL_SHADOW_CTL_2                   (DISPSYS_AAL0_BASE + 0x0f0)
#define DISP_AAL_MAX_HIST_CONFIG_00             (DISPSYS_AAL0_BASE + 0x204)
#define DISP_AAL_CABC_00                        (DISPSYS_AAL0_BASE + 0x20c)
#define DISP_AAL_CABC_02                        (DISPSYS_AAL0_BASE + 0x214)
#define DISP_AAL_CABC_04                        (DISPSYS_AAL0_BASE + 0x21c)
#define DISP_AAL_STATUS_00                      (DISPSYS_AAL0_BASE + 0x224)
/* 00 ~ 32: max histogram */
#define DISP_AAL_STATUS_32                      (DISPSYS_AAL0_BASE + 0x2a4)
/* bit 8: dre_gain_force_en */
#define DISP_AAL_DRE_GAIN_FILTER_00             (DISPSYS_AAL0_BASE + 0x354)
#define DISP_AAL_DRE_FLT_FORCE(idx)             (DISPSYS_AAL0_BASE + 0x358 + (idx) * 4)
#define DISP_AAL_DRE_FLT_FORCE_11               (DISPSYS_AAL0_BASE + 0x44C)
#define DISP_AAL_DRE_CRV_CAL_00                 (DISPSYS_AAL0_BASE + 0x344)
#define DISP_AAL_DRE_MAPPING_00                 (DISPSYS_AAL0_BASE + 0x3b0)
#define DISP_AAL_DRE_MAPPING_00_2               (DISPSYS_AAL0_BASE + 0x3b4)
#define DISP_AAL_CABC_GAINLMT_TBL(idx)          (DISPSYS_AAL0_BASE + 0x40c + (idx) * 4)
#define DISP_AAL_CABC_GAINLMT_TBL_2(idx)        (DISPSYS_AAL0_BASE + 0x410 + (idx) * 4)


#define DISP_PWM_EN_OFF                         (0x00)
#define DISP_PWM_RST_RX                         (0x04)
#define DISP_PWM_COMMIT_OFF                     (0x0C)
#define DISP_PWM_CON_0_OFF                      (0x18)
#define DISP_PWM_CON_1_OFF                      (0x1C)
#define DISP_PWM_DEBUG                          (0x80)

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
#define DISP_COLOR_SHADOW_CTRL          (DISPSYS_COLOR0_BASE+0xCB0)
#define DISP_COLOR_R0_CRC               (DISPSYS_COLOR0_BASE+0xCF0)
#define DISP_COLOR_S_GAIN_BY_Y0_0       (DISPSYS_COLOR0_BASE+0xCF4)
#define DISP_COLOR_S_GAIN_BY_Y0_1       (DISPSYS_COLOR0_BASE+0xCF8)
#define DISP_COLOR_S_GAIN_BY_Y0_2       (DISPSYS_COLOR0_BASE+0xCFC)
#define DISP_COLOR_S_GAIN_BY_Y0_3       (DISPSYS_COLOR0_BASE+0xD00)
#define DISP_COLOR_S_GAIN_BY_Y0_4       (DISPSYS_COLOR0_BASE+0xD04)
#define DISP_COLOR_S_GAIN_BY_Y64_0      (DISPSYS_COLOR0_BASE+0xD08)
#define DISP_COLOR_S_GAIN_BY_Y64_1      (DISPSYS_COLOR0_BASE+0xD0C)
#define DISP_COLOR_S_GAIN_BY_Y64_2      (DISPSYS_COLOR0_BASE+0xD10)
#define DISP_COLOR_S_GAIN_BY_Y64_3      (DISPSYS_COLOR0_BASE+0xD14)
#define DISP_COLOR_S_GAIN_BY_Y64_4      (DISPSYS_COLOR0_BASE+0xD18)
#define DISP_COLOR_S_GAIN_BY_Y128_0     (DISPSYS_COLOR0_BASE+0xD1C)
#define DISP_COLOR_S_GAIN_BY_Y128_1     (DISPSYS_COLOR0_BASE+0xD20)
#define DISP_COLOR_S_GAIN_BY_Y128_2     (DISPSYS_COLOR0_BASE+0xD24)
#define DISP_COLOR_S_GAIN_BY_Y128_3     (DISPSYS_COLOR0_BASE+0xD28)
#define DISP_COLOR_S_GAIN_BY_Y128_4     (DISPSYS_COLOR0_BASE+0xD2C)
#define DISP_COLOR_S_GAIN_BY_Y192_0     (DISPSYS_COLOR0_BASE+0xD30)
#define DISP_COLOR_S_GAIN_BY_Y192_1     (DISPSYS_COLOR0_BASE+0xD34)
#define DISP_COLOR_S_GAIN_BY_Y192_2     (DISPSYS_COLOR0_BASE+0xD38)
#define DISP_COLOR_S_GAIN_BY_Y192_3     (DISPSYS_COLOR0_BASE+0xD3C)
#define DISP_COLOR_S_GAIN_BY_Y192_4     (DISPSYS_COLOR0_BASE+0xD40)
#define DISP_COLOR_S_GAIN_BY_Y256_0     (DISPSYS_COLOR0_BASE+0xD44)
#define DISP_COLOR_S_GAIN_BY_Y256_1     (DISPSYS_COLOR0_BASE+0xD48)
#define DISP_COLOR_S_GAIN_BY_Y256_2     (DISPSYS_COLOR0_BASE+0xD4C)
#define DISP_COLOR_S_GAIN_BY_Y256_3     (DISPSYS_COLOR0_BASE+0xD50)
#define DISP_COLOR_S_GAIN_BY_Y256_4     (DISPSYS_COLOR0_BASE+0xD54)
#define DISP_COLOR_LSP_1                (DISPSYS_COLOR0_BASE+0xD58)
#define DISP_COLOR_LSP_2                (DISPSYS_COLOR0_BASE+0xD5C)

/* field definition */
/* ------------------------------------------------------------- */
/* Config */
#define DISP_REG_CONFIG_MMSYS_INTEN				(DISPSYS_CONFIG_BASE + 0x0)
#define DISP_REG_CONFIG_MMSYS_INTSTA				(DISPSYS_CONFIG_BASE + 0x4)
#define DISP_REG_CONFIG_ISP_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0x8)
#define DISP_REG_CONFIG_MJC_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0xc)
#define DISP_REG_CONFIG_CAM_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0x010)
#define DISP_REG_CONFIG_IPU_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0x014)
#define DISP_REG_CONFIG_VDEC_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0x018)
#define DISP_REG_CONFIG_VENC_APB_TX_CON				(DISPSYS_CONFIG_BASE + 0x01C)
/* #define DISP_REG_CONFIG_PWM_APB_ERR_ADDR			(DISPSYS_CONFIG_BASE + 0xc) */
#if 0
#define DISP_REG_CONFIG_ISP_MOUT_EN				(DISPSYS_CONFIG_BASE + 0x01c)
#define DISP_REG_CONFIG_MDP_RDMA0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x020)
#define DISP_REG_CONFIG_MDP_PRZ0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x024)
#define DISP_REG_CONFIG_MDP_PRZ1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x028)
#define DISP_REG_CONFIG_MDP_PRZ2_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x02c)
/* #define DISP_REG_CONFIG_MDP_TDSHP0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x02C) */
#define DISP_REG_CONFIG_MDP_COLOR_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x030)
#define DISP_REG_CONFIG_DISP_OVL0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x034)
#define DISP_REG_CONFIG_DISP_OVL1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x038)
#define DISP_REG_CONFIG_DISP_DITHER_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x03c)
#define DISP_REG_CONFIG_DISP_UFOE_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x040)
#define DISP_REG_CONFIG_DISP_DSC_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x044)
#endif
#define DISP_REG_CONFIG_MMSYS_MOUT_RST				(DISPSYS_CONFIG_BASE + 0x048)
#if 0
#define DISP_REG_CONFIG_MDP_PRZ0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x04c)
#define DISP_REG_CONFIG_MDP_PRZ1_SEL_IN				(DISPSYS_CONFIG_BASE + 0x050)
#define DISP_REG_CONFIG_MDP_PRZ2_SEL_IN				(DISPSYS_CONFIG_BASE + 0x054)
#define DISP_REG_CONFIG_MDP_TDSHP0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x058)
#define DISP_REG_CONFIG_MDP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x05c)
#define DISP_REG_CONFIG_MDP_WROT0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x060)
#define DISP_REG_CONFIG_MDP_WROT1_SEL_IN			(DISPSYS_CONFIG_BASE + 0x064)
#define DISP_REG_CONFIG_DISP_COLOR0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x068)
#define DISP_REG_CONFIG_DISP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x06c)
#define DISP_REG_CONFIG_DISP_WDMA1_SEL_IN			(DISPSYS_CONFIG_BASE + 0x070)
#define DISP_REG_CONFIG_DISP_UFOE_SEL_IN			(DISPSYS_CONFIG_BASE + 0x074)
#define DISP_REG_CONFIG_DISP_DSC_SEL_IN				(DISPSYS_CONFIG_BASE + 0x078)
#define DISP_REG_CONFIG_DSI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x07c)
#define DISP_REG_CONFIG_DSI1_SEL_IN				(DISPSYS_CONFIG_BASE + 0x080)
#define DISP_REG_CONFIG_DPI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x084)
#define DISP_REG_CONFIG_DISP_PATH0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x088)
#define DISP_REG_CONFIG_DISP_PATH0_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x08c)
#define DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x090)
#define DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x094)
#define DISP_REG_CONFIG_DISP_OVL0_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x098)
#define DISP_REG_CONFIG_DISP_OVL0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x09c)
#define DISP_REG_CONFIG_DISP_OVL1_SOUT_SEL_IN			(DISPSYS_CONFIG_BASE + 0x0a0)
#endif
#define DISP_REG_CONFIG_MMSYS_MISC				(DISPSYS_CONFIG_BASE + 0x0F0)
#define DISP_REG_CONFIG_MMSYS_SMI_LARB_SEL			(DISPSYS_CONFIG_BASE + 0x0F4)
#define DISP_REG_CONFIG_MMSYS_SODI_REQ_MASK			(DISPSYS_CONFIG_BASE + 0x0F8)
#define DISP_REG_CONFIG_MMSYS_CG_CON0				(DISPSYS_CONFIG_BASE + 0x100)
#define DISP_REG_CONFIG_MMSYS_CG_SET0				(DISPSYS_CONFIG_BASE + 0x104)
#define DISP_REG_CONFIG_MMSYS_CG_CLR0				(DISPSYS_CONFIG_BASE + 0x108)
#define DISP_REG_CONFIG_MMSYS_CG_CON1				(DISPSYS_CONFIG_BASE + 0x110)
#define DISP_REG_CONFIG_MMSYS_CG_SET1				(DISPSYS_CONFIG_BASE + 0x114)
#define DISP_REG_CONFIG_MMSYS_CG_CLR1				(DISPSYS_CONFIG_BASE + 0x118)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0			(DISPSYS_CONFIG_BASE + 0x120)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET0			(DISPSYS_CONFIG_BASE + 0x124)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR0			(DISPSYS_CONFIG_BASE + 0x128)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS1			(DISPSYS_CONFIG_BASE + 0x130)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET1			(DISPSYS_CONFIG_BASE + 0x134)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR1			(DISPSYS_CONFIG_BASE + 0x138)
#define DISP_REG_CONFIG_MMSYS_CG_CON2				(DISPSYS_CONFIG_BASE + 0x140)
#define DISP_REG_CONFIG_MMSYS_CG_SET2                           (DISPSYS_CONFIG_BASE + 0x144)
#define DISP_REG_CONFIG_MMSYS_CG_CLR2                           (DISPSYS_CONFIG_BASE + 0x148)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS2                       (DISPSYS_CONFIG_BASE + 0x150)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET2                   (DISPSYS_CONFIG_BASE + 0x154)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR2                   (DISPSYS_CONFIG_BASE + 0x158)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS3                       (DISPSYS_CONFIG_BASE + 0x160)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET3                   (DISPSYS_CONFIG_BASE + 0x164)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR3                   (DISPSYS_CONFIG_BASE + 0x168)
#define DISP_REG_CONFIG_MMSYS_SW0_RST_B				(DISPSYS_CONFIG_BASE + 0x170)
#define DISP_REG_CONFIG_MMSYS_SW1_RST_B				(DISPSYS_CONFIG_BASE + 0x174)
#define DISP_REG_CONFIG_MMSYS_LCM_RST_B				(DISPSYS_CONFIG_BASE + 0x178)

#define DISP_REG_CONFIG_PROC_TRACK_EMI_BUSY_CON			(DISPSYS_CONFIG_BASE + 0x190)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_EN			(DISPSYS_CONFIG_BASE + 0x200)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RST			(DISPSYS_CONFIG_BASE + 0x204)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON0			(DISPSYS_CONFIG_BASE + 0x208)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON1			(DISPSYS_CONFIG_BASE + 0x20c)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RD_ADDR			(DISPSYS_CONFIG_BASE + 0x210)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_WR_ADDR			(DISPSYS_CONFIG_BASE + 0x214)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_STATE			(DISPSYS_CONFIG_BASE + 0x218)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_EN			(DISPSYS_CONFIG_BASE + 0x220)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_RST			(DISPSYS_CONFIG_BASE + 0x224)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_CON0			(DISPSYS_CONFIG_BASE + 0x228)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_CON1			(DISPSYS_CONFIG_BASE + 0x22c)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_RD_ADDR			(DISPSYS_CONFIG_BASE + 0x230)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_WR_ADDR			(DISPSYS_CONFIG_BASE + 0x234)
#define DISP_REG_CONFIG_DISP_FAKE2_ENG_STATE			(DISPSYS_CONFIG_BASE + 0x238)

#define DISP_REG_CONFIG_MMSYS_MBIST_CON				(DISPSYS_CONFIG_BASE + 0x800)
#define DISP_REG_CONFIG_MMSYS_MBIST_DONE			(DISPSYS_CONFIG_BASE + 0x804)
#define DISP_REG_CONFIG_MMSYS_MBIST_HOLDB			(DISPSYS_CONFIG_BASE + 0x808)
#define DISP_REG_CONFIG_MMSYS_MBIST_MODE			(DISPSYS_CONFIG_BASE + 0x80c)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL0			(DISPSYS_CONFIG_BASE + 0x810)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL1			(DISPSYS_CONFIG_BASE + 0x814)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL2			(DISPSYS_CONFIG_BASE + 0x818)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL3			(DISPSYS_CONFIG_BASE + 0x81c)
#define DISP_REG_CONFIG_MMSYS_MMSYS_MBIST_DEBUG			(DISPSYS_CONFIG_BASE + 0x824)
#define DISP_REG_CONFIG_MMSYS_MBIST_DIAG_SCANOUT		(DISPSYS_CONFIG_BASE + 0x828)
#define DISP_REG_CONFIG_MMSYS_MBIST_PRE_FUSE			(DISPSYS_CONFIG_BASE + 0x82c)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL0			(DISPSYS_CONFIG_BASE + 0x830)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL1			(DISPSYS_CONFIG_BASE + 0x834)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL2			(DISPSYS_CONFIG_BASE + 0x838)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL3			(DISPSYS_CONFIG_BASE + 0x83c)

#define DISP_REG_CONFIG_MMSYS_MBIST_HDEN			(DISPSYS_CONFIG_BASE + 0x840)
#define DISP_REG_CONFIG_MDP_RDMA0_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x844)
#define DISP_REG_CONFIG_MDP_RDMA1_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x848)

#define DISP_REG_CONFIG_MDP_RSZ_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x84c)
#define DISP_REG_CONFIG_MDP_TDSHP_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x850)
#define DISP_REG_CONFIG_MDP_WDMA_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x854)

#define DISP_REG_CONFIG_MDP_WROT0_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x858)
#define DISP_REG_CONFIG_MDP_WROT1_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x85c)

#define DISP_REG_CONFIG_DISP_OVL_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x860)
#define DISP_REG_CONFIG_DISP_OVL_2L_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x864)
#define DISP_REG_CONFIG_DISP_RDMA_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x868)
#define DISP_REG_CONFIG_DISP_WDMA0_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x86c)
#define DISP_REG_CONFIG_DISP_WDMA1_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x870)
#define DISP_REG_CONFIG_DISP_GAMMA_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x874)
#define DISP_REG_CONFIG_DSI_MEM_DELSEL				(DISPSYS_CONFIG_BASE + 0x878)
#define DISP_REG_CONFIG_DISP_UFOE_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x87c)
#define DISP_REG_CONFIG_DISP_DSC_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x880)
#define DISP_REG_CONFIG_DISP_OD_MEM_DELSEL0			(DISPSYS_CONFIG_BASE + 0x884)
#define DISP_REG_CONFIG_DISP_OD_MEM_DELSEL1			(DISPSYS_CONFIG_BASE + 0x888)
#define DISP_REG_CONFIG_DISP_OD_MEM_DELSEL2			(DISPSYS_CONFIG_BASE + 0x88c)
#define DISP_REG_CONFIG_DISP_RSZ_MEM_DELSEL			(DISPSYS_CONFIG_BASE + 0x890)
#define DISP_REG_CONFIG_MMSYS_MBIST_RP_RST_B			(DISPSYS_CONFIG_BASE + 0x898)


/* #define DISP_REG_CONFIG_MMSYS_MEM_DELSEL0			(DISPSYS_CONFIG_BASE + 0x830) */
/* #define DISP_REG_CONFIG_MMSYS_MEM_DELSEL1			(DISPSYS_CONFIG_BASE + 0x834) */
/* #define DISP_REG_CONFIG_MMSYS_MEM_DELSEL2			(DISPSYS_CONFIG_BASE + 0x838) */
/* #define DISP_REG_CONFIG_MMSYS_MEM_DELSEL3			(DISPSYS_CONFIG_BASE + 0x83c) */
/* #define DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL			(DISPSYS_CONFIG_BASE + 0x880) */

#define DISP_REG_CONFIG_MMSYS_DUMMY0				(DISPSYS_CONFIG_BASE + 0x8a4)
#define DISP_REG_CONFIG_MMSYS_DUMMY1				(DISPSYS_CONFIG_BASE + 0x8a8)
#define DISP_REG_CONFIG_MMSYS_DUMMY2				(DISPSYS_CONFIG_BASE + 0x8ac)
#define DISP_REG_CONFIG_MMSYS_DUMMY3				(DISPSYS_CONFIG_BASE + 0x8b0)

#define DISP_REG_CONFIG_DISP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8b4)
#define DISP_REG_CONFIG_DISP_DL_VALID_1				(DISPSYS_CONFIG_BASE + 0x8b8)
#define DISP_REG_CONFIG_DISP_DL_VALID_2				(DISPSYS_CONFIG_BASE + 0x8bc)
#define DISP_REG_CONFIG_DISP_DL_VALID_3				(DISPSYS_CONFIG_BASE + 0x8c0)
#define DISP_REG_CONFIG_DISP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8c4)
#define DISP_REG_CONFIG_DISP_DL_READY_1				(DISPSYS_CONFIG_BASE + 0x8c8)
#define DISP_REG_CONFIG_DISP_DL_READY_2				(DISPSYS_CONFIG_BASE + 0x8cc)
#define DISP_REG_CONFIG_DISP_DL_READY_3				(DISPSYS_CONFIG_BASE + 0x8d0)

#define DISP_REG_CONFIG_MDP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8d4)
#define DISP_REG_CONFIG_MDP_DL_VALID_1				(DISPSYS_CONFIG_BASE + 0x8d8)
#define DISP_REG_CONFIG_MDP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8dc)
#define DISP_REG_CONFIG_MDP_DL_READY_1				(DISPSYS_CONFIG_BASE + 0x8e0)
#define DISP_REG_CONFIG_SMI_LARB0_GREQ				(DISPSYS_CONFIG_BASE + 0x8e4)

#define DISP_REG_CONFIG_DISP_MOUT_MASK0				(DISPSYS_CONFIG_BASE + 0x8e8)
#define DISP_REG_CONFIG_DISP_MOUT_MASK1				(DISPSYS_CONFIG_BASE + 0x8ec)
#define DISP_REG_CONFIG_MDP_MOUT_MASK				(DISPSYS_CONFIG_BASE + 0x8f0)

#define DISP_REG_CONFIG_MMSYS_POWER_READ			(DISPSYS_CONFIG_BASE + 0x8f4)
#define DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL			(DISPSYS_CONFIG_BASE + 0x93c)

#define DISP_REG_CONFIG_DISP_OVL0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0xf00)
#define DISP_REG_CONFIG_DISP_OVL0_PQ_MOUT_EN		(DISPSYS_CONFIG_BASE + 0xf04)
#define DISP_REG_CONFIG_DISP_OVL1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0xf08)
#define DISP_REG_CONFIG_DISP_OVL1_PQ_MOUT_EN		(DISPSYS_CONFIG_BASE + 0xf0c)
#define DISP_REG_CONFIG_DISP_DITHER0_MOUT_EN		(DISPSYS_CONFIG_BASE + 0xf10)
#define DISP_REG_CONFIG_DISP_DITHER1_MOUT_EN		(DISPSYS_CONFIG_BASE + 0xf14)
#define DISP_REG_CONFIG_DISP_UFOE_MOUT_EN			(DISPSYS_CONFIG_BASE + 0xf18)
#define DISP_REG_CONFIG_DISP_DSC_MOUT_EN			(DISPSYS_CONFIG_BASE + 0xf1c)
#define DISP_REG_CONFIG_DISP_DSC_2ND_MOUT_EN		(DISPSYS_CONFIG_BASE + 0xf20)

#define DISP_REG_CONFIG_DISP_COLOR0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf24)
#define DISP_REG_CONFIG_DISP_COLOR1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf28)
#define DISP_REG_CONFIG_DISP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf2c)
#define DISP_REG_CONFIG_DISP_WDMA1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf30)
#define DISP_REG_CONFIG_DISP_UFOE_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf34)
#define DISP_REG_CONFIG_DISP_DSC_SEL_IN				(DISPSYS_CONFIG_BASE + 0xf38)
#define DISP_REG_CONFIG_DISP_DSI0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf3c)
#define DISP_REG_CONFIG_DISP_DSI1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf40)
#define DISP_REG_CONFIG_DISP_DPI0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf44)
#define DISP_REG_CONFIG_DISP_PATH0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf48)
#define DISP_REG_CONFIG_DISP_PATH1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf4c)
#define DISP_REG_CONFIG_DISP_OVL0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf50)
#define DISP_REG_CONFIG_DISP_OVL1_2L_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf54)
#define DISP_REG_CONFIG_DISP_OVL0_2L_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf58)
#define DISP_REG_CONFIG_DISP_RSZ0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf5c)
#define DISP_REG_CONFIG_DISP_RSZ1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf60)
#define DISP_REG_CONFIG_DISP_AAL0_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf64)
#define DISP_REG_CONFIG_DISP_AAL1_SEL_IN			(DISPSYS_CONFIG_BASE + 0xf68)

#define DISP_REG_CONFIG_DISP_PATH0_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf6c)
#define DISP_REG_CONFIG_DISP_PATH1_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf70)
#define DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf74)
#define DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf78)
#define DISP_REG_CONFIG_DISP_RDMA2_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf7c)
#define DISP_REG_CONFIG_DISP_OVL0_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf80)
#define DISP_REG_CONFIG_DISP_RSZ0_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf84)
#define DISP_REG_CONFIG_DISP_RSZ1_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf88)
#define DISP_REG_CONFIG_DISP_CCORR0_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf8c)
#define DISP_REG_CONFIG_DISP_CCORR1_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0xf90)
#define DISP_REG_CONFIG_DISP_OVL1_INT_SOUT_SEL_IN	(DISPSYS_CONFIG_BASE + 0xf94)
#define DISP_REG_CONFIG_DISP_OVL0_2L_INT_SOUT_SEL_IN	(DISPSYS_CONFIG_BASE + 0xf98)
#define DISP_REG_CONFIG_DISP_OVL1_2L_INT_SOUT_SEL_IN	(DISPSYS_CONFIG_BASE + 0xf9c)


/* #define DISP_REG_CONFIG_C08					(DISPSYS_CONFIG_BASE + 0xc08) */
/* #define DISP_REG_CLK_CFG_0_MM_CLK				(DISPSYS_CONFIG_BASE + 0x40) */
/* #define DISP_REG_CLK_CFG_0_CLR				(DISPSYS_CONFIG_BASE + 0x48) */
/* #define DISP_REG_CLK_CFG_1_CLR				(DISPSYS_CONFIG_BASE + 0x58) */
/* #define DISP_REG_CLK_CFG_6_DPI				(DISPSYS_CONFIG_BASE + 0xA0) */
/* #define DISP_REG_CLK_CFG_6_CLR				(DISPSYS_CONFIG_BASE + 0xA8) */
/* #define DISP_REG_VENCPLL_CON0				0xf0209260  // bit0 1 */

#define MMSYS_INTEN_FLD_MMSYS_INTEN				REG_FLD(8, 0)
#define MMSYS_INSTA_FLD_MMSYS_INSTA				REG_FLD(1, 0)


#define MMSYS_MOUT_RST_FLD_MMSYS_MOUT_RST				REG_FLD(16, 0)

#define MMSYS_MISC_FLD_SMI_LARB0_TEST_MODE					    REG_FLD(1, 0)

#define MMSYS_SMI_LARB_SEL_FLD_DISP_OVL0_2L_LARB_SEL		REG_FLD(1, 0)
#define MMSYS_SMI_LARB_SEL_FLD_DISP_WDMA0_LARB_SEL		REG_FLD(1, 1)
#define MMSYS_SODI_REQ_MASK_FLD_MMSYS_SODI_REQ_MASK		REG_FLD(5, 0)
#define MMSYS_SODI_REQ_MASK_FLD_DVFS_HALT_MASK			REG_FLD(5, 16)
#define MMSYS_CG_CON0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_SET0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CON1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_SET1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_CON2_FLD_CG2                                   REG_FLD(32, 0)
#define MMSYS_CG_SET2_FLD_CG2                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR2_FLD_CG2                                   REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS0_FLD_DCM_DIS0                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS1_FLD_DCM_DIS1                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET1_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR1_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS2_FLD_DCM_DIS2                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET2_FLD_DCM_DIS2                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR2_FLD_DCM_DIS2                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS3_FLD_DCM_DIS3                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET3_FLD_DCM_DIS3                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR3_FLD_DCM_DIS3                      REG_FLD(32, 0)
#define MMSYS_SW0_RST_B_FLD_SW0_RST_B                           REG_FLD(32, 0)
#define MMSYS_SW1_RST_B_FLD_SW1_RST_B                           REG_FLD(32, 0)
#define MMSYS_LCM_RST_B_FLD_LCM_RST_B                           REG_FLD(1, 0)
#define MMSYS_LCM_RST_B_FLD_LCM1_RST_B                          REG_FLD(1, 1)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_BACKGROUND			REG_FLD(3, 16)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_RSTB			REG_FLD(1, 15)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_SCANOUT_SEL			REG_FLD(4, 8)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_DEBUG				REG_FLD(1, 7)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_FAILOUT_SEL			REG_FLD(6, 0)
#define MMSYS_MBIST_DONE_FLD_MMSYS_MBIST_DONE				REG_FLD(32, 0)

#define MMSYS_MBIST_HOLDB_FLD_MMSYS_MBIST_HOLDB			REG_FLD(32, 0)
#define MMSYS_MBIST_MODE_FLD_MMSYS_MBIST_MODE				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL0_FLD_MMSYS_MBIST_FAIL0				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL1_FLD_MMSYS_MBIST_FAIL1				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL2_FLD_MMSYS_MBIST_FAIL2				REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL2_FLD_MMSYS_MBIST_FAIL3				REG_FLD(32, 0)
#define MMSYS_MBIST_DEBUG_FLD_MMSYS_MBIST_DEBUG				REG_FLD(8, 0)
#define MMSYS_MBIST_DIAG_SCANOUT_FLD_MMSYS_MBIST_DIAG_SCANOUT		REG_FLD(6, 0)

#define MMSYS_MBIST_BSEL0_FLD_MDP_TDSHP_MBIST_BSEL			REG_FLD(16, 0)
#define MMSYS_MBIST_BSEL0_FLD_MDP_RDMA0_MBIST_BSEL		REG_FLD(5, 16)
#define MMSYS_MBIST_BSEL1_FLD_MDP_RDMA1_MBIST_BSEL		REG_FLD(5, 0)
#define MMSYS_MBIST_BSEL1_FLD_DISP_RSZ0_MBIST_BSEL		REG_FLD(7, 8)
#define MMSYS_MBIST_BSEL1_FLD_MDP_WROT0_MBIST_BSEL		REG_FLD(8, 16)
#define MMSYS_MBIST_BSEL1_FLD_MDP_WROT1_MBIST_BSEL		REG_FLD(8, 24)

#define MMSYS_MBIST_BSEL3_FLD_MDP_RSZ2_MBIST_BSEL		REG_FLD(12, 0)
#define MMSYS_MBIST_BSEL3_FLD_DISP_UFOE_MBIST_BSEL		REG_FLD(6, 16)
#define MMSYS_MBIST_BSEL3_FLD_DISP_RSZ1_MBIST_BSEL		REG_FLD(7, 24)

#define MMSYS_MEM_DELSEL0_FLD_MDP_RSZ_MEM_DELSEL                REG_FLD(16, 0)
#define MMSYS_MEM_DELSEL0_FLD_MDP_RDMA_MEM_DELSEL               REG_FLD(14, 16)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WROT_MEM_DELSEL				REG_FLD(22, 0)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WDMA_MEM_DELSEL               REG_FLD(6, 22)
#define MMSYS_MEM_DELSEL1_FLD_MDP_TDSHP_MEM_DELSEL              REG_FLD(4, 28)
#define MMSYS_MBIST_HDEN_FLD_MMSYS_MBIST_HDEN			REG_FLD(1, 0)
#define DISP_OVL_MEM_DELSEL_FLD_DISP_OVL_MEM_DELSEL		REG_FLD(6, 0)
#define DISP_OVL_2L_MEM_DELSEL_FLD_DISP_OVL_2L_MEM_DELSEL	REG_FLD(6, 0)
#define DISP_RDMA_MEM_DELSEL_FLD_DISP_RDMA_MEM_DELSEL		REG_FLD(6, 0)
#define DISP_WDMA0_MEM_DELSEL_FLD_DISP_WDMA0_MEM_DELSEL		REG_FLD(12, 0)
#define DISP_WDMA1_MEM_DELSEL_FLD_DISP_WDMA1_MEM_DELSEL		REG_FLD(12, 0)
#define DISP_GAMMA_MEM_DELSEL_FLD_DISP_GAMMA_MEM_DELSEL		REG_FLD(6, 0)
#define DSI_MEM_DELSEL_FLD_DSI_MEM_DELSEL			REG_FLD(10, 0)
#define DISP_UFOE_MEM_DELSEL_FLD_DISP_UFOE_MEM_DELSEL		REG_FLD(18, 0)
#define DISP_DSC_MEM_DELSEL_FLD_DISP_DSC_MEM_DELSEL		REG_FLD(24, 0)
#define DISP_OD_MEM_DELSEL0_FLD_DISP_OD_MEM_DELSEL0		REG_FLD(30, 0)
#define DISP_OD_MEM_DELSEL1_FLD_DISP_OD_MEM_DELSEL1		REG_FLD(30, 0)
#define DISP_OD_MEM_DELSEL2_FLD_DISP_OD_MEM_DELSEL2		REG_FLD(30, 0)
#define DISP_RSZ_MEM_DELSEL_FLD_DISP_RSZ_MEM_DELSEL		REG_FLD(6, 0)
#define MMSYS_MBIST_RP_RST_B_FLD_MMSYS_MBIST_RP_RST_B		REG_FLD(1, 0)
#define MMSYS_DUMMY0_FLD_MMSYS_DUMMY0                           REG_FLD(32, 0)
#define MMSYS_DUMMY1_FLD_MMSYS_DUMMY1                           REG_FLD(32, 0)
#define MMSYS_DUMMY2_FLD_MMSYS_DUMMY2                           REG_FLD(32, 0)
#define MMSYS_DUMMY3_FLD_MMSYS_DUMMY3                           REG_FLD(32, 0)
#define DISP_DL_VALID_0_FLD_DISP_DL_VALID_0                     REG_FLD(32, 0)
#define DISP_DL_VALID_1_FLD_DISP_DL_VALID_1                     REG_FLD(32, 0)
#define DISP_DL_VALID_2_FLD_DISP_DL_VALID_2                     REG_FLD(32, 0)
#define DISP_DL_VALID_3_FLD_DISP_DL_VALID_3                     REG_FLD(32, 0)
#define DISP_DL_READY_0_FLD_DISP_DL_READY_0                     REG_FLD(32, 0)
#define DISP_DL_READY_1_FLD_DISP_DL_READY_1                     REG_FLD(32, 0)
#define DISP_DL_READY_2_FLD_DISP_DL_READY_2                     REG_FLD(32, 0)
#define DISP_DL_READY_3_FLD_DISP_DL_READY_3                     REG_FLD(32, 0)
#define SMI_LARB0_GREQ_FLD_SMI_LARB0_GREQ                       REG_FLD(8, 0)
#define SMI_LARB0_GREQ_FLD_SMI_LARB1_GREQ                       REG_FLD(10, 16)

#define DISP_MOUT_MASK0_FLD_DISP_MOUT_MASK0			REG_FLD(32, 0)
#define DISP_MOUT_MASK1_FLD_DISP_MOUT_MASK1			REG_FLD(32, 0)

#define MMSYS_POWER_READ_FLD_DISP_RDMA0_SMI_REQ			REG_FLD(1, 0)
#define MMSYS_POWER_READ_FLD_DISP_RDMA1_SMI_REQ			REG_FLD(1, 1)
#define MMSYS_POWER_READ_FLD_DISP_RDMA2_SMI_REQ			REG_FLD(1, 2)
#define MMSYS_POWER_READ_FLD_DISP_OD_R_SMI_REQ			REG_FLD(2, 4)
#define MMSYS_POWER_READ_FLD_DISP_RDMA0_HALT_FOR_DVFS		REG_FLD(1, 8)
#define MMSYS_POWER_READ_FLD_DISP_RDMA1_HALT_FOR_DVFS		REG_FLD(1, 9)
#define MMSYS_POWER_READ_FLD_DISP_RDMA2_HALT_FOR_DVFS		REG_FLD(1, 10)
#define MMSYS_POWER_READ_FLD_DISP_RDMA0_WROT_SHARE_EN		REG_FLD(1, 12)
#define MMSYS_POWER_READ_FLD_DISP_RDMA1_WROT_SHARE_EN		REG_FLD(1, 13)

#define MMSYS_DEBUG_OUT_SEL_FLD_MMSYS_DEBUG_OUT_SEL             REG_FLD(5, 0)
#define MMSYS_DEBUG_OUT_SEL_FLD_SUBSYS_DEBUG_OUT_SEL		REG_FLD(3, 8)

#define DISP_OVL0_MOUT_EN_FLD_DISP_OVL0_MOUT_EN			REG_FLD(8, 0)
#define DISP_OVL0_PQ_MOUT_EN_FLD_DISP_OVL0_PQ_MOUT_EN		REG_FLD(8, 0)
#define DISP_OVL1_MOUT_EN_FLD_DISP_OVL1_MOUT_EN			REG_FLD(8, 0)
#define DISP_OVL1_PQ_MOUT_EN_FLD_DISP_OVL1_PQ_MOUT_EN		REG_FLD(8, 0)
#define DISP_DITHER0_MOUT_EN_FLD_DISP_DITHER0_MOUT_EN		REG_FLD(8, 0)
#define DISP_DITHER1_MOUT_EN_FLD_DISP_DITHER1_MOUT_EN		REG_FLD(8, 0)
#define DISP_UFOE_MOUT_EN_FLD_DISP_UFOE_MOUT_EN			REG_FLD(8, 0)
#define DISP_DSC_MOUT_EN_FLD_DISP_DSC_MOUT_EN			REG_FLD(8, 0)
#define DISP_DSC_2ND_MOUT_EN_FLD_DISP_DSC_2ND_MOUT_EN		REG_FLD(8, 0)
#define DISP_COLOR0_SEL_IN_FLD_DISP_COLOR0_SEL_IN		REG_FLD(4, 0)
#define DISP_COLOR1_SEL_IN_FLD_DISP_COLOR1_SEL_IN		REG_FLD(4, 0)
#define DISP_WDMA0_SEL_IN_FLD_DISP_WDMA0_SEL_IN			REG_FLD(4, 0)
#define DISP_WDMA1_SEL_IN_FLD_DISP_WDMA1_SEL_IN			REG_FLD(4, 0)
#define DISP_UFOE_SEL_IN_FLD_DISP_UFOE_SEL_IN			REG_FLD(4, 0)
#define DISP_DSC_SEL_IN_FLD_DISP_DSC_SEL_IN			REG_FLD(4, 0)
#define DSI0_SEL_IN_FLD_DSI0_SEL_IN				REG_FLD(4, 0)
#define DSI1_SEL_IN_FLD_DSI1_SEL_IN				REG_FLD(4, 0)
#define DPI0_SEL_IN_FLD_DPI0_SEL_IN				REG_FLD(4, 0)
#define DISP_PATH0_SEL_IN_FLD_DISP_PATH0_SEL_IN			REG_FLD(4, 0)
#define DISP_PATH1_SEL_IN_FLD_DISP_PATH1_SEL_IN			REG_FLD(4, 0)
#define DISP_OVL0_SEL_IN_FLD_DISP_OVL0_SEL_IN			REG_FLD(4, 0)
#define DISP_OVL1_2L_SEL_IN_FLD_DISP_OVL1_2L_SEL_IN		REG_FLD(4, 0)
#define DISP_OVL0_2L_SEL_IN_FLD_DISP_OVL0_2L_SEL_IN		REG_FLD(4, 0)
#define DISP_RSZ0_SEL_IN_FLD_DISP_RSZ0_SEL_IN			REG_FLD(4, 0)
#define DISP_RSZ1_SEL_IN_FLD_DISP_RSZ1_SEL_IN			REG_FLD(4, 0)
#define DISP_AAL0_SEL_IN_FLD_DISP_AAL0_SEL_IN			REG_FLD(4, 0)
#define DISP_AAL1_SEL_IN_FLD_DISP_AAL1_SEL_IN			REG_FLD(4, 0)
#define DISP_PATH0_SOUT_SEL_IN_FLD_DISP_PATH0_SOUT_SEL_IN	REG_FLD(4, 0)
#define DISP_PATH1_SOUT_SEL_IN_FLD_DISP_PATH1_SOUT_SEL_IN	REG_FLD(4, 0)
#define DISP_RDMA0_SOUT_SEL_IN_FLD_DISP_RDMA0_SOUT_SEL_IN	REG_FLD(4, 0)
#define DISP_RDMA1_SOUT_SEL_IN_FLD_DISP_RDMA1_SOUT_SEL_IN	REG_FLD(4, 0)
#define DISP_RDMA2_SOUT_SEL_IN_FLD_DISP_RDMA2_SOUT_SEL_IN	REG_FLD(4, 0)

#define DISP_OVL0_SOUT_SEL_IN_FLD_DISP_OVL0_SOUT_SEL_IN		REG_FLD(4, 0)
#define DISP_RSZ0_SOUT_SEL_IN_FLD_DISP_RSZ0_SOUT_SEL_IN		REG_FLD(4, 0)
#define DISP_RSZ1_SOUT_SEL_IN_FLD_DISP_RSZ1_SOUT_SEL_IN		REG_FLD(4, 0)
#define DISP_CCORR0_SOUT_SEL_IN_FLD_DISP_CCORR0_SOUT_SEL_IN	REG_FLD(4, 0)
#define DISP_CCORR1_SOUT_SEL_IN_FLD_DISP_CCORR1_SOUT_SEL_IN	REG_FLD(4, 0)
#define	OVL1_INT_SOUT_SEL_IN_FLD_OVL1_INT_SOUT_SEL_IN		REG_FLD(4, 0)
#define	OVL0_2L_INT_SOUT_SEL_IN_FLD_OVL0_2L_INT_SOUT_SEL_IN	REG_FLD(4, 0)
#define	OVL1_2L_INT_SOUT_SEL_IN_FLD_OVL1_2L_INT_SOUT_SEL_IN	REG_FLD(4, 0)



#define MDP_DL_VALID_0_FLD_MDP_DL_VALID_0                       REG_FLD(32, 0)
#define MDP_DL_READY_0_FLD_MDP_DL_READY_0                       REG_FLD(32, 0)

/* SMI_LARB0 */
#define DISP_REG_SMI_LARB0_NON_SEC_CON (DISPSYS_SMI_LARB0_BASE+0x380)
#define DISP_REG_SMI_LARB0_SEC_CON (DISPSYS_SMI_LARB0_BASE+0xf80)
/* SMI_LARB1 */
#define DISP_REG_SMI_LARB1_NON_SEC_CON (DISPSYS_SMI_LARB1_BASE+0x380)
#define DISP_REG_SMI_LARB1_SEC_CON (DISPSYS_SMI_LARB1_BASE+0xf80)

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
/* #define DISP_REG_DPI_ESAV_VTIM_R					(DISPSYS_DPI_BASE + 0x084) */
#define DISP_REG_DPI_ESAV_VTIM_R					(DISPSYS_DPI_BASE + 0x088)
#define DISP_REG_DPI_CLPF_SETTING					(DISPSYS_DPI_BASE + 0x08c)
#define DISP_REG_DPI_Y_LIMIT						(DISPSYS_DPI_BASE + 0x090)
#define DISP_REG_DPI_C_LIMIT						(DISPSYS_DPI_BASE + 0x094)
#define DISP_REG_DPI_YUV422_SETTING					(DISPSYS_DPI_BASE + 0x098)
#define DISP_REG_DPI_EMBSYNC_SETTING				(DISPSYS_DPI_BASE + 0x09c)
/* #define DISP_REG_DPI_ESAV_CODE_SET0					(DISPSYS_DPI_BASE + 0x0a0) */
/* #define DISP_REG_DPI_ESAV_CODE_SET1					(DISPSYS_DPI_BASE + 0x0a4) */
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
#define DISP_REG_CCORR_EN                                    (DISPSYS_CCORR0_BASE + 0x000)
#define DISP_REG_CCORR_RESET                                 (DISPSYS_CCORR0_BASE + 0x004)
#define DISP_REG_CCORR_INTEN                                 (DISPSYS_CCORR0_BASE + 0x008)
#define DISP_REG_CCORR_INTSTA                                (DISPSYS_CCORR0_BASE + 0x00c)
#define DISP_REG_CCORR_STATUS                                (DISPSYS_CCORR0_BASE + 0x010)
#define DISP_REG_CCORR_CFG                                   (DISPSYS_CCORR0_BASE + 0x020)
#define DISP_REG_CCORR_IN_CNT                                (DISPSYS_CCORR0_BASE + 0x024)
#define DISP_REG_CCORR_OUT_CNT                               (DISPSYS_CCORR0_BASE + 0x028)
#define DISP_REG_CCORR_CHKSUM                                (DISPSYS_CCORR0_BASE + 0x02c)
#define DISP_REG_CCORR_SIZE                                  (DISPSYS_CCORR0_BASE + 0x030)
#define DISP_REG_CCORR_COEF_0                                (DISPSYS_CCORR0_BASE + 0x080)
#define DISP_REG_CCORR_SHADOW                                (DISPSYS_CCORR0_BASE + 0x0a0)
#define DISP_REG_CCORR_DUMMY_REG                             (DISPSYS_CCORR0_BASE + 0x0c0)

#define CCORR_SIZE_FLD_HSIZE                                  REG_FLD(13, 16)
#define CCORR_SIZE_FLD_VSIZE                                  REG_FLD(13, 0)
#define CCORR_CFG_FLD_CHKSUM_SEL                              REG_FLD(3, 29)
#define CCORR_CFG_FLD_CHKSUM_EN                               REG_FLD(1, 28)
#define CCORR_CFG_FLD_CCORR_GAMMA_OFF                         REG_FLD(1, 2)
#define CCORR_CFG_FLD_CCORR_ENGINE_EN                         REG_FLD(1, 1)
/* field definition */
/* ------------------------------------------------------------- */
/* GAMMA */
#define DISP_REG_GAMMA_EN								(DISPSYS_GAMMA0_BASE + 0x000)
#define DISP_REG_GAMMA_RESET							(DISPSYS_GAMMA0_BASE + 0x004)
#define DISP_REG_GAMMA_INTEN							(DISPSYS_GAMMA0_BASE + 0x008)
#define DISP_REG_GAMMA_INTSTA							(DISPSYS_GAMMA0_BASE + 0x00c)
#define DISP_REG_GAMMA_STATUS						    (DISPSYS_GAMMA0_BASE + 0x010)
#define DISP_REG_GAMMA_CFG							    (DISPSYS_GAMMA0_BASE + 0x020)
#define DISP_REG_GAMMA_INPUT_COUNT					    (DISPSYS_GAMMA0_BASE + 0x024)
#define DISP_REG_GAMMA_OUTPUT_COUNT					    (DISPSYS_GAMMA0_BASE + 0x028)
#define DISP_REG_GAMMA_CHKSUM						    (DISPSYS_GAMMA0_BASE + 0x02c)
#define DISP_REG_GAMMA_SIZE							    (DISPSYS_GAMMA0_BASE + 0x030)
#define DISP_REG_GAMMA_DEBUG						    (DISPSYS_GAMMA0_BASE + 0x034)
#define DISP_REG_GAMMA_DUMMY_REG					    (DISPSYS_GAMMA0_BASE + 0x0c0)
#define DISP_REG_GAMMA_LUT							    (DISPSYS_GAMMA0_BASE + 0x700)

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
#define DITHER_16_FLD_INPUT_RSHIFT_G				REG_FLD(3, 0)
#define DITHER_17_FLD_CRC_RDY					REG_FLD(1, 16)
#define DITHER_17_FLD_CRC_OUT					REG_FLD(16, 0)
#define LUT_FLD_GAMMA_LUT_R					REG_FLD(10, 20)
#define LUT_FLD_GAMMA_LUT_G					REG_FLD(10, 10)
#define LUT_FLD_GAMMA_LUT_B					REG_FLD(10, 0)

/* ------------------------------------------------------------- */
/* Dither */
#define DISP_REG_DITHER_EN                                        (DISPSYS_DITHER0_BASE + 0x000)
#define DISP_REG_DITHER_RESET                                     (DISPSYS_DITHER0_BASE + 0x004)
#define DISP_REG_DITHER_INTEN                                     (DISPSYS_DITHER0_BASE + 0x008)
#define DISP_REG_DITHER_INTSTA                                    (DISPSYS_DITHER0_BASE + 0x00c)
#define DISP_REG_DITHER_STATUS                                    (DISPSYS_DITHER0_BASE + 0x010)
#define DISP_REG_DITHER_CFG                                       (DISPSYS_DITHER0_BASE + 0x020)
#define DISP_REG_DITHER_IN_CNT                                    (DISPSYS_DITHER0_BASE + 0x024)
#define DISP_REG_DITHER_OUT_CNT                                   (DISPSYS_DITHER0_BASE + 0x028)
#define DISP_REG_DITHER_CHKSUM                                    (DISPSYS_DITHER0_BASE + 0x02c)
#define DISP_REG_DITHER_SIZE                                      (DISPSYS_DITHER0_BASE + 0x030)
#define DISP_REG_DITHER_DUMMY_REG                                 (DISPSYS_DITHER0_BASE + 0x0c0)
#define DISP_REG_DITHER_0                                         (DISPSYS_DITHER0_BASE + 0x100)
#define DISP_REG_DITHER_5                                         (DISPSYS_DITHER0_BASE + 0x114)
#define DISP_REG_DITHER_6                                         (DISPSYS_DITHER0_BASE + 0x118)
#define DISP_REG_DITHER_7                                         (DISPSYS_DITHER0_BASE + 0x11c)
#define DISP_REG_DITHER_8                                         (DISPSYS_DITHER0_BASE + 0x120)
#define DISP_REG_DITHER_9                                         (DISPSYS_DITHER0_BASE + 0x124)
#define DISP_REG_DITHER_10                                        (DISPSYS_DITHER0_BASE + 0x128)
#define DISP_REG_DITHER_11                                        (DISPSYS_DITHER0_BASE + 0x12c)
#define DISP_REG_DITHER_12                                        (DISPSYS_DITHER0_BASE + 0x130)
#define DISP_REG_DITHER_13                                        (DISPSYS_DITHER0_BASE + 0x134)
#define DISP_REG_DITHER_14                                        (DISPSYS_DITHER0_BASE + 0x138)
#define DISP_REG_DITHER_15                                        (DISPSYS_DITHER0_BASE + 0x13c)
#define DISP_REG_DITHER_16                                        (DISPSYS_DITHER0_BASE + 0x140)
#define DISP_REG_DITHER_17                                        (DISPSYS_DITHER0_BASE + 0x144)

#define DITHER_CFG_FLD_CHKSUM_SEL				REG_FLD(2, 29)
#define DITHER_CFG_FLD_CHKSUM_EN				REG_FLD(1, 28)
#define DITHER_CFG_FLD_DITHER_ENGINE_EN				REG_FLD(1, 1)
#define DITHER_CFG_FLD_RELAY_MODE				REG_FLD(1, 0)
#define DITHER_SIZE_FLD_HSIZE					REG_FLD(13, 16)
#define DITHER_SIZE_FLD_VSIZE					REG_FLD(13, 0)

/* ------------------------------------------------------------- */
/* MUTEX */
#define DISP_OVL_SEPARATE_MUTEX_ID (DISP_MUTEX_DDP_LAST+1)	/* other disp will not see mutex 4 */
#define DISP_REG_CONFIG_MUTEX_INTEN					(DISPSYS_MUTEX_BASE + 0x000)
#define DISP_REG_CONFIG_MUTEX_INTSTA				(DISPSYS_MUTEX_BASE + 0x004)
#define DISP_REG_CONFIG_MUTEX_CFG					(DISPSYS_MUTEX_BASE + 0x008)
	#define CFG_FLD_HW_CG								REG_FLD(1, 0)

#define DISP_REG_CONFIG_MUTEX_REG_UPD_TIMEOUT		(DISPSYS_MUTEX_BASE + 0x00C)
	#define UPD_FLD_UPD_TIMEOUT							REG_FLD(8, 0)

#define DISP_REG_CONFIG_MUTEX_REG_COMMIT0			(DISPSYS_MUTEX_BASE + 0x010)
#define DISP_REG_CONFIG_MUTEX_REG_COMMIT1			(DISPSYS_MUTEX_BASE + 0x014)
#define DISP_REG_CONFIG_MUTEX_INTEN_1				(DISPSYS_MUTEX_BASE + 0x018)
#define DISP_REG_CONFIG_MUTEX_INTSTA_1				(DISPSYS_MUTEX_BASE + 0x01C)

/* mutex0 */
#define DISP_REG_CONFIG_MUTEX0_EN					(DISPSYS_MUTEX_BASE + 0x020)
	#define EN_FLD_MUTEX0_EN							REG_FLD(1, 0)

#define DISP_REG_CONFIG_MUTEX0_GET					(DISPSYS_MUTEX_BASE + 0x024)
	#define GET_FLD_MUTEX0_GET							REG_FLD(1, 0)
	#define GET_FLD_INT_MUTEX0_EN						REG_FLD(1, 1)

#define DISP_REG_CONFIG_MUTEX0_RST					(DISPSYS_MUTEX_BASE + 0x028)
	#define RST_FLD_MUTEX0_RST							REG_FLD(1, 0)

#define DISP_REG_CONFIG_MUTEX0_SOF					(DISPSYS_MUTEX_BASE + 0x02C)
	#define SOF_FLD_MUTEX0_SOF							REG_FLD(3, 0)
	#define SOF_FLD_MUTEX0_SOF_TIMING					REG_FLD(2, 3)
	#define SOF_FLD_MUTEX0_SOF_WAIT						REG_FLD(1, 5)
	#define SOF_FLD_MUTEX0_EOF							REG_FLD(3, 6)
	#define SOF_FLD_MUTEX0_FOF_TIMING					REG_FLD(2, 9)
	#define SOF_FLD_MUTEX0_EOF_WAIT						REG_FLD(1, 11)

#define SOF_VAL_MUTEX0_SOF_SINGLE_MODE					(0)
#define SOF_VAL_MUTEX0_SOF_FROM_DSI0					(1)
#define SOF_VAL_MUTEX0_SOF_FROM_DSI1					(2)
#define SOF_VAL_MUTEX0_SOF_FROM_DPI						(3)
#define SOF_VAL_MUTEX0_SOF_RESERVED						(5)
#define SOF_VAL_MUTEX0_EOF_SINGLE_MODE					(0)
#define SOF_VAL_MUTEX0_EOF_FROM_DSI0					(1)
#define SOF_VAL_MUTEX0_EOF_FROM_DSI1					(2)
#define SOF_VAL_MUTEX0_EOF_FROM_DPI						(3)
#define SOF_VAL_MUTEX0_EOF_RESERVED						(5)

#define DISP_REG_CONFIG_MUTEX0_MOD0					(DISPSYS_MUTEX_BASE + 0x030)
#define DISP_REG_CONFIG_MUTEX0_MOD1					(DISPSYS_MUTEX_BASE + 0x034)
#define DISP_REG_CONFIG_MUTEX1_EN					(DISPSYS_MUTEX_BASE + 0x040)
#define DISP_REG_CONFIG_MUTEX1_GET					(DISPSYS_MUTEX_BASE + 0x044)
#define DISP_REG_CONFIG_MUTEX1_RST					(DISPSYS_MUTEX_BASE + 0x048)
#define DISP_REG_CONFIG_MUTEX1_SOF					(DISPSYS_MUTEX_BASE + 0x04C)
#define DISP_REG_CONFIG_MUTEX1_MOD0					(DISPSYS_MUTEX_BASE + 0x050)
#define DISP_REG_CONFIG_MUTEX1_MOD1					(DISPSYS_MUTEX_BASE + 0x054)
#define DISP_REG_CONFIG_MUTEX2_EN					(DISPSYS_MUTEX_BASE + 0x060)
#define DISP_REG_CONFIG_MUTEX2_GET					(DISPSYS_MUTEX_BASE + 0x064)
#define DISP_REG_CONFIG_MUTEX2_RST					(DISPSYS_MUTEX_BASE + 0x068)
#define DISP_REG_CONFIG_MUTEX2_SOF					(DISPSYS_MUTEX_BASE + 0x06C)
#define DISP_REG_CONFIG_MUTEX2_MOD0					(DISPSYS_MUTEX_BASE + 0x070)
#define DISP_REG_CONFIG_MUTEX2_MOD1					(DISPSYS_MUTEX_BASE + 0x074)
#define DISP_REG_CONFIG_MUTEX3_EN					(DISPSYS_MUTEX_BASE + 0x080)
#define DISP_REG_CONFIG_MUTEX3_GET					(DISPSYS_MUTEX_BASE + 0x084)
#define DISP_REG_CONFIG_MUTEX3_RST					(DISPSYS_MUTEX_BASE + 0x088)
#define DISP_REG_CONFIG_MUTEX3_SOF					(DISPSYS_MUTEX_BASE + 0x08C)
#define DISP_REG_CONFIG_MUTEX3_MOD0					(DISPSYS_MUTEX_BASE + 0x090)
#define DISP_REG_CONFIG_MUTEX3_MOD1					(DISPSYS_MUTEX_BASE + 0x094)
#define DISP_REG_CONFIG_MUTEX4_EN					(DISPSYS_MUTEX_BASE + 0x0A0)
#define DISP_REG_CONFIG_MUTEX4_GET					(DISPSYS_MUTEX_BASE + 0x0A4)
#define DISP_REG_CONFIG_MUTEX4_RST					(DISPSYS_MUTEX_BASE + 0x0A8)
#define DISP_REG_CONFIG_MUTEX4_SOF					(DISPSYS_MUTEX_BASE + 0x0AC)
#define DISP_REG_CONFIG_MUTEX4_MOD0					(DISPSYS_MUTEX_BASE + 0x0B0)
#define DISP_REG_CONFIG_MUTEX4_MOD1					(DISPSYS_MUTEX_BASE + 0x0B4)
#define DISP_REG_CONFIG_MUTEX5_EN					(DISPSYS_MUTEX_BASE + 0x0C0)
#define DISP_REG_CONFIG_MUTEX5_GET					(DISPSYS_MUTEX_BASE + 0x0C4)
#define DISP_REG_CONFIG_MUTEX5_RST					(DISPSYS_MUTEX_BASE + 0x0C8)
#define DISP_REG_CONFIG_MUTEX5_SOF					(DISPSYS_MUTEX_BASE + 0x0CC)
#define DISP_REG_CONFIG_MUTEX5_MOD0					(DISPSYS_MUTEX_BASE + 0x0D0)
#define DISP_REG_CONFIG_MUTEX5_MOD1					(DISPSYS_MUTEX_BASE + 0x0D4)
#define DISP_REG_CONFIG_DEBUG_OUT_SEL				(DISPSYS_MUTEX_BASE + 0x30C)
	#define DEBUG_OUT_SEL_FLD_DEBUG_OUT_SEL					REG_FLD(2, 0)

#define DISP_REG_CONFIG_MUTEX_EN(n)					(DISP_REG_CONFIG_MUTEX0_EN + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_GET(n)					(DISP_REG_CONFIG_MUTEX0_GET + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_RST(n)					(DISP_REG_CONFIG_MUTEX0_RST + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_MOD0(n)				(DISP_REG_CONFIG_MUTEX0_MOD0 + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_MOD1(n)				(DISP_REG_CONFIG_MUTEX0_MOD1 + (0x20 * (n)))
#define DISP_REG_CONFIG_MUTEX_SOF(n)					(DISP_REG_CONFIG_MUTEX0_SOF + (0x20 * (n)))

/* ------------------------------------------------------------- */
/* OD */
#define DISP_REG_OD_EN           (DISPSYS_OD_BASE+0x000)
#define DISP_REG_OD_RESET        (DISPSYS_OD_BASE+0x004)
#define DISP_REG_OD_INTEN        (DISPSYS_OD_BASE+0x008)
#define DISP_REG_OD_INTSTA       (DISPSYS_OD_BASE+0x00C)
#define DISP_REG_OD_STATUS       (DISPSYS_OD_BASE+0x010)
#define DISP_REG_OD_SHADOW_CTRL  (DISPSYS_OD_BASE+0x014)
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
#define DISP_REG_OD_DMA_ADDR_0   (DISPSYS_OD_BASE+0x210)
#define DISP_REG_OD_DMA_ADDR_1   (DISPSYS_OD_BASE+0x214)


/* ------------------------------------------------------------- */


/* ------------------------------------------------------------- */
/* RDMA */
#define DISP_REG_RDMA_INT_ENABLE				(0x000)
#define DISP_REG_RDMA_INT_STATUS				(0x004)
#define DISP_REG_RDMA_GLOBAL_CON				(0x010)
#define DISP_REG_RDMA_SIZE_CON_0                (0x014)
#define DISP_REG_RDMA_SIZE_CON_1                (0x018)
#define DISP_REG_RDMA_TARGET_LINE               (0x01C)
#define DISP_REG_RDMA_MEM_CON                   (0x024)
#define DISP_REG_RDMA_MEM_SRC_PITCH				(0x02C)
#define DISP_REG_RDMA_MEM_GMC_SETTING_0			(0x030)
#define DISP_REG_RDMA_MEM_GMC_SETTING_1			(0x034)
#define DISP_REG_RDMA_MEM_SLOW_CON				(0x038)
#define DISP_REG_RDMA_MEM_GMC_SETTING_2			(0x03c)
#define DISP_REG_RDMA_FIFO_CON                  (0x040)
#define DISP_REG_RDMA_FIFO_LOG                  (0x044)
#define DISP_REG_RDMA_C00                       (0x054)
#define DISP_REG_RDMA_C01                       (0x058)
#define DISP_REG_RDMA_C02                       (0x05C)
#define DISP_REG_RDMA_C10                       (0x060)
#define DISP_REG_RDMA_C11                       (0x064)
#define DISP_REG_RDMA_C12                       (0x068)
#define DISP_REG_RDMA_C20                       (0x06C)
#define DISP_REG_RDMA_C21                       (0x070)
#define DISP_REG_RDMA_C22                       (0x074)
#define DISP_REG_RDMA_PRE_ADD_0					(0x078)
#define DISP_REG_RDMA_PRE_ADD_1                 (0x07C)
#define DISP_REG_RDMA_PRE_ADD_2                 (0x080)
#define DISP_REG_RDMA_POST_ADD_0				(0x084)
#define DISP_REG_RDMA_POST_ADD_1                (0x088)
#define DISP_REG_RDMA_POST_ADD_2                (0x08C)
#define DISP_REG_RDMA_DUMMY                     (0x090)
#define DISP_REG_RDMA_DEBUG_OUT_SEL             (0x094)
#define DISP_REG_RDMA_MEM_START_ADDR			(0xf00)
#define DISP_REG_RDMA_BG_CON_0                  (0x0a0)
#define DISP_REG_RDMA_BG_CON_1                  (0x0a4)
#define DISP_REG_RDMA_THRESHOLD_FOR_SODI        (0x0a8)
#define DISP_REG_RDMA_THRESHOLD_FOR_DVFS		(0x0ac)
#define DISP_REG_RDMA_SRAM_SEL					(0x0b0)
#define DISP_REG_RDMA_STALL_CG_CON				(0x0b4)
#define DISP_REG_RDMA_SHADOW_UPDATE				(0x0bc)
	#define RDMA_FORCE_COMMIT					REG_FLD(1, 0)
	#define RDMA_BYPASS_SHADOW					REG_FLD(1, 1)
	#define RDMA_READ_WORK_REG					REG_FLD(1, 2)
#define DISP_REG_RDMA_DRAM_CON					(0x0c0)
#define DISP_REG_RDMA_DVFS_SETTING_PREULTRA		(0x0d0)
#define DISP_REG_RDMA_DVFS_SETTING_ULTRA		(0x0d4)
#define DISP_REG_RDMA_LEAVE_DRS_SETTING			(0x0d8)
#define DISP_REG_RDMA_ENTER_DRS_SETTING			(0x0dc)
#define DISP_REG_RDMA_DBG_OUT					(0x100)
#define DISP_REG_RDMA_LINE_BUF_USE				(0x110)

#define DISP_REG_RDMA_IN_P_CNT                                  (0x0f0)
#define DISP_REG_RDMA_IN_LINE_CNT                               (0x0f4)
#define DISP_REG_RDMA_OUT_P_CNT                                 (0x0f8)
#define DISP_REG_RDMA_OUT_LINE_CNT                              (0x0fc)

#define INT_ENABLE_FLD_SOF_ABNORMAL_INT_EN			REG_FLD(1, 7)
#define INT_ENABLE_FLD_FIFO_EMPTY_INT_EN			REG_FLD(1, 6)
#define INT_ENABLE_FLD_TARGET_LINE_INT_EN			REG_FLD(1, 5)
#define INT_ENABLE_FLD_FIFO_UNDERFLOW_INT_EN		        REG_FLD(1, 4)
#define INT_ENABLE_FLD_EOF_ABNORMAL_INT_EN		    REG_FLD(1, 3)
#define INT_ENABLE_FLD_FRAME_END_INT_EN				    REG_FLD(1, 2)
#define INT_ENABLE_FLD_FRAME_START_INT_EN			    REG_FLD(1, 1)
#define INT_ENABLE_FLD_REG_UPDATE_INT_EN			    REG_FLD(1, 0)
#define INT_STATUS_FLD_SOF_ABNORMAL_INT_FLAG			REG_FLD(1, 7)
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
#define MEM_GMC_SETTING_0_FLD_RG_VDE_FORCE_PREULTRA		REG_FLD(1, 31)
#define MEM_GMC_SETTING_0_FLD_RG_VALID_THRESHOLD_FORCE_PREULTRA	REG_FLD(1, 30)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_HIGH	REG_FLD(12, 16)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_LOW   REG_FLD(12, 0)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_COUNT			REG_FLD(16, 16)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_EN			REG_FLD(1, 0)
#define MEM_GMC_SETTING_1_FLD_ULTRA_THRESHOLD_LOW		REG_FLD(12, 0)
#define MEM_GMC_SETTING_1_FLD_ULTRA_THRESHOLD_HIGH		REG_FLD(12, 16)
#define MEM_GMC_SETTING_1_FLD_RG_VALID_THRESHOLD_BLOCK_ULTRA	REG_FLD(1, 30)
#define MEM_GMC_SETTING_1_FLD_RG_VDE_BLOCK_ULTRA		REG_FLD(1, 31)
#define MEM_GMC_SETTING_2_FLD_ISSUE_REQ_THRESHOLD	REG_FLD(12, 0)
#define FIFO_CON_FLD_FIFO_UNDERFLOW_EN				REG_FLD(1, 31)
#define FIFO_CON_FLD_FIFO_PSEUDO_SIZE				REG_FLD(12, 16)
#define FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD		REG_FLD(12, 0)
#define FIFO_LOG_FLD_RDMA_FIFO_LOG					REG_FLD(12, 0)
#define RDMA_FLD_THRESHOLD_LOW					REG_FLD(12, 0)
#define RDMA_FLD_THRESHOLD_HIGH					REG_FLD(12, 16)
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
#define DISP_REG_SPLIT_ENABLE				(DISPSYS_SPLIT0_BASE+0x00)
#define DISP_REG_SPLIT_SW_RESET				(DISPSYS_SPLIT0_BASE+0x04)
#define DISP_REG_SPLIT_CFG				(DISPSYS_SPLIT0_BASE+0x20)
#define DISP_REG_SPLIT_DEBUG				(DISPSYS_SPLIT0_BASE+0x60)

#define ENABLE_FLD_SPLIT_EN				REG_FLD(1, 0)
#define W_RESET_FLD_SPLIT_SW_RST			REG_FLD(1, 0)
#define DEBUG_FLD_SPLIT_FSM				REG_FLD(2, 28)
#define DEBUG_FLD_IN_PIXEL_CNT				REG_FLD(24, 0)
#define DEBUG_FLD_PIXEL_CNT_EN			REG_FLD(1, 31)

/* ------------------------------------------------------------- */
/* CLOCK_MUX */
#define DISP_REG_CLOCK_MUX_START		(DISPSYS_CLOCK_MUX_BASE+0x000)
#define DISP_REG_CLOCK_MUX_CLK_CFG(n)		(DISPSYS_CLOCK_MUX_BASE + 0x100 + (0x10 * (n)))

#define CLK_CFG_0_MM_SEL				REG_FLD(4, 24)
#define CLK_CFG_5_SMI0_2X_SEL				REG_FLD(3, 24)

/* ------------------------------------------------------------- */
/* UFO */
#define DISP_REG_UFO_START				(DISPSYS_UFOE_BASE+0x000)

#define DISP_REG_UFO_INTEN				(DISPSYS_UFOE_BASE+0x004)
#define DISP_REG_UFO_INTSTA				(DISPSYS_UFOE_BASE+0x008)
#define DISP_REG_UFO_DBUF				(DISPSYS_UFOE_BASE+0x00C)
#define DISP_REG_UFO_CRC				(DISPSYS_UFOE_BASE+0x014)
#define DISP_REG_UFO_SW_SCRATCH				(DISPSYS_UFOE_BASE+0x018)
#define DISP_REG_UFO_CR0P6_PAD				(DISPSYS_UFOE_BASE+0x020)
#define DISP_REG_UFO_LR_OVERLAP				(DISPSYS_UFOE_BASE+0x024)
#define DISP_REG_UFO_CK_ON				(DISPSYS_UFOE_BASE+0x028)
#define DISP_REG_UFO_STALL_CG				(DISPSYS_UFOE_BASE+0x030)
#define DISP_REG_UFO_FRAME_WIDTH			(DISPSYS_UFOE_BASE+0x050)
#define DISP_REG_UFO_FRAME_HEIGHT			(DISPSYS_UFOE_BASE+0x054)
#define DISP_REG_UFO_OUTEN				(DISPSYS_UFOE_BASE+0x058)

#define DISP_REG_UFO_R0_CRC				(DISPSYS_UFOE_BASE+0x0F0)

#define DISP_REG_UFO_CFG_0B				(DISPSYS_UFOE_BASE+0x100)
#define DISP_REG_UFO_CFG_1B				(DISPSYS_UFOE_BASE+0x104)
#define DISP_REG_UFO_CFG_2B				(DISPSYS_UFOE_BASE+0x108)
#define DISP_REG_UFO_CFG_3B				(DISPSYS_UFOE_BASE+0x10C)
#define DISP_REG_UFO_CFG_4B				(DISPSYS_UFOE_BASE+0x110)

#define DISP_REG_UFO_RO_0B				(DISPSYS_UFOE_BASE+0x120)
#define DISP_REG_UFO_RO_1B				(DISPSYS_UFOE_BASE+0x124)
#define DISP_REG_UFO_RO_2B				(DISPSYS_UFOE_BASE+0x128)
#define DISP_REG_UFO_RO_3B				(DISPSYS_UFOE_BASE+0x12C)
#define DISP_REG_UFO_RO_4B				(DISPSYS_UFOE_BASE+0x130)

#define DISP_REG_UFO_DBG0				(DISPSYS_UFOE_BASE+0x140)
#define DISP_REG_UFO_DBG1				(DISPSYS_UFOE_BASE+0x144)
#define DISP_REG_UFO_DBG2				(DISPSYS_UFOE_BASE+0x148)
#define DISP_REG_UFO_DBG3				(DISPSYS_UFOE_BASE+0x14C)

#define DISP_REG_UFO_SHADOW				(DISPSYS_UFOE_BASE+0x090)
	#define UFO_READ_WORK_REG					REG_FLD(1, 0)
	#define UFO_FORCE_COMMIT					REG_FLD(1, 1)
	#define UFO_BYPASS_SHADOW					REG_FLD(1, 2)

#define START_FLD_DISP_UFO_START			REG_FLD(1, 0)
#define START_FLD_DISP_UFO_OUT_SEL			REG_FLD(1, 1)
#define START_FLD_DISP_UFO_BYPASS			REG_FLD(1, 2)
#define START_FLD_DISP_UFO_LR_EN			REG_FLD(1, 3)
#define START_FLD_DISP_UFO_SW_RST_ENGINE		REG_FLD(1, 8)
#define START_FLD_DISP_UFO_DBG_SEL			REG_FLD(8, 16)

#define CFG_0B_FLD_DISP_UFO_CFG_VLC_EN			REG_FLD(1, 0)
#define CFG_0B_FLD_DISP_UFO_CFG_COM_RATIO		REG_FLD(2, 1)

#define CFG_1B_FLD_DISP_UFO_CFG_1B			REG_FLD(32, 0)
#define CFG_2B_FLD_DISP_UFO_CFG_2B			REG_FLD(32, 0)
#define CFG_3B_FLD_DISP_UFO_CFG_3B			REG_FLD(32, 0)
#define CFG_4B_FLD_DISP_UFO_CFG_4B			REG_FLD(32, 0)

#define RO_0B_FLD_DISP_UFO_RO_0B			REG_FLD(32, 0)
#define RO_1B_FLD_DISP_UFO_RO_1B			REG_FLD(32, 0)
#define RO_2B_FLD_DISP_UFO_RO_2B			REG_FLD(32, 0)
#define RO_3B_FLD_DISP_UFO_RO_3B			REG_FLD(32, 0)
#define RO_4B_FLD_DISP_UFO_RO_4B			REG_FLD(32, 0)

#define CR0P6_PAD_FLD_DISP_UFO_STR_PAD_NUM		REG_FLD(3, 0)

#define INTEN_FLD_DISP_UFO_INTEN_FR_UNDERRUN		REG_FLD(1, 2)
#define INTEN_FLD_DISP_UFO_INTEN_FR_DONE		REG_FLD(1, 1)
#define INTEN_FLD_DISP_UFO_INTEN_FR_COMPLETE		REG_FLD(1, 0)
#define INTSTA_FLD_DISP_UFO_INTSTA_FR_UNDERRUN		REG_FLD(1, 2)
#define INTSTA_FLD_DISP_UFO_INTSTA_FR_DONE		REG_FLD(1, 1)
#define INTSTA_FLD_DISP_UFO_INTSTA_FR_COMPLETE		REG_FLD(1, 0)

#define DBUF_FLD_DISP_UFO_DBUF_DIS			REG_FLD(5, 0)
#define CRC_FLD_DISP_UFO_CRC_CLR			REG_FLD(1, 2)
#define CRC_FLD_DISP_UFO_CRC_START			REG_FLD(1, 1)
#define CRC_FLD_DISP_UFO_CRC_CEN			REG_FLD(1, 0)

#define SW_SCRATCH_FLD_DISP_UFO_SW_SCRATCH		REG_FLD(32, 0)
#define CK_ON_FLD_DISP_UFO_CK_ON			REG_FLD(1, 0)
#define FRAME_WIDTH_FLD_DISP_UFO_FRAME_WIDTH		REG_FLD(12, 0)
#define FRAME_HEIGHT_FLD_DISP_UFO_FRAME_HEIGHT		REG_FLD(13, 0)
#define R0_CRC_FLD_DISP_UFO_ENGINE_END			REG_FLD(1, 17)
#define R0_CRC_FLD_DISP_UFO_CRC_RDY_0			REG_FLD(1, 16)
#define R0_CRC_FLD_DISP_UFO_CRC_OUT_0			REG_FLD(16, 0)

#define UFOE_READ_WORK_REG					REG_FLD(1, 0)
#define UFOE_FORCE_COMMIT					REG_FLD(1, 1)
#define UFOE_BYPASS_SHADOW					REG_FLD(1, 2)

/* ------------------------------------------------------------- */
/* DSC */
#define DISP_REG_DSC_CON			(0x000)
#define DISP_REG_DSC_INTEN			(0x004)
#define DISP_REG_DSC_INTSTA			(0x008)
#define DISP_REG_DSC_INTACK			(0x00C)
#define DISP_REG_DSC_STA			(0x010)
#define DISP_REG_DSC_PIC_W			(0x018)
#define DISP_REG_DSC_PIC_H			(0x01C)
#define DISP_REG_DSC_SLICE_W			(0x020)
#define DISP_REG_DSC_SLICE_H			(0x024)
#define DISP_REG_DSC_CHUNK_SIZE			(0x028)
#define DISP_REG_DSC_BUF_SIZE			(0x02C)
#define DISP_REG_DSC_MODE			(0x030)
#define DISP_REG_DSC_CFG			(0x034)
#define DISP_REG_DSC_DBG_CON			(0x060)
#define DISP_REG_DSC_CKSM_MON0			(0x064)
#define DISP_REG_DSC_CKSM_MON1			(0x068)
#define DISP_REG_DSC_RESV			(0x06C)
#define DISP_REG_DSC_PPS0			(0x080)
#define DISP_REG_DSC_PPS1			(0x084)
#define DISP_REG_DSC_PPS2			(0x088)
#define DISP_REG_DSC_PPS3			(0x08C)
#define DISP_REG_DSC_PPS4			(0x090)
#define DISP_REG_DSC_PPS5			(0x094)
#define DISP_REG_DSC_PPS6			(0x098)
#define DISP_REG_DSC_PPS7			(0x09C)
#define DISP_REG_DSC_PPS8			(0x0A0)
#define DISP_REG_DSC_PPS9			(0x0A4)
#define DISP_REG_DSC_PPS10			(0x0A8)
#define DISP_REG_DSC_PPS11			(0x0AC)
#define DISP_REG_DSC_PPS12			(0x0B0)
#define DISP_REG_DSC_PPS13			(0x0B4)
#define DISP_REG_DSC_PPS14			(0x0B8)
#define DISP_REG_DSC_PPS15			(0x0BC)
#define DISP_REG_DSC_PPS16			(0x0C0)
#define DISP_REG_DSC_PPS17			(0x0C4)
#define DISP_REG_DSC_PPS18			(0x0C8)
#define DISP_REG_DSC_PPS19			(0x0CC)
#define DISP_REG_DSC_SHADOW			(0x200)

#define CON_FLD_DISP_DSC_EN		REG_FLD(1, 0)
#define CON_FLD_DISP_DSC_DUAL_INOUT	REG_FLD(1, 2)
#define CON_FLD_DISP_DSC_IN_SEL		REG_FLD(1, 3)
#define CON_FLD_DISP_DSC_BYPASS		REG_FLD(1, 4)
#define CON_FLD_DISP_DSC_RELAY		REG_FLD(1, 5)
#define CON_FLD_DISP_DSC_SW_RESET		REG_FLD(1, 8)
#define CON_FLD_DISP_DSC_STALL_CLK_GATE_EN	REG_FLD(1, 9)
#define CON_FLD_DISP_DSC_UFOE_SEL		REG_FLD(1, 16)

#define INTEN_FLD_DISP_DSC_DONE_INTEN		REG_FLD(1, 0)
#define INTEN_FLD_DISP_DSC_ERR_INTEN		REG_FLD(1, 1)
#define INTEN_FLD_DISP_DSC_ZERO_FIFO_ERR_INTEN	REG_FLD(1, 2)
#define INTEN_FLD_DISP_DSC_ABNORMAL_EOF_INTEN	REG_FLD(1, 3)

#define INTSTA_FLD_DISP_DSC_DONE_INTEN		REG_FLD(1, 0)
#define INTSTA_FLD_DISP_DSC_ERR_INTEN		REG_FLD(1, 1)
#define INTSTA_FLD_DISP_DSC_ZERO_FIFO_ERR_INTEN	REG_FLD(1, 2)
#define INTSTA_FLD_DISP_DSC_ABNORMAL_EOF_INTEN	REG_FLD(1, 3)

#define CFG_FLD_PIC_WIDTH			REG_FLD(16, 0)
#define CFG_FLD_PIC_GROUP_WIDTH_M1		REG_FLD(16, 16)

#define CFG_FLD_PIC_HEIGHT_M1			REG_FLD(16, 0)
#define CFG_FLD_PIC_HEIGHT_EXT_M1		REG_FLD(16, 16)

#define CFG_FLD_SLICE_WIDTH			REG_FLD(16, 0)
#define CFG_FLD_SLICE_GROUP_WIDTH_M1		REG_FLD(16, 16)

#define CFG_FLD_SLICE_HEIGHT_M1			REG_FLD(16, 0)
#define CFG_FLD_SLICE_NUM_M1			REG_FLD(14, 16)
#define CFG_FLD_SLICE_WIDTH_MOD3		REG_FLD(2, 30)

#define CFG_FLD_CHUNK_SIZE			REG_FLD(16, 0)

#define CFG_FLD_BUF_SIZE			REG_FLD(24, 0)

#define CFG_FLD_SLICE_MODE			REG_FLD(1, 0)
#define CFG_FLD_PIX_TYPE			REG_FLD(1, 1)
#define CFG_FLD_RGB_SWAP			REG_FLD(1, 2)
#define CFG_FLD_INIT_DELAY_HEIGHT		REG_FLD(4, 8)
#define CFG_FLD_OBUF_STR_IF_BUF_FULL		REG_FLD(1, 16)

#define CFG_FLD_CFG				REG_FLD(16, 0)

#define CFG_FLD_DEBUG_OUT_SEL			REG_FLD(5, 0)
#define CFG_FLD_DEBUG_OUT_EN			REG_FLD(1, 8)
#define CFG_FLD_CKSM_CAL_EN			REG_FLD(1, 9)

#define CFG_FLD_UP_LINE_BUF_DEPTH		REG_FLD(4, 0)
#define CFG_FLD_BIT_PER_CHANNEL			REG_FLD(4, 4)
#define CFG_FLD_BIT_PER_PIXEL			REG_FLD(10, 8)
#define CFG_FLD_RCT_ON				REG_FLD(1, 18)
#define CFG_FLD_BP_ENABLE			REG_FLD(1, 19)

#define CFG_FLD_INITIAL_XMIT_DELAY		REG_FLD(16, 0)
#define CFG_FLD_INITIAL_DEC_DELAY		REG_FLD(16, 16)

#define CFG_FLD_INITIAL_SCALE_VALUE		REG_FLD(16, 0)
#define CFG_FLD_SCALE_INCREMENT_INTERVAL	REG_FLD(16, 16)

#define CFG_FLD_SCALE_DECREMENT_INTERVAL	REG_FLD(16, 0)
#define CFG_FLD_FIRST_LINE_BPG_OFFSET		REG_FLD(16, 16)

#define CFG_FLD_NFL_BPG_OFFSET			REG_FLD(16, 0)
#define CFG_FLD_SLICE_BPG_OFFSET		REG_FLD(16, 16)

#define CFG_FLD_INITIAL_OFFSET			REG_FLD(16, 0)
#define CFG_FLD_FINAL_OFFSET			REG_FLD(16, 16)

#define CFG_FLD_FLATNESS_MIN_QP			REG_FLD(5, 0)
#define CFG_FLD_FLATNESS_MAX_QP			REG_FLD(5, 8)
#define CFG_FLD_RC_MODEL_SIZE			REG_FLD(16, 16)

#define SHADOW_FLD_FORCE_COMMIT			REG_FLD(1, 0)
#define SHADOW_FLD_BYPASS_SHADOW		REG_FLD(1, 1)
#define SHADOW_FLD_READ_WORKING			REG_FLD(1, 2)

/* ------------------------------------------------------------- */
/* WDMA */
#define DISP_REG_WDMA_INTEN							(0x000)
#define DISP_REG_WDMA_INTSTA						(0x004)
#define DISP_REG_WDMA_EN							(0x008)
	#define WDMA_ENABLE								REG_FLD(1, 0)
	#define WDMA_BYPASS_SHADOW						REG_FLD(1, 1)
	#define WDMA_FORCE_COMMIT						REG_FLD(1, 2)
	#define WDMA_READ_WORK_REG						REG_FLD(1, 3)
#define DISP_REG_WDMA_RST							(0x00C)
#define DISP_REG_WDMA_SMI_CON						(0x010)
#define DISP_REG_WDMA_CFG							(0x014)
#define DISP_REG_WDMA_SRC_SIZE						(0x018)
#define DISP_REG_WDMA_CLIP_SIZE						(0x01C)
#define DISP_REG_WDMA_CLIP_COORD					(0x020)
#define DISP_REG_WDMA_DST_W_IN_BYTE					(0x028)
#define DISP_REG_WDMA_ALPHA							(0x02C)
#define DISP_REG_WDMA_BUF_CON1						(0x038)
#define DISP_REG_WDMA_C00							(0x040)
#define DISP_REG_WDMA_C02							(0x044)
#define DISP_REG_WDMA_C10							(0x048)
#define DISP_REG_WDMA_C12							(0x04C)
#define DISP_REG_WDMA_C20							(0x050)
#define DISP_REG_WDMA_C22							(0x054)
#define DISP_REG_WDMA_PRE_ADD0						(0x058)
#define DISP_REG_WDMA_PRE_ADD2						(0x05C)
#define DISP_REG_WDMA_POST_ADD0						(0x060)
#define DISP_REG_WDMA_POST_ADD2						(0x064)
#define DISP_REG_WDMA_DST_UV_PITCH					(0x078)
#define DISP_REG_WDMA_DST_ADDR_OFFSET0				(0x080)
#define DISP_REG_WDMA_DST_ADDR_OFFSET1				(0x084)
#define DISP_REG_WDMA_DST_ADDR_OFFSET2				(0x088)
#define DISP_REG_WDMA_PROC_TRACK_CON_0				(0x090)
#define DISP_REG_WDMA_PROC_TRACK_CON_1				(0x094)
#define DISP_REG_WDMA_PROC_TRACK_CON_2				(0x098)
#define DISP_REG_WDMA_FLOW_CTRL_DBG					(0x0A0)
#define DISP_REG_WDMA_EXEC_DBG						(0x0A4)
#define DISP_REG_WDMA_CT_DBG						(0x0A8)
#define DISP_REG_WDMA_SMI_TRAFFIC_DBG               (0x0AC)
#define DISP_REG_WDMA_PROC_TRACK_DBG_0              (0x0B0)
#define DISP_REG_WDMA_PROC_TRACK_DBG_1              (0x0B4)
#define DISP_REG_WDMA_DEBUG							(0x0B8)
#define DISP_REG_WDMA_DUMMY							(0x100)
#define DISP_REG_WDMA_BUF_CON3						(0x104)
#define DISP_REG_WDMA_BUF_CON4						(0x108)
#define DISP_REG_WDMA_BUF_CON5						(0x200)
#define DISP_REG_WDMA_BUF_CON6						(0x204)
#define DISP_REG_WDMA_BUF_CON7						(0x208)
#define DISP_REG_WDMA_BUF_CON8						(0x20C)
#define DISP_REG_WDMA_BUF_CON9						(0x210)
#define DISP_REG_WDMA_BUF_CON10						(0x214)
#define DISP_REG_WDMA_BUF_CON11						(0x218)
#define DISP_REG_WDMA_BUF_CON12						(0x21C)
#define DISP_REG_WDMA_BUF_CON13						(0x220)
#define DISP_REG_WDMA_BUF_CON14						(0x224)
#define DISP_REG_WDMA_BUF_CON15						(0x228)
#define DISP_REG_WDMA_BUF_CON16						(0x22C)
#define DISP_REG_WDMA_BUF_CON17						(0x230)
#define DISP_REG_WDMA_BUF_CON18						(0x234)
#define DISP_REG_DRS_CON0							(0x250)
#define DISP_REG_DRS_CON1							(0x254)
#define DISP_REG_DRS_CON2							(0x258)
#define DISP_REG_DRS_CON3							(0x25C)



#define DISP_REG_WDMA_DITHER_0						(0xE00)
#define DISP_REG_WDMA_DITHER_5						(0xE14)
#define DISP_REG_WDMA_DITHER_6						(0xE18)
#define DISP_REG_WDMA_DITHER_7						(0xE1C)
#define DISP_REG_WDMA_DITHER_8						(0xE20)
#define DISP_REG_WDMA_DITHER_9						(0xE24)
#define DISP_REG_WDMA_DITHER_10						(0xE28)
#define DISP_REG_WDMA_DITHER_11						(0xE2C)
#define DISP_REG_WDMA_DITHER_12						(0xE30)
#define DISP_REG_WDMA_DITHER_13						(0xE34)
#define DISP_REG_WDMA_DITHER_14						(0xE38)
#define DISP_REG_WDMA_DITHER_15						(0xE3C)
#define DISP_REG_WDMA_DITHER_16						(0xE40)
#define DISP_REG_WDMA_DITHER_17						(0xE44)
#define DISP_REG_WDMA_DST_ADDR0						(0xF00)
#define DISP_REG_WDMA_DST_ADDR1						(0xF04)
#define DISP_REG_WDMA_DST_ADDR2						(0xF08)

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
#define BUF_CON1_FLD_FIFO_PSEUDO_SIZE					REG_FLD(9, 0)
#define BUF_CON_FLD_PRE_ULTRA_LOW						REG_FLD(10, 0)
#define BUF_CON_FLD_ULTRA_LOW							REG_FLD(10, 16)
#define BUF_CON_FLD_PRE_ULTRA_HIGH						REG_FLD(10, 0)
#define BUF_CON_FLD_ULTRA_HIGH							REG_FLD(10, 16)
#define BUF_CON17_FLD_DVFS_EN							REG_FLD(1, 0)
#define BUF_CON17_FLD_DVFS_TH_Y							REG_FLD(10, 16)
#define BUF_CON18_FLD_DVFS_TH_U							REG_FLD(10, 0)
#define BUF_CON18_FLD_DVFS_TH_V							REG_FLD(10, 16)
#define BUF_DRS_FLD_DRS_EN								REG_FLD(1, 0)
#define BUF_DRS_FLD_ENTER_DRS_TH_Y						REG_FLD(10, 16)
#define BUF_DRS_FLD_ENTER_DRS_TH_U						REG_FLD(10, 0)
#define BUF_DRS_FLD_ENTER_DRS_TH_V						REG_FLD(10, 16)
#define BUF_DRS_FLD_LEAVE_DRS_TH_Y						REG_FLD(10, 16)
#define BUF_DRS_FLD_LEAVE_DRS_TH_U						REG_FLD(10, 0)
#define BUF_DRS_FLD_LEAVE_DRS_TH_V						REG_FLD(10, 16)
#define BUF_CON3_FLD_ISSUE_REQ_TH_Y						REG_FLD(9, 0)
#define BUF_CON3_FLD_ISSUE_REQ_TH_U						REG_FLD(9, 16)
#define BUF_CON4_FLD_ISSUE_REQ_TH_V						REG_FLD(9, 0)

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

/* ------------------------------------------------------------- */
/* DISP_RSZ reg offset */
#define DISP_REG_RSZ_ENABLE				(0x000)
# define FLD_RSZ_RST					REG_FLD_MSB_LSB(16, 16)
# define FLD_RSZ_EN					REG_FLD_MSB_LSB(0, 0)
#define DISP_REG_RSZ_CONTROL_1				(0x004)
# define FLD_RSZ_INT_WCLR_EN				REG_FLD_MSB_LSB(31, 31)
# define FLD_RSZ_INTEN					REG_FLD_MSB_LSB(30, 28)
# define FLD_RSZ_DCM_DIS				REG_FLD_MSB_LSB(27, 27)
# define FLD_RSZ_VERTICAL_TABLE_SELECT			REG_FLD_MSB_LSB(25, 21)
# define FLD_RSZ_HORIZONTAL_TABLE_SELECT		REG_FLD_MSB_LSB(20, 16)
# define FLD_RSZ_LBCSON_MODE				REG_FLD_MSB_LSB(9, 9)
# define FLD_RSZ_VERTICAL_EN				REG_FLD_MSB_LSB(1, 1)
# define FLD_RSZ_HORIZONTAL_EN				REG_FLD_MSB_LSB(0, 0)
#define DISP_REG_RSZ_CONTROL_2				(0x008)
# define FLD_RSZ_RGB_BIT_MODE				REG_FLD_MSB_LSB(28, 28)
# define FLD_RSZ_POWER_SAVING				REG_FLD_MSB_LSB(9, 9)
#define DISP_REG_RSZ_INT_FLAG				(0x00c)
# define FLD_RSZ_SOF_RESET				REG_FLD_MSB_LSB(5, 5)
# define FLD_RSZ_SIZE_ERR				REG_FLD_MSB_LSB(4, 4)
# define FLD_RSZ_FRAME_END				REG_FLD_MSB_LSB(1, 1)
# define FLD_RSZ_FRAME_START				REG_FLD_MSB_LSB(0, 0)
#define DISP_REG_RSZ_INPUT_IMAGE			(0x010)
# define FLD_RSZ_INPUT_IMAGE_H				REG_FLD_MSB_LSB(31, 16)
# define FLD_RSZ_INPUT_IMAGE_W				REG_FLD_MSB_LSB(15, 0)
#define DISP_REG_RSZ_OUTPUT_IMAGE			(0x014)
# define FLD_RSZ_OUTPUT_IMAGE_H				REG_FLD_MSB_LSB(31, 16)
# define FLD_RSZ_OUTPUT_IMAGE_W				REG_FLD_MSB_LSB(15, 0)
#define DISP_REG_RSZ_HORIZONTAL_COEFF_STEP		(0x018)
# define FLD_RSZ_HORIZONTAL_COEFF_STEP			REG_FLD_MSB_LSB(22, 0)
#define DISP_REG_RSZ_VERTICAL_COEFF_STEP		(0x01c)
# define FLD_RSZ_VERTICAL_COEFF_STEP			REG_FLD_MSB_LSB(22, 0)
#define DISP_REG_RSZ_LUMA_HORIZONTAL_INTEGER_OFFSET	(0x020)
# define FLD_RSZ_LUMA_HORIZONTAL_INTEGER_OFFSET		REG_FLD_MSB_LSB(15, 0)
#define DISP_REG_RSZ_LUMA_HORIZONTAL_SUBPIXEL_OFFSET	(0x024)
# define FLD_RSZ_LUMA_HORIZONTAL_SUBPIXEL_OFFSET	REG_FLD_MSB_LSB(20, 0)
#define DISP_REG_RSZ_LUMA_VERTICAL_INTEGER_OFFSET	(0x028)
# define FLD_RSZ_LUMA_VERTICAL_INTEGER_OFFSET		REG_FLD_MSB_LSB(15, 0)
#define DISP_REG_RSZ_LUMA_VERTICAL_SUBPIXEL_OFFSET	(0x02c)
# define FLD_RSZ_LUMA_VERTICAL_SUBPIXEL_OFFSET		REG_FLD_MSB_LSB(20, 0)
#define DISP_REG_RSZ_RSV				(0x040)
#define DISP_REG_RSZ_DEBUG_SEL				(0x044)
#define DISP_REG_RSZ_DEBUG				(0x048)
#define DISP_REG_RSZ_SHADOW_CTRL			(0x0f0)
# define FLD_RSZ_READ_WRK_REG				REG_FLD_MSB_LSB(2, 2)
# define FLD_RSZ_FORCE_COMMIT				REG_FLD_MSB_LSB(1, 1)
# define FLD_RSZ_BYPASS_SHADOW				REG_FLD_MSB_LSB(0, 0)
#define DISP_REG_RSZ_ATPG				(0x0fc)
#define DISP_REG_PAT2_GET_SET				(0x200)
#define DISP_REG_PAT2_GEN_COLOR0			(0x208)
#define DISP_REG_PAT2_GEN_COLOR1			(0x20c)
#define DISP_REG_PAT2_GEN_CURSOR_RB0			(0x214)
#define DISP_REG_PAT2_GEN_CURSOR_RB1			(0x218)
#define DISP_REG_PAT2_GEN_TILE_POS			(0x224)
#define DISP_REG_PAT2_GEN_TILE_OV			(0x228)

/* DISP_RSZ bit field */
#define FLD_RSZ_RSV				REG_FLD_MSB_LSB(31, 0)
#define FLD_RSZ_DEBUG_SEL			REG_FLD_MSB_LSB(3, 0)
#define FLD_RSZ_DEBUG				REG_FLD_MSB_LSB(31, 0)
#define FLD_RSZ_ATPG_CT				REG_FLD_MSB_LSB(1, 1)
#define FLD_RSZ_ATPG_OB				REG_FLD_MSB_LSB(0, 0)
#define FLD_RSZ_REG2_PAT_TYPE			REG_FLD_MSB_LSB(23, 16)
#define FLD_RSZ_REG2_CURSOR_SHOW		REG_FLD_MSB_LSB(1, 1)
#define FLD_RSZ_REG2_PAT_GEN_EN			REG_FLD_MSB_LSB(0, 0)
#define FLD_RSZ_REG2_COLOR_V			REG_FLD_MSB_LSB(29, 20)
#define FLD_RSZ_REG2_COLOR_U			REG_FLD_MSB_LSB(19, 10)
#define FLD_RSZ_REG2_COLOR_Y			REG_FLD_MSB_LSB(9, 0)
#define FLD_RSZ_REG2_COLOR_A			REG_FLD_MSB_LSB(7, 0)
#define FLD_RSZ_REG2_POS_Y			REG_FLD_MSB_LSB(28, 16)
#define FLD_RSZ_REG2_POS_X			REG_FLD_MSB_LSB(12, 0)
#define FLD_RSZ_REG2_RB_CURSOR_U		REG_FLD_MSB_LSB(25, 16)
#define FLD_RSZ_REG2_RB_CURSOR_Y		REG_FLD_MSB_LSB(9, 0)
#define FLD_RSZ_REG2_RB_CURSOR_A		REG_FLD_MSB_LSB(23, 16)
#define FLD_RSZ_REG2_RB_CURSOR_V		REG_FLD_MSB_LSB(9, 0)
#define FLD_RSZ_REG2_TILE_POS_Y			REG_FLD_MSB_LSB(28, 16)
#define FLD_RSZ_REG2_TILE_POS_X			REG_FLD_MSB_LSB(12, 0)
#define FLD_RSZ_REG2_TILE_OV_Y			REG_FLD_MSB_LSB(15, 8)
#define FLD_RSZ_REG2_TILE_OV_X			REG_FLD_MSB_LSB(7, 0)

#endif
