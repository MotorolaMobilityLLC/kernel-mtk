/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program
 * If not, see <http://www.gnu.org/licenses/>.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Clk.c
 *
 * Project:
 * --------
 *   MT6757  Audio Driver clock control implement
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang (MTK02308)
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

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#else
#include <mach/mt_clkmgr.h>
#endif

/*#include <mach/mt_pm_ldo.h>*/
/*#include <mach/pmic_mt6325_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>*/

#include "AudDrv_Common.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Afe.h"
#include "mt_soc_digital_type.h"

#include <linux/spinlock.h>
#include <linux/delay.h>
#ifdef _MT_IDLE_HEADER
#include "mt_idle.h"
#include "mt_clk_id.h"
#endif
#include <linux/err.h>
#include <linux/platform_device.h>

/* do not BUG on during FPGA or CCF not ready*/
#ifdef CONFIG_FPGA_EARLY_PORTING
#ifdef BUG
#undef BUG
#define BUG()
#endif
#endif

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

static int APLL1Counter;
static int APLL2Counter;
static int Aud_APLL_DIV_APLL1_cntr;
static int Aud_APLL_DIV_APLL2_cntr;
static unsigned int MCLKFS = 128;
static unsigned int MCLKFS_HDMI = 256;

int Aud_Core_Clk_cntr = 0;
int Aud_AFE_Clk_cntr = 0;
int Aud_I2S_Clk_cntr = 0;
int Aud_ADC_Clk_cntr = 0;
int Aud_ADC2_Clk_cntr = 0;
int Aud_ADC3_Clk_cntr = 0;
int Aud_ANA_Clk_cntr = 0;
#ifdef MT6757_READY
int Aud_HDMI_Clk_cntr = 0;
#endif
int Aud_APLL22M_Clk_cntr = 0;
int Aud_APLL24M_Clk_cntr = 0;
int Aud_APLL1_Tuner_cntr = 0;
int Aud_APLL2_Tuner_cntr = 0;
static int Aud_EMI_cntr;

static DEFINE_SPINLOCK(auddrv_Clk_lock);

/* amp mutex lock */
static DEFINE_MUTEX(auddrv_pmic_mutex);
static DEFINE_MUTEX(audEMI_Clk_mutex);

/* AUDIO_APLL_DIVIDER_GROUP may vary by chip!!! */
typedef enum {
	AUDIO_APLL1_DIV0,
	AUDIO_APLL2_DIV0,
	AUDIO_APLL12_DIV1,
	AUDIO_APLL12_DIV2,
	AUDIO_APLL12_DIV3,
	AUDIO_APLL12_DIV4,
	AUDIO_APLL12_DIVB,
	AUDIO_APLL_DIV_NUM
} AUDIO_APLL_DIVIDER_GROUP;

/* mI2SAPLLDivSelect may vary by chip!!! */
static const uint32 mI2SAPLLDivSelect[Soc_Aud_I2S_CLKDIV_NUMBER] = {
	AUDIO_APLL1_DIV0,
	AUDIO_APLL2_DIV0,
	AUDIO_APLL12_DIV1,
	AUDIO_APLL12_DIV2,
	AUDIO_APLL12_DIV3,
	AUDIO_APLL12_DIV4,
	AUDIO_APLL12_DIVB
};

enum audio_system_clock_type {
	CLOCK_AFE = 0,
	CLOCK_I2S,
	CLOCK_DAC,
	CLOCK_DAC_PREDIS,
	CLOCK_ADC,
	CLOCK_TML,
	CLOCK_APLL22M,
	CLOCK_APLL24M,
	CLOCK_APLL1_TUNER,
	CLOCK_APLL2_TUNER,
	CLOCK_SCP_SYS_AUD,
	CLOCK_INFRA_SYS_AUDIO,
	CLOCK_PERI_AUDIO26M,
	CLOCK_TOP_AUD_MUX1,
	CLOCK_TOP_AUD_MUX2,
	CLOCK_TOP_AD_APLL1_CK,
	CLOCK_TOP_AD_APLL2_CK,
	CLOCK_MUX_AUDIOINTBUS,
	CLOCK_TOP_SYSPLL1_D4,
	CLOCK_APMIXED_APLL1_CK,
	CLOCK_APMIXED_APLL2_CK,
	CLOCK_CLK26M,
	CLOCK_NUM
};

struct audio_clock_attr {
	const char *name;
	bool clk_prepare;
	bool clk_status;
	struct clk *clock;
};

static struct audio_clock_attr aud_clks[CLOCK_NUM] = {
	[CLOCK_AFE] = {"aud_afe_clk", false, false, NULL},
	[CLOCK_I2S] = {"aud_i2s_clk", false, false, NULL},
	[CLOCK_DAC] = {"aud_dac_clk", false, false, NULL},
	[CLOCK_DAC_PREDIS] = {"aud_dac_predis_clk", false, false, NULL},
	[CLOCK_ADC] = {"aud_adc_clk", false, false, NULL},
	[CLOCK_TML] = {"aud_tml_clk", false, false, NULL},
	[CLOCK_APLL22M] = {"aud_apll22m_clk", false, false, NULL},
	[CLOCK_APLL24M] = {"aud_apll24m_clk", false, false, NULL},
	[CLOCK_APLL1_TUNER] = {"aud_apll1_tuner_clk", false, false, NULL},
	[CLOCK_APLL2_TUNER] = {"aud_apll2_tuner_clk", false, false, NULL},
	[CLOCK_SCP_SYS_AUD] = {"scp_sys_aud", false, false, NULL},
	[CLOCK_INFRA_SYS_AUDIO] = {"aud_infra_clk", false, false, NULL},
	[CLOCK_PERI_AUDIO26M] = {"aud_peri_26m_clk", false, false, NULL},
	[CLOCK_TOP_AUD_MUX1] = {"aud_mux1_clk", false, false, NULL},
	[CLOCK_TOP_AUD_MUX2] = {"aud_mux2_clk", false, false, NULL},
	[CLOCK_TOP_AD_APLL1_CK] = {"top_ad_apll1_clk", false, false, NULL},
	[CLOCK_TOP_AD_APLL2_CK] = {"top_ad_apll2_clk", false, false, NULL},
	[CLOCK_MUX_AUDIOINTBUS] = {"top_mux_audio_int", false, false, NULL},
	[CLOCK_TOP_SYSPLL1_D4] = {"top_sys_pll1_d4", false, false, NULL},
	[CLOCK_APMIXED_APLL1_CK] = {"apmixed_apll1_clk", false, false, NULL},
	[CLOCK_APMIXED_APLL2_CK] = {"apmixed_apll2_clk", false, false, NULL},
	[CLOCK_CLK26M] = {"top_clk26m_clk", false, false, NULL}
};

int AudDrv_Clk_probe(void *dev)
{
	size_t i;
	int ret = 0;

	Aud_EMI_cntr = 0;

	pr_warn("%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		aud_clks[i].clock = devm_clk_get(dev, aud_clks[i].name);
		if (IS_ERR(aud_clks[i].clock)) {
			ret = PTR_ERR(aud_clks[i].clock);
			pr_err("%s devm_clk_get %s fail %d\n", __func__, aud_clks[i].name, ret);
			break;
		}
		aud_clks[i].clk_status = true;
	}

	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		if (i == CLOCK_SCP_SYS_AUD)	/* CLOCK_SCP_SYS_AUD is MTCMOS */
			continue;

		if (aud_clks[i].clk_status) {
			ret = clk_prepare(aud_clks[i].clock);
			if (ret) {
				pr_err("%s clk_prepare %s fail %d\n",
				       __func__, aud_clks[i].name, ret);
				break;
			}
			aud_clks[i].clk_prepare = true;
		}
	}

	return ret;
}

void AudDrv_Clk_Deinit(void *dev)
{
	size_t i;

	pr_warn("%s\n", __func__);
	for (i = 0; i < ARRAY_SIZE(aud_clks); i++) {
		if (i == CLOCK_SCP_SYS_AUD)	/* CLOCK_SCP_SYS_AUD is MTCMOS */
			continue;

		if (aud_clks[i].clock && !IS_ERR(aud_clks[i].clock) && aud_clks[i].clk_prepare) {
			clk_unprepare(aud_clks[i].clock);
			aud_clks[i].clk_prepare = false;
		}
	}
}

