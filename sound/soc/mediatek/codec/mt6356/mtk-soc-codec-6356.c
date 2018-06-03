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
 *   mtk_soc_codec_63xx
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   Audio codec stub file
 *
 * Author:
 * -------
 * Chipeng Chang
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

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "mtk-auddrv-def.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-gpio.h"

#include "mtk-soc-analog-type.h"
#include "mtk-soc-codec-63xx.h"
#ifdef _GIT318_READY
#include <mtk_clkbuf_ctl.h>
#endif
#ifdef _GIT318_PMIC_READY
#include <mach/mtk_pmic.h>
#endif

/* HP IMPEDANCE Current Calibration from EFUSE */
/* #define EFUSE_HP_IMPEDANCE */

/* static function declaration */
static bool AudioPreAmp1_Sel(int Mul_Sel);
static bool GetAdcStatus(void);
static void TurnOffDacPower(void);
static void TurnOnDacPower(int device);
static void setDlMtkifSrc(bool enable);
static void SetDcCompenSation(void);
static void Voice_Amp_Change(bool enable);
static void Speaker_Amp_Change(bool enable);

static mt6356_Codec_Data_Priv *mCodec_data;
static uint32 mBlockSampleRate[AUDIO_ANALOG_DEVICE_INOUT_MAX] = { 48000, 48000, 48000 };

#define MAX_DL_SAMPLE_RATE (192000)
#define MAX_UL_SAMPLE_RATE (192000)

static DEFINE_MUTEX(Ana_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_buf_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_Clk_Mutex);
static DEFINE_MUTEX(Ana_Power_Mutex);
static DEFINE_MUTEX(AudAna_lock);

static int mAudio_Analog_Mic1_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic2_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic3_mode = AUDIO_ANALOGUL_MODE_ACC;
static int mAudio_Analog_Mic4_mode = AUDIO_ANALOGUL_MODE_ACC;

static int TrimOffset = 2048;
static const int DC1unit_in_uv = 19184;	/* in uv with 0DB */
/* static const int DC1unit_in_uv = 21500; */	/* in uv with 0DB */
static const int DC1devider = 8;	/* in uv */

enum hp_depop_flow {
	HP_DEPOP_FLOW_DEPOP_HW,
	HP_DEPOP_FLOW_33OHM,
	HP_DEPOP_FLOW_DEPOP_HW_33OHM,
	HP_DEPOP_FLOW_NONE,
};
static unsigned int mUseHpDepopFlow;

static int low_power_mode;

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef EFUSE_HP_TRIM
static unsigned int RG_AUDHPLTRIM_VAUDP15, RG_AUDHPRTRIM_VAUDP15, RG_AUDHPLFINETRIM_VAUDP15,
	RG_AUDHPRFINETRIM_VAUDP15, RG_AUDHPLTRIM_VAUDP15_SPKHP, RG_AUDHPRTRIM_VAUDP15_SPKHP,
	RG_AUDHPLFINETRIM_VAUDP15_SPKHP, RG_AUDHPRFINETRIM_VAUDP15_SPKHP;
#endif
#endif

static int mAdc_Power_Mode;
static unsigned int dAuxAdcChannel = 16;
static const int mDcOffsetTrimChannel = 9;
static bool mInitCodec;

/* Jogi: Need? @{ */
#define SND_SOC_ADV_MT_FMTS (\
				SNDRV_PCM_FMTBIT_S16_LE |\
				SNDRV_PCM_FMTBIT_S16_BE |\
				SNDRV_PCM_FMTBIT_U16_LE |\
				SNDRV_PCM_FMTBIT_U16_BE |\
				SNDRV_PCM_FMTBIT_S24_LE |\
				SNDRV_PCM_FMTBIT_S24_BE |\
				SNDRV_PCM_FMTBIT_U24_LE |\
				SNDRV_PCM_FMTBIT_U24_BE |\
				SNDRV_PCM_FMTBIT_S32_LE |\
				SNDRV_PCM_FMTBIT_S32_BE |\
				SNDRV_PCM_FMTBIT_U32_LE |\
				SNDRV_PCM_FMTBIT_U32_BE)

#define SND_SOC_STD_MT_FMTS (\
				SNDRV_PCM_FMTBIT_S16_LE |\
				SNDRV_PCM_FMTBIT_S16_BE |\
				SNDRV_PCM_FMTBIT_U16_LE |\
				SNDRV_PCM_FMTBIT_U16_BE)
/* @} Build pass: */
#define SOC_HIGH_USE_RATE	(SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000)

static void Audio_Amp_Change(int channels, bool enable);
static void SavePowerState(void)
{
	int i = 0;

	for (i = 0; i < AUDIO_ANALOG_DEVICE_MAX; i++) {
		mCodec_data->mAudio_BackUpAna_DevicePower[i] =
		    mCodec_data->mAudio_Ana_DevicePower[i];
	}
}

static void RestorePowerState(void)
{
	int i = 0;

	for (i = 0; i < AUDIO_ANALOG_DEVICE_MAX; i++) {
		mCodec_data->mAudio_Ana_DevicePower[i] =
		    mCodec_data->mAudio_BackUpAna_DevicePower[i];
	}
}

static bool GetDLStatus(void)
{
	int i = 0;

	for (i = 0; i < AUDIO_ANALOG_DEVICE_2IN1_SPK; i++) {
		if (mCodec_data->mAudio_Ana_DevicePower[i] == true)
			return true;
	}
	return false;
}

static bool mAnaSuspend;
void SetAnalogSuspend(bool bEnable)
{
	pr_warn("%s bEnable ==%d mAnaSuspend = %d\n", __func__, bEnable, mAnaSuspend);
	if ((bEnable == true) && (mAnaSuspend == false)) {
		/*Ana_Log_Print();*/
		SavePowerState();
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == true) {
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
			    false;
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false);
		}
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == true) {
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
			    false;
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false);
		}
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] == true) {
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] =
			    false;
			Voice_Amp_Change(false);
		}
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] == true) {
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] =
			    false;
			Speaker_Amp_Change(false);
		}
		/*Ana_Log_Print();*/
		mAnaSuspend = true;
	} else if ((bEnable == false) && (mAnaSuspend == true)) {
		/*Ana_Log_Print();*/
		if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] ==
		    true) {
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true);
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
			    true;
		}
		if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] ==
		    true) {
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true);
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
			    false;
		}
		if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] ==
		    true) {
			Voice_Amp_Change(true);
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] =
			    false;
		}
		if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] ==
		    true) {
			Speaker_Amp_Change(true);
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] =
			    false;
		}
		RestorePowerState();
		/*Ana_Log_Print();*/
		mAnaSuspend = false;
	}
}

static int audck_buf_Count;
void audckbufEnable(bool enable)
{
	pr_aud("audckbufEnable(), audck_buf_Count = %d, enable = %d\n",
	       audck_buf_Count, enable);

	mutex_lock(&Ana_buf_Ctrl_Mutex);
	if (enable) {
		if (audck_buf_Count == 0) {
			Ana_Set_Reg(DCXO_CW14, 0x1 << 13, 0x1 << 13);
			pr_warn("-PMIC DCXO XO_AUDIO_EN_M enable, DCXO_CW14 = 0x%x\n",
				Ana_Get_Reg(DCXO_CW14));
		}
		audck_buf_Count++;
	} else {
		audck_buf_Count--;
		if (audck_buf_Count == 0) {
			Ana_Set_Reg(DCXO_CW14, 0x0 << 13, 0x1 << 13);
			pr_warn("-PMIC DCXO XO_AUDIO_EN_M disable, DCXO_CW14 = 0x%x\n",
				Ana_Get_Reg(DCXO_CW14));
			/* if didn't close, 60uA leak at audio analog */
		}
		if (audck_buf_Count < 0) {
			pr_warn("audck_buf_Count count < 0\n");
			audck_buf_Count = 0;
		}
	}
	mutex_unlock(&Ana_buf_Ctrl_Mutex);
}

static int ClsqCount;
static void ClsqEnable(bool enable)
{
	pr_aud("ClsqEnable ClsqCount = %d enable = %d\n", ClsqCount, enable);
	mutex_lock(&AudAna_lock);
	if (enable) {
		if (ClsqCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ, 0x0001, 0x0001);
			/* Enable CLKSQ 26MHz */
			Ana_Set_Reg(TOP_CLKSQ_SET, 0x0001, 0x0001);
			/* Turn on 26MHz source clock */
		}
		ClsqCount++;
	} else {
		ClsqCount--;
		if (ClsqCount < 0) {
			pr_warn("ClsqEnable count <0\n");
			ClsqCount = 0;
		}
		if (ClsqCount == 0) {
			Ana_Set_Reg(TOP_CLKSQ_CLR, 0x0001, 0x0001);
			/* Turn off 26MHz source clock */
			Ana_Set_Reg(TOP_CLKSQ, 0x0000, 0x0001);
			/* Disable CLKSQ 26MHz */
		}
	}
	mutex_unlock(&AudAna_lock);
}

static int TopCkCount;
static void Topck_Enable(bool enable)
{
	pr_aud("Topck_Enable enable = %d TopCkCount = %d\n", enable, TopCkCount);
	mutex_lock(&Ana_Clk_Mutex);
	if (enable == true) {
		if (TopCkCount == 0) {
			Ana_Set_Reg(AUD_TOP_CKPDN_CON0, 0x0, 0x66);
			/* Turn on AUDNCP_CLKDIV engine clock,Turn on AUD 26M */
		}
		TopCkCount++;
	} else {
		TopCkCount--;
		if (TopCkCount == 0) {
			Ana_Set_Reg(AUD_TOP_CKPDN_CON0, 0x66, 0x66);
			/* Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M */
		}

		if (TopCkCount < 0) {
			pr_warn("TopCkCount <0 =%d\n ", TopCkCount);
			TopCkCount = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

static int NvRegCount;
static void NvregEnable(bool enable)
{
	pr_aud("NvregEnable NvRegCount == %d enable = %d\n", NvRegCount, enable);
	mutex_lock(&Ana_Clk_Mutex);
	if (enable == true) {
		if (NvRegCount == 0) {
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1 << 4);
			/* Enable AUDGLB */
		}
		NvRegCount++;
	} else {
		NvRegCount--;
		if (NvRegCount == 0) {
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1 << 4, 0x1 << 4);
			/* Disable AUDGLB */
		}
		if (NvRegCount < 0) {
			pr_warn("NvRegCount <0 =%d\n ", NvRegCount);
			NvRegCount = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

bool hasHpDepopHw(void)
{
	return mUseHpDepopFlow == HP_DEPOP_FLOW_DEPOP_HW ||
	       mUseHpDepopFlow == HP_DEPOP_FLOW_DEPOP_HW_33OHM;
}

bool hasHp33Ohm(void)
{
	return mUseHpDepopFlow == HP_DEPOP_FLOW_33OHM ||
	       mUseHpDepopFlow == HP_DEPOP_FLOW_DEPOP_HW_33OHM;
}

/*extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);*/
void Auddrv_Read_Efuse_HPOffset(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef EFUSE_HP_TRIM
	U32 ret = 0;
	U32 reg_val = 0;
	int i = 0, j = 0;
	U32 efusevalue[3];

	pr_warn("Auddrv_Read_Efuse_HPOffset(+)\n");

	/* 1. enable efuse ctrl engine clock */
	ret = pmic_config_interface(0x026C, 0x0040, 0xFFFF, 0);
	ret = pmic_config_interface(0x024E, 0x0004, 0xFFFF, 0);

	/* 2. */
	ret = pmic_config_interface(0x0C16, 0x1, 0x1, 0);

	for (i = 0xe; i <= 0x10; i++) {
		/* 3. set row to read */
		ret = pmic_config_interface(0x0C00, i, 0x1F, 1);

		/* 4. Toggle */
		ret = pmic_read_interface(0xC10, &reg_val, 0x1, 0);

		if (reg_val == 0)
			ret = pmic_config_interface(0xC10, 1, 0x1, 0);
		else
			ret = pmic_config_interface(0xC10, 0, 0x1, 0);


		/* 5. polling Reg[0xC1A] */
		reg_val = 1;
		while (reg_val == 1) {
			ret = pmic_read_interface(0xC1A, &reg_val, 0x1, 0);
			pr_warn("Auddrv_Read_Efuse_HPOffset polling 0xC1A=0x%x\n", reg_val);
		}

		udelay(1000);	/* Need to delay at least 1ms for 0xC1A and than can read 0xC18 */

		/* 6. read data */
		efusevalue[j] = upmu_get_reg_value(0x0C18);
		pr_warn("HPoffset : efuse[%d]=0x%x\n", j, efusevalue[j]);
		j++;
	}

	/* 7. Disable efuse ctrl engine clock */
	ret = pmic_config_interface(0x024C, 0x0004, 0xFFFF, 0);
	ret = pmic_config_interface(0x026A, 0x0040, 0xFFFF, 0);

	RG_AUDHPLTRIM_VAUDP15 = (efusevalue[0] >> 10) & 0xf;
	RG_AUDHPRTRIM_VAUDP15 = ((efusevalue[0] >> 14) & 0x3) + ((efusevalue[1] & 0x3) << 2);
	RG_AUDHPLFINETRIM_VAUDP15 = (efusevalue[1] >> 3) & 0x3;
	RG_AUDHPRFINETRIM_VAUDP15 = (efusevalue[1] >> 5) & 0x3;
	RG_AUDHPLTRIM_VAUDP15_SPKHP = (efusevalue[1] >> 7) & 0xF;
	RG_AUDHPRTRIM_VAUDP15_SPKHP = (efusevalue[1] >> 11) & 0xF;
	RG_AUDHPLFINETRIM_VAUDP15_SPKHP =
	    ((efusevalue[1] >> 15) & 0x1) + ((efusevalue[2] & 0x1) << 1);
	RG_AUDHPRFINETRIM_VAUDP15_SPKHP = ((efusevalue[2] >> 1) & 0x3);

	pr_warn("RG_AUDHPLTRIM_VAUDP15 = %x\n", RG_AUDHPLTRIM_VAUDP15);
	pr_warn("RG_AUDHPRTRIM_VAUDP15 = %x\n", RG_AUDHPRTRIM_VAUDP15);
	pr_warn("RG_AUDHPLFINETRIM_VAUDP15 = %x\n", RG_AUDHPLFINETRIM_VAUDP15);
	pr_warn("RG_AUDHPRFINETRIM_VAUDP15 = %x\n", RG_AUDHPRFINETRIM_VAUDP15);
	pr_warn("RG_AUDHPLTRIM_VAUDP15_SPKHP = %x\n", RG_AUDHPLTRIM_VAUDP15_SPKHP);
	pr_warn("RG_AUDHPRTRIM_VAUDP15_SPKHP = %x\n", RG_AUDHPRTRIM_VAUDP15_SPKHP);
	pr_warn("RG_AUDHPLFINETRIM_VAUDP15_SPKHP = %x\n", RG_AUDHPLFINETRIM_VAUDP15_SPKHP);
	pr_warn("RG_AUDHPRFINETRIM_VAUDP15_SPKHP = %x\n", RG_AUDHPRFINETRIM_VAUDP15_SPKHP);
#endif
#endif
	pr_warn("Auddrv_Read_Efuse_HPOffset(-)\n");
}
EXPORT_SYMBOL(Auddrv_Read_Efuse_HPOffset);

static void Hp_Zcd_Enable(bool _enable)
{
	if (_enable) {
		/* Enable ZCD, for minimize pop noise */
		/* when adjust gain during HP buffer on */
		Ana_Set_Reg(ZCD_CON0, 0x1 << 8, 0x7 << 8);
		Ana_Set_Reg(ZCD_CON0, 0x0 << 7, 0x1 << 7);

		/* timeout, 1=5ms, 0=30ms */
		Ana_Set_Reg(ZCD_CON0, 0x1 << 6, 0x1 << 6);

		Ana_Set_Reg(ZCD_CON0, 0x0 << 4, 0x3 << 4);
		Ana_Set_Reg(ZCD_CON0, 0x5 << 1, 0x7 << 1);
		Ana_Set_Reg(ZCD_CON0, 0x1 << 0, 0x1 << 0);
	} else {
		Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);
	}
}

static void hp_main_output_ramp(bool up)
{
	int i = 0, stage = 0;
	int target = 0;

	/* Enable/Reduce HPL/R main output stage step by step */
	target = (low_power_mode == 1) ? 3 : 7;
	for (i = 0; i <= target; i++) {
		stage = up ? i : target - i;
		Ana_Set_Reg(AUDDEC_ANA_CON1, stage << 8, 0x7 << 8);
		Ana_Set_Reg(AUDDEC_ANA_CON1, stage << 11, 0x7 << 11);
		usleep_range(100, 150);
	}
}

static void hp_aux_feedback_loop_gain_ramp(bool up)
{
	int i = 0, stage = 0;

	/* Reduce HP aux feedback loop gain step by step */
	for (i = 0; i <= 0xf; i++) {
		stage = up ? i : 0xf - i;
		Ana_Set_Reg(AUDDEC_ANA_CON9, stage << 12, 0xf << 12);
		usleep_range(100, 150);
	}
}

static bool is_valid_hp_pga_idx(int reg_idx)
{
	return (reg_idx >= 0 && reg_idx <= 0x12) || reg_idx == 0x1f;
}

static void HeadsetVoloumeRestore(void)
{
	int index = 0, oldindex = 0, offset = 0, count = 1, reg_idx;

	/*pr_warn("%s\n", __func__);*/
	index = 18;
	oldindex = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	if (index > oldindex) {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = index - oldindex;
		while (offset > 0) {
			reg_idx = oldindex + count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(ZCD_CON2,
					    (reg_idx << 7) | reg_idx,
					    0xf9f);
			}
			offset--;
			count++;
			udelay(100);
		}
	} else {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = oldindex - index;
		while (offset > 0) {
			reg_idx = oldindex - count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(ZCD_CON2,
					    (reg_idx << 7) | reg_idx,
					    0xf9f);
			}
			offset--;
			count++;
			udelay(100);
		}
	}
	Ana_Set_Reg(ZCD_CON2, 0xf9f, 0xf9f);
}

static void HeadsetVoloumeSet(void)
{
	int index = 0, oldindex = 0, offset = 0, count = 1, reg_idx;
	/* pr_warn("%s\n", __func__); */
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	oldindex = 18;
	if (index > oldindex) {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = index - oldindex;
		while (offset > 0) {
			reg_idx = oldindex + count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(ZCD_CON2,
					    (reg_idx << 7) | reg_idx,
					    0xf9f);
			}
			offset--;
			count++;
			udelay(200);
		}
	} else {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = oldindex - index;
		while (offset > 0) {
			reg_idx = oldindex - count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(ZCD_CON2,
					    (reg_idx << 7) | reg_idx,
					    0xf9f);
			}
			offset--;
			count++;
			udelay(200);
		}
	}
	Ana_Set_Reg(ZCD_CON2, (index << 7) | (index), 0xf9f);
}

