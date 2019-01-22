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
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Ana.h
 *
 * Project:
 * --------
 *   MT6797  Audio Driver Ana
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

#ifndef _AUDDRV_ANA_H_
#define _AUDDRV_ANA_H_

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include "mtk-auddrv-common.h"

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/


/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
typedef	uint8 kal_uint8;
typedef	int8 kal_int8;
typedef	uint32 kal_uint32;
typedef	int32 kal_int32;
typedef	uint64 kal_uint64;
typedef	int64 kal_int64;

#define PMIC_REG_BASE          (0x0)
#define AUD_TOP_ID                    ((UINT32)(PMIC_REG_BASE + 0x1540))
#define AUD_TOP_REV0                  ((UINT32)(PMIC_REG_BASE + 0x1542))
#define AUD_TOP_REV1                  ((UINT32)(PMIC_REG_BASE + 0x1544))
#define AUD_TOP_CKPDN_PM0             ((UINT32)(PMIC_REG_BASE + 0x1546))
#define AUD_TOP_CKPDN_PM1             ((UINT32)(PMIC_REG_BASE + 0x1548))
#define AUD_TOP_CKPDN_CON0            ((UINT32)(PMIC_REG_BASE + 0x154a))
#define AUD_TOP_CKSEL_CON0            ((UINT32)(PMIC_REG_BASE + 0x154c))
#define AUD_TOP_CKTST_CON0            ((UINT32)(PMIC_REG_BASE + 0x154e))
#define AUD_TOP_RST_CON0              ((UINT32)(PMIC_REG_BASE + 0x1550))
#define AUD_TOP_RST_BANK_CON0         ((UINT32)(PMIC_REG_BASE + 0x1552))
#define AUD_TOP_INT_CON0              ((UINT32)(PMIC_REG_BASE + 0x1554))
#define AUD_TOP_INT_CON0_SET          ((UINT32)(PMIC_REG_BASE + 0x1556))
#define AUD_TOP_INT_CON0_CLR          ((UINT32)(PMIC_REG_BASE + 0x1558))
#define AUD_TOP_INT_MASK_CON0         ((UINT32)(PMIC_REG_BASE + 0x155a))
#define AUD_TOP_INT_MASK_CON0_SET     ((UINT32)(PMIC_REG_BASE + 0x155c))
#define AUD_TOP_INT_MASK_CON0_CLR     ((UINT32)(PMIC_REG_BASE + 0x155e))
#define AUD_TOP_INT_STATUS0           ((UINT32)(PMIC_REG_BASE + 0x1560))
#define AUD_TOP_INT_RAW_STATUS0       ((UINT32)(PMIC_REG_BASE + 0x1562))
#define AUD_TOP_INT_MISC_CON0         ((UINT32)(PMIC_REG_BASE + 0x1564))
#define AUDNCP_CLKDIV_CON0            ((UINT32)(PMIC_REG_BASE + 0x1566))
#define AUDNCP_CLKDIV_CON1            ((UINT32)(PMIC_REG_BASE + 0x1568))
#define AUDNCP_CLKDIV_CON2            ((UINT32)(PMIC_REG_BASE + 0x156a))
#define AUDNCP_CLKDIV_CON3            ((UINT32)(PMIC_REG_BASE + 0x156c))
#define AUDNCP_CLKDIV_CON4            ((UINT32)(PMIC_REG_BASE + 0x156e))
#define AUD_TOP_MON_CON0              ((UINT32)(PMIC_REG_BASE + 0x1570))
#define AUDIO_DIG_ID                  ((UINT32)(PMIC_REG_BASE + 0x1580))
#define AUDIO_DIG_REV0                ((UINT32)(PMIC_REG_BASE + 0x1582))
#define AUDIO_DIG_REV1                ((UINT32)(PMIC_REG_BASE + 0x1584))
#define AFE_UL_DL_CON0                ((UINT32)(PMIC_REG_BASE + 0x1586))
#define AFE_DL_SRC2_CON0_L            ((UINT32)(PMIC_REG_BASE + 0x1588))
#define AFE_UL_SRC_CON0_H             ((UINT32)(PMIC_REG_BASE + 0x158a))
#define AFE_UL_SRC_CON0_L             ((UINT32)(PMIC_REG_BASE + 0x158c))
#define PMIC_AFE_TOP_CON0             ((UINT32)(PMIC_REG_BASE + 0x158e))
#define PMIC_AUDIO_TOP_CON0           ((UINT32)(PMIC_REG_BASE + 0x1590))
#define AFE_MON_DEBUG0                ((UINT32)(PMIC_REG_BASE + 0x1592))
#define AFUNC_AUD_CON0                ((UINT32)(PMIC_REG_BASE + 0x1594))
#define AFUNC_AUD_CON1                ((UINT32)(PMIC_REG_BASE + 0x1596))
#define AFUNC_AUD_CON2                ((UINT32)(PMIC_REG_BASE + 0x1598))
#define AFUNC_AUD_CON3                ((UINT32)(PMIC_REG_BASE + 0x159a))
#define AFUNC_AUD_CON4                ((UINT32)(PMIC_REG_BASE + 0x159c))
#define AFUNC_AUD_MON0                ((UINT32)(PMIC_REG_BASE + 0x159e))
#define AUDRC_TUNE_MON0               ((UINT32)(PMIC_REG_BASE + 0x15a0))
#define AFE_ADDA_MTKAIF_FIFO_CFG0     ((UINT32)(PMIC_REG_BASE + 0x15a2))
#define AFE_ADDA_MTKAIF_FIFO_LOG_MON1 ((UINT32)(PMIC_REG_BASE + 0x15a4))
#define PMIC_AFE_ADDA_MTKAIF_MON0     ((UINT32)(PMIC_REG_BASE + 0x15a6))
#define PMIC_AFE_ADDA_MTKAIF_MON1     ((UINT32)(PMIC_REG_BASE + 0x15a8))
#define PMIC_AFE_ADDA_MTKAIF_MON2     ((UINT32)(PMIC_REG_BASE + 0x15aa))
#define PMIC_AFE_ADDA_MTKAIF_MON3     ((UINT32)(PMIC_REG_BASE + 0x15ac))
#define PMIC_ADDA_MTKAIF_CFG0         ((UINT32)(PMIC_REG_BASE + 0x15ae))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG0  ((UINT32)(PMIC_REG_BASE + 0x15b0))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG1  ((UINT32)(PMIC_REG_BASE + 0x15b2))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG2  ((UINT32)(PMIC_REG_BASE + 0x15b4))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG3  ((UINT32)(PMIC_REG_BASE + 0x15b6))
#define PMIC_AFE_ADDA_MTKAIF_TX_CFG1  ((UINT32)(PMIC_REG_BASE + 0x15b8))
#define AFE_SGEN_CFG0                 ((UINT32)(PMIC_REG_BASE + 0x15ba))
#define AFE_SGEN_CFG1                 ((UINT32)(PMIC_REG_BASE + 0x15bc))
#define AFE_ADC_ASYNC_FIFO_CFG        ((UINT32)(PMIC_REG_BASE + 0x15be))
#define AFE_DCCLK_CFG0                ((UINT32)(PMIC_REG_BASE + 0x15c0))
#define AFE_DCCLK_CFG1                ((UINT32)(PMIC_REG_BASE + 0x15c2))
#define AUDIO_DIG_CFG                 ((UINT32)(PMIC_REG_BASE + 0x15c4))
#define AFE_AUD_PAD_TOP               ((UINT32)(PMIC_REG_BASE + 0x15c6))
#define AFE_AUD_PAD_TOP_MON           ((UINT32)(PMIC_REG_BASE + 0x15c8))
#define AFE_AUD_PAD_TOP_MON1          ((UINT32)(PMIC_REG_BASE + 0x15ca))
#define AUDENC_DSN_ID                 ((UINT32)(PMIC_REG_BASE + 0x1600))
#define AUDENC_DSN_REV0               ((UINT32)(PMIC_REG_BASE + 0x1602))
#define AUDENC_DSN_REV1               ((UINT32)(PMIC_REG_BASE + 0x1604))
#define AUDENC_ANA_CON0               ((UINT32)(PMIC_REG_BASE + 0x1606))
#define AUDENC_ANA_CON1               ((UINT32)(PMIC_REG_BASE + 0x1608))
#define AUDENC_ANA_CON2               ((UINT32)(PMIC_REG_BASE + 0x160a))
#define AUDENC_ANA_CON3               ((UINT32)(PMIC_REG_BASE + 0x160c))
#define AUDENC_ANA_CON4               ((UINT32)(PMIC_REG_BASE + 0x160e))
#define AUDENC_ANA_CON5               ((UINT32)(PMIC_REG_BASE + 0x1610))
#define AUDENC_ANA_CON6               ((UINT32)(PMIC_REG_BASE + 0x1612))
#define AUDENC_ANA_CON7               ((UINT32)(PMIC_REG_BASE + 0x1614))
#define AUDENC_ANA_CON8               ((UINT32)(PMIC_REG_BASE + 0x1616))
#define AUDENC_ANA_CON9               ((UINT32)(PMIC_REG_BASE + 0x1618))
#define AUDENC_ANA_CON10              ((UINT32)(PMIC_REG_BASE + 0x161a))
#define AUDENC_ANA_CON11              ((UINT32)(PMIC_REG_BASE + 0x161c))
#define AUDENC_ANA_CON12              ((UINT32)(PMIC_REG_BASE + 0x161e))
#define AUDDEC_DSN_ID                 ((UINT32)(PMIC_REG_BASE + 0x1640))
#define AUDDEC_DSN_REV0               ((UINT32)(PMIC_REG_BASE + 0x1642))
#define AUDDEC_DSN_REV1               ((UINT32)(PMIC_REG_BASE + 0x1644))
#define AUDDEC_ANA_CON0               ((UINT32)(PMIC_REG_BASE + 0x1646))
#define AUDDEC_ANA_CON1               ((UINT32)(PMIC_REG_BASE + 0x1648))
#define AUDDEC_ANA_CON2               ((UINT32)(PMIC_REG_BASE + 0x164a))
#define AUDDEC_ANA_CON3               ((UINT32)(PMIC_REG_BASE + 0x164c))
#define AUDDEC_ANA_CON4               ((UINT32)(PMIC_REG_BASE + 0x164e))
#define AUDDEC_ANA_CON5               ((UINT32)(PMIC_REG_BASE + 0x1650))
#define AUDDEC_ANA_CON6               ((UINT32)(PMIC_REG_BASE + 0x1652))
#define AUDDEC_ANA_CON7               ((UINT32)(PMIC_REG_BASE + 0x1654))
#define AUDDEC_ANA_CON8               ((UINT32)(PMIC_REG_BASE + 0x1656))
#define AUDDEC_ANA_CON9               ((UINT32)(PMIC_REG_BASE + 0x1658))
#define AUDDEC_ANA_CON10              ((UINT32)(PMIC_REG_BASE + 0x165a))
#define AUDDEC_ANA_CON11              ((UINT32)(PMIC_REG_BASE + 0x165c))
#define AUDDEC_ANA_CON12              ((UINT32)(PMIC_REG_BASE + 0x165e))
#define AUDDEC_ANA_CON13              ((UINT32)(PMIC_REG_BASE + 0x1660))
#define AUDDEC_ANA_CON14              ((UINT32)(PMIC_REG_BASE + 0x1662))
#define AUDDEC_ANA_CON15              ((UINT32)(PMIC_REG_BASE + 0x1664))
#define AUDDEC_ELR_NUM                ((UINT32)(PMIC_REG_BASE + 0x1666))
#define AUDDEC_ELR_0                  ((UINT32)(PMIC_REG_BASE + 0x1668))

