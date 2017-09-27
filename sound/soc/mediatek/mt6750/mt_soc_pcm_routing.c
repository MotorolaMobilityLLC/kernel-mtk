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
 *   mt6583.c
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
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

#include <linux/dma-mapping.h>
#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_codec_63xx.h"
#include "mt_soc_pcm_common.h"
#include "mt_auddrv_devtree_parser.h"
#include "auddrv_underflow_mach.h"
#include "AudDrv_Common_func.h"
#include "AudDrv_Gpio.h"

/*
#include <mach/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pmic_wrap.h>
#include <mach/pmic_mt6325_sw.h>*/

#include <linux/time.h>
/*#include <cust_pmic.h>*/
/*#include <cust_battery_meter.h>*/
#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#else
#include <mach/mt_clkmgr.h>
#endif
/*#include <mach/mt_pm_ldo.h>
#if defined(CONFIG_MTK_LEGACY)
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#endif
*/

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

/*
 *    function implementation
 */

/* MT6755 GIT318 temporary workaround */

static int mtk_afe_routing_probe(struct platform_device *pdev);
static int mtk_routing_pcm_close(struct snd_pcm_substream *substream);
static int mtk_asoc_routing_pcm_new(struct snd_soc_pcm_runtime *rtd);
static int mtk_afe_routing_platform_probe(struct snd_soc_platform *platform);
/* extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd); */

static int mDac_Sinegen = 27;
static const char const *DAC_DL_SIDEGEN[] = {"I0I1", "I2", "I3I4", "I5I6",
				       "I7I8", "I9", "I10I11", "I12I13",
				       "I14", "I15I16", "I17I18", "I19I20",
				       "I21I22", "O0O1", "O2", "O3O4",
				       "O5O6", "O7O8", "O9O10", "O11",
				       "O12", "O13O14", "O15O16", "O17O18",
				       "O19O20", "O21O22", "O23O24", "OFF",
				       "O3", "O4"
				      };

static int mDac_SampleRate = 8;
static const char const *DAC_DL_SIDEGEN_SAMEPLRATE[] = {
	"8K", "11K", "12K", "16K", "22K", "24K", "32K", "44K", "48K" };

static int mDac_Sidegen_Amplitude = 6;
static const char const *DAC_DL_SIDEGEN_AMPLITUE[] = {
	"1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1" };

static bool mEnableSidetone;
static const char const *ENABLESIDETONE[] = { "Off", "On" };

static int mAudio_Mode;
static const char const *ANDROID_AUDIO_MODE[] = {
	"Normal_Mode", "Ringtone_Mode", "Incall_Mode", "Communication_Mode", "Incall2_Mode",
	    "Incall_External_Mode"
};
static const char const *InterModemPcm_ASRC_Switch[] = { "Off", "On" };
static const char const *Audio_Debug_Setting[] = { "Off", "On" };
static const char const *Audio_IPOH_State[] = { "Off", "On" };
static const char const *Audio_I2S1_Setting[] = { "Off", "On" };
static const char * const audio_dctrim_flag_setting[] = { "Not_Yet", "Calibrated" };

#define MAX_HP_GAIN_LEVEL (19)

#ifndef SOC_SINGLE_MULTI_EXT
struct soc_multi_mixer_control {
	int min, max, platform_max, count;
	unsigned int reg, rreg, shift, rshift, invert;
};

#define SOC_SINGLE_MULTI_EXT(xname, xreg, xshift, xmax, xinvert, xcount,\
	xhandler_get, xhandler_put) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_multi_ext, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = (unsigned long)&(struct soc_multi_mixer_control) \
		{.reg = xreg, .shift = xshift, .rshift = xshift, .max = xmax, \
		.count = xcount, .platform_max = xmax, .invert = xinvert} }

int snd_soc_info_multi_ext(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	struct soc_multi_mixer_control *mc =
		(struct soc_multi_mixer_control *)kcontrol->private_value;
	int platform_max;

	if (!mc->platform_max)
		mc->platform_max = mc->max;
	platform_max = mc->platform_max;
	if (platform_max == 1 && !strnstr(kcontrol->id.name, " Volume", 30))
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = mc->count;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = platform_max;
	return 0;
}
#endif

static bool AudDrvSuspendStatus;
/* static bool mModemPcm_ASRC_on = false; */
static bool AudioI2S1Setting;

static bool mHplCalibrated;
static int  mHplOffset;
static bool mHprCalibrated;
static int  mHprOffset;
static bool mHplSpkCalibrated;
static int  mHplSpkOffset[MAX_HP_GAIN_LEVEL];
static bool mHprSpkCalibrated;
static int  mHprSpkOffset;
static bool AudDrvSuspend_ipoh_Status;

int Get_Audio_Mode(void)
{
	return mAudio_Mode;
}

static int Audio_SideGen_Get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_AmpR_Get = %d\n", mDac_Sinegen);
	ucontrol->value.integer.value[0] = mDac_Sinegen;
	return 0;
}

static int Audio_SideGen_Set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_warn("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_SIDEGEN)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	switch (index) {
	case 0:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I00,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 1:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I02,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 2:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I03,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 3:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I05,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 4:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I07,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 5:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I09,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 6:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I11,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 7:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I12,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 8:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I14,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 9:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I15,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 10:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I17,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 11:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I19,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 12:
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I21,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
		break;
	case 13:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O01,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 14:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O02,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 15:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O03,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 16:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O05,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 17:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O07,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 18:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O09,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 19:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O11,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 20:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O12,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 21:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O13,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 22:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O15,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 23:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O17,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 24:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O19,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 25:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O21,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 26:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O23,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
		break;
	case 27:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O11,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, false);
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I00,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, false);
		break;
	case 28:
		Afe_Set_Reg(AFE_SGEN_CON0, 0x2E8c28c2, 0xffffffff); /* o3o4 but mute o4 */
		break;
	case 29:
		Afe_Set_Reg(AFE_SGEN_CON0, 0x2D8c28c2, 0xffffffff); /* o3o4 but mute o3 */
		break;
	default:
		EnableSideGenHw(Soc_Aud_InterConnectionOutput_O11,
				Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, false);
		EnableSideGenHw(Soc_Aud_InterConnectionInput_I00,
				Soc_Aud_MemIF_Direction_DIRECTION_INPUT, false);
		break;
	}
	mDac_Sinegen = index;
	return 0;
}

