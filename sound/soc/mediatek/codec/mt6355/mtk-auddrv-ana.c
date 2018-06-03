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
 *   AudDrv_Ana.c
 *
 * Project:
 * --------
 *   MT6355  Audio Driver ana Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Michael Hsiao (mtk08429)
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-common.h"

#ifdef AUDIO_USING_WRAP_DRIVER
#include <mach/mtk_pmic_wrap.h>
#endif

/* #include "AudDrv_Type_Def.h" */

static DEFINE_SPINLOCK(ana_set_reg_lock);

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/
unsigned int Ana_Get_Reg(unsigned int offset)
{
	/* get pmic register */
	int ret = 0;
	unsigned int Rdata = 0;
#ifdef AUDIO_USING_WRAP_DRIVER
	ret = pwrap_read(offset, &Rdata);
#endif
	PRINTK_ANA_REG("Ana_Get_Reg offset=0x%x,Rdata=0x%x,ret=%d\n", offset, Rdata, ret);
	return Rdata;
}
EXPORT_SYMBOL(Ana_Get_Reg);

void Ana_Set_Reg(unsigned int offset, unsigned int value, unsigned int mask)
{
	/* set pmic register or analog CONTROL_IFACE_PATH */
	int ret = 0;
	unsigned int Reg_Value;
	unsigned long flags = 0;

	PRINTK_ANA_REG("Ana_Set_Reg offset= 0x%x , value = 0x%x mask = 0x%x\n", offset, value,
		       mask);
#ifdef AUDIO_USING_WRAP_DRIVER
	spin_lock_irqsave(&ana_set_reg_lock, flags);
	Reg_Value = Ana_Get_Reg(offset);
	Reg_Value &= (~mask);
	Reg_Value |= (value & mask);
	ret = pwrap_write(offset, Reg_Value);
	spin_unlock_irqrestore(&ana_set_reg_lock, flags);

	Reg_Value = Ana_Get_Reg(offset);
	if ((Reg_Value & mask) != (value & mask))
		pr_warn("Ana_Set_Reg  mask = 0x%x ret = %d Reg_Value = 0x%x\n", mask, ret,
			 Reg_Value);
#endif
}
EXPORT_SYMBOL(Ana_Set_Reg);