void AudDrv_Clk_Global_Variable_Init(void)
{
	APLL1Counter = 0;
	APLL2Counter = 0;
	Aud_APLL_DIV_APLL1_cntr = 0;
	Aud_APLL_DIV_APLL2_cntr = 0;
	MCLKFS = 128;
	MCLKFS_HDMI = 256;
}

void AudDrv_Bus_Init(void)
{
	unsigned long flags;

	pr_warn("%s\n", __func__);
	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	Afe_Set_Reg(AUDIO_TOP_CON0, 0x00004000,
		    0x00004000);    /* must set, system will default set bit14 to 0 */
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}


/*****************************************************************************
 * FUNCTION
 *  AudDrv_Clk_Power_On / AudDrv_Clk_Power_Off
 *
 * DESCRIPTION
 *  Power on this function , then all register can be access and  set.
 *
 *****************************************************************************
 */

void AudDrv_Clk_Power_On(void)
{
	/*volatile uint32 *AFE_Register = (volatile uint32 *)Get_Afe_Powertop_Pointer();*/
	volatile uint32 val_tmp;

	pr_warn("%s\n", __func__);
#ifdef MT6757_READY
	val_tmp = 0x3330000d;
#endif
	val_tmp = 0xd;
	/*mt_reg_sync_writel(val_tmp, AFE_Register);*/
}

void AudDrv_Clk_Power_Off(void)
{
}

/*****************************************************************************
 * FUNCTION
 *  AudDrv_Clk_On / AudDrv_Clk_Off
 *
 * DESCRIPTION
 *  Enable/Disable PLL(26M clock) \ AFE clock
 *
 *****************************************************************************
*/

void AudDrv_AUDINTBUS_Sel(int parentidx)
{
	int ret = 0;

	PRINTK_AUD_CLK("+AudDrv_AUDINTBUS_Sel, parentidx = %d\n",
			parentidx);
	if (parentidx == 1) {
		if (aud_clks[CLOCK_MUX_AUDIOINTBUS].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		ret = clk_set_parent(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock,
				     aud_clks[CLOCK_TOP_SYSPLL1_D4].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
			       __func__, aud_clks[CLOCK_MUX_AUDIOINTBUS].name,
			       aud_clks[CLOCK_TOP_SYSPLL1_D4].name, ret);
			BUG();
			goto EXIT;
		}
	} else if (parentidx == 0) {
		if (aud_clks[CLOCK_MUX_AUDIOINTBUS].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		ret = clk_set_parent(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock,
				     aud_clks[CLOCK_CLK26M].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
			       __func__, aud_clks[CLOCK_MUX_AUDIOINTBUS].name,
			       aud_clks[CLOCK_CLK26M].name, ret);
			BUG();
			goto EXIT;
		}
	}
EXIT:
	PRINTK_AUD_CLK("-%s()\n", __func__);
}


/*****************************************************************************
 * FUNCTION
 *  AudDrv_AUD_Sel
 *
 * DESCRIPTION
 *  TOP_MUX_AUDIO select source
 *
 *****************************************************************************
*/

void AudDrv_AUD_Sel(int parentidx)
{
	/* Keep for extension */
}

void AudDrv_Clk_On(void)
{
	unsigned long flags;
	int ret = 0;

	PRINTK_AUD_CLK("+AudDrv_Clk_On, Aud_AFE_Clk_cntr:%d\n", Aud_AFE_Clk_cntr);
	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	Aud_AFE_Clk_cntr++;
	if (Aud_AFE_Clk_cntr == 1) {
#ifdef PM_MANAGER_API
/* MT6755 CLOCK_PERI_AUDIO26M
		if (aud_clks[CLOCK_INFRA_SYS_AUDIO_26M].clk_prepare) {
*/
		/*
		* Clock enable order:
		* 26M -> Infra -> Int bus -> AFE -> DAC -> DAC pre-distortion -> MTCMOS
		*/
		if (aud_clks[CLOCK_PERI_AUDIO26M].clk_prepare) {

			/* MT6755 CLOCK_PERI_AUDIO26M
			ret = clk_enable(aud_clks[CLOCK_INFRA_SYS_AUDIO_26M].clock);
			*/
			ret = clk_enable(aud_clks[CLOCK_PERI_AUDIO26M].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock %s fail\n", __func__,
					/* MT6755 CLOCK_PERI_AUDIO26M
				       aud_clks[CLOCK_INFRA_SYS_AUDIO_26M].name);
					*/
						aud_clks[CLOCK_PERI_AUDIO26M].name);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_INFRA_SYS_AUDIO_26M fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_INFRA_SYS_AUDIO].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_INFRA_SYS_AUDIO].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock %s fail\n", __func__,
						aud_clks[CLOCK_INFRA_SYS_AUDIO].name);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock MT_CG_INFRA_AUDIO fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_MUX_AUDIOINTBUS].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
			PRINTK_AUD_CLK("AudDrv_Clk_On: enable CLOCK_MUX_AUDIOINTBUS\n");
		} else {
			pr_err("%s [CCF]clk_status error Aud enable_clock CLOCK_MUX_AUDIOINTBUS fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_AFE].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_AFE].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock %s fail\n", __func__,
						aud_clks[CLOCK_AFE].name);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock MT_CG_AUDIO_AFE fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_DAC].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_DAC].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock MT_CG_AUDIO_DAC fail\n", __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_status error Aud enable_clock MT_CG_AUDIO_DAC fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_DAC_PREDIS].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_DAC_PREDIS].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock MT_CG_AUDIO_DAC_PREDIS fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_status error Aud enable_clock MT_CG_AUDIO_DAC_PREDIS fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
		/*
		* CLOCK_SCP_SYS_AUD is MTCMOS
		* MTCMOS will be enabled when prepare in CCF API,
		* Don't prepare MTCMOS in probe for low power
		*/
		if (aud_clks[CLOCK_SCP_SYS_AUD].clk_status) {
			ret = clk_prepare_enable(aud_clks[CLOCK_SCP_SYS_AUD].clock);
			if (ret) {
				pr_err("%s [CCF]Aud clk_prepare_enable %s fail\n", __func__,
						aud_clks[CLOCK_SCP_SYS_AUD].name);
				BUG();
				goto EXIT_SKIP_UNLOCK;
			}
		}

		goto EXIT_SKIP_UNLOCK;
#else
		SetInfraCfg(AUDIO_CG_CLR, 0x2000000, 0x2000000);
		/* bit 25=0, without 133m master and 66m slave bus clock cg gating */
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x4000, 0x06004044);
#endif
	}
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
EXIT_SKIP_UNLOCK:
	PRINTK_AUD_CLK("-AudDrv_Clk_On, Aud_AFE_Clk_cntr:%d\n", Aud_AFE_Clk_cntr);
}
EXPORT_SYMBOL(AudDrv_Clk_On);