void setOffsetTrimMux(unsigned int Mux)
{
	pr_aud("%s Mux = %d\n", __func__, Mux);
	/* Audio offset trimming buffer mux selection */
	Ana_Set_Reg(AUDDEC_ANA_CON8, Mux, 0xf);
}

void setOffsetTrimBufferGain(unsigned int gain)
{
	/* Audio offset trimming buffer gain selection */
	Ana_Set_Reg(AUDDEC_ANA_CON8, gain << 4, 0x3 << 4);
}

void EnableTrimbuffer(bool benable)
{
	if (benable == true) {
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x1 << 6, 0x1 << 6);
		/* Audio offset trimming buffer enable */
	} else {
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0, 0x1 << 6);
		/* Audio offset trimming buffer disable */
	}
}

static int mHplTrimOffset = 2048;
static int mHprTrimOffset = 2048;

void SetHplTrimOffset(int Offset)
{
	pr_warn("%s Offset = %d\n", __func__, Offset);
	mHplTrimOffset = Offset;
	if ((Offset > 2198) || (Offset < 1898)) {
		mHplTrimOffset = 2048;
		pr_err("SetHplTrimOffset offset may be invalid offset = %d\n", Offset);
	}
}

void SetHprTrimOffset(int Offset)
{
	pr_warn("%s Offset = %d\n", __func__, Offset);
	mHprTrimOffset = Offset;
	if ((Offset > 2198) || (Offset < 1898)) {
		mHprTrimOffset = 2048;
		pr_err("SetHprTrimOffset offset may be invalid offset = %d\n", Offset);
	}
}

void OpenTrimBufferHardware(bool enable)
{
	pr_warn("%s(), enable %d\n", __func__, enable);

	/* TODO: KC: not final yet, need de last confirm */
	if (enable) {
		TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_HEADSETL);

		/* Disable headphone short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
		/* Disable handset short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
		/* Disable linout short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
		/* Reduce ESD resistance of AU_REFN */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
		/* Set HPR/HPL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);

		/* Turn on DA_600K_NCP_VA18 */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
		/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
		/* Toggle RG_DIVCKS_CHG */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
		/* Set NCP soft start mode as default mode: 100us */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
		/* Enable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
		udelay(250);

		/* Enable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
		/* Enable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
		udelay(100);

		/* Disable AUD_ZCD */
		Hp_Zcd_Enable(false);

		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Set HP DR bias current optimization, 010: 6uA */
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x4900, 0xffff);
		/* Set HP & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Set HPP/N STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc033, 0xffff);
		/* No Pull-down HPL/R to AVSS28_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4033, 0xffff);

		/* Enable HP driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30c0, 0xffff);
		/* Enable HP driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30f0, 0xffff);
		/* Enable HP main CMFB loop */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0200, 0xffff);

		/* Enable HP aux output stage */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0003, 0xffff);
		/* Enable HPR/L main output stage step by step */
		hp_main_output_ramp(true);

		/* apply volume setting */
		HeadsetVoloumeSet();

		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
		/* Enable Audio DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30ff, 0xffff);

		/* Enable low-noise mode of DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xf201, 0xffff);
		udelay(100);

		/* Switch HPL MUX to audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x32ff, 0xffff);
		/* Switch HPR MUX to audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3aff, 0xffff);
	} else {
		/* HPR/HPL mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
		/* Disable Audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

		/* Disable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

		/* decrease HPL/R gain to normal gain step by step */
		HeadsetVoloumeRestore();

		/* decrease HPR/L main output stage step by step */
		hp_main_output_ramp(false);

		/* Disable HP main output stage */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0, 0x3);

		/* Disable HP driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 4);

		/* Disable HP driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 6);

		/* Disable HP aux CMFB loop */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

		/* Enable HP main CMFB Switch */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

		/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

		/* Disable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
		/* Disable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
		/* Disable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
		/* Disable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

		TurnOffDacPower();

	}
}

void OpenAnalogTrimHardware(bool enable)
{
	if (enable) {
		pr_warn("%s true\n", __func__);
		TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_HEADSETL);
		/* set analog part (HP playback) */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xA155, 0xA000);
		/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0100, 0x0100);
		/* Enable NV regulator (-1.6V) */
		Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);
		/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE080, 0xffff);
		/* Disable headphone, voice and short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xA055, 0x0100);
		/* Enable IBIST */
		Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);
		/* Set HPR/HPL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON3, 0x001F, 0xffff);
		/* Set HS gain as minimum (~ -40dB) */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0001, 0x0001);
		/* De_OSC of HP */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x2000, 0xffff);
		/* enable output STBENH */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x2100, 0xffff);
		/* De_OSC of voice, enable output STBENH */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE090, 0xffff);
		/* Enable voice driver */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x2140, 0xffff);
		/* Enable pre-charge buffer  */

		udelay(50);

		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xA255, 0x0200);
		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE09F, 0xffff);
		/* Enable Audio DAC  */

		/* Apply digital DC compensation value to DAC */
		Ana_Set_Reg(ZCD_CON2, 0x0489, 0xffff);
		/* Set HPR/HPL gain to -1dB, step by step */
		/* SetDcCompenSation(); */

		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF49F, 0xffff);
		/* Switch HP MUX to audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF4FF, 0xffff);
		/* Enable HPR/HPL */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x2100, 0xffff);
		/* Disable pre-charge buffer */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x2000, 0xffff);
		/* Disable De_OSC of voice */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xF4EF, 0xffff);
		/* Disable voice buffer */
	} else {
		pr_warn("%s false\n", __func__);
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0xE000, 0xffff);
		/* Disable Audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xA055, 0x0200);
		/* Audio decoder reset - bit 9 */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0xA155, 0x0100);
		/* Disable IBIST  - bit 8 */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0000, 0x0100);
		/* Disable NV regulator (-1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0155, 0xA000);
		/* Disable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x0001);
		/* De_OSC of HP */
		TurnOffDacPower();
	}
}

void OpenAnalogHeadphone(bool bEnable)
{
	pr_warn("OpenAnalogHeadphone bEnable = %d", bEnable);
	if (bEnable) {
		SetHplTrimOffset(2048);
		SetHprTrimOffset(2048);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = 44100;
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] = true;
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] = true;
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] = false;
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] = false;
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false);
	}
}

bool OpenHeadPhoneImpedanceSetting(bool bEnable)
{
	pr_aud("%s benable = %d\n", __func__, bEnable);
	if (GetDLStatus() == true)
		return false;

	if (bEnable == true) {
		TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_HEADSETL);

		/* Disable headphone short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
		/* Disable handset short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
		/* Disable linout short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
		/* Reduce ESD resistance of AU_REFN */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);

		/* Turn on DA_600K_NCP_VA18 */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
		/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
		/* Toggle RG_DIVCKS_CHG */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
		/* Set NCP soft start mode as default mode: 100us */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
		/* Enable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
		udelay(250);

		/* Enable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
		/* Enable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
		udelay(100);

		/* Disable AUD_ZCD */
		Hp_Zcd_Enable(false);

		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Disable HPR/L STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
		/* No Pull-down HPL/R to AVSS28_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0xffff);

		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
		/* Enable Audio L channel DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3009, 0xffff);

		/* Enable HPDET circuit, select DACLP as HPDET input and HPR as HPDET output */
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x1900, 0xffff);
	} else {
		/* Disable Audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

		/* Disable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

		/* Disable HP main output stage */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0, 0x3);

		/* Disable HP driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 4);

		/* Disable HP driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 6);

		/* Disable HP aux CMFB loop */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

		/* Enable HP main CMFB Switch */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

		/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

		/* Disable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
		/* Disable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
		/* Disable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
		/* Disable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

		TurnOffDacPower();
	}
	return true;
}

void mtk_read_hp_detection_parameter(struct mtk_hpdet_param *hpdet_param)
{
	hpdet_param->auxadc_upper_bound = 4050; /* should little lower than auxadc max resolution */
	hpdet_param->dc_Step = 100; /* Dc ramp up and ramp down step */
	hpdet_param->dc_Phase0 = 200; /* Phase 0 : high impedance with worst resolution */
	hpdet_param->dc_Phase1 = 2200; /* Phase 1 : median impedance with normal resolution */
	hpdet_param->dc_Phase2 = 8800; /* Phase 2 : low impedance with better resolution */
	hpdet_param->resistance_first_threshold = 150; /* Resistance Threshold of phase 2 and phase 1 */
	hpdet_param->resistance_second_threshold = 650; /* Resistance Threshold of phase 1 and phase 0 */
}
int mtk_calculate_impedance_formula(int pcm_offset, int aux_diff)
{
	/* The formula is from DE programming guide */
	/* should be mantain by pmic owner */
	/* R = V /I */
	/* V = auxDiff * (1800mv /auxResolution)  /TrimBufGain */
	/* I =  pcmOffset * DAC_constant * Gsdm * Gibuf */
	return (28800000 / pcm_offset * aux_diff + 4051) / 8102;
}

void setHpGainZero(void)
{
	Ana_Set_Reg(ZCD_CON2, 0x8 << 7, 0x0f80);
	Ana_Set_Reg(ZCD_CON2, 0x8, 0x001f);
}

void SetSdmLevel(unsigned int level)
{
	/* move to ap side */
}

static void SetHprOffset(int OffsetTrimming)
{
	short Dccompsentation = 0;
	int DCoffsetValue = 0;
	unsigned short RegValue = 0;

	/* 10589 for square wave 34uA i-DAC output current */
	DCoffsetValue = (OffsetTrimming * 10589 + 2048) / 4096;
	/* pr_warn("%s DCoffsetValue = %d\n", __func__, DCoffsetValue); */
	Dccompsentation = DCoffsetValue;
	RegValue = Dccompsentation;
	/* pr_warn("%s RegValue = 0x%x\n", __func__, RegValue); */
	/*Ana_Set_Reg(AFE_DL_DC_COMP_CFG1, RegValue, 0xffff);*/
}

static void SetHplOffset(int OffsetTrimming)
{
	short Dccompsentation = 0;
	int DCoffsetValue = 0;
	unsigned short RegValue = 0;

	/* 10589 for square wave 34uA i-DAC output current */
	DCoffsetValue = (OffsetTrimming * 10589 + 2048) / 4096;
	/* pr_warn("%s DCoffsetValue = %d\n", __func__, DCoffsetValue); */
	Dccompsentation = DCoffsetValue;
	RegValue = Dccompsentation;
	/* pr_warn("%s RegValue = 0x%x\n", __func__, RegValue); */
	/*Ana_Set_Reg(AFE_DL_DC_COMP_CFG0, RegValue, 0xffff);*/
}

static void EnableDcCompensation(bool bEnable)
{
#ifndef EFUSE_HP_TRIM
/*	Ana_Set_Reg(AFE_DL_DC_COMP_CFG2, bEnable, 0x1);*/
#endif
}

static void SetHprOffsetTrim(void)
{
	int OffsetTrimming = mHprTrimOffset - TrimOffset;

	pr_aud("%s OffsetTrimming = %d (mHprTrimOffset(%d)- TrimOffset(%d))\n", __func__,
	       OffsetTrimming, mHprTrimOffset, TrimOffset);
	SetHprOffset(OffsetTrimming);
}

static void SetHpLOffsetTrim(void)
{
	int OffsetTrimming = mHplTrimOffset - TrimOffset;

	pr_aud("%s OffsetTrimming = %d (mHplTrimOffset(%d)- TrimOffset(%d))\n", __func__,
	       OffsetTrimming, mHplTrimOffset, TrimOffset);
	SetHplOffset(OffsetTrimming);
}

static void SetDcCompenSation(void)
{
#ifndef EFUSE_HP_TRIM
	SetHprOffsetTrim();
	SetHpLOffsetTrim();
	EnableDcCompensation(true);
#else				/* use efuse trim */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0800, 0x0800);	/* Enable trim circuit of HP */
	Ana_Set_Reg(AUDDEC_ANA_CON2, RG_AUDHPLTRIM_VAUDP15 << 3, 0x0078);	/* Trim offset voltage of HPL */
	Ana_Set_Reg(AUDDEC_ANA_CON2, RG_AUDHPRTRIM_VAUDP15 << 7, 0x0780);	/* Trim offset voltage of HPR */
	Ana_Set_Reg(AUDDEC_ANA_CON2, RG_AUDHPLFINETRIM_VAUDP15 << 12, 0x3000);	/* Fine trim offset voltage of HPL */
	Ana_Set_Reg(AUDDEC_ANA_CON2, RG_AUDHPRFINETRIM_VAUDP15 << 14, 0xC000);	/* Fine trim offset voltage of HPR */
#endif
}

static int mt63xx_codec_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *Daiport)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_aud("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_CAPTURE rate = %d\n",
		       substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = substream->runtime->rate;

	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_aud("mt63xx_codec_prepare set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n",
		       substream->runtime->rate);
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = substream->runtime->rate;
	}
	return 0;
}

static const struct snd_soc_dai_ops mt6323_aif1_dai_ops = {
	.prepare = mt63xx_codec_prepare,
};

