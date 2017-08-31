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
 *   Chipeng Chang (mtk02308)
 *   Chien-Wei Hsu (mtk10177)
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

#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <linux/delay.h>

#include "mtk-auddrv-def.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-gpio.h"

#include "mtk-soc-analog-type.h"
#include "mtk-soc-codec-63xx.h"
#include "mtk-soc-afe-control.h"
#ifdef _GIT318_READY
#include <mtk_clkbuf_ctl.h>
#endif
#ifdef _GIT318_PMIC_READY
#include <mach/mtk_pmic.h>
#endif
#ifdef CONFIG_MTK_VOW_SUPPORT
#include <mach/vow_api.h>
#endif

#ifdef CONFIG_MTK_SPEAKER
#include "mtk-soc-codec-speaker-63xx.h"
#endif

/*#define MT6755_AW8736_REWORK*/	/* use different GPIO for rework version */
#define AW8736_MODE_CTRL /* AW8736 PA output power mode control*/

#ifdef MT6755_AW8736_REWORK
#include "../../../../drivers/misc/mediatek/auxadc/mt6755/mt_auxadc_sw.h"
#endif

/* static function declaration */
static bool get_analog_input_status(void);
static void Apply_Speaker_Gain(void);
static bool TurnOnVOWDigitalHW(bool enable);
static void TurnOffDacPower(void);
static void TurnOnDacPower(void);
static void setDlMtkifSrc(bool enable);
static void SetDcCompenSation(bool enable);
static void Voice_Amp_Change(bool enable);
static void Speaker_Amp_Change(bool enable);
static bool TurnOnVOWADcPower(int MicType, bool enable);
static void audio_preamp1_en(bool power);
static void audio_preamp2_en(bool power);
static void audio_preamp3_en(bool power);
static void audio_preamp4_en(bool power);
static void audio_dmic_input_enable(bool power, enum audio_analog_device_type device_in);
static void hp_main_output_ramp(bool up);
static void hp_aux_feedback_loop_gain_ramp(bool up);
static void hp_gain_ramp(bool up);
static void set_lch_dc_compensation_reg(int lch_value);
static void set_rch_dc_compensation_reg(int rch_value);
static void enable_dc_compensation(bool enable);

#ifdef CONFIG_MTK_VOW_SUPPORT
static void TurnOnVOWPeriodicOnOff(int MicType, int On_period, int enable);
static void VOW_GPIO_Enable(bool enable);
static void VOW_Pwr_Enable(int MicType, bool enable);
static void VOW_DCC_CLK_Enable(bool enable);
static void VOW_MIC_DCC_Enable(int MicType, bool enable);
static void VOW_MIC_ACC_Enable(int MicType, bool enable);
static void VOW12MCK_Enable(bool enable);
static void VOW32KCK_Enable(bool enable);
#endif

static struct codec_data_private *mCodec_data;
static uint32 mBlockSampleRate[AUDIO_DAI_INTERFACE_MAX] = { 48000, 48000, 48000, 48000};

static DEFINE_MUTEX(Ana_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_buf_Ctrl_Mutex);
static DEFINE_MUTEX(Ana_Clk_Mutex);
static DEFINE_MUTEX(Ana_Power_Mutex);
static DEFINE_MUTEX(AudAna_lock);
static DEFINE_MUTEX(Ana_SwOp_Mutex);

static int adc1_mic_mode_mux = AUDIO_ANALOGUL_MODE_ACC;
static int adc2_mic_mode_mux = AUDIO_ANALOGUL_MODE_ACC;
static int adc3_mic_mode_mux = AUDIO_ANALOGUL_MODE_ACC;
static int adc4_mic_mode_mux = AUDIO_ANALOGUL_MODE_ACC;

static int mAudio_Vow_Analog_Func_Enable;
static int mAudio_Vow_Digital_Func_Enable;

static const int DC1unit_in_uv = 19184;	/* in uv with 0DB */
/* static const int DC1unit_in_uv = 21500; */	/* in uv with 0DB */
static const int DC1devider = 8;	/* in uv */

static int ANC_enabled;
static int low_power_mode;
static int hp_differential_mode;

/* Pmic Headphone Impedance varible */
static unsigned short hp_impedance;
static const unsigned short auxcable_impedance = 5000;
static int efuse_current_calibration;

/* DPD efuse variable and table */
static unsigned int dpd_lch[DPD_IMPEDANCE_MAX][DPD_HARMONIC_MAX];
static unsigned int dpd_rch[DPD_IMPEDANCE_MAX][DPD_HARMONIC_MAX];
static int dpd_on;
static int dpd_version;
static int dpd_offset_table[16][32] = {
	/* T = [1/(10^(Fund-HDX/20)] * 8 * 32768 */
	{273, 243, 217, 193, 172, 153, 137, 122,
	 109,  97,  86,  77,  68,  61,  54,  48,
	  43,  39,  34,  31,  27,  24,  22,  19,
	  17,  15,  14,  12,  11,  10,   9,   8}, /* 8T with 1.72 offset 16Ohm L HD2*/
	{204, 181, 162, 144, 128, 114, 102, 91,
	  81,  72,  64,  57,  51,  46,  41, 36,
	  32,  29,  26,  23,  20,  18,  16, 14,
	  13,  11,  10,   9,   8,   7,   6,  6},  /* 8T with 4.26 offset 16Ohm L HD3*/
	{218, 194, 173, 154, 137, 122, 109, 97,
	  87,  77,  69,  61,  55,  49,  43, 39,
	  34,  31,  27,  24,  22,  19,  17, 15,
	  14,  12,  11,  10,   9,   8,   7,  6},  /* 8T with 3.68 offset 16Ohm R HD2*/
	{195, 174, 155, 138, 123, 110, 98, 87,
	  78,  69,  62,  55,  49,  44,  39, 35,
	  31,  28,  25,  22,  19,  17,  15, 14,
	  12,  11,  10,   9,   8,   7,   6,  5},  /* 8T with 4.64 offset 16Ohm R HD3*/
	{268, 239, 213, 190, 169, 151, 134, 120,
	 107,  95,  85,  75,  67,  60,  53,  48,
	  42,  38,  34,  30,  27,  24,  21,  19,
	  17,  15,  13,  12,  11,   9,   8,   8}, /* 8T with 1.88 offset 32Ohm L HD2*/
	{248, 221, 197, 175, 156, 139, 124, 111,
	  99,  88,  78,  70,  62,  55,  49,  44,
	  39,  35,  31,  28,  25,  22,  20,  18,
	  16,  14,  12,  11,  10,   9,   8,   7}, /* 8T with 2.56 offset 32Ohm L HD3*/
	{241, 214, 191, 170, 152, 135, 121, 107,
	  96,  85,  76,  68,  60,  54,  48,  43,
	  38,  34,  30,  27,  24,  21,  19,  17,
	  15,  14,  12,  11,  10,   9,   8,   7}, /* 8T with 2.81 offset 32Ohm R HD2*/
	{233, 208, 185, 165, 147, 131, 117, 104,
	  93,  83,  74,  66,  58,  52,  46,  41,
	  37,  33,  29,  26,  23,  21,  18,  16,
	  15,  13,  12,  10,   9,   8,   7,   7,},/* 8T with 3.09 offset 32Ohm R HD3*/
	{209, 186, 166, 148, 132, 118, 105, 93,
	  83,  74,  66,  59,  53,  47,  42, 37,
	  33,  30,  26,  23,  21,  19,  17, 15,
	  13,  12,  10,   9,   8,   7,   7,  6}, /* 8T with 4.02 offset 560Ohm L HD2*/
	{195, 173, 155, 138, 123, 109, 98, 87,
	  77,  69,  62,  55,  49,  44, 39, 35,
	  31,  27,  24,  22,  19,  17, 15, 14,
	  12,  11,  10,   9,   8,   7,  6,  5}, /* 8T with 4.65 offset 560Ohm L HD3*/
	{208, 185, 165, 147, 131, 117, 104, 93,
	  83,  74,  66,  59,  52,  47,  42, 37,
	  33,  29,  26,  23,  21,  19,  17, 15,
	  13,  12,  10,   9,   8,   7,   7,  6}, /* 8T with 4.07 offset 560Ohm R HD2*/
	{239, 213, 190, 169, 151, 134, 120, 107,
	  95,  85,  76,  67,  60,  54,  48,  43,
	  38,  34,  30,  27,  24,  21,  19,  17,
	  15,  13,  12,  11,  10,   8,   8,   7},/* 8T with 2.86 offset 560Ohm R HD3*/
	{332, 296, 264, 235, 210, 187, 167, 148,
	 132, 118, 105,  94,  83,  74,  66,  59,
	  53,  47,  42,  37,  33,  30,  26,  24,
	  21,  19,  17,  15,  13,  12,  11,   9},/* 8T with 0 offset 1kOhm L HD2*/
	{270, 241, 215, 191, 170, 152, 135, 121,
	 108,  96,  85,  76,  68,  60,  54,  48,
	  43,  38,  34,  30,  27,  24,  21,  19,
	  17,  15,  14,  12,  11,  10,   9,   8},/* 8T with 1.8 offset 1kOhm L HD3*/
	{332, 296, 264, 235, 210, 187, 167, 148,
	 132, 118, 105,  94,  83,  74,  66,  59,
	  53,  47,  42,  37,  33,  30,  26,  24,
	  21,  19,  17,  15,  13,  12,  11,   9},/* 8T with 0 offset 1kOhm R HD2*/
	{321, 286, 255, 227, 203, 181, 161, 143,
	 128, 114, 102,  90,  81,  72,  64,  57,
	  51,  45,  40,  36,  32,  29,  26,  23,
	  20,  18,  16,  14,  13,  11,  10,   9},/* 8T with 0.3 offset 1kOhm R HD3*/
};

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef EFUSE_HP_TRIM
static unsigned int RG_AUDHPLTRIM_VAUDP15, RG_AUDHPRTRIM_VAUDP15, RG_AUDHPLFINETRIM_VAUDP15,
	RG_AUDHPRFINETRIM_VAUDP15, RG_AUDHPLTRIM_VAUDP15_SPKHP, RG_AUDHPRTRIM_VAUDP15_SPKHP,
	RG_AUDHPLFINETRIM_VAUDP15_SPKHP, RG_AUDHPRFINETRIM_VAUDP15_SPKHP;
#endif
#endif

static unsigned int pin_extspkamp, pin_extspkamp_2, pin_vowclk, pin_audmiso, pin_rcvspkswitch;
static unsigned int pin_mode_extspkamp, pin_mode_extspkamp_2, pin_mode_vowclk, pin_mode_audmiso,
	pin_mode_rcvspkswitch;

#ifdef CONFIG_MTK_SPEAKER
static int Speaker_mode = AUDIO_SPEAKER_MODE_AB;
static unsigned int Speaker_pga_gain = 1;	/* default 0Db. */
static bool mSpeaker_Ocflag;
#endif
static unsigned int dAuxAdcChannel = 16;
static const int mDcOffsetTrimChannel = 9;
static bool mInitCodec;
static uint32 MicbiasRef, GetMicbias;

static int reg_AFE_VOW_CFG0;			/* VOW AMPREF Setting */
static int reg_AFE_VOW_CFG1;			/* VOW A,B timeout initial value (timer) */
static int reg_AFE_VOW_CFG2 = 0x2222;		/* VOW A,B value setting (BABA) */
static int reg_AFE_VOW_CFG3 = 0x8767;		/* alhpa and beta K value setting (beta_rise,fall,alpha_rise,fall) */
static int reg_AFE_VOW_CFG4 = 0x006E;		/* gamma K value setting (gamma), bit4:8 should not modify */
static int reg_AFE_VOW_CFG5 = 0x0001;		/* N mini value setting (Nmin) */
static int reg_AFE_VOW_CFG6 = 0x0100;		/* FLR ratio */
static int reg_AFE_VOW_PERIODIC;		/* Periodic On/Off setting (On percent)*/
static bool mIsVOWOn;

/* VOW using */
enum audio_vow_mic_type {
	AUDIO_VOW_MIC_TYPE_Handset_AMIC = 0,
	AUDIO_VOW_MIC_TYPE_Headset_MIC,
	AUDIO_VOW_MIC_TYPE_Handset_DMIC,		/* 1P6 */
	AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K,		/* 800K */
	AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC,		/* DCC mems */
	AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC,
	AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM,		/* DCC ECM, dual differential */
	AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM,		/* DCC ECM, signal differential */
	AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01,	/* DMIC Vendor01 */
	AUDIO_VOW_MIC_TYPE_NUM
};

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
#ifdef CONFIG_MTK_VOW_SUPPORT

/* AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC */
/* AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM */
static const uint16 Handset_AMIC_DCC_PeriodicOnOff[7][22] = {
	/*  PGA,  PreCG,    ADC,  glblp,   dmic, mbias0, mbias1,    pll,  pwrdm,    vow,   dmic, period */
	{0x8000, 0x8000, 0x81AA, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x0000, 0x81EC, 0x0000,
	 0x1917, 0x8021, 0x1917, 0x0000, 0x0000, 0x0000, 0x0000, 0x1917, 0x0000, 0x18F6, 0x0000},/* 90% */
	{0x828F, 0x828F, 0x8439, 0x0000, 0x0000, 0x0000, 0x0000, 0x828F, 0x0000, 0x847B, 0x0000,
	 0x1917, 0x82B0, 0x1917, 0x0000, 0x0000, 0x0000, 0x0000, 0x1917, 0x0000, 0x18F6, 0x0000},/* 80% */
	{0x851F, 0x851F, 0x86C9, 0x0000, 0x0000, 0x0000, 0x0000, 0x851F, 0x0000, 0x870A, 0x0000,
	 0x1917, 0x853F, 0x1917, 0x0000, 0x0000, 0x0000, 0x0000, 0x1917, 0x0000, 0x18F6, 0x0000},/* 70% */
	{0x87AE, 0x87AE, 0x8958, 0x0000, 0x0000, 0x80A4, 0x0000, 0x87AE, 0xC0A4, 0x899A, 0x0000,
	 0x1917, 0x87CF, 0x1917, 0x0000, 0x0000, 0x1917, 0x0000, 0x1917, 0x1917, 0x18F6, 0x0000},/* 60% */
	{0x8A3D, 0x8A3D, 0x8BE7, 0x0000, 0x0000, 0x8333, 0x0000, 0x8A3D, 0xC333, 0x8C29, 0x0000,
	 0x1917, 0x8A5E, 0x1917, 0x0000, 0x0000, 0x1917, 0x0000, 0x1917, 0x1917, 0x18F6, 0x0000},/* 50% */
	{0x8CCD, 0x8CCD, 0x8E77, 0x0000, 0x0000, 0x85C3, 0x0000, 0x8CCD, 0xC5C3, 0x8EB8, 0x0000,
	 0x1917, 0x8CEE, 0x1917, 0x0000, 0x0000, 0x1917, 0x0000, 0x1917, 0x1917, 0x18F6, 0x0000},/* 40% */
	{0x8F5C, 0x8F5C, 0x9106, 0x0000, 0x0000, 0x8852, 0x0000, 0x8F5C, 0xC852, 0x9148, 0x0000,
	 0x1917, 0x8F7D, 0x1917, 0x0000, 0x0000, 0x1917, 0x0000, 0x1917, 0x1917, 0x18F6, 0x0000} /* 30% */
};

/* AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC */
/* AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM */
static const uint16 Headset_MIC_DCC_PeriodicOnOff[7][22] = {
	/*  PGA,  PreCG,    ADC,  glblp,   dmic, mbias0, mbias1,    pll,  pwrdm,    vow,   dmic, period */
	{0x8000, 0x8000, 0x81AA, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0xC000, 0x81EC, 0x0000,
	 0x1917, 0x8021, 0x1917, 0x0000, 0x0000, 0x0000, 0x0000, 0x1917, 0x1917, 0x18F6, 0x0000},/* 90% */
	{0x8148, 0x8148, 0x82F2, 0x0000, 0x0000, 0x0000, 0x80A4, 0x8148, 0xC0A4, 0x8333, 0x0000,
	 0x17CF, 0x8168, 0x17CF, 0x0000, 0x0000, 0x0000, 0x17CF, 0x17CF, 0x17CF, 0x17AE, 0x0000},/* 80% */
	{0x828F, 0x828F, 0x8439, 0x0000, 0x0000, 0x0000, 0x81EC, 0x828F, 0xC1EC, 0x847B, 0x0000,
	 0x1687, 0x82B0, 0x1687, 0x0000, 0x0000, 0x0000, 0x1687, 0x1687, 0x1687, 0x1666, 0x0000},/* 70% */
	{0x83D7, 0x83D7, 0x8581, 0x0000, 0x0000, 0x0000, 0x8333, 0x83D7, 0xC333, 0x85C3, 0x0000,
	 0x153F, 0x83F8, 0x153F, 0x0000, 0x0000, 0x0000, 0x153F, 0x153F, 0x153F, 0x151F, 0x0000},/* 60% */
	{0x851F, 0x851F, 0x86C9, 0x0000, 0x0000, 0x0000, 0x847B, 0x851F, 0xC47B, 0x870A, 0x0000,
	 0x13F8, 0x853F, 0x13F8, 0x0000, 0x0000, 0x0000, 0x13F8, 0x13F8, 0x13F8, 0x13D7, 0x0000},/* 50% */
	{0x8666, 0x8666, 0x8810, 0x0000, 0x0000, 0x0000, 0x85C3, 0x8666, 0xC5C3, 0x8852, 0x0000,
	 0x12B0, 0x8687, 0x12B0, 0x0000, 0x0000, 0x0000, 0x12B0, 0x12B0, 0x12B0, 0x128F, 0x0000},/* 40% */
	{0x87AE, 0x87AE, 0x8958, 0x0000, 0x0000, 0x0000, 0x870A, 0x87AE, 0xC70A, 0x899A, 0x0000,
	 0x1168, 0x87CF, 0x1168, 0x0000, 0x0000, 0x0000, 0x1168, 0x1168, 0x1168, 0x1148, 0x0000} /* 30% */
};

#endif

static int mAudio_VOW_Mic_type = AUDIO_VOW_MIC_TYPE_Handset_AMIC;
static void Audio_Amp_Change(int channels, bool enable, bool is_anc);
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
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false, false);
		}
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == true) {
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
			    false;
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false, false);
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
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true, false);
			mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
			    true;
		}
		if (mCodec_data->mAudio_BackUpAna_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] ==
		    true) {
			Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true, false);
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
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef _GIT318_READY
			clk_buf_ctrl(CLK_BUF_AUDIO, true);
#endif
#endif
		}
		audck_buf_Count++;
	} else {
		audck_buf_Count--;
		if (audck_buf_Count == 0) {
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef _GIT318_READY
			clk_buf_ctrl(CLK_BUF_AUDIO, false);
#endif
#endif
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
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x3800, 0x3800);
			/* Turn on AUDNCP_CLKDIV engine clock,Turn on AUD 26M */
			/* Ana_Set_Reg(TOP_CKPDN_CON0, 0x00FD, 0xFFFF); */
			/* AUD clock power down released */
		}
		TopCkCount++;
	} else {
		TopCkCount--;
		if (TopCkCount == 0) {
			Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3800, 0x3800);
			/* Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M */
			/* Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0xFF00, 0xFF00); */
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
			/* Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0155, 0xffff);  Enable AUDGLB */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0010);
			/* Enable AUDGLB */
		}
		NvRegCount++;
	} else {
		NvRegCount--;
		if (NvRegCount == 0) {
			/* Ana_Set_Reg(AUDDEC_ANA_CON9, 0x1155, 0xffff);  Disable AUDGLB */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0010, 0x0010);
			/* Disable AUDGLB */
		}
		if (NvRegCount < 0) {
			pr_warn("NvRegCount <0 =%d\n ", NvRegCount);
			NvRegCount = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

static int SwOperationCount;
static void SwOperationEnable(bool enable)
{
	pr_aud("%s SwOperationCount == %d enable = %d\n", __func__, SwOperationCount, enable);
	mutex_lock(&Ana_SwOp_Mutex);
	if (enable == true) {
		if (SwOperationCount == 0) {
			Ana_Set_Reg(LDO_VA25_CON0, 0x0001, 0x0001);
			Ana_Set_Reg(LDO_VA25_OP_EN, 0x0001, 0x0003);
			/* Enable SW OPERATION */
		}
		SwOperationCount++;
	} else {
		SwOperationCount--;
		if (SwOperationCount == 0) {
			Ana_Set_Reg(LDO_VA25_CON0, 0x0000, 0x0001);
			Ana_Set_Reg(LDO_VA25_OP_EN, 0x0000, 0x0007);
			/* Disable SW OPERATION */
		}
		if (SwOperationCount < 0) {
			pr_warn("SwOperationCount <0 =%d\n ", NvRegCount);
			SwOperationCount = 0;
		}
	}
	mutex_unlock(&Ana_SwOp_Mutex);
}

static int anc_clk_counter;
static void anc_clk_enable(bool _enable)
{
	pr_aud("anc_clk_enable, anc_clk_counter = %d, enable = %d\n",
		anc_clk_counter, _enable);
	mutex_lock(&Ana_Clk_Mutex);
	if (_enable == true) {
		if (anc_clk_counter == 0) {
			Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 0, 0x1 << 0);
			/* release hp_anc_pdn */
		}
		anc_clk_counter++;
	} else {
		anc_clk_counter--;
		if (anc_clk_counter == 0) {
			Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 0, 0x1 << 0);
			/* pdn hp_anc_pdn */
		}
		if (anc_clk_counter < 0) {
			pr_warn("anc_clk_counter < 0 = %d\n", anc_clk_counter);
			anc_clk_counter = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

static int anc_ul_src_counter;
static void anc_ul_src_enable(bool _enable)
{
	pr_aud("anc_ul_src_enable, anc_ul_src_counter = %d, enable = %d\n",
		anc_ul_src_counter, _enable);
	mutex_lock(&Ana_Clk_Mutex);
	if (_enable == true) {
		if (anc_ul_src_counter == 0) {
			Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 1, 0x1 << 1);
			/* enable anc ul src */
		}
		anc_ul_src_counter++;
	} else {
		anc_ul_src_counter--;
		if (anc_ul_src_counter == 0) {
			Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 1, 0x1 << 1);
			/* disable anc ul src */
		}
		if (anc_ul_src_counter < 0) {
			pr_warn("anc_clk_counter < 0 = %d\n",
				anc_ul_src_counter);
			anc_ul_src_counter = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

#ifdef CONFIG_MTK_VOW_SUPPORT
static int VOW12MCKCount;
static int VOW32KCKCount;
#endif

#ifdef CONFIG_MTK_VOW_SUPPORT

static void VOW12MCK_Enable(bool enable)
{
	/* pr_debug("VOW12MCK_Enable VOW12MCKCount == %d enable = %d\n", VOW12MCKCount, enable); */
	mutex_lock(&Ana_Clk_Mutex);
	if (enable == true) {
		if (VOW12MCKCount == 0)
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x0040, 0x0040);
		/* Enable  TOP_CKPDN_CON0 bit6 for enable VOW 12M clock */
		VOW12MCKCount++;
	} else {
		VOW12MCKCount--;
		if (VOW12MCKCount == 0)
			Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x0040, 0x0040);
		/* disable TOP_CKPDN_CON0 bit6 for enable VOW 12M clock */

		if (VOW12MCKCount < 0) {
			pr_debug("VOW12MCKCount <0 =%d\n ", VOW12MCKCount);
			VOW12MCKCount = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}

static void VOW32KCK_Enable(bool enable)
{
	/* pr_debug("VOW32KCK_Enable VOW32KCKCount == %d enable = %d\n", VOW32KCKCount, enable); */
	mutex_lock(&Ana_Clk_Mutex);
	if (enable == true) {
		if (VOW32KCKCount == 0)
			Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x0020, 0x0020);
		/* Enable  TOP_CKPDN_CON0 bit5 for enable VOW 32k clock (for periodic on/off use)*/
		VOW32KCKCount++;
	} else {
		VOW32KCKCount--;
		if (VOW32KCKCount == 0)
			Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x0020, 0x0020);
		/* disable TOP_CKPDN_CON0 bit5 for enable VOW 32k clock */

		if (VOW32KCKCount < 0) {
			pr_debug("VOW32KCKCount <0 =%d\n ", VOW32KCKCount);
			VOW32KCKCount = 0;
		}
	}
	mutex_unlock(&Ana_Clk_Mutex);
}
#endif

void vow_irq_handler(void)
{
#ifdef CONFIG_MTK_VOW_SUPPORT

	pr_debug("vow_irq_handler,audio irq event....\n");
	/* TurnOnVOWADcPower(AUDIO_ANALOG_DEVICE_IN_ADC1, false); */
	/* TurnOnVOWDigitalHW(false); */
#if defined(VOW_TONE_TEST)
	EnableSineGen(Soc_Aud_InterConnectionOutput_O03, Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
#endif
	/* VowDrv_ChangeStatus(); */
#endif
}

/*extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);*/

void read_efuse_dpd(void)
{
	int i = 0;

	dpd_on = (Ana_Get_Reg(OTP_DOUT_352_367) >> 12) & 0x1;
	dpd_version = ((Ana_Get_Reg(OTP_DOUT_352_367) >> 13) & 0x7) +
		      ((Ana_Get_Reg(OTP_DOUT_368_383) & 0x1) << 3);
	dpd_lch[DPD_16K][DPD_HD2] = (Ana_Get_Reg(OTP_DOUT_256_271) >> 8) & 0x1f;
	dpd_lch[DPD_16K][DPD_HD3] = ((Ana_Get_Reg(OTP_DOUT_256_271) >> 13) & 0x7) +
				    ((Ana_Get_Reg(OTP_DOUT_272_287) & 0x3) << 3);
	dpd_lch[DPD_16K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_272_287) >> 2) & 0x1f;
	dpd_lch[DPD_16K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_272_287) >> 7) & 0x1f;
	dpd_rch[DPD_16K][DPD_HD2] = ((Ana_Get_Reg(OTP_DOUT_272_287) >> 12) & 0xf) +
				    ((Ana_Get_Reg(OTP_DOUT_288_303) & 0x1) << 4);
	dpd_rch[DPD_16K][DPD_HD3] = (Ana_Get_Reg(OTP_DOUT_288_303) >> 1) & 0x1f;
	dpd_rch[DPD_16K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_288_303) >> 6) & 0x1f;
	dpd_rch[DPD_16K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_288_303) >> 11) & 0x1f;
	dpd_lch[DPD_32K][DPD_HD2] = Ana_Get_Reg(OTP_DOUT_304_319) & 0x1f;
	dpd_lch[DPD_32K][DPD_HD3] = (Ana_Get_Reg(OTP_DOUT_304_319) >> 5) & 0x1f;
	dpd_lch[DPD_32K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_304_319) >> 10) & 0x1f;
	dpd_lch[DPD_32K][DPD_HD5] = ((Ana_Get_Reg(OTP_DOUT_304_319) >> 15) & 0x1) +
				    ((Ana_Get_Reg(OTP_DOUT_320_335) & 0xf) << 1);
	dpd_rch[DPD_32K][DPD_HD2] = (Ana_Get_Reg(OTP_DOUT_320_335) >> 4) & 0x1f;
	dpd_rch[DPD_32K][DPD_HD3] = (Ana_Get_Reg(OTP_DOUT_320_335) >> 9) & 0x1f;
	dpd_rch[DPD_32K][DPD_HD4] = ((Ana_Get_Reg(OTP_DOUT_320_335) >> 14) & 0x3) +
				    ((Ana_Get_Reg(OTP_DOUT_336_351) & 0x7) << 2);
	dpd_rch[DPD_32K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_336_351) >> 3) & 0x1f;
	dpd_lch[DPD_560K][DPD_HD2] = (Ana_Get_Reg(OTP_DOUT_336_351) >> 8) & 0x1f;
	dpd_lch[DPD_560K][DPD_HD3] = ((Ana_Get_Reg(OTP_DOUT_336_351) >> 13) & 0x7) +
				     ((Ana_Get_Reg(OTP_DOUT_352_367) & 0x3) << 3);
	dpd_lch[DPD_560K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_352_367) >> 2) & 0x1f;
	dpd_lch[DPD_560K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_352_367) >> 7) & 0x1f;
	dpd_rch[DPD_560K][DPD_HD2] = Ana_Get_Reg(OTP_DOUT_448_463) & 0x1f;
	dpd_rch[DPD_560K][DPD_HD3] = (Ana_Get_Reg(OTP_DOUT_448_463) >> 5) & 0x1f;
	dpd_rch[DPD_560K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_448_463) >> 10) & 0x1f;
	dpd_rch[DPD_560K][DPD_HD5] = ((Ana_Get_Reg(OTP_DOUT_448_463) >> 15) & 0x1) +
				     ((Ana_Get_Reg(OTP_DOUT_464_479) & 0xf) << 1);
	dpd_lch[DPD_1K][DPD_HD2] = (Ana_Get_Reg(OTP_DOUT_464_479) >> 4) & 0x1f;
	dpd_lch[DPD_1K][DPD_HD3] = (Ana_Get_Reg(OTP_DOUT_464_479) >> 9) & 0x1f;
	dpd_lch[DPD_1K][DPD_HD4] = ((Ana_Get_Reg(OTP_DOUT_464_479) >> 14) & 0x3) +
				   ((Ana_Get_Reg(OTP_DOUT_480_495) & 0x7) << 2);
	dpd_lch[DPD_1K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_480_495) >> 3) & 0x1f;
	dpd_rch[DPD_1K][DPD_HD2] = (Ana_Get_Reg(OTP_DOUT_480_495) >> 8) & 0x1f;
	dpd_rch[DPD_1K][DPD_HD3] = ((Ana_Get_Reg(OTP_DOUT_480_495) >> 13) & 0x7) +
				   ((Ana_Get_Reg(OTP_DOUT_496_511) & 0x3) << 3);
	dpd_rch[DPD_1K][DPD_HD4] = (Ana_Get_Reg(OTP_DOUT_496_511) >> 2) & 0x1f;
	dpd_rch[DPD_1K][DPD_HD5] = (Ana_Get_Reg(OTP_DOUT_496_511) >> 7) & 0x1f;

	for (i = DPD_16K; i < DPD_IMPEDANCE_MAX; i++) {
		pr_debug("%s dpd type(%d) lch : %u, %u, %u, %u   rch : %u, %u, %u, %u\n", __func__, i,
			 dpd_lch[i][DPD_HD2], dpd_lch[i][DPD_HD3],
			 dpd_lch[i][DPD_HD4], dpd_lch[i][DPD_HD5],
			 dpd_rch[i][DPD_HD2], dpd_rch[i][DPD_HD3],
			 dpd_rch[i][DPD_HD4], dpd_rch[i][DPD_HD5]);
	}
}