#define ZCD_CON0                      ((UINT32)(PMIC_REG_BASE + 0x1686))
#define ZCD_CON1                      ((UINT32)(PMIC_REG_BASE + 0x1688))
#define ZCD_CON2                      ((UINT32)(PMIC_REG_BASE + 0x168a))
#define ZCD_CON3                      ((UINT32)(PMIC_REG_BASE + 0x168c))
#define ZCD_CON4                      ((UINT32)(PMIC_REG_BASE + 0x168e))
#define ZCD_CON5                      ((UINT32)(PMIC_REG_BASE + 0x1690))

#define TOP_CLKSQ           ((UINT32)(PMIC_REG_BASE + 0x132))
#define TOP_CLKSQ_SET       ((UINT32)(PMIC_REG_BASE + 0x134))
#define TOP_CLKSQ_CLR       ((UINT32)(PMIC_REG_BASE + 0x136))

#define DRV_CON3            ((UINT32)(PMIC_REG_BASE + 0x3a))
#define GPIO_DIR0           ((UINT32)(PMIC_REG_BASE + 0x4c))
#define GPIO_MODE2          ((UINT32)(PMIC_REG_BASE + 0x7a)) /* mosi */
#define GPIO_MODE2_SET      ((UINT32)(PMIC_REG_BASE + 0x7c))
#define GPIO_MODE3          ((UINT32)(PMIC_REG_BASE + 0x80)) /* miso */
#define GPIO_MODE3_SET      ((UINT32)(PMIC_REG_BASE + 0x82))
#define GPIO_MODE3_CLR      ((UINT32)(PMIC_REG_BASE + 0x84))

#define DCXO_CW14           ((UINT32)(PMIC_REG_BASE + 0x9ea))

#if 1
/* register number */

#else
#include <mach/upmu_hw.h>
#endif

void Ana_Set_Reg(uint32 offset, uint32 value, uint32 mask);
uint32 Ana_Get_Reg(uint32 offset);

/* for debug usage */
void Ana_Log_Print(void);

int Ana_Debug_Read(char *buffer, const int size);

#endif