static int Audio_SideGen_SampleRate_Get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s\n", __func__);
	ucontrol->value.integer.value[0] = mDac_SampleRate;
	return 0;
}

static int Audio_SideGen_SampleRate_Set(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_warn("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_SIDEGEN_SAMEPLRATE)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];


	switch (index) {
	case 0:
		SetSideGenSampleRate(8000);
		break;
	case 1:
		SetSideGenSampleRate(11025);
		break;
	case 2:
		SetSideGenSampleRate(12000);
		break;
	case 3:
		SetSideGenSampleRate(16000);
		break;
	case 4:
		SetSideGenSampleRate(22050);
		break;
	case 5:
		SetSideGenSampleRate(24000);
		break;
	case 6:
		SetSideGenSampleRate(32000);
		break;
	case 7:
		SetSideGenSampleRate(44100);
		break;
	case 8:
		SetSideGenSampleRate(48000);
		break;
	default:
		SetSideGenSampleRate(32000);
		break;
	}

	mDac_SampleRate = index;
	return 0;
}

static int Audio_SideGen_Amplitude_Get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_AmpR_Get = %d\n", mDac_Sidegen_Amplitude);
	return 0;
}

static int Audio_SideGen_Amplitude_Set(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_warn("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(DAC_DL_SIDEGEN_AMPLITUE)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	mDac_Sidegen_Amplitude = index;
	return 0;
}

static int Audio_SideTone_Get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_SideTone_Get = %d\n", mEnableSidetone);
	ucontrol->value.integer.value[0] = mEnableSidetone;
	return 0;
}

static int Audio_SideTone_Set(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int index = 0;

	pr_warn("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ENABLESIDETONE)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	index = ucontrol->value.integer.value[0];
	if (mEnableSidetone != index) {
		mEnableSidetone = index;
		EnableSideToneFilter(mEnableSidetone);
	}
	return 0;
}
#if 0				/* not used */
static int Audio_ModemPcm_ASRC_Get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s(), mModemPcm_ASRC_on=%d\n", __func__, mModemPcm_ASRC_on);
	ucontrol->value.integer.value[0] = mModemPcm_ASRC_on;
	return 0;
}
#endif

static int AudioDebug_Setting_Set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_Debug_Setting)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	EnableSideGenHw(Soc_Aud_InterConnectionOutput_O03,
			Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, true);
	msleep(5 * 1000);
	EnableSideGenHw(Soc_Aud_InterConnectionOutput_O03,
			Soc_Aud_MemIF_Direction_DIRECTION_OUTPUT, false);
	EnableSideGenHw(Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_MemIF_Direction_DIRECTION_INPUT, true);
	msleep(5 * 1000);
	EnableSideGenHw(Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_MemIF_Direction_DIRECTION_INPUT, false);

	Ana_Log_Print();
	Afe_Log_Print();

	return 0;
}

static void Auddrv_I2S1GpioSet(void)
{

#ifndef CONFIG_FPGA_EARLY_PORTING

#ifdef CONFIG_OF
#if defined(CONFIG_MTK_LEGACY)

	AUDDRV_I2S_ATTRIBUTE *I2S1Settingws = GetI2SSetting(Auddrv_I2S1_Setting,
							    Auddrv_I2S_Setting_ws);
	AUDDRV_I2S_ATTRIBUTE *I2S1Settingbck = GetI2SSetting(Auddrv_I2S1_Setting,
							     Auddrv_I2S_Setting_bck);
	AUDDRV_I2S_ATTRIBUTE *I2S1SettingD00 = GetI2SSetting(Auddrv_I2S1_Setting,
							     Auddrv_I2S_Setting_D00);
	AUDDRV_I2S_ATTRIBUTE *I2S1SettingMclk = GetI2SSetting(Auddrv_I2S1_Setting,
							      Auddrv_I2S_Setting_Mclk);

	mt_set_gpio_mode(I2S1Settingws->Gpio_Number, I2S1Settingws->Gpio_Mode);
	mt_set_gpio_mode(I2S1Settingbck->Gpio_Number, I2S1Settingws->Gpio_Mode);
	mt_set_gpio_mode(I2S1SettingD00->Gpio_Number, I2S1Settingws->Gpio_Mode);
	mt_set_gpio_mode(I2S1SettingMclk->Gpio_Number, I2S1Settingws->Gpio_Mode);
#else
	AudDrv_GPIO_I2S_Select(true);
#endif
#else
	mt_set_gpio_mode(GPIO_I2S1_CK_PIN, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO_I2S1_DAT_PIN, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO_I2S1_MCLK_PIN, GPIO_MODE_01);
	mt_set_gpio_mode(GPIO_I2S1_WS_PIN, GPIO_MODE_01);
#endif

#endif
}

static void Auddrv_I2S1GpioReset(void)
{

#ifndef CONFIG_FPGA_EARLY_PORTING

#ifdef CONFIG_OF
#if defined(CONFIG_MTK_LEGACY)

	AUDDRV_I2S_ATTRIBUTE *I2S1Settingws = GetI2SSetting(Auddrv_I2S1_Setting,
							    Auddrv_I2S_Setting_ws);
	AUDDRV_I2S_ATTRIBUTE *I2S1Settingbck = GetI2SSetting(Auddrv_I2S1_Setting,
							     Auddrv_I2S_Setting_bck);
	AUDDRV_I2S_ATTRIBUTE *I2S1SettingD00 = GetI2SSetting(Auddrv_I2S1_Setting,
							     Auddrv_I2S_Setting_D00);
	AUDDRV_I2S_ATTRIBUTE *I2S1SettingMclk = GetI2SSetting(Auddrv_I2S1_Setting,
							      Auddrv_I2S_Setting_Mclk);

	mt_set_gpio_mode(I2S1Settingws->Gpio_Number, 0);
	mt_set_gpio_mode(I2S1Settingbck->Gpio_Number, 0);
	mt_set_gpio_mode(I2S1SettingD00->Gpio_Number, 0);
	mt_set_gpio_mode(I2S1SettingMclk->Gpio_Number, 0);
#else
	AudDrv_GPIO_I2S_Select(false);
#endif
#else
	mt_set_gpio_mode(GPIO_I2S1_CK_PIN, GPIO_MODE_00);
	mt_set_gpio_mode(GPIO_I2S1_DAT_PIN, GPIO_MODE_00);
	mt_set_gpio_mode(GPIO_I2S1_MCLK_PIN, GPIO_MODE_00);
	mt_set_gpio_mode(GPIO_I2S1_WS_PIN, GPIO_MODE_00);
#endif

#endif
}