void mtk_read_dpd_parameter(struct mtk_dpd_param *dpd_param)

{
	int a2_lch = 0, a3_lch = 0, a2_rch = 0, a3_rch = 0;
	int a4_lch = 0, a5_lch = 0, a4_rch = 0, a5_rch = 0;

	if (hp_differential_mode) {
		a2_lch = dpd_offset_table[12][dpd_lch[DPD_1K][DPD_HD2]];
		a3_lch = dpd_offset_table[13][dpd_lch[DPD_1K][DPD_HD3]];
		a2_rch = dpd_offset_table[14][dpd_rch[DPD_1K][DPD_HD2]];
		a3_rch = dpd_offset_table[15][dpd_rch[DPD_1K][DPD_HD3]];
	} else if (hp_impedance < 24) {
		a2_lch = dpd_offset_table[0][dpd_lch[DPD_16K][DPD_HD2]] - 1;
		a3_lch = dpd_offset_table[1][dpd_lch[DPD_16K][DPD_HD3]];
		a2_rch = dpd_offset_table[2][dpd_rch[DPD_16K][DPD_HD2]] - 1;
		a3_rch = dpd_offset_table[3][dpd_rch[DPD_16K][DPD_HD3]];
	} else if (hp_impedance < 100) {
		a2_lch = dpd_offset_table[4][dpd_lch[DPD_32K][DPD_HD2]] - 1;
		a3_lch = dpd_offset_table[5][dpd_lch[DPD_32K][DPD_HD3]] - 1;
		a2_rch = dpd_offset_table[6][dpd_rch[DPD_32K][DPD_HD2]] - 1;
		a3_rch = dpd_offset_table[7][dpd_rch[DPD_32K][DPD_HD3]] - 1;
	} else if (hp_impedance < 1000) {
		a2_lch = dpd_offset_table[8][dpd_lch[DPD_560K][DPD_HD2]];
		a3_lch = dpd_offset_table[9][dpd_lch[DPD_560K][DPD_HD3]];
		a2_rch = dpd_offset_table[10][dpd_rch[DPD_560K][DPD_HD2]];
		a3_rch = dpd_offset_table[11][dpd_rch[DPD_560K][DPD_HD3]];
	}
	a2_lch = (a2_lch < 12) && (a2_lch > -12) ? 0 : a2_lch;
	a3_lch = (a3_lch < 12) && (a3_lch > -12) ? 0 : a3_lch;
	a2_rch = (a2_rch < 12) && (a2_rch > -12) ? 0 : a2_rch;
	a3_rch = (a3_rch < 12) && (a3_rch > -12) ? 0 : a3_rch;

	dpd_param->efuse_on = dpd_on;
	dpd_param->version = dpd_version;
	dpd_param->a2_lch = a2_lch;
	dpd_param->a3_lch = a3_lch;
	dpd_param->a4_lch = a4_lch;
	dpd_param->a5_lch = a5_lch;
	dpd_param->a2_rch = a2_rch;
	dpd_param->a3_rch = a3_rch;
	dpd_param->a4_rch = a4_rch;
	dpd_param->a5_rch = a5_rch;
	pr_warn("%s(), impedance = %d, dpd_param lch : %d, %d, %d, %d   rch : %d, %d, %d, %d\n", __func__,
		hp_impedance, a2_lch, a3_lch, a4_lch, a5_lch, a2_rch, a3_rch, a4_rch, a5_rch);
}


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

#ifdef CONFIG_MTK_SPEAKER
static void Apply_Speaker_Gain(void)
{
	pr_warn("%s Speaker_pga_gain= %d\n", __func__, Speaker_pga_gain);
	Ana_Set_Reg(SPK_ANA_CON0, Speaker_pga_gain << 11, 0x7800);
}
#else
static void Apply_Speaker_Gain(void)
{
	Ana_Set_Reg(ZCD_CON1,
		    (mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTR] << 7) |
		    mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_LINEOUTL],
		    0x0f9f);
}
#endif

static void setOffsetTrimMux(unsigned int Mux)
{
	pr_aud("%s Mux = %d\n", __func__, Mux);
	Ana_Set_Reg(AUDDEC_ANA_CON8, Mux << 1, 0xf << 1);	/* Audio offset trimming buffer mux selection */
}

static void setOffsetTrimBufferGain(unsigned int gain)
{
	Ana_Set_Reg(AUDDEC_ANA_CON8, gain << 5, 0x3 << 5);	/* Audio offset trimming buffer gain selection */
}

static int hpl_trim_code, hpr_trim_code;
static int hpl_dc_offset, hpr_dc_offset;
static int dctrim_calibrated;
static int last_lch_comp_value, last_rch_comp_value;
static const char * const dctrim_control_state[] = { "Not_Yet", "Calibrating", "Calibrated"};
static const int dBFactor_Nom[45] = {
	2058, 2309, 2591, 2907, 3261, 3659, 4106, 4607,
	5169, 5799, 6507, 7301, 8192, 9192, 10313, 11572,
	12983, 14568, 16345, 18340, 20577, 23088, 25905, 29066,
	32613, 36592, 41057, 46067, 51688, 57995, 65071, 73011,
	81920, 91916, 103131, 115715, 129834, 145677, 163452, 183396,
	205774, 230882, 259054, 290663, 326129
};
static const int dBFactor_Den = 8192;

static void EnableTrimbuffer(bool benable)
{
	if (benable == true) {
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0001, 0x0001);
		/* Audio offset trimming buffer enable */
	} else {
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0x0001);
		/* Audio offset trimming buffer disable */
	}
}

static int read_average_auxadc_value(int channels, unsigned int times)
{
	const int default_size = 20;
	int counter = 0, average = 0;
	int offset[default_size];

	if (times > default_size) {
		pr_err("%s(), times %d over the array size\n", __func__, times);
		return 0;
	}

	setOffsetTrimMux(channels);
	setOffsetTrimBufferGain(3); /* TrimBufferGain 18db */
	EnableTrimbuffer(true);

	usleep_range(1 * 1000, 5 * 1000);

	for (counter = 0; counter < times; ++counter)
		offset[counter] = audio_get_auxadc_value();

	EnableTrimbuffer(false);
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_GROUND);

	for (counter = 0; counter < times; ++counter)
		average = average + offset[counter];

	average = DIV_ROUND_CLOSEST(average, times);

	return average;
}

void set_hp_trim_code(bool enable, int lch, int rch)
{
	if (lch < -8 || lch > 7 || rch < -8 || rch > 7) {
		pr_err("%s(), trimcode out of range lch = %d, rch = %d\n", __func__, lch, rch);
		return;
	}

	lch = lch < 0 ? ~lch | 0x1000 : lch;
	rch = rch < 0 ? ~rch | 0x1000 : rch;

	Ana_Set_Reg(AUDDEC_ANA_CON3, enable | lch << 1 | rch << 7, 0x079F);
}

void trigger_headphone_dctrim_hardware(int channels, bool accurate, int trimcode, int *base,
				       int *point1, int *point2, int *point3)
{
	int avg_times = accurate ? 20 : 5;

	pr_aud("%s(), Enable flow\n", __func__);
	TurnOnDacPower();

	/* reset all dc compensation */
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG0, 0x0000, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG1, 0x0000, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG3, 0x0000, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG4, 0x0000, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG2, 0x0000, 0xffff);

	Ana_Set_Reg(AFUNC_AUD_CON0, 0xC3AD, 0xf7ff);
	/* close sdm */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0x8000);
	/* Pull-down HPL/R to AVSS30_AUD for de-pop noise */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
	/* Reduce ESD resistance of AU_REFN */
	Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0x002c, 0x003f);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0x002c, 0x003f);
	/* Set HPL/HPR gain to -32db */

	usleep_range(100, 200);

	Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0x002D);
	/* Enable cap-less LDOs (1.6V) */
	Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
	/* Enable NV regulator */
	Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
	/* Set NV regulator to -1.5V */

	usleep_range(100, 200);

	Ana_Set_Reg(ZCD_CON0, 0x0000, 0x0001);
	/* Disable AUD_ZCD */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0x3000);
	/* Disable headphone short-ckt protection. */
	Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
	if (low_power_mode == 0) {
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xff80);
	} else {
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4880, 0xff80);
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A00, 0xff80);
	}
	/* Enable IBIST */
	/* Set HP DR bias and HP & ZCD bias current optimization */
	if (hp_differential_mode)
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x001B, 0x003f);
	else
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x001A, 0x003f);
	/* Set HPP/N STB enhance circuits */
	Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0004, 0x000E);
	/* Set HP bias in HIFI mdoe */

	set_hp_trim_code(true, trimcode, trimcode);
	/* enable HP trim code */

	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x000C, 0x000C);
	/* Enable HP aux output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0030, 0x0030);
	/* Enable HP aux feedback loop */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0D00, 0xff00);
	/* Enable HP aux CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x00C0, 0x00C0);
	/* Enable HP driver bias circuits */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0030, 0x0030);
	/* Enable HP driver core circuits */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00C0, 0x00C0);
	/* Short HP main output to HP aux output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0F00, 0xff00);
	/* Enable HP main CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0001, 0x0001);
	/* Change compensation for HP main loop */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0300, 0xff00);
	/* Disable HP aux CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0003, 0x0003);
	/* Enable HP main output stage */
	hp_main_output_ramp(true);

	if (!accurate)
		*point1 = read_average_auxadc_value(channels, avg_times);

	hp_aux_feedback_loop_gain_ramp(true);
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0030);
	/* Disable HP aux feedback loop */
	usleep_range(100, 150);
	hp_gain_ramp(true);
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x000C);
	/* Disable HP aux output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x00C0);
	/* Unshort HP main output to HP aux output stage */
	usleep_range(100, 150);
	Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
	/* Enable AUD_CLK */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x000f, 0x000f);
	/* Enable Audio DAC  */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x8000, 0x8000);
	/* Enable low-noise mode of DAC  */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0A00, 0x0f00);
	/* Switch HPL/R MUX to audio DAC  */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x8000);
	/* No Pull-down HPL/R to AVSS30_AUD for de-pop noise */
	Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0080, 0xffff);
	/* Set HS STB enhance circuits */

	*point2 = read_average_auxadc_value(channels, avg_times);
	*base = read_average_auxadc_value(AUDIO_OFFSET_TRIM_MUX_HSP, avg_times);

	if (!accurate) {
		if ((*point2 + *point1) / 2 > *base)
			set_hp_trim_code(true, 7, 7);
		else
			set_hp_trim_code(true, -7, -7);

		*point3 = read_average_auxadc_value(channels, avg_times);
	}

	pr_aud("%s(), Disable flow\n", __func__);
	Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0xffff);
	/* Disable HS STB enhance circuit */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0x8000);
	/* Pull-down HPL/R to AVSS30_AUD for de-pop noise */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0xf200, 0xff00);
	/* Set HP status as power-down */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
	/* Open HPL/R MUX to audio DAC  */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x7200, 0xff00);
	/* Disable low-noise mode of DAC */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);
	/* Disable Audio DAC */
	Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
	/* Disable AUD_CLK  */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00C0, 0x00C0);
	/* Short HP main output to HP aux output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x000C, 0x000C);
	/* Enable HP aux output stage */
	hp_gain_ramp(false);
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0030, 0x0030);
	/* Enable HP aux feedback loop */
	hp_aux_feedback_loop_gain_ramp(false);
	hp_main_output_ramp(false);
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0003);
	/* Disable HP main output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0e00, 0xff00);
	/* Enable HP aux CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0x0001);
	/* Change compensation for HP aux loop */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0c00, 0xff00);
	/* Enable HP aux CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x00C0);
	/* Unshort HP main output to HP aux output stage */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0030);
	/* Disable HP driver core circuits */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x00C0);
	/* Disable HP driver bias circuits */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xff00);
	/* Disable HP aux CMFB loop */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0030);
	/* Disable HP aux feedback loop */
	Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x000C);
	/* Disable HP aux output stage */
	set_hp_trim_code(false, 0, 0);
	/* Disable HP trim code index 0 */
	Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x8000);
	/* No Pull-down HPL/R to AVSS30_AUD for de-pop noise */
	Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
	/* Disable IBIST */
	Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
	/* Restore NV regulator to -1.3V */
	Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
	/* Disable NV regulator */
	Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0x002D);
	/* Disable cap-less LDOs (1.6V) */
	TurnOffDacPower();
}

static void pmic_get_dctrim_parameter(enum audio_offset_trim_mux channel, int *hp_trim_code, int *hp_trim_offset)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	int point1 = 0, point2 = 0, base = 0, point3 = 0;
	int max1, max2, middle;

	if (channel != AUDIO_OFFSET_TRIM_MUX_HPL &&
	    channel != AUDIO_OFFSET_TRIM_MUX_HPR){
		pr_warn("%s(), Not support trim mux channel(%d)\n", __func__, channel);
		return;
	}

	trigger_headphone_dctrim_hardware(channel, false, 0, &base, &point1, &point2, &point3);

	max1 = point1 - base;
	max2 = point2 - base;
	middle = ((point1 + point2) / 2 - base);
	*hp_trim_code = middle * 7 / (point2 - point3);

	pr_debug("%s(), max1=%d, max2=%d, middle=%d, base=%d, point3=%d\n", __func__, max1, max2, middle, base, point3);

	trigger_headphone_dctrim_hardware(channel, true, *hp_trim_code, &base, NULL, &point2, NULL);

	*hp_trim_offset = point2 - base;

	pr_debug("%s(), accurate: point2=%d, base=%d\n", __func__, point2, base);
	pr_debug("%s(), channel = %d, hp_trim_code = %d, hp_trim_offset = %d\n",
		 __func__, channel, *hp_trim_code, *hp_trim_offset);
#endif
}

static int pmic_dc_offset_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), %d, %d\n", __func__, hpl_dc_offset, hpr_dc_offset);
	ucontrol->value.integer.value[0] = hpl_dc_offset;
	ucontrol->value.integer.value[1] = hpr_dc_offset;
	return 0;
}

static int pmic_dc_offset_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), %ld, %ld\n", __func__, ucontrol->value.integer.value[0], ucontrol->value.integer.value[1]);
	hpl_dc_offset = ucontrol->value.integer.value[0];
	hpr_dc_offset = ucontrol->value.integer.value[1];
	return 0;
}

static int pmic_hp_trim_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), %d, %d\n", __func__, hpl_trim_code, hpr_trim_code);
	ucontrol->value.integer.value[0] = hpl_trim_code;
	ucontrol->value.integer.value[1] = hpr_trim_code;
	return 0;
}

static int pmic_hp_trim_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), %ld, %ld\n", __func__, ucontrol->value.integer.value[0], ucontrol->value.integer.value[1]);
	hpl_trim_code = ucontrol->value.integer.value[0];
	hpr_trim_code = ucontrol->value.integer.value[1];
	return 0;
}

static int pmic_dctrim_control_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), dctrim_calibrated = %d\n", __func__, dctrim_calibrated);
	ucontrol->value.integer.value[0] = dctrim_calibrated;
	return 0;
}

static int pmic_dctrim_control_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(dctrim_control_state)) {
		pr_err("%s(), return -EINVAL\n", __func__);
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 1) {
		pr_debug("%s(), Start DCtrim Calibrating", __func__);
		pmic_get_dctrim_parameter(AUDIO_OFFSET_TRIM_MUX_HPL, &hpl_trim_code, &hpl_dc_offset);
		pmic_get_dctrim_parameter(AUDIO_OFFSET_TRIM_MUX_HPR, &hpr_trim_code, &hpr_dc_offset);
		dctrim_calibrated = 2;
		pr_debug("%s(), End DCtrim Calibrating", __func__);
	} else {
		dctrim_calibrated = ucontrol->value.integer.value[0];
	}
	return 0;
}

static bool OpenHeadPhoneImpedanceSetting(bool bEnable)
{
	pr_aud("%s benable = %d\n", __func__, bEnable);
	if (GetDLStatus() == true || ANC_enabled)
		return false;

	if (bEnable == true) {
		TurnOnDacPower();

		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0300, 0xff00);
		/* Set HP status as power-up */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
		/* Reduce ESD resistance of AU_REFN */

		udelay(100);

		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0x002D);
		/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
		/* Enable NV regulator */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
		/* Set NV regulator to -1.5V */

		udelay(100);

		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON3, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0xffff);
		/* reset other register to default, influence the output voltage */

		Ana_Set_Reg(ZCD_CON0, 0x0000, 0x0001);
		/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0x3000);
		/* Disable headphone short-ckt protection. */
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x003f);
		/* Disable HPP/N STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0x0009);
		/* Enable LCH Audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x1300, 0x1f00);
		/* Enable HPDET circuit, select DACLP as HPDET input and HPR as HPDET output */
	} else {
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0x1f00);
		/* Disable HPDET circuit */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0009);
		/* Disable Audio DAC */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
		/* Disable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
		/* Disable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
		/* Restore NV regulator to -1.3V */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
		/* Disable NV regulator */
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0x002D);
		/* Disable cap-less LDOs (1.6V) */

		TurnOffDacPower();
	}
	return true;
}

static void mtk_read_hp_detection_parameter(struct mtk_hpdet_param *hpdet_param)
{
	hpdet_param->auxadc_upper_bound = 32630; /* should little lower than auxadc max resolution */
	hpdet_param->dc_Step = 100; /* Dc ramp up and ramp down step */
	hpdet_param->dc_Phase0 = 300; /* Phase 0 : high impedance with worst resolution */
	hpdet_param->dc_Phase1 = 1500; /* Phase 1 : median impedance with normal resolution */
	hpdet_param->dc_Phase2 = 6000; /* Phase 2 : low impedance with better resolution */
	hpdet_param->resistance_first_threshold = 250; /* Resistance Threshold of phase 2 and phase 1 */
	hpdet_param->resistance_second_threshold = 1000; /* Resistance Threshold of phase 1 and phase 0 */
}

static int mtk_calculate_impedance_formula(int pcm_offset, int aux_diff)
{
	/* The formula is from DE programming guide */
	/* should be mantain by pmic owner */
	/* R = V /I */
	/* V = auxDiff * (1800mv /auxResolution)  /TrimBufGain */
	/* I =  pcmOffset * DAC_constant * Gsdm * Gibuf */
	return DIV_ROUND_CLOSEST(3600000 / pcm_offset * aux_diff, 7832);
}

static int mtk_calculate_hp_impedance(int dc_init, int dc_input, short pcm_offset,
				      const unsigned int detect_times)
{
	int dc_value;
	int r_tmp = 0;

	if (dc_input < dc_init) {
		pr_warn("%s(), Wrong[%d] : dc_input(%d) > dc_init(%d)\n", __func__, pcm_offset, dc_input, dc_init);
		return 0;
	}

	dc_value = dc_input - dc_init;
	r_tmp = mtk_calculate_impedance_formula(pcm_offset, dc_value);
	r_tmp = DIV_ROUND_CLOSEST(r_tmp, detect_times);

	/* Efuse calibration */
	if ((efuse_current_calibration != 0) && (r_tmp != 0)) {
		pr_aud("%s(), Before Calibration from EFUSE: %d, R: %d\n",
		       __func__, efuse_current_calibration, r_tmp);
		r_tmp = DIV_ROUND_CLOSEST(r_tmp * (128 + efuse_current_calibration), 128);
	}

	pr_aud("%s(), pcm_offset %d dcoffset %d detected resistor is %d\n",
	       __func__, pcm_offset, dc_value, r_tmp);

	return r_tmp;
}

static void setHpGainZero(void)
{
	Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0x000c, 0x003f);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0x000c, 0x003f);
}

static int detect_impedance_by_phase(void)
{
	const unsigned int kDetectTimes = 8;
	unsigned int counter;
	int dcSum = 0, detectSum = 0;
	int detectsOffset[kDetectTimes];
	unsigned short pick_impedance = 0, detect_impedance = 0, phase_flag = 0, dcValue = 0;
	struct mtk_hpdet_param hpdet_param;

	mtk_read_hp_detection_parameter(&hpdet_param);

	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPR);
	setOffsetTrimBufferGain(3); /* HPDET trim. buffer gain : 18db */
	EnableTrimbuffer(true);
	setHpGainZero();
	set_lch_dc_compensation_reg(0);
	set_rch_dc_compensation_reg(0);
	enable_dc_compensation(true);

	for (dcValue = 0; dcValue <= hpdet_param.dc_Phase2; dcValue += hpdet_param.dc_Step) {

		/* apply dc by dc compensation: 16bit and negative value */
		set_lch_dc_compensation_reg(-dcValue << 8);
		set_rch_dc_compensation_reg(-dcValue << 8);

		/* save for DC =0 offset */
		if (dcValue == 0) {
			usleep_range(1*1000, 1*1000);
			dcSum = 0;
			for (counter = 0; counter < kDetectTimes; counter++) {
				detectsOffset[counter] = audio_get_auxadc_value();
				dcSum = dcSum + detectsOffset[counter];
			}
		}

		/* start checking */
		if (dcValue == hpdet_param.dc_Phase0) {
			usleep_range(1*1000, 1*1000);
			detectSum = 0;
			detectSum = audio_get_auxadc_value();
			pick_impedance = mtk_calculate_hp_impedance(dcSum/kDetectTimes,
								    detectSum, dcValue, 1);

			if (pick_impedance < hpdet_param.resistance_first_threshold) {
				phase_flag = 2;
				continue;
			} else if (pick_impedance < hpdet_param.resistance_second_threshold) {
				phase_flag = 1;
				continue;
			}

			/* Phase 0 : detect  range 1kohm to 5kohm impedance */
			for (counter = 1; counter < kDetectTimes; counter++) {
				detectsOffset[counter] = audio_get_auxadc_value();
				detectSum = detectSum + detectsOffset[counter];
			}
			/* if detect auxadc value over 32630 , the hpImpedance is over 5k ohm */
			if ((detectSum / kDetectTimes) > hpdet_param.auxadc_upper_bound)
				detect_impedance = auxcable_impedance;
			else
				detect_impedance = mtk_calculate_hp_impedance(dcSum, detectSum,
									      dcValue, kDetectTimes);
			break;
		}

		/* Phase 1 : detect  range 250ohm to 1000ohm impedance */
		if (dcValue == hpdet_param.dc_Phase1 && phase_flag == 1) {
			usleep_range(1*1000, 1*1000);
			detectSum = 0;
			for (counter = 0; counter < kDetectTimes; counter++) {
				detectsOffset[counter] = audio_get_auxadc_value();
				detectSum = detectSum + detectsOffset[counter];
			}
			detect_impedance = mtk_calculate_hp_impedance(dcSum, detectSum,
								      dcValue, kDetectTimes);
			break;
		}

		/* Phase 2 : detect under 250ohm impedance */
		if (dcValue == hpdet_param.dc_Phase2 && phase_flag == 2) {
			usleep_range(1*1000, 1*1000);
			detectSum = 0;
			for (counter = 0; counter < kDetectTimes; counter++) {
				detectsOffset[counter] = audio_get_auxadc_value();
				detectSum = detectSum + detectsOffset[counter];
			}
			detect_impedance = mtk_calculate_hp_impedance(dcSum, detectSum,
								      dcValue, kDetectTimes);
			break;
		}
		usleep_range(1*200, 1*200);
	}

	pr_debug("%s(), phase %d [dc,detect]Sum %d times = [%d,%d], hp_impedance = %d, pick_impedance = %d\n",
		 __func__, phase_flag, kDetectTimes, dcSum, detectSum, detect_impedance, pick_impedance);

	/* Ramp-Down */
	while (dcValue > 0) {
		dcValue = dcValue - hpdet_param.dc_Step;
		/* apply dc by dc compensation: 16bit and negative value */
		set_lch_dc_compensation_reg(-dcValue << 8);
		set_rch_dc_compensation_reg(-dcValue << 8);
		usleep_range(1*200, 1*200);
	}
	enable_dc_compensation(false);
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_GROUND);
	EnableTrimbuffer(false);

	return detect_impedance;
}

static int pmic_hp_impedance_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	if (OpenHeadPhoneImpedanceSetting(true) == true) {
		hp_impedance = detect_impedance_by_phase();
		OpenHeadPhoneImpedanceSetting(false);
	} else
		pr_warn("%s(), Pmic DL Busy, HPDET do nothing\n", __func__);

	ucontrol->value.integer.value[0] = hp_impedance;
	pr_debug("-%s(), mhp_impedance = %d, efuse = %d\n",
		 __func__, hp_impedance, efuse_current_calibration);
	return 0;
}

static int pmic_hp_impedance_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), hp_impedance = %ld\n", __func__, ucontrol->value.integer.value[0]);
	hp_impedance = ucontrol->value.integer.value[0];
	return 0;
}

void mtkaif_calibration_set_loopback(bool enable)
{
	if (enable) {
		audckbufEnable(true);
		ClsqEnable(true);	/* Turn on 26MHz source clock */
		Topck_Enable(true);	/* Turn on AUDNCP_CLKDIV engine clock */
	} else {
		Topck_Enable(false);
		ClsqEnable(false);
		audckbufEnable(false);
	}
	/* set data miso and miso2 loopback */
	Ana_Set_Reg(AUD_TOP_CFG, enable << 7 | enable << 15, 0x8080);
}

void mtkaif_calibration_set_phase(int mode1, int mode2)
{
	/* phase mode is how many delay chain going through */
	Ana_Set_Reg(AUD_TOP_CFG, mode1 | (mode2 << 8), 0x1f1f);
}

static int calOffsetToDcComp(int TrimOffset)
{
	/* The formula is from DE programming guide */
	/* should be mantain by pmic owner */
	return DIV_ROUND_CLOSEST(TrimOffset * 2804225, 32768);
}

static void set_lch_dc_compensation_reg(int lch_value)
{
	unsigned short dcCompLchHigh = 0, dcCompLchLow = 0;

	dcCompLchHigh = (unsigned short)(lch_value >> 8) & 0x0000ffff;
	dcCompLchLow = (unsigned short)(lch_value << 8) & 0x0000ffff;
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG0, dcCompLchHigh, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG3, dcCompLchLow, 0xffff);
}

static void set_rch_dc_compensation_reg(int rch_value)
{
	unsigned short dcCompRchHigh = 0, dcCompRchLow = 0;

	dcCompRchHigh = (unsigned short)(rch_value >> 8) & 0x0000ffff;
	dcCompRchLow = (unsigned short)(rch_value << 8) & 0x0000ffff;
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG1, dcCompRchHigh, 0xffff);
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG4, dcCompRchLow, 0xffff);
}

static void enable_dc_compensation(bool enable)
{
#ifndef EFUSE_HP_TRIM
	Ana_Set_Reg(AFE_DL_DC_COMP_CFG2, enable, 0x1);
#endif
}

static void SetDcCompenSation(bool enable)
{
	int lch_value = 0, rch_value = 0, tmp_ramp = 0;
	int times = 0, i = 0;
	int sign_lch = 0, sign_rch = 0;
	int abs_lch = 0, abs_rch = 0;
	int index_lgain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];
	int index_rgain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	int diff_lch = 0, diff_rch = 0, ramp_l = 0, ramp_r = 0;

	lch_value = calOffsetToDcComp(DIV_ROUND_CLOSEST(hpl_dc_offset * dBFactor_Nom[index_lgain], dBFactor_Den));
	rch_value = calOffsetToDcComp(DIV_ROUND_CLOSEST(hpr_dc_offset * dBFactor_Nom[index_rgain], dBFactor_Den));
	diff_lch = enable ? lch_value - last_lch_comp_value : lch_value;
	diff_rch = enable ? rch_value - last_rch_comp_value : rch_value;
	sign_lch = diff_lch < 0 ? -1 : 1;
	sign_rch = diff_rch < 0 ? -1 : 1;
	abs_lch = sign_lch * diff_lch;
	abs_rch = sign_rch * diff_rch;
	times = abs_lch > abs_rch ? (abs_lch >> 10) : (abs_rch >> 10);
	pr_aud("%s enable = %d, index_gain = %d, times = %d, lch_value = %d -> %d, rch_value = %d -> %d\n",
	       __func__, enable, index_lgain, times, last_lch_comp_value, lch_value, last_rch_comp_value, rch_value);

	if (enable) {
		enable_dc_compensation(true);
		for (i = 0; i <= times; i++) {
			tmp_ramp = i << 10;
			if (tmp_ramp < abs_lch) {
				ramp_l = last_lch_comp_value + sign_lch * tmp_ramp;
				set_lch_dc_compensation_reg(ramp_l);
			}
			if (tmp_ramp < abs_rch) {
				ramp_r = last_rch_comp_value + sign_rch * tmp_ramp;
				set_rch_dc_compensation_reg(ramp_r);
			}
			udelay(100);
		}
		set_lch_dc_compensation_reg(lch_value);
		set_rch_dc_compensation_reg(rch_value);
		last_lch_comp_value = lch_value;
		last_rch_comp_value = rch_value;
	} else {
		for (i = times; i >= 0; i--) {
			tmp_ramp = i << 10;
			if (tmp_ramp < abs_lch)
				set_lch_dc_compensation_reg(tmp_ramp);

			if (tmp_ramp < abs_rch)
				set_rch_dc_compensation_reg(tmp_ramp);
			udelay(100);
		}
		enable_dc_compensation(false);
		last_lch_comp_value = 0;
		last_rch_comp_value = 0;
	}
}
#if 0
static void SetDCcoupleNP(int MicBias, int mode)
{
	pr_aud("%s MicBias= %d mode = %d\n", __func__, MicBias, mode);
	switch (mode) {
	case AUDIO_ANALOGUL_MODE_ACC:
	case AUDIO_ANALOGUL_MODE_DCC:
	case AUDIO_ANALOGUL_MODE_DMIC:{
			if (MicBias == AUDIO_MIC_BIAS0)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0x000E);
			else if (MicBias == AUDIO_MIC_BIAS1)
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0000, 0x0006);
			else if (MicBias == AUDIO_MIC_BIAS2)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0x0E00);
		}
		break;
	case AUDIO_ANALOGUL_MODE_DCCECMDIFF:{
			if (MicBias == AUDIO_MIC_BIAS0)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x000E, 0x000E);
			else if (MicBias == AUDIO_MIC_BIAS1)
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0004, 0x0006);
			else if (MicBias == AUDIO_MIC_BIAS2)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0E00, 0x0E00);
		}
		break;
	case AUDIO_ANALOGUL_MODE_DCCECMSINGLE:{
			if (MicBias == AUDIO_MIC_BIAS0)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0002, 0x000E);
			else if (MicBias == AUDIO_MIC_BIAS1)
				Ana_Set_Reg(AUDENC_ANA_CON10, 0x0004, 0x0006);
			else if (MicBias == AUDIO_MIC_BIAS2)
				Ana_Set_Reg(AUDENC_ANA_CON9, 0x0200, 0x0E00);
		}
		break;
	default:
		break;
	}
}
#endif
uint32 GetULFrequency(uint32 frequency)
{
	uint32 Reg_value = 0;

	pr_aud("%s frequency =%d\n", __func__, frequency);
	switch (frequency) {
	case 8000:
	case 16000:
	case 32000:
		Reg_value = 0x0;
		break;
	case 48000:
		Reg_value = 0x1;
	default:
		break;

	}
	return Reg_value;
}