void AudDrv_Clk_Off(void)
{
	unsigned long flags;

	PRINTK_AUD_CLK("+!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr:%d\n", Aud_AFE_Clk_cntr);
	spin_lock_irqsave(&auddrv_Clk_lock, flags);

	Aud_AFE_Clk_cntr--;
	if (Aud_AFE_Clk_cntr == 0) {
		/* Disable AFE clock */
#ifdef PM_MANAGER_API
		/* Make sure all IRQ status is cleared */
		Afe_Set_Reg(AFE_IRQ_MCU_CLR, 0xffff, 0xffff);

		/*
		* Clock disable order:
		* MTCMOS -> DAC pre-distortion -> DAC -> AFE -> Int bus -> Infra -> 26M
		*/

		/* CLOCK_SCP_SYS_AUD is MTCMOS */
		spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
		if (aud_clks[CLOCK_SCP_SYS_AUD].clk_status)
			clk_disable_unprepare(aud_clks[CLOCK_SCP_SYS_AUD].clock);
		spin_lock_irqsave(&auddrv_Clk_lock, flags);

		if (aud_clks[CLOCK_DAC_PREDIS].clk_prepare)
			clk_disable(aud_clks[CLOCK_DAC_PREDIS].clock);

		if (aud_clks[CLOCK_DAC].clk_prepare)
			clk_disable(aud_clks[CLOCK_DAC].clock);

		if (aud_clks[CLOCK_AFE].clk_prepare)
			clk_disable(aud_clks[CLOCK_AFE].clock);

		if (aud_clks[CLOCK_MUX_AUDIOINTBUS].clk_prepare) {
			clk_disable(aud_clks[CLOCK_MUX_AUDIOINTBUS].clock);
		}
		if (aud_clks[CLOCK_INFRA_SYS_AUDIO].clk_prepare)
			clk_disable(aud_clks[CLOCK_INFRA_SYS_AUDIO].clock);

		if (aud_clks[CLOCK_PERI_AUDIO26M].clk_prepare)
			clk_disable(aud_clks[CLOCK_PERI_AUDIO26M].clock);
#else
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x06000044, 0x06000044);
		SetInfraCfg(AUDIO_CG_SET, 0x2000000, 0x2000000);
		/* bit25=1, with 133m mastesr and 66m slave bus clock cg gating */
#endif
	} else if (Aud_AFE_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr<0 (%d)\n",
				Aud_AFE_Clk_cntr);
		AUDIO_ASSERT(true);
		Aud_AFE_Clk_cntr = 0;
	}
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
	PRINTK_AUD_CLK("-!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr:%d\n", Aud_AFE_Clk_cntr);
}
EXPORT_SYMBOL(AudDrv_Clk_Off);


/*****************************************************************************
 * FUNCTION
 *  AudDrv_ANA_Clk_On / AudDrv_ANA_Clk_Off
 *
 * DESCRIPTION
 *  Enable/Disable analog part clock
 *
 *****************************************************************************/
void AudDrv_ANA_Clk_On(void)
{
	mutex_lock(&auddrv_pmic_mutex);
	if (Aud_ANA_Clk_cntr == 0)
		PRINTK_AUD_CLK("+AudDrv_ANA_Clk_On, Aud_ANA_Clk_cntr:%d\n", Aud_ANA_Clk_cntr);

	Aud_ANA_Clk_cntr++;
	mutex_unlock(&auddrv_pmic_mutex);
	/* PRINTK_AUD_CLK("-AudDrv_ANA_Clk_Off, Aud_ANA_Clk_cntr:%d\n",Aud_ANA_Clk_cntr); */
}
EXPORT_SYMBOL(AudDrv_ANA_Clk_On);

void AudDrv_ANA_Clk_Off(void)
{
	/* PRINTK_AUD_CLK("+AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr:%d\n",  Aud_ANA_Clk_cntr); */
	mutex_lock(&auddrv_pmic_mutex);
	Aud_ANA_Clk_cntr--;
	if (Aud_ANA_Clk_cntr == 0) {
		PRINTK_AUD_CLK("+AudDrv_ANA_Clk_Off disable_clock Ana clk(%x)\n",
			       Aud_ANA_Clk_cntr);
		/* Disable ADC clock */
#ifdef PM_MANAGER_API

#else
		/* TODO:: open ADC clock.... */
#endif
	} else if (Aud_ANA_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr<0 (%d)\n",
				 Aud_ANA_Clk_cntr);
		AUDIO_ASSERT(true);
		Aud_ANA_Clk_cntr = 0;
	}
	mutex_unlock(&auddrv_pmic_mutex);
	/* PRINTK_AUD_CLK("-AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr:%d\n", Aud_ANA_Clk_cntr); */
}
EXPORT_SYMBOL(AudDrv_ANA_Clk_Off);

/*****************************************************************************
 * FUNCTION
  *  AudDrv_ADC_Clk_On / AudDrv_ADC_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/

void AudDrv_ADC_Clk_On(void)
{
	/* PRINTK_AUDDRV("+AudDrv_ADC_Clk_On, Aud_ADC_Clk_cntr:%d\n", Aud_ADC_Clk_cntr); */
	int ret = 0;

	mutex_lock(&auddrv_pmic_mutex);

	if (Aud_ADC_Clk_cntr == 0) {
		PRINTK_AUDDRV("+AudDrv_ADC_Clk_On enable_clock ADC clk(%x)\n",
			      Aud_ADC_Clk_cntr);
		/* Afe_Set_Reg(AUDIO_TOP_CON0, 0 << 24 , 1 << 24); */
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_ADC].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_ADC].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock ADC fail", __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock ADC fail", __func__);
			BUG();
			goto EXIT;
		}
#else
		Afe_Set_Reg(AUDIO_TOP_CON0, 0 << 24, 1 << 24);
#endif
	}
	Aud_ADC_Clk_cntr++;
EXIT:
	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_ADC_Clk_Off(void)
{
	/* PRINTK_AUDDRV("+AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d\n", Aud_ADC_Clk_cntr); */
	mutex_lock(&auddrv_pmic_mutex);
	Aud_ADC_Clk_cntr--;
	if (Aud_ADC_Clk_cntr == 0) {
		PRINTK_AUDDRV("+AudDrv_ADC_Clk_On disable_clock ADC clk(%x)\n",
			      Aud_ADC_Clk_cntr);
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_ADC].clk_prepare)
			clk_disable(aud_clks[CLOCK_ADC].clock);
#else
		Afe_Set_Reg(AUDIO_TOP_CON0, 1 << 24, 1 << 24);
#endif
	}
	if (Aud_ADC_Clk_cntr < 0) {
		PRINTK_AUDDRV("!! AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr<0 (%d)\n",
			      Aud_ADC_Clk_cntr);
		Aud_ADC_Clk_cntr = 0;
	}
	mutex_unlock(&auddrv_pmic_mutex);
	/* PRINTK_AUDDRV("-AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d\n", Aud_ADC_Clk_cntr); */
}

/*****************************************************************************
 * FUNCTION
  *  AudDrv_ADC2_Clk_On / AudDrv_ADC2_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable clock
  *
  *****************************************************************************/