static int AudioDebug_Setting_Get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	return 0;
}

static int AudioI2S1_Setting_Set(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_I2S1_Setting)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	AudioI2S1Setting = ucontrol->value.enumerated.item[0];
	if (AudioI2S1Setting == true)
		Auddrv_I2S1GpioSet();
	else
		Auddrv_I2S1GpioReset();
	return 0;
}

static int AudioI2S1_Setting_Get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.enumerated.item[0] = AudioI2S1Setting;
	return 0;
}

#if 0
static int Audio_ModemPcm_ASRC_Set(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("+%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(InterModemPcm_ASRC_Switch)) {
		pr_warn("return -EINVAL\n");
		return -EINVAL;
	}
	mModemPcm_ASRC_on = (bool)ucontrol->value.integer.value[0];
	Audio_ModemPcm2_ASRC_Set(mModemPcm_ASRC_on);
	pr_warn("-%s(), mModemPcm_ASRC_on=%d\n", __func__, mModemPcm_ASRC_on);
	return 0;
}
#endif

static int Audio_Ipoh_Setting_Get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = AudDrvSuspend_ipoh_Status;
	return 0;
}

static int Audio_Ipoh_Setting_Set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("+%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(Audio_IPOH_State)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	AudDrvSuspend_ipoh_Status = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_Mode_Get(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("Audio_SideTone_Get = %d\n", mAudio_Mode);
	ucontrol->value.integer.value[0] = mAudio_Mode;
	return 0;
}

static int Audio_Mode_Set(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(ANDROID_AUDIO_MODE)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mAudio_Mode = ucontrol->value.integer.value[0];
	return 0;
}

static int Audio_LowLatencyDebug_Get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = get_LowLatencyDebug();
	return 0;
}

static int Audio_LowLatencyDebug_Set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	set_LowLatencyDebug(ucontrol->value.integer.value[0]);
	return 0;
}


/* static struct snd_dma_buffer *Dl1_Playback_dma_buf  = NULL; */

static int GetAudioTrimOffsetAverage(int *buffer_value, int trim_num)
{
	int i , j, tmp;

	for (i = 0; i < trim_num+1; i++) {
		for (j = i+1; j < trim_num+2; j++) {
			if (buffer_value[i] > buffer_value[j]) {
				tmp = buffer_value[i];
				buffer_value[i] = buffer_value[j];
				buffer_value[j] = tmp;
			}
		}
	}

	tmp = 0;
	for (i = 0; i <  trim_num; i++)
		tmp = tmp + buffer_value[i+1];

	tmp = (tmp + (trim_num/2)) / trim_num;
	return tmp;
}

#ifdef AUDIO_DL2_ISR_COPY_SUPPORT

static int Audio_DL2_DataTransfer(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
#ifdef CONFIG_COMPAT
	void *addr =  compat_ptr(ucontrol->value.integer.value[0]);
#else
	void *addr =  (void *)ucontrol->value.integer.value[0];
#endif

	uint32 size =  ucontrol->value.integer.value[1];

	/* pr_warn("%s(), addr %p, size %d\n", __func__, addr, size); */

	mtk_dl2_copy2buffer(addr, size);
	return 0;
}

#endif

#define TRIM_NUM (10)