static struct snd_soc_dai_driver mtk_6356_dai_codecs[] = {
	{
	 .name = MT_SOC_CODEC_TXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_DL1_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_RXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .capture = {
		     .stream_name = MT_SOC_UL1_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SOC_HIGH_USE_RATE,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_TDMRX_DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .capture = {
		     .stream_name = MT_SOC_TDM_CAPTURE_STREAM_NAME,
		     .channels_min = 2,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = (SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
				 SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_S16_LE |
				 SNDRV_PCM_FMTBIT_U16_BE | SNDRV_PCM_FMTBIT_S16_BE |
				 SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE |
				 SNDRV_PCM_FMTBIT_U24_BE | SNDRV_PCM_FMTBIT_S24_BE |
				 SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_S24_3LE |
				 SNDRV_PCM_FMTBIT_U24_3BE | SNDRV_PCM_FMTBIT_S24_3BE |
				 SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_S32_LE |
				 SNDRV_PCM_FMTBIT_U32_BE | SNDRV_PCM_FMTBIT_S32_BE),
		     },
	 },
	{
	 .name = MT_SOC_CODEC_I2S0TXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_I2SDL1_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rate_min = 8000,
		      .rate_max = 192000,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      }
	 },
	{
	 .name = MT_SOC_CODEC_VOICE_MD1DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_VOICE_MD1_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_VOICE_MD1_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_VOICE_MD2DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_VOICE_MD2_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_VOICE_MD2_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
#ifdef _NON_COMMON_FEATURE_READY
	{
	 .name = MT_SOC_CODEC_VOICE_ULTRADAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_VOICE_ULTRA_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_VOICE_ULTRA_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
#endif
	{
		.name = MT_SOC_CODEC_VOICE_USBDAI_NAME,
		.ops = &mt6323_aif1_dai_ops,
		.playback = {
			   .stream_name = MT_SOC_VOICE_USB_STREAM_NAME,
			   .channels_min = 1,
			   .channels_max = 2,
			   .rates = SNDRV_PCM_RATE_8000_192000,
			   .formats = SND_SOC_ADV_MT_FMTS,
			   },
		.capture = {
			  .stream_name = MT_SOC_VOICE_USB_STREAM_NAME,
			  .channels_min = 1,
			  .channels_max = 2,
			  .rates = SNDRV_PCM_RATE_8000_192000,
			  .formats = SND_SOC_ADV_MT_FMTS,
			  },
	},
	{
		.name = MT_SOC_CODEC_VOICE_USB_ECHOREF_DAI_NAME,
		.ops = &mt6323_aif1_dai_ops,
		.playback = {
			   .stream_name = MT_SOC_VOICE_USB_ECHOREF_STREAM_NAME,
			   .channels_min = 1,
			   .channels_max = 2,
			   .rates = SNDRV_PCM_RATE_8000_192000,
			   .formats = SND_SOC_ADV_MT_FMTS,
			   },
		.capture = {
			  .stream_name = MT_SOC_VOICE_USB_ECHOREF_STREAM_NAME,
			  .channels_min = 1,
			  .channels_max = 2,
			  .rates = SNDRV_PCM_RATE_8000_192000,
			  .formats = SND_SOC_ADV_MT_FMTS,
			  },
	},
	{
	 .name = MT_SOC_CODEC_FMI2S2RXDAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_FM_I2S2_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_FM_I2S2_RECORD_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_FMMRGTXDAI_DUMMY_DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_FM_MRGTX_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_ULDLLOOPBACK_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_ULDLLOOPBACK_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_ULDLLOOPBACK_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_STUB_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_ROUTING_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_RXDAI2_NAME,
	 .capture = {
		     .stream_name = MT_SOC_UL1DATA2_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_MRGRX_DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_MRGRX_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 8,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 .capture = {
		     .stream_name = MT_SOC_MRGRX_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 8,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_HP_IMPEDANCE_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_HP_IMPEDANCE_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_FM_I2S_DAI_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_FM_I2S_PLAYBACK_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 8,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_TXDAI2_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_DL2_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	 },
	{
	 .name = MT_SOC_CODEC_OFFLOAD_NAME,
	 .ops = &mt6323_aif1_dai_ops,
	 .playback = {
		      .stream_name = MT_SOC_OFFLOAD_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	},
#ifdef _NON_COMMON_FEATURE_READY
	{
		.name = MT_SOC_CODEC_ANC_NAME,
		.ops = &mt6323_aif1_dai_ops,
		.playback = {
			.stream_name = MT_SOC_ANC_STREAM_NAME,
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = SND_SOC_ADV_MT_FMTS,
			},
	}
#endif
};

static void TurnOnDacPower(int device)
{
	pr_warn("TurnOnDacPower\n");
	audckbufEnable(true);

	/* Enable HP main CMFB Switch */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0200, 0xffff);
	/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
	Ana_Set_Reg(AUDDEC_ANA_CON10, 0x00a8, 0xffff);

	switch (device) {
	case AUDIO_ANALOG_DEVICE_OUT_EARPIECEL:
	case AUDIO_ANALOG_DEVICE_OUT_EARPIECER:
		/* Release HS CMFB pull down */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0088, 0xffff);
		break;
	case AUDIO_ANALOG_DEVICE_OUT_SPEAKERL:
	case AUDIO_ANALOG_DEVICE_OUT_SPEAKERR:
		/* Release LO CMFB pull down */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0028, 0xffff);
		break;
	case AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_L:
	case AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R:
		/* Dsiable HP main CMFB Switch */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xffff);
		/* Release HP CMFB pull down */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x00A0, 0xffff);
		/* Release LO CMFB pull down */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0020, 0xffff);
		break;
	case AUDIO_ANALOG_DEVICE_OUT_HEADSETL:
	case AUDIO_ANALOG_DEVICE_OUT_HEADSETR:
	default:
		/* Dsiable HP main CMFB Switch */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xffff);
		/* Release HP CMFB pull down */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x00a0, 0xffff);
		break;
	};


	NvregEnable(true);	/* Enable AUDGLB */

	/* Pull-down HPL/R to AVSS28_AUD */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0xffff);

	ClsqEnable(true);	/* Turn on 26MHz source clock */
	Topck_Enable(true);	/* Turn on AUDNCP_CLKDIV engine clock */
	/* Turn on AUD 26M */
	udelay(250);

	if (GetAdcStatus() == false)
		Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8020, 0xBfff);	/* power on clock */
	else
		Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8000, 0xBfff);	/* power on clock */

	udelay(250);

	/* enable aud_pad RX fifos */
	Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x0031, 0x00ff);

	/* Audio system digital clock power down release */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff);
	/* sdm audio fifo clock power on */
	Ana_Set_Reg(AFUNC_AUD_CON0, 0xCBA1, 0xffff);
	/* scrambler clock on enable */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff);
	/* sdm power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x000B, 0xffff);
	/* sdm fifo enable */

	/* afe enable */
	Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);

	setDlMtkifSrc(true);
}

static void TurnOffDacPower(void)
{
	pr_warn("TurnOffDacPower\n");
	setDlMtkifSrc(false);

	/* DL scrambler disabling sequence */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0000, 0xffff);
	Ana_Set_Reg(AFUNC_AUD_CON0, 0xcba0, 0xffff);

	if (GetAdcStatus() == false)
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);	/* turn off afe */

	/* disable aud_pad RX fifos */
	Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x0000, 0x00ff);

	Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x0040, 0x0040);	/* down-link power down */

	udelay(250);

	Topck_Enable(false);
	ClsqEnable(false);
	NvregEnable(false);
	audckbufEnable(false);
}

static void setDlMtkifSrc(bool enable)
{
	pr_warn("%s(), enable = %d, freq = %d\n",
		__func__,
		enable,
		mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]);
	if (enable) {
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffff);
		/* turn on dl */

		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* set DL in normal path, not from sine gen table */
	} else {
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0000, 0xffff);
		/* bit0, Turn off down-link */
	}
}

static void Audio_Amp_Change(int channels, bool enable)
{
	pr_warn("%s(), enable %d, HSL %d, HSR %d\n",
		__func__,
		enable,
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL],
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR]);

	if (enable) {
		if (GetDLStatus() == false)
			TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_HEADSETL);

		/* here pmic analog control */
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false) {
			/* switch to ground to de pop-noise */
			/*HP_Switch_to_Ground();*/

			/* Disable headphone short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
			/* Disable handset short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
			/* Disable linout short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
			/* Reduce ESD resistance of AU_REFN */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
			/* Set HPR/HPL gain as minimum (~ -40dB) */
			Ana_Set_Reg(ZCD_CON2, 0x0F9F, 0xffff);

			/* Turn on DA_600K_NCP_VA18 */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
			/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
			/* Toggle RG_DIVCKS_CHG */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
			/* Set NCP soft start mode as default mode: 100us */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
			/* Enable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
			udelay(250);

			/* Enable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
			/* Enable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
			udelay(100);

			/* Disable AUD_ZCD */
			Hp_Zcd_Enable(false);

			/* Enable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
			/* Set HP DR bias current optimization, 010: 6uA */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x4900, 0xffff);
			/* Set HP & ZCD bias current optimization */
			/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
			/* Set HPP/N STB enhance circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc033, 0xffff);

			/* Enable HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x000c, 0xffff);
			/* Enable HP aux feedback loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x003c, 0xffff);
			/* Enable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0c00, 0xffff);
			/* Enable HP driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30c0, 0xffff);
			/* Enable HP driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30f0, 0xffff);
			/* Short HP main output to HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00fc, 0xffff);

			/* Enable HP main CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0e00, 0xffff);
			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0200, 0xffff);
			/* Enable HP main output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00ff, 0xffff);
			/* Enable HPR/L main output stage step by step */
			hp_main_output_ramp(true);

			/* Reduce HP aux feedback loop gain */
			hp_aux_feedback_loop_gain_ramp(true);
			/* Disable HP aux feedback loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x3fcf, 0xffff);

			/* apply volume setting */
			HeadsetVoloumeSet();

			/* Disable HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x3fc3, 0xffff);
			/* Unshort HP main output to HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x3f03, 0xffff);
			udelay(100);

			/* Enable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
			/* Enable Audio DAC  */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30ff, 0xffff);
			/* Enable low-noise mode of DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0xf201, 0xffff);
			udelay(100);

			/* Switch HPL MUX to audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x32ff, 0xffff);
			/* Switch HPR MUX to audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3aff, 0xffff);

			/* Apply digital DC compensation value to DAC */
			SetDcCompenSation();

			/* No Pull-down HPL/R to AVSS28_AUD */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4033, 0xffff);
		}

	} else {
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false) {
			/* HPR/HPL mux to open */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

			/* Disable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

			/* decrease HPL/R gain to normal gain step by step */
			HeadsetVoloumeRestore();

			/* decrease HPR/L main output stage step by step */
			hp_main_output_ramp(false);

			/* Disable HP main output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0, 0x3);

			/* Disable HP driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 4);

			/* Disable HP driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 6);

			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

			/* Enable HP main CMFB Switch */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

			/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
			Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
			/* Disable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
			/* Disable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
			/* Disable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

			EnableDcCompensation(false);
		}

		if (GetDLStatus() == false)
			TurnOffDacPower();
	}
}

static int Audio_AmpL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_AmpL_Get = %d\n",
	       mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL];
	return 0;
}

static int Audio_AmpL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);

	pr_aud("%s(): enable = %ld, mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] = %d\n",
			__func__, ucontrol->value.integer.value[0],
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL]);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false)) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static int Audio_AmpR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_AmpR_Get = %d\n",
	       mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR];
	return 0;
}

static int Audio_AmpR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);

	pr_aud("%s(): enable = %ld, mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] = %d\n",
			__func__, ucontrol->value.integer.value[0],
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR]);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false)) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static int PMIC_REG_CLEAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
#if 0
	Ana_Set_Reg(ABB_AFE_CON2, 0, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON4, 0, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON5, 0x28, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON6, 0x218, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON7, 0x204, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON8, 0x0, 0xffff);
	Ana_Set_Reg(ABB_AFE_CON9, 0x0, 0xffff);
	Ana_Set_Reg(AFE_PMIC_NEWIF_CFG1, 0x18, 0xffff);
	Ana_Set_Reg(AFE_PMIC_NEWIF_CFG3, 0xf872, 0xffff);
#endif
	return 0;
}

static int PMIC_REG_CLEAR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), not support\n", __func__);

	return 0;
}
#if 0				/* not used */
static void SetVoiceAmpVolume(void)
{
	int index;

	pr_warn("%s\n", __func__);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
	Ana_Set_Reg(ZCD_CON3, index, 0x001f);
}
#endif

static void Voice_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false) {
			TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_EARPIECEL);
			pr_warn("%s(), amp on\n", __func__);

			/* Disable headphone short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
			/* Disable handset short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
			/* Disable linout short-circuit protection */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
			/* Reduce ESD resistance of AU_REFN */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
			/* Set HS gain as minimum (~ -40dB) */
			Ana_Set_Reg(ZCD_CON2, 0x001F, 0xffff);

			/* Turn on DA_600K_NCP_VA18 */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
			/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
			/* Toggle RG_DIVCKS_CHG */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
			/* Set NCP soft start mode as default mode: 100us */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
			/* Enable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
			udelay(250);

			/* Enable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
			/* Enable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
			udelay(100);

			/* Disable AUD_ZCD */
			Hp_Zcd_Enable(false);

			/* Enable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
			/* Set HP DR bias current optimization, 010: 6uA */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x4900, 0xffff);
			/* Set HP & ZCD bias current optimization */
			/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);

			/* Set HS STB enhance circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0090, 0xffff);
			/* Enable HS driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0092, 0xffff);
			/* Enable HS driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0093, 0xffff);
			/* Set HS gain to normal gain step by step */
			Ana_Set_Reg(ZCD_CON3, 0x9, 0xffff);
			/* Enable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
			/* Enable Audio DAC  */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3009, 0xffff);
			/* Enable low-noise mode of DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0201, 0xffff);
			/* Switch HS MUX to audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x009b, 0xffff);
		}
	} else {
		pr_warn("%s(), amp off\n", __func__);
		/* HS mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x3 << 2);

		if (GetDLStatus() == false) {
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

			/* Disable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

			/* decrease HS gain to minimum gain step by step */
			Ana_Set_Reg(ZCD_CON3, 0x001f, 0xffff);

			/* Disable HS driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0, 0x1);

			/* Disable HS driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x1 << 1);

			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

			/* Enable HP main CMFB Switch */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

			/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
			Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
			/* Disable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
			/* Disable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
			/* Disable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

			TurnOffDacPower();
		}
	}
}

static int Voice_Amp_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Voice_Amp_Get = %d\n",
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL];
	return 0;
}

static int Voice_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);
	pr_aud("%s()\n", __func__);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] == false)) {
		Voice_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EARPIECEL] =
		    ucontrol->value.integer.value[0];
		Voice_Amp_Change(false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

/* For MT6356 LO */
static void Apply_Speaker_Gain(void)
{
	Ana_Set_Reg(ZCD_CON1,
		    (mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTR] << 7) |
		    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTL],
		    0x0f9f);
}

static void Speaker_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false)
			TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_SPEAKERL);

		pr_warn("%s(), enable %d\n", __func__, enable);

		/* Disable headphone short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
		/* Disable handset short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
		/* Disable linout short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
		/* Reduce ESD resistance of AU_REFN */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
		/* Set HS gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON1, 0x0f9f, 0xffff);

		/* Turn on DA_600K_NCP_VA18 */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
		/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
		/* Toggle RG_DIVCKS_CHG */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
		/* Set NCP soft start mode as default mode: 100us */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
		/* Enable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
		udelay(250);

		/* Enable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
		/* Enable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
		udelay(100);

		/* Disable AUD_ZCD */
		Hp_Zcd_Enable(false);

		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Set HP DR bias current optimization, 010: 6uA */
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x4900, 0xffff);
		/* Set HP & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);

		/* Set LO STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0110, 0xffff);
		/* Enable LO driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0112, 0xffff);
		/* Enable LO driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0113, 0xffff);

		/* Set LOL gain to normal gain step by step */
		Apply_Speaker_Gain();

		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
		/* Enable Audio DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3009, 0xffff);
		/* Enable low-noise mode of DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0201, 0xffff);
		/* Switch LOL MUX to audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x011b, 0xffff);
	} else {
		pr_warn("%s(), enable %d\n", __func__, enable);

		/* LOL mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x3 << 2);

		if (GetDLStatus() == false) {
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

			/* Disable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

			/* decrease LOL gain to minimum gain step by step */
			Ana_Set_Reg(ZCD_CON1, 0x0f9f, 0xffff);

			/* Disable LO driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0, 0x1);

			/* Disable LO driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x1 << 1);

			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

			/* Enable HP main CMFB Switch */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

			/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
			Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
			/* Disable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
			/* Disable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
			/* Disable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

			TurnOffDacPower();
		}

	}
}

static int Speaker_Amp_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL];
	return 0;
}

static int Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s() value = %ld\n ", __func__, ucontrol->value.integer.value[0]);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] == false)) {
		Speaker_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKERL] =
		    ucontrol->value.integer.value[0];
		Speaker_Amp_Change(false);
	}
	return 0;
}

static void Ext_Speaker_Amp_Change(bool enable)
{
#define SPK_WARM_UP_TIME        (25)	/* unit is ms */
	if (enable) {
		pr_debug("Ext_Speaker_Amp_Change ON+\n");

		AudDrv_GPIO_EXTAMP_Select(false, 3);

		/*udelay(1000); */
		usleep_range(1 * 1000, 20 * 1000);

		AudDrv_GPIO_EXTAMP_Select(true, 3);

		msleep(SPK_WARM_UP_TIME);

		pr_debug("Ext_Speaker_Amp_Change ON-\n");
	} else {
		pr_debug("Ext_Speaker_Amp_Change OFF+\n");

		AudDrv_GPIO_EXTAMP_Select(false, 3);

		udelay(500);

		pr_debug("Ext_Speaker_Amp_Change OFF-\n");
	}
}

static int Ext_Speaker_Amp_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()\n", __func__);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP];
	return 0;
}

static int Ext_Speaker_Amp_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() gain = %ld\n ", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0]) {
		Ext_Speaker_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_EXTSPKAMP] =
		    ucontrol->value.integer.value[0];
		Ext_Speaker_Amp_Change(false);
	}
	return 0;
}

static void Receiver_Speaker_Switch_Change(bool enable)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_OF
	pr_debug("%s\n", __func__);

	if (enable)
		AudDrv_GPIO_RCVSPK_Select(true);
	else
		AudDrv_GPIO_RCVSPK_Select(false);
#endif
#endif
}

static int Receiver_Speaker_Switch_Get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s() : %d\n", __func__,
		 mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH];
	return 0;
}

static int Receiver_Speaker_Switch_Set(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower
		[AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH] == false)) {
		Receiver_Speaker_Switch_Change(true);
		mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   &&
		   (mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH] == true)) {
		mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH] =
		    ucontrol->value.integer.value[0];
		Receiver_Speaker_Switch_Change(false);
	}
	return 0;
}