uint32 ULSampleRateTransform(uint32 SampleRate)
{
	switch (SampleRate) {
	case 8000:
		return 0;
	case 16000:
		return 1;
	case 32000:
		return 2;
	case 48000:
		return 3;
	case 96000:
		return 4;
	case 192000:
		return 5;
	default:
		return 3;
	}
}

static int mtk_codec_dai_1_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *Daiport)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_aud("mtk_codec_dai_1_prepare set up SNDRV_PCM_STREAM_CAPTURE rate = %d, channel = %d\n",
		       substream->runtime->rate, substream->runtime->channels);
		mBlockSampleRate[AUDIO_DAI_UL1] = substream->runtime->rate;
		/* If 4-ch record set UL1/UL2 same sampleRate */
		if (substream->runtime->channels > 2)
			mBlockSampleRate[AUDIO_DAI_UL2] = substream->runtime->rate;
	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_aud("mtk_codec_dai_1_prepare set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n",
		       substream->runtime->rate);
		mBlockSampleRate[AUDIO_DAI_DL1] = substream->runtime->rate;
	}
	return 0;
}

static int mtk_codec_dai_2_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *Daiport)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_aud("mtk_codec_dai_2_prepare set up SNDRV_PCM_STREAM_CAPTURE rate = %d, channel = %d\n",
		       substream->runtime->rate, substream->runtime->channels);
		mBlockSampleRate[AUDIO_DAI_UL2] = substream->runtime->rate;
		/* If 4-ch record set UL1/UL2 same sampleRate */
		if (substream->runtime->channels > 2)
			mBlockSampleRate[AUDIO_DAI_UL1] = substream->runtime->rate;
	} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_aud("mtk_codec_dai_2_prepare set up SNDRV_PCM_STREAM_PLAYBACK rate = %d\n",
		       substream->runtime->rate);
		mBlockSampleRate[AUDIO_DAI_DL2] = substream->runtime->rate;
	}
	return 0;
}


static const struct snd_soc_dai_ops mtk_codec_dai_1_ops = {
	.prepare = mtk_codec_dai_1_prepare,
};

static const struct snd_soc_dai_ops mtk_codec_dai_2_ops = {
	.prepare = mtk_codec_dai_2_prepare,
};

static struct snd_soc_dai_driver mtk_6331_dai_codecs[] = {
	{
	 .name = MT_SOC_CODEC_TXDAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
	 .capture = {
		     .stream_name = MT_SOC_UL1_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 4,
		     .rates = SOC_HIGH_USE_RATE,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_TDMRX_DAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	  .name = MT_SOC_CODEC_DEEPBUFFER_TX_DAI_NAME,
	  .ops = &mtk_codec_dai_1_ops,
	  .playback = {
		      .stream_name = MT_SOC_DEEP_BUFFER_DL_STREAM_NAME,
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
	 .ops = &mtk_codec_dai_1_ops,
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
		     .channels_max = 4,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_SPKSCPTXDAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
	 .playback = {
		      .stream_name = MT_SOC_DL1SCPSPK_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rate_min = 8000,
		      .rate_max = 192000,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      }
	 },
	{
	 .name = MT_SOC_CODEC_VOICE_MD2DAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
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
		     .channels_max = 4,
		     .rates = SNDRV_PCM_RATE_8000_48000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_VOICE_ULTRADAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
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
	{
		.name = MT_SOC_CODEC_VOICE_USBDAI_NAME,
		.ops = &mtk_codec_dai_1_ops,
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
		.ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_2_ops,
	 .capture = {
		     .stream_name = MT_SOC_UL1DATA2_STREAM_NAME,
		     .channels_min = 1,
		     .channels_max = 4,
		     .rates = SNDRV_PCM_RATE_8000_192000,
		     .formats = SND_SOC_ADV_MT_FMTS,
		     },
	 },
	{
	 .name = MT_SOC_CODEC_MRGRX_DAI_NAME,
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
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
	 .ops = &mtk_codec_dai_1_ops,
	 .playback = {
		      .stream_name = MT_SOC_OFFLOAD_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_192000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		      },
	},
	{
	 .name = MT_SOC_CODEC_ANC_NAME,
	 .ops = &mtk_codec_dai_1_ops,
	 .playback = {
		      .stream_name = MT_SOC_ANC_STREAM_NAME,
		      .channels_min = 1,
		      .channels_max = 2,
		      .rates = SNDRV_PCM_RATE_8000_48000,
		      .formats = SND_SOC_ADV_MT_FMTS,
		     },
	}
};

uint32 GetDLNewIFFrequency(unsigned int frequency)
{
	uint32 Reg_value = 0;

	switch (frequency) {
	case 8000:
		Reg_value = 0;
		break;
	case 11025:
		Reg_value = 1;
		break;
	case 12000:
		Reg_value = 2;
		break;
	case 16000:
		Reg_value = 3;
		break;
	case 22050:
		Reg_value = 4;
		break;
	case 24000:
		Reg_value = 5;
		break;
	case 32000:
		Reg_value = 6;
		break;
	case 44100:
		Reg_value = 7;
		break;
	case 48000:
	case 96000:
	case 192000:
		Reg_value = 8;
		break;
	default:
		pr_warn("GetDLNewIFFrequency invalid freq %d\n", frequency);
		break;
	}
	return Reg_value;
}

static void TurnOnDacPower(void)
{
	pr_aud("%s(),\n", __func__);
	audckbufEnable(true);
	SwOperationEnable(true); /* SW operation enable */
	NvregEnable(true);	/* Enable AUDGLB */
	ClsqEnable(true);	/* Turn on 26MHz source clock */
	Topck_Enable(true);	/* Turn on AUDNCP_CLKDIV engine clock */

	usleep_range(250, 350);

	if (get_analog_input_status() == false)
		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x8020, 0xffff);
	else
		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x8000, 0xffff);

	/* set digital part */
	Ana_Set_Reg(AFE_NCP_CFG1, 0x1515, 0xff7f);
	/* NCP: ck1 and ck2 clock frequecy adjust configure */
	Ana_Set_Reg(AFE_NCP_CFG0, 0xC801, 0xfE01);
	/* NCP enable */

	usleep_range(250, 350);

	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0x009f);
	/* sdm audio fifo clock power on */
	Ana_Set_Reg(AFUNC_AUD_CON0, 0xC3A1, 0xf7ff);
	/* scrambler clock on enable */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0x009f);
	/* sdm power on */
	Ana_Set_Reg(AFUNC_AUD_CON2, 0x000B, 0x009f);
	/* sdm fifo enable */
	Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001D, 0x007f);
	/* set attenuation gain */
	Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xC001);
	/* afe enable */

	setDlMtkifSrc(true);
}

static void TurnOffDacPower(void)
{
	pr_aud("%s(),\n", __func__);

	setDlMtkifSrc(false);

	if (get_analog_input_status() == false)
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);	/* turn off afe */

	usleep_range(250, 500);

	Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0040, 0x0040);	/* down-link power down */
	Ana_Set_Reg(AFE_NCP_CFG0, 0x0000, 0x0001);		/* NCP disable */

#if 0
	Ana_Set_Reg(AUDNCP_CLKDIV_CON0, 0x0000, 0xffff);	/* Toggle RG_DIVCKS_CHG */
	Ana_Set_Reg(AUDNCP_CLKDIV_CON1, 0x0000, 0xffff);	/* Turn off DA_600K_NCP_VA18 */
	Ana_Set_Reg(AUDNCP_CLKDIV_CON3, 0x0001, 0xffff);	/* Disable NCP */
#endif
	/* upmu_set_rg_vio18_cal(0);// for MT6328 E1 VIO18 patch only */
	Topck_Enable(false);
	ClsqEnable(false);
	NvregEnable(false);
	SwOperationEnable(false); /* SW operation enable */
	audckbufEnable(false);
}

static void setDlMtkifSrc(bool enable)
{
	if (enable) {
		pr_warn("%s(), enable = %d, freq = %d\n", __func__, enable,
			mBlockSampleRate[AUDIO_DAI_DL1]);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0,
			    ((GetDLNewIFFrequency(mBlockSampleRate[AUDIO_DAI_DL1]) << 12) |
			     0x330), 0xfff0);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H,
			    ((GetDLNewIFFrequency(mBlockSampleRate[AUDIO_DAI_DL1]) << 12) |
			     0x300), 0xf300);

		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG2, 0x8000, 0x8000);
		/* pmic rxif sck inverse */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0x0001);
		/* turn on dl */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* set DL in normal path, not from sine gen table */
	} else {
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0000, 0x0001);
		/* bit0, Turn off down-link */
	}
}

static bool is_valid_hp_pga_idx(int reg_idx)
{
	return (reg_idx >= 0 && reg_idx <= 0x2c) || reg_idx == 0x3f;
}

static void HeadsetVoloumeRestore(void)
{
	int index = 0, oldindex = 0, offset = 0, count = 1, reg_idx = 0;

	/*pr_warn("%s\n", __func__);*/
	index = 12;
	oldindex = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	if (index > oldindex) {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = index - oldindex;
		while (offset > 0) {
			reg_idx = oldindex + count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
				Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
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
				Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
				Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
			}
			offset--;
			count++;
			udelay(100);
		}
	}
	Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0xc, 0x003f);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0xc, 0x003f);
}

static void HeadsetVoloumeSet(void)
{
	int index = 0, oldindex = 0, offset = 0, count = 1, reg_idx = 0;
	/* pr_warn("%s\n", __func__); */
	index = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];
	oldindex = 12;
	if (index > oldindex) {
		pr_aud("index = %d oldindex = %d\n", index, oldindex);
		offset = index - oldindex;
		while (offset > 0) {
			reg_idx = oldindex + count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
				Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
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
				Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
				Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
			}
			offset--;
			count++;
			udelay(200);
		}
	}
	Ana_Set_Reg(AFE_DL_NLE_L_CFG0, index, 0x003f);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG0, index, 0x003f);
}

static void headset_volume_ramp(int source, int target)
{
	int index = 0, oldindex = 0, offset = 0, count = 1, reg_idx = 0;
	/* pr_warn("%s\n", __func__); */
	if (!is_valid_hp_pga_idx(reg_idx) || !is_valid_hp_pga_idx(reg_idx))
		pr_warn("%s, volume index is not valid, source=%d, target=%d\n",
			__func__, source, target);

	oldindex = source == 63 ? 45 : source;
	index = target == 63 ? 45 : target;
	if (index > oldindex) {
		pr_aud("%s, index = %d oldindex = %d\n", __func__, index, oldindex);
		offset = index - oldindex;
		while (offset > 0) {
			reg_idx = oldindex + count;
			reg_idx = reg_idx == 45 ? 63 : reg_idx;
			Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
			Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
			offset--;
			count++;
			udelay(200);
		}
	} else {
		pr_aud("%s, index = %d oldindex = %d\n", __func__, index, oldindex);
		offset = oldindex - index;
		while (offset > 0) {
			reg_idx = oldindex - count;
			reg_idx = reg_idx == 45 ? 63 : reg_idx;
			Ana_Set_Reg(AFE_DL_NLE_L_CFG0, reg_idx, 0x003f);
			Ana_Set_Reg(AFE_DL_NLE_R_CFG0, reg_idx, 0x003f);
			offset--;
			count++;
			udelay(200);
		}
	}
}

static void hp_main_output_ramp(bool up)
{
	int i = 0, stage = 0;
	int target = 0;

	if (hp_differential_mode) {
		Ana_Set_Reg(AUDDEC_ANA_CON1, up << 8, 0x7 << 8);
		Ana_Set_Reg(AUDDEC_ANA_CON1, up << 11, 0x7 << 11);
		return;
	}

	/* Enable/Reduce HPP/N main output stage step by step */
	target = (low_power_mode == 1) ? 3 : 7;
	for (i = 0; i <= target; i++) {
		stage = up ? i : target - i;
		Ana_Set_Reg(AUDDEC_ANA_CON1, stage << 8, 0x7 << 8);
		usleep_range(100, 150);
	}
}

static void hp_aux_feedback_loop_gain_ramp(bool up)
{
	int i = 0, stage = 0;

	/* Reduce HP aux feedback loop gain step by step */
	for (i = 0; i < 8; i++) {
		stage = up ? i : 7 - i;
		Ana_Set_Reg(AUDDEC_ANA_CON9, stage << 12, 0x7 << 12);
		usleep_range(100, 150);
	}
}

static void hp_gain_ramp(bool up)
{
	int i = 0, stage = 0;

	/* Increase HPL gain to normal gain step by step */
	for (i = 0; i < 33; i++) {
		stage = up ? 0x2C - i : 0xc + i;
		Ana_Set_Reg(AFE_DL_NLE_L_CFG0, stage, 0x003f);
		Ana_Set_Reg(AFE_DL_NLE_R_CFG0, stage, 0x003f);
		usleep_range(100, 150);
	}
}

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

static void Audio_Amp_Change(int channels, bool enable, bool is_anc)
{
	pr_warn("%s(), enable %d, ANC %d, HSL %d, HSR %d, is_anc %d\n",
		__func__,
		enable,
		ANC_enabled,
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL],
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR],
		is_anc);

	if (enable) {
		if (GetDLStatus() == false && !ANC_enabled)
			TurnOnDacPower();

		if (GetDLStatus() == false && !is_anc) {
			/* reset sample rate */
			setDlMtkifSrc(false);
			setDlMtkifSrc(true);
		}

		/* here pmic analog control */
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false &&
		    !ANC_enabled) {
			pr_warn("%s\n", __func__);

			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0x8000);
			/* Pull-down HPL/R to AVSS30_AUD for de-pop noise */

			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0100, 0xff00);
			/* Set HP status as power-up */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
			/* Reduce ESD resistance of AU_REFN */
			Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0x002c, 0x003f);
			Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0x002c, 0x003f);
			/* Set HPL/HPR gain to -32db */

			usleep_range(100, 200);

			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0x002D);
			/* Enable cap-less LDOs (1.6V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
			/* Enable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
			/* Set NV regulator to -1.5V */

			usleep_range(100, 200);

			Hp_Zcd_Enable(false);
			/* Disable AUD_ZCD */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0xffff);
			/* Disable headphone short-ckt protection. */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
			if (low_power_mode == 0) {
				Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xff80);
			} else {
				Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4880, 0xff80);
				Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A00, 0xff80);
			}
			/* Enable IBIST */
			/* Set HP DR bias and HP & ZCD bias current optimization */
			if (hp_differential_mode)
				Ana_Set_Reg(AUDDEC_ANA_CON2, 0x001B, 0x003f);
			else
				Ana_Set_Reg(AUDDEC_ANA_CON2, 0x001A, 0x003f);
			/* Set HPP/N STB enhance circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0004, 0x000E);
			/* Set HP bias in HIFI mdoe */
			set_hp_trim_code(true, hpl_trim_code, hpr_trim_code);
			/* enable HP trim code */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x000C, 0x000C);
			/* Enable HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0030, 0x0030);
			/* Enable HP aux feedback loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0D00, 0xff00);
			/* Enable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x00C0, 0x00C0);
			/* Enable HP driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0030, 0x0030);
			/* Enable HP driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00C0, 0x00C0);
			/* Short HP main output to HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0F00, 0xff00);
			/* Enable HP main CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0001, 0x0001);
			/* Change compensation for HP main loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0300, 0xff00);
			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0003, 0x0003);
			/* Enable HP main output stage */

			hp_main_output_ramp(true);
			hp_aux_feedback_loop_gain_ramp(true);

			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0030);
			/* Disable HP aux feedback loop */
			usleep_range(100, 150);

			headset_volume_ramp(44, mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR]);

			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x000C);
			/* Disable HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x00C0);
			/* Unshort HP main output to HP aux output stage */
			usleep_range(100, 150);

			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
			/* Enable AUD_CLK  */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x000f, 0x000f);
			/* Enable Audio DAC  */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0xf300, 0xff00);
			/* Enable low-noise mode of DAC */
			usleep_range(100, 150);

			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0A00, 0x0f00);
			/* Switch HPL MUX and HPR MUX to audio DAC */
			usleep_range(100, 150);

			/* Apply digital DC compensation value to DAC */
			SetDcCompenSation(true);

			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x8000);
			/* No Pull-down HPL/R to AVSS30_AUD for de-pop noise */
			usleep_range(100, 150);
		}

	} else {
		if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false &&
		    mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false &&
		    !ANC_enabled) {
			pr_warn("Audio_Amp_Change off amp\n");
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0x8000);
			/* Pull-down HPL/R to AVSS30_AUD for de-pop noise */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0xf200, 0xff00);
			/* Set HP status as power-down */

			SetDcCompenSation(false);

			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
			/* HPR/HPL mux to open */
		}

		if (GetDLStatus() == false && !ANC_enabled) {
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x7200, 0xff00);
			/* Disable low-noise mode of DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x000f);
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
			/* Disable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x00C0, 0x00C0);
			/* Short HP main output to HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x000C, 0x000C);
			/* Enable HP aux output stage */

			headset_volume_ramp(mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR], 44);

			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0030, 0x0030);
			/* Enable HP aux feedback loop */

			hp_aux_feedback_loop_gain_ramp(false);
			hp_main_output_ramp(false);

			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0003);
			/* Disable HP main output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0e00, 0xff00);
			/* Enable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0x0001);
			/* Change compensation for HP aux loop */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0c00, 0xff00);
			/* Enable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x00C0);
			/* Unshort HP main output to HP aux output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0030);
			/* Disable HP driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x00C0);
			/* Disable HP driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xff00);
			/* Disable HP aux CMFB loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0030);
			/* Disable HP aux feedback loop */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x000C);
			/* Disable HP aux output stage */
			set_hp_trim_code(false, 0, 0);
			/* Disable HP trim code */
			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x8000);
			/* No Pull-down HPL/R to AVSS30_AUD for de-pop noise */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
			/* Restore NV regulator to -1.3V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
			/* Disable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0x002D);
			/* Disable cap-less LDOs (1.6V) */

			TurnOffDacPower();
		}
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

	pr_aud("%s() enable = %ld\n ", __func__, ucontrol->value.integer.value[0]);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] == false)) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, true, false);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_LEFT1, false, false);
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

	pr_aud("%s()\n", __func__);
	if ((ucontrol->value.integer.value[0] == true)
	    && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] == false)) {
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, true, false);
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
		    ucontrol->value.integer.value[0];
	} else if ((ucontrol->value.integer.value[0] == false)
		   && (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] ==
		       true)) {
		mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR] =
		    ucontrol->value.integer.value[0];
		Audio_Amp_Change(AUDIO_ANALOG_CHANNELS_RIGHT1, false, false);
	}
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static void Voice_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false) {
			TurnOnDacPower();
			pr_warn("Voice_Amp_Change on amp\n");

			Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
			/* Reduce ESD resistance of AU_REFN */

			udelay(100);

			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x8300, 0xff00);
			/* Enable low-noise mode of DAC  */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x032D, 0xff2D);
			/* Enable cap-less LDOs (1.6V) */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
			/* Enable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
			/* Set NV regulator to -1.5V */

			udelay(100);

			Ana_Set_Reg(ZCD_CON0, 0x0000, 0x0001);
			/* Disable AUD_ZCD */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0x0010);
			/* Disable headphone short-ckt protection.*/
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
			Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xff80);
			/* Enable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0080, 0x0080);
			/* Set HS STB enhance circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0002, 0x0002);
			/* Enable HS driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0001, 0x0001);
			/* Enable HS driver core circuits */
			Ana_Set_Reg(ZCD_CON3, 0x0009, 0x001f);
			/* Set HS gain to normal gain step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
			/* Enable AUD_CLK */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0x0009);
			/* Enable Audio DAC  */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0008, 0x000C);
			/* Switch HS MUX to audio DAC */
		}
	} else {
		pr_warn("Voice_Amp_Change off amp\n");
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x000C);
		/* Switch HS MUX to open */

		if (GetDLStatus() == false) {
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0009);
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
			/* Disable AUD_CLK */
			Ana_Set_Reg(ZCD_CON3, 0x0009, 0x001f);
			/* Set HS gain to normal gain step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x0001);
			/* Disable HS driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0x0002);
			/* Disable HS driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
			/* Restore NV regulator to -1.3V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
			/* Disable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0xff2D);
			/* Disable cap-less LDOs (1.6V) */
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

/* For MT6351 LO */
static void Speaker_Amp_Change(bool enable)
{
	if (enable) {
		if (GetDLStatus() == false)
			TurnOnDacPower();

		pr_warn("%s\n", __func__);
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
		/* Reduce ESD resistance of AU_REFN */

		udelay(100);

		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0x002D);
		/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
		/* Enable NV regulator */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
		/* Set NV regulator to -1.5V */

		udelay(100);

		Ana_Set_Reg(ZCD_CON0, 0x0000, 0x0001);
		/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0x0010);
		/* Disable headphone short-ckt protection.*/
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xff80);
		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0100, 0x0100);
		/* Set LO STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0002, 0x0002);
		/* Enable LO driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0001, 0x0001);
		/* Enable LO driver core circuits */
		Ana_Set_Reg(ZCD_CON1, 0x0009, 0x001f);
		/* Set LO gain to normal gain step by step */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0x0009);
		/* Enable Audio DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x8000, 0xff00);
		/* Enable low-noise mode of DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0008, 0x000C);
		/* Switch LOL MUX to audio DAC */

		/* Apply_Speaker_Gain(); */
	} else {
		/* pr_warn("turn off Speaker_Amp_Change\n"); */
#ifdef CONFIG_MTK_SPEAKER
		if (Speaker_mode == AUDIO_SPEAKER_MODE_D)
			Speaker_ClassD_close();
		else if (Speaker_mode == AUDIO_SPEAKER_MODE_AB)
			Speaker_ClassAB_close();
		else if (Speaker_mode == AUDIO_SPEAKER_MODE_RECEIVER)
			Speaker_ReveiverMode_close();
#endif
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x000C);
		/* Switch LOL MUX to open */

		if (GetDLStatus() == false) {
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0009);
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
			/* Disable AUD_CLK */
			Ana_Set_Reg(ZCD_CON3, 0x0009, 0x001f);
			/* Set HS gain to normal gain step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0001);
			/* Disable LO driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0002);
			/* Disable LO driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
			/* Restore NV regulator to -1.3V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
			/* Disable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0x002D);
			/* Disable cap-less LDOs (1.6V) */
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



#ifdef CONFIG_OF

#define GAP (2)			/* unit: us */
#if defined(CONFIG_MTK_LEGACY)
#define AW8736_MODE3 /*0.8w*/ \
	do { \
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ONE); \
		udelay(GAP); \
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ZERO); \
		udelay(GAP); \
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ONE); \
		udelay(GAP); \
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ZERO); \
		udelay(GAP); \
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ONE); \
	} while (0)
#endif

#define NULL_PIN_DEFINITION    (-1)
static void Ext_Speaker_Amp_Change(bool enable)
{
#define SPK_WARM_UP_TIME        (25)	/* unit is ms */
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (enable) {
		pr_debug("Ext_Speaker_Amp_Change ON+\n");
#ifndef CONFIG_MTK_SPEAKER

#ifndef MT6755_AW8736_REWORK
		AudDrv_GPIO_EXTAMP_Select(false, 3);
#else
		if (pin_extspkamp != 54)
			AudDrv_GPIO_EXTAMP_Select(false, 3);
		else
			AudDrv_GPIO_EXTAMP2_Select(false, 3);
#endif

		/*udelay(1000); */
		usleep_range(1 * 1000, 20 * 1000);
#if defined(CONFIG_MTK_LEGACY)
		mt_set_gpio_dir(pin_extspkamp, GPIO_DIR_OUT);	/* output */
		if (pin_extspkamp_2 != NULL_PIN_DEFINITION)
			mt_set_gpio_dir(pin_extspkamp_2, GPIO_DIR_OUT);	/* output */

#ifdef AW8736_MODE_CTRL
		AW8736_MODE3;
#else
		mt_set_gpio_out(pin_extspkamp, GPIO_OUT_ONE);	/* high enable */
#endif				/*AW8736_MODE_CTRL */
		if (pin_extspkamp_2 != NULL_PIN_DEFINITION)
			mt_set_gpio_out(pin_extspkamp_2, GPIO_OUT_ONE);	/* high enable */
#else
#ifndef MT6755_AW8736_REWORK
		AudDrv_GPIO_EXTAMP_Select(true, 3);
#else
		if (pin_extspkamp != 54)
			AudDrv_GPIO_EXTAMP_Select(true, 3);
		else
			AudDrv_GPIO_EXTAMP2_Select(true, 3);
#endif
#endif				/*CONFIG_MTK_LEGACY */
		msleep(SPK_WARM_UP_TIME);
#endif
		pr_debug("Ext_Speaker_Amp_Change ON-\n");
	} else {
		pr_debug("Ext_Speaker_Amp_Change OFF+\n");
#ifndef CONFIG_MTK_SPEAKER

#ifndef MT6755_AW8736_REWORK
		AudDrv_GPIO_EXTAMP_Select(false, 3);
#else
		if (pin_extspkamp != 54)
			AudDrv_GPIO_EXTAMP_Select(false, 3);
		else
			AudDrv_GPIO_EXTAMP2_Select(false, 3);
#endif
		udelay(500);
#endif
		pr_debug("Ext_Speaker_Amp_Change OFF-\n");
	}
#endif
}

#else				/*CONFIG_OF */
#ifndef CONFIG_MTK_SPEAKER
#ifdef AW8736_MODE_CTRL
/* 0.75us<TL<10us; 0.75us<TH<10us */
#define GAP (2)			/* unit: us */
/*1.2w*/
static void AW8736_MODE1(void)
{
	mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
}
/*1.0w*/
static void AW8736_MODE2(void)
{
	do {
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
	} while (0)
}
/*0.8w*/
static void AW8736_MODE3(void)
{
	do {
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
	} while (0)
}

/*it depends on THD, range: 1.5 ~ 2.0w*/
static void AW8736_MODE4(void)
{
	do {
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);
		udelay(GAP);
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);
	} while (0)
}
#endif

static void Ext_Speaker_Amp_Change(bool enable)
{
#define SPK_WARM_UP_TIME        (25)	/* unit is ms */
#ifndef CONFIG_FPGA_EARLY_PORTING

	if (enable) {
		pr_debug("Ext_Speaker_Amp_Change ON+\n");
#ifndef CONFIG_MTK_SPEAKER
		pr_warn("Ext_Speaker_Amp_Change ON set GPIO\n");
		mt_set_gpio_mode(GPIO_EXT_SPKAMP_EN_PIN, GPIO_MODE_00);	/* GPIO117: DPI_D3, mode 0 */
		mt_set_gpio_pull_enable(GPIO_EXT_SPKAMP_EN_PIN, GPIO_PULL_ENABLE);
		mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT);	/* output */
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);	/* low disable */
		udelay(1000);
		mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT);	/* output */

#ifdef AW8736_MODE_CTRL
		AW8736_MODE3;
#else
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ONE);	/* high enable */
#endif

		msleep(SPK_WARM_UP_TIME);
#endif
		pr_debug("Ext_Speaker_Amp_Change ON-\n");
	} else {
		pr_debug("Ext_Speaker_Amp_Change OFF+\n");
#ifndef CONFIG_MTK_SPEAKER
		/* mt_set_gpio_mode(GPIO_EXT_SPKAMP_EN_PIN, GPIO_MODE_00); //GPIO117: DPI_D3, mode 0 */
		mt_set_gpio_dir(GPIO_EXT_SPKAMP_EN_PIN, GPIO_DIR_OUT);	/* output */
		mt_set_gpio_out(GPIO_EXT_SPKAMP_EN_PIN, GPIO_OUT_ZERO);	/* low disbale */
		udelay(500);
#endif
		pr_debug("Ext_Speaker_Amp_Change OFF-\n");
	}
#endif
}
#endif
#endif

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
			TurnOnDacPower();

		pr_warn("%s\n", __func__);
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0x8000);
		/* Pull-down HPL/R to AVSS30_AUD for de-pop noise */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x4000, 0x4000);
		/* Reduce ESD resistance of AU_REFN */

		udelay(100);

		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0x002D);
		/* Enable cap-less LDOs (1.6V) */
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0x0003);
		/* Enable NV regulator */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0x0780);
		/* Set NV regulator to -1.5V */

		udelay(100);

		Hp_Zcd_Enable(false);
		/* Disable AUD_ZCD */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0010, 0x0010);
		/* Disable headphone short-ckt protection.*/
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xff80);
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xff80);
		/* Enable IBIST */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0100, 0x0100);
		/* Set LO STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0002, 0x0002);
		/* Enable LO driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0001, 0x0001);
		/* Enable LO driver core circuits */
		Ana_Set_Reg(ZCD_CON1, 0x0009, 0x001f);
		/* Set LO gain to normal gain step by step */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0001, 0x0001);
		/* Enable AUD_CLK */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0x0009);
		/* Enable Audio DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x8000, 0xff00);
		/* Enable low-noise mode of DAC  */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0008, 0x000C);
		/* Switch LOL MUX to audio DAC */

		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0100, 0xff00);
		/* Set HP status as power-up */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x3000, 0x3000);
		/* Disable headphone short-ckt protection. */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x001A, 0x003f);
		/* Set HPP/N STB enhance circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0x8000);
		/* No Pull-down HPL/R to AVSS30_AUD */
		Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0004, 0x000E);
		/* Set HP bias in HIFI mdoe */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x00C0, 0x00C0);
		/* Enable HP driver bias circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0030, 0x0030);
		/* Enable HP driver core circuits */
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0300, 0xff00);
		/* Enable HP main CMFB loop */
		Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0001, 0x0001);
		/* Change compensation for HP main loop */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0003, 0x0003);
		/* Enable HP main output stage */
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0700, 0x3f00);
		/* Enable HPP/N main output stage step by step */
		Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0x0006, 0x007f);
		Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0x0006, 0x007f);
		/* Increase HPL and HPR gain to normal gain step by step */

		/* Apply digital DC compensation value to DAC */
		setHpGainZero();
		/* SetDcCompenSation(); */
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0500, 0x0f00);
		/* Switch HP MUX to audio DAC */

		/* apply volume setting */
		HeadsetVoloumeSet();
		Apply_Speaker_Gain();
	} else {
		HeadsetVoloumeRestore();
		/* Set HPR/HPL gain as 0dB, step by step */
		setHpGainZero();

		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0f00);
		/* HPR/HPL mux to open */
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x000C);
		/* Switch LOL MUX to open */

		if (GetDLStatus() == false) {
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x0009);
			/* Disable Audio DAC */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0001);
			/* Disable AUD_CLK */

			Ana_Set_Reg(AFE_DL_NLE_L_CFG0, 0x002C, 0x007f);
			Ana_Set_Reg(AFE_DL_NLE_R_CFG0, 0x002C, 0x007f);
			/* Increase HPL and HPR gain to normal gain step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x3f00);
			/* Enable HPP/N main output stage step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0x0003);
			/* Disable HP main output stage */
			Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0x0001);
			/* Change compensation for HP aux loop */
			Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0x00f0);
			/* Disable HPR/HPL */
			Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xff00);
			/* Disable HP aux CMFB loop */

			Ana_Set_Reg(ZCD_CON3, 0x0009, 0x001f);
			/* Set LOL gain to normal gain step by step */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0001);
			/* Disable LO driver core circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0x0002);
			/* Disable LO driver bias circuits */
			Ana_Set_Reg(AUDDEC_ANA_CON11, 0x8000, 0x8000);
			/* Disable IBIST */
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0780);
			/* Restore NV regulator to -1.3V */
			Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0x0003);
			/* Disable NV regulator */
			Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0x002D);
			/* Disable cap-less LDOs (1.6V) */
			TurnOffDacPower();
		}
		/* enable_dc_compensation(false); */
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