static void GetAudioTrimOffset_HP(int channels)
{
	const int off_counter = 20, on_counter = 20, Const_DC_OFFSET = 2048;
	int Buffer_on_value = 0, Buffer_off_value = 0;
	int mHplOffset_tmp[TRIM_NUM+2], mHprOffset_tmp[TRIM_NUM+2];
	int i;

	pr_warn("%s channels = %d\n", __func__, channels);

	for (i = 0; i < (TRIM_NUM + 2); i++) {
		mHplOffset_tmp[i] = 0;
		mHprOffset_tmp[i] = 0;
	}
	/* open headphone and digital part */
	AudDrv_Clk_On();
	AudDrv_Emi_Clk_On();
	OpenAfeDigitaldl1(true);

	/* AUXADC large scale channel 9- AUXADC_CON2(AUXADC ADC AVG SELECTION[9]) */
	Ana_Set_Reg(MT6353_AUXADC_CON2, 0x0200, 0x0200);

	/* HPL */
	for (i = 0; i < (TRIM_NUM + 2); i++) {
		/* Get HPL off offset */
		SetSdmLevel(AUDIO_SDM_LEVEL_MUTE);
		/*msleep(1); */
		setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPL);
		setOffsetTrimBufferGain(3);

		OpenTrimBufferHardware(true);
		setHpGain(HP_TRIM_INDEX); /* HP only */

		EnableTrimbuffer(true);
		/*msleep(1); */
		usleep_range(10 * 1000, 20 * 1000);
		Buffer_off_value = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, off_counter, 0);

		EnableTrimbuffer(false);
		OpenTrimBufferHardware(false);

		/* calibrate HPL offset trim */
		setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPL);
		setOffsetTrimBufferGain(3);
		/* EnableTrimbuffer(true); */
		/* msleep(5); */

		OpenAnalogHeadphone(true);
		setHpGain(HP_TRIM_INDEX); /* HP only */

		EnableTrimbuffer(true);

		/*msleep(1); */
		usleep_range(10 * 1000, 20 * 1000);
		Buffer_on_value  = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, on_counter, 0);

		mHplOffset_tmp[i] += Buffer_on_value - Buffer_off_value + Const_DC_OFFSET;  /* HP only */
		pr_warn("Lch: Buffer_on_value = %d, Buffer_off_value = %d, mHplOffset_tmp = %d\n",
			Buffer_on_value, Buffer_off_value, mHplOffset_tmp[i]);

		EnableTrimbuffer(false);
		OpenAnalogHeadphone(false);  /* HP only */
	}

	mHplOffset = GetAudioTrimOffsetAverage(mHplOffset_tmp, TRIM_NUM); /* calculate HPL dc average */
	pr_warn("[Average %d times] mHplOffset = %d\n", TRIM_NUM, mHplOffset);

	/* HPR */
	for (i = 0; i < (TRIM_NUM + 2); i++) {
		/* Get HPR off offset */
		SetSdmLevel(AUDIO_SDM_LEVEL_MUTE);
		/*msleep(1); */
		setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPR);
		setOffsetTrimBufferGain(3);

		OpenTrimBufferHardware(true);
		setHpGain(HP_TRIM_INDEX); /* HP only */

		EnableTrimbuffer(true);
		/*msleep(1); */
		usleep_range(10 * 1000, 20 * 1000);
		Buffer_off_value = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, off_counter, 0);

		EnableTrimbuffer(false);
		OpenTrimBufferHardware(false);

		/* calibrate HPR offset trim */
		setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPR);
		setOffsetTrimBufferGain(3);
		/* EnableTrimbuffer(true); */
		/* msleep(5); */

		OpenAnalogHeadphone(true);
		setHpGain(HP_TRIM_INDEX); /* HP only */

		EnableTrimbuffer(true);

		/*msleep(1); */
		usleep_range(10 * 1000, 20 * 1000);
		Buffer_on_value  = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, on_counter, 0);

		mHprOffset_tmp[i] += Buffer_on_value - Buffer_off_value + Const_DC_OFFSET;  /* HP only */
		pr_warn("Rch: Buffer_on_value = %d, Buffer_off_value = %d, mHprOffset_tmp = %d\n",
			Buffer_on_value, Buffer_off_value, mHprOffset_tmp[i]);

		EnableTrimbuffer(false);
		OpenAnalogHeadphone(false);  /* HP only */
	}

	mHprOffset = GetAudioTrimOffsetAverage(mHprOffset_tmp, TRIM_NUM); /* calculate HPR dc average */
	pr_warn("[Average %d times] mHprOffset = %d\n", TRIM_NUM, mHprOffset);

	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_GROUND);
	OpenAfeDigitaldl1(false);

	SetSdmLevel(AUDIO_SDM_LEVEL_NORMAL);
	AudDrv_Emi_Clk_Off();
	AudDrv_Clk_Off();
}

static void GetAudioTrimOffset_HPSPK(int channels)
{
	const int off_counter = 20, on_counter = 20, Const_DC_OFFSET = 2048;
	int Buffer_on_value[MAX_HP_GAIN_LEVEL], Buffer_offl_value[MAX_HP_GAIN_LEVEL],
		Buffer_offr_value[MAX_HP_GAIN_LEVEL];
	int Buffer_tmp[TRIM_NUM+2];
	int i, j;

	pr_warn("%s channels = %d\n", __func__, channels);
	/* open headphone and digital part */
	AudDrv_Clk_On();
	AudDrv_Emi_Clk_On();
	OpenAfeDigitaldl1(true);

	/* AUXADC large scale channel 9- AUXADC_CON2(AUXADC ADC AVG SELECTION[9]) */
	Ana_Set_Reg(MT6353_AUXADC_CON2, 0x0200, 0x0200);

	switch (channels) {
	case AUDIO_OFFSET_TRIM_MUX_HPL:
	case AUDIO_OFFSET_TRIM_MUX_HPR:{
		OpenTrimBufferHardware(true);

		/* HP+SPK */
#ifdef CONFIG_MTK_SPEAKER
		Ana_Set_Reg(MT6353_ZCD_CON4, 0x0505, 0xffff); /* set IV buffer gain to 0dB */
#else
		setLineOutGainZero();
#endif
		break;
		}
	default:
		break;
	}

	/* Get HPL off offset */
	SetSdmLevel(AUDIO_SDM_LEVEL_MUTE);
	/*msleep(1); */
	usleep_range(1 * 1000, 20 * 1000);
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPL);
	setOffsetTrimBufferGain(3);
	EnableTrimbuffer(true);
	/*msleep(1); */
	usleep_range(1 * 1000, 20 * 1000);

	for (j = 0; j < MAX_HP_GAIN_LEVEL; j++) {
		setHpGain(j);
		usleep_range(1 * 1000, 20 * 1000);

		for (i = 0; i < (TRIM_NUM + 2); i++) {
			Buffer_tmp[i] = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, off_counter, 0);
			/* pr_warn("#%d Buffer_off_value_L = %d\n", i, Buffer_tmp[i]); */
		}
		Buffer_offl_value[j] = GetAudioTrimOffsetAverage(Buffer_tmp, TRIM_NUM);
		/* pr_warn("Buffer_offl_value[%d] = %d\n", j, Buffer_offl_value[j]); */
	}

	EnableTrimbuffer(false);

	/* Get HPR off offset */
	SetSdmLevel(AUDIO_SDM_LEVEL_MUTE);
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPR);
	setOffsetTrimBufferGain(3);
	EnableTrimbuffer(true);

	/*msleep(1); */
	usleep_range(1 * 1000, 20 * 1000);

	for (j = 0; j < MAX_HP_GAIN_LEVEL; j++) {
#if 0
		setHpGain(j);
		usleep_range(1 * 1000, 20 * 1000);

		for (i = 0; i < (TRIM_NUM + 2); i++) {
			Buffer_tmp[i] = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, off_counter, 0);
			/* pr_warn("#%d Buffer_off_value_R = %d\n", i, Buffer_tmp[i]); */
		}
		Buffer_offr_value[j] = GetAudioTrimOffsetAverage(Buffer_tmp, TRIM_NUM);
		/* pr_warn("Buffer_offl_value[%d] = %d\n", j, Buffer_offl_value[j]); */
#else
		Buffer_offr_value[j] = 0; /* not used */