static void Headset_Speaker_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false)
			TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_L);

		pr_warn("%s(), enable %d\n", __func__, enable);

		/* Disable headphone short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
		/* Disable handset short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
		/* Disable linout short-circuit protection */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0xffff);
		/* Reduce ESD resistance of AU_REFN */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc000, 0xffff);
		/* Set HS gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON1, 0x0f9f, 0xffff);
		/* Set HPR/HPL gain as minimum (~ -40dB) */
		Ana_Set_Reg(ZCD_CON2, 0x0f9f, 0xffff);

		/* Turn on DA_600K_NCP_VA18 */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0001, 0xffff);
		/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON2, 0x002c, 0xffff);
		/* Toggle RG_DIVCKS_CHG */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0001, 0xffff);
		/* Set NCP soft start mode as default mode: 100us */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON4, 0x0003, 0xffff);
		/* Enable NCP */
		Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0000, 0xffff);
		udelay(250);

		/* Enable cap-less LDOs (1.5V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x1055, 0x1055);
		/* Enable NV regulator (-1.2V) */
		Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0001, 0xffff);
		udelay(100);

		/* Disable AUD_ZCD */
		Hp_Zcd_Enable(false);

		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Set HP DR bias current optimization, 010: 6uA */
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x4900, 0xffff);
		/* Set HP & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0055, 0xffff);
		/* Set HPP/N STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xc033, 0xffff);
		/* No Pull-down HPL/R to AVSS28_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4033, 0xffff);

		/* Enable HP driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30c0, 0xffff);
		/* Enable HP driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30f0, 0xffff);
		/* Enable HP main CMFB loop */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0200, 0xffff);
		/* Enable HP main output stage */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0003, 0xffff);
		/* Enable HPR/L main output stage step by step */
		hp_main_output_ramp(true);

		/* apply volume setting */
		HeadsetVoloumeSet();

		/* HP IVBUF (Vin path) de-gain enable: -12dB */
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x0004, 0xffff);

		/* Set LO STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0110, 0xffff);
		/* Enable LO driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0112, 0xffff);
		/* Enable LO driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0113, 0xffff);

		/* Set LOL gain to normal gain step by step */
		Apply_Speaker_Gain();

		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1, 0x1);
		/* Enable Audio DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x30F9, 0xffff);
		/* Enable low-noise mode of DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0201, 0xffff);
		/* Switch LOL MUX to audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x011b, 0xffff);
		/* Switch HPL/R MUX to Line-out */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x35f9, 0xffff);
	} else {
		pr_warn("%s(), enable %d\n", __func__, enable);
		/* HPR/HPL mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
		/* LOL mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x3 << 2);

		if (GetDLStatus() == false) {
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);

			/* Disable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0, 0x1);

			/* decrease HPL/R gain to normal gain step by step */
			HeadsetVoloumeRestore();

			/* decrease LOL gain to minimum gain step by step */
			Ana_Set_Reg(ZCD_CON1, 0x0f9f, 0xffff);

			/* decrease HPR/L main output stage step by step */
			hp_main_output_ramp(false);

			/* Disable HP main output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0, 0x3);

			/* Disable HP driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 4);
			/* Disable LO driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0, 0x1);

			/* Disable HP driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0, 0x3 << 6);
			/* Disable LO driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x1 << 1);

			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0, 0xff << 8);

			/* Enable HP main CMFB Switch */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x2 << 8, 0xff << 8);

			/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
			Ana_Set_Reg(AUDDEC_ANA_CON10, 0xa8, 0xff);

			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x1 << 8, 0x1 << 8);
			/* Disable NV regulator (-1.2V) */
			Ana_Set_Reg(AUDDEC_ANA_CON15, 0x0, 0x1);
			/* Disable cap-less LDOs (1.5V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0, 0x1055);
			/* Disable NCP */
			Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x1, 0x1);

			TurnOffDacPower();
		}
	}
}


static int Headset_Speaker_Amp_Get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R];
	return 0;
}

static int Headset_Speaker_Amp_Set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol); */

	pr_warn("%s() gain = %lu\n ", __func__, ucontrol->value.integer.value[0]);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower
		[AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R] == false)) {
		Headset_Speaker_Amp_Change(true);
		mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R] = ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   &&
		   (mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R] == true)) {
		mCodec_data->mAudio_Ana_DevicePower
		    [AUDIO_ANALOG_DEVICE_OUT_SPEAKER_HEADSET_R] = ucontrol->value.integer.value[0];
		Headset_Speaker_Amp_Change(false);
	}
	return 0;
}

static int Audio_AuxAdcData_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	pr_warn("%s dMax = 0x%lx\n", __func__, ucontrol->value.integer.value[0]);
	return 0;

}

static int Audio_AuxAdcData_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	dAuxAdcChannel = ucontrol->value.integer.value[0];
	pr_warn("%s dAuxAdcChannel = 0x%x\n", __func__, dAuxAdcChannel);
	return 0;
}


static const struct snd_kcontrol_new Audio_snd_auxadc_controls[] = {
	SOC_SINGLE_EXT("Audio AUXADC Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_AuxAdcData_Get,
		       Audio_AuxAdcData_Set),
};


static const char *const amp_function[] = { "Off", "On" };
static const char *const aud_clk_buf_function[] = { "Off", "On" };

/* static const char *DAC_SampleRate_function[] = {"8000", "11025", "16000", "24000", "32000", "44100", "48000"}; */
static const char *const DAC_DL_PGA_Headset_GAIN[] = {
	"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db",
	"-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

static const char *const DAC_DL_PGA_Handset_GAIN[] = {
	"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db",
	"-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

static const char *const DAC_DL_PGA_Speaker_GAIN[] = {
	"8Db", "7Db", "6Db", "5Db", "4Db", "3Db", "2Db", "1Db", "0Db",
	"-1Db", "-2Db", "-3Db",
	"-4Db", "-5Db", "-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

/* static const char *Voice_Mux_function[] = {"Voice", "Speaker"}; */

static int Lineout_PGAL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Speaker_PGA_Get = %d\n",
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTL]);
	ucontrol->value.integer.value[0] =
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTL];
	return 0;
}

static int Lineout_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	pr_aud("%s(), index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN)) {
		pr_warn("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1))
		index = 0x1f;

	Ana_Set_Reg(ZCD_CON1, index, 0x001f);

	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTL] = index;
	return 0;
}

static int Lineout_PGAR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s  = %d\n", __func__,
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTR]);
	ucontrol->value.integer.value[0] =
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTR];
	return 0;
}

static int Lineout_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	pr_aud("%s(), index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN)) {
		pr_warn("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN) - 1))
		index = 0x1f;

	Ana_Set_Reg(ZCD_CON1, index << 7, 0x0f10);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTR] = index;
	return 0;
}

static int Handset_PGA_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Handset_PGA_Get = %d\n",
	       mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL];
	return 0;
}

static int Handset_PGA_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	pr_aud("%s(), index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN)) {
		pr_warn("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN) - 1))
		index = 0x1f;

	Ana_Set_Reg(ZCD_CON3, index, 0x001f);

	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HSOUTL] = index;
	return 0;
}

static int Headset_PGAL_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Headset_PGAL_Get = %d\n",
	       mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
	return 0;
}

static int Headset_PGAL_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
		index = 0x1f;

	Ana_Set_Reg(ZCD_CON2, index, 0x001f);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = index;
	return 0;
}

static int Headset_PGAR_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Headset_PGAR_Get = %d\n",
	       mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	return 0;
}


static int Headset_PGAR_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	pr_aud("%s(), index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
		index = 0x1f;

	Ana_Set_Reg(ZCD_CON2, index << 7, 0x0f80);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = index;
	return 0;
}

static int codec_adc_sample_rate_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = %d\n", __func__,
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
	ucontrol->value.integer.value[0] = mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC];
	return 0;

}

static int codec_adc_sample_rate_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = ucontrol->value.integer.value[0];
	pr_warn("%s mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC] = %d\n", __func__,
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);
	return 0;
}

static int codec_dac_sample_rate_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = %d\n", __func__,
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]);
	ucontrol->value.integer.value[0] = mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC];
	return 0;

}

static int codec_dac_sample_rate_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = ucontrol->value.integer.value[0];
	pr_warn("%s mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC] = %d\n", __func__,
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC]);
	return 0;
}

static int Aud_Clk_Buf_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("\%s n", __func__);
	ucontrol->value.integer.value[0] = audck_buf_Count;
	return 0;
}

static int Aud_Clk_Buf_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s(), value = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	if (ucontrol->value.integer.value[0])
		audckbufEnable(true);
	else
		audckbufEnable(false);

	return 0;
}

static const struct soc_enum Audio_DL_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	/* here comes pga gain setting */
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN),
			    DAC_DL_PGA_Headset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN),
			    DAC_DL_PGA_Headset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN),
			    DAC_DL_PGA_Handset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN),
			    DAC_DL_PGA_Speaker_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN),
			    DAC_DL_PGA_Speaker_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aud_clk_buf_function),
			    aud_clk_buf_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
};

static const struct snd_kcontrol_new mt6356_snd_controls[] = {
	SOC_ENUM_EXT("Audio_Amp_R_Switch", Audio_DL_Enum[0], Audio_AmpR_Get,
		     Audio_AmpR_Set),
	SOC_ENUM_EXT("Audio_Amp_L_Switch", Audio_DL_Enum[1], Audio_AmpL_Get,
		     Audio_AmpL_Set),
	SOC_ENUM_EXT("Voice_Amp_Switch", Audio_DL_Enum[2], Voice_Amp_Get,
		     Voice_Amp_Set),
	SOC_ENUM_EXT("Speaker_Amp_Switch", Audio_DL_Enum[3],
		     Speaker_Amp_Get, Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_Speaker_Amp_Switch", Audio_DL_Enum[4],
		     Headset_Speaker_Amp_Get,
		     Headset_Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_PGAL_GAIN", Audio_DL_Enum[5],
		     Headset_PGAL_Get, Headset_PGAL_Set),
	SOC_ENUM_EXT("Headset_PGAR_GAIN", Audio_DL_Enum[6],
		     Headset_PGAR_Get, Headset_PGAR_Set),
	SOC_ENUM_EXT("Handset_PGA_GAIN", Audio_DL_Enum[7], Handset_PGA_Get,
		     Handset_PGA_Set),
	SOC_ENUM_EXT("Lineout_PGAR_GAIN", Audio_DL_Enum[8],
		     Lineout_PGAR_Get, Lineout_PGAR_Set),
	SOC_ENUM_EXT("Lineout_PGAL_GAIN", Audio_DL_Enum[9],
		     Lineout_PGAL_Get, Lineout_PGAL_Set),
	SOC_ENUM_EXT("AUD_CLK_BUF_Switch", Audio_DL_Enum[10],
		     Aud_Clk_Buf_Get, Aud_Clk_Buf_Set),
	SOC_ENUM_EXT("Ext_Speaker_Amp_Switch", Audio_DL_Enum[11],
		     Ext_Speaker_Amp_Get,
		     Ext_Speaker_Amp_Set),
	SOC_ENUM_EXT("Receiver_Speaker_Switch", Audio_DL_Enum[11],
		     Receiver_Speaker_Switch_Get,
		     Receiver_Speaker_Switch_Set),
	SOC_ENUM_EXT("PMIC_REG_CLEAR", Audio_DL_Enum[12], PMIC_REG_CLEAR_Get, PMIC_REG_CLEAR_Set),
	SOC_SINGLE_EXT("Codec_ADC_SampleRate", SND_SOC_NOPM, 0, MAX_UL_SAMPLE_RATE, 0,
			codec_adc_sample_rate_get,
			codec_adc_sample_rate_set),
	SOC_SINGLE_EXT("Codec_DAC_SampleRate", SND_SOC_NOPM, 0, MAX_DL_SAMPLE_RATE, 0,
			codec_dac_sample_rate_get,
			codec_dac_sample_rate_set),
};

void SetMicPGAGain(void)
{
	int index = 0;

	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
	pr_aud("%s  AUDIO_ANALOG_VOLUME_MICAMP1 index =%d\n", __func__, index);
	Ana_Set_Reg(AUDENC_ANA_CON0, index << 8, 0x0700);
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
	Ana_Set_Reg(AUDENC_ANA_CON1, index << 8, 0x0700);

}

static bool GetAdcStatus(void)
{
	int i = 0;

	for (i = AUDIO_ANALOG_DEVICE_IN_ADC1; i < AUDIO_ANALOG_DEVICE_MAX; i++) {
		if ((mCodec_data->mAudio_Ana_DevicePower[i] == true)
		    && (i != AUDIO_ANALOG_DEVICE_RECEIVER_SPEAKER_SWITCH))
			return true;
	}
	return false;
}

static bool GetDacStatus(void)
{
	int i = 0;

	for (i = AUDIO_ANALOG_DEVICE_OUT_EARPIECER; i < AUDIO_ANALOG_DEVICE_2IN1_SPK; i++) {
		if (mCodec_data->mAudio_Ana_DevicePower[i] == true)
			return true;
	}
	return false;
}


static bool TurnOnADcPowerACC(int ADCType, bool enable)
{
	pr_warn("%s ADCType = %d enable = %d\n", __func__, ADCType, enable);

	if (enable) {
		if (GetAdcStatus() == false) {
			audckbufEnable(true);

			/* Enable audio globe bias */
			NvregEnable(true);

			/* Enable CLKSQ 26MHz */
			ClsqEnable(true);

			/* set gpio miso mode */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0xffff, 0xffff);
			Ana_Set_Reg(GPIO_MODE3_SET, 0x0249, 0xffff);
			Ana_Set_Reg(GPIO_MODE3, 0x0249, 0xffff);

			/* Enable audio ADC CLKGEN  */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1 << 5, 0x1 << 5);
			/* ADC CLK from CLKGEN (13MHz) */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);
			/* Enable  LCLDO_ENC 1P8V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0100, 0x2500);
			/* LCLDO_ENC remote sense */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x2500, 0x2500);

			/* mic bias */
			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* phone mic */
				/* Enable MICBIAS0, MISBIAS0 = 1P9V */
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0021, 0xffff);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* headset mic */
				/* Enable MICBIAS1, MISBIAS1 = 2P6V */
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0061, 0xffff);
			}

			SetMicPGAGain();
		}

		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {	/* main and headset mic */
			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* "ADC1", main_mic */
				/* Audio L preamplifier input sel : AIN0. Enable audio L PGA */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0xf0ff);
				/* Audio L ADC input sel : L PGA. Enable audio L ADC */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x5041, 0xf000);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* "ADC2", headset mic */
				/* Audio L preamplifier input sel : AIN1. Enable audio L PGA */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x0081, 0xf0ff);
				/* Audio L ADC input sel : L PGA. Enable audio L ADC */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x5081, 0xf000);
			}
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			/* ref mic */
			/* Audio R preamplifier input sel : AIN2. Enable audio R PGA */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x00c1, 0xf0ff);
			/* Audio R ADC input sel : R PGA. Enable audio R ADC */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x50c1, 0xf000);
		}

		if (GetAdcStatus() == false) {
			/* here to set digital part */
			/* AdcClockEnable(true); */
			Topck_Enable(true);

			/* power on clock */
			if (GetDacStatus() == false)
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8040, 0xffff);
			else
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8000, 0xffff);

			/* configure ADC setting */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);

			/* [0] afe enable */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);

			/* MTKAIF TX format setting */
			Ana_Set_Reg(AFE_ADDA_MTKAIF_CFG0, 0x0000, 0xffff);

			/* enable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3100, 0xff00);

			/* UL dmic setting */
			Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xffff);

			/* UL turn on */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);
		}
	} else {
		if (GetAdcStatus() == false) {
			/* UL turn off */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0x0001);

			/* disable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3000, 0xff00);

			if (GetDLStatus() == false) {
				/* afe disable */
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe power down and total audio clk disable */
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
			}

			/* up-link power down */
			Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x0020, 0x0020);

		}

		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xf000);
			/* Audio L ADC input sel : off, disable audio L ADC */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0fff);
			/* Audio L preamplifier input sel : off, Audio L PGA 0 dB gain */
			/* Disable audio L PGA */
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0xf000);
			/* Audio R ADC input sel : off, disable audio R ADC */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x0fff);
			/* Audio R preamplifier input sel : off, Audio R PGA 0 dB gain */
			/* Disable audio R PGA */
		}

		if (GetAdcStatus() == false) {
			/* mic bias */
			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* phone mic */
				/* Disable MICBIAS0, MISBIAS0 = 1P7V */
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xffff);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* headset mic */
				/* Disable MICBIAS1, MISBIAS1 = 1P7V */
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0000, 0xffff);
			}

			/* LCLDO_ENC remote sense off */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0100, 0x2500);
			/* disable LCLDO_ENC 1P8V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x2500);

			/* ADC CLK from CLKGEN (13MHz) */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);
			/* disable audio ADC CLKGEN  */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0 << 5, 0x1 << 5);

			/* set gpio pad_clk_miso to mode 0, for save power */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0x7, 0x7);
			Ana_Set_Reg(GPIO_MODE3, 0x0, 0x7);
			Ana_Set_Reg(GPIO_DIR0, 0x0, 0x1 << 12);

			/* AdcClockEnable(false); */
			Topck_Enable(false);
			/* ClsqAuxEnable(false); */
			ClsqEnable(false);
			NvregEnable(false);
			audckbufEnable(false);
		}

	}
	return true;
}

static bool TurnOnADcPowerDmic(int ADCType, bool enable)
{
	pr_warn("%s(), ADCType = %d enable = %d\n", __func__, ADCType, enable);
	if (enable) {
		if (GetAdcStatus() == false) {
			audckbufEnable(true);

			/* Enable audio globe bias */
			NvregEnable(true);

			/* Enable CLKSQ 26MHz */
			ClsqEnable(true);

			/* set gpio miso mode */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0xffff, 0xffff);
			Ana_Set_Reg(GPIO_MODE3_SET, 0x0249, 0xffff);
			Ana_Set_Reg(GPIO_MODE3, 0x0249, 0xffff);

			/* mic bias */
			/* Enable MICBIAS0, MISBIAS0 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON9, 0x0021, 0xffff);

			/* RG_BANDGAPGEN=1'b0 */
			Ana_Set_Reg(AUDENC_ANA_CON10, 0x0, 0x1 << 12);

			/* DMIC enable */
			Ana_Set_Reg(AUDENC_ANA_CON8, 0x0005, 0xffff);

			/* here to set digital part */
			/* AdcClockEnable(true); */
			Topck_Enable(true);

			/* power on clock */
			if (GetDacStatus() == false)
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8040, 0xffff);
			else
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8000, 0xffff);

			/* configure ADC setting */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);

			/* [0] afe enable */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);

			/* MTKAIF TX format setting */
			Ana_Set_Reg(AFE_ADDA_MTKAIF_CFG0, 0x0000, 0xffff);

			/* enable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3100, 0xff00);

			/* UL dmic setting */
			Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xffff);

			/* UL turn on */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);
		}
	} else {
		if (GetAdcStatus() == false) {
			/* UL turn off */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0x0001);

			/* disable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3000, 0xff00);

			if (GetDLStatus() == false) {
				/* afe disable */
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe power down and total audio clk disable */
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
			}

			/* up-link power down */
			Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x0020, 0x0020);

			/* DMIC disable */
			Ana_Set_Reg(AUDENC_ANA_CON8, 0x0000, 0xffff);

			/* mic bias */
			/* MISBIAS0 = 1P7V */
			Ana_Set_Reg(AUDENC_ANA_CON9, 0x0001, 0xffff);

			/* RG_BANDGAPGEN=1'b0 */
			Ana_Set_Reg(AUDENC_ANA_CON10, 0x0, 0x1 << 12);

			/* MICBIA0 disable */
			Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xffff);

			/* set gpio pad_clk_miso to mode 0, for save power */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0x7, 0x7);
			Ana_Set_Reg(GPIO_MODE3, 0x0, 0x7);
			Ana_Set_Reg(GPIO_DIR0, 0x0, 0x1 << 12);

			/* AdcClockEnable(false); */
			Topck_Enable(false);
			/* ClsqAuxEnable(false); */
			ClsqEnable(false);
			NvregEnable(false);
			audckbufEnable(false);
		}
	}
	return true;
}