#ifdef CONFIG_MTK_SPEAKER
static const char *const speaker_amp_function[] = { "CALSSD", "CLASSAB", "RECEIVER" };

static const char *const speaker_PGA_function[] = {
	"4Db", "5Db", "6Db", "7Db", "8Db", "9Db", "10Db",
	"11Db", "12Db", "13Db", "14Db", "15Db", "16Db", "17Db"
};

static const char *const speaker_OC_function[] = { "Off", "On" };
static const char *const speaker_CS_function[] = { "Off", "On" };
static const char *const speaker_CSPeakDetecReset_function[] = { "Off", "On" };

static int Audio_Speaker_Class_Set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&Ana_Ctrl_Mutex);
	Speaker_mode = ucontrol->value.integer.value[0];
	mutex_unlock(&Ana_Ctrl_Mutex);
	return 0;
}

static int Audio_Speaker_Class_Get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = Speaker_mode;
	return 0;
}

static int Audio_Speaker_Pga_Gain_Set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	Speaker_pga_gain = ucontrol->value.integer.value[0];

	pr_warn("%s Speaker_pga_gain= %d\n", __func__, Speaker_pga_gain);
	Ana_Set_Reg(SPK_ANA_CON0, Speaker_pga_gain << 11, 0x7800);
	return 0;
}

static int Audio_Speaker_OcFlag_Get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	mSpeaker_Ocflag = GetSpeakerOcFlag();
	ucontrol->value.integer.value[0] = mSpeaker_Ocflag;
	return 0;
}

static int Audio_Speaker_OcFlag_Set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s is not support setting\n", __func__);
	return 0;
}

static int Audio_Speaker_Pga_Gain_Get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = Speaker_pga_gain;
	return 0;
}

static int Audio_Speaker_Current_Sensing_Set(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.integer.value[0])
		Ana_Set_Reg(SPK_CON12, 0x9300, 0xff00);
	else
		Ana_Set_Reg(SPK_CON12, 0x1300, 0xff00);
	return 0;
}

static int Audio_Speaker_Current_Sensing_Get(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (Ana_Get_Reg(SPK_CON12) >> 15) & 0x01;
	return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Set(struct snd_kcontrol *kcontrol,
							   struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.integer.value[0])
		Ana_Set_Reg(SPK_CON12, 1 << 14, 1 << 14);
	else
		Ana_Set_Reg(SPK_CON12, 0, 1 << 14);
	return 0;
}

static int Audio_Speaker_Current_Sensing_Peak_Detector_Get(struct snd_kcontrol *kcontrol,
							   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (Ana_Get_Reg(SPK_CON12) >> 14) & 0x01;
	return 0;
}


static const struct soc_enum Audio_Speaker_Enum[] = {
	/* speaker class setting */
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_amp_function),
			    speaker_amp_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_PGA_function),
			    speaker_PGA_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_OC_function),
			    speaker_OC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CS_function),
			    speaker_CS_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(speaker_CSPeakDetecReset_function),
			    speaker_CSPeakDetecReset_function),
};

static const struct snd_kcontrol_new mt6331_snd_Speaker_controls[] = {
	SOC_ENUM_EXT("Audio_Speaker_class_Switch", Audio_Speaker_Enum[0],
		     Audio_Speaker_Class_Get,
		     Audio_Speaker_Class_Set),
	SOC_ENUM_EXT("Audio_Speaker_PGA_gain", Audio_Speaker_Enum[1],
		     Audio_Speaker_Pga_Gain_Get,
		     Audio_Speaker_Pga_Gain_Set),
	SOC_ENUM_EXT("Audio_Speaker_OC_Falg", Audio_Speaker_Enum[2],
		     Audio_Speaker_OcFlag_Get,
		     Audio_Speaker_OcFlag_Set),
	SOC_ENUM_EXT("Audio_Speaker_CurrentSensing", Audio_Speaker_Enum[3],
		     Audio_Speaker_Current_Sensing_Get,
		     Audio_Speaker_Current_Sensing_Set),
	SOC_ENUM_EXT("Audio_Speaker_CurrentPeakDetector",
		     Audio_Speaker_Enum[4],
		     Audio_Speaker_Current_Sensing_Peak_Detector_Get,
		     Audio_Speaker_Current_Sensing_Peak_Detector_Set),
};

int Audio_AuxAdcData_Get_ext(void)
{
#ifdef _GIT318_PMIC_READY
	int dRetValue = PMIC_IMM_GetOneChannelValue(AUX_ICLASSAB_AP, 1, 0);

	pr_warn("%s dRetValue 0x%x\n", __func__, dRetValue);
	return dRetValue;
#else
return 0;
#endif
}


#endif

static int Audio_AuxAdcData_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{

#ifdef CONFIG_MTK_SPEAKER
	ucontrol->value.integer.value[0] = Audio_AuxAdcData_Get_ext();
	/* PMIC_IMM_GetSPK_THR_IOneChannelValue(0x001B, 1, 0); */
#else
	ucontrol->value.integer.value[0] = 0;
#endif
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
static const char *const audio_power_mode[] = { "Hifi", "Low_Power" };

/* static const char *DAC_SampleRate_function[] = {"8000", "11025", "16000", "24000", "32000", "44100", "48000"}; */
static const char *const DAC_DL_PGA_Headset_GAIN[] = {
	"12Db", "11Db", "10Db", "9Db", "8Db", "7Db", "6Db", "5Db", "4Db",
	"3Db", "2Db", "1Db", "0Db", "-1Db", "-2Db", "-3Db", "-4Db", "-5Db",
	"-6Db", "-7Db", "-8Db", "-9Db", "-10Db", "-11Db", "-12Db", "-13Db",
	"-14Db", "-15Db", "-16Db", "-17Db", "-18Db", "-19Db", "-20Db", "-21Db",
	"-22Db", "-23Db", "-24Db", "-25Db", "-26Db", "-27Db", "-28Db", "-29Db",
	"-30Db", "-31Db", "-32Db", "-40Db"
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

	Ana_Set_Reg(ZCD_CON1, index << 7, 0x0f80);
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
	int oldindex = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
		index = 0x3f;

	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = index;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = index;

	if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETL]) {
		headset_volume_ramp(oldindex, index);
		SetDcCompenSation(true);
	}
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
	int oldindex = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR];

	if (index >= ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN) - 1))
		index = 0x3f;

	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = index;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = index;

	if (mCodec_data->mAudio_Ana_DevicePower[AUDIO_ANALOG_DEVICE_OUT_HEADSETR]) {
		headset_volume_ramp(oldindex, index);
		SetDcCompenSation(true);
	}
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

void enable_anc_loopback3(void)
{
	Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x1 << 12, 0x1 << 12);
}

void disable_anc_loopback3(void)
{
	Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0 << 12, 0x1 << 12);
}

void pmic_select_dl_only(void)
{
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 4,
			0x1 << 4); /* dac_hp_anc_ch1_sel(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 5,
			0x1 << 5); /* dac_hp_anc_ch2_7el(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 6,
			0x1 << 6); /* hp_anc_fpga_de bug_ch1(0: add dl, 1: pure anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 7,
			0x1 << 7); /* hp_anc_fpga_de bug_ch2(0: add dl, 1: pure anc) */
}

void pmic_select_dl_and_anc(void)
{
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 4,
			0x1 << 4); /* dac_hp_anc_ch1_sel(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 5,
			0x1 << 5); /* dac_hp_anc_ch2_7el(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 6,
			0x1 << 6); /* hp_anc_fpga_de bug_ch1(0: add dl, 1: pure anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 7,
			0x1 << 7); /* hp_anc_fpga_de bug_ch2(0: add dl, 1: pure anc) */
}

void pmic_select_anc_only(void)
{
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 4,
			0x1 << 4); /* dac_hp_anc_ch1_sel(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 5,
			0x1 << 5); /* dac_hp_anc_ch2_7el(0: dl, 1: anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 6,
			0x1 << 6); /* hp_anc_fpga_de bug_ch1(0: add dl, 1: pure anc) */
	Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 7,
			0x1 << 7); /* hp_anc_fpga_de bug_ch2(0: add dl, 1: pure anc) */
}
static int Set_ANC_Enable(bool enable)
{
	pr_warn("%s(), value = %d\n", __func__, enable);
	if (enable == true) {
		pmic_select_dl_only();

		Audio_Amp_Change(0, true, true);
		ANC_enabled = true;

		/* mtkif sclk invert */
		Ana_Set_Reg(AFE_ADDA2_PMIC_NEWIF_CFG2, 0x1 << 15, 0x1 << 15);

		/* reset timing workaround */
		Ana_Set_Reg(AFE_ADDA2_PMIC_NEWIF_CFG1, 0x0 << 13, 0x1 << 13); /* pmic rx */
		msleep(20);
		Ana_Set_Reg(AFE_HPANC_CFG0, 0x1 << 2, 0x1 << 2); /* toggle cic */
		Ana_Set_Reg(AFE_HPANC_CFG0, 0x0 << 2, 0x1 << 2);
		Ana_Set_Reg(AFE_ADDA2_PMIC_NEWIF_CFG1, 0x1 << 13, 0x1 << 13); /* pmic rx */
		msleep(20);
		pmic_select_dl_and_anc();
	} else {
		pmic_select_dl_only();
		ANC_enabled = false;
		Audio_Amp_Change(0, false, true);
	}
	return 0;
}

static int Audio_ANC_Get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s(), value = %d\n", __func__, ANC_enabled);
	ucontrol->value.integer.value[0] = ANC_enabled;
	return 0;
}

static int Audio_ANC_Set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s(), value = %ld\n", __func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0])
		Set_ANC_Enable(true);
	else
		Set_ANC_Enable(false);
	return 0;
}

static int audio_power_mode_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = low_power_mode;
	return 0;
}

static int audio_power_mode_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_power_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	low_power_mode = ucontrol->value.integer.value[0];
	pr_debug("%s() audio_power_mode = %d\n", __func__, low_power_mode);
	return 0;
}

static int audio_hp_differential_mode_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = hp_differential_mode;
	return 0;
}

static int audio_hp_differential_mode_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(amp_function)) {
		pr_err("%s(), return -EINVAL\n", __func__);
		return -EINVAL;
	}
	hp_differential_mode = ucontrol->value.integer.value[0];
	pr_debug("%s(), hp_differential_mode = %d\n", __func__, hp_differential_mode);

	return 0;
}

static int audio_ul_rate_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), codec uplink1 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_UL1]);
	ucontrol->value.integer.value[0] = mBlockSampleRate[AUDIO_DAI_UL1];
	return 0;
}

static int audio_ul_rate_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mBlockSampleRate[AUDIO_DAI_UL1] = ucontrol->value.integer.value[0];
	pr_warn("%s(), codec uplink1 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_UL1]);
	return 0;
}

static int audio_ul2_rate_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), codec uplink2 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_UL2]);
	ucontrol->value.integer.value[0] = mBlockSampleRate[AUDIO_DAI_UL2];
	return 0;
}

static int audio_ul2_rate_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mBlockSampleRate[AUDIO_DAI_UL2] = ucontrol->value.integer.value[0];
	pr_warn("%s(), codec uplink2 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_UL2]);
	return 0;
}

static int audio_dl_rate_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), codec downlink1 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_DL1]);
	ucontrol->value.integer.value[0] = mBlockSampleRate[AUDIO_DAI_DL1];
	return 0;
}

static int audio_dl_rate_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	mBlockSampleRate[AUDIO_DAI_DL1] = ucontrol->value.integer.value[0];
	pr_warn("%s(), codec downlink1 samplerate = %d\n", __func__, mBlockSampleRate[AUDIO_DAI_DL1]);
	return 0;
}

static const struct soc_enum Audio_DL_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(amp_function), amp_function),
	/* here comes pga gain setting */
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Headset_GAIN),
			    DAC_DL_PGA_Headset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Handset_GAIN),
			    DAC_DL_PGA_Handset_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_PGA_Speaker_GAIN),
			    DAC_DL_PGA_Speaker_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aud_clk_buf_function),
			    aud_clk_buf_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_power_mode), audio_power_mode),
};

static const struct snd_kcontrol_new mt6331_snd_controls[] = {
	SOC_ENUM_EXT("Audio_Amp_R_Switch", Audio_DL_Enum[0], Audio_AmpR_Get,
		     Audio_AmpR_Set),
	SOC_ENUM_EXT("Audio_Amp_L_Switch", Audio_DL_Enum[0], Audio_AmpL_Get,
		     Audio_AmpL_Set),
	SOC_ENUM_EXT("Voice_Amp_Switch", Audio_DL_Enum[0], Voice_Amp_Get,
		     Voice_Amp_Set),
	SOC_ENUM_EXT("Speaker_Amp_Switch", Audio_DL_Enum[0],
		     Speaker_Amp_Get, Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_Speaker_Amp_Switch", Audio_DL_Enum[0],
		     Headset_Speaker_Amp_Get,
		     Headset_Speaker_Amp_Set),
	SOC_ENUM_EXT("Headset_PGAL_GAIN", Audio_DL_Enum[1],
		     Headset_PGAL_Get, Headset_PGAL_Set),
	SOC_ENUM_EXT("Headset_PGAR_GAIN", Audio_DL_Enum[1],
		     Headset_PGAR_Get, Headset_PGAR_Set),
	SOC_ENUM_EXT("Handset_PGA_GAIN", Audio_DL_Enum[2], Handset_PGA_Get,
		     Handset_PGA_Set),
	SOC_ENUM_EXT("Lineout_PGAR_GAIN", Audio_DL_Enum[3],
		     Lineout_PGAR_Get, Lineout_PGAR_Set),
	SOC_ENUM_EXT("Lineout_PGAL_GAIN", Audio_DL_Enum[3],
		     Lineout_PGAL_Get, Lineout_PGAL_Set),
	SOC_ENUM_EXT("AUD_CLK_BUF_Switch", Audio_DL_Enum[4],
		     Aud_Clk_Buf_Get, Aud_Clk_Buf_Set),
	SOC_ENUM_EXT("Ext_Speaker_Amp_Switch", Audio_DL_Enum[0],
		     Ext_Speaker_Amp_Get,
		     Ext_Speaker_Amp_Set),
	SOC_ENUM_EXT("Receiver_Speaker_Switch", Audio_DL_Enum[0],
		     Receiver_Speaker_Switch_Get,
		     Receiver_Speaker_Switch_Set),
	SOC_ENUM_EXT("Audio_ANC_Switch", Audio_DL_Enum[0], Audio_ANC_Get, Audio_ANC_Set),
	SOC_ENUM_EXT("Audio_Power_Mode", Audio_DL_Enum[5], audio_power_mode_get, audio_power_mode_set),
	SOC_ENUM_EXT("Audio_Hp_Differential_Mode", Audio_DL_Enum[0],
		     audio_hp_differential_mode_get, audio_hp_differential_mode_set),
	SOC_SINGLE_EXT("Codec_ADC_SampleRate", SND_SOC_NOPM, 0, 0x80000, 0, audio_ul_rate_get,
		       audio_ul_rate_set),
	SOC_SINGLE_EXT("Codec_ADC2_SampleRate", SND_SOC_NOPM, 0, 0x80000, 0, audio_ul2_rate_get,
		       audio_ul2_rate_set),
	SOC_SINGLE_EXT("Codec_DAC_SampleRate", SND_SOC_NOPM, 0, 0x80000, 0, audio_dl_rate_get,
		       audio_dl_rate_set),
};

static const struct snd_kcontrol_new mt6331_Voice_Switch[] = {
	/* SOC_DAPM_ENUM_EXT("Voice Mux", Audio_DL_Enum[10], Voice_Mux_Get, Voice_Mux_Set), */
};

static bool get_analog_input_status(void)
{
	int i = 0;

	for (i = AUDIO_ANALOG_DEVICE_IN_ADC1; i <= AUDIO_ANALOG_DEVICE_IN_DMIC2; i++) {
		if ((mCodec_data->mAudio_Ana_DevicePower[i] > 0)
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

static bool get_adc_clock_status(void)
{
	int i = 0;

	for (i = AUDIO_ANALOG_DEVICE_IN_ADC1; i <= AUDIO_ANALOG_DEVICE_IN_ADC4; i++) {
		if (mCodec_data->mAudio_Ana_DevicePower[i] > 0)
			return true;
	}
	return false;
}

static bool is_amic(enum audio_analog_device_type device_in)
{
	return (device_in >= AUDIO_ANALOG_DEVICE_IN_ADC1
		&& device_in <= AUDIO_ANALOG_DEVICE_IN_ADC4);
}

static bool is_dmic(enum audio_analog_device_type device_in)
{
	return (device_in >= AUDIO_ANALOG_DEVICE_IN_DMIC0
		&& device_in <= AUDIO_ANALOG_DEVICE_IN_DMIC2);
}

static unsigned int amic_array_status(void)
{
	int mask = 0x3;
	int mux0 = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX] & mask;
	int mux1 = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX] & mask;
	int mux2 = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX] & mask;
	int mux3 = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX] & mask;
	/* Mux (0~3) => amic array value range 0~3 */
	return mux0 | (mux1 << 2) | (mux2 << 4) | (mux3 << 6);
}

static unsigned int dmic_array_status(void)
{
	int mask = 0x7;
	int offset = AUDIO_UL_ARRAY_DMIC0_LCH;
	int mux0 = (mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX] - offset) & mask;
	int mux1 = (mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX] - offset) & mask;
	int mux2 = (mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX] - offset) & mask;
	int mux3 = (mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX] - offset) & mask;

	/* Mux(4~8) => dmic array value range 0~4 */
	return mux0 | (mux1 << 4) | (mux2 << 8) | (mux3 << 12);
}

static bool TurnOnADcPowerACC(enum audio_analog_device_type device_in, bool enable)
{
	pr_warn("%s device_in = %d enable = %d\n", __func__, device_in, enable);

	if (enable) {
		if (get_analog_input_status() == false) {
			audckbufEnable(true);
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0060);
			/* Globe bias VOW LPW mode (Default off) */
			NvregEnable(true);
			/* Enable audio globe bias */

			/* this part should be checked, if need open */
			/* Ana_Set_Reg(TOP_CLKSQ_SET, 0x0800, 0x0800); */
			/* RG_CLKSQ_IN_SEL_VA18_SWCTRL: 0_HW, 1_SW */
			/* Ana_Set_Reg(TOP_CLKSQ_CLR, 0x0400, 0x0400); */
			/* RG_CLKSQ_IN_SEL_VA18: 0_PAD, 1_DCXO */

			ClsqEnable(true); /* Enable CLKSQ 26MHz */
			Topck_Enable(true); /* Turn on AUDNCP_CLKDIV engine clock */
		}

		if (get_adc_clock_status() == false)
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0040, 0x007C);
			/* Enable Audio ADC clock gen and ADC CLK from CLKGEN(13MHz) */

		/* PMIC analog uplink setting */
		switch (device_in) {
		case AUDIO_ANALOG_DEVICE_IN_ADC1:
			audio_preamp1_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC2:
			audio_preamp2_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC3:
			audio_preamp3_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC4:
			audio_preamp4_en(true);
			break;
		default:
			pr_warn("%s unsupport adc type = %d", __func__, device_in);
		}

		/* digital part setting */
		if (get_analog_input_status() == false) {
			if (GetDacStatus() == false) {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0040, 0xffff);
				/* power on clock */
			} else {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);
				/* power on clock */
			}
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);
			/* [0] afe enable */
		}

		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x00ff);
		/* AMIC array setting */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* ADDA loopback/sgen setting : off all */

	} else {
		/* PMIC analog uplink setting */
		switch (device_in) {
		case AUDIO_ANALOG_DEVICE_IN_ADC1:
			audio_preamp1_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC2:
			audio_preamp2_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC3:
			audio_preamp3_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC4:
			audio_preamp4_en(false);
			break;
		default:
			pr_warn("%s unsupport adc type = %d", __func__, device_in);
		}

		if (get_adc_clock_status() == false)
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0000, 0x007C);
			/* disable Audio ADC clock gen and ADC CLK from CLKGEN(13MHz) */

		if (get_analog_input_status() == false) {
			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);
			/* up-link power down */
			if (GetDLStatus() == false) {
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe disable */
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
				/* afe power down and total audio clk disable */
			}
			Topck_Enable(false);
			ClsqEnable(false);
			NvregEnable(false);
			audckbufEnable(false);
		}
	}
	return true;
}

static bool audio_dmic_enable(bool enable, enum audio_analog_device_type device_in)
{
	pr_warn("%s DeviceType = %d enable = %d\n", __func__, device_in, enable);
	if (enable) {
		if (get_analog_input_status() == false) {
			audckbufEnable(true);
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0060);
			/* Globe bias VOW LPW mode (Default off) */
			NvregEnable(true); /* Enable audio globe bias */

			/* this part should be checked, if need open */
			/* Ana_Set_Reg(TOP_CLKSQ_SET, 0x0800, 0x0800); */
			/* RG_CLKSQ_IN_SEL_VA18_SWCTRL: 0_HW, 1_SW */
			/* Ana_Set_Reg(TOP_CLKSQ_CLR, 0x0400, 0x0400); */
			/* RG_CLKSQ_IN_SEL_VA18: 0_PAD, 1_DCXO */

			ClsqEnable(true); /* Enable CLKSQ 26MHz */
			Topck_Enable(true); /* Turn on AUDNCP_CLKDIV engine clock */
		}

		audio_dmic_input_enable(true, device_in); /* enable dmic power */

		/* digital part setting */
		if (get_analog_input_status() == false) {
			if (GetDacStatus() == false) {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0040, 0xffff);
				/* power on clock : except Dac clock*/
			} else {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);
				/* power on clock */
			}
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);
			/* [0] afe enable */
		}

		Ana_Set_Reg(AFE_DMIC_ARRAY_CFG, dmic_array_status(), 0x7777);
		/* dmic adc  array */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* ADDA loopback/sgen setting : off all */
	} else {
		if (get_analog_input_status() == false)
			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);
			/* up-link power down  */

		audio_dmic_input_enable(false, device_in); /* disable dmic power */

		if (get_analog_input_status() == false) {
			if (GetDLStatus() == false) {
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe disable */
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
				/* afe power down and total audio clk disable */
			}

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

static bool TurnOnADcPowerDCC(enum audio_analog_device_type device_in, bool enable, int ECMmode)
{
	pr_aud("%s(), enable %d, device_in %d\n", __func__, enable, device_in);

	if (enable) {
		if (get_analog_input_status() == false) {
			audckbufEnable(true);
			Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0060);
			/* Globe bias VOW LPW mode (Default off) */
			NvregEnable(true); /* Enable audio globe bias */

			/* this part should be checked, if need open */
			/* Ana_Set_Reg(TOP_CLKSQ_SET, 0x0800, 0x0800); */
			/* RG_CLKSQ_IN_SEL_VA18_SWCTRL: 0_HW, 1_SW */
			/* Ana_Set_Reg(TOP_CLKSQ_CLR, 0x0400, 0x0400); */
			/* RG_CLKSQ_IN_SEL_VA18: 0_PAD, 1_DCXO */

			ClsqEnable(true); /* Enable CLKSQ 26MHz */
			Topck_Enable(true); /* Turn on AUDNCP_CLKDIV engine clock */
		}

		if (get_adc_clock_status() == false) {
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0040, 0x007C);
			/* Enable Audio ADC clock gen and ADC CLK from CLKGEN(13MHz) */

			/* DCC 50k CLK (from 26M) */
			Ana_Set_Reg(TOP_CLKSQ_SET, 0x0003, 0xffff);
			/* TOP CLKSQ_EN */
			Ana_Set_Reg(TOP_CKPDN_CON0, 0x0040, 0x1840);
			/* AUD_CK power on */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0001, 0x0001);
			/* dcclk_gen_on=1'b1 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2060, 0xffEE);
			/* dcclk_div=11'b00100000011, dcclk_ref_ck_sel=2'b00 dcclk_pdn=1'b0 */
			Ana_Set_Reg(AFE_DCCLK_CFG1, 0x0100, 0x0100);
			/* bypass resync function */
			Ana_Set_Reg(AFE_DCCLK_CFG2, 0x0001, 0x0001);
			/* dcclk_gen_on=1'b1 */
			Ana_Set_Reg(AFE_DCCLK_CFG2, 0x2060, 0xffEE);
			/* dcclk_div=11'b00100000011, dcclk_ref_ck_sel=2'b00 dcclk_pdn=1'b0 */
			Ana_Set_Reg(AFE_DCCLK_CFG3, 0x0100, 0x0100);
			/* bypass resync function */
		}

		/* PMIC analog uplink setting */
		switch (device_in) {
		case AUDIO_ANALOG_DEVICE_IN_ADC1:
			audio_preamp1_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC2:
			audio_preamp2_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC3:
			audio_preamp3_en(true);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC4:
			audio_preamp4_en(true);
			break;
		default:
			pr_warn("%s unsupport adc type = %d", __func__, device_in);
		}

		/* digital part setting */
		if (get_analog_input_status() == false) {
			if (GetDacStatus() == false) {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0040, 0xffff);
				/* power on clock */
			} else {
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0000, 0xffff);
				/* power on clock */
			}
			Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0x0001);
			/* [0] afe enable */
		}

		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x00ff);
		/* AMIC array setting */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* ADDA loopback/sgen setting : off all */

	} else {
		/* PMIC analog uplink setting */
		switch (device_in) {
		case AUDIO_ANALOG_DEVICE_IN_ADC1:
			audio_preamp1_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC2:
			audio_preamp2_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC3:
			audio_preamp3_en(false);
			break;
		case AUDIO_ANALOG_DEVICE_IN_ADC4:
			audio_preamp4_en(false);
			break;
		default:
			pr_warn("%s unsupport adc type = %d", __func__, device_in);
		}

		if (get_adc_clock_status() == false) {
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0000, 0x0001);
			/* dcclk_gen_on=1'b0 */
			Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0fe2, 0xffEE);
			/* Default Value, dcclk_pdn=1'b0 */

			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0000, 0x000C);
			/* ADC CLK from CLKGEN (13MHz) */
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0000, 0x0070);
			/* disable CLKGEN */
		}

		if (get_analog_input_status() == false) {
			Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x0020, 0x0020);
			/* up-link power down */

			if (GetDLStatus() == false) {
				Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);
				/* afe disable */
				Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x00C4, 0x00C4);
				/* afe power down and total audio clk disable */
			}

			Topck_Enable(false);
			ClsqEnable(false);
			NvregEnable(false);
			audckbufEnable(false);
		}
	}
	return true;
}


static bool TurnOnVOWDigitalHW(bool enable)
{
#ifdef CONFIG_MTK_VOW_SUPPORT
	pr_debug("%s enable = %d\n", __func__, enable);
	if (enable) {
		/*move to vow driver*/
#ifdef VOW_STANDALONE_CONTROL
		if (mAudio_VOW_Mic_type == AUDIO_VOW_MIC_TYPE_Handset_DMIC)
			Ana_Set_Reg(AFE_VOW_TOP, 0x6850, 0xffff);   /*VOW enable*/
		else
			Ana_Set_Reg(AFE_VOW_TOP, 0x4810, 0xffff);   /*VOW enable*/
#endif
	} else {
		/*disable VOW interrupt here*/
		/*Ana_Set_Reg(INT_CON0, 0x0015, 0x0800); //disable VOW interrupt. BIT11*/
#ifdef VOW_STANDALONE_CONTROL
		/*move to vow driver*/

		Ana_Set_Reg(AFE_VOW_TOP, 0x4010, 0xffff);   /*VOW disable*/
		Ana_Set_Reg(AFE_VOW_TOP, 0xC010, 0xffff);   /*VOW clock power down*/
#endif
	}
#endif
	return true;
}