void AudDrv_ADC2_Clk_On(void)
{
	PRINTK_AUD_CLK("+%s %d\n", __func__, Aud_ADC2_Clk_cntr);
	mutex_lock(&auddrv_pmic_mutex);

	if (Aud_ADC2_Clk_cntr == 0)
		PRINTK_AUDDRV("+%s  enable_clock ADC2 clk(%x)\n", __func__, Aud_ADC2_Clk_cntr);

	Aud_ADC2_Clk_cntr++;

	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_ADC2_Clk_Off(void)
{
	/* PRINTK_AUDDRV("+%s %d\n", __func__,Aud_ADC2_Clk_cntr); */
	mutex_lock(&auddrv_pmic_mutex);
	Aud_ADC2_Clk_cntr--;
	if (Aud_ADC2_Clk_cntr == 0)
		PRINTK_AUDDRV("+%s disable_clock ADC clk(%x)\n", __func__, Aud_ADC2_Clk_cntr);


	if (Aud_ADC2_Clk_cntr < 0) {
		PRINTK_AUDDRV("%s  <0 (%d)\n", __func__, Aud_ADC2_Clk_cntr);
		Aud_ADC2_Clk_cntr = 0;
	}
	mutex_unlock(&auddrv_pmic_mutex);
	/* PRINTK_AUDDRV("-AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d\n", Aud_ADC_Clk_cntr); */
}


/*****************************************************************************
 * FUNCTION
  *  AudDrv_ADC3_Clk_On / AudDrv_ADC3_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable clock
  *
  *****************************************************************************/

void AudDrv_ADC3_Clk_On(void)
{
	PRINTK_AUD_CLK("+%s %d\n", __func__, Aud_ADC3_Clk_cntr);
	mutex_lock(&auddrv_pmic_mutex);

	if (Aud_ADC3_Clk_cntr == 0)
		PRINTK_AUDDRV("+%s  enable_clock ADC clk(%x)\n", __func__, Aud_ADC3_Clk_cntr);

	Aud_ADC3_Clk_cntr++;
	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_ADC3_Clk_Off(void)
{
	/* PRINTK_AUDDRV("+%s %d\n", __func__,Aud_ADC2_Clk_cntr); */
	mutex_lock(&auddrv_pmic_mutex);
	Aud_ADC3_Clk_cntr--;

	if (Aud_ADC3_Clk_cntr == 0)
		PRINTK_AUDDRV("+%s disable_clock ADC clk(%x)\n", __func__, Aud_ADC3_Clk_cntr);


	if (Aud_ADC3_Clk_cntr < 0) {
		PRINTK_AUDDRV("%s  <0 (%d)\n", __func__, Aud_ADC3_Clk_cntr);
		Aud_ADC3_Clk_cntr = 0;
	}

	mutex_unlock(&auddrv_pmic_mutex);
	/* PRINTK_AUDDRV("-AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d\n", Aud_ADC_Clk_cntr); */
}

/*****************************************************************************
 * FUNCTION
  *  AudDrv_ADC_Hires_Clk_On / AudDrv_ADC_Hires_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/

void AudDrv_ADC_Hires_Clk_On(void)
{
	/* No Hires Clk in mt6757 */
}

void AudDrv_ADC_Hires_Clk_Off(void)
{
	/* No Hires Clk in mt6757 */
}


/*****************************************************************************
 * FUNCTION
  *  AudDrv_APLL22M_Clk_On / AudDrv_APLL22M_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable clock
  *
  *****************************************************************************/

void AudDrv_APLL22M_Clk_On(void)
{
	int ret = 0;

	mutex_lock(&auddrv_pmic_mutex);

	PRINTK_AUD_CLK("+%s counter = %d\n",
			__func__, Aud_APLL22M_Clk_cntr);

	if (Aud_APLL22M_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		/* pdn_aud_1 => power down hf_faud_1_ck, hf_faud_1_ck is mux of 26M and APLL1_CK */
		/* pdn_aud_2 => power down hf_faud_2_ck, hf_faud_2_ck is mux of 26M and APLL2_CK (D1 is WHPLL) */

		if (aud_clks[CLOCK_TOP_AD_APLL1_CK].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock CLOCK_TOP_AD_APLL1_CK fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud CLOCK_TOP_AD_APLL1_CK fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AUD_MUX1].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_TOP_AUD_MUX1].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock CLOCK_TOP_AUD_MUX1 fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_TOP_AUD_MUX1 fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX1].clock,
				aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
					__func__, aud_clks[CLOCK_TOP_AUD_MUX1].name,
					aud_clks[CLOCK_TOP_AD_APLL1_CK].name, ret);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_APMIXED_APLL1_CK].clk_prepare) {

			ret = clk_set_rate(aud_clks[CLOCK_APMIXED_APLL1_CK].clock, 180633600);
			if (ret) {
				pr_err("%s clk_set_rate %s-180633600 fail %d\n",
						__func__, aud_clks[CLOCK_APMIXED_APLL1_CK].name, ret);
				BUG();
				goto EXIT;
			}
		}

		if (aud_clks[CLOCK_APLL22M].clk_prepare) {

			ret = clk_enable(aud_clks[CLOCK_APLL22M].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock aud_apll22m_clk fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll22m_clk fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_APLL1_TUNER].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock aud_apll1_tuner_clk fail\n",
						__func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll1_tuner_clk fail\n",
					__func__);
			BUG();
			goto EXIT;
		}
#endif
	}
	Aud_APLL22M_Clk_cntr++;
EXIT:

	PRINTK_AUD_CLK("-%s: counter = %d\n",
			__func__, Aud_APLL22M_Clk_cntr);

	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_APLL22M_Clk_Off(void)
{
	int ret = 0;

	mutex_lock(&auddrv_pmic_mutex);

	PRINTK_AUD_CLK("+%s: counter = %d\n",
			__func__, Aud_APLL22M_Clk_cntr);

	Aud_APLL22M_Clk_cntr--;

	if (Aud_APLL22M_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_APLL22M].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL22M].clock);

		if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL1_TUNER].clock);

		ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX1].clock,
				aud_clks[CLOCK_CLK26M].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
					__func__, aud_clks[CLOCK_TOP_AUD_MUX1].name,
					aud_clks[CLOCK_CLK26M].name, ret);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AUD_MUX1].clk_prepare) {
			clk_disable(aud_clks[CLOCK_TOP_AUD_MUX1].clock);
			pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AUD_MUX1\n",
					__func__);
		} else {
			pr_err("%s [CCF]clk_prepare error clk_disable CLOCK_TOP_AUD_MUX1 fail\n",
					__func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AD_APLL1_CK].clk_prepare) {
			clk_disable(aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
			pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AD_APLL1_CK\n",
					__func__);

		} else {
			pr_err("%s [CCF]clk_prepare error CLOCK_TOP_AD_APLL1_CK fail\n",
					__func__);
			BUG();
			goto EXIT;
		}
#endif
	}

EXIT:
	if (Aud_APLL22M_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("err, %s <0 (%d)\n", __func__,
				Aud_APLL22M_Clk_cntr);
		Aud_APLL22M_Clk_cntr = 0;
	}

	PRINTK_AUD_CLK("-%s: counter = %d\n",
			__func__, Aud_APLL22M_Clk_cntr);

	mutex_unlock(&auddrv_pmic_mutex);

}


/*****************************************************************************
 * FUNCTION
  *  AudDrv_APLL24M_Clk_On / AudDrv_APLL24M_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable clock
  *
  *****************************************************************************/

void AudDrv_APLL24M_Clk_On(void)
{
	int ret = 0;

	PRINTK_AUD_CLK("+%s counter = %d\n", __func__, Aud_APLL24M_Clk_cntr);

	mutex_lock(&auddrv_pmic_mutex);

	if (Aud_APLL24M_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_TOP_AD_APLL2_CK].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock CLOCK_TOP_AD_APLL2_CK fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud CLOCK_TOP_AD_APLL2_CK fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AUD_MUX2].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_TOP_AUD_MUX2].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock CLOCK_TOP_AUD_MUX2 fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_TOP_AUD_MUX2 fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX2].clock,
				     aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
			       __func__, aud_clks[CLOCK_TOP_AUD_MUX2].name,
			       aud_clks[CLOCK_TOP_AD_APLL2_CK].name, ret);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_APMIXED_APLL2_CK].clk_prepare) {

			ret = clk_set_rate(aud_clks[CLOCK_APMIXED_APLL2_CK].clock, 196607998);
			if (ret) {
				pr_err("%s clk_set_rate %s-196607998 fail %d\n",
				       __func__, aud_clks[CLOCK_APMIXED_APLL2_CK].name, ret);
				BUG();
				goto EXIT;
			}
		}

		if (aud_clks[CLOCK_APLL24M].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_APLL24M].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock aud_apll24m_clk fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll24m_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_APLL2_TUNER].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock aud_apll2_tuner_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll2_tuner_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}
#endif
	}
	Aud_APLL24M_Clk_cntr++;
EXIT:
	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_APLL24M_Clk_Off(void)
{
	int ret = 0;

	PRINTK_AUD_CLK("+%s counter = %d\n", __func__, Aud_APLL24M_Clk_cntr);

	mutex_lock(&auddrv_pmic_mutex);

	Aud_APLL24M_Clk_cntr--;

	if (Aud_APLL24M_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_APLL24M].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL24M].clock);

		if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL2_TUNER].clock);

		ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX2].clock,
				     aud_clks[CLOCK_CLK26M].clock);
		if (ret) {
			pr_err("%s clk_set_parent %s-%s fail %d\n",
			       __func__, aud_clks[CLOCK_TOP_AUD_MUX2].name,
			       aud_clks[CLOCK_CLK26M].name, ret);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AUD_MUX2].clk_prepare) {
			clk_disable(aud_clks[CLOCK_TOP_AUD_MUX2].clock);

			pr_err("%s [CCF]Aud clk_disable CLOCK_TOP_AUD_MUX2 fail\n",
			       __func__);

		} else {
			pr_err
			("%s [CCF]clk_prepare error clk_disable CLOCK_TOP_AUD_MUX2 fail\n",
			 __func__);
			BUG();
			goto EXIT;
		}

		if (aud_clks[CLOCK_TOP_AD_APLL2_CK].clk_prepare) {
			clk_disable(aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
			pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AD_APLL2_CK fail\n",
				 __func__);

		} else {
			pr_err
			("%s [CCF]clk_prepare error CLOCK_TOP_AD_APLL2_CK fail\n",
			 __func__);
			BUG();
			goto EXIT;
		}