#endif
	}

	EnableTrimbuffer(false);

	switch (channels) {
	case AUDIO_OFFSET_TRIM_MUX_HPL:
	case AUDIO_OFFSET_TRIM_MUX_HPR:{
			OpenTrimBufferHardware(false);
			break;
		}
	default:
		break;
	}

	/* calibrate HPL offset trim */
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPL);
	setOffsetTrimBufferGain(3);
	/* EnableTrimbuffer(true); */
	/* msleep(5); */

	switch (channels) {
	case AUDIO_OFFSET_TRIM_MUX_HPL:
	case AUDIO_OFFSET_TRIM_MUX_HPR:{
		/* HP+SPK */
		OpenAnalogHeadphoneSpeaker(true);
#ifdef CONFIG_MTK_SPEAKER
		Ana_Set_Reg(MT6353_ZCD_CON4, 0x0505, 0xffff); /* set IV buffer gain to 0dB */
		Ana_Set_Reg(MT6353_SPK_ANA_CON0, SPK_D_TRIM_INDEX << 11, 0x7800);
		/* Ana_Set_Reg(MT6353_SPK_ANA_CON0, 1 << 11, 0x7800); */
		/* set spk pga gain to SPK_D_TRIM_INDEX */
#else
		setLineOutGainZero();
#endif
		/* set IV buf mux and hp mux */
		Ana_Set_Reg(MT6353_AUDDEC_ANA_CON0, (0x2 << 11) | (0x1 << 9), 0x1e00);
		/* R hp input mux select "Audio playback", L hp input mux select "LoudSPK playback"  */
		Ana_Set_Reg(MT6353_SPK_ANA_CON3, 0x4 << 6, 0x01c0);
		/* Set IV buffer MUX select */
		break;
		}
	default:
		break;
	}
	EnableTrimbuffer(true);

	/*msleep(1); */
	usleep_range(1 * 1000, 20 * 1000);

	for (j = 0; j < MAX_HP_GAIN_LEVEL ; j++) {
		setHpGain(j);
		usleep_range(1 * 1000, 20 * 1000);

		for (i = 0; i < (TRIM_NUM + 2); i++) {
			Buffer_tmp[i]  = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, on_counter, 0);
			/* pr_warn("#%d Buffer_on_value_L = %d\n", i, Buffer_tmp[i]); */
		}
		Buffer_on_value[j] = GetAudioTrimOffsetAverage(Buffer_tmp, TRIM_NUM);
		/*mHplSpkOffset = Buffer_on_value - Buffer_offl_value + Const_DC_OFFSET;*/

		/* HP+SPK */
		mHplSpkOffset[j] = Const_DC_OFFSET - (Buffer_on_value[j] - Buffer_offl_value[j]);

		pr_warn("[inverse] Buffer_on_mHplSpkOffset_L = %d, Buffer_off_value_L = %d, mHplSpkOffset = %d\n",
			Buffer_on_value[j], Buffer_offl_value[j], mHplSpkOffset[j]);
	}
	EnableTrimbuffer(false);

	/* calibrate HPR offset trim */
	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_HPR);

	/* HP+SPK */
	Ana_Set_Reg(MT6353_AUDDEC_ANA_CON0, (0x2 << 11) | (0x1 << 9), 0x1e00);
	/* R hp input mux select "Audio playback", L hp input mux select "LoudSPK playback"  */
	Ana_Set_Reg(MT6353_SPK_ANA_CON3, 0x4 << 6, 0x01c0);
	/* Set IV buffer MUX select */

	setOffsetTrimBufferGain(3);
	EnableTrimbuffer(true);

	/*msleep(1);    */
	usleep_range(1 * 1000, 20 * 1000);

	/* for(j = 0; j < MAX_HP_GAIN_LEVEL ; j++) */
	for (j = 0; j < 1 ; j++) {
		setHpGain(j);
		usleep_range(1 * 1000, 20 * 1000);

		for (i = 0; i < (TRIM_NUM + 2); i++) {
			Buffer_tmp[i]  = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH9, on_counter, 0);
			/* pr_warn("#%d Buffer_on_value_R = %d\n", i, Buffer_tmp[i]); */
		}
		Buffer_on_value[j] = GetAudioTrimOffsetAverage(Buffer_tmp, TRIM_NUM);
		/* pr_warn("Buffer_on_value[%d] = %d\n", j, Buffer_on_value[j]); */

		/* HP+SPK */
		mHprSpkOffset = (mHprOffset - Const_DC_OFFSET) + Const_DC_OFFSET;

		pr_warn("[inverse] Buffer_on_mHprSpkOffset_R = %d, Buffer_off_value_R = %d, mHprSpkOffset = %d\n",
				Buffer_on_value[j], Buffer_offr_value[j], mHprSpkOffset);
	}
	switch (channels) {
	case AUDIO_OFFSET_TRIM_MUX_HPL:
	case AUDIO_OFFSET_TRIM_MUX_HPR:
		/* HP+SPK */
		OpenAnalogHeadphoneSpeaker(false);
		break;
	}

	setOffsetTrimMux(AUDIO_OFFSET_TRIM_MUX_GROUND);
#if 1
	/* HP+SPK */
	/* restore IV buf mux and hp mux */
	Ana_Set_Reg(MT6353_AUDDEC_ANA_CON0, (0x0 << 11) | (0x0 << 9), 0x1e00);
	/* R hp input mux select "Open", L hp input mux select "Open"  */
	Ana_Set_Reg(MT6353_SPK_ANA_CON3, 0x0 << 6, 0x01c0);
	/* Set IV buffer MUX select */
#endif
	EnableTrimbuffer(false);
	OpenAfeDigitaldl1(false);

	SetSdmLevel(AUDIO_SDM_LEVEL_NORMAL);
	AudDrv_Emi_Clk_Off();
	AudDrv_Clk_Off();
}