#ifdef CONFIG_MTK_VOW_SUPPORT
static void TurnOnVOWPeriodicOnOff(int MicType, int On_period, int enable)
{
	int i = 0;
	const uint16 (*pBuf)[22];

	/* give a default value */
	pBuf = Handset_AMIC_DCC_PeriodicOnOff;

	if ((MicType == AUDIO_VOW_MIC_TYPE_Headset_MIC)
	 || (MicType == AUDIO_VOW_MIC_TYPE_Handset_AMIC)
	 || (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC)
	 || (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)
	 || (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01)
	 || (MicType >= AUDIO_VOW_MIC_TYPE_NUM)
	 || (MicType < 0)) {
		pr_debug("MicType:%d, No support periodic On/Off\n", MicType);
		return;
	}

	if (enable == 0) {
		VOW32KCK_Enable(false);
		Ana_Set_Reg(AFE_VOW_PERIODIC_CFG13, 0x8000, 0x8000);
		Ana_Set_Reg(AFE_VOW_PERIODIC_CFG14, 0x0000, 0x8000);
		for (i = 0; i < 22; i++) {
			if ((i == 11) || (i == 12)) /*AFE_VOW_PERIODIC_CFG13 & 14 */
				Ana_Set_Reg(AFE_VOW_PERIODIC_CFG2 + (i<<1), 0x0000, 0x3FFF);
			else
				Ana_Set_Reg(AFE_VOW_PERIODIC_CFG2 + (i<<1), 0x0000, 0xFFFF);
		}
		/* Set Period */
		Ana_Set_Reg(AFE_VOW_PERIODIC_CFG0, 0x0000, 0xFFFF);

	} else {
		pr_debug("%s, On_period:%d\n", __func__, 100 - (On_period * 10));

		VOW32KCK_Enable(true);
		switch (MicType) {
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM:
			pBuf = Handset_AMIC_DCC_PeriodicOnOff;
			break;

		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM:
			pBuf = Headset_MIC_DCC_PeriodicOnOff;
			break;

		default:
			break;
		}
		if (On_period > 0) {
			uint16 value;

			/* send ipi to SCP */
			VowDrv_SetPeriodicEnable(true);

			/*  <Periodic ON>  */
			/* 32k_switch, [15]=0 */
			/*  Ana_Set_Reg(AFE_VOW_PERIODIC_CFG13, 0x0000, 0x8000); */
			/* vow_snrdet_periodic_cfg  = 1 */
			/*  Ana_Set_Reg(AFE_VOW_PERIODIC_CFG14, 0x8000, 0x8000); */
			for (i = 0; i < 22; i++) {
				/* Only set periodic cycle value */
				if (i < 11) /* ON periodic */
					value = pBuf[On_period - 1][i] & 0x3FFF;
				else if (i == 11)
					value = 0x8000 | (pBuf[On_period - 1][i] & 0x3FFF);
				else
					value = pBuf[On_period - 1][i] & 0xFFFF;

				Ana_Set_Reg(AFE_VOW_PERIODIC_CFG2 + (i<<1), value, 0xFFFF);
				/* pr_debug("Addr:%x, Value:%x\n",                               */
				/*	AFE_VOW_PERIODIC_CFG2 + (i<<1), pBuf[On_period - 1][i]); */
			}
		} else {
			Ana_Set_Reg(AFE_VOW_PERIODIC_CFG13, 0x8000, 0x8000);
			Ana_Set_Reg(AFE_VOW_PERIODIC_CFG14, 0x0000, 0x8000);
			for (i = 0; i < 22; i++) {
				if ((i == 11) || (i == 12)) /* AFE_VOW_PERIODIC_CFG13 & 14 */
					Ana_Set_Reg(AFE_VOW_PERIODIC_CFG2 + (i<<1), 0x0000, 0x3FFF);
				else
					Ana_Set_Reg(AFE_VOW_PERIODIC_CFG2 + (i<<1), 0x0000, 0xFFFF);
			}
		}
		/* Set Period */
		Ana_Set_Reg(AFE_VOW_PERIODIC_CFG0, 0x999A, 0x3FFF);
	}
}

static void VOW_GPIO_Enable(bool enable)
{
	if (enable == true) {
		/* set AP side GPIO */
		/*Enable VOW_CLK_MISO*/
		/*Enable VOW_DAT_MISO*/
		AudDrv_GPIO_Request(true, Soc_Aud_Digital_Block_ADDA_VOW);
		/*set PMIC side GPIO*/
		Ana_Set_Reg(GPIO_MODE1, 0x1449, 0x0E00); /* 0x9C28 GPIO8 Set to VOW data pin */
	} else {
		/* set AP side GPIO */
		/*Disable VOW_CLK_MISO*/
		/*Disable VOW_DAT_MISO*/
		AudDrv_GPIO_Request(false, Soc_Aud_Digital_Block_ADDA_VOW);
		/*set PMIC GPIO*/
		Ana_Set_Reg(GPIO_MODE1, 0x1249, 0x0E00); /* 0x9C28 GPIO8 Set to default */
	}
}

static void VOW_Pwr_Enable(int MicType, bool enable)
{
	if ((MicType >= AUDIO_VOW_MIC_TYPE_NUM) || (MicType < 0)) {
		pr_debug("%s(),Not support this Mic Type\n", __func__);
		return;
	}
	if (enable == true) {
		/* 0x9818 Enable Globe bias VOW LPW mode */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0050, 0x0040);

		NvregEnable(true); /* 0x9818 Enable audio globe bias */

		if ((MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC)
		 && (MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)) {
			/* 0x9826 Enable audio uplink LPW mode */
			/* Enable Audio ADC 1st, 2nd & 3rd Stage LPW */
			/* Enable Audio ADC flash Audio ADC flash */
			Ana_Set_Reg(AUDENC_ANA_CON4,  0x0039, 0x0039);
		}

		/* 0x984E set PLL VCOBAND */
		Ana_Set_Reg(AUDENC_ANA_CON24, 0x0013, 0x0038);
		/* 0x984C PLL low power */
		Ana_Set_Reg(AUDENC_ANA_CON23, 0x8180, 0x8000);
		/* 0x984A PLL devider ratio, Enable fbdiv relatch, Set DCKO = 1/4 F_PLL, Enable VOWPLL CLK */
		Ana_Set_Reg(AUDENC_ANA_CON22, 0x06F8, 0x07FC);
		/* 0x984A PLL devider ratio, Enable fbdiv relatch, Set DCKO = 1/4 F_PLL, Enable VOWPLL CLK */
		Ana_Set_Reg(AUDENC_ANA_CON22, 0x06F9, 0x0001);

		if ((MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC)
		 && (MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)) {
			/* 0x982A ADC CLK from: 10_12.58MHz from 32KHz PLL */
			/* Enable Audio ADC FBDAC 0.25FS LPW */
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0009, 0x000D);
		}
	} else {
		if ((MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC)
		 && (MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)) {
			/* 0x982A Disable CLKSOURCE */
			Ana_Set_Reg(AUDENC_ANA_CON6, 0x0000, 0x000D);
		}
		/* 0x984A Disable PLL */
		Ana_Set_Reg(AUDENC_ANA_CON22, 0x06F8, 0x0001);
		/* 0x984A Disable PLL */
		Ana_Set_Reg(AUDENC_ANA_CON22, 0x0310, 0x07FC);
		/* 0x984C Disable PLL */
		Ana_Set_Reg(AUDENC_ANA_CON23, 0x0180, 0x8000);
		/* 0x984E PLL VCOBAND (Default 010) */
		Ana_Set_Reg(AUDENC_ANA_CON24, 0x0013, 0x0038);
		if ((MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC)
		 && (MicType != AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)) {
			/* 0x9826 Disable audio uplink LPW mode, */
			/* Disable Audio ADC 1st, 2nd & 3rd Stage LPW, */
			/*Disable Audio ADC flash Audio ADC flash */
			Ana_Set_Reg(AUDENC_ANA_CON4,  0x0000, 0x0039);
		}

		NvregEnable(false); /* 0x9818 Disable audio globe bias */
		/* 0x9818 Disable Globe bias VOW LPW mode */
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0x0040);
	}
}

static void VOW_DCC_CLK_Enable(bool enable)
{
	if (enable == true) {
		VOW12MCK_Enable(true); /* 0x8200 VOW12M_CK power on */

		/* 0xE070 VOW source clock power on, vow_1p6m_800k_sel=1.6m */
		Ana_Set_Reg(AFE_VOW_TOP,      0x4000, 0x8000);
		/* 0xE090 PGA DCC CLK = 13M(12.854M) */
		/* PGA DCC CLK reference select: 01_vow 12.854M */
		/* PGA DCC CLK power down gating release */
		Ana_Set_Reg(AFE_DCCLK_CFG0,   0x2064, 0xFFEE);
		/* 0xE092 bypass resync, dcclk_resync_bypass=1 */
		Ana_Set_Reg(AFE_DCCLK_CFG1,   0x1105, 0x0100);
		/* 0xE090 Enable PGA DCC CLK, dcclk_gen_on = 1'b1 */
		Ana_Set_Reg(AFE_DCCLK_CFG0,   0x2065, 0x0001);
	} else {
		/* 0xE090 Disable PGA DCC CLK */
		Ana_Set_Reg(AFE_DCCLK_CFG0,   0x2064, 0x0001);
		/* 0xE090 PGA DCC CLK = Default 00001111111 */
		/* PGA DCC CLK reference select: 00_normal 26M/2 */
		/* PGA DCC CLK power down gating */
		Ana_Set_Reg(AFE_DCCLK_CFG0,   0x0FE2, 0xFFEE);
		/* 0xE070 VOW source clock power off */
		Ana_Set_Reg(AFE_VOW_TOP,      0xC000, 0x8000);

		VOW12MCK_Enable(false); /* VOW clock power down enable */
	}
}

static void VOW_ACC_CLK_Enable(bool enable)
{
	if (enable == true) {
		VOW12MCK_Enable(true); /* VOW12M_CK power on */

		/* 0xE070 VOW source clock power on, vow_1p6m_800k_sel=1.6m */
		Ana_Set_Reg(AFE_VOW_TOP, 0x4000, 0x8000);
	} else {
		/* 0xE070 VOW source clock power off */
		Ana_Set_Reg(AFE_VOW_TOP, 0xC000, 0x8000);

		VOW12MCK_Enable(false); /* VOW clock power down enable */
	}
}

static void VOW_DMIC_CLK_Enable(bool enable)
{
	if (enable == true) {
		VOW12MCK_Enable(true); /* VOW12M_CK power on */

		/* 0xE070 VOW source clock power on, vow_1p6m_800k_sel=1.6m */
		Ana_Set_Reg(AFE_VOW_TOP, 0x4000, 0x8000);
	} else {
		/* 0xE070 VOW source clock power off */
		Ana_Set_Reg(AFE_VOW_TOP, 0xC000, 0x8000);

		VOW12MCK_Enable(false); /* VOW clock power down enable */
	}
}

static void VOW_MIC_DCC_Enable(int MicType, bool enable)
{
	if ((MicType >= AUDIO_VOW_MIC_TYPE_NUM) || (MicType < 0)) {
		pr_debug("%s(),Not support this Mic Type\n", __func__);
		return;
	}
	if (enable == true) {
		Ana_Set_Reg(AUDENC_ANA_CON10, 0x0030, 0xFFFF); /* for little signal be broken issue */

		switch (MicType) {
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM:
			/* 0x983E MIC Bias 0 LowPower enable, MISBIAS0 = 1.9V, Enable MICBIAS0 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x00A1, 0x00F1);
			if (MicType == AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM)
				Ana_Set_Reg(AUDENC_ANA_CON16, 0x000E, 0x000E); /* ECM diff mode */
			else
				Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x000E); /* normal mode */

			/* 0x981E Enable audio L PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0317, 0x0737);
			break;

		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM:
			/* 0x9840 MIC Bias 1 LowPower: 0_Normal, 1_LPW, MISBIAS1 = 2P7V, Enable MICBIAS1 */
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x00F1, 0x00F1);
			if (MicType == AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM)
				Ana_Set_Reg(AUDENC_ANA_CON17, 0x0002, 0x0006); /* ECM single mode */
			else
				Ana_Set_Reg(AUDENC_ANA_CON17, 0x0000, 0x0006); /* normal mode */

			/* 0x981E Enable audio L PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0327, 0x0737);
			break;
		default:
			break;
		}
		/* 0x981E Audio L ADC input sel : L PGA, Enable audio L ADC */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x5000, 0x7000);

		msleep(20);/* delay */

		/* 0x981E Audio L PGA DCC precharge off */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0004);
	} else {
		/* 0x981E Disable AUDADCCH0_01 */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x7000);
		/* 0x981E Disable AUDPREAMPCH0_01 */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0737);
		switch (MicType) {
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM:
			/* 0x983E Disable AUDMICBIAS0 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x00FF);
			break;
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM:
			/* 0x9840 Disable AUDMICBIAS1 */
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x0000, 0x00F7);
			break;
		default:
			break;
		}
		Ana_Set_Reg(AUDENC_ANA_CON10, 0x1515, 0xFFFF); /* for little signal be broken issue */
	}
}

static void VOW_MIC_ACC_Enable(int MicType, bool enable)
{
	if ((MicType >= AUDIO_VOW_MIC_TYPE_NUM) || (MicType < 0)) {
		pr_debug("%s(),Not support this Mic Type\n", __func__);
		return;
	}
	if (enable == true) {
		Ana_Set_Reg(AUDENC_ANA_CON10, 0x0030, 0xFFFF); /* for little signal be broken issue */

		switch (MicType) {
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC:
			/* 0x983E MIC Bias 0 LowPower: 0_Normal, 1_LPW, MISBIAS1 = 2P7V, Enable MICBIAS1 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x00A1, 0x00F1);
			/* 0x981E Audio L PGA precharge off, */
			/* Audio L PGA mode: 0_ACC, */
			/* Audio L preamplifier input sel : AIN0, */
			/* Audio L PGA 18 dB gain, */
			/* Enable audio L PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0311, 0x0737);
			break;
		case AUDIO_VOW_MIC_TYPE_Headset_MIC:
			/* 0x9840 MIC Bias 1 LowPower: 0_Normal, 1_LPW, MISBIAS1 = 2P7V, Enable MICBIAS1 */
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x00F1, 0x00F1);
			/* 0x981E Audio L PGA precharge off, */
			/* Audio L PGA mode: 0_ACC, */
			/* Audio L preamplifier input sel : AIN1, */
			/* Audio L PGA 18 dB gain, Enable audio L PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0,  0x0321, 0x0737);
			break;
		default:
			break;
		}

		/* 0x981E Audio L ADC input sel : L PGA, Enable audio L ADC */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x5000, 0x7000);
	} else {
		/* 0x981E Disable AUDADCCH0_01 */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x7000);
		/* 0x981E Disable AUDPREAMPCH0_01 */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0737);
		switch (MicType) {
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC:
			/* 0x983E Disable AUDMICBIAS0 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x00F1);
			break;
		case AUDIO_VOW_MIC_TYPE_Headset_MIC:
			/* 0x9840 Disable AUDMICBIAS1 */
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x0000, 0x00F1);
			break;
		default:
			break;
		}
		Ana_Set_Reg(AUDENC_ANA_CON10, 0x1515, 0xFFFF); /* for little signal be broken issue */
	}
}
#endif

static bool TurnOnVOWADcPower(int MicType, bool enable)
{
#ifdef CONFIG_MTK_VOW_SUPPORT
	pr_debug("%s MicType = %d enable = %d, mIsVOWOn=%d, mAudio_VOW_Mic_type=%d\n",
		__func__, MicType, enable, mIsVOWOn, mAudio_VOW_Mic_type);

	if ((MicType >= AUDIO_VOW_MIC_TYPE_NUM) || (MicType < 0)) {
		pr_debug("%s(),Not support this Mic Type\n", __func__);
		return false;
	}

	/*already on, no need to set again*/
	if (enable == mIsVOWOn)
		return true;

	if (enable) {
		mIsVOWOn = true;
		SetVOWStatus(mIsVOWOn);
#if 0
		if (GetMicbias == 0) {
			/* save current micbias ref set by accdet */
			MicbiasRef = Ana_Get_Reg(AUDENC_ANA_CON9) & 0x0700;
			pr_debug("MicbiasRef=0x%x\n", MicbiasRef);
			GetMicbias = 1;
		}
#endif
		VOW_Pwr_Enable(MicType, true);

		switch (MicType) {
		/* for ACC Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC:  /* AMIC_ACC */
		case AUDIO_VOW_MIC_TYPE_Headset_MIC: /* Earphone_ACC */
			VOW_ACC_CLK_Enable(true);
			VOW_MIC_ACC_Enable(MicType, true);
			break;
		/* for DCC Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC:  /* AMIC_DCC */
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC:   /* Earphone_DCC */
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM:
			VOW_DCC_CLK_Enable(true);
			VOW_MIC_DCC_Enable(MicType, true);
			break;
		/* for Digital Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC:  /* DMIC */
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K:
			VOW_DMIC_CLK_Enable(true);
			/* 0x9838 Enable DMIC*/
			Ana_Set_Reg(AUDENC_ANA_CON13,  0x0005, 0x0007);
			break;
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01:
			VOW_DMIC_CLK_Enable(true);
			/* 0x0D1A MIC Bias 0 LowPower: 1_LPW, MISBIAS0 = 1P9V, Enable MICBIAS0 */
			/* Ana_Set_Reg(AUDENC_ANA_CON9,  0x00A1, 0x00F1); */
			Ana_Set_Reg(AUDENC_ANA_CON13,  0x0005, 0x0007); /* 0xD18 Enable DMIC*/
			/* Set Eint GPIO */
			VowDrv_SetSmartDevice_GPIO(true);
			break;
		default:
			break;
		}

		VOW_GPIO_Enable(true);

		/* VOW AMPREF Setting, set by MD32 after DC calibration */
		Ana_Set_Reg(AFE_VOW_CFG0, reg_AFE_VOW_CFG0, 0xffff);   /* 0xffff */
		Ana_Set_Reg(AFE_VOW_CFG1, reg_AFE_VOW_CFG1, 0xffff);   /*VOW A,B timeout initial value 0x0200*/
		Ana_Set_Reg(AFE_VOW_CFG2, reg_AFE_VOW_CFG2, 0xffff);   /*VOW A,B value setting 0x2424*/
		Ana_Set_Reg(AFE_VOW_CFG3, reg_AFE_VOW_CFG3, 0xffff);   /*alhpa and beta K value setting 0xDBAC*/
		Ana_Set_Reg(AFE_VOW_CFG4, reg_AFE_VOW_CFG4, 0x000f);   /*gamma K value setting 0x029E*/
		Ana_Set_Reg(AFE_VOW_CFG5, reg_AFE_VOW_CFG5, 0xffff);   /*N mini value setting 0x0000*/
		if (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01)
			Ana_Set_Reg(AFE_VOW_CFG6, reg_AFE_VOW_CFG6, 0x0000);   /* FLR value setting 0x0000 */
		else
			Ana_Set_Reg(AFE_VOW_CFG6, reg_AFE_VOW_CFG6, 0x0700);   /* FLR value setting 0x0000 */

		if (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01) {
			/* 16K */
			Ana_Set_Reg(AFE_VOW_CFG4, 0x024E, 0xfff0); /* 16k */
			Ana_Set_Reg(AFE_VOW_POSDIV_CFG0, 0x0C0A, 0xffff);
			/* Ana_Set_Reg(AFE_VOW_CFG4, 0x022E, 0xfff0);*/ /*32K*/
			/* Ana_Set_Reg(AFE_VOW_POSDIV_CFG0, 0x2C0A, 0xffff);*/ /* 32K*/
		} else {
		/* 16K */
			Ana_Set_Reg(AFE_VOW_CFG4, 0x029E, 0xfff0); /* 16k */
			if (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)
				Ana_Set_Reg(AFE_VOW_POSDIV_CFG0, 0x0C08, 0xffff); /* 800k */
			else
				Ana_Set_Reg(AFE_VOW_POSDIV_CFG0, 0x0C00, 0xffff); /* 1.6m */
		}
		TurnOnVOWPeriodicOnOff(MicType, reg_AFE_VOW_PERIODIC, true);

#ifndef VOW_STANDALONE_CONTROL
		if (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC) {
			/*digital MIC need to config bit13 and bit6, (bit7 need to check)  0x6840*/

			VowDrv_SetDmicLowPower(false);

			Ana_Set_Reg(AFE_VOW_TOP, 0x20C0, 0x20C0);   /*VOW enable, with bit7*/
		} else if (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K) {

			VowDrv_SetDmicLowPower(true);

			Ana_Set_Reg(AFE_VOW_TOP, 0x20C0, 0x20C0);   /*VOW enable, with bit7*/
		} else {
			/* Normal */
			VowDrv_SetDmicLowPower(false);
		}
#endif /* #ifndef VOW_STANDALONE_CONTROL */


		/*VOW enable, set AFE_VOW_TOP in VOW kernel driver*/
		/*need to inform VOW driver mic type*/
		VowDrv_EnableHW(true);

		VowDrv_ChangeStatus();

	} else { /* disable VOW */

		TurnOnVOWPeriodicOnOff(MicType, reg_AFE_VOW_PERIODIC, false);

		/*Set VOW driver disable, vow driver will do close all digital part setting*/
		VowDrv_EnableHW(false);

		VowDrv_ChangeStatus();
		msleep(20);

		VOW_GPIO_Enable(false);
		if ((MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC)
		 || (MicType == AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K)) {
			VowDrv_SetDmicLowPower(false);
			Ana_Set_Reg(AFE_VOW_TOP, 0x0000, 0x20C0);   /*VOW disable, with bit7*/
		}
		switch (MicType) {
		/* for ACC Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC:
			/*turn off analog part*/
			VOW_MIC_ACC_Enable(MicType, false);
			VOW_ACC_CLK_Enable(false);
			break;
		/* for DCC Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Handset_AMIC_DCCECM:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCC:
		case AUDIO_VOW_MIC_TYPE_Headset_MIC_DCCECM:
			/*turn off analog part*/
			VOW_MIC_DCC_Enable(MicType, false);
			VOW_DCC_CLK_Enable(false);
			break;
		/* for Digital Mic */
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC:
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC_800K:
			/* 0x9838 Disable DMIC */
			Ana_Set_Reg(AUDENC_ANA_CON13,  0x0004, 0x0007);
			VOW_DMIC_CLK_Enable(false);
			break;
		case AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01:
			/* Set Eint GPIO */
			VowDrv_SetSmartDevice_GPIO(false);
			Ana_Set_Reg(AFE_VOW_TOP, 0x0000, 0x0080); /* bit7 , clock select */
			VOW_DMIC_CLK_Enable(false);
			break;
		default:
			break;
		}

		VOW_Pwr_Enable(MicType, false);

		mIsVOWOn = false;
		SetVOWStatus(mIsVOWOn);
#if 0
		GetMicbias = 0;
#endif
	}
#endif /* #ifdef CONFIG_MTK_VOW_SUPPORT */
	return true;
}


/* here start uplink power function */
static const char *const ADC_function[] = { "Off", "On" };

/* OPEN:0, IN_ADC1: 1, IN_ADC2:2, IN_ADC3:3 */
static const char *const ADC_UL_PGA_GAIN[] = {"0Db", "6Db", "12Db", "18Db", "24Db"};
static const char *const audio_adc1_mic_source[] = {"AIN0", "AIN1", "AIN2"};
static const char *const audio_adc2_mic_source[] = {"AIN0", "AIN3", "AIN2"};
static const char *const audio_adc3_mic_source[] = {"AIN5", "AIN4", "AIN6"};
static const char *const audio_adc4_mic_source[] = {"AIN5", "AIN0", "AIN6"};
static const char *const audio_ul_array_in[] = {
	"ADC1", "ADC2", "ADC3", "ADC4",
	"DMIC0", "DMIC1_Lch", "DMIC1_Rch", "DMIC2_Lch", "DMIC2_Rch"
};
static const char *const audio_adc_mic_mode[] = {
	"ACCMODE", "DCCMODE", "DCCECMDIFFMODE", "DCCECMSINGLEMODE"
};
static const char *const Audio_VOW_ADC_Function[] = { "Off", "On" };
static const char *const Audio_VOW_Digital_Function[] = { "Off", "On" };

static const char *const Audio_VOW_MIC_Type[] = {
	"HandsetAMIC", "HeadsetMIC", "HandsetDMIC", "HandsetDMIC_800K",
	"HandsetAMIC_DCC", "HeadsetMIC_DCC", "HandsetAMIC_DCCECM",
	"HeadsetMIC_DCCECM", "HandsetDMIC_VENDOR01"
};
/* here start uplink power function */
static const char * const Pmic_Test_function[] = { "Off", "On" };
static const char * const Pmic_LPBK_function[] = { "Off", "LPBK3" };

static const struct soc_enum Audio_UL_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_function), ADC_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_ul_array_in), audio_ul_array_in),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_adc1_mic_source), audio_adc1_mic_source),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_adc2_mic_source), audio_adc2_mic_source),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_adc3_mic_source), audio_adc3_mic_source),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_adc4_mic_source), audio_adc4_mic_source),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ADC_UL_PGA_GAIN), ADC_UL_PGA_GAIN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_adc_mic_mode), audio_adc_mic_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_ADC_Function),
			    Audio_VOW_ADC_Function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_Digital_Function),
			    Audio_VOW_Digital_Function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_VOW_MIC_Type),
			    Audio_VOW_MIC_Type),
};

static int channel_map_to_device(enum audio_analog_uplink_array_type channel)
{
	switch (channel) {
	case AUDIO_UL_ARRAY_ADC1:
		return AUDIO_ANALOG_DEVICE_IN_ADC1;
	case AUDIO_UL_ARRAY_ADC2:
		return AUDIO_ANALOG_DEVICE_IN_ADC2;
	case AUDIO_UL_ARRAY_ADC3:
		return AUDIO_ANALOG_DEVICE_IN_ADC3;
	case AUDIO_UL_ARRAY_ADC4:
		return AUDIO_ANALOG_DEVICE_IN_ADC4;
	case AUDIO_UL_ARRAY_DMIC0_LCH:
		return AUDIO_ANALOG_DEVICE_IN_DMIC0;
	case AUDIO_UL_ARRAY_DMIC1_LCH:
	case AUDIO_UL_ARRAY_DMIC1_RCH:
		return AUDIO_ANALOG_DEVICE_IN_DMIC1;
	case AUDIO_UL_ARRAY_DMIC2_LCH:
	case AUDIO_UL_ARRAY_DMIC2_RCH:
		return AUDIO_ANALOG_DEVICE_IN_DMIC2;
	default:
		pr_err("%s can't mapping channel %d to device", __func__, channel);
	}
	return 0;
}

static void audio_preamp1_en(bool power)
{
	int mic_mode = adc1_mic_mode_mux;
	int mic_source = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];
	int mic_gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];

	pr_warn("%s power=%d, mic_mode=%d, mic_source=%d, mic_gain=%d", __func__,
	       power, mic_mode, mic_source, mic_gain);

	if (power) { /* power on */
		switch (mic_source) {
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0021, 0x00f1);
			/* Enable MICBIAS0, MISBIAS0 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0010, 0x0030);
			/* Setting and Enable Audio CH0_01 PGA  input sel : AIN0*/
			break;
		case AUDIO_AMIC_AIN1:
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x0001, 0x0081);
			/* MIC Bias 1 high power mode, power on : normal mode */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0020, 0x0030);
			/* Setting and Enable Audio CH0_01 PGA  input sel : AIN1*/
			break;
		case AUDIO_AMIC_AIN2:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x2100, 0xf100);
			/* Enable MICBIAS2, MISBIAS2 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0030, 0x0030);
			/* Setting and Enable Audio CH0_01 PGA  input sel : AIN2*/
			break;
		default:
			pr_warn("preamp1 : wrong mic source %d", mic_source);
			break;
		}

		Ana_Set_Reg(AUDENC_ANA_CON0, mic_gain << 8, 0x7 << 8);
		/* Mic Gain : Audio CH0_01 preamplifier gain adjust */

		if (mic_mode == AUDIO_ANALOGUL_MODE_ACC) {
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0001, 0x0007);
			/* Setting and Enable Audio CH0_01 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x5000, 0x7000);
			/* Select Input CH0 PGA and Enable audio CH0 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0008, 0x00C8);
			/* Audio CH0_01 PGA high performance mode*/
		} else if (mic_mode == AUDIO_ANALOGUL_MODE_DCC) {
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0007, 0x0007);
			/* Setting and Enable Audio CH0_01 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x5000, 0x7000);
			/* Select Input CH0 PGA and Enable audio CH0 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x00C8, 0x00C8);
			/* Audio CH0_01 PGA high performance mode*/
			udelay(100);
			Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0004);
			/* Audio CH0_01 PGA DCC precharge off */
		}
	} else {  /* power off */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x00C8);
		/* disable high performance mode*/
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x7000);
		/* disable AUDADCCH0_01 */
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0x0737);
		/* disable AUDPREAMPCH0_01  input sel : NONE */
		switch (mic_source) {
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x0001);
			/* disable MICBIAS0 */
			break;
		case AUDIO_AMIC_AIN1:
			Ana_Set_Reg(AUDENC_ANA_CON17, 0x0080, 0x0081);
			/* disable MICBIAS1 */
			break;
		case AUDIO_AMIC_AIN2:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0xf100);
			/* disable MICBIAS2 */
			break;
		default:
			pr_warn("preamp1 : wrong mic source %d", mic_source);
			break;
		}
	}
}

static void audio_preamp2_en(bool power)
{
	int mic_mode = adc2_mic_mode_mux;
	int mic_source = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2];
	int mic_gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];

	pr_warn("%s power=%d, mic_mode=%d, mic_source=%d, mic_gain=%d", __func__,
	       power, mic_mode, mic_source, mic_gain);

	if (power) {  /* power on */
		switch (mic_source) {
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0021, 0x00f1);
			/* Enable MICBIAS0, MISBIAS0 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0010, 0x0030);
			/* Setting and Enable Audio CH1_23 PGA  input sel : AIN0*/
			break;
		case AUDIO_AMIC_AIN3:
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0020, 0x0030);
			/* Setting and Enable Audio CH1_23 PGA  input sel : AIN3*/
			break;
		case AUDIO_AMIC_AIN2:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x2100, 0xf100);
			/* Enable MICBIAS2, MISBIAS2 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0030, 0x0030);
			/* Setting and Enable AudioCH1_23 PGA  input sel : AIN2*/
			break;
		default:
			pr_warn("preamp2 : wrong mic source %d", mic_source);
			break;
		}

		Ana_Set_Reg(AUDENC_ANA_CON1, mic_gain << 8, 0x7 << 8);
		/* Mic Gain : Audio CH2_23 preamplifier gain adjust */

		if (mic_mode == AUDIO_ANALOGUL_MODE_ACC) {
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0001, 0x0007);
			/* Setting and Enable Audio CH1 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x5000, 0x7000);
			/* Select Input CH1 PGA and Enable audio CH1 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0008, 0x00C8);
			/* Audio CH1 PGA high performance mode*/
		} else if (mic_mode == AUDIO_ANALOGUL_MODE_DCC) {
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0007, 0x0007);
			/* Setting and Enable Audio CH1PGA */
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x5000, 0x7000);
			/* Select Input CH1 PGA and Enable audio CH1 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x00C8, 0x00C8);
			/* Audio CH1 PGA high performance mode*/
			udelay(100);
			Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x0004);
			/* Audio CH1 PGA DCC precharge off */
		}
	} else {  /* power off */
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x00C8);
		/* disable high performance mode*/
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x7000);
		/* disable AUDADCCH1_23 */
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0x0737);
		/* disable AUDPREAMPCH1_23  input sel : NONE */
		switch (mic_source) {
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x0001);
			/* disable MICBIAS0 */
			break;
		case AUDIO_AMIC_AIN3:
			break;
		case AUDIO_AMIC_AIN2:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0xf100);
			/* disable MICBIAS2 */
			break;
		default:
			pr_warn("preamp2 : wrong mic source %d", mic_source);
			break;
		}
	}
}