static bool TurnOnADcPowerDCC(int ADCType, bool enable, int ECMmode)
{
	pr_warn("%s(), enable %d, ADCType %d, AUDIO_MICSOURCE_MUX_IN_1 %d, ECMmode %d\n",
		__func__,
		enable,
		ADCType,
		mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1],
		ECMmode);

	if (enable) {
		if (GetAdcStatus() == false) {
			audckbufEnable(true);

			/* Enable audio globe bias */
			NvregEnable(true);

			/* Enable CLKSQ 26MHz */
			ClsqEnable(true);

			/* set gpio miso mode */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0xffff, 0xffff);
			Ana_Set_Reg(GPIO_MODE3_SET, 0x0249, 0xffff);
			Ana_Set_Reg(GPIO_MODE3, 0x0249, 0xffff);

			/* Enable audio ADC CLKGEN  */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1 << 5, 0x1 << 5);
			/* ADC CLK from CLKGEN (13MHz) */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);
			/* Enable  LCLDO_ENC 1P8V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0100, 0x2500);
			/* LCLDO_ENC remote sense */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x2500, 0x2500);

			/* DCC 50k CLK (from 26M) */
			Ana_Set_Reg(TOP_CLKSQ_SET, 0x3, 0x3);
			/* AdcClockEnable(true); */
			Topck_Enable(true);
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0xffff);
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0xffff);
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2060, 0xffff);
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2061, 0xffff);
			Ana_Set_Reg(AFE_DCCLK_CFG1, 0x1105, 0xffff);

			/* mic bias */
			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* phone mic */
				/* Enable MICBIAS0, MISBIAS0 = 1P9V */
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0021, 0xffff);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* headset mic */
				/* Enable MICBIAS1, MISBIAS1 = 2P6V */
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0061, 0xffff);
			}

			SetMicPGAGain();
		}

		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {	/* main and headset mic */
			/* Audio L preamplifier DCC precharge */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0004, 0xffff);

			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* "ADC1", main_mic */
				/* Audio L preamplifier input sel : AIN0. Enable audio L PGA */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x0041, 0xf0ff);
				/* Audio L preamplifier DCCEN */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x1 << 1, 0x1 << 1);
				/* Audio L ADC input sel : L PGA. Enable audio L ADC */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x5041, 0xf000);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* "ADC2", headset mic */
				/* Audio L preamplifier input sel : AIN1. Enable audio L PGA */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x0081, 0xf0ff);
				/* Audio L preamplifier DCCEN */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x1 << 1, 0x1 << 1);
				/* Audio L ADC input sel : L PGA. Enable audio L ADC */
				Ana_Set_Reg(AUDENC_ANA_CON0, 0x5081, 0xf000);
			}
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			/* Audio R preamplifier DCC precharge */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0004, 0xffff);

			/* ref mic */
			/* Audio R preamplifier input sel : AIN2. Enable audio R PGA */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x00c1, 0xf0ff);
			/* Audio R preamplifier DCCEN */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x1 << 1, 0x1 << 1);
			/* Audio R ADC input sel : R PGA. Enable audio R ADC */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x50c1, 0xf000);
		}

		if (GetAdcStatus() == false) {

			/* Short body to VCM in PGA */
			Ana_Set_Reg(AUDENC_ANA_CON7, 0x0400, 0xffff);
			usleep_range(100, 150);

			/* Audio R preamplifier DCC precharge off */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0, 0x1 << 2);
			/* Audio L preamplifier DCC precharge off */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0, 0x1 << 2);

			/* here to set digital part */
			/* power on clock */
			if (GetDacStatus() == false)
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8040, 0xffff);
			else
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8000, 0xffff);

			/* configure ADC setting */
			Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);

			/* [0] afe enable */
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);

			/* MTKAIF TX format setting */
			Ana_Set_Reg(AFE_ADDA_MTKAIF_CFG0, 0x0000, 0xffff);

			/* enable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3100, 0xff00);

			/* UL dmic setting */
			Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xffff);

			/* UL turn on */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);
		}
	} else {
		if (GetAdcStatus() == false) {
			/* UL turn off */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0x0001);

			/* disable aud_pad TX fifos */
			Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3000, 0xff00);

			if (GetDLStatus() == false) {
				/* afe disable */
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe power down and total audio clk disable */
				Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
			}

			/* up-link power down */
			Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x0020, 0x0020);

			/* Disable Short body to VCM in PGA */
			Ana_Set_Reg(AUDENC_ANA_CON7, 0x0000, 0xffff);
		}

		if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC1) {
			/* Audio L ADC input sel : off, disable audio L ADC */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xf000);
			/* Audio L preamplifier DCCEN */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0 << 1, 0x1 << 1);
			/* Audio L preamplifier input sel : off, Audio L PGA 0 dB gain */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xfffb);
			/* Disable audio L PGA */

			/* disable Audio L preamplifier DCC precharge */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0, 0x1 << 2);
		} else if (ADCType == AUDIO_ANALOG_DEVICE_IN_ADC2) {
			/* Audio R ADC input sel : off, disable audio R ADC */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0xf000);
			/* Audio r preamplifier DCCEN */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0 << 1, 0x1 << 1);
			/* Audio R preamplifier input sel : off, Audio R PGA 0 dB gain */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x0ffb);
			/* Disable audio R PGA */

			/* disable Audio R preamplifier DCC precharge */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0, 0x1 << 2);
		}

		if (GetAdcStatus() == false) {
			/* mic bias */
			if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 0) {
				/* phone mic */
				/* Disable MICBIAS0, MISBIAS0 = 1P7V */
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0xffff);
			} else if (mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] == 1) {
				/* headset mic */
				/* Disable MICBIAS1, MISBIAS1 = 1P7V */
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0000, 0xffff);
			}

			/* dcclk_gen_on=1'b0 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2060, 0xffff);
			/* dcclk_pdn=1'b1 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0xffff);
			/* dcclk_ref_ck_sel=2'b00 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0xffff);
			/* dcclk_div=11'b00100000011 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2062, 0xffff);

			/* LCLDO_ENC remote sense off */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0100, 0x2500);
			/* disable LCLDO_ENC 1P8V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x2500);

			/* ADC CLK from CLKGEN (13MHz) */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);
			/* disable audio ADC CLKGEN  */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0 << 5, 0x1 << 5);

			/* set gpio pad_clk_miso to mode 0, for save power */
			Ana_Set_Reg(GPIO_MODE3_CLR, 0x7, 0x7);
			Ana_Set_Reg(GPIO_MODE3, 0x0, 0x7);
			Ana_Set_Reg(GPIO_DIR0, 0x0, 0x1 << 12);

			/* AdcClockEnable(false); */
			Topck_Enable(false);
			/* ClsqAuxEnable(false); */
			ClsqEnable(false);
			NvregEnable(false);
			audckbufEnable(false);
		}
	}
	return true;
}

/* here start uplink power function */
static const char *const ADC_function[] = { "Off", "On" };
static const char *const ADC_power_mode[] = { "normal", "lowpower" };
static const char *const PreAmp_Mux_function[] = { "OPEN", "IN_ADC1", "IN_ADC2", "IN_ADC3" };

/* OPEN:0, IN_ADC1: 1, IN_ADC2:2, IN_ADC3:3 */
static const char *const ADC_UL_PGA_GAIN[] = { "0Db", "6Db", "12Db", "18Db", "24Db", "30Db" };
static const char *const Pmic_Digital_Mux[] = { "ADC1", "ADC2", "ADC3", "ADC4" };
static const char *const Adc_Input_Sel[] = { "idle", "AIN", "Preamp" };

static const char *const Audio_AnalogMic_Mode[] = {
	"ACCMODE", "DCCMODE", "DMIC", "DCCECMDIFFMODE", "DCCECMSINGLEMODE"
};

/* here start uplink power function */
static const char * const Pmic_Test_function[] = { "Off", "On" };
static const char * const Pmic_LPBK_function[] = { "Off", "LPBK3" };

static const struct soc_enum Audio_UL_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(PreAmp_Mux_function),
			    PreAmp_Mux_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Adc_Input_Sel), Adc_Input_Sel),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Digital_Mux), Pmic_Digital_Mux),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode),
			    Audio_AnalogMic_Mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode),
			    Audio_AnalogMic_Mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode),
			    Audio_AnalogMic_Mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_AnalogMic_Mode),
			    Audio_AnalogMic_Mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_power_mode), ADC_power_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(PreAmp_Mux_function),
			    PreAmp_Mux_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
};

static int Audio_ADC1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_ADC1_Get = %d\n",
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1];
	return 0;
}

static int Audio_ADC1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()\n", __func__);
	mutex_lock(&Ana_Power_Mutex);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC)
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, true, 0);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC)
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, true);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, true, 1);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, true, 2);

		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC1] =
		    ucontrol->value.integer.value[0];
		if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_ACC)
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCC)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, false, 0);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DMIC)
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC1, false);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, false, 1);
		else if (mAudio_Analog_Mic1_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC1, false, 2);

	}
	mutex_unlock(&Ana_Power_Mutex);
	return 0;
}

static int Audio_ADC2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_ADC2_Get = %d\n",
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2];
	return 0;
}

static int Audio_ADC2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()\n", __func__);
	mutex_lock(&Ana_Power_Mutex);
	if (ucontrol->value.integer.value[0]) {
		if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC)
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2, true);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, true, 0);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC)
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2, true);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, true, 1);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, true, 2);

		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] =
		    ucontrol->value.integer.value[0];
	} else {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_IN_ADC2] =
		    ucontrol->value.integer.value[0];
		if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_ACC)
			TurnOnADcPowerACC(AUDIO_ANALOG_DEVICE_IN_ADC2, false);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCC)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, false, 0);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DMIC)
			TurnOnADcPowerDmic(AUDIO_ANALOG_DEVICE_IN_ADC2, false);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMDIFF)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, false, 1);
		else if (mAudio_Analog_Mic2_mode == AUDIO_ANALOGUL_MODE_DCCECMSINGLE)
			TurnOnADcPowerDCC(AUDIO_ANALOG_DEVICE_IN_ADC2, false, 2);

	}
	mutex_unlock(&Ana_Power_Mutex);
	return 0;
}

static int Audio_ADC3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC1_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1];
	return 0;
}

static int Audio_ADC1_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0)
		Ana_Set_Reg(AUDENC_ANA_CON0, (0x00 << 13), 0x6000);	/* pinumx sel */
	else if (ucontrol->value.integer.value[0] == 1)
		Ana_Set_Reg(AUDENC_ANA_CON0, (0x01 << 13), 0x6000);	/* AIN0 */
	else if (ucontrol->value.integer.value[0] == 2)
		Ana_Set_Reg(AUDENC_ANA_CON0, (0x02 << 13), 0x6000);	/* Left preamp */
	else
		pr_warn("%s() [AudioWarn]\n ", __func__);

	pr_warn("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_ADC2_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s() = %d\n", __func__, mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2];
	return 0;
}

static int Audio_ADC2_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Adc_Input_Sel)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0)
		Ana_Set_Reg(AUDENC_ANA_CON1, (0x00 << 13), 0x6000);	/* pinumx sel */
	else if (ucontrol->value.integer.value[0] == 1)
		Ana_Set_Reg(AUDENC_ANA_CON1, (0x01 << 13), 0x6000);	/* AIN0 */
	else if (ucontrol->value.integer.value[0] == 2)
		Ana_Set_Reg(AUDENC_ANA_CON1, (0x02 << 13), 0x6000);	/* Right preamp */
	else
		pr_warn("%s() [AudioWarn]\n ", __func__);

	pr_warn("%s() done\n", __func__);
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2] = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_ADC3_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC3_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC4_Sel_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_ADC4_Sel_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}


static bool AudioPreAmp1_Sel(int Mul_Sel)
{
	pr_aud("%s Mul_Sel = %d ", __func__, Mul_Sel);
	if (Mul_Sel == 0)
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0030);	/* pinumx open */
	else if (Mul_Sel == 1)
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0010, 0x0030);	/* AIN0 */
	else if (Mul_Sel == 2)
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0020, 0x0030);	/* AIN1 */
	else if (Mul_Sel == 3)
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0030, 0x0030);	/* AIN2 */
	else
		pr_warn("[AudioWarn] AudioPreAmp1_Sel Mul_Sel=%d", Mul_Sel);

	return true;
}


static int Audio_PreAmp1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1];
	return 0;
}

static int Audio_PreAmp1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(PreAmp_Mux_function)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1] =
	    ucontrol->value.integer.value[0];
	AudioPreAmp1_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_1]);

	return 0;
}

static bool AudioPreAmp2_Sel(int Mul_Sel)
{
	pr_aud("%s Mul_Sel = %d ", __func__, Mul_Sel);

	if (Mul_Sel == 0)
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x0030);	/* pinumx open */
	else if (Mul_Sel == 1)
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0010, 0x0030);	/* AIN0 */
	else if (Mul_Sel == 2)
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0020, 0x0030);	/* AIN3 */
	else if (Mul_Sel == 3)
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0030, 0x0030);	/* AIN2 */
	else
		pr_warn("[AudioWarn] AudioPreAmp1_Sel, Mul_Sel=%d", Mul_Sel);

	return true;
}


static int Audio_PreAmp2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2];
	return 0;
}

static int Audio_PreAmp2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(PreAmp_Mux_function)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2] =
	    ucontrol->value.integer.value[0];
	AudioPreAmp2_Sel(mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_PREAMP_2]);
	return 0;
}

/* PGA1: PGA_L */
static int Audio_PGA1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_AmpR_Get = %d\n",
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];
	return 0;
}

static int Audio_PGA1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDENC_ANA_CON0, (index << 8), 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] =
	    ucontrol->value.integer.value[0];
	return 0;
}

/* PGA2: PGA_R */
static int Audio_PGA2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_PGA2_Get = %d\n",
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2]);
	ucontrol->value.integer.value[0] =
	    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];
	return 0;
}

static int Audio_PGA2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AUDENC_ANA_CON1, index << 8, 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] =
	    ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_PGA3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_PGA3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_PGA4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_PGA4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_MicSource1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("Audio_MicSource1_Get = %d\n",
		mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1]);
	ucontrol->value.integer.value[0] = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];
	return 0;
}

static int Audio_MicSource1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* ADC1 Mic source selection, "ADC1" is main_mic, "ADC2" is headset_mic */
	int index = 0;

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Digital_Mux)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	pr_aud("%s() index = %d done\n", __func__, index);
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] = ucontrol->value.integer.value[0];

	return 0;
}

static int Audio_MicSource2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_MicSource2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_MicSource3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_MicSource3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}


static int Audio_MicSource4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

static int Audio_MicSource4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 removed */
	return 0;
}

/* Mic ACC/DCC Mode Setting */
static int Audio_Mic1_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic1_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic1_mode;
	return 0;
}

static int Audio_Mic1_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic1_mode = ucontrol->value.integer.value[0];
	pr_aud("%s() mAudio_Analog_Mic1_mode = %d\n", __func__, mAudio_Analog_Mic1_mode);
	return 0;
}

static int Audio_Mic2_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()  = %d\n", __func__, mAudio_Analog_Mic2_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic2_mode;
	return 0;
}

static int Audio_Mic2_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic2_mode = ucontrol->value.integer.value[0];
	pr_aud("%s() mAudio_Analog_Mic2_mode = %d\n", __func__, mAudio_Analog_Mic2_mode);
	return 0;
}


static int Audio_Mic3_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()  = %d\n", __func__, mAudio_Analog_Mic3_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic3_mode;
	return 0;
}

static int Audio_Mic3_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic3_mode = ucontrol->value.integer.value[0];
	pr_aud("%s() mAudio_Analog_Mic3_mode = %d\n", __func__, mAudio_Analog_Mic3_mode);
	return 0;
}

static int Audio_Mic4_Mode_Select_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s()  = %d\n", __func__, mAudio_Analog_Mic4_mode);
	ucontrol->value.integer.value[0] = mAudio_Analog_Mic4_mode;
	return 0;
}

static int Audio_Mic4_Mode_Select_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_AnalogMic_Mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Analog_Mic4_mode = ucontrol->value.integer.value[0];
	pr_aud("%s() mAudio_Analog_Mic4_mode = %d\n", __func__, mAudio_Analog_Mic4_mode);
	return 0;
}