static int Audio_HpSpkl_Offset_Get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	int i;

	pr_warn("%s\n", __func__);
	AudDrv_Clk_On();
	if (mHplSpkCalibrated == false) {
		GetAudioTrimOffset_HPSPK(AUDIO_OFFSET_TRIM_MUX_HPL);
		SetHprSpkTrimOffset(mHprSpkOffset);
		SetHplSpkTrimOffset(mHplSpkOffset);
		CalculateSPKHP_DCCompenForEachdB();
		mHplSpkCalibrated = true;
		mHprSpkCalibrated = true;
	}

	for (i = 0; i < MAX_HP_GAIN_LEVEL; i++) {
		ucontrol->value.integer.value[i] = mHplSpkOffset[i];
		pr_warn("%s(), mHplSpkOffset[%d] = 0x%x\n", __func__, i, mHplSpkOffset[i]);
	}
	AudDrv_Clk_Off();
#else
	ucontrol->value.integer.value[0] = 2048;
#endif
	return 0;
}

static int Audio_HpSpkl_Offset_Set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	int i;

	pr_warn("%s()\n", __func__);

	for (i = 0; i < MAX_HP_GAIN_LEVEL; i++) {
		mHplSpkOffset[i] = ucontrol->value.integer.value[i];
		pr_warn("%s(), mHplSpkOffset[%d] = 0x%x\n", __func__, i, mHplSpkOffset[i]);
	}
	SetHplSpkTrimOffset(mHplSpkOffset);
	CalculateSPKHP_DCCompenForEachdB();
#else
	mHplSpkOffset = ucontrol->value.integer.value[0];
#endif
	return 0;
}

static int Audio_HpSpkr_Offset_Get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	pr_warn("%s\n", __func__);
	ucontrol->value.integer.value[0] = mHprSpkOffset;
#else
	ucontrol->value.integer.value[0] = 2048;
#endif
	return 0;
}

static int Audio_HpSpkr_Offset_Set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	mHprSpkOffset = ucontrol->value.integer.value[0];
	pr_warn("%s(), mHprSpkOffset = %d\n", __func__, mHprSpkOffset);
	SetHprSpkTrimOffset(mHprSpkOffset);
	CalculateSPKHP_DCCompenForEachdB();
#else
	mHprSpkOffset = ucontrol->value.integer.value[0];
#endif
	return 0;
}

static int Audio_Hpl_Offset_Get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	pr_warn("%s\n", __func__);
	AudDrv_Clk_On();
	if (mHplCalibrated == false) {
		GetAudioTrimOffset_HP(AUDIO_OFFSET_TRIM_MUX_HPL);
		SetHprTrimOffset(mHprOffset);
		SetHplTrimOffset(mHplOffset);
		CalculateDCCompenForEachdB();
		mHplCalibrated = true;
		mHprCalibrated = true;
	}
	ucontrol->value.integer.value[0] = mHplOffset;
	AudDrv_Clk_Off();
#else
	ucontrol->value.integer.value[0] = 2048;
#endif
	return 0;
}

static int Audio_Hpl_Offset_Set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	pr_warn("%s()\n", __func__);
	mHplOffset = ucontrol->value.integer.value[0];
	SetHplTrimOffset(mHplOffset);
	CalculateDCCompenForEachdB();
#else
	mHplOffset = ucontrol->value.integer.value[0];
#endif
	return 0;
}

static int Audio_Hpr_Offset_Get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	ucontrol->value.integer.value[0] = mHprOffset;
#else
	ucontrol->value.integer.value[0] = 2048;
#endif
	return 0;
}

static int Audio_Hpr_Offset_Set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
#ifndef EFUSE_HP_TRIM
	pr_warn("%s()\n", __func__);
	mHprOffset = ucontrol->value.integer.value[0];
	SetHprTrimOffset(mHprOffset);
	CalculateDCCompenForEachdB();
#else
	mHprOffset = ucontrol->value.integer.value[0];
#endif
	return 0;
}

static int audio_dctrim_flag_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), mHplCalibrated = %d, mHprCalibrated = %d, mHplSpkCalibrated = %d, mHprSpkCalibrated = %d\n",
			__func__, mHplCalibrated, mHprCalibrated, mHplSpkCalibrated, mHprSpkCalibrated);
	ucontrol->value.integer.value[0] = mHplCalibrated && mHprCalibrated &&
			mHplSpkCalibrated && mHprSpkCalibrated;
	return 0;
}

static int audio_dctrim_flag_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(audio_dctrim_flag_setting)) {
		pr_err("return -EINVAL\n");
		return -EINVAL;
	}
	mHplCalibrated = ucontrol->value.integer.value[0];
	mHprCalibrated = ucontrol->value.integer.value[0];
	mHplSpkCalibrated = ucontrol->value.integer.value[0];
	mHprSpkCalibrated = ucontrol->value.integer.value[0];
	return 0;
}

static const struct soc_enum Audio_Routing_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_SIDEGEN), DAC_DL_SIDEGEN),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_SIDEGEN_SAMEPLRATE), DAC_DL_SIDEGEN_SAMEPLRATE),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(DAC_DL_SIDEGEN_AMPLITUE), DAC_DL_SIDEGEN_AMPLITUE),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ENABLESIDETONE), ENABLESIDETONE),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ANDROID_AUDIO_MODE), ANDROID_AUDIO_MODE),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(InterModemPcm_ASRC_Switch), InterModemPcm_ASRC_Switch),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_Debug_Setting), Audio_Debug_Setting),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_IPOH_State), Audio_IPOH_State),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Audio_I2S1_Setting), Audio_I2S1_Setting),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_dctrim_flag_setting), audio_dctrim_flag_setting),
};