static void audio_preamp3_en(bool power)
{
	int mic_mode = adc3_mic_mode_mux;
	int mic_source = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3];
	int mic_gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3];

	pr_warn("%s power=%d, mic_mode=%d, mic_source=%d, mic_gain=%d", __func__,
	       power, mic_mode, mic_source, mic_gain);

	if (power) { /* power on */
		switch (mic_source) {
		case AUDIO_AMIC_AIN5:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3, MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0010, 0x0030);
			/* Setting and Enable Audio CH2_45 PGA  input sel : AIN5*/
			break;
		case AUDIO_AMIC_AIN4:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3, MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0020, 0x0030);
			/* Setting and Enable Audio CH2_45 PGA  input sel : AIN4*/
			break;
		case AUDIO_AMIC_AIN6:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3, MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0030, 0x0030);
			/* Setting and Enable Audio CH2_45 PGA  input sel : AIN6*/
			break;
		default:
			pr_warn("preamp3 : wrong mic source %d", mic_source);
			break;
		}

		Ana_Set_Reg(AUDENC_ANA_CON2, mic_gain << 8, 0x7 << 8);
		/* Mic Gain : Audio CH2_45 preamplifier gain adjust */

		if (mic_mode == AUDIO_ANALOGUL_MODE_ACC) {
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0001, 0x0007);
			/* Setting and Enable Audio CH2_45 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x5000, 0x7000);
			/* Select Input CH2 PGA and Enable audio CH2 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0008, 0x00C8);
			/* Audio CH2 PGA high performance mode*/
		} else if (mic_mode == AUDIO_ANALOGUL_MODE_DCC) {
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0007, 0x0007);
			/* Setting and Enable Audio CH2 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x5000, 0x7000);
			/* Select Input CH2 PGA and Enable audio CH2 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x00C8, 0x00C8);
			/* Audio CH2 PGA high performance mode*/
			udelay(100);
			Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0x0004);
			/* Audio CH2 PGA DCC precharge off */
		}
	} else {  /* power off */
		Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0x00C8);
		/* disable high performance mode*/
		Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0x7000);
		/* disable AUDADCCH2_45 */
		Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0x0737);
		/* disable AUDPREAMPCH2_45  input sel : NONE */
		switch (mic_source) {
		case AUDIO_AMIC_AIN5:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* disable MICBIAS3 */
			break;
		case AUDIO_AMIC_AIN4:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* disable MICBIAS3 */
			break;
		case AUDIO_AMIC_AIN6:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* disable MICBIAS3 */
			break;
		default:
			pr_warn("preamp3 : wrong mic source %d", mic_source);
			break;
		}
	}
}

static void audio_preamp4_en(bool power)
{
	int mic_mode = adc4_mic_mode_mux;
	int mic_source = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4];
	int mic_gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4];

	pr_warn("%s power=%d, mic_mode=%d, mic_source=%d, mic_gain=%d", __func__,
	       power, mic_mode, mic_source, mic_gain);

	if (power) { /* power on */
		switch (mic_source) {
		case AUDIO_AMIC_AIN5:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3, MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0010, 0x0030);
			/* Setting and Enable Audio CH3_6 PGA  input sel : AIN5*/
			break;
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0021, 0x00f1);
			/* Enable MICBIAS0, MISBIAS0 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0020, 0x0030);
			/* Setting and Enable Audio CH3_6 PGA  input sel : AIN0*/
			break;
		case AUDIO_AMIC_AIN6:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3, MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0030, 0x0030);
			/* Setting and Enable Audio CH3_6 PGA  input sel : AIN6*/
			break;
		default:
			pr_warn("preamp4 : wrong mic source %d", mic_source);
			break;
		}

		Ana_Set_Reg(AUDENC_ANA_CON3, mic_gain << 8, 0x7 << 8);
		/* Mic Gain : Audio CH3_6 preamplifier gain adjust */

		if (mic_mode == AUDIO_ANALOGUL_MODE_ACC) {
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0001, 0x0007);
			/* Setting and Enable Audio CH3_6 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x5000, 0x7000);
			/* Select Input CH3 PGA and Enable audio CH3 ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0008, 0x00C8);
			/* Audio CH3PGA high performance mode*/
		} else if (mic_mode == AUDIO_ANALOGUL_MODE_DCC) {
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0007, 0x0007);
			/* Setting and Enable Audio CH3 PGA */
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x5000, 0x7000);
			/* Select Input CH3 PGA and Enable audio CH3ADC*/
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x00C8, 0x00C8);
			/* Audio CH3 PGA high performance mode*/
			udelay(100);
			Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0x0004);
			/* Audio CH3 PGA DCC precharge off */
		}
	} else {  /* power off */
		Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0x00C8);
		/* disable high performance mode*/
		Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0x7000);
		/* disable AUDADCCH3_6 */
		Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0x0737);
		/* disable AUDPREAMPCH3_6 input sel : NONE */
		switch (mic_source) {
		case AUDIO_AMIC_AIN5:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* disable MICBIAS3 */
			break;
		case AUDIO_AMIC_AIN0:
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x0001);
			/* disable MICBIAS0 */
			break;
		case AUDIO_AMIC_AIN6:
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* disable MICBIAS3 */
			break;
		default:
			pr_warn("preamp4 : wrong mic source %d", mic_source);
			break;
		}
	}
}

static void audio_dmic_input_enable(bool power, enum audio_analog_device_type device_in)
{
	switch (device_in) {
	case AUDIO_ANALOG_DEVICE_IN_DMIC0:
		if (power) {
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0021, 0x00f1);
			/* Enable MICBIAS0 , MISBIAS0 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON13, 0x0005, 0x0007);
			/* Enable DMIC0 */
		} else {
			Ana_Set_Reg(AUDENC_ANA_CON13, 0x0000, 0x0007);
			/* disable DMIC0 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0x0001);
			/* Enable MICBIAS0 , MISBIAS0 = 1P9V */
		}
		break;
	case AUDIO_ANALOG_DEVICE_IN_DMIC1:
		if (power) {
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x2100, 0xf100);
			/* Enable MICBIAS2 , MISBIAS2 = 1P9V */
			Ana_Set_Reg(AUDENC_ANA_CON14, 0x0005, 0x0007);
			/* Enable DMIC1 */
		} else {
			Ana_Set_Reg(AUDENC_ANA_CON14, 0x0000, 0x0007);
			/* disable DMIC1 */
			Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0xf100);
			/* Enable MICBIAS2 , MISBIAS2 = 1P9V */
		}
		break;
	case AUDIO_ANALOG_DEVICE_IN_DMIC2:
		if (power) {
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0071, 0x00f1);
			/* Enable MICBIAS3 , MISBIAS3 = 2P7V */
			Ana_Set_Reg(AUDENC_ANA_CON15, 0x0005, 0x0007);
			/* Enable DMIC2 */
		} else {
			Ana_Set_Reg(AUDENC_ANA_CON15, 0x0000, 0x0007);
			/* disable DMIC2 */
			Ana_Set_Reg(AUDENC_ANA_CON18, 0x0000, 0x00f1);
			/* Enable MICBIAS3 , MISBIAS3 = 2P7V */
		}
		break;
	default:
		pr_warn("%s not support this device_type = %d", __func__, device_in);
		break;
	}
	pr_aud("%s power = %d, device_type = %d,", __func__, power, device_in);
}

static void audio_adc_enable(bool power, enum audio_analog_device_type adc)
{
	int mic_mode = 0;

	switch (adc) {
	case AUDIO_ANALOG_DEVICE_IN_ADC1:
		mic_mode = adc1_mic_mode_mux;
		break;
	case AUDIO_ANALOG_DEVICE_IN_ADC2:
		mic_mode = adc2_mic_mode_mux;
		break;
	case AUDIO_ANALOG_DEVICE_IN_ADC3:
		mic_mode = adc3_mic_mode_mux;
		break;
	case AUDIO_ANALOG_DEVICE_IN_ADC4:
		mic_mode = adc4_mic_mode_mux;
		break;
	default:
		pr_warn("%s can't mapping adc %d to mic mode", __func__, adc);
		break;
	}

	pr_aud("%s, power(%d), adc(%d), mic_mode(%d)\n", __func__, power, adc, mic_mode);

	switch (mic_mode) {
	case AUDIO_ANALOGUL_MODE_ACC:
		TurnOnADcPowerACC(adc, power);
		break;
	case AUDIO_ANALOGUL_MODE_DCC:
		TurnOnADcPowerDCC(adc, power, 0);
		break;
	case AUDIO_ANALOGUL_MODE_DCCECMDIFF:
		TurnOnADcPowerDCC(adc, power, 1);
		break;
	case AUDIO_ANALOGUL_MODE_DCCECMSINGLE:
		TurnOnADcPowerDCC(adc, power, 2);
		break;
	default:
		pr_err("return -EINVAL of Mic mode = %d\n", mic_mode);
		break;
	}
}

static void audio_uplink1_cic_setting(bool power, bool dmic_flag)
{
	int samplerate_ul1 = mBlockSampleRate[AUDIO_DAI_UL1];
	int ul1_index = GetULFrequency(mBlockSampleRate[AUDIO_DAI_UL1]);

	pr_aud("%s samplerate(%d) power(%d) dmic_flag(%d)",
	       __func__, samplerate_ul1, power, dmic_flag);

	if (power && !dmic_flag) { /* amic power on setting*/
		/* UL1 setting and turn on */
		Ana_Set_Reg(AFE_UL_SRC_CON0_H,
			    (ULSampleRateTransform(samplerate_ul1) << 1),
			    0x7 << 1);
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);

	} else if (power && dmic_flag) { /* dmic power on setting*/
		/* UL1 setting and turn on */
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x00e0, 0xffe0);
		Ana_Set_Reg(AFE_UL_SRC_CON0_H,
			    (ULSampleRateTransform(samplerate_ul1) << 1),
			    0x7 << 1);

		if (samplerate_ul1 <= 48000) {
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, (ul1_index << 13), 0x2000);
			/* dmic sample rate, ch1 and ch2 set to 3.25MHz 48k */
		} else {
			/* hires : use 4.33MHz mode */
			Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0040, 0x0040);
		}

		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0003, 0x0003);
	} else {
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0xffff);
		/* UL1 turn off */
	}
}

static void audio_uplink2_cic_setting(bool power, bool dmic_flag)
{
	int samplerate_ul2 = mBlockSampleRate[AUDIO_DAI_UL2];
	int ul2_index = GetULFrequency(mBlockSampleRate[AUDIO_DAI_UL2]);

	if (power && !dmic_flag) { /* amic power on setting*/
		/* UL2 fixed 260k anc path and turn on */
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_H,
			    (ULSampleRateTransform(samplerate_ul2) << 1),
			    0x7 << 1);
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, 0x0001, 0xffff);

		if (samplerate_ul2 <= 48000) {
			anc_ul_src_enable(true); /* anc ul path src on */
			anc_clk_enable(true); /* ANC clk pdn release */
		}

	} else if (power && dmic_flag) { /* dmic power on setting*/
		/* UL2 fixed 260k anc path and turn on */
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_H, 0x00e0, 0xffe0);
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_H,
			    (ULSampleRateTransform(samplerate_ul2) << 1),
			    0x7 << 1);

		if (samplerate_ul2 <= 48000) {
			Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, (ul2_index << 13), 0x2000);
			/* dmic sample rate, ch1 and ch2 set to 3.25MHz 48k */
			Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, 0x0800, 0x0800);
			/* digmic_duplicate_mode to 6.25M */
			anc_ul_src_enable(true); /* anc ul path src on */
			anc_clk_enable(true); /* ANC clk pdn release */
		} else {
			/* hires : use 4.33MHz mode */
			Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, 0x0040, 0x0040);
		}

		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, 0x0003, 0x0003);
	} else {
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_H, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_ADDA6_UL_SRC_CON0_L, 0x0000, 0xffff);
		/*UL2 turn off */
		if (samplerate_ul2 <= 48000) {
			anc_ul_src_enable(false); /* anc ul path src off */
			anc_clk_enable(false); /* ANC clk pdn enable */
		}
	}
}

static int audio_capture1_enable(bool power)
{
	int lch = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX];
	int rch = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX];
	int device_lch = channel_map_to_device(lch);
	int device_rch = channel_map_to_device(rch);
	int *counter1 = &mCodec_data->mAudio_Ana_DevicePower[device_lch];
	int *counter2 = &mCodec_data->mAudio_Ana_DevicePower[device_rch];

	if ((is_amic(device_lch) && is_dmic(device_rch)) ||
	    (is_dmic(device_lch) && is_amic(device_rch))) {
		pr_err("%s one is amic and another is dmic, lch(%d) rch(%d)", __func__, device_lch, device_rch);
		return -EPERM;
	}

	if (power) {
		/* uplink1 left channel device */
		if (*counter1 == 0) {
			if (is_amic(device_lch))
				audio_adc_enable(true, device_lch);
			else if (is_dmic(device_lch))
				audio_dmic_enable(true, device_lch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_lch);
		}
		(*counter1)++;

		/* uplink1 right channel device*/
		if (*counter2 == 0) {
			if (is_amic(device_rch))
				audio_adc_enable(true, device_rch);
			else if (is_dmic(device_rch))
				audio_dmic_enable(true, device_rch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_rch);
		}
		(*counter2)++;

		audio_uplink1_cic_setting(true, is_dmic(device_lch));
	} else {
		audio_uplink1_cic_setting(false, is_dmic(device_lch));

		/* uplink1 left channel device */
		(*counter1)--;
		if (*counter1 == 0) {
			if (is_amic(device_lch))
				audio_adc_enable(false, device_lch);
			else if (is_dmic(device_lch))
				audio_dmic_enable(false, device_lch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_lch);
		}
		if (*counter1 < 0) {
			pr_warn(" device power(%d) < 0 = %d\n ", device_lch, *counter1);
			*counter1 = 0;
		}

		/* uplink1 right channel device*/
		(*counter2)--;
		if (*counter2 == 0) {
			if (is_amic(device_rch))
				audio_adc_enable(false, device_rch);
			else if (is_dmic(device_rch))
				audio_dmic_enable(false, device_rch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_rch);
		}
		if (*counter2 < 0) {
			pr_warn(" device power(%d) < 0 = %d\n ", device_rch, *counter2);
			*counter2 = 0;
		}
	}
	return 0;
}

static int audio_capture2_enable(bool power)
{
	int lch = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX];
	int rch = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX];
	int device_lch = channel_map_to_device(lch);
	int device_rch = channel_map_to_device(rch);
	int *counter1 = &mCodec_data->mAudio_Ana_DevicePower[device_lch];
	int *counter2 = &mCodec_data->mAudio_Ana_DevicePower[device_rch];

	if ((is_amic(device_lch) && is_dmic(device_rch)) ||
	    (is_dmic(device_lch) && is_amic(device_rch))) {
		pr_err("channel one is amic and another is dmic, lch(%d) rch(%d)", device_lch, device_rch);
		return -EPERM;
	}

	if (power) {
		/* uplink1 left channel device */
		if (*counter1 == 0) {
			if (is_amic(device_lch))
				audio_adc_enable(true, device_lch);
			else if (is_dmic(device_lch))
				audio_dmic_enable(true, device_lch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_lch);
		}
		(*counter1)++;

		/* uplink1 right channel device*/
		if (*counter2 == 0) {
			if (is_amic(device_rch))
				audio_adc_enable(true, device_rch);
			else if (is_dmic(device_rch))
				audio_dmic_enable(true, device_rch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_rch);
		}
		(*counter2)++;

		audio_uplink2_cic_setting(true, is_dmic(device_lch));
	} else {
		audio_uplink2_cic_setting(false, is_dmic(device_lch));

		/* uplink1 left channel device */
		(*counter1)--;
		if (*counter1 == 0) {
			if (is_amic(device_lch))
				audio_adc_enable(false, device_lch);
			else if (is_dmic(device_lch))
				audio_dmic_enable(false, device_lch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_lch);
		}
		if (*counter1 < 0) {
			pr_warn(" device power(%d) < 0 = %d\n ", device_lch, *counter1);
			*counter1 = 0;
		}

		/* uplink1 right channel device*/
		(*counter2)--;
		if (*counter2 == 0) {
			if (is_amic(device_rch))
				audio_adc_enable(false, device_rch);
			else if (is_dmic(device_rch))
				audio_dmic_enable(false, device_rch);
			else
				pr_warn("%s is not uplink device %d", __func__, device_rch);
		}
		if (*counter2 < 0) {
			pr_warn(" device power(%d) < 0 = %d\n ", device_rch, *counter2);
			*counter2 = 0;
		}
	}
	return 0;
}

static int audio_capture1_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int power = mCodec_data->audio_interface_power[AUDIO_DAI_UL1];

	pr_aud("%s = %d\n", __func__, power);
	ucontrol->value.integer.value[0] = power;
	return 0;
}

static int audio_capture1_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int power = ucontrol->value.integer.value[0];
	int *ul1_power = &mCodec_data->audio_interface_power[AUDIO_DAI_UL1];

	pr_aud("%s power = %d , ul_power = %d\n", __func__, power, *ul1_power);
	mutex_lock(&Ana_Power_Mutex);

	if (power && !(*ul1_power)) {
		audio_capture1_enable(true);
		*ul1_power = power;
	} else if (!power && (*ul1_power)) {
		*ul1_power = power;
		audio_capture1_enable(false);
	} else {
		pr_warn("%s Nothing happened : ctrl_power = %d , ul_power = %d\n",
			__func__, power, *ul1_power);
	}

	mutex_unlock(&Ana_Power_Mutex);
	return 0;
}

static int audio_capture2_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int power = mCodec_data->audio_interface_power[AUDIO_DAI_UL2];

	pr_aud("%s = %d\n", __func__, power);
	ucontrol->value.integer.value[0] = power;
	return 0;
}

static int audio_capture2_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int power = ucontrol->value.integer.value[0];
	int *ul2_power = &mCodec_data->audio_interface_power[AUDIO_DAI_UL2];

	pr_aud("%s power = %d , ul_power = %d\n", __func__, power, *ul2_power);
	mutex_lock(&Ana_Power_Mutex);

	if (power && !(*ul2_power)) {
		audio_capture2_enable(true);
		*ul2_power = power;
	} else if (!power && (*ul2_power)) {
		*ul2_power = power;
		audio_capture2_enable(false);
	} else {
		pr_warn("%s Nothing happened : ctrl_power = %d , ul_power = %d\n",
			__func__, power, *ul2_power);
	}

	mutex_unlock(&Ana_Power_Mutex);
	return 0;
}


/* PGA1: ADC1 MIC GAIN */
static int audio_pga_amp1_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1];

	pr_aud("%s = %d\n", __func__, gain);
	ucontrol->value.integer.value[0] = gain;
	return 0;
}

static int audio_pga_amp1_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = ucontrol->value.integer.value[0];

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	Ana_Set_Reg(AUDENC_ANA_CON0, (gain << 8), 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = gain;
	return 0;
}

/* PGA2: ADC2 MIC GAIN */
static int audio_pga_amp2_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2];

	pr_aud("%s = %d\n", __func__, gain);
	ucontrol->value.integer.value[0] = gain;
	return 0;
}

static int audio_pga_amp2_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = ucontrol->value.integer.value[0];

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	Ana_Set_Reg(AUDENC_ANA_CON1, gain << 8, 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = gain;
	return 0;
}

/* PGA3: ADC3 MIC GAIN */
static int audio_pga_amp3_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3];

	pr_aud("%s = %d\n", __func__, gain);
	ucontrol->value.integer.value[0] = gain;
	return 0;
}

static int audio_pga_amp3_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = ucontrol->value.integer.value[0];

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	Ana_Set_Reg(AUDENC_ANA_CON2, (gain << 8), 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = gain;
	return 0;
}

/* PGA4: ADC4 MIC GAIN */
static int audio_pga_amp4_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4];

	pr_aud("%s = %d\n", __func__, gain);
	ucontrol->value.integer.value[0] = gain;
	return 0;
}

static int audio_pga_amp4_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int gain = ucontrol->value.integer.value[0];

	pr_aud("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ADC_UL_PGA_GAIN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	Ana_Set_Reg(AUDENC_ANA_CON3, (gain << 8), 0x0700);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = gain;
	return 0;
}

static int audio_adc1_mic_source_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int adc1_mic = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];

	switch (adc1_mic) {
	case AUDIO_AMIC_AIN0:
		ucontrol->value.integer.value[0] = 0;
		break;
	case AUDIO_AMIC_AIN1:
		ucontrol->value.integer.value[0] = 1;
		break;
	case AUDIO_AMIC_AIN2:
		ucontrol->value.integer.value[0] = 2;
		break;
	default:
		pr_warn("%s adc1 not support this mic source = %d\n", __func__, adc1_mic);
		break;
	}

	pr_aud("adc1_mic_source = %d\n", adc1_mic);
	return 0;
}

static int audio_adc1_mic_source_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];
	int *adc1_mic = &mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1];

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc1_mic_source)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	switch (index) {
	case 0:
		*adc1_mic = AUDIO_AMIC_AIN0;
		break;
	case 1:
		*adc1_mic = AUDIO_AMIC_AIN1;
		break;
	case 2:
		*adc1_mic = AUDIO_AMIC_AIN2;
		break;
	default:
		pr_warn("%s adc1 not support this index = %d\n", __func__, index);
		break;
	}
	pr_aud("%s() index = %d mic_source = %d\n", __func__, index, *adc1_mic);

	return 0;
}

static int audio_adc2_mic_source_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int adc2_mic = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2];

	switch (adc2_mic) {
	case AUDIO_AMIC_AIN0:
		ucontrol->value.integer.value[0] = 0;
		break;
	case AUDIO_AMIC_AIN3:
		ucontrol->value.integer.value[0] = 1;
		break;
	case AUDIO_AMIC_AIN2:
		ucontrol->value.integer.value[0] = 2;
		break;
	default:
		pr_warn("%s adc2 not support this mic source = %d\n", __func__, adc2_mic);
		break;
	}

	pr_aud("adc2_mic_source = %d\n", adc2_mic);
	return 0;
}

static int audio_adc2_mic_source_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];
	int *adc2_mic = &mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2];

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc2_mic_source)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	switch (index) {
	case 0:
		*adc2_mic = AUDIO_AMIC_AIN0;
		break;
	case 1:
		*adc2_mic = AUDIO_AMIC_AIN3;
		break;
	case 2:
		*adc2_mic = AUDIO_AMIC_AIN2;
		break;
	default:
		pr_warn("%s adc2 not support this index = %d\n", __func__, index);
		break;
	}
	pr_aud("%s() index = %d mic_source = %d\n", __func__, index, *adc2_mic);

	return 0;
}

static int audio_adc3_mic_source_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int adc3_mic = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3];

	switch (adc3_mic) {
	case AUDIO_AMIC_AIN5:
		ucontrol->value.integer.value[0] = 0;
		break;
	case AUDIO_AMIC_AIN4:
		ucontrol->value.integer.value[0] = 1;
		break;
	case AUDIO_AMIC_AIN6:
		ucontrol->value.integer.value[0] = 2;
		break;
	default:
		pr_warn("%s adc3 not support this mic source = %d\n", __func__, adc3_mic);
		break;
	}

	pr_aud("adc3_mic_source = %d\n", adc3_mic);
	return 0;
}

static int audio_adc3_mic_source_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];
	int *adc3_mic = &mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3];

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc3_mic_source)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	switch (index) {
	case 0:
		*adc3_mic = AUDIO_AMIC_AIN5;
		break;
	case 1:
		*adc3_mic = AUDIO_AMIC_AIN4;
		break;
	case 2:
		*adc3_mic = AUDIO_AMIC_AIN6;
		break;
	default:
		pr_warn("%s adc3 not support this index = %d\n", __func__, index);
		break;
	}
	pr_aud("%s() index = %d mic_source = %d\n", __func__, index, *adc3_mic);

	return 0;
}


static int audio_adc4_mic_source_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int adc4_mic = mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4];

	switch (adc4_mic) {
	case AUDIO_AMIC_AIN5:
		ucontrol->value.integer.value[0] = 0;
		break;
	case AUDIO_AMIC_AIN0:
		ucontrol->value.integer.value[0] = 1;
		break;
	case AUDIO_AMIC_AIN6:
		ucontrol->value.integer.value[0] = 2;
		break;
	default:
		pr_warn("%s adc4 not support this mic source = %d\n", __func__, adc4_mic);
		break;
	}

	pr_aud("adc4_mic_source = %d\n", adc4_mic);
	return 0;
}

static int audio_adc4_mic_source_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];
	int *adc4_mic = &mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4];

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc4_mic_source)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	switch (index) {
	case 0:
		*adc4_mic = AUDIO_AMIC_AIN5;
		break;
	case 1:
		*adc4_mic = AUDIO_AMIC_AIN0;
		break;
	case 2:
		*adc4_mic = AUDIO_AMIC_AIN6;
		break;
	default:
		pr_warn("%s adc4 not support this index = %d\n", __func__, index);
		break;
	}
	pr_aud("%s() index = %d mic_source = %d\n", __func__, index, *adc4_mic);

	return 0;
}

/* ADC MIC Mode Setting */
static int audio_adc1_mic_mode_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() mic_mode = %d\n", __func__, adc1_mic_mode_mux);
	ucontrol->value.integer.value[0] = adc1_mic_mode_mux;
	return 0;
}

static int audio_adc1_mic_mode_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc_mic_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	adc1_mic_mode_mux = ucontrol->value.integer.value[0];
	pr_aud("%s() mic_mode = %d\n", __func__, adc1_mic_mode_mux);
	return 0;
}

static int audio_adc2_mic_mode_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() mic_mode = %d\n", __func__, adc2_mic_mode_mux);
	ucontrol->value.integer.value[0] = adc2_mic_mode_mux;
	return 0;
}

static int audio_adc2_mic_mode_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc_mic_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	adc2_mic_mode_mux = ucontrol->value.integer.value[0];
	pr_aud("%s() mic_mode = %d\n", __func__, adc2_mic_mode_mux);
	return 0;
}


static int audio_adc3_mic_mode_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() mic_mode = %d\n", __func__, adc3_mic_mode_mux);
	ucontrol->value.integer.value[0] = adc3_mic_mode_mux;
	return 0;
}

static int audio_adc3_mic_mode_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc_mic_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	adc3_mic_mode_mux = ucontrol->value.integer.value[0];
	pr_aud("%s() mic_mode = %d\n", __func__, adc3_mic_mode_mux);
	return 0;
}

static int audio_adc4_mic_mode_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_aud("%s() mic_mode = %d\n", __func__, adc4_mic_mode_mux);
	ucontrol->value.integer.value[0] = adc4_mic_mode_mux;
	return 0;
}

static int audio_adc4_mic_mode_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_adc_mic_mode)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	adc4_mic_mode_mux = ucontrol->value.integer.value[0];
	pr_aud("%s() mic_mode = %d\n", __func__, adc4_mic_mode_mux);
	return 0;
}

static int audio_ul1_lch_in_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int lch_in = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX];

	pr_aud("%s() lch_in = %d\n", __func__, lch_in);
	ucontrol->value.integer.value[0] = lch_in;
	return 0;
}

static int audio_ul1_lch_in_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int *lch_in = &mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX];
	int device = 0;

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_ul_array_in)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	*lch_in = ucontrol->value.integer.value[0];
	device = channel_map_to_device(*lch_in);

	if (is_amic(device))
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x0003);
	else if (is_dmic(device))
		Ana_Set_Reg(AFE_DMIC_ARRAY_CFG, dmic_array_status(), 0x0007);
	else
		pr_warn("%s is not uplink device %d", __func__, device);

	pr_aud("%s() lch_in = %d\n", __func__, *lch_in);
	return 0;
}

static int audio_ul1_rch_in_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int rch_in = mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX];

	pr_aud("%s() rch_in = %d\n", __func__, rch_in);
	ucontrol->value.integer.value[0] = rch_in;
	return 0;
}

static int audio_ul1_rch_in_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int *rch_in = &mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX];
	int device = 0;

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_ul_array_in)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	*rch_in = ucontrol->value.integer.value[0];
	device = channel_map_to_device(*rch_in);

	if (is_amic(device))
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x000c);
	else if (is_dmic(device))
		Ana_Set_Reg(AFE_DMIC_ARRAY_CFG, dmic_array_status(), 0x0070);
	else
		pr_warn("%s is not uplink device %d", __func__, device);

	pr_aud("%s() rch_in = %d\n", __func__, *rch_in);
	return 0;
}

static int audio_ul2_lch_in_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int lch_in = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX];

	pr_aud("%s() lch_in = %d\n", __func__, lch_in);
	ucontrol->value.integer.value[0] = lch_in;
	return 0;
}

static int audio_ul2_lch_in_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int *lch_in = &mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX];
	int device = 0;

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_ul_array_in)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	*lch_in = ucontrol->value.integer.value[0];
	device = channel_map_to_device(*lch_in);

	if (is_amic(device))
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x0030);
	else if (is_dmic(device))
		Ana_Set_Reg(AFE_DMIC_ARRAY_CFG, dmic_array_status(), 0x0700);
	else
		pr_warn("%s is not uplink device %d", __func__, device);

	pr_aud("%s() lch_in = %d\n", __func__, *lch_in);
	return 0;
}

static int audio_ul2_rch_in_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int rch_in = mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX];

	pr_aud("%s() rch_in = %d\n", __func__, rch_in);
	ucontrol->value.integer.value[0] = rch_in;
	return 0;
}

static int audio_ul2_rch_in_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int *rch_in = &mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX];
	int device = 0;

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_ul_array_in)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	*rch_in = ucontrol->value.integer.value[0];
	device = channel_map_to_device(*rch_in);

	if (is_amic(device))
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, amic_array_status(), 0x00c0);
	else if (is_dmic(device))
		Ana_Set_Reg(AFE_DMIC_ARRAY_CFG, dmic_array_status(), 0x7000);
	else
		pr_warn("%s is not uplink device %d", __func__, device);

	pr_aud("%s() rch_in = %d\n", __func__, *rch_in);
	return 0;
}

static int Audio_Vow_ADC_Func_Switch_Get(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_Vow_Analog_Func_Enable);
	ucontrol->value.integer.value[0] = mAudio_Vow_Analog_Func_Enable;
	return 0;
}

static int Audio_Vow_ADC_Func_Switch_Set(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_ADC_Function)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0])
		TurnOnVOWADcPower(mAudio_VOW_Mic_type, true);
	else
		TurnOnVOWADcPower(mAudio_VOW_Mic_type, false);


	mAudio_Vow_Analog_Func_Enable = ucontrol->value.integer.value[0];

	return 0;
}

static int Audio_Vow_Digital_Func_Switch_Get(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_Vow_Digital_Func_Enable);
	ucontrol->value.integer.value[0] = mAudio_Vow_Digital_Func_Enable;
	return 0;
}