static int Audio_Adc_Power_Mode_Get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()  = %d\n", __func__, mAdc_Power_Mode);
	ucontrol->value.integer.value[0] = mAdc_Power_Mode;
	return 0;
}

static int Audio_Adc_Power_Mode_Set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_power_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAdc_Power_Mode = ucontrol->value.integer.value[0];
	pr_warn("%s() mAdc_Power_Mode = %d\n", __func__, mAdc_Power_Mode);
	return 0;
}

static bool ul_lr_swap_enable;

static int Audio_UL_LR_Swap_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ul_lr_swap_enable;
	return 0;
}

static int Audio_UL_LR_Swap_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	ul_lr_swap_enable = ucontrol->value.integer.value[0];
	Ana_Set_Reg(AFE_UL_DL_CON0, ul_lr_swap_enable << 15, 0x1 << 15);
	return 0;
}

static bool SineTable_DAC_HP_flag;
static bool SineTable_UL2_flag;

static int SineTable_UL2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.integer.value[0]) {
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0002, 0x2);	/* set UL from sinetable */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0080, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff);
	} else {
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0x2);	/* set UL from normal path */
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff);
	}
	SineTable_UL2_flag = ucontrol->value.integer.value[0];
	return 0;
}

static int SineTable_UL2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = SineTable_UL2_flag;
	return 0;
}

static int32 Pmic_Loopback_Type;

#define PMIC_AUDIO_REG_COUNT 100
#define PMIC_REG_BASE          (0x0)
#define PMIC_AUD_TOP_ID                    ((UINT32)(PMIC_REG_BASE + 0x1540))
#define PMIC_AUD_TOP_REV0                  ((UINT32)(PMIC_REG_BASE + 0x1542))
#define PMIC_AUD_TOP_REV1                  ((UINT32)(PMIC_REG_BASE + 0x1544))
#define PMIC_AUD_TOP_CKPDN_PM0             ((UINT32)(PMIC_REG_BASE + 0x1546))
#define PMIC_AUD_TOP_CKPDN_PM1             ((UINT32)(PMIC_REG_BASE + 0x1548))
#define PMIC_AUD_TOP_CKPDN_CON0            ((UINT32)(PMIC_REG_BASE + 0x154a))
#define PMIC_AUD_TOP_CKSEL_CON0            ((UINT32)(PMIC_REG_BASE + 0x154c))
#define PMIC_AUD_TOP_CKTST_CON0            ((UINT32)(PMIC_REG_BASE + 0x154e))
#define PMIC_AUD_TOP_RST_CON0              ((UINT32)(PMIC_REG_BASE + 0x1550))
#define PMIC_AUD_TOP_RST_BANK_CON0         ((UINT32)(PMIC_REG_BASE + 0x1552))
#define PMIC_AUD_TOP_INT_CON0              ((UINT32)(PMIC_REG_BASE + 0x1554))
#define PMIC_AUD_TOP_INT_CON0_SET          ((UINT32)(PMIC_REG_BASE + 0x1556))
#define PMIC_AUD_TOP_INT_CON0_CLR          ((UINT32)(PMIC_REG_BASE + 0x1558))
#define PMIC_AUD_TOP_INT_MASK_CON0         ((UINT32)(PMIC_REG_BASE + 0x155a))
#define PMIC_AUD_TOP_INT_MASK_CON0_SET     ((UINT32)(PMIC_REG_BASE + 0x155c))
#define PMIC_AUD_TOP_INT_MASK_CON0_CLR     ((UINT32)(PMIC_REG_BASE + 0x155e))
#define PMIC_AUD_TOP_INT_STATUS0           ((UINT32)(PMIC_REG_BASE + 0x1560))
#define PMIC_AUD_TOP_INT_RAW_STATUS0       ((UINT32)(PMIC_REG_BASE + 0x1562))
#define PMIC_AUD_TOP_INT_MISC_CON0         ((UINT32)(PMIC_REG_BASE + 0x1564))
#define PMIC_AUDNCP_CLKDIV_CON0            ((UINT32)(PMIC_REG_BASE + 0x1566))
#define PMIC_AUDNCP_CLKDIV_CON1            ((UINT32)(PMIC_REG_BASE + 0x1568))
#define PMIC_AUDNCP_CLKDIV_CON2            ((UINT32)(PMIC_REG_BASE + 0x156a))
#define PMIC_AUDNCP_CLKDIV_CON3            ((UINT32)(PMIC_REG_BASE + 0x156c))
#define PMIC_AUDNCP_CLKDIV_CON4            ((UINT32)(PMIC_REG_BASE + 0x156e))
#define PMIC_AUD_TOP_MON_CON0              ((UINT32)(PMIC_REG_BASE + 0x1570))
#define PMIC_AUDIO_DIG_ID                  ((UINT32)(PMIC_REG_BASE + 0x1580))
#define PMIC_AUDIO_DIG_REV0                ((UINT32)(PMIC_REG_BASE + 0x1582))
#define PMIC_AUDIO_DIG_REV1                ((UINT32)(PMIC_REG_BASE + 0x1584))
#define PMIC_AFE_UL_DL_CON0                ((UINT32)(PMIC_REG_BASE + 0x1586))
#define PMIC_AFE_DL_SRC2_CON0_L            ((UINT32)(PMIC_REG_BASE + 0x1588))
#define PMIC_AFE_UL_SRC_CON0_H             ((UINT32)(PMIC_REG_BASE + 0x158a))
#define PMIC_AFE_UL_SRC_CON0_L             ((UINT32)(PMIC_REG_BASE + 0x158c))
#define PMIC_AFE_TOP_CON0                  ((UINT32)(PMIC_REG_BASE + 0x158e))
#define PMIC_AUDIO_TOP_CON0                ((UINT32)(PMIC_REG_BASE + 0x1590))
#define PMIC_AFE_MON_DEBUG0                ((UINT32)(PMIC_REG_BASE + 0x1592))
#define PMIC_AFUNC_AUD_CON0                ((UINT32)(PMIC_REG_BASE + 0x1594))
#define PMIC_AFUNC_AUD_CON1                ((UINT32)(PMIC_REG_BASE + 0x1596))
#define PMIC_AFUNC_AUD_CON2                ((UINT32)(PMIC_REG_BASE + 0x1598))
#define PMIC_AFUNC_AUD_CON3                ((UINT32)(PMIC_REG_BASE + 0x159a))
#define PMIC_AFUNC_AUD_CON4                ((UINT32)(PMIC_REG_BASE + 0x159c))
#define PMIC_AFUNC_AUD_MON0                ((UINT32)(PMIC_REG_BASE + 0x159e))
#define PMIC_AUDRC_TUNE_MON0               ((UINT32)(PMIC_REG_BASE + 0x15a0))
#define PMIC_AFE_ADDA_MTKAIF_FIFO_CFG0     ((UINT32)(PMIC_REG_BASE + 0x15a2))
#define PMIC_AFE_ADDA_MTKAIF_FIFO_LOG_MON1 ((UINT32)(PMIC_REG_BASE + 0x15a4))
#define PMIC_AFE_ADDA_MTKAIF_MON0          ((UINT32)(PMIC_REG_BASE + 0x15a6))
#define PMIC_AFE_ADDA_MTKAIF_MON1          ((UINT32)(PMIC_REG_BASE + 0x15a8))
#define PMIC_AFE_ADDA_MTKAIF_MON2          ((UINT32)(PMIC_REG_BASE + 0x15aa))
#define PMIC_AFE_ADDA_MTKAIF_MON3          ((UINT32)(PMIC_REG_BASE + 0x15ac))
#define PMIC_AFE_ADDA_MTKAIF_CFG0          ((UINT32)(PMIC_REG_BASE + 0x15ae))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG0       ((UINT32)(PMIC_REG_BASE + 0x15b0))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG1       ((UINT32)(PMIC_REG_BASE + 0x15b2))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG2       ((UINT32)(PMIC_REG_BASE + 0x15b4))
#define PMIC_AFE_ADDA_MTKAIF_RX_CFG3       ((UINT32)(PMIC_REG_BASE + 0x15b6))
#define PMIC_AFE_ADDA_MTKAIF_TX_CFG1       ((UINT32)(PMIC_REG_BASE + 0x15b8))
#define PMIC_AFE_SGEN_CFG0                 ((UINT32)(PMIC_REG_BASE + 0x15ba))
#define PMIC_AFE_SGEN_CFG1                 ((UINT32)(PMIC_REG_BASE + 0x15bc))
#define PMIC_AFE_ADC_ASYNC_FIFO_CFG        ((UINT32)(PMIC_REG_BASE + 0x15be))
#define PMIC_AFE_DCCLK_CFG0                ((UINT32)(PMIC_REG_BASE + 0x15c0))
#define PMIC_AFE_DCCLK_CFG1                ((UINT32)(PMIC_REG_BASE + 0x15c2))
#define PMIC_AUDIO_DIG_CFG                 ((UINT32)(PMIC_REG_BASE + 0x15c4))
#define PMIC_AFE_AUD_PAD_TOP               ((UINT32)(PMIC_REG_BASE + 0x15c6))
#define PMIC_AFE_AUD_PAD_TOP_MON           ((UINT32)(PMIC_REG_BASE + 0x15c8))
#define PMIC_AFE_AUD_PAD_TOP_MON1          ((UINT32)(PMIC_REG_BASE + 0x15ca))
#define PMIC_AUDENC_DSN_ID                 ((UINT32)(PMIC_REG_BASE + 0x1600))
#define PMIC_AUDENC_DSN_REV0               ((UINT32)(PMIC_REG_BASE + 0x1602))
#define PMIC_AUDENC_DSN_REV1               ((UINT32)(PMIC_REG_BASE + 0x1604))
#define PMIC_AUDENC_ANA_CON0               ((UINT32)(PMIC_REG_BASE + 0x1606))
#define PMIC_AUDENC_ANA_CON1               ((UINT32)(PMIC_REG_BASE + 0x1608))
#define PMIC_AUDENC_ANA_CON2               ((UINT32)(PMIC_REG_BASE + 0x160a))
#define PMIC_AUDENC_ANA_CON3               ((UINT32)(PMIC_REG_BASE + 0x160c))
#define PMIC_AUDENC_ANA_CON4               ((UINT32)(PMIC_REG_BASE + 0x160e))
#define PMIC_AUDENC_ANA_CON5               ((UINT32)(PMIC_REG_BASE + 0x1610))
#define PMIC_AUDENC_ANA_CON6               ((UINT32)(PMIC_REG_BASE + 0x1612))
#define PMIC_AUDENC_ANA_CON7               ((UINT32)(PMIC_REG_BASE + 0x1614))
#define PMIC_AUDENC_ANA_CON8               ((UINT32)(PMIC_REG_BASE + 0x1616))
#define PMIC_AUDENC_ANA_CON9               ((UINT32)(PMIC_REG_BASE + 0x1618))
#define PMIC_AUDENC_ANA_CON10              ((UINT32)(PMIC_REG_BASE + 0x161a))
#define PMIC_AUDENC_ANA_CON11              ((UINT32)(PMIC_REG_BASE + 0x161c))
#define PMIC_AUDENC_ANA_CON12              ((UINT32)(PMIC_REG_BASE + 0x161e))
#define PMIC_AUDDEC_DSN_ID                 ((UINT32)(PMIC_REG_BASE + 0x1640))
#define PMIC_AUDDEC_DSN_REV0               ((UINT32)(PMIC_REG_BASE + 0x1642))
#define PMIC_AUDDEC_DSN_REV1               ((UINT32)(PMIC_REG_BASE + 0x1644))
#define PMIC_AUDDEC_ANA_CON0               ((UINT32)(PMIC_REG_BASE + 0x1646))
#define PMIC_AUDDEC_ANA_CON1               ((UINT32)(PMIC_REG_BASE + 0x1648))
#define PMIC_AUDDEC_ANA_CON2               ((UINT32)(PMIC_REG_BASE + 0x164a))
#define PMIC_AUDDEC_ANA_CON3               ((UINT32)(PMIC_REG_BASE + 0x164c))
#define PMIC_AUDDEC_ANA_CON4               ((UINT32)(PMIC_REG_BASE + 0x164e))
#define PMIC_AUDDEC_ANA_CON5               ((UINT32)(PMIC_REG_BASE + 0x1650))
#define PMIC_AUDDEC_ANA_CON6               ((UINT32)(PMIC_REG_BASE + 0x1652))
#define PMIC_AUDDEC_ANA_CON7               ((UINT32)(PMIC_REG_BASE + 0x1654))
#define PMIC_AUDDEC_ANA_CON8               ((UINT32)(PMIC_REG_BASE + 0x1656))
#define PMIC_AUDDEC_ANA_CON9               ((UINT32)(PMIC_REG_BASE + 0x1658))
#define PMIC_AUDDEC_ANA_CON10              ((UINT32)(PMIC_REG_BASE + 0x165a))
#define PMIC_AUDDEC_ANA_CON11              ((UINT32)(PMIC_REG_BASE + 0x165c))
#define PMIC_AUDDEC_ANA_CON12              ((UINT32)(PMIC_REG_BASE + 0x165e))
#define PMIC_AUDDEC_ANA_CON13              ((UINT32)(PMIC_REG_BASE + 0x1660))
#define PMIC_AUDDEC_ANA_CON14              ((UINT32)(PMIC_REG_BASE + 0x1662))
#define PMIC_AUDDEC_ANA_CON15              ((UINT32)(PMIC_REG_BASE + 0x1664))
#define PMIC_AUDDEC_ELR_NUM                ((UINT32)(PMIC_REG_BASE + 0x1666))
#define PMIC_AUDDEC_ELR_0                  ((UINT32)(PMIC_REG_BASE + 0x1668))

#define PMIC_TOP_CLKSQ_SET (0x134)

#define GPIO_DOUT0 0x64
#define SMT_CON0 0x2c
#define SMT_CON1 0x2e
#define FILTER_CON0 0x3c

typedef struct {
	unsigned int offset;
	unsigned int readMask;
	unsigned int writeMask;
	unsigned int defaultvalue;
	unsigned int regid;
	char *name;
} mem_rw_table_struct;