static const struct snd_kcontrol_new Audio_snd_routing_controls[] = {
	SOC_ENUM_EXT("Audio_SineGen_Switch", Audio_Routing_Enum[0],
		     Audio_SideGen_Get, Audio_SideGen_Set),
	SOC_ENUM_EXT("Audio_SineGen_SampleRate", Audio_Routing_Enum[1],
		     Audio_SideGen_SampleRate_Get, Audio_SideGen_SampleRate_Set),
	SOC_ENUM_EXT("Audio_SineGen_Amplitude", Audio_Routing_Enum[2],
		     Audio_SideGen_Amplitude_Get, Audio_SideGen_Amplitude_Set),
	SOC_ENUM_EXT("Audio_Sidetone_Switch", Audio_Routing_Enum[3],
		     Audio_SideTone_Get, Audio_SideTone_Set),
	SOC_ENUM_EXT("Audio_Mode_Switch", Audio_Routing_Enum[4],
		     Audio_Mode_Get, Audio_Mode_Set),
	SOC_ENUM_EXT("Audio_Dctrim_Flag", Audio_Routing_Enum[9],
			audio_dctrim_flag_get, audio_dctrim_flag_set),
	SOC_SINGLE_MULTI_EXT("Audio HPL_SPK Offset", SND_SOC_NOPM, 0, 0xffff,
			0, MAX_HP_GAIN_LEVEL, Audio_HpSpkl_Offset_Get, Audio_HpSpkl_Offset_Set),
	SOC_SINGLE_EXT("Audio HPR_SPK Offset", SND_SOC_NOPM, 0, 0xffff, 0,
			Audio_HpSpkr_Offset_Get, Audio_HpSpkr_Offset_Set),
	SOC_SINGLE_EXT("Audio HPL Offset", SND_SOC_NOPM, 0, 0xffff, 0,
		       Audio_Hpl_Offset_Get, Audio_Hpl_Offset_Set),
	SOC_SINGLE_EXT("Audio HPR Offset", SND_SOC_NOPM, 0, 0xffff, 0,
		       Audio_Hpr_Offset_Get, Audio_Hpr_Offset_Set),
	SOC_ENUM_EXT("Audio_Debug_Setting", Audio_Routing_Enum[6],
		     AudioDebug_Setting_Get, AudioDebug_Setting_Set),
	SOC_ENUM_EXT("Audio_Ipoh_Setting", Audio_Routing_Enum[7],
		     Audio_Ipoh_Setting_Get, Audio_Ipoh_Setting_Set),
	SOC_ENUM_EXT("Audio_I2S1_Setting", Audio_Routing_Enum[8],
		     AudioI2S1_Setting_Get, AudioI2S1_Setting_Set),
	SOC_SINGLE_EXT("Audio_LowLatency_Debug", SND_SOC_NOPM, 0, 0x20000, 0,
	Audio_LowLatencyDebug_Get, Audio_LowLatencyDebug_Set),
#ifdef AUDIO_DL2_ISR_COPY_SUPPORT
	SOC_DOUBLE_EXT("Audio_DL2_DataTransfer", SND_SOC_NOPM, 0, 1, 65536, 0,
		     NULL, Audio_DL2_DataTransfer),
#endif
};


void EnAble_Anc_Path(int state)
{
	/* 6752 todo? */
	pr_warn("%s not supported in 6752!!!\n ", __func__);

}

static int m_Anc_State = AUDIO_ANC_ON;
static int Afe_Anc_Get(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	ucontrol->value.integer.value[0] = m_Anc_State;
	return 0;
}

static int Afe_Anc_Set(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
	pr_warn("%s()\n", __func__);
	EnAble_Anc_Path(ucontrol->value.integer.value[0]);
	m_Anc_State = ucontrol->value.integer.value[0];
	return 0;
}

/* here start uplink power function */
static const char const *Afe_Anc_function[] = {"ANCON", "ANCOFF"};

static const struct soc_enum Afe_Anc_Enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(Afe_Anc_function), Afe_Anc_function),
};

static const struct snd_kcontrol_new Afe_Anc_controls[] = {
	SOC_ENUM_EXT("Pmic_Anc_Switch", Afe_Anc_Enum[0], Afe_Anc_Get, Afe_Anc_Set),
};

static struct snd_soc_pcm_runtime *pruntimepcm;

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	.mask = 0,
};

static int mtk_routing_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;
	int ret = 0;

	pr_warn("mtk_routing_pcm_open\n");

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);
	if (ret < 0)
		pr_warn("snd_pcm_hw_constraint_integer failed\n");

	/* print for hw pcm information */
	pr_warn("mtk_routing_pcm_open runtime rate = %d channels = %d\n", runtime->rate,
	       runtime->channels);
	if (substream->pcm->device & 1) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	}
	if (substream->pcm->device & 2)
		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pr_warn("SNDRV_PCM_STREAM_PLAYBACK mtkalsa_playback_constraints\n");

	if (err < 0) {
		pr_warn("mtk_routing_pcm_close\n");
		mtk_routing_pcm_close(substream);
		return err;
	}
	pr_warn("mtk_routing_pcm_open return\n");
	return 0;
}

static int mtk_routing_pcm_close(struct snd_pcm_substream *substream)
{
	return 0;
}

static int mtk_routing_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	pr_warn("%s cmd = %d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		break;
	}
	return -EINVAL;
}

static int mtk_routing_pcm_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	count = count << 2;
	return 0;
}

static int mtk_routing_pcm_silence(struct snd_pcm_substream *substream,
				   int channel, snd_pcm_uframes_t pos,
				   snd_pcm_uframes_t count)
{
	pr_warn("mtk_routing_pcm_silence\n");
	return 0;		/* do nothing */
}


static void *dummy_page[2];

static struct page *mtk_routing_pcm_page(struct snd_pcm_substream *substream,
					 unsigned long offset)
{
	pr_warn("dummy_pcm_page\n");
	return virt_to_page(dummy_page[substream->stream]);	/* the same page */
}

static int mtk_routing_pcm_prepare(struct snd_pcm_substream *substream)
{
	pr_warn("mtk_alsa_prepare\n");
	return 0;
}

static int mtk_routing_pcm_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	PRINTK_AUDDRV("mtk_routing_pcm_hw_params\n");
	return ret;
}

static int mtk_routing_pcm_hw_free(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("mtk_routing_pcm_hw_free\n");
	return snd_pcm_lib_free_pages(substream);
}

static struct snd_pcm_ops mtk_afe_ops = {
	.open = mtk_routing_pcm_open,
	.close = mtk_routing_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mtk_routing_pcm_hw_params,
	.hw_free = mtk_routing_pcm_hw_free,
	.prepare = mtk_routing_pcm_prepare,
	.trigger = mtk_routing_pcm_trigger,
	.copy = mtk_routing_pcm_copy,
	.silence = mtk_routing_pcm_silence,
	.page = mtk_routing_pcm_page,
};