#endif
	}
EXIT:
	if (Aud_APLL24M_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("%s <0 (%d)\n", __func__,
				 Aud_APLL24M_Clk_cntr);
		Aud_APLL24M_Clk_cntr = 0;
	}

	mutex_unlock(&auddrv_pmic_mutex);
}

/*****************************************************************************
  * FUNCTION
  *  AudDrv_I2S_Clk_On / AudDrv_I2S_Clk_Off
  *
  * DESCRIPTION
  * Enable I2S In clock (bck)
  * This should be enabled in slave i2s mode.
  *
  *****************************************************************************/
void aud_top_con_pdn_i2s(bool _pdn)
{
	if (_pdn)
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x1 << 6, 0x1 << 6); /* power off I2S clock */
	else
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x0 << 6, 0x1 << 6); /* power on I2S clock */
}

void AudDrv_I2S_Clk_On(void)
{
	unsigned long flags;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);

	if (Aud_I2S_Clk_cntr == 0)
		aud_top_con_pdn_i2s(false);

	Aud_I2S_Clk_cntr++;
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}
EXPORT_SYMBOL(AudDrv_I2S_Clk_On);

void AudDrv_I2S_Clk_Off(void)
{
	unsigned long flags;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	Aud_I2S_Clk_cntr--;
	if (Aud_I2S_Clk_cntr == 0) {
		aud_top_con_pdn_i2s(true);
	} else if (Aud_I2S_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr<0 (%d)\n",
				 Aud_I2S_Clk_cntr);
		AUDIO_ASSERT(true);
		Aud_I2S_Clk_cntr = 0;
	}
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}
EXPORT_SYMBOL(AudDrv_I2S_Clk_Off);

/*****************************************************************************
  * FUNCTION
  *  AudDrv_TDM_Clk_On / AudDrv_TDM_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable TDM clock
  *
  *****************************************************************************/
void aud_top_con_pdn_tdm_ck(bool _pdn)
{
	/* mt6757 no TDM */
}

void AudDrv_TDM_Clk_On(void)
{
	/* mt6757 no TDM */
}
EXPORT_SYMBOL(AudDrv_TDM_Clk_On);

void AudDrv_TDM_Clk_Off(void)
{
	/* mt6757 no TDM */
}
EXPORT_SYMBOL(AudDrv_TDM_Clk_Off);


/*****************************************************************************
  * FUNCTION
  *  AudDrv_Core_Clk_On / AudDrv_Core_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/

void AudDrv_Core_Clk_On(void)
{
	/* PRINTK_AUD_CLK("+AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d\n", Aud_Core_Clk_cntr); */
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_Core_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_AFE].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_AFE].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock aud_afe_clk fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_afe_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}
#endif
	}
	Aud_Core_Clk_cntr++;
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
	/* PRINTK_AUD_CLK("-AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d\n", Aud_Core_Clk_cntr); */
}

void AudDrv_Core_Clk_Off(void)
{
	/* PRINTK_AUD_CLK("+AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d\n", Aud_Core_Clk_cntr); */
	unsigned long flags;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_Core_Clk_cntr == 0) {
#ifdef PM_MANAGER_API
		if (aud_clks[CLOCK_AFE].clk_prepare)
			clk_disable(aud_clks[CLOCK_AFE].clock);
#endif
	}
	Aud_Core_Clk_cntr++;
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
	/* PRINTK_AUD_CLK("-AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d\n", Aud_Core_Clk_cntr); */
}

void AudDrv_APLL1Tuner_Clk_On(void)
{
	unsigned long flags;
#ifndef CONFIG_MTK_CLKMGR
	int ret = 0;
#endif
	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_APLL1_Tuner_cntr == 0) {
		PRINTK_AUD_CLK("+AudDrv_APLLTuner_Clk_On, Aud_APLL1_Tuner_cntr:%d\n",
			       Aud_APLL1_Tuner_cntr);
#ifdef CONFIG_MTK_CLKMGR
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x0 << 19, 0x1 << 19);
#else
		if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_APLL1_TUNER].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock aud_apll1_tuner_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll1_tuner_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}
#endif
	}
	Aud_APLL1_Tuner_cntr++;
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}

void AudDrv_APLL1Tuner_Clk_Off(void)
{
	unsigned long flags;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);

	Aud_APLL1_Tuner_cntr--;
	if (Aud_APLL1_Tuner_cntr == 0) {
#ifdef CONFIG_MTK_CLKMGR
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x1 << 19, 0x1 << 19);
		/*Afe_Set_Reg(AFE_APLL1_TUNER_CFG, 0x00000033, 0x1 << 19);*/
#else
		if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL1_TUNER].clock);
#endif
	}
	/* handle for clock error */
	else if (Aud_APLL1_Tuner_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_APLLTuner_Clk_Off, Aud_APLL1_Tuner_cntr<0 (%d)\n",
				 Aud_APLL1_Tuner_cntr);
		Aud_APLL1_Tuner_cntr = 0;
	}
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}


void AudDrv_APLL2Tuner_Clk_On(void)
{
	unsigned long flags;
#ifndef CONFIG_MTK_CLKMGR
	int ret = 0;
#endif
	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_APLL2_Tuner_cntr == 0) {
		PRINTK_AUD_CLK("+Aud_APLL2_Tuner_cntr, Aud_APLL2_Tuner_cntr:%d\n",
			       Aud_APLL2_Tuner_cntr);
#ifdef CONFIG_MTK_CLKMGR
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x0 << 18, 0x1 << 18);
		/*Afe_Set_Reg(AFE_APLL2_TUNER_CFG, 0x00000033, 0x1 << 19);*/
#else
		if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_APLL2_TUNER].clock);
			if (ret) {
				pr_err
				("%s [CCF]Aud enable_clock enable_clock aud_apll2_tuner_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_apll2_tuner_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}
#endif
	}
	Aud_APLL2_Tuner_cntr++;
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}

void AudDrv_APLL2Tuner_Clk_Off(void)
{
	unsigned long flags;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);

	Aud_APLL2_Tuner_cntr--;

	if (Aud_APLL2_Tuner_cntr == 0) {
#ifdef CONFIG_MTK_CLKMGR
		Afe_Set_Reg(AUDIO_TOP_CON0, 0x1 << 18, 0x1 << 18);
#else
		if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare)
			clk_disable(aud_clks[CLOCK_APLL2_TUNER].clock);
#endif
		PRINTK_AUD_CLK("AudDrv_APLL2Tuner_Clk_Off\n");
	}
	/* handle for clock error */
	else if (Aud_APLL2_Tuner_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_APLL2Tuner_Clk_Off, Aud_APLL1_Tuner_cntr<0 (%d)\n",
				 Aud_APLL2_Tuner_cntr);
		Aud_APLL2_Tuner_cntr = 0;
	}
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}