static int Audio_Vow_Digital_Func_Switch_Set(struct snd_kcontrol *kcontrol,
					     struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_Digital_Function)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0])
		TurnOnVOWDigitalHW(true);
	else
		TurnOnVOWDigitalHW(false);


	mAudio_Vow_Digital_Func_Enable = ucontrol->value.integer.value[0];

	return 0;
}


static int Audio_Vow_MIC_Type_Select_Get(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, mAudio_VOW_Mic_type);
	ucontrol->value.integer.value[0] = mAudio_VOW_Mic_type;
	return 0;
}

static int Audio_Vow_MIC_Type_Select_Set(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_VOW_MIC_Type)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_VOW_Mic_type = ucontrol->value.integer.value[0];
	pr_debug("%s() mAudio_VOW_Mic_type = %d\n", __func__, mAudio_VOW_Mic_type);
#ifdef CONFIG_MTK_VOW_SUPPORT
	if (mAudio_VOW_Mic_type == AUDIO_VOW_MIC_TYPE_Handset_DMIC_VENDOR01)
		VowDrv_SetSmartDevice(true);
	else
		VowDrv_SetSmartDevice(false);
#endif
	return 0;
}


static int Audio_Vow_Cfg0_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG0;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg0_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %d\n", __func__, (int)(ucontrol->value.integer.value[0]));
	reg_AFE_VOW_CFG0 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg1_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG1;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg1_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);

	reg_AFE_VOW_CFG1 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg2_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG2;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg2_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_CFG2 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg3_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG3;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg3_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_CFG3 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg4_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG4;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg4_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_CFG4 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg5_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG5;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg5_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_CFG5 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_Cfg6_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_CFG6;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Cfg6_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_CFG6 = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Vow_State_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = mIsVOWOn;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_State_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	/* pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]); */
	/* reg_AFE_VOW_CFG5 = ucontrol->value.integer.value[0]; */
	return 0;
}

static int Audio_Vow_Periodic_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int value = reg_AFE_VOW_PERIODIC;

	pr_debug("%s()  = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int Audio_Vow_Periodic_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()  = %ld\n", __func__, ucontrol->value.integer.value[0]);
	reg_AFE_VOW_PERIODIC = ucontrol->value.integer.value[0];
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

static int Pmic_Loopback_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = Pmic_Loopback_Type;
	return 0;
}

static int Pmic_Loopback_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	unsigned int dl_rate = mBlockSampleRate[AUDIO_DAI_DL1];
	unsigned int ul_rate = mBlockSampleRate[AUDIO_DAI_UL1];

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_LPBK_function)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 0) { /* disable pmic lpbk */
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0x0005); /* power off uplink */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0000, 0x0001); /* turn off mute function and turn on dl */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0x0001);   /* turn off afe UL & DL */
		Topck_Enable(false);
		ClsqEnable(false);
		audckbufEnable(false);
	} else if (ucontrol->value.integer.value[0] > 0) { /* enable pmic lpbk */
		audckbufEnable(true);
		ClsqEnable(true);
		Topck_Enable(true);

		pr_warn("set PMIC LPBK3, DLSR=%u, ULSR=%u\n", dl_rate, ul_rate);

		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x8000, 0xffff);	/* power on clock */

		/* Se DL Part */
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0x009f);
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xC3A1, 0x7381);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0x009f);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x000B, 0x009f);
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001D, 0x007f);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG1, 0x0018, 0x03ff);
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xC001);

		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0,
			    ((GetDLNewIFFrequency(dl_rate) << 12) |
			     0x330), 0xfff0);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H,
			    ((GetDLNewIFFrequency(dl_rate) << 12) |
			     0x300), 0xf300);

		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG2, 0x8000, 0x8000);
		/* set sck inverse in dac newif  */
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0x0001);
		/* turn on dl */
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		/* set DL in normal path, not from sine gen table */

		/* Set UL Part */
		/* set UL sample rate */
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xfff0);
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, (ULSampleRateTransform(ul_rate) << 1),
			    0x7 << 1);
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0005, 0xffff);   /* power on uplink, and loopback from DL */
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);   /* turn on afe */
	}

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
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Pmic_Test_function)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	Voice_Call_DAC_DAC_HS_flag = ucontrol->value.integer.value[0];
	pr_aud("%s() Hardcode flag= %d\n", __func__, Voice_Call_DAC_DAC_HS_flag);
	if (Voice_Call_DAC_DAC_HS_flag == 1) {
		pr_aud("%s() Hardcode for voice phone call 16k\n", __func__);
		/* Digital */
		Ana_Set_Reg(LDO_VA25_CON0, 0x0001, 0xffff);
		Ana_Set_Reg(LDO_VA25_OP_EN, 0x0001, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x8000, 0xffff);
		Ana_Set_Reg(TOP_CLKSQ_SET, 0x0001, 0xffff);
		Ana_Set_Reg(TOP_CKPDN_CON0_CLR, 0x1800, 0xffff);
		udelay(250);
		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x8000, 0xffff);
		Ana_Set_Reg(AFE_NCP_CFG1, 0x1515, 0xffff);
		Ana_Set_Reg(AFE_NCP_CFG0, 0xC801, 0xffff);
		udelay(250);
		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0x8000, 0xffff);
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, 0x00e4, 0xffff);
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG1, 0x0018, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0006, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xC3A1, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0003, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x000B, 0xffff);
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x001D, 0xffff);
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0001, 0xffff);
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0002, 0xffff);
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0001, 0xffff);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x3330, 0xffff);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x3300, 0xffff);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0001, 0xffff);
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff);
		/* Analog */
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0xC000, 0xffff);
		udelay(100);
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x002D, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0003, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0280, 0xffff);
		udelay(100);
		Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0010, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0x2A80, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0090, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0092, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0093, 0xffff);
		Ana_Set_Reg(ZCD_CON3, 0x0009, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0281, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0009, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x8000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x009B, 0xffff);

		Ana_Set_Reg(TOP_CLKSQ, 0x0801, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON6, 0x0040, 0xffff);
		Ana_Set_Reg(TOP_CKPDN_CON0, 0x26E0, 0xffff);
		Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0FE3, 0xffff);
		Ana_Set_Reg(AFE_DCCLK_CFG0, 0x2061, 0xffff);
		Ana_Set_Reg(AFE_DCCLK_CFG1, 0x1105, 0xffff);

		Ana_Set_Reg(AUDENC_ANA_CON16, 0x0021, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0317, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x5317, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x53DF, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON16, 0x2121, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON6, 0x0240, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0337, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x5337, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x53FF, 0xffff);
		udelay(100);
		Ana_Set_Reg(AUDENC_ANA_CON0, 0x53DB, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x53FB, 0xffff);
	} else {
		pr_aud("%s() Hardcode for pmic reset\n", __func__);
		Ana_Set_Reg(AUDDEC_ANA_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON1, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON2, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON3, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON4, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON5, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON6, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON7, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON8, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON9, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON10, 0x4900, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON11, 0xaa80, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON12, 0x0010, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON13, 0x0000, 0xffff);
		Ana_Set_Reg(AUDDEC_ANA_CON14, 0x0000, 0xffff);

		Ana_Set_Reg(AUDENC_ANA_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON1, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON2, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON3, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON4, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON5, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON6, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON7, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON8, 0x0800, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON9, 0x0800, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON10, 0x1515, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON11, 0x1515, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON12, 0x0000, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON13, 0x0004, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON14, 0x0004, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON15, 0x0004, 0xffff);
		Ana_Set_Reg(AUDENC_ANA_CON16, 0x0000, 0xffff);

		Ana_Set_Reg(ZCD_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(ZCD_CON1, 0x0f9f, 0xffff);
		Ana_Set_Reg(ZCD_CON2, 0x0f9f, 0xffff);
		Ana_Set_Reg(ZCD_CON3, 0x001f, 0xffff);
		Ana_Set_Reg(ZCD_CON4, 0x0707, 0xffff);
		Ana_Set_Reg(ZCD_CON5, 0x3f3f, 0xffff);

		Ana_Set_Reg(AFE_AUDIO_TOP_CON0, 0xc000, 0xffff);
		Ana_Set_Reg(AFE_NCP_CFG1, 0x1515, 0xffff);
		Ana_Set_Reg(AFE_NCP_CFG0, 0x4600, 0xffff);
		Ana_Set_Reg(AFE_AMIC_ARRAY_CFG, 0x00e4, 0xffff);
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG1, 0x0018, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON2, 0x0000, 0xffff);
		Ana_Set_Reg(AFUNC_AUD_CON0, 0xd021, 0xffff);
		Ana_Set_Reg(AFE_DL_SDM_CON1, 0x003a, 0xffff);
		Ana_Set_Reg(AFE_UL_DL_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_UL_SRC_CON0_H, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_UL_SRC_CON0_L, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_PMIC_NEWIF_CFG0, 0x9330, 0xffff);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_H, 0x0300, 0xffff);
		Ana_Set_Reg(AFE_DL_SRC2_CON0_L, 0x0000, 0xffff);
		Ana_Set_Reg(PMIC_AFE_TOP_CON0, 0x0000, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG0, 0x0008, 0xffff);
		Ana_Set_Reg(AFE_SGEN_CFG1, 0x0101, 0xffff);
		Ana_Set_Reg(AFE_DCCLK_CFG0, 0x0fe2, 0xffff);
		Ana_Set_Reg(AFE_DCCLK_CFG1, 0x1105, 0xffff);
	}
	return 0;
}

static const struct soc_enum Pmic_Test_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_Test_function), Pmic_Test_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Pmic_LPBK_function), Pmic_LPBK_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dctrim_control_state), dctrim_control_state)
};

static const struct snd_kcontrol_new mt6331_pmic_Test_controls[] = {
	SOC_ENUM_EXT("SineTable_DAC_HP", Pmic_Test_Enum[0], SineTable_DAC_HP_Get,
		     SineTable_DAC_HP_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HS", Pmic_Test_Enum[0], ADC_LOOP_DAC_HS_Get,
		     ADC_LOOP_DAC_HS_Set),
	SOC_ENUM_EXT("DAC_LOOP_DAC_HP", Pmic_Test_Enum[0], ADC_LOOP_DAC_HP_Get,
		     ADC_LOOP_DAC_HP_Set),
	SOC_ENUM_EXT("Voice_Call_DAC_DAC_HS", Pmic_Test_Enum[0], Voice_Call_DAC_DAC_HS_Get,
		     Voice_Call_DAC_DAC_HS_Set),
	SOC_ENUM_EXT("SineTable_UL2", Pmic_Test_Enum[0], SineTable_UL2_Get, SineTable_UL2_Set),
	SOC_ENUM_EXT("Pmic_Loopback", Pmic_Test_Enum[1], Pmic_Loopback_Get, Pmic_Loopback_Set),
	SOC_ENUM_EXT("Dctrim_Control_Switch", Pmic_Test_Enum[2],
		     pmic_dctrim_control_get, pmic_dctrim_control_set),
	SOC_DOUBLE_EXT("DcTrim_DC_Offset", SND_SOC_NOPM, 0, 1, 0x20000, 0,
		       pmic_dc_offset_get, pmic_dc_offset_set),
	SOC_DOUBLE_EXT("DcTrim_Hp_Trimcode", SND_SOC_NOPM, 0, 1, 0x20000, 0,
		       pmic_hp_trim_get, pmic_hp_trim_set),
	SOC_SINGLE_EXT("Audio HP ImpeDance Setting", SND_SOC_NOPM, 0, 0x10000, 0,
		       pmic_hp_impedance_get, pmic_hp_impedance_set),
};

static const struct snd_kcontrol_new mt6331_UL_Codec_controls[] = {
	SOC_ENUM_EXT("Audio_Capture1_Switch", Audio_UL_Enum[0],
		     audio_capture1_get, audio_capture1_set),
	SOC_ENUM_EXT("Audio_Capture2_Switch", Audio_UL_Enum[0],
		     audio_capture2_get, audio_capture2_set),
	SOC_ENUM_EXT("Audio_UL1_Lch_Mux", Audio_UL_Enum[1],
		     audio_ul1_lch_in_get, audio_ul1_lch_in_set),
	SOC_ENUM_EXT("Audio_UL1_Rch_Mux", Audio_UL_Enum[1],
		     audio_ul1_rch_in_get, audio_ul1_rch_in_set),
	SOC_ENUM_EXT("Audio_UL2_Lch_Mux", Audio_UL_Enum[1],
		     audio_ul2_lch_in_get, audio_ul2_lch_in_set),
	SOC_ENUM_EXT("Audio_UL2_Rch_Mux", Audio_UL_Enum[1],
		     audio_ul2_rch_in_get, audio_ul2_rch_in_set),
	SOC_ENUM_EXT("Audio_MicSource1_Setting", Audio_UL_Enum[2],
		     audio_adc1_mic_source_get, audio_adc1_mic_source_set),
	SOC_ENUM_EXT("Audio_MicSource2_Setting", Audio_UL_Enum[3],
		     audio_adc2_mic_source_get, audio_adc2_mic_source_set),
	SOC_ENUM_EXT("Audio_MicSource3_Setting", Audio_UL_Enum[4],
		     audio_adc3_mic_source_get, audio_adc3_mic_source_set),
	SOC_ENUM_EXT("Audio_MicSource4_Setting", Audio_UL_Enum[5],
		     audio_adc4_mic_source_get, audio_adc4_mic_source_set),
	SOC_ENUM_EXT("Audio_PGA1_Setting", Audio_UL_Enum[6],
		     audio_pga_amp1_get, audio_pga_amp1_set),
	SOC_ENUM_EXT("Audio_PGA2_Setting", Audio_UL_Enum[6],
		     audio_pga_amp2_get, audio_pga_amp2_set),
	SOC_ENUM_EXT("Audio_PGA3_Setting", Audio_UL_Enum[6],
		     audio_pga_amp3_get, audio_pga_amp3_set),
	SOC_ENUM_EXT("Audio_PGA4_Setting", Audio_UL_Enum[6],
		     audio_pga_amp4_get, audio_pga_amp4_set),
	SOC_ENUM_EXT("Audio_MIC1_Mode_Select", Audio_UL_Enum[7],
		     audio_adc1_mic_mode_get, audio_adc1_mic_mode_set),
	SOC_ENUM_EXT("Audio_MIC2_Mode_Select", Audio_UL_Enum[7],
		     audio_adc2_mic_mode_get, audio_adc2_mic_mode_set),
	SOC_ENUM_EXT("Audio_MIC3_Mode_Select", Audio_UL_Enum[7],
		     audio_adc3_mic_mode_get, audio_adc3_mic_mode_set),
	SOC_ENUM_EXT("Audio_MIC4_Mode_Select", Audio_UL_Enum[7],
		     audio_adc4_mic_mode_get, audio_adc4_mic_mode_set),
	SOC_ENUM_EXT("Audio_Vow_ADC_Func_Switch", Audio_UL_Enum[8],
		     Audio_Vow_ADC_Func_Switch_Get,
		     Audio_Vow_ADC_Func_Switch_Set),
	SOC_ENUM_EXT("Audio_Vow_Digital_Func_Switch", Audio_UL_Enum[9],
		     Audio_Vow_Digital_Func_Switch_Get,
		     Audio_Vow_Digital_Func_Switch_Set),
	SOC_ENUM_EXT("Audio_Vow_MIC_Type_Select", Audio_UL_Enum[10],
		     Audio_Vow_MIC_Type_Select_Get,
		     Audio_Vow_MIC_Type_Select_Set),
	SOC_SINGLE_EXT("Audio VOWCFG0 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg0_Get,
		       Audio_Vow_Cfg0_Set),
	SOC_SINGLE_EXT("Audio VOWCFG1 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg1_Get,
		       Audio_Vow_Cfg1_Set),
	SOC_SINGLE_EXT("Audio VOWCFG2 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg2_Get,
		       Audio_Vow_Cfg2_Set),
	SOC_SINGLE_EXT("Audio VOWCFG3 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg3_Get,
		       Audio_Vow_Cfg3_Set),
	SOC_SINGLE_EXT("Audio VOWCFG4 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg4_Get,
		       Audio_Vow_Cfg4_Set),
	SOC_SINGLE_EXT("Audio VOWCFG5 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg5_Get,
		       Audio_Vow_Cfg5_Set),
	SOC_SINGLE_EXT("Audio VOWCFG6 Data", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Cfg6_Get,
		       Audio_Vow_Cfg6_Set),
	SOC_SINGLE_EXT("Audio_VOW_State", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_State_Get,
		       Audio_Vow_State_Set),
	SOC_SINGLE_EXT("Audio_VOW_Periodic", SND_SOC_NOPM, 0, 0x80000, 0,
		       Audio_Vow_Periodic_Get,
		       Audio_Vow_Periodic_Set),
	SOC_ENUM_EXT("Audio_UL_LR_Swap", Audio_UL_Enum[0],
		     Audio_UL_LR_Swap_Get,
		     Audio_UL_LR_Swap_Set),
};

static int read_efuse_hp_impedance_current_calibration(void)
{
	int value = 0, sign = 0;

	sign  = (Ana_Get_Reg(OTP_DOUT_256_271) >> 7) & 0x1;
	value = Ana_Get_Reg(OTP_DOUT_256_271) & 0x7f;
	value = sign ? -value : value;

	pr_aud("%s(), EFUSE: %d\n", __func__, value);
	return value;
}

static const struct snd_soc_dapm_widget mt6331_dapm_widgets[] = {
	/* Outputs */
	SND_SOC_DAPM_OUTPUT("EARPIECE"),
	SND_SOC_DAPM_OUTPUT("HEADSET"),
	SND_SOC_DAPM_OUTPUT("SPEAKER"),
	/* SND_SOC_DAPM_MUX_E("VOICE_Mux_E", SND_SOC_NOPM, 0, 0  , &mt6331_Voice_Switch, codec_enable_rx_bias, */
	/* SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD |*/
	/* SND_SOC_DAPM_PRE_REG | SND_SOC_DAPM_POST_REG), */

};

static const struct snd_soc_dapm_route mtk_audio_map[] = {
	{"VOICE_Mux_E", "Voice Mux", "SPEAKER PGA"},
};

#define WAIT_NLE_TARGET_COUNT (25)
#define AUDIO_NLE_CHANNEL_L (0)
#define AUDIO_NLE_CHANNEL_R (1)
#define NLE_ANA_GAIN_MAX_DB (12)
#define NLE_ANA_GAIN_MIN_DB (-32)
#define NLE_ANA_GAIN_MIN_VALUE (0x002C)
#define NLE_ANA_GAIN_MUTE_DB (-40)
#define NLE_ANA_GAIN_MUTE_VALUE (0x003F)
#define NLE_DIG_GAIN_MAX_DB (44)
#define NLE_DIG_GAIN_MAX_VALUE (44)
#define NLE_DIG_GAIN_MIN_DB (0)
#define NLE_DELAY_ANA_OUTPUT_T (0x0D) /* User could change it by setting Audio_HyBridNLE_TurnOn or Off */


static int Audio_HyBridNLE_getAnalogIdx(int32 Db, uint32 *pAnalogIdx)
{
	uint32 mAnalogIdx;

	if (pAnalogIdx == NULL) {
		pr_err("[Err] %s -1", __func__);
		return -1;
	}

	if (Db > NLE_ANA_GAIN_MAX_DB || (Db < NLE_ANA_GAIN_MIN_DB && Db != NLE_ANA_GAIN_MUTE_DB)) {
		pr_err("[Err] %s -2 Db %d Reserved", __func__, Db);
		return -2;
	}

	if (Db == NLE_ANA_GAIN_MUTE_DB)
		mAnalogIdx = NLE_ANA_GAIN_MUTE_VALUE;
	else
		mAnalogIdx = (uint32) (NLE_ANA_GAIN_MAX_DB - Db);

	*pAnalogIdx = mAnalogIdx;

	return 0;
}

static int Audio_HyBridNLE_getDbFromDigitalIdx(uint32 DigitalIdx, int32 *pDb)
{
	int32 mDb;

	if (pDb == NULL) {
		pr_err("[Err] %s -1", __func__);
		return -1;
	}

	if (DigitalIdx > NLE_DIG_GAIN_MAX_VALUE) {
		pr_err("[Err] %s -2 DigitalIdx %x Reserved", __func__, DigitalIdx);
		return -2;
	}

	mDb =  (int32) DigitalIdx;

	*pDb = mDb;

	return 0;
}

static int Audio_HyBridNLE_getDigitalIdx(int32 Db, uint32 *pDigitalIdx)
{
	uint32 mDigitalIdx;

	if (pDigitalIdx == NULL) {
		pr_err("[Err] %s -1", __func__);
		return -1;
	}

	if (Db < NLE_DIG_GAIN_MIN_DB || Db > NLE_DIG_GAIN_MAX_DB) {
		pr_err("[Err] %s -2 Db %d Reserved", __func__, Db);
		return -2;
	}

	mDigitalIdx = (uint32) Db;

	*pDigitalIdx = mDigitalIdx;

	return 0;
}

static void Audio_NLE_RegDump(void)
{
	pr_warn("AFE_DL_NLE_R_CFG0	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG0));
	pr_warn("AFE_DL_NLE_R_CFG1	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG1));
	pr_warn("AFE_DL_NLE_R_CFG2	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG2));
	pr_warn("AFE_DL_NLE_R_CFG3	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_R_CFG3));
	pr_warn("AFE_DL_NLE_L_CFG0	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG0));
	pr_warn("AFE_DL_NLE_L_CFG1	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG1));
	pr_warn("AFE_DL_NLE_L_CFG2	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG2));
	pr_warn("AFE_DL_NLE_L_CFG3	= 0x%x\n", Ana_Get_Reg(AFE_DL_NLE_L_CFG3));

	pr_warn("AFE_RGS_NLE_R_CFG0	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG0));
	pr_warn("AFE_RGS_NLE_R_CFG1	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG1));
	pr_warn("AFE_RGS_NLE_R_CFG2	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG2));
	pr_warn("AFE_RGS_NLE_R_CFG3	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_R_CFG3));
	pr_warn("AFE_RGS_NLE_L_CFG0	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG0));
	pr_warn("AFE_RGS_NLE_L_CFG1	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG1));
	pr_warn("AFE_RGS_NLE_L_CFG2	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG2));
	pr_warn("AFE_RGS_NLE_L_CFG3	= 0x%x\n", Ana_Get_Reg(AFE_RGS_NLE_L_CFG3));
}

static int Audio_HyBridNLE_TurnOn_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_TurnOn_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	uint32 rg_nle_l_gain_dig_tar, rg_nle_r_gain_dig_tar;
	uint32 rg_nle_l_initiate, rg_nle_r_initiate;
	uint32 rg_temp_value;
	int32 para_temp_value;
	uint32 rg_nle_gain_ana_tar_idx;
	unsigned char rg_nle_delay_ana = ucontrol->value.bytes.data[0];
	uint32 rg_nle_delay_ana_idx;
	int oldindex = mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL];

	if (ucontrol->value.bytes.data[1] == 1)
		para_temp_value = ((int32) ucontrol->value.bytes.data[2]) * (-1);
	else
		para_temp_value = (int32) ucontrol->value.bytes.data[2];

	if (rg_nle_delay_ana == 0 || para_temp_value > NLE_ANA_GAIN_MAX_DB ||
		(para_temp_value < NLE_ANA_GAIN_MIN_DB && para_temp_value != NLE_ANA_GAIN_MUTE_DB)) {
		pr_err("%s Parameter invalid -10\n", __func__);
		pr_err("ana_delay [%d] ana_db [%d]\n", rg_nle_delay_ana, para_temp_value);
		return -10;
	}
	rg_nle_delay_ana_idx = ((uint32) rg_nle_delay_ana) - 1;
	if (Audio_HyBridNLE_getAnalogIdx(para_temp_value, &rg_nle_gain_ana_tar_idx) < 0) {
		pr_err("%s -1\n", __func__);
		return -1;
	}
	pr_aud("%s %d %d enter\n", __func__, rg_nle_delay_ana_idx, rg_nle_gain_ana_tar_idx);
	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG3);
	if ((rg_temp_value & 0x00C0) != 0x40) {
		pr_err("%s R Err rg_temp_value 0x%x\n", __func__, rg_temp_value);
		Audio_NLE_RegDump();
		return -2;
	}

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG3);
	if ((rg_temp_value & 0x0080) != 0) {
		pr_err("%s L Err rg_temp_value 0x%x\n", __func__, rg_temp_value);
		Audio_NLE_RegDump();
		return -3;
	}

	rg_nle_r_gain_dig_tar = Ana_Get_Reg(AFE_DL_NLE_R_CFG0) & 0x3f00;
	rg_nle_l_gain_dig_tar = Ana_Get_Reg(AFE_DL_NLE_L_CFG0) & 0x3f00;
	if (rg_nle_r_gain_dig_tar > 0 || rg_nle_l_gain_dig_tar > 0) {
		/* It should be zero */
		pr_err("%s Err %d %d\n", __func__, rg_nle_r_gain_dig_tar, rg_nle_l_gain_dig_tar);
		Audio_NLE_RegDump();
	}
	headset_volume_ramp(oldindex, rg_nle_gain_ana_tar_idx);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = (int32) rg_nle_gain_ana_tar_idx;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = (int32) rg_nle_gain_ana_tar_idx;
	Ana_Set_Reg(AFE_DL_NLE_R_CFG0, rg_nle_gain_ana_tar_idx, 0x3f3f);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG0, rg_nle_gain_ana_tar_idx, 0x3f3f);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG3, (0 << 6) | rg_nle_delay_ana_idx, 0x7f);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG3, rg_nle_delay_ana_idx, 0x3f);
	rg_nle_r_initiate = (Ana_Get_Reg(AFE_DL_NLE_R_CFG2) & 0x0100);
	rg_nle_l_initiate = (Ana_Get_Reg(AFE_DL_NLE_L_CFG2) & 0x0100);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG2, rg_nle_r_initiate?0:0x100, 0x0100);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG2, rg_nle_l_initiate?0:0x100, 0x0100);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG3, (1<<7), 0x80);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG3, (1<<7), 0x80);
	pr_aud("%s exit\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_TurnOff_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_TurnOff_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	unsigned char rg_nle_delay_ana = ucontrol->value.bytes.data[0];
	int32 para_temp_value;
	uint32 rg_nle_gain_ana_tar_idx;
	uint32 rg_temp_value;
	uint32 rg_nle_r_gain_dig_tar, rg_nle_r_gain_ana_tar;
	uint32 nle_r_gain_dig_cur, nle_r_gain_ana_cur;
	uint32 nle_r_dig_gain_targeted, nle_r_ana_gain_targeted;
	uint32 rg_nle_l_gain_dig_tar, rg_nle_l_gain_ana_tar;
	uint32 nle_l_gain_dig_cur, nle_l_gain_ana_cur;
	uint32 nle_l_dig_gain_targeted, nle_l_ana_gain_targeted;
	bool bLNoZCEEnable = false;
	bool bRNoZCEEnable = false;
	bool bSleepFlg = false;
	uint32 dCount = 0;
	uint32 rg_nle_delay_ana_idx;
	bool bLTargeted = false;
	bool bRTargeted = false;

	if (ucontrol->value.bytes.data[1] == 1)
		para_temp_value = ((int32) ucontrol->value.bytes.data[2]) * (-1);
	else
		para_temp_value = (int32) ucontrol->value.bytes.data[2];

	if (rg_nle_delay_ana == 0 || para_temp_value > NLE_ANA_GAIN_MAX_DB ||
		(para_temp_value < NLE_ANA_GAIN_MIN_DB && para_temp_value != NLE_ANA_GAIN_MUTE_DB)) {
		pr_err("%s Parameter invalid -10\n", __func__);
		pr_err("ana_delay [%d] ana_db [%d]\n", rg_nle_delay_ana, para_temp_value);
		return -10;
	}
	rg_nle_delay_ana_idx = ((uint32)rg_nle_delay_ana) - 1;

	if (Audio_HyBridNLE_getAnalogIdx(para_temp_value, &rg_nle_gain_ana_tar_idx) < 0) {
		pr_err("%s -1\n", __func__);
		return -1;
	}

	pr_aud("%s %d %d enter\n", __func__, rg_nle_delay_ana_idx, rg_nle_gain_ana_tar_idx);

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG3);
	if ((rg_temp_value & 0x00C0) != 0x0080) {
		pr_warn("%s R Warn Off rg_temp_value 0x%x\n", __func__, rg_temp_value);
		Audio_NLE_RegDump();
		return 0;
	}

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG3);
	if ((rg_temp_value & 0x0080) != 0x0080) {
		pr_warn("%s L Warn Off rg_temp_value 0x%x\n", __func__, rg_temp_value);
		Audio_NLE_RegDump();
		return 0;
	}

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG0);
	rg_nle_r_gain_dig_tar = (rg_temp_value & 0x3f00) >> 8;
	rg_nle_r_gain_ana_tar = rg_temp_value & 0x003f;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG0);
	rg_nle_l_gain_dig_tar = (rg_temp_value & 0x3f00) >> 8;
	rg_nle_l_gain_ana_tar = rg_temp_value & 0x003f;

	do {
		if (bRTargeted == false) {
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG0);
			nle_r_gain_dig_cur = (rg_temp_value & 0x3f00) >> 8;
			nle_r_gain_ana_cur = rg_temp_value & 0x003f;
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG3);
			nle_r_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
			nle_r_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;
			if (rg_nle_r_gain_dig_tar != 0 || rg_nle_r_gain_ana_tar != rg_nle_gain_ana_tar_idx) {
				if (bRNoZCEEnable == false) {
					/* Send NonZeroEvent*/
					Ana_Set_Reg(AFE_DL_NLE_R_CFG0, rg_nle_gain_ana_tar_idx, 0x3f3f);
					Ana_Set_Reg(AFE_DL_NLE_R_CFG1, 0x8000, 0xB700);
					rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG2) & 0x01;
					Ana_Set_Reg(AFE_DL_NLE_R_CFG2, rg_temp_value?0:1, 0x0001);
					bRNoZCEEnable = true;
					rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG0);
					rg_nle_r_gain_dig_tar = (rg_temp_value & 0x3f00) >> 8;
					rg_nle_r_gain_ana_tar = rg_temp_value & 0x003f;
					bSleepFlg = true;
					pr_warn("%s R Do NoZCE [%d]->[%d], [%d]->[%d]\n", __func__,
						nle_r_gain_dig_cur, 0, nle_r_gain_ana_cur, rg_nle_gain_ana_tar_idx);
				} else {
					pr_warn("%s R Err bRNoZCEEnable %d\n", __func__, bRNoZCEEnable);
					Audio_NLE_RegDump();
				}
			} else if (nle_r_gain_dig_cur != rg_nle_r_gain_dig_tar ||
					nle_r_gain_ana_cur != rg_nle_r_gain_ana_tar) {
				if (nle_r_dig_gain_targeted && nle_r_ana_gain_targeted) {
					pr_warn("%s R Err nle_r_dig_gain_targeted %d nle_r_ana_gain_targeted %d\n",
						__func__, nle_r_dig_gain_targeted, nle_r_ana_gain_targeted);
					Audio_NLE_RegDump();
					/* break; */
				} else {
					pr_warn("%s R Wait [%d]->[%d], [%d]->[%d]\n", __func__, nle_r_gain_dig_cur,
						rg_nle_r_gain_dig_tar, nle_r_gain_ana_cur, rg_nle_r_gain_ana_tar);
					bSleepFlg = true;
				}
			} else if (nle_r_dig_gain_targeted && nle_r_ana_gain_targeted) {
				pr_warn("%s R successfully\n", __func__);
				bRTargeted = true;
				/* break; */
			} else if (nle_r_gain_dig_cur == rg_nle_r_gain_dig_tar &&
					nle_r_gain_ana_cur == rg_nle_r_gain_ana_tar) {
				pr_warn("%s R successfully(NotTargeted)\n", __func__);
				bRTargeted = true;
				Audio_NLE_RegDump();
				/* break; */
			} else {
				pr_warn("%s R Err Unknown status\n", __func__);
				Audio_NLE_RegDump();
				bSleepFlg = true;
			}
		}

		if (bLTargeted == false) {
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG0);
			nle_l_gain_dig_cur = (rg_temp_value & 0x3f00) >> 8;
			nle_l_gain_ana_cur = rg_temp_value & 0x003f;
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG3);
			nle_l_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
			nle_l_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;
			if (rg_nle_l_gain_dig_tar != 0 || rg_nle_l_gain_ana_tar != rg_nle_gain_ana_tar_idx) {
				if (bLNoZCEEnable == false) {
					/* Send NonZeroEvent*/
					Ana_Set_Reg(AFE_DL_NLE_L_CFG0, rg_nle_gain_ana_tar_idx, 0x3f3f);
					Ana_Set_Reg(AFE_DL_NLE_L_CFG1, 0x8000, 0xB700);
					rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG2) & 0x01;
					Ana_Set_Reg(AFE_DL_NLE_L_CFG2, rg_temp_value?0:1, 0x0001);
					bLNoZCEEnable = true;
					rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG0);
					rg_nle_l_gain_dig_tar = (rg_temp_value & 0x3f00) >> 8;
					rg_nle_l_gain_ana_tar = rg_temp_value & 0x3f;
					bSleepFlg = true;
					pr_warn("%s L Do NoZCE [%d]->[%d], [%d]->[%d]\n", __func__,
						nle_l_gain_dig_cur, 0, nle_l_gain_ana_cur, rg_nle_gain_ana_tar_idx);
				} else {
					pr_warn("%s L Err bLNoZCEEnable %d\n", __func__, bLNoZCEEnable);
					Audio_NLE_RegDump();
				}
			} else if (nle_l_gain_dig_cur != rg_nle_l_gain_dig_tar ||
					nle_l_gain_ana_cur != rg_nle_l_gain_ana_tar) {
				if (nle_l_dig_gain_targeted && nle_l_ana_gain_targeted) {
					pr_warn("%s L Err nle_l_dig_gain_targeted %d nle_l_ana_gain_targeted %d\n",
						__func__, nle_l_dig_gain_targeted, nle_l_ana_gain_targeted);
					Audio_NLE_RegDump();
					/* break; */
				} else {
					pr_warn("%s L Wait [%d]->[%d], [%d]->[%d]\n", __func__, nle_l_gain_dig_cur,
						rg_nle_l_gain_dig_tar, nle_l_gain_ana_cur, rg_nle_l_gain_ana_tar);
					bSleepFlg = true;
				}
			} else if (nle_l_dig_gain_targeted && nle_l_ana_gain_targeted) {
				pr_warn("%s L successfully\n", __func__);
				bLTargeted = true;
				/* break; */
			} else if (nle_l_gain_dig_cur == rg_nle_l_gain_dig_tar &&
					nle_l_gain_ana_cur == rg_nle_l_gain_ana_tar) {
				pr_warn("%s L successfully(NotTargeted)\n", __func__);
				bRTargeted = true;
				Audio_NLE_RegDump();
				/* break; */
			} else {
				pr_warn("%s L Err Unknown status\n", __func__);
				Audio_NLE_RegDump();
				bSleepFlg = true;
			}
		}

		if (bSleepFlg) {
			dCount++;
			if (dCount > WAIT_NLE_TARGET_COUNT) {
				pr_warn("%s Err TimeOut R Ready ? %d, L Ready ? %d\n", __func__, bRTargeted,
					bLTargeted);
				Audio_NLE_RegDump();
				break;
			}
			udelay(1000);
			bSleepFlg = false;
		} else
			break;
	} while (1);
	Ana_Set_Reg(AFE_DL_NLE_L_CFG3, 0, 0x0080);
	Ana_Set_Reg(AFE_DL_NLE_R_CFG3, (1 << 6), 0x00C0);  /* pdn last trigger on+pdn*/
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = rg_nle_gain_ana_tar_idx;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = rg_nle_gain_ana_tar_idx;
	pr_aud("%s exit\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_Info_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	uint32 rg_temp_value;

	pr_aud("%s enter\n", __func__);
	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG0);
	ucontrol->value.bytes.data[0] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[1] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG1);
	ucontrol->value.bytes.data[2] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[3] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG2);
	ucontrol->value.bytes.data[4] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[5] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG3);
	ucontrol->value.bytes.data[6] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[7] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG0);
	ucontrol->value.bytes.data[8] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[9] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG1);
	ucontrol->value.bytes.data[10] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[11] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG2);
	ucontrol->value.bytes.data[12] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[13] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG3);
	ucontrol->value.bytes.data[14] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[15] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG0);
	ucontrol->value.bytes.data[16] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[17] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG1);
	ucontrol->value.bytes.data[18] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[19] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG2);
	ucontrol->value.bytes.data[20] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[21] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG3);
	ucontrol->value.bytes.data[22] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[23] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG0);
	ucontrol->value.bytes.data[24] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[25] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG1);
	ucontrol->value.bytes.data[26] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[27] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG2);
	ucontrol->value.bytes.data[28] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[29] = (rg_temp_value & 0x0000ff00) >> 8;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG3);
	ucontrol->value.bytes.data[30] = rg_temp_value & 0x000000ff;
	ucontrol->value.bytes.data[31] = (rg_temp_value & 0x0000ff00) >> 8;
	pr_aud("%s exit\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_Info_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_Targeted_Get(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	uint32 rg_temp_value;
	uint32 nle_r_dig_gain_targeted, nle_r_ana_gain_targeted;
	uint32 nle_l_dig_gain_targeted, nle_l_ana_gain_targeted;

	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG3);
	nle_r_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
	nle_r_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;
	rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG3);
	nle_l_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
	nle_l_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;

	if (nle_r_dig_gain_targeted && nle_r_ana_gain_targeted
	&& nle_l_dig_gain_targeted && nle_l_ana_gain_targeted)
		ucontrol->value.integer.value[0] = 1;
	else {
		pr_warn("%s R a[%d]d[%d], L a[%d]d[%d]", __func__, nle_r_ana_gain_targeted, nle_r_dig_gain_targeted,
		nle_l_ana_gain_targeted, nle_l_dig_gain_targeted);
		ucontrol->value.integer.value[0] = 0;
	}
	return 0;
}