void Ana_Log_Print(void)
{
	pr_debug("+Ana_Log_Print\n");
	pr_debug("AFE_UL_DL_CON0	       = 0x%x\n", Ana_Get_Reg(AFE_UL_DL_CON0));
	pr_debug("AFE_DL_SRC2_CON0_H	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC2_CON0_H));
	pr_debug("AFE_DL_SRC2_CON0_L	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC2_CON0_L));
	pr_debug("AFE_DL_SDM_CON0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_CON0));
	pr_debug("AFE_DL_SDM_CON1	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_CON1));
	pr_debug("AFE_UL_SRC_CON0_H	       = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC_CON0_H));
	pr_debug("AFE_UL_SRC_CON0_L	       = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC_CON0_L));
	pr_debug("AFE_UL_SRC_CON1_H	       = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC_CON1_H));
	pr_debug("AFE_UL_SRC_CON1_L	       = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC_CON1_L));
	pr_debug("AFE_TOP_CON0		       = 0x%x\n", Ana_Get_Reg(PMIC_AFE_TOP_CON0));
	pr_debug("AUDIO_TOP_CON0	       = 0x%x\n", Ana_Get_Reg(AFE_AUDIO_TOP_CON0));
	pr_debug("AFE_DL_SRC_MON0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC_MON0));
	pr_debug("AFE_DL_SDM_TEST0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_TEST0));
	pr_debug("AFE_MON_DEBUG0	       = 0x%x\n", Ana_Get_Reg(AFE_MON_DEBUG0));
	pr_debug("AFUNC_AUD_CON0	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON0));
	pr_debug("AFUNC_AUD_CON1	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON1));
	pr_debug("AFUNC_AUD_CON2	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON2));
	pr_debug("AFUNC_AUD_CON3	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON3));
	pr_debug("AFUNC_AUD_CON4	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON4));
	pr_debug("AFUNC_AUD_MON0	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_MON0));
	pr_debug("AFUNC_AUD_MON1	       = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_MON1));
	pr_debug("AUDRC_TUNE_MON0	       = 0x%x\n", Ana_Get_Reg(AUDRC_TUNE_MON0));
	pr_debug("AFE_UP8X_FIFO_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_CFG0));
	pr_debug("AFE_UP8X_FIFO_LOG_MON0       = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON0));
	pr_debug("AFE_UP8X_FIFO_LOG_MON1       = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON1));
	pr_debug("AFE_DL_DC_COMP_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG0));
	pr_debug("AFE_DL_DC_COMP_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG1));
	pr_debug("AFE_DL_DC_COMP_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG2));
	pr_debug("AFE_PMIC_NEWIF_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG0));
	pr_debug("AFE_PMIC_NEWIF_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG1));
	pr_debug("AFE_PMIC_NEWIF_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG2));
	pr_debug("AFE_PMIC_NEWIF_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG3));
	pr_debug("AFE_SGEN_CFG0		       = 0x%x\n", Ana_Get_Reg(AFE_SGEN_CFG0));
	pr_debug("AFE_SGEN_CFG1		       = 0x%x\n", Ana_Get_Reg(AFE_SGEN_CFG1));
	pr_debug("AFE_VOW_TOP		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_TOP));
	pr_debug("AFE_VOW_CFG0		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG0));
	pr_debug("AFE_VOW_CFG1		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG1));
	pr_debug("AFE_VOW_CFG2		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG2));
	pr_debug("AFE_VOW_CFG3		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG3));
	pr_debug("AFE_VOW_CFG4		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG4));
	pr_debug("AFE_VOW_CFG5		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG5));
	pr_debug("AFE_VOW_CFG6		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG6));
	pr_debug("AFE_VOW_MON0		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON0));
	pr_debug("AFE_VOW_MON1		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON1));
	pr_debug("AFE_VOW_MON2		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON2));
	pr_debug("AFE_VOW_MON3		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON3));
	pr_debug("AFE_VOW_SN_INI_CFG	   = 0x%x\n", Ana_Get_Reg(AFE_VOW_SN_INI_CFG));
	pr_debug("AFE_VOW_TGEN_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_VOW_TGEN_CFG0));
	pr_debug("AFE_VOW_POSDIV_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_VOW_POSDIV_CFG0));
	pr_debug("AFE_VOW_HPF_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_VOW_HPF_CFG0));
	pr_debug("AFE_DCCLK_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_DCCLK_CFG0));
	pr_debug("AFE_DCCLK_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_DCCLK_CFG1));
	pr_debug("AFE_NCP_CFG0		       = 0x%x\n", Ana_Get_Reg(AFE_NCP_CFG0));
	pr_debug("AFE_NCP_CFG1		       = 0x%x\n", Ana_Get_Reg(AFE_NCP_CFG1));
	pr_debug("AFE_VOW_MON5		       = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON5));
	pr_debug("AFE_VOW_PERIODIC_CFG0        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG0));
	pr_debug("AFE_VOW_PERIODIC_CFG1        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG1));
	pr_debug("AFE_VOW_PERIODIC_CFG2        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG2));
	pr_debug("AFE_VOW_PERIODIC_CFG3        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG3));
	pr_debug("AFE_VOW_PERIODIC_CFG4        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG4));
	pr_debug("AFE_VOW_PERIODIC_CFG5        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG5));
	pr_debug("AFE_VOW_PERIODIC_CFG6        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG6));
	pr_debug("AFE_VOW_PERIODIC_CFG7        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG7));
	pr_debug("AFE_VOW_PERIODIC_CFG8        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG8));
	pr_debug("AFE_VOW_PERIODIC_CFG9        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG9));
	pr_debug("AFE_VOW_PERIODIC_CFG10       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG10));
	pr_debug("AFE_VOW_PERIODIC_CFG11       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG11));
	pr_debug("AFE_VOW_PERIODIC_CFG12       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG12));
	pr_debug("AFE_VOW_PERIODIC_CFG13       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG13));
	pr_debug("AFE_VOW_PERIODIC_CFG14       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG14));
	pr_debug("AFE_VOW_PERIODIC_CFG15       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG15));
	pr_debug("AFE_VOW_PERIODIC_CFG16       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG16));
	pr_debug("AFE_VOW_PERIODIC_CFG17       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG17));
	pr_debug("AFE_VOW_PERIODIC_CFG18       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG18));
	pr_debug("AFE_VOW_PERIODIC_CFG19       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG19));
	pr_debug("AFE_VOW_PERIODIC_CFG20       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG20));
	pr_debug("AFE_VOW_PERIODIC_CFG21       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG21));
	pr_debug("AFE_VOW_PERIODIC_CFG22       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG22));
	pr_debug("AFE_VOW_PERIODIC_CFG23       = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_CFG23));
	pr_debug("AFE_VOW_PERIODIC_MON0        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_MON0));
	pr_debug("AFE_VOW_PERIODIC_MON1        = 0x%x\n", Ana_Get_Reg(AFE_VOW_PERIODIC_MON1));
	pr_debug("AFE_NCP_CFG2		       = 0x%x\n", Ana_Get_Reg(AFE_NCP_CFG2));
	pr_debug("AFE_DL_NLE_R_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG0));
	pr_debug("AFE_DL_NLE_R_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG1));
	pr_debug("AFE_DL_NLE_R_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG2));
	pr_debug("AFE_DL_NLE_R_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG3));
	pr_debug("AFE_DL_NLE_L_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG0));
	pr_debug("AFE_DL_NLE_L_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG1));
	pr_debug("AFE_DL_NLE_L_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG2));
	pr_debug("AFE_DL_NLE_L_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG3));
	pr_debug("AFE_RGS_NLE_R_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG0));
	pr_debug("AFE_RGS_NLE_R_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG1));
	pr_debug("AFE_RGS_NLE_R_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG2));
	pr_debug("AFE_RGS_NLE_R_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG3));
	pr_debug("AFE_RGS_NLE_L_CFG0	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG0));
	pr_debug("AFE_RGS_NLE_L_CFG1	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG1));
	pr_debug("AFE_RGS_NLE_L_CFG2	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG2));
	pr_debug("AFE_RGS_NLE_L_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG3));
	pr_debug("AUD_TOP_CFG		       = 0x%x\n", Ana_Get_Reg(AUD_TOP_CFG));
	pr_debug("AFE_DL_DC_COMP_CFG3	       = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG3));
	pr_debug("AFE_DL_DC_COMP_CFG4	       = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG4));
	pr_debug("AUDDEC_ANA_CON0  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON0));
	pr_debug("AUDDEC_ANA_CON1  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON1));
	pr_debug("AUDDEC_ANA_CON2  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON2));
	pr_debug("AUDDEC_ANA_CON3  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON3));
	pr_debug("AUDDEC_ANA_CON4  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON4));
	pr_debug("AUDDEC_ANA_CON5  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON5));
	pr_debug("AUDDEC_ANA_CON6  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON6));
	pr_debug("AUDDEC_ANA_CON7  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON7));
	pr_debug("AUDDEC_ANA_CON8  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON8));
	pr_debug("AUDDEC_ANA_CON9  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON9));
	pr_debug("AUDDEC_ANA_CON10 = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON10));
	pr_debug("AUDDEC_ANA_CON11 = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON11));
	pr_debug("AUDDEC_ANA_CON12 = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON12));
	pr_debug("AUDDEC_ANA_CON13 = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON13));
	pr_debug("AUDDEC_ANA_CON14 = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON14));
	pr_debug("AUDENC_ANA_CON0  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON0));
	pr_debug("AUDENC_ANA_CON1  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON1));
	pr_debug("AUDENC_ANA_CON2  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON2));
	pr_debug("AUDENC_ANA_CON3  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON3));
	pr_debug("AUDENC_ANA_CON4  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON4));
	pr_debug("AUDENC_ANA_CON5  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON5));
	pr_debug("AUDENC_ANA_CON6  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON6));
	pr_debug("AUDENC_ANA_CON7  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON7));
	pr_debug("AUDENC_ANA_CON8  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON8));
	pr_debug("AUDENC_ANA_CON9  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON9));
	pr_debug("AUDENC_ANA_CON10 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON10));
	pr_debug("AUDENC_ANA_CON11 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON11));
	pr_debug("AUDENC_ANA_CON12 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON12));
	pr_debug("AUDENC_ANA_CON13 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON13));
	pr_debug("AUDENC_ANA_CON14 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON14));
	pr_debug("AUDENC_ANA_CON15 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON15));
	pr_debug("AUDENC_ANA_CON16 = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON16));
	pr_debug("ZCD_CON0 = 0x%x\n", Ana_Get_Reg(ZCD_CON0));
	pr_debug("ZCD_CON1 = 0x%x\n", Ana_Get_Reg(ZCD_CON1));
	pr_debug("ZCD_CON2 = 0x%x\n", Ana_Get_Reg(ZCD_CON2));
	pr_debug("ZCD_CON3 = 0x%x\n", Ana_Get_Reg(ZCD_CON3));
	pr_debug("ZCD_CON4 = 0x%x\n", Ana_Get_Reg(ZCD_CON4));
	pr_debug("ZCD_CON5 = 0x%x\n", Ana_Get_Reg(ZCD_CON5));
	pr_debug("TOP_CLKSQ = 0x%x\n", Ana_Get_Reg(TOP_CLKSQ));
	pr_debug("TOP_CKPDN_CON0 = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON0));
	pr_debug("TOP_CKPDN_CON3 = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON3));
	pr_debug("GPIO_MODE2 = 0x%x\n", Ana_Get_Reg(GPIO_MODE2));
	pr_debug("LDO_VA18_CON0 = 0x%x\n", Ana_Get_Reg(LDO_VA18_CON0));
	pr_debug("LDO_VA18_CON1 = 0x%x\n", Ana_Get_Reg(LDO_VA18_CON1));
	pr_debug("LDO_VA18_CON2 = 0x%x\n", Ana_Get_Reg(LDO_VA18_CON2));
	pr_debug("LDO_VA18_CON3 = 0x%x\n", Ana_Get_Reg(LDO_VA18_CON3));
	pr_debug("DCXO_CW14 = 0x%x\n", Ana_Get_Reg(DCXO_CW14));
	pr_debug("ACCDET_CON14 = 0x%x\n", Ana_Get_Reg(ACCDET_CON14));
	pr_debug("AUXADC_IMPEDANCE = 0x%x\n", Ana_Get_Reg(AUXADC_IMPEDANCE));
	pr_debug("AUXADC_CON2 = 0x%x\n", Ana_Get_Reg(AUXADC_CON2));
	pr_debug("RG_BUCK_VS1_VOTER_EN = 0x%x\n", Ana_Get_Reg(RG_BUCK_VS1_VOTER_EN));
	pr_debug("-Ana_Log_Print\n");
}
EXPORT_SYMBOL(Ana_Log_Print);

int Ana_Debug_Read(char *buffer, const int size)
{
	int n = 0;

	n += scnprintf(buffer + n, size - n, "AFE_UL_DL_CON0 = 0x%x\n",
			Ana_Get_Reg(AFE_UL_DL_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC2_CON0_H = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SRC2_CON0_H));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC2_CON0_L = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SRC2_CON0_L));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_CON0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SDM_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_CON1 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SDM_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC_CON0_H = 0x%x\n",
			Ana_Get_Reg(AFE_UL_SRC_CON0_H));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC_CON0_L = 0x%x\n",
			Ana_Get_Reg(AFE_UL_SRC_CON0_L));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC_CON1_H = 0x%x\n",
			Ana_Get_Reg(AFE_UL_SRC_CON1_H));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC_CON1_L = 0x%x\n",
			Ana_Get_Reg(AFE_UL_SRC_CON1_L));
	n += scnprintf(buffer + n, size - n, "AFE_TOP_CON0 = 0x%x\n",
			Ana_Get_Reg(PMIC_AFE_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON0 = 0x%x\n",
			Ana_Get_Reg(AFE_AUDIO_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC_MON0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SRC_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_TEST0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_SDM_TEST0));
	n += scnprintf(buffer + n, size - n, "AFE_MON_DEBUG0 = 0x%x\n",
			Ana_Get_Reg(AFE_MON_DEBUG0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON0 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_CON0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON1 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_CON1));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON2 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_CON2));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON3 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_CON3));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON4 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_CON4));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_MON0 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_MON0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_MON1 = 0x%x\n",
			Ana_Get_Reg(AFUNC_AUD_MON1));
	n += scnprintf(buffer + n, size - n, "AUDRC_TUNE_MON0 = 0x%x\n",
			Ana_Get_Reg(AUDRC_TUNE_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_UP8X_FIFO_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_LOG_MON0 = 0x%x\n",
			Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_LOG_MON1 = 0x%x\n",
			Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_DC_COMP_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_DC_COMP_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_DC_COMP_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_PMIC_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_PMIC_NEWIF_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_PMIC_NEWIF_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_PMIC_NEWIF_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_SGEN_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_SGEN_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_TOP = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_TOP));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG4 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG4));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG5 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG5));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG6 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_CFG6));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON1 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON2 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON3 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON3));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON4 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON4));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_SN_INI_CFG = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_SN_INI_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_TGEN_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_TGEN_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_POSDIV_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_POSDIV_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_HPF_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_HPF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DCCLK_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_DCCLK_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DCCLK_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_DCCLK_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_NCP_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_NCP_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_NCP_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_NCP_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON5 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_MON5));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG4 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG4));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG5 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG5));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG6 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG6));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG7 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG7));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG8 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG8));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG9 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG9));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG10 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG10));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG11 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG11));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG12 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG12));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG13 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG13));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG14 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG14));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG15 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG15));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG16 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG16));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG17 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG17));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG18 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG18));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG19 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG19));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG20 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG20));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG21 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG21));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG22 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG22));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_CFG23 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_CFG23));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_MON0 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_PERIODIC_MON1 = 0x%x\n",
			Ana_Get_Reg(AFE_VOW_PERIODIC_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_NCP_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_NCP_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_R_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_R_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_R_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_R_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_R_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_R_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_R_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_R_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_L_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_L_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_L_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_L_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_L_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_L_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_DL_NLE_L_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_NLE_L_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_R_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_R_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_R_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_R_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_R_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_R_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_R_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_R_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_L_CFG0 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_L_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_L_CFG1 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_L_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_L_CFG2 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_L_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_RGS_NLE_L_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_RGS_NLE_L_CFG3));
	n += scnprintf(buffer + n, size - n, "AUD_TOP_CFG = 0x%x\n",
			Ana_Get_Reg(AUD_TOP_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG3 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_DC_COMP_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG4 = 0x%x\n",
			Ana_Get_Reg(AFE_DL_DC_COMP_CFG4));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON0 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON0));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON1 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON1));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON2 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON2));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON3 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON3));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON4 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON4));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON5 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON5));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON6 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON6));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON7 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON7));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON8 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON8));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON9 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON9));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON10 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON10));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON11 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON11));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON12 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON12));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON13 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON13));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON14 = 0x%x\n",
			Ana_Get_Reg(AUDDEC_ANA_CON14));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON0 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON0));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON1 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON1));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON2 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON2));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON3 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON3));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON4 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON4));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON5 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON5));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON6 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON6));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON7 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON7));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON8 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON8));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON9 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON9));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON10 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON10));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON11 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON11));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON12 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON12));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON13 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON13));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON14 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON14));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON15 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON15));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON16 = 0x%x\n",
			Ana_Get_Reg(AUDENC_ANA_CON16));
	n += scnprintf(buffer + n, size - n, "ZCD_CON0 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON0));
	n += scnprintf(buffer + n, size - n, "ZCD_CON1 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON1));
	n += scnprintf(buffer + n, size - n, "ZCD_CON2 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON2));
	n += scnprintf(buffer + n, size - n, "ZCD_CON3 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON3));
	n += scnprintf(buffer + n, size - n, "ZCD_CON4 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON4));
	n += scnprintf(buffer + n, size - n, "ZCD_CON5 = 0x%x\n",
			Ana_Get_Reg(ZCD_CON5));
	n += scnprintf(buffer + n, size - n, "TOP_CLKSQ = 0x%x\n",
		       Ana_Get_Reg(TOP_CLKSQ));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON3  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON3));
	n += scnprintf(buffer + n, size - n, "GPIO_MODE2  = 0x%x\n",
		       Ana_Get_Reg(GPIO_MODE2));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON0  = 0x%x\n",
		       Ana_Get_Reg(LDO_VA18_CON0));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON1  = 0x%x\n",
		       Ana_Get_Reg(LDO_VA18_CON1));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON2  = 0x%x\n",
		       Ana_Get_Reg(LDO_VA18_CON2));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON3  = 0x%x\n",
		       Ana_Get_Reg(LDO_VA18_CON3));
	n += scnprintf(buffer + n, size - n, "DCXO_CW14  = 0x%x\n",
		       Ana_Get_Reg(DCXO_CW14));
	n += scnprintf(buffer + n, size - n, "ACCDET_CON14  = 0x%x\n",
		       Ana_Get_Reg(ACCDET_CON14));
	n += scnprintf(buffer + n, size - n, "AUXADC_IMPEDANCE  = 0x%x\n",
		       Ana_Get_Reg(AUXADC_IMPEDANCE));
	n += scnprintf(buffer + n, size - n, "AUXADC_CON2  = 0x%x\n",
		       Ana_Get_Reg(AUXADC_CON2));
	n += scnprintf(buffer + n, size - n, "RG_BUCK_VS1_VOTER_EN  = 0x%x\n",
		       Ana_Get_Reg(RG_BUCK_VS1_VOTER_EN));
	return n;
}
EXPORT_SYMBOL(Ana_Debug_Read);

/* export symbols for other module using */