const mem_rw_table_struct u4PMICAudioRegTable[PMIC_AUDIO_REG_COUNT] = {
	{0x1540, 0xffff, 0x0000, 0xc900, PMIC_AUD_TOP_ID, "PMIC_AUD_TOP_ID"},
	{0x1542, 0xffff, 0x0000, 0x1000, PMIC_AUD_TOP_REV0, "PMIC_AUD_TOP_REV0"},
	{0x1544, 0xffff, 0x0000, 0x0010, PMIC_AUD_TOP_REV1, "PMIC_AUD_TOP_REV1"},
	{0x1546, 0xffff, 0x0000, 0x100a, PMIC_AUD_TOP_CKPDN_PM0, "PMIC_AUD_TOP_CKPDN_PM0"},
	{0x1548, 0xffff, 0x0000, 0x0814, PMIC_AUD_TOP_CKPDN_PM1, "PMIC_AUD_TOP_CKPDN_PM1"},
	{0x154a, 0x0067, 0x0067, 0x0000, PMIC_AUD_TOP_CKPDN_CON0, "PMIC_AUD_TOP_CKPDN_CON0"},
	{0x154c, 0x000c, 0x000c, 0x0000, PMIC_AUD_TOP_CKSEL_CON0, "PMIC_AUD_TOP_CKSEL_CON0"},
	{0x154e, 0x001d, 0x001d, 0x0000, PMIC_AUD_TOP_CKTST_CON0, "PMIC_AUD_TOP_CKTST_CON0"},
	{0x1554, 0x00e1, 0x00e1, 0x0000, PMIC_AUD_TOP_INT_CON0, "PMIC_AUD_TOP_INT_CON0"},
	{0x1556, 0x0000, 0xffff, 0x0000, PMIC_AUD_TOP_INT_CON0_SET, "PMIC_AUD_TOP_INT_CON0_SET"},
	{0x1558, 0x0000, 0xffff, 0x0000, PMIC_AUD_TOP_INT_CON0_CLR, "PMIC_AUD_TOP_INT_CON0_CLR"},
	{0x155a, 0x00e1, 0x00e1, 0x0000, PMIC_AUD_TOP_INT_MASK_CON0, "PMIC_AUD_TOP_INT_MASK_CON0"},
	{0x155c, 0x0000, 0xffff, 0x0000, PMIC_AUD_TOP_INT_MASK_CON0_SET, "PMIC_AUD_TOP_INT_MASK_CON0_SET"},
	{0x155e, 0x0000, 0xffff, 0x0000, PMIC_AUD_TOP_INT_MASK_CON0_CLR, "PMIC_AUD_TOP_INT_MASK_CON0_CLR"},
	{0x1560, 0x00e1, 0x0000, 0x0000, PMIC_AUD_TOP_INT_STATUS0, "PMIC_AUD_TOP_INT_STATUS0"},
	{0x1562, 0x00e1, 0x0000, 0x0000, PMIC_AUD_TOP_INT_RAW_STATUS0, "PMIC_AUD_TOP_INT_RAW_STATUS0"},
	{0x1564, 0x0001, 0x0001, 0x0000, PMIC_AUD_TOP_INT_MISC_CON0, "PMIC_AUD_TOP_INT_MISC_CON0"},
	{0x1566, 0x0001, 0x0001, 0x0000, PMIC_AUDNCP_CLKDIV_CON0, "PMIC_AUDNCP_CLKDIV_CON0"},
	{0x1568, 0x0001, 0x0001, 0x0000, PMIC_AUDNCP_CLKDIV_CON1, "PMIC_AUDNCP_CLKDIV_CON1"},
	{0x156a, 0x01ff, 0x01ff, 0x002c, PMIC_AUDNCP_CLKDIV_CON2, "PMIC_AUDNCP_CLKDIV_CON2"},
	{0x156c, 0x0001, 0x0001, 0x0001, PMIC_AUDNCP_CLKDIV_CON3, "PMIC_AUDNCP_CLKDIV_CON3"},
	{0x156e, 0x0003, 0x0003, 0x0002, PMIC_AUDNCP_CLKDIV_CON4, "PMIC_AUDNCP_CLKDIV_CON4"},
	{0x1570, 0x0fff, 0x0fff, 0x0000, PMIC_AUD_TOP_MON_CON0, "PMIC_AUD_TOP_MON_CON0"},
	{0x1580, 0xffff, 0x0000, 0xc900, PMIC_AUDIO_DIG_ID, "PMIC_AUDIO_DIG_ID"},
	{0x1582, 0xffff, 0x0000, 0x1000, PMIC_AUDIO_DIG_REV0, "PMIC_AUDIO_DIG_REV0"},
	{0x1584, 0xffff, 0x0000, 0x0010, PMIC_AUDIO_DIG_REV1, "PMIC_AUDIO_DIG_REV1"},
	{0x1586, 0xc001, 0xc001, 0x0000, PMIC_AFE_UL_DL_CON0, "PMIC_AFE_UL_DL_CON0"},
	{0x1588, 0x0001, 0x0001, 0x0000, PMIC_AFE_DL_SRC2_CON0_L, "PMIC_AFE_DL_SRC2_CON0_L"},
	{0x158a, 0x3f80, 0x3f80, 0x0000, PMIC_AFE_UL_SRC_CON0_H, "PMIC_AFE_UL_SRC_CON0_H"},
	{0x158c, 0xc027, 0xc027, 0x0000, PMIC_AFE_UL_SRC_CON0_L, "PMIC_AFE_UL_SRC_CON0_L"},
	{0x158e, 0x0003, 0x0003, 0x0000, PMIC_AFE_TOP_CON0, "PMIC_AFE_TOP_CON0"},
	{0x1590, 0xffff, 0xffff, 0xc000, PMIC_AUDIO_TOP_CON0, "PMIC_AUDIO_TOP_CON0"},
	{0x1592, 0xdfff, 0xdfff, 0x0000, PMIC_AFE_MON_DEBUG0, "PMIC_AFE_MON_DEBUG0"},
	{0x1594, 0xffff, 0xffff, 0xd821, PMIC_AFUNC_AUD_CON0, "PMIC_AFUNC_AUD_CON0"},
	{0x1596, 0xffff, 0xffff, 0x0000, PMIC_AFUNC_AUD_CON1, "PMIC_AFUNC_AUD_CON1"},
	{0x1598, 0x00df, 0x00df, 0x0000, PMIC_AFUNC_AUD_CON2, "PMIC_AFUNC_AUD_CON2"},
	{0x159a, 0xf771, 0xf771, 0x0000, PMIC_AFUNC_AUD_CON3, "PMIC_AFUNC_AUD_CON3"},
	{0x159c, 0x017f, 0x017f, 0x0000, PMIC_AFUNC_AUD_CON4, "PMIC_AFUNC_AUD_CON4"},
	{0x159e, 0xffff, 0x0000, 0x0000, PMIC_AFUNC_AUD_MON0, "PMIC_AFUNC_AUD_MON0"},
	{0x15a0, 0x9f1f, 0x0000, 0x0000, PMIC_AUDRC_TUNE_MON0, "PMIC_AUDRC_TUNE_MON0"},
	{0x15a2, 0xffff, 0xffff, 0x0001, PMIC_AFE_ADDA_MTKAIF_FIFO_CFG0, "PMIC_AFE_ADDA_MTKAIF_FIFO_CFG0"},
	{0x15a4, 0x0003, 0x0000, 0x0000, PMIC_AFE_ADDA_MTKAIF_FIFO_LOG_MON1, "PMIC_AFE_ADDA_MTKAIF_FIFO_LOG_MON1"},
	{0x15a6, 0x7fff, 0x0000, 0x0000, PMIC_AFE_ADDA_MTKAIF_MON0, "PMIC_AFE_ADDA_MTKAIF_MON0"},
	{0x15a8, 0x79ff, 0x0000, 0x0000, PMIC_AFE_ADDA_MTKAIF_MON1, "PMIC_AFE_ADDA_MTKAIF_MON1"},
	{0x15aa, 0xffff, 0x0000, 0x0000, PMIC_AFE_ADDA_MTKAIF_MON2, "PMIC_AFE_ADDA_MTKAIF_MON2"},
	{0x15ac, 0xffff, 0x0000, 0x0000, PMIC_AFE_ADDA_MTKAIF_MON3, "PMIC_AFE_ADDA_MTKAIF_MON3"},
	{0x15ae, 0x8117, 0x8117, 0x0000, PMIC_AFE_ADDA_MTKAIF_CFG0, "PMIC_AFE_ADDA_MTKAIF_CFG0"},
	{0x15b0, 0xf779, 0xf779, 0x0730, PMIC_AFE_ADDA_MTKAIF_RX_CFG0, "PMIC_AFE_ADDA_MTKAIF_RX_CFG0"},
	{0x15b2, 0xffff, 0xffff, 0x4330, PMIC_AFE_ADDA_MTKAIF_RX_CFG1, "PMIC_AFE_ADDA_MTKAIF_RX_CFG1"},
	{0x15b4, 0x1fff, 0x1fff, 0x0003, PMIC_AFE_ADDA_MTKAIF_RX_CFG2, "PMIC_AFE_ADDA_MTKAIF_RX_CFG2"},
	{0x15b6, 0xf178, 0xf178, 0x0030, PMIC_AFE_ADDA_MTKAIF_RX_CFG3, "PMIC_AFE_ADDA_MTKAIF_RX_CFG3"},
	{0x15b8, 0x0077, 0x0077, 0x0063, PMIC_AFE_ADDA_MTKAIF_TX_CFG1, "PMIC_AFE_ADDA_MTKAIF_TX_CFG1"},
	{0x15ba, 0xf0c0, 0xf0c0, 0x0000, PMIC_AFE_SGEN_CFG0, "PMIC_AFE_SGEN_CFG0"},
	{0x15bc, 0xc01f, 0xc01f, 0x0001, PMIC_AFE_SGEN_CFG1, "PMIC_AFE_SGEN_CFG1"},
	{0x15be, 0x0032, 0x0032, 0x0010, PMIC_AFE_ADC_ASYNC_FIFO_CFG, "PMIC_AFE_ADC_ASYNC_FIFO_CFG"},
	{0x15c0, 0xffff, 0xffff, 0x0fe2, PMIC_AFE_DCCLK_CFG0, "PMIC_AFE_DCCLK_CFG0"},
	{0x15c2, 0x1fff, 0x1fff, 0x1105, PMIC_AFE_DCCLK_CFG1, "PMIC_AFE_DCCLK_CFG1"},
	{0x15c4, 0xffff, 0xffff, 0x0000, PMIC_AUDIO_DIG_CFG, "PMIC_AUDIO_DIG_CFG"},
	{0x15c6, 0xffff, 0xffff, 0x0000, PMIC_AFE_AUD_PAD_TOP, "PMIC_AFE_AUD_PAD_TOP"},
	{0x15c8, 0xffff, 0x0000, 0x0000, PMIC_AFE_AUD_PAD_TOP_MON, "PMIC_AFE_AUD_PAD_TOP_MON"},
	{0x15ca, 0xffff, 0x0000, 0x0000, PMIC_AFE_AUD_PAD_TOP_MON1, "PMIC_AFE_AUD_PAD_TOP_MON1"},
	{0x1600, 0xffff, 0x0000, 0xa084, PMIC_AUDENC_DSN_ID, "PMIC_AUDENC_DSN_ID"},
	{0x1602, 0xffff, 0x0000, 0x1010, PMIC_AUDENC_DSN_REV0, "PMIC_AUDENC_DSN_REV0"},
	{0x1604, 0xffff, 0x0000, 0x0010, PMIC_AUDENC_DSN_REV1, "PMIC_AUDENC_DSN_REV1"},
	{0x1606, 0x77ff, 0x77ff, 0x0000, PMIC_AUDENC_ANA_CON0, "PMIC_AUDENC_ANA_CON0"},
	{0x1608, 0x77ff, 0x77ff, 0x0000, PMIC_AUDENC_ANA_CON1, "PMIC_AUDENC_ANA_CON1"},
	{0x160a, 0xffff, 0xffff, 0x0000, PMIC_AUDENC_ANA_CON2, "PMIC_AUDENC_ANA_CON2"},
	{0x160c, 0x0f0f, 0x0f0f, 0x0000, PMIC_AUDENC_ANA_CON3, "PMIC_AUDENC_ANA_CON3"},
	{0x160e, 0x3fff, 0x3fff, 0x0800, PMIC_AUDENC_ANA_CON4, "PMIC_AUDENC_ANA_CON4"},
	{0x1610, 0xffff, 0xffff, 0x0000, PMIC_AUDENC_ANA_CON5, "PMIC_AUDENC_ANA_CON5"},
	{0x1612, 0x3f3f, 0x3f3f, 0x1515, PMIC_AUDENC_ANA_CON6, "PMIC_AUDENC_ANA_CON6"},
	{0x1614, 0xffff, 0xffff, 0x0000, PMIC_AUDENC_ANA_CON7, "PMIC_AUDENC_ANA_CON7"},
	{0x1616, 0xffff, 0xffff, 0x0004, PMIC_AUDENC_ANA_CON8, "PMIC_AUDENC_ANA_CON8"},
	{0x1618, 0x7777, 0x7777, 0x0000, PMIC_AUDENC_ANA_CON9, "PMIC_AUDENC_ANA_CON9"},
	{0x161a, 0x1377, 0x1377, 0x0000, PMIC_AUDENC_ANA_CON10, "PMIC_AUDENC_ANA_CON10"},
	{0x161c, 0xfff7, 0xfff7, 0x0000, PMIC_AUDENC_ANA_CON11, "PMIC_AUDENC_ANA_CON11"},
	{0x161e, 0x1f1f, 0x0000, 0x0000, PMIC_AUDENC_ANA_CON12, "PMIC_AUDENC_ANA_CON12"},
	{0x1640, 0xffff, 0x0000, 0xa080, PMIC_AUDDEC_DSN_ID, "PMIC_AUDDEC_DSN_ID"},
	{0x1642, 0xffff, 0x0000, 0x1010, PMIC_AUDDEC_DSN_REV0, "PMIC_AUDDEC_DSN_REV0"},
	{0x1644, 0xffff, 0x0000, 0x2610, PMIC_AUDDEC_DSN_REV1, "PMIC_AUDDEC_DSN_REV1"},
	{0x1646, 0xffff, 0xffff, 0x0000, PMIC_AUDDEC_ANA_CON0, "PMIC_AUDDEC_ANA_CON0"},
	{0x1648, 0x3fff, 0x3fff, 0x0000, PMIC_AUDDEC_ANA_CON1, "PMIC_AUDDEC_ANA_CON1"},
	{0x164a, 0xe077, 0xe077, 0x0000, PMIC_AUDDEC_ANA_CON2, "PMIC_AUDDEC_ANA_CON2"},
	{0x164c, 0xe000, 0xe000, 0x0000, PMIC_AUDDEC_ANA_CON3, "PMIC_AUDDEC_ANA_CON3"},
	{0x164e, 0xb777, 0xb777, 0x0000, PMIC_AUDDEC_ANA_CON4, "PMIC_AUDDEC_ANA_CON4"},
	{0x1650, 0x0077, 0x0077, 0x0000, PMIC_AUDDEC_ANA_CON5, "PMIC_AUDDEC_ANA_CON5"},
	{0x1652, 0x0fff, 0x0fff, 0x0000, PMIC_AUDDEC_ANA_CON6, "PMIC_AUDDEC_ANA_CON6"},
	{0x1654, 0x0fff, 0x0fff, 0x0000, PMIC_AUDDEC_ANA_CON7, "PMIC_AUDDEC_ANA_CON7"},
	{0x1656, 0x1f7f, 0x1f7f, 0x0000, PMIC_AUDDEC_ANA_CON8, "PMIC_AUDDEC_ANA_CON8"},
	{0x1658, 0xffff, 0xffff, 0x0000, PMIC_AUDDEC_ANA_CON9, "PMIC_AUDDEC_ANA_CON9"},
	{0x165a, 0xffff, 0xffff, 0x0000, PMIC_AUDDEC_ANA_CON10, "PMIC_AUDDEC_ANA_CON10"},
	{0x165c, 0xff8f, 0xff8f, 0x4900, PMIC_AUDDEC_ANA_CON11, "PMIC_AUDDEC_ANA_CON11"},
	{0x165e, 0x01ff, 0x01ff, 0x0155, PMIC_AUDDEC_ANA_CON12, "PMIC_AUDDEC_ANA_CON12"},
	{0x1660, 0x0077, 0x0077, 0x0010, PMIC_AUDDEC_ANA_CON13, "PMIC_AUDDEC_ANA_CON13"},
	{0x1662, 0xf777, 0xf777, 0x0000, PMIC_AUDDEC_ANA_CON14, "PMIC_AUDDEC_ANA_CON14"},
	{0x1664, 0xfff3, 0xfff3, 0x0000, PMIC_AUDDEC_ANA_CON15, "PMIC_AUDDEC_ANA_CON15"},
	{0x1666, 0x00ff, 0x0000, 0x0002, PMIC_AUDDEC_ELR_NUM, "PMIC_AUDDEC_ELR_NUM"},
	{0x1668, 0x1fff, 0x1fff, 0x0000, PMIC_AUDDEC_ELR_0, "PMIC_AUDDEC_ELR_0"},

	{0x1552, 0x0007, 0x0007, 0x0000, PMIC_AUD_TOP_RST_BANK_CON0, "PMIC_AUD_TOP_RST_BANK_CON0"},
	{0x1550, 0x000f, 0x000f, 0x0000, PMIC_AUD_TOP_RST_CON0, "PMIC_AUD_TOP_RST_CON0"},
};

void vDumpPmicRegs(void)
{
	unsigned int addr;
	int i;

	pr_warn("Dump PMIC MT6328 AUDIO_SYS_TOP Registers\n\n");
	/* dump all pmic regs */

	/* Init MT6328 PMIC Audio Registers */
	for (i = 0; i < PMIC_AUDIO_REG_COUNT; i++) {
		addr = u4PMICAudioRegTable[i].regid;
		pr_warn("[0x%x] = 0x%x\n", addr, Ana_Get_Reg(addr));
	}

	pr_warn("GPIO_MODE2 [0x%x] = 0x%x\n", GPIO_MODE2, Ana_Get_Reg(GPIO_MODE2));
	pr_warn("GPIO_MODE3 [0x%x] = 0x%x\n", GPIO_MODE3, Ana_Get_Reg(GPIO_MODE3));
	pr_warn("[0x%x] = 0x%x\n", 0x0, Ana_Get_Reg(0x0));
	pr_warn("DRV_CON3 [0x%x] = 0x%x\n", DRV_CON3, Ana_Get_Reg(DRV_CON3));
	pr_warn("[0x%x] = 0x%x\n", 0xc, Ana_Get_Reg(0xc));
	pr_warn("SMT_CON0 [0x%x] = 0x%x\n", SMT_CON0, Ana_Get_Reg(SMT_CON0));
	pr_warn("SMT_CON1 [0x%x] = 0x%x\n", SMT_CON1, Ana_Get_Reg(SMT_CON1));
	pr_warn("FILTER_CON0 [0x%x] = 0x%x\n", FILTER_CON0, Ana_Get_Reg(FILTER_CON0));
}

static int Pmic_Loopback_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = Pmic_Loopback_Type;
	return 0;
}

static int Pmic_Loopback_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_LPBK_function)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0) { /* disable pmic lpbk */
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0x0001); /* power off uplink */
		Ana_Set_Reg(PMIC_ADDA_MTKAIF_CFG0, 0x0, 0xffff);   /* disable new lpbk 2 */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);   /* turn off afe UL & DL */

		/* disable aud_pad RX & TX fifos */
		Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x0, 0x101);

		TurnOffDacPower();
	} else if (ucontrol->value.integer.value[0] > 0) { /* enable pmic lpbk */
		pr_warn("set PMIC LPBK3, DLSR=%d, ULSR=%d\n",
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_OUT_DAC],
			mBlockSampleRate[AUDIO_ANALOG_DEVICE_IN_ADC]);

		/* set dl part */
		TurnOnDacPower(AUDIO_ANALOG_DEVICE_OUT_HEADSETL);

		Ana_Set_Reg(PMIC_AUDIO_TOP_CON0, 0x8000, 0xffff);	/* power on clock */

		/* enable aud_pad TX fifos */
		Ana_Set_Reg(AFE_AUD_PAD_TOP, 0x3100, 0xff00);

		/* Set UL Part */
		Ana_Set_Reg(PMIC_ADDA_MTKAIF_CFG0, 0x2, 0xffff);   /* enable new lpbk 2 */

		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);   /* power on uplink */

		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0x0002);       /* configure ADC setting */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);   /* turn on afe */
	}

	vDumpPmicRegs();

	/* remember to set, AP side 0xe00 [1] = 1, for new lpbk2 */

	pr_warn("%s() done\n", __func__);
	Pmic_Loopback_Type = ucontrol->value.integer.value[0];
	return 0;
}