static int Audio_HyBridNLE_Targeted_Set(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_SetGain_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_SetGain_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int32 nle_channel = (int32) ucontrol->value.bytes.data[0];
	uint32 zerocount;
	uint32 maxdbPerStep = (uint32) ucontrol->value.bytes.data[5];
	int32 gainNleDb = (int32) ucontrol->value.bytes.data[6];
	int32 gainHPDb;
	uint32 rg_temp_value;
	uint32 nle_dig_gain_targeted, nle_ana_gain_targeted;
	uint32 gainNleIndex, gainHPIndex, dGainStepIndex;
	int32 srcGainNleDb, srcCurGainNleDb;
	uint32 absSPL_diff, dGainStep, dToogleNum, dTempdGainStep, dTempToogleNum;

	if (ucontrol->value.bytes.data[7] > 0)
		gainHPDb = ((int32) ucontrol->value.bytes.data[8]) * (-1);
	else
		gainHPDb = (int32) ucontrol->value.bytes.data[8];

	if ((maxdbPerStep > 8) || (maxdbPerStep == 0) || (nle_channel > AUDIO_NLE_CHANNEL_R) ||
		(gainNleDb > NLE_DIG_GAIN_MAX_DB) || (gainHPDb > NLE_ANA_GAIN_MAX_DB) ||
		(gainHPDb < NLE_ANA_GAIN_MIN_DB && gainHPDb != NLE_ANA_GAIN_MUTE_DB)) {
		pr_err("%s Parameter invalid -10\n", __func__);
		pr_err("max [%d] ch [%d] dGain [%d] aGain [%d]\n", maxdbPerStep, nle_channel, gainNleDb, gainHPDb);
		return -10;
	}
	dGainStepIndex = maxdbPerStep - 1;
	if (Audio_HyBridNLE_getAnalogIdx(gainHPDb, &gainHPIndex) < 0) {
		pr_err("%s -1\n", __func__);
		return -1;
	}

	if (Audio_HyBridNLE_getDigitalIdx(gainNleDb, &gainNleIndex) < 0) {
		pr_err("%s -2\n", __func__);
		return -2;
	}

	zerocount = (uint32) ucontrol->value.bytes.data[4];
	zerocount = (zerocount << 8) | ((uint32) ucontrol->value.bytes.data[3]);
	zerocount = (zerocount << 8) | ((uint32) ucontrol->value.bytes.data[2]);
	zerocount = (zerocount << 8) | ((uint32) ucontrol->value.bytes.data[1]);
	pr_aud("%d AlgIdx %d(%d dB) dgtIdx %d(%d dB)\n", nle_channel, gainHPIndex, gainHPDb, gainNleIndex, gainNleDb);
	if (nle_channel == AUDIO_NLE_CHANNEL_R) {
		rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG3);
		nle_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
		nle_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;
		if (nle_dig_gain_targeted && nle_ana_gain_targeted) {
			if (zerocount == 0) {
				Ana_Set_Reg(AFE_DL_NLE_R_CFG0, (gainNleIndex << 8) | gainHPIndex, 0x3f3f);
				Ana_Set_Reg(AFE_DL_NLE_R_CFG1, 0x8000|(dGainStepIndex << 8), 0xB700);
			} else {
				rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG0);
				if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8),
					&srcGainNleDb) < 0) {
					pr_err("%s R-3\n", __func__);
					return -3;
				}
			}
		} else {
			rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG0);
			if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8), &srcGainNleDb) < 0) {
				pr_err("%s R-4\n", __func__);
				return -4;
			}
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_R_CFG0);
			if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8),
				&srcCurGainNleDb) < 0) {
				pr_err("%s R-5\n", __func__);
				return -5;
			}

			if (srcCurGainNleDb > srcGainNleDb) {
				if (gainNleDb < srcGainNleDb)
					srcGainNleDb = srcCurGainNleDb;
				else if ((gainNleDb - srcGainNleDb) < (srcCurGainNleDb - gainNleDb))
					srcGainNleDb = srcCurGainNleDb;
			} else {
				if (gainNleDb > srcGainNleDb)
					srcGainNleDb = srcCurGainNleDb;
				else if ((gainNleDb - srcCurGainNleDb) > (srcGainNleDb - gainNleDb))
					srcGainNleDb = srcCurGainNleDb;
			}
		}

		if (zerocount != 0) {
			if (gainNleDb > srcGainNleDb)
				absSPL_diff = gainNleDb - srcGainNleDb;
			else
				absSPL_diff = srcGainNleDb - gainNleDb;

			if (zerocount >= absSPL_diff) {
				dGainStep = 1;
				dToogleNum = 1;
			} else {
				dTempdGainStep = (absSPL_diff + zerocount) / zerocount;
				if (dTempdGainStep <= maxdbPerStep) {
					dGainStep = dTempdGainStep;
					dToogleNum = 1;
				} else {
					dTempToogleNum = (dTempdGainStep+maxdbPerStep) / maxdbPerStep;
					dGainStep = maxdbPerStep;
					dToogleNum = dTempToogleNum;
				}
			}
			dGainStepIndex = dGainStep - 1;
			Ana_Set_Reg(AFE_DL_NLE_R_CFG0, (gainNleIndex << 8) | gainHPIndex, 0x3f3f);
			Ana_Set_Reg(AFE_DL_NLE_R_CFG1, (dGainStepIndex << 8) | dToogleNum, 0xB73f);
		}
		rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_R_CFG2) & 0x01;
		Ana_Set_Reg(AFE_DL_NLE_R_CFG2, rg_temp_value?0:1, 0x0001);
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = gainHPIndex;
	} else if (nle_channel == AUDIO_NLE_CHANNEL_L) {
		rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG3);
		nle_dig_gain_targeted = (rg_temp_value & 0x8000) >> 15;
		nle_ana_gain_targeted = (rg_temp_value & 0x1000) >> 12;
		if (nle_dig_gain_targeted && nle_ana_gain_targeted) {
			if (zerocount == 0) {
				Ana_Set_Reg(AFE_DL_NLE_L_CFG0, (gainNleIndex << 8) | gainHPIndex, 0x3f3f);
				Ana_Set_Reg(AFE_DL_NLE_L_CFG1, 0x8000 | (dGainStepIndex << 8), 0xB700);
			} else {
				rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG0);
				if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8),
					&srcGainNleDb) < 0) {
					pr_err("%s R-3\n", __func__);
					return -3;
				}
			}
		} else {
			rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG0);
			if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8), &srcGainNleDb) < 0) {
				pr_err("%s R-4\n", __func__);
				return -4;
			}
			rg_temp_value = Ana_Get_Reg(AFE_RGS_NLE_L_CFG0);
			if (Audio_HyBridNLE_getDbFromDigitalIdx(((rg_temp_value & 0x3f00) >> 8),
				&srcCurGainNleDb) < 0) {
				pr_err("%s R-5\n", __func__);
				return -5;
			}

			if (srcCurGainNleDb > srcGainNleDb) {
				if (gainNleDb < srcGainNleDb)
					srcGainNleDb = srcCurGainNleDb;
				else if ((gainNleDb - srcGainNleDb) < (srcCurGainNleDb - gainNleDb))
					srcGainNleDb = srcCurGainNleDb;
			} else {
				if (gainNleDb > srcGainNleDb)
					srcGainNleDb = srcCurGainNleDb;
				else if ((gainNleDb - srcCurGainNleDb) > (srcGainNleDb - gainNleDb))
					srcGainNleDb = srcCurGainNleDb;
			}
		}

		if (zerocount != 0) {
			if (gainNleDb > srcGainNleDb)
				absSPL_diff = gainNleDb - srcGainNleDb;
			else
				absSPL_diff = srcGainNleDb - gainNleDb;

			if (zerocount >= absSPL_diff) {
				dGainStep = 1;
				dToogleNum = 1;
			} else {
				dTempdGainStep = (absSPL_diff + zerocount) / zerocount;
				if (dTempdGainStep <= maxdbPerStep) {
					dGainStep = dTempdGainStep;
					dToogleNum = 1;
				} else {
					dTempToogleNum = (dTempdGainStep+maxdbPerStep) / maxdbPerStep;
					dGainStep = maxdbPerStep;
					dToogleNum = dTempToogleNum;
				}
			}
			dGainStepIndex = dGainStep - 1;
			Ana_Set_Reg(AFE_DL_NLE_L_CFG0, (gainNleIndex << 8) | gainHPIndex, 0x3f3f);
			Ana_Set_Reg(AFE_DL_NLE_L_CFG1, (dGainStepIndex << 8) | dToogleNum, 0xB73f);
		}
		rg_temp_value = Ana_Get_Reg(AFE_DL_NLE_L_CFG2) & 0x01;
		Ana_Set_Reg(AFE_DL_NLE_L_CFG2, rg_temp_value?0:1, 0x0001);
		mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = gainHPIndex;
	} else
		pr_warn("%s err no channel match\n", __func__);
	pr_aud("%s exit\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_HwCapability_Get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	if (NLE_ANA_GAIN_MAX_DB < 0) {
		ucontrol->value.bytes.data[0] = 1;
		ucontrol->value.bytes.data[1] = (NLE_ANA_GAIN_MAX_DB * (-1));
	} else {
		ucontrol->value.bytes.data[0] = 0;
		ucontrol->value.bytes.data[1] = NLE_ANA_GAIN_MAX_DB;
	}

	if (NLE_ANA_GAIN_MIN_DB < 0) {
		ucontrol->value.bytes.data[2] = 1;
		ucontrol->value.bytes.data[3] = (NLE_ANA_GAIN_MIN_DB * (-1));
	} else {
		ucontrol->value.bytes.data[2] = 0;
		ucontrol->value.bytes.data[3] = NLE_ANA_GAIN_MIN_DB;
	}

	if (NLE_ANA_GAIN_MIN_VALUE < 0) {
		ucontrol->value.bytes.data[4] = 1;
		ucontrol->value.bytes.data[5] = (NLE_ANA_GAIN_MIN_VALUE * (-1));
	} else {
		ucontrol->value.bytes.data[4] = 0;
		ucontrol->value.bytes.data[5] = NLE_ANA_GAIN_MIN_VALUE;
	}

	if (NLE_ANA_GAIN_MUTE_DB < 0) {
		ucontrol->value.bytes.data[6] = 1;
		ucontrol->value.bytes.data[7] = (NLE_ANA_GAIN_MUTE_DB * (-1));
	} else {
		ucontrol->value.bytes.data[6] = 0;
		ucontrol->value.bytes.data[7] = NLE_ANA_GAIN_MUTE_DB;
	}

	if (NLE_ANA_GAIN_MUTE_VALUE < 0) {
		ucontrol->value.bytes.data[8] = 1;
		ucontrol->value.bytes.data[9] = (NLE_ANA_GAIN_MUTE_VALUE * (-1));
	} else {
		ucontrol->value.bytes.data[8] = 0;
		ucontrol->value.bytes.data[9] = NLE_ANA_GAIN_MUTE_VALUE;
	}

	if (NLE_DIG_GAIN_MAX_DB < 0) {
		ucontrol->value.bytes.data[10] = 1;
		ucontrol->value.bytes.data[11] = (NLE_DIG_GAIN_MAX_DB * (-1));
	} else {
		ucontrol->value.bytes.data[10] = 0;
		ucontrol->value.bytes.data[11] = NLE_DIG_GAIN_MAX_DB;
	}

	if (NLE_DIG_GAIN_MAX_VALUE < 0) {
		ucontrol->value.bytes.data[12] = 1;
		ucontrol->value.bytes.data[13] = (NLE_DIG_GAIN_MAX_VALUE * (-1));
	} else {
		ucontrol->value.bytes.data[12] = 0;
		ucontrol->value.bytes.data[13] = NLE_DIG_GAIN_MAX_VALUE;
	}

	if (NLE_DIG_GAIN_MIN_DB < 0) {
		ucontrol->value.bytes.data[14] = 1;
		ucontrol->value.bytes.data[15] = (NLE_DIG_GAIN_MIN_DB * (-1));
	} else {
		ucontrol->value.bytes.data[14] = 0;
		ucontrol->value.bytes.data[15] = NLE_DIG_GAIN_MIN_DB;
	}

	if (NLE_DELAY_ANA_OUTPUT_T < 0) {
		ucontrol->value.bytes.data[16] = 1;
		ucontrol->value.bytes.data[17] = (NLE_DELAY_ANA_OUTPUT_T * (-1));
	} else {
		ucontrol->value.bytes.data[16] = 0;
		ucontrol->value.bytes.data[17] = NLE_DELAY_ANA_OUTPUT_T;
	}
	pr_aud("%s exit\n", __func__);
	return 0;
}

static int Audio_HyBridNLE_HwCapability_Set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s unsupport\n", __func__);
	return 0;
}

static const char *const Audio_HyBridNLE_Targeted[] = { "Off", "On" };

static const struct soc_enum Audio_HyBridNLE_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_HyBridNLE_Targeted),
			    Audio_HyBridNLE_Targeted),
};

static const struct snd_kcontrol_new Audio_snd_hybridNLE_controls[] = {
	SND_SOC_BYTES_EXT("Audio_HyBridNLE_TurnOn", 3, Audio_HyBridNLE_TurnOn_Get, Audio_HyBridNLE_TurnOn_Set),
	SND_SOC_BYTES_EXT("Audio_HyBridNLE_TurnOff", 3, Audio_HyBridNLE_TurnOff_Get, Audio_HyBridNLE_TurnOff_Set),
	SND_SOC_BYTES_EXT("Audio_HyBridNLE_SetGain", 9, Audio_HyBridNLE_SetGain_Get, Audio_HyBridNLE_SetGain_Set),
	SND_SOC_BYTES_EXT("Audio_HyBridNLE_Info", 32, Audio_HyBridNLE_Info_Get, Audio_HyBridNLE_Info_Set),
	SND_SOC_BYTES_EXT("Audio_HyBridNLE_HwCapability", 20, Audio_HyBridNLE_HwCapability_Get,
			Audio_HyBridNLE_HwCapability_Set),
	SOC_ENUM_EXT("Audio_HyBridNLE_Targeted", Audio_HyBridNLE_Enum[0],
		     Audio_HyBridNLE_Targeted_Get,
		     Audio_HyBridNLE_Targeted_Set),
};

static void mt6331_codec_init_reg(struct snd_soc_codec *codec)
{
	pr_warn("%s\n", __func__);

	audckbufEnable(true);
	Ana_Set_Reg(TOP_CLKSQ, 0x0, 0x0001);
	/* Disable CLKSQ 26MHz */
	Ana_Set_Reg(AUDDEC_ANA_CON9, 0x1000, 0x1000);
	/* disable AUDGLB */
	Ana_Set_Reg(TOP_CKPDN_CON0_SET, 0x3800, 0x3800);
	/* Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M */
	Ana_Set_Reg(AUDDEC_ANA_CON0, 0xe000, 0xe000);
	/* Disable HeadphoneL/HeadphoneR/voice short circuit protection */
	/* Ana_Set_Reg(AUDENC_ANA_CON9, 0x0000, 0x0010); */
	/* power off mic bias1 */
	/* Ana_Set_Reg(AUDDEC_ANA_CON3, 0x4228, 0xffff); */
	/* [5] = 1, disable LO buffer left short circuit protection */
	/* Ana_Set_Reg(DRV_CON2, 0xe << 4, 0xf << 4); */
	/* PAD_AUD_DAT_MISO gpio driving MAX */
	Ana_Set_Reg(GPIO_MODE1, 0x1249, 0x7fff);

	Ana_Set_Reg(AFE_PMIC_NEWIF_CFG1, 0x0018, 0x03ff);
	Ana_Set_Reg(AFE_ADDA6_PMIC_NEWIF_CFG1, 0x0000, 0x0300);
	/* UL/UL2 /DL mtkaif format setting : mtk1.5 */
	audckbufEnable(false);
}

void InitCodecDefault(void)
{
	pr_warn("%s\n", __func__);
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP3] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_MICAMP4] = 3;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTL] = 12;
	mCodec_data->mAudio_Ana_Volume[AUDIO_ANALOG_VOLUME_HPOUTR] = 12;

	mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_LCH_MUX] = AUDIO_UL_ARRAY_ADC1;
	mCodec_data->mAudio_Ana_Mux[AUDIO_UL1_RCH_MUX] = AUDIO_UL_ARRAY_ADC2;
	mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_LCH_MUX] = AUDIO_UL_ARRAY_ADC3;
	mCodec_data->mAudio_Ana_Mux[AUDIO_UL2_RCH_MUX] = AUDIO_UL_ARRAY_ADC4;

	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_1] = AUDIO_AMIC_AIN0;
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_2] = AUDIO_AMIC_AIN0;
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_3] = AUDIO_AMIC_AIN5;
	mCodec_data->mAudio_Ana_Mux[AUDIO_MICSOURCE_MUX_IN_4] = AUDIO_AMIC_AIN5;
}

static void InitGlobalVarDefault(void)
{
	mCodec_data = NULL;
	mAudio_Vow_Analog_Func_Enable = false;
	mAudio_Vow_Digital_Func_Enable = false;
	mIsVOWOn = false;
	mInitCodec = false;
	mAnaSuspend = false;
	audck_buf_Count = 0;
	ClsqCount = 0;
	TopCkCount = 0;
	NvRegCount = 0;
	SwOperationCount = 0;
	MicbiasRef = 0;
	GetMicbias = 0;
#ifdef CONFIG_MTK_SPEAKER
	mSpeaker_Ocflag = false;
#endif

}

static int mt6331_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->component.dapm;

	pr_warn("%s()\n", __func__);

	if (mInitCodec == true)
		return 0;

	ANC_enabled = 0;

#ifdef MT6755_AW8736_REWORK
	int data[4];
	int rawdata;
	int ret;
#endif

	pin_extspkamp = pin_extspkamp_2 = pin_vowclk = pin_audmiso = pin_rcvspkswitch = 0;
	pin_mode_extspkamp = pin_mode_extspkamp_2 = pin_mode_vowclk =
	    pin_mode_audmiso = pin_mode_rcvspkswitch = 0;

	snd_soc_dapm_new_controls(dapm, mt6331_dapm_widgets, ARRAY_SIZE(mt6331_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, mtk_audio_map, ARRAY_SIZE(mtk_audio_map));

	/* add codec controls */
	snd_soc_add_codec_controls(codec, mt6331_snd_controls, ARRAY_SIZE(mt6331_snd_controls));
	snd_soc_add_codec_controls(codec, mt6331_UL_Codec_controls,
				   ARRAY_SIZE(mt6331_UL_Codec_controls));
	snd_soc_add_codec_controls(codec, mt6331_Voice_Switch, ARRAY_SIZE(mt6331_Voice_Switch));
	snd_soc_add_codec_controls(codec, mt6331_pmic_Test_controls,
				   ARRAY_SIZE(mt6331_pmic_Test_controls));

#ifdef CONFIG_MTK_SPEAKER
	snd_soc_add_codec_controls(codec, mt6331_snd_Speaker_controls,
				   ARRAY_SIZE(mt6331_snd_Speaker_controls));
#endif

	snd_soc_add_codec_controls(codec, Audio_snd_auxadc_controls,
				   ARRAY_SIZE(Audio_snd_auxadc_controls));

	snd_soc_add_codec_controls(codec, Audio_snd_hybridNLE_controls,
				   ARRAY_SIZE(Audio_snd_hybridNLE_controls));

	/* here to set  private data */
	mCodec_data = kzalloc(sizeof(struct codec_data_private), GFP_KERNEL);
	if (!mCodec_data) {
		/*pr_warn("Failed to allocate private data\n");*/
		return -ENOMEM;
	}
	snd_soc_codec_set_drvdata(codec, mCodec_data);

	memset((void *)mCodec_data, 0, sizeof(struct codec_data_private));
	mt6331_codec_init_reg(codec);
	InitCodecDefault();
	read_efuse_dpd();
	efuse_current_calibration = read_efuse_hp_impedance_current_calibration();
	mInitCodec = true;

	return 0;
}

static int mt6331_codec_remove(struct snd_soc_codec *codec)
{
	pr_warn("%s()\n", __func__);
	return 0;
}

static unsigned int mt6331_read(struct snd_soc_codec *codec, unsigned int reg)
{
	pr_warn("mt6331_read reg = 0x%x", reg);
	Ana_Get_Reg(reg);
	return 0;
}

static int mt6331_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	pr_warn("mt6331_write reg = 0x%x  value= 0x%x\n", reg, value);
	Ana_Set_Reg(reg, value, 0xffffffff);
	return 0;
}

static struct snd_soc_codec_driver soc_mtk_codec = {
	.probe = mt6331_codec_probe,
	.remove = mt6331_codec_remove,

	.read = mt6331_read,
	.write = mt6331_write,

	/* use add control to replace */
	/* .controls = mt6331_snd_controls, */
	/* .num_controls = ARRAY_SIZE(mt6331_snd_controls), */

	.dapm_widgets = mt6331_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt6331_dapm_widgets),
	.dapm_routes = mtk_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mtk_audio_map),

};

static int mtk_mt6331_codec_dev_probe(struct platform_device *pdev)
{
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);

	if (pdev->dev.dma_mask == NULL)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;


	if (pdev->dev.of_node) {
		dev_set_name(&pdev->dev, "%s", MT_SOC_CODEC_NAME);

		/* check if use UL 260k flow */
		of_property_read_u32(pdev->dev.of_node,
				     "use_low_power_mode",
				     &low_power_mode);

		pr_warn("%s(), use_low_power_mode = %d\n", __func__, low_power_mode);
	} else {
		pr_warn("%s(), pdev->dev.of_node = NULL!!!\n", __func__);
	}


	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_codec(&pdev->dev,
				      &soc_mtk_codec, mtk_6331_dai_codecs,
				      ARRAY_SIZE(mtk_6331_dai_codecs));
}

static int mtk_mt6331_codec_dev_remove(struct platform_device *pdev)
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

static struct platform_driver mtk_codec_6331_driver = {
	.driver = {
		   .name = MT_SOC_CODEC_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_soc_codec_63xx_of_ids,
#endif
		   },
	.probe = mtk_mt6331_codec_dev_probe,
	.remove = mtk_mt6331_codec_dev_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtk_codec6331_dev;
#endif

static int __init mtk_mt6331_codec_init(void)
{
	pr_warn("%s:\n", __func__);
#ifndef CONFIG_OF
	int ret = 0;

	soc_mtk_codec6331_dev = platform_device_alloc(MT_SOC_CODEC_NAME, -1);

	if (!soc_mtk_codec6331_dev)
		return -ENOMEM;


	ret = platform_device_add(soc_mtk_codec6331_dev);
	if (ret != 0) {
		platform_device_put(soc_mtk_codec6331_dev);
		return ret;
	}
#endif
	InitGlobalVarDefault();

	return platform_driver_register(&mtk_codec_6331_driver);
}

module_init(mtk_mt6331_codec_init);

static void __exit mtk_mt6331_codec_exit(void)
{
	pr_warn("%s:\n", __func__);

	platform_driver_unregister(&mtk_codec_6331_driver);
}

module_exit(mtk_mt6331_codec_exit);

/* Module information */
MODULE_DESCRIPTION("MTK  codec driver");
MODULE_LICENSE("GPL v2");