static struct snd_soc_platform_driver mtk_soc_routing_platform = {
	.ops = &mtk_afe_ops,
	.pcm_new = mtk_asoc_routing_pcm_new,
	.probe = mtk_afe_routing_platform_probe,
};

static int mtk_afe_routing_probe(struct platform_device *pdev)
{
	pr_warn("mtk_afe_routing_probe\n");

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_ROUTING_PCM);

	pr_warn("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_platform(&pdev->dev, &mtk_soc_routing_platform);
}

static int mtk_asoc_routing_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;

	pruntimepcm = rtd;
	pr_warn("%s\n", __func__);
	return ret;
}

static int mtk_afe_routing_platform_probe(struct snd_soc_platform *platform)
{
	pr_warn("mtk_afe_routing_platform_probe\n");

	/* add  controls */
	snd_soc_add_platform_controls(platform, Audio_snd_routing_controls,
				      ARRAY_SIZE(Audio_snd_routing_controls));
	snd_soc_add_platform_controls(platform, Afe_Anc_controls, ARRAY_SIZE(Afe_Anc_controls));
	Auddrv_Devtree_Init();
	return 0;
}


static int mtk_afe_routing_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/* supend and resume function */
static void SetAudMosiSuspend(bool bEnable)
{
#if defined(CONFIG_MTK_LEGACY)
	int ret;
	unsigned int pin_audmosi, pin_mode_audmosi;
#endif

	if (bEnable == true) {
#if defined(CONFIG_MTK_LEGACY)
		ret = GetGPIO_Info(3, &pin_audmosi, &pin_mode_audmosi);
		if (ret < 0) {
			pr_err("EnableAfe GetGPIO_Info FAIL3!!!\n");
			return;
		}
		mt_set_gpio_mode(pin_audmosi, GPIO_MODE_00);
		mt_set_gpio_dir(pin_audmosi, GPIO_DIR_IN);
		mt_set_gpio_pull_select(pin_audmosi, GPIO_PULL_UP);
		mt_set_gpio_pull_enable(pin_audmosi, GPIO_PULL_ENABLE);
#else
		AudDrv_GPIO_PMIC_Select(false);
#endif
	} else {
#if defined(CONFIG_MTK_LEGACY)
		ret = GetGPIO_Info(3, &pin_audmosi, &pin_mode_audmosi);
		if (ret < 0) {
			pr_err("EnableAfe GetGPIO_Info FAIL3!!!\n");
			return;
		}
		mt_set_gpio_pull_enable(pin_audmosi, GPIO_PULL_DISABLE);
		mt_set_gpio_mode(pin_audmosi, GPIO_MODE_01);
#else
		AudDrv_GPIO_PMIC_Select(true);
#endif
	}
}

static int mtk_routing_pm_ops_suspend(struct device *device)
{
	pr_warn("%s\n", __func__);
	if (get_voice_status() == true || get_voice_md2_status() == true)
		return 0;

	if (AudDrvSuspendStatus == false) {
		AudDrv_Clk_Power_On();
		BackUp_Audio_Register();
		if (ConditionEnterSuspend() == true) {
			SetAudMosiSuspend(true);
			SetAnalogSuspend(true);
			/* clkmux_sel(MT_MUX_AUDINTBUS, 0, "AUDIO"); //select 26M */
			AudDrv_Suspend_Clk_Off();
		}
		AudDrvSuspendStatus = true;
	}
	return 0;
}

static int mtk_pm_ops_suspend_ipo(struct device *device)
{
	pr_warn("%s", __func__);
	AudDrvSuspend_ipoh_Status = true;
	return mtk_routing_pm_ops_suspend(device);
}

static int mtk_routing_pm_ops_resume(struct device *device)
{
	pr_warn("%s\n ", __func__);
	if (AudDrvSuspendStatus == true) {
		AudDrv_Suspend_Clk_On();
		if (ConditionEnterSuspend() == true) {
			Restore_Audio_Register();
			SetAnalogSuspend(false);
			SetAudMosiSuspend(false);
		}
		AudDrvSuspendStatus = false;
	}
	return 0;
}

static int mtk_pm_ops_resume_ipo(struct device *device)
{
	pr_warn("%s", __func__);
	return mtk_routing_pm_ops_resume(device);
}

const struct dev_pm_ops mtk_routing_pm_ops = {
	.suspend = mtk_routing_pm_ops_suspend,
	.resume = mtk_routing_pm_ops_resume,
	.freeze = mtk_pm_ops_suspend_ipo,
	.thaw = mtk_pm_ops_suspend_ipo,
	.poweroff = mtk_pm_ops_suspend_ipo,
	.restore = mtk_pm_ops_resume_ipo,
	.restore_noirq = mtk_pm_ops_resume_ipo,
};

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_routing_of_ids[] = {
	{.compatible = "mediatek,mt_soc_pcm_routing",},
	{}
};
#endif

static struct platform_driver mtk_afe_routing_driver = {
	.driver = {
		   .name = MT_SOC_ROUTING_PCM,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_soc_pcm_routing_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &mtk_routing_pm_ops,
#endif
		   },
	.probe = mtk_afe_routing_probe,
	.remove = mtk_afe_routing_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtkafe_routing_dev;
#endif

static int __init mtk_soc_routing_platform_init(void)
{
	int ret = 0;

	pr_warn("%s\n", __func__);
#ifndef CONFIG_OF
	soc_mtkafe_routing_dev = platform_device_alloc(MT_SOC_ROUTING_PCM, -1);
	if (!soc_mtkafe_routing_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_mtkafe_routing_dev);
	if (ret != 0) {
		platform_device_put(soc_mtkafe_routing_dev);
		return ret;
	}
#endif

	ret = platform_driver_register(&mtk_afe_routing_driver);

	return ret;

}
module_init(mtk_soc_routing_platform_init);

static void __exit mtk_soc_routing_platform_exit(void)
{

	pr_warn("%s\n", __func__);
	platform_driver_unregister(&mtk_afe_routing_driver);
}
module_exit(mtk_soc_routing_platform_exit);

MODULE_DESCRIPTION("afe eouting driver");
MODULE_LICENSE("GPL");