/*****************************************************************************
  * FUNCTION
  *  AudDrv_HDMI_Clk_On / AudDrv_HDMI_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/

void AudDrv_HDMI_Clk_On(void)
{
#ifdef _NON_COMMON_FEATURE_READY
	PRINTK_AUD_CLK("+AudDrv_HDMI_Clk_On, Aud_I2S_Clk_cntr:%d\n", Aud_HDMI_Clk_cntr);
	if (Aud_HDMI_Clk_cntr == 0) {
		AudDrv_ANA_Clk_On();
		AudDrv_Clk_On();
	}
	Aud_HDMI_Clk_cntr++;
#endif
}

void AudDrv_HDMI_Clk_Off(void)
{
#ifdef _NON_COMMON_FEATURE_READY
	PRINTK_AUD_CLK("+AudDrv_HDMI_Clk_Off, Aud_I2S_Clk_cntr:%d\n",
		       Aud_HDMI_Clk_cntr);
	Aud_HDMI_Clk_cntr--;
	if (Aud_HDMI_Clk_cntr == 0) {
		AudDrv_ANA_Clk_Off();
		AudDrv_Clk_Off();
	} else if (Aud_HDMI_Clk_cntr < 0) {
		PRINTK_AUD_ERROR("!! AudDrv_Linein_Clk_Off, Aud_I2S_Clk_cntr<0 (%d)\n",
				 Aud_HDMI_Clk_cntr);
		AUDIO_ASSERT(true);
		Aud_HDMI_Clk_cntr = 0;
	}
	PRINTK_AUD_CLK("-AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr:%d\n", Aud_HDMI_Clk_cntr);
#endif
}

/*****************************************************************************
* FUNCTION
*  AudDrv_Suspend_Clk_Off / AudDrv_Suspend_Clk_On
*
* DESCRIPTION
*  Enable/Disable AFE clock for suspend
*
*****************************************************************************
*/