static int SineTable_DAC_HP_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = SineTable_DAC_HP_flag;
	return 0;
}

static int SineTable_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 TODO? */
	pr_warn("%s()\n", __func__);
	return 0;
}

static void ADC_LOOP_DAC_Func(int command)
{
	/* 6752 TODO? */
}

static bool DAC_LOOP_DAC_HS_flag;
static int ADC_LOOP_DAC_HS_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HS_flag;
	return 0;
}

static int ADC_LOOP_DAC_HS_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_ON);
	} else {
		DAC_LOOP_DAC_HS_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HS_OFF);
	}
	return 0;
}

static bool DAC_LOOP_DAC_HP_flag;
static int ADC_LOOP_DAC_HP_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = DAC_LOOP_DAC_HP_flag;
	return 0;
}

static int ADC_LOOP_DAC_HP_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

	pr_warn("%s()\n", __func__);
	if (ucontrol->value.integer.value[0]) {
		DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_ON);
	} else {
		DAC_LOOP_DAC_HP_flag = ucontrol->value.integer.value[0];
		ADC_LOOP_DAC_Func(AUDIO_ANALOG_DAC_LOOP_DAC_HP_OFF);
	}
	return 0;
}

static bool Voice_Call_DAC_DAC_HS_flag;
static int Voice_Call_DAC_DAC_HS_Get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = Voice_Call_DAC_DAC_HS_flag;
	return 0;
}

static int Voice_Call_DAC_DAC_HS_Set(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	/* 6752 TODO */
	pr_warn("%s()\n", __func__);
	return 0;
}

static const struct soc_enum Pmic_Test_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_LPBK_function), Pmic_LPBK_function),
};

static const struct snd_kcontrol_new mt6356_pmic_Test_controls[] = {
	SOC_ENUM_EXT("SineTable_DAC_HP", Pmic_Test_Enum[0], SineTable_DAC_HP_Get,
		     SineTable_DAC_HP_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HS", Pmic_Test_Enum[1], ADC_LOOP_DAC_HS_Get,
		     ADC_LOOP_DAC_HS_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HP", Pmic_Test_Enum[2], ADC_LOOP_DAC_HP_Get,
		     ADC_LOOP_DAC_HP_Set),
	SOC_ENUM_EXT("Voice_Call_DAC_DAC_HS", Pmic_Test_Enum[3], Voice_Call_DAC_DAC_HS_Get,
		     Voice_Call_DAC_DAC_HS_Set),
	SOC_ENUM_EXT("SineTable_UL2", Pmic_Test_Enum[4], SineTable_UL2_Get, SineTable_UL2_Set),
	SOC_ENUM_EXT("Pmic_Loopback", Pmic_Test_Enum[5], Pmic_Loopback_Get, Pmic_Loopback_Set),
};

static const struct snd_kcontrol_new mt6356_UL_Codec_controls[] = {
	SOC_ENUM_EXT("Audio_ADC_1_Switch", Audio_UL_Enum[0], Audio_ADC1_Get,
		     Audio_ADC1_Set),
	SOC_ENUM_EXT("Audio_ADC_2_Switch", Audio_UL_Enum[1], Audio_ADC2_Get,
		     Audio_ADC2_Set),
	SOC_ENUM_EXT("Audio_ADC_3_Switch", Audio_UL_Enum[2], Audio_ADC3_Get,
		     Audio_ADC3_Set),
	SOC_ENUM_EXT("Audio_ADC_4_Switch", Audio_UL_Enum[3], Audio_ADC4_Get,
		     Audio_ADC4_Set),
	SOC_ENUM_EXT("Audio_Preamp1_Switch", Audio_UL_Enum[4],
		     Audio_PreAmp1_Get,
		     Audio_PreAmp1_Set),
	SOC_ENUM_EXT("Audio_ADC_1_Sel", Audio_UL_Enum[5],
		     Audio_ADC1_Sel_Get, Audio_ADC1_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_2_Sel", Audio_UL_Enum[6],
		     Audio_ADC2_Sel_Get, Audio_ADC2_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_3_Sel", Audio_UL_Enum[7],
		     Audio_ADC3_Sel_Get, Audio_ADC3_Sel_Set),
	SOC_ENUM_EXT("Audio_ADC_4_Sel", Audio_UL_Enum[8],
		     Audio_ADC4_Sel_Get, Audio_ADC4_Sel_Set),
	SOC_ENUM_EXT("Audio_PGA1_Setting", Audio_UL_Enum[9], Audio_PGA1_Get,
		     Audio_PGA1_Set),
	SOC_ENUM_EXT("Audio_PGA2_Setting", Audio_UL_Enum[10],
		     Audio_PGA2_Get, Audio_PGA2_Set),
	SOC_ENUM_EXT("Audio_PGA3_Setting", Audio_UL_Enum[11],
		     Audio_PGA3_Get, Audio_PGA3_Set),
	SOC_ENUM_EXT("Audio_PGA4_Setting", Audio_UL_Enum[12],
		     Audio_PGA4_Get, Audio_PGA4_Set),
	SOC_ENUM_EXT("Audio_MicSource1_Setting", Audio_UL_Enum[13],
		     Audio_MicSource1_Get,
		     Audio_MicSource1_Set),
	SOC_ENUM_EXT("Audio_MicSource2_Setting", Audio_UL_Enum[14],
		     Audio_MicSource2_Get,
		     Audio_MicSource2_Set),
	SOC_ENUM_EXT("Audio_MicSource3_Setting", Audio_UL_Enum[15],
		     Audio_MicSource3_Get,
		     Audio_MicSource3_Set),
	SOC_ENUM_EXT("Audio_MicSource4_Setting", Audio_UL_Enum[16],
		     Audio_MicSource4_Get,
		     Audio_MicSource4_Set),
	SOC_ENUM_EXT("Audio_MIC1_Mode_Select", Audio_UL_Enum[17],
		     Audio_Mic1_Mode_Select_Get,
		     Audio_Mic1_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC2_Mode_Select", Audio_UL_Enum[18],
		     Audio_Mic2_Mode_Select_Get,
		     Audio_Mic2_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC3_Mode_Select", Audio_UL_Enum[19],
		     Audio_Mic3_Mode_Select_Get,
		     Audio_Mic3_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_MIC4_Mode_Select", Audio_UL_Enum[20],
		     Audio_Mic4_Mode_Select_Get,
		     Audio_Mic4_Mode_Select_Set),
	SOC_ENUM_EXT("Audio_Mic_Power_Mode", Audio_UL_Enum[21],
		     Audio_Adc_Power_Mode_Get,
		     Audio_Adc_Power_Mode_Set),
	SOC_ENUM_EXT("Audio_Preamp2_Switch", Audio_UL_Enum[22],
		     Audio_PreAmp2_Get,
		     Audio_PreAmp2_Set),
	SOC_ENUM_EXT("Audio_UL_LR_Swap", Audio_UL_Enum[23],
		     Audio_UL_LR_Swap_Get,
		     Audio_UL_LR_Swap_Set),
};

/* Verify in muse6753 phone for LGE HP impedance requirement (2016/02) */
int Audio_Read_Efuse_HP_Impedance_Current_Calibration(void)
{
	int ret;

	/* Initialize to zero*/
	ret = 0;

#ifdef EFUSE_HP_IMPEDANCE
	pr_warn("+%s\n", __func__);

	/* 1. enable efuse ctrl engine clock */
	Ana_Set_Reg(TOP_CKHWEN_CON0_CLR, 0x0040, 0x0040);
	Ana_Set_Reg(TOP_CKPDN_CON3_CLR, 0x0004, 0x0004);

	/* 2. set RG_OTP_RD_SW */
	Ana_Set_Reg(0x0C16, 0x0001, 0x0001);

	/* 3. Select which row to read in MACRO_1 */
	/* set RG_OTP_PA for bit#786 ~ #793*/
	Ana_Set_Reg(0x0C00, 0x0022, 0x003F);

	/* 4. Toggle RG_OTP_RD_TRIG */
	ret = Ana_Get_Reg(0x0C10);
	if (ret == 0)
		Ana_Set_Reg(0x0C10, 0x0001, 0x0001);
	else
		Ana_Set_Reg(0x0C10, 0x0000, 0x0001);

	/* 5. Polling RG_OTP_RD_BUSY */
	ret = 1;
	while (ret == 1) {
		usleep_range(100, 200);
		ret = Ana_Get_Reg(0x0C1A);
		ret = ret & 0x0001;
		pr_warn("%s polling 0xC1A=0x%x\n", __func__, ret);
	}

	/* Need to delay at least 1ms for 0xC1A and than can read */
	usleep_range(500, 1000);

	/* 6. Read RG_OTP_DOUT_SW */
	/* BIT#786 ~ #793 */
	ret = Ana_Get_Reg(0x0C18);
	pr_warn("%s HPoffset : efuse=0x%x\n", __func__, ret);
	ret = (ret >> 2) & 0x00FF;
	if (ret >= 64)
		ret = ret - 128;

	/* 7. Disables efuse_ctrl egine clock */
	Ana_Set_Reg(TOP_CKPDN_CON3_SET, 0x0004, 0x0004);
	Ana_Set_Reg(TOP_CKHWEN_CON0_SET, 0x0040, 0x0040);
#endif

	pr_warn("-%s EFUSE: %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(Audio_Read_Efuse_HP_Impedance_Current_Calibration);

static const struct snd_soc_dapm_widget mt6356_dapm_widgets[] = {
	/* Outputs */
	SND_SOC_DAPM_OUTPUT("EARPIECE"),
	SND_SOC_DAPM_OUTPUT("HEADSET"),
	SND_SOC_DAPM_OUTPUT("SPEAKER"),
};

static const struct snd_soc_dapm_route mtk_audio_map[] = {
	{"VOICE_Mux_E", "Voice Mux", "SPEAKER PGA"},
};

static void mt6356_codec_init_reg(struct snd_soc_codec *codec)
{
	pr_warn("%s\n", __func__);

	audckbufEnable(true);
	/* Disable CLKSQ 26MHz */
	Ana_Set_Reg(TOP_CLKSQ_CLR, 0x1, 0x0001);
	Ana_Set_Reg(TOP_CLKSQ, 0x0, 0x0001);
	/* disable AUDGLB */
	Ana_Set_Reg(AUDDEC_ANA_CON13, 0x1 << 4, 0x1 << 4);
	/* Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M */
	Ana_Set_Reg(AUD_TOP_CKPDN_CON0, 0x66, 0x66);
	/* Disable HeadphoneL/HeadphoneR short circuit protection */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3 << 12, 0x3 << 12);
	/* Disable voice short circuit protection */
	Ana_Set_Reg(AUDDEC_ANA_CON6, 0x1 << 4, 0x1 << 4);
	/* disable LO buffer left short circuit protection */
	Ana_Set_Reg(AUDDEC_ANA_CON7, 0x1 << 4, 0x1 << 4);
	/* disable LO buffer left short circuit protection */
	Ana_Set_Reg(DRV_CON3, 0xcccc, 0xffff);
	/* gpio miso driving set to default 6mA, 0xcccc */
	Ana_Set_Reg(DRV_CON3, 0xcccc, 0xffff);
	/* gpio mosi mode */
	Ana_Set_Reg(GPIO_MODE2, 0x249, 0xffff);

	/* gpio miso mode, pad_aud_clk_miso set mode 0 for power saving */
	Ana_Set_Reg(GPIO_MODE3_CLR, 0xffff, 0xffff);
	Ana_Set_Reg(GPIO_MODE3_SET, 0x0248, 0xffff);
	Ana_Set_Reg(GPIO_MODE3, 0x0248, 0xffff);
	/* gpio pad_aud_clk_miso set dir input for mode 0 for power saving */
	Ana_Set_Reg(GPIO_DIR0, 0x0, 0x1 << 12);

	audckbufEnable(false);
}

void InitCodecDefault(void)
{
	pr_warn("%s\n", __func__);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = 8;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = 8;

	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC1] =
	    AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC2] =
	    AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC3] =
	    AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
	mCodec_data->mAudio_Ana_Mux[AUDIO_ANALOG_MUX_IN_MIC4] =
	    AUDIO_ANALOG_AUDIOANALOG_INPUT_PREAMP;
}

static void InitGlobalVarDefault(void)
{
	mCodec_data = NULL;
	mAdc_Power_Mode = 0;
	mInitCodec = false;
	mAnaSuspend = false;
	audck_buf_Count = 0;
	ClsqCount = 0;
	TopCkCount = 0;
	NvRegCount = 0;
}

static int mt6356_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->component.dapm;

	pr_warn("%s()\n", __func__);

	if (mInitCodec == true)
		return 0;

	snd_soc_dapm_new_controls(dapm, mt6356_dapm_widgets, ARRAY_SIZE(mt6356_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, mtk_audio_map, ARRAY_SIZE(mtk_audio_map));

	/* add codec controls */
	snd_soc_add_codec_controls(codec, mt6356_snd_controls, ARRAY_SIZE(mt6356_snd_controls));
	snd_soc_add_codec_controls(codec, mt6356_UL_Codec_controls,
				   ARRAY_SIZE(mt6356_UL_Codec_controls));
	snd_soc_add_codec_controls(codec, mt6356_pmic_Test_controls,
				   ARRAY_SIZE(mt6356_pmic_Test_controls));

	snd_soc_add_codec_controls(codec, Audio_snd_auxadc_controls,
				   ARRAY_SIZE(Audio_snd_auxadc_controls));

	/* here to set  private data */
	mCodec_data = kzalloc(sizeof(mt6356_Codec_Data_Priv), GFP_KERNEL);
	if (!mCodec_data) {
		/*pr_warn("Failed to allocate private data\n");*/
		return -ENOMEM;
	}
	snd_soc_codec_set_drvdata(codec, mCodec_data);

	memset((void *)mCodec_data, 0, sizeof(mt6356_Codec_Data_Priv));
	mt6356_codec_init_reg(codec);
	InitCodecDefault();
	mInitCodec = true;

	return 0;
}

static int mt6356_codec_remove(struct snd_soc_codec *codec)
{
	pr_warn("%s()\n", __func__);
	return 0;
}

static unsigned int mt6356_read(struct snd_soc_codec *codec, unsigned int reg)
{
	pr_warn("mt6356_read reg = 0x%x", reg);
	Ana_Get_Reg(reg);
	return 0;
}

static int mt6356_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	pr_warn("mt6356_write reg = 0x%x  value= 0x%x\n", reg, value);
	Ana_Set_Reg(reg, value, 0xffffffff);
	return 0;
}

static struct snd_soc_codec_driver soc_mtk_codec = {
	.probe = mt6356_codec_probe,
	.remove = mt6356_codec_remove,

	.read = mt6356_read,
	.write = mt6356_write,

	/* use add control to replace */
	/* .controls = mt6356_snd_controls, */
	/* .num_controls = ARRAY_SIZE(mt6356_snd_controls), */

	.dapm_widgets = mt6356_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt6356_dapm_widgets),
	.dapm_routes = mtk_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mtk_audio_map),

};

static int mtk_mt6356_codec_dev_probe(struct platform_device *pdev)
{
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);

	if (pdev->dev.dma_mask == NULL)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;


	if (pdev->dev.of_node) {
		dev_set_name(&pdev->dev, "%s", MT_SOC_CODEC_NAME);

		/* check if use hp depop flow */
		of_property_read_u32(pdev->dev.of_node,
				     "use_hp_depop_flow",
				     &mUseHpDepopFlow);

		pr_warn("%s(), use_hp_depop_flow = %d\n",
			__func__, mUseHpDepopFlow);

	} else {
		pr_warn("%s(), pdev->dev.of_node = NULL!!!\n", __func__);
	}

	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_codec(&pdev->dev,
				      &soc_mtk_codec, mtk_6356_dai_codecs,
				      ARRAY_SIZE(mtk_6356_dai_codecs));
}

static int mtk_mt6356_codec_dev_remove(struct platform_device *pdev)
{
	pr_warn("%s:\n", __func__);

	snd_soc_unregister_codec(&pdev->dev);
	return 0;

}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_codec_63xx_of_ids[] = {
	{.compatible = "mediatek,mt_soc_codec_63xx",},
	{}
};
#endif

static struct platform_driver mtk_codec_6356_driver = {
	.driver = {
		   .name = MT_SOC_CODEC_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_soc_codec_63xx_of_ids,
#endif
		   },
	.probe = mtk_mt6356_codec_dev_probe,
	.remove = mtk_mt6356_codec_dev_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtk_codec6356_dev;
#endif

static int __init mtk_mt6356_codec_init(void)
{
	pr_warn("%s:\n", __func__);
#ifndef CONFIG_OF
	int ret = 0;

	soc_mtk_codec6356_dev = platform_device_alloc(MT_SOC_CODEC_NAME, -1);

	if (!soc_mtk_codec6356_dev)
		return -ENOMEM;


	ret = platform_device_add(soc_mtk_codec6356_dev);
	if (ret != 0) {
		platform_device_put(soc_mtk_codec6356_dev);
		return ret;
	}
#endif
	InitGlobalVarDefault();

	return platform_driver_register(&mtk_codec_6356_driver);
}

module_init(mtk_mt6356_codec_init);

static void __exit mtk_mt6356_codec_exit(void)
{
	pr_warn("%s:\n", __func__);

	platform_driver_unregister(&mtk_codec_6356_driver);
}

module_exit(mtk_mt6356_codec_exit);

/* Module information */
MODULE_DESCRIPTION("MTK  codec driver");
MODULE_LICENSE("GPL v2");