void AudDrv_Suspend_Clk_Off(void)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_Core_Clk_cntr > 0) {
		if (Aud_I2S_Clk_cntr > 0)
			aud_top_con_pdn_i2s(true);

		if (Aud_ADC_Clk_cntr > 0) {
			if (aud_clks[CLOCK_ADC].clk_prepare)
				clk_disable(aud_clks[CLOCK_ADC].clock);
		}

		if (Aud_APLL22M_Clk_cntr > 0) {
			if (aud_clks[CLOCK_APLL22M].clk_prepare)
				clk_disable(aud_clks[CLOCK_APLL22M].clock);

			if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare)
				clk_disable(aud_clks[CLOCK_APLL1_TUNER].clock);

			ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX1].clock,
					     aud_clks[CLOCK_CLK26M].clock);
			if (ret) {
				pr_err("%s clk_set_parent %s-%s fail %d\n",
				       __func__, aud_clks[CLOCK_TOP_AUD_MUX1].name,
				       aud_clks[CLOCK_CLK26M].name, ret);
				BUG();
				goto EXIT;
			}

			if (aud_clks[CLOCK_TOP_AUD_MUX1].clk_prepare) {
				clk_disable(aud_clks[CLOCK_TOP_AUD_MUX1].clock);
				pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AUD_MUX1 fail\n",
					 __func__);

			} else {
				pr_err
				("%s [CCF]clk_prepare error clk_disable CLOCK_TOP_AUD_MUX1 fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
			/* Follow MT6755 */
			if (aud_clks[CLOCK_TOP_AD_APLL1_CK].clk_prepare) {
				clk_disable(aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
				pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AD_APLL1_CK fail\n",
				__func__);

			} else {
				pr_err
				("%s [CCF]clk_prepare error CLOCK_TOP_AD_APLL1_CK fail\n",
				     __func__);
				BUG();
				goto EXIT;
			}
		}
		if (Aud_APLL24M_Clk_cntr > 0) {
			if (aud_clks[CLOCK_APLL24M].clk_prepare)
				clk_disable(aud_clks[CLOCK_APLL24M].clock);

			if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare)
				clk_disable(aud_clks[CLOCK_APLL2_TUNER].clock);

			ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX2].clock,
					     aud_clks[CLOCK_CLK26M].clock);
			if (ret) {
				pr_err("%s clk_set_parent %s-%s fail %d\n",
				       __func__, aud_clks[CLOCK_TOP_AUD_MUX2].name,
				       aud_clks[CLOCK_CLK26M].name, ret);
				BUG();
				goto EXIT;
			}

			if (aud_clks[CLOCK_TOP_AUD_MUX2].clk_prepare) {
				clk_disable(aud_clks[CLOCK_TOP_AUD_MUX2].clock);
				pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AUD_MUX2 fail\n",
					 __func__);

			} else {
				pr_err
				("%s [CCF]clk_prepare error clk_disable CLOCK_TOP_AUD_MUX2 fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
			/* Follow MT6755 */
			if (aud_clks[CLOCK_TOP_AD_APLL2_CK].clk_prepare) {
				clk_disable(aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
				pr_debug("%s [CCF]Aud clk_disable CLOCK_TOP_AD_APLL2_CK fail\n",
					 __func__);
			} else {
				pr_err
				("%s [CCF]clk_prepare error CLOCK_TOP_AD_APLL2_CK fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		}
		/* Follow MT6755
		if (Aud_AFE_Clk_cntr > 0) {
			if (aud_clks[CLOCK_AFE].clk_prepare)
				clk_disable(aud_clks[CLOCK_AFE].clock);
		}
		*/
	}
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}

void AudDrv_Suspend_Clk_On(void)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&auddrv_Clk_lock, flags);
	if (Aud_Core_Clk_cntr > 0) {
		if (aud_clks[CLOCK_AFE].clk_prepare) {
			ret = clk_enable(aud_clks[CLOCK_AFE].clock);
			if (ret) {
				pr_err("%s [CCF]Aud enable_clock enable_clock aud_afe_clk fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}
		} else {
			pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_afe_clk fail\n",
			       __func__);
			BUG();
			goto EXIT;
		}
		/* Follow MT6755 */
		if (Aud_I2S_Clk_cntr > 0) {
			if (aud_clks[CLOCK_I2S].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_I2S].clock);
				if (ret) {
					pr_err
					    ("%s [CCF]Aud enable_clock enable_clock aud_i2s_clk fail\n",
					     __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud enable_clock aud_i2s_clk fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}
		}
		/* Follow MT6755 */
		if (Aud_ADC_Clk_cntr > 0) {
			if (aud_clks[CLOCK_ADC].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_ADC].clock);
				if (ret) {
					pr_err
					    ("%s [CCF]Aud enable_clock aud_adc_clk fail\n",
					     __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error aud_adc_clk fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}
		}

		if (Aud_APLL22M_Clk_cntr > 0) {
			/* Follow MT6755 */
			if (aud_clks[CLOCK_TOP_AD_APLL1_CK].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock CLOCK_TOP_AD_APLL1_CK fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud CLOCK_TOP_AD_APLL1_CK fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}

			if (aud_clks[CLOCK_TOP_AUD_MUX1].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_TOP_AUD_MUX1].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock CLOCK_TOP_AUD_MUX1 fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_TOP_AUD_MUX1 fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}

			ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX1].clock,
					     aud_clks[CLOCK_TOP_AD_APLL1_CK].clock);
			if (ret) {
				pr_err("%s clk_set_parent %s-%s fail %d\n",
				       __func__, aud_clks[CLOCK_TOP_AUD_MUX1].name,
				       aud_clks[CLOCK_TOP_AD_APLL1_CK].name, ret);
				BUG();
				goto EXIT;
			}
			/* Follow MT6755 */
			if (aud_clks[CLOCK_APMIXED_APLL1_CK].clk_prepare) {

				ret = clk_set_rate(aud_clks[CLOCK_APMIXED_APLL1_CK].clock, 180633600);
				if (ret) {
					pr_err("%s clk_set_rate %s-180633600 fail %d\n",
					       __func__, aud_clks[CLOCK_APMIXED_APLL1_CK].name, ret);
					BUG();
					goto EXIT;
				}
			}

			if (aud_clks[CLOCK_APLL22M].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_APLL22M].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock aud_apll22m_clk fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err
				("%s [CCF]clk_prepare error Aud enable_clock aud_apll22m_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}

			if (aud_clks[CLOCK_APLL1_TUNER].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_APLL1_TUNER].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock aud_apll1_tuner_clk fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err
				("%s [CCF]clk_prepare error Aud enable_clock aud_apll1_tuner_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		}

		if (Aud_APLL24M_Clk_cntr > 0) {
			/* Follow MT6755 */
			if (aud_clks[CLOCK_TOP_AD_APLL2_CK].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock CLOCK_TOP_AD_APLL2_CK fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud CLOCK_TOP_AD_APLL2_CK fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}

			if (aud_clks[CLOCK_TOP_AUD_MUX2].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_TOP_AUD_MUX2].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock CLOCK_TOP_AUD_MUX2 fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud enable_clock CLOCK_TOP_AUD_MUX2 fail\n",
				       __func__);
				BUG();
				goto EXIT;
			}

			ret = clk_set_parent(aud_clks[CLOCK_TOP_AUD_MUX2].clock,
					     aud_clks[CLOCK_TOP_AD_APLL2_CK].clock);
			if (ret) {
				pr_err("%s clk_set_parent %s-%s fail %d\n",
				       __func__, aud_clks[CLOCK_TOP_AUD_MUX2].name,
				       aud_clks[CLOCK_TOP_AD_APLL2_CK].name, ret);
				BUG();
				goto EXIT;
			}
			/* Follow MT6755 */
			if (aud_clks[CLOCK_APMIXED_APLL2_CK].clk_prepare) {

				ret = clk_set_rate(aud_clks[CLOCK_APMIXED_APLL2_CK].clock, 196607998);
				if (ret) {
					pr_err("%s clk_set_rate %s-196607998 fail %d\n",
					       __func__, aud_clks[CLOCK_APMIXED_APLL2_CK].name, ret);
					BUG();
					goto EXIT;
				}
			}

			if (aud_clks[CLOCK_APLL24M].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_APLL24M].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock aud_apll24m_clk fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err
				("%s [CCF]clk_prepare error Aud enable_clock aud_apll24m_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}


			if (aud_clks[CLOCK_APLL2_TUNER].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_APLL2_TUNER].clock);
				if (ret) {
					pr_err
					("%s [CCF]Aud enable_clock enable_clock aud_apll2_tuner_clk fail\n",
					 __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err
				("%s [CCF]clk_prepare error Aud enable_clock aud_apll2_tuner_clk fail\n",
				 __func__);
				BUG();
				goto EXIT;
			}
		}
		/* Follow MT6755
		if (Aud_I2S_Clk_cntr > 0)
			aud_top_con_pdn_i2s(false);

		if (Aud_ADC_Clk_cntr > 0) {
			if (aud_clks[CLOCK_ADC].clk_prepare) {
				ret = clk_enable(aud_clks[CLOCK_ADC].clock);
				if (ret) {
					pr_err("%s [CCF]Aud enable_clock enable_clock ADC fail", __func__);
					BUG();
					goto EXIT;
				}
			} else {
				pr_err("%s [CCF]clk_prepare error Aud enable_clock ADC fail", __func__);
				BUG();
				goto EXIT;
			}
		}
		*/
	}
EXIT:
	spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
}

void AudDrv_Emi_Clk_On(void)
{
	mutex_lock(&auddrv_pmic_mutex);
	if (Aud_EMI_cntr == 0) {
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef _MT_IDLE_HEADER
		/* mutex is used in these api */
		disable_dpidle_by_bit(MT_CG_ID_AUDIO_AFE);
		disable_soidle_by_bit(MT_CG_ID_AUDIO_AFE);
#endif
#endif
	}
	Aud_EMI_cntr++;
	mutex_unlock(&auddrv_pmic_mutex);
}

void AudDrv_Emi_Clk_Off(void)
{
	mutex_lock(&auddrv_pmic_mutex);
	Aud_EMI_cntr--;
	if (Aud_EMI_cntr == 0) {
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef _MT_IDLE_HEADER
		/* mutex is used in these api */
		enable_dpidle_by_bit(MT_CG_ID_AUDIO_AFE);
		enable_soidle_by_bit(MT_CG_ID_AUDIO_AFE);
#endif
#endif
	}

	if (Aud_EMI_cntr < 0) {
		Aud_EMI_cntr = 0;
		PRINTK_AUD_ERROR("Aud_EMI_cntr = %d\n", Aud_EMI_cntr);
	}
	mutex_unlock(&auddrv_pmic_mutex);
}

/*****************************************************************************
 * FUNCTION
  *  AudDrv_ANC_Clk_On / AudDrv_ANC_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable ANC clock
  *
  *****************************************************************************/

void AudDrv_ANC_Clk_On(void)
{
	/* MT6755 ANC? */
}

void AudDrv_ANC_Clk_Off(void)
{
	/* MT6755 ANC? */
}

uint32 GetApllbySampleRate(uint32 SampleRate)
{
	if (SampleRate == 176400 || SampleRate == 88200 || SampleRate == 44100 ||
	    SampleRate == 22050 || SampleRate == 11025)
		return Soc_Aud_APLL1;
	else
		return Soc_Aud_APLL2;
}

void SetckSel(uint32 I2snum, uint32 SampleRate)
{
	/* always from APLL1 */
	uint32 ApllSource;

	if (GetApllbySampleRate(SampleRate) == Soc_Aud_APLL1)
		ApllSource = 0;
	else
		ApllSource = 1;

	switch (I2snum) {
	case Soc_Aud_I2S0:
		Afe_Set_Reg(CLK_AUDDIV_0, ApllSource << 8, 1 << 8);
		break;
	case Soc_Aud_I2S1:
		Afe_Set_Reg(CLK_AUDDIV_0, ApllSource << 9, 1 << 9);
		break;
	case Soc_Aud_I2S2:
		Afe_Set_Reg(CLK_AUDDIV_0, ApllSource << 10, 1 << 10);
		break;
	case Soc_Aud_I2S3:
		Afe_Set_Reg(CLK_AUDDIV_0, ApllSource << 11, 1 << 11);
		break;
	case Soc_Aud_I2SConnSys:
		/* TODO: It may need to be implemented */
		break;
	}
	pr_warn("%s I2snum = %d ApllSource = %d\n", __func__, I2snum, ApllSource);
}

void EnableALLbySampleRate(uint32 SampleRate)
{
	pr_warn("%s, APLL1Counter = %d, APLL2Counter = %d, SampleRate = %d\n", __func__,
			APLL1Counter, APLL2Counter, SampleRate);

	switch (GetApllbySampleRate(SampleRate)) {
	case Soc_Aud_APLL1:
		APLL1Counter++;
		if (APLL1Counter == 1) {
			AudDrv_Clk_On();
			EnableApll1(true);
			/*EnableI2SDivPower(AUDIO_APLL1_DIV0, true);*/
			EnableI2SCLKDiv(Soc_Aud_APLL1_DIV, true);
			AudDrv_APLL1Tuner_Clk_On();
			/*AudDrv_APLL2Tuner_Clk_On();*/
		}
		break;
	case Soc_Aud_APLL2:
		APLL2Counter++;
		if (APLL2Counter == 1) {
			AudDrv_Clk_On();
			EnableApll2(true);
			/*EnableI2SDivPower(AUDIO_APLL2_DIV0, true);*/
			EnableI2SCLKDiv(Soc_Aud_APLL2_DIV, true);
			/*AudDrv_APLL1Tuner_Clk_On();*/
			AudDrv_APLL2Tuner_Clk_On();
		}
		break;
	default:
		pr_warn("[AudioWarn] GetApllbySampleRate(%d) = %d not recognized\n",
				SampleRate, GetApllbySampleRate(SampleRate));
		break;
	}
}

void DisableALLbySampleRate(uint32 SampleRate)
{
	pr_warn("%s, APLL1Counter = %d, APLL2Counter = %d, SampleRate = %d\n", __func__,
		APLL1Counter, APLL2Counter, SampleRate);

	switch (GetApllbySampleRate(SampleRate)) {
	case Soc_Aud_APLL1:
		APLL1Counter--;
		if (APLL1Counter == 0) {
			/* disable APLL1 */
			/*EnableI2SDivPower(AUDIO_APLL1_DIV0, false);*/
			EnableI2SCLKDiv(Soc_Aud_APLL1_DIV, false);
			AudDrv_APLL1Tuner_Clk_Off();
			EnableApll1(false);
			AudDrv_Clk_Off();
		} else if (APLL1Counter < 0) {
			pr_warn("%s(), APLL1Counter %d < 0\n",
				__func__,
				APLL1Counter);
			APLL1Counter = 0;
		}
		break;
	case Soc_Aud_APLL2:
		APLL2Counter--;
		if (APLL2Counter == 0) {
			/* disable APLL2 */
			/*EnableI2SDivPower(AUDIO_APLL2_DIV0, false);*/
			EnableI2SCLKDiv(Soc_Aud_APLL2_DIV, false);
			AudDrv_APLL2Tuner_Clk_Off();
			EnableApll2(false);
			AudDrv_Clk_Off();
		} else if (APLL2Counter < 0) {
			pr_warn("%s(), APLL2Counter %d < 0\n",
				__func__,
				APLL2Counter);
			APLL2Counter = 0;
		}
		break;
	default:
		pr_warn("[AudioWarn] GetApllbySampleRate(%d) = %d not recognized\n",
			SampleRate, GetApllbySampleRate(SampleRate));
		break;
	}
}

void EnableI2SDivPower(uint32 Diveder_name, bool bEnable)
{
	pr_warn("%s bEnable = %d", __func__, bEnable);
	if (bEnable)
		Afe_Set_Reg(CLK_AUDDIV_0, 0 << Diveder_name, 1 << Diveder_name);
	else
		Afe_Set_Reg(CLK_AUDDIV_0, 1 << Diveder_name, 1 << Diveder_name);
}

void EnableI2SCLKDiv(uint32 I2snum, bool bEnable)
{
	pr_warn("%s mI2SAPLLDivSelect = %d, i2snum = %d\n", __func__, mI2SAPLLDivSelect[I2snum], I2snum);
	EnableI2SDivPower(mI2SAPLLDivSelect[I2snum], bEnable);
}

void EnableApll1(bool bEnable)
{
	pr_warn("%s bEnable = %d\n", __func__, bEnable);

	if (bEnable) {
		if (Aud_APLL_DIV_APLL1_cntr == 0) {
			/* apll1_div0_pdn power down */
			Afe_Set_Reg(CLK_AUDDIV_0, 1 << 0, 1 << 0);

			/* apll2_div0_pdn power down */
/* TODO: KC: this should be unnecessary // Afe_Set_Reg(CLK_AUDDIV_0, 1 << 1, 1 << 1); */

			/* set appl1_ck_div0 = 7,  f_faud_engen1_ck = clock source / 8*/
			/* 180.6336 / 8 = 22.5792MHz */
			Afe_Set_Reg(CLK_AUDDIV_0, 7 << 24, 0xf << 24);

			AudDrv_APLL22M_Clk_On();
			/* apll1_div0_pdn power up */
			Afe_Set_Reg(CLK_AUDDIV_0, 0 << 0, 1 << 0);

			/* apll2_div0_pdn power up. */
/* TODO: KC: this should be unnecessary // Afe_Set_Reg(CLK_AUDDIV_0, 0x0, 0x2); */
		}
		Aud_APLL_DIV_APLL1_cntr++;
	} else {
		Aud_APLL_DIV_APLL1_cntr--;
		if (Aud_APLL_DIV_APLL1_cntr == 0) {
			AudDrv_APLL22M_Clk_Off();
			/* apll1_div0_pdn power down */
			Afe_Set_Reg(CLK_AUDDIV_0, 1 << 0, 1 << 0);
		}
	}
}

void EnableApll2(bool bEnable)
{
	pr_warn("%s bEnable = %d\n", __func__, bEnable);

	if (bEnable) {
		if (Aud_APLL_DIV_APLL2_cntr == 0) {
			/* apll2_div0_pdn power down */
			Afe_Set_Reg(CLK_AUDDIV_0, 1 << 1, 1 << 1);

			/* apll2_div0_pdn power down */
/* TODO: KC: this should be unnecessary // Afe_Set_Reg(CLK_AUDDIV_0, 1 << 1, 1 << 1); */

			/* set appl2_ck_div0 = 7,  f_faud_engen2_ck = clock source / 8 */
			/* 196.608 / 8 = 24.576MHz */
			Afe_Set_Reg(CLK_AUDDIV_0, 7 << 28, 0xf << 28);

			AudDrv_APLL24M_Clk_On();

			/* apll2_div0_pdn power up */
			Afe_Set_Reg(CLK_AUDDIV_0, 0 << 1, 1 << 1);
		}
		Aud_APLL_DIV_APLL2_cntr++;
	} else {
		Aud_APLL_DIV_APLL2_cntr--;
		if (Aud_APLL_DIV_APLL2_cntr == 0) {
			AudDrv_APLL24M_Clk_Off();
			/* apll2_div0_pdn power down */
			Afe_Set_Reg(CLK_AUDDIV_0, 1 << 1, 1 << 1);
		}
	}
}

uint32 SetCLkMclk(uint32 I2snum, uint32 SampleRate)
{
	uint32 I2S_APll = 0;
	uint32 I2s_ck_div = 0;

	if (GetApllbySampleRate(SampleRate) == Soc_Aud_APLL1)
		I2S_APll = 180633600;
	else
		I2S_APll = 196608000;

	SetckSel(I2snum, SampleRate);	/* set I2Sx mck source */

	switch (I2snum) {
	case Soc_Aud_I2S0:
		I2s_ck_div = (I2S_APll / MCLKFS / SampleRate) - 1;
		Afe_Set_Reg(CLK_AUDDIV_1, I2s_ck_div << 0, 0xff << 0);
		break;
	case Soc_Aud_I2S1:
		I2s_ck_div = (I2S_APll / MCLKFS / SampleRate) - 1;
		Afe_Set_Reg(CLK_AUDDIV_1, I2s_ck_div << 8, 0xff << 8);
		break;
	case Soc_Aud_I2S2:
		I2s_ck_div = (I2S_APll / MCLKFS / SampleRate) - 1;
		Afe_Set_Reg(CLK_AUDDIV_1, I2s_ck_div << 16, 0xff << 16);
		break;
	case Soc_Aud_I2S3:
		I2s_ck_div = (I2S_APll / MCLKFS / SampleRate) - 1;
		Afe_Set_Reg(CLK_AUDDIV_1, I2s_ck_div << 24, 0xff << 24);
		break;
	case Soc_Aud_I2SConnSys:
		/* TODO: It may need to be implemented */
		break;
	default:
		pr_warn("[AudioWarn] SetCLkMclk: I2snum = %d not recognized\n",
				I2snum);
		break;
	}

	pr_debug("%s I2snum = %d I2s_ck_div = %d I2S_APll = %d\n", __func__, I2snum, I2s_ck_div,
		I2S_APll);

	return I2s_ck_div;
}

void SetCLkBclk(uint32 MckDiv, uint32 SampleRate, uint32 Channels, uint32 Wlength)
{
	/* BCK set only required in 6595 TDM function div4/div5 */
	uint32 I2S_APll = 0;
	uint32 I2S_Bclk = 0;
	uint32 I2s_Bck_div = 0;

	pr_debug("%s MckDiv = %dv SampleRate = %d  Channels = %d Wlength = %d\n", __func__, MckDiv,
		SampleRate, Channels, Wlength);
	MckDiv++;

	if (GetApllbySampleRate(SampleRate) == Soc_Aud_APLL1)
		I2S_APll = 180633600;
	else
		I2S_APll = 196608000;

	I2S_Bclk = SampleRate * Channels * (Wlength + 1) * 16;
	I2s_Bck_div = (I2S_APll / MckDiv) / I2S_Bclk;

	pr_warn("%s I2S_APll = %dv I2S_Bclk = %d  I2s_Bck_div = %d\n", __func__, I2S_APll,
		I2S_Bclk, I2s_Bck_div);

	if (I2s_Bck_div > 0)
		I2s_Bck_div--;

	/* Follow MT6755: No TDM
	Afe_Set_Reg(CLK_AUDDIV_2, I2s_Bck_div << 8, 0x0000ff00);
	*/

}

void PowerDownAllI2SDiv(void)
{
	int i = 0;

	for (i = AUDIO_APLL1_DIV0; i < AUDIO_APLL_DIV_NUM; i++)
		EnableI2SDivPower(i, false);
}
