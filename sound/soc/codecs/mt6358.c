/*
 * mt6358.c  --  mt6358 ALSA SoC audio codec driver
 *
 * Copyright (c) 2017 MediaTek Inc.
 * Author: KaiChieh Chuang <kaichieh.chuang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/debugfs.h>

#include <sound/soc.h>

#include <mach/mtk_pmic_wrap.h>

#include "mt6358.h"

enum {
	AUDIO_ANALOG_VOLUME_HSOUTL,
	AUDIO_ANALOG_VOLUME_HSOUTR,
	AUDIO_ANALOG_VOLUME_HPOUTL,
	AUDIO_ANALOG_VOLUME_HPOUTR,
	AUDIO_ANALOG_VOLUME_LINEOUTL,
	AUDIO_ANALOG_VOLUME_LINEOUTR,
	AUDIO_ANALOG_VOLUME_MICAMP1,
	AUDIO_ANALOG_VOLUME_MICAMP2,
	AUDIO_ANALOG_VOLUME_TYPE_MAX
};

enum {
	MUX_ADC_L,
	MUX_ADC_R,
	MUX_PGA_L,
	MUX_PGA_R,
	MUX_MIC_TYPE,
	MUX_NUM,
};

enum {
	DEVICE_HP,
	DEVICE_LO,
	DEVICE_RCV,
	DEVICE_MIC1,
	DEVICE_MIC2,
	DEVICE_NUM
};

/* Supply widget subseq */
enum {
	/* common */
	SUPPLY_SEQ_CLK_BUF,
	SUPPLY_SEQ_AUD_GLB,
	SUPPLY_SEQ_CLKSQ,
	SUPPLY_SEQ_TOP_CK,
	SUPPLY_SEQ_TOP_CK_LAST,
	SUPPLY_SEQ_AUD_TOP,
	SUPPLY_SEQ_AUD_TOP_LAST,
	SUPPLY_SEQ_AFE,
	/* capture */
	SUPPLY_SEQ_ADC_SUPPLY,
};

#define REG_STRIDE 2

struct mt6358_priv {
	struct device *dev;
	struct regmap *regmap;

	unsigned int dl_rate;
	unsigned int ul_rate;

	int ana_gain[AUDIO_ANALOG_VOLUME_TYPE_MAX];
	unsigned int mux_select[MUX_NUM];

	int dev_counter[DEVICE_NUM];

	struct dentry *debugfs;
};

static DEFINE_SPINLOCK(ana_set_reg_lock);
static unsigned int ana_get_reg(unsigned int offset)
{
	int ret = 0;
	unsigned int data = 0;

	ret = pwrap_read(offset, &data);

	pr_debug("ana_get_reg offset 0x%x, data 0x%x, ret %d\n",
		 offset, data, ret);
	return data;
}

static void ana_set_reg(unsigned int offset,
			unsigned int value,
			unsigned int mask)
{
	int ret = 0;
	unsigned int reg_value;
	unsigned long flags = 0;

	pr_debug("ana_set_reg offset 0x%x, value 0x%x, mask 0x%x\n",
		 offset, value, mask);

	spin_lock_irqsave(&ana_set_reg_lock, flags);
	reg_value = ana_get_reg(offset);
	reg_value &= (~mask);
	reg_value |= (value & mask);
	ret = pwrap_write(offset, reg_value);
	spin_unlock_irqrestore(&ana_set_reg_lock, flags);

	reg_value = ana_get_reg(offset);
	if ((reg_value & mask) != (value & mask))
		pr_warn("ana_set_reg, offset %u, mask = 0x%x ret = %d reg_value = 0x%x\n",
			offset, mask, ret, reg_value);
}

static void set_playback_gpio(struct snd_soc_codec *codec, bool enable)
{
	if (enable) {
		/* set gpio mosi mode */
		snd_soc_update_bits(codec, GPIO_MODE2_CLR,
				    0xffff, 0xffff);
		snd_soc_update_bits(codec, GPIO_MODE2_SET,
				    0xffff, 0x0249);
		snd_soc_update_bits(codec, GPIO_MODE2,
				    0xffff, 0x0249);
	} else {
		/* set pad_aud_*_mosi to GPIO mode and dir input
		 * reason:
		 * pad_aud_dat_mosi*, because the pin is used as boot strap
		 */
		snd_soc_update_bits(codec, GPIO_MODE2_CLR,
				    0xffff, 0xffff);
		snd_soc_update_bits(codec, GPIO_MODE2,
				    0xffff, 0x0000);
		snd_soc_update_bits(codec, GPIO_DIR0,
				    0xf << 8, 0x0);
	}
}

static void set_capture_gpio(struct snd_soc_codec *codec, bool enable)
{
	if (enable) {
		/* set gpio miso mode */
		snd_soc_update_bits(codec, GPIO_MODE3_CLR,
				    0xffff, 0xffff);
		snd_soc_update_bits(codec, GPIO_MODE3_SET,
				    0xffff, 0x0249);
		snd_soc_update_bits(codec, GPIO_MODE3,
				    0xffff, 0x0249);
	} else {
		/* set pad_aud_*_miso to GPIO mode and dir input
		 * reason:
		 * pad_aud_clk_miso, because when playback only the miso_clk
		 * will also have 26m, so will have power leak
		 * pad_aud_dat_miso*, because the pin is used as boot strap
		 */
		snd_soc_update_bits(codec, GPIO_MODE3_CLR,
				    0xffff, 0xffff);
		snd_soc_update_bits(codec, GPIO_MODE3,
				    0xffff, 0x0000);
		snd_soc_update_bits(codec, GPIO_DIR0,
				    0xf << 12, 0x0);
	}
}

/* dl pga gain */
enum {
	DL_GAIN_8DB = 0,
	DL_GAIN_0DB = 8,
	DL_GAIN_N_1DB = 9,
	DL_GAIN_N_10DB = 18,
	DL_GAIN_N_40DB = 0x1f,
};
#define DL_GAIN_N_40DB_REG (DL_GAIN_N_40DB << 7 | DL_GAIN_N_40DB)
#define DL_GAIN_REG_MASK 0x0f9f

/* reg idx for -40dB*/
#define PGA_MINUS_40_DB_REG_VAL 0x1f
#define HP_PGA_MINUS_40_DB_REG_VAL 0x3f
static const char *const dl_pga_gain[] = {
	"8Db", "7Db", "6Db", "5Db", "4Db",
	"3Db", "2Db", "1Db", "0Db", "-1Db",
	"-2Db", "-3Db",	"-4Db", "-5Db", "-6Db",
	"-7Db", "-8Db", "-9Db", "-10Db", "-40Db"
};

static void hp_zcd_disable(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, ZCD_CON0, 0xffff, 0x0000);
}

static void hp_main_output_ramp(struct snd_soc_codec *codec, bool up)
{
	int i = 0, stage = 0;
	int target = 7;

	/* Enable/Reduce HPL/R main output stage step by step */
	for (i = 0; i <= target; i++) {
		stage = up ? i : target - i;
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1,
				    0x7 << 8, stage << 8);
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1,
				    0x7 << 11, stage << 11);
		usleep_range(100, 150);
	}
}

static void hp_aux_feedback_loop_gain_ramp(struct snd_soc_codec *codec, bool up)
{
	int i = 0, stage = 0;

	/* Reduce HP aux feedback loop gain step by step */
	for (i = 0; i <= 0xf; i++) {
		stage = up ? i : 0xf - i;
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9,
				    0xf << 12, stage << 12);
		usleep_range(100, 150);
	}
}

static bool is_valid_hp_pga_idx(int reg_idx)
{
	return (reg_idx >= DL_GAIN_8DB && reg_idx <= DL_GAIN_N_10DB) ||
	       reg_idx == DL_GAIN_N_40DB;
}

static void headset_volume_ramp(struct snd_soc_codec *codec,
				int from, int to)
{
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	int offset = 0, count = 1, reg_idx;

	if (!is_valid_hp_pga_idx(from) || !is_valid_hp_pga_idx(to))
		dev_warn(priv->dev, "%s(), volume index is not valid, from %d, to %d\n",
			 __func__, from, to);


	dev_info(priv->dev, "%s(), from %d, to %d\n",
		 __func__, from, to);

	if (to > from) {
		offset = to - from;
		while (offset > 0) {
			reg_idx = from + count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				snd_soc_update_bits(codec, ZCD_CON2,
						    DL_GAIN_REG_MASK,
						    (reg_idx << 7) | reg_idx);
				usleep_range(200, 300);
			}
			offset--;
			count++;
		}
	} else if (to < from) {
		offset = from - to;
		while (offset > 0) {
			reg_idx = from - count;
			if (is_valid_hp_pga_idx(reg_idx)) {
				snd_soc_update_bits(codec, ZCD_CON2,
						    DL_GAIN_REG_MASK,
						    (reg_idx << 7) | reg_idx);
				usleep_range(200, 300);
			}
			offset--;
			count++;
		}
	}
}

static int dl_pga_get(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int id = kcontrol->id.device;
	int array_size, reg_minus_40db;

	array_size = ARRAY_SIZE(dl_pga_gain);
	reg_minus_40db = PGA_MINUS_40_DB_REG_VAL;

	ucontrol->value.integer.value[0] = priv->ana_gain[id];

	if (ucontrol->value.integer.value[0] == reg_minus_40db)
		ucontrol->value.integer.value[0] = array_size - 1;

	return 0;
}

static int dl_pga_set(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	int index = ucontrol->value.integer.value[0];
	unsigned int id = kcontrol->id.device;
	int array_size, reg_minus_40db;

	dev_info(priv->dev, "%s(), id %d, index %d\n", __func__, id, index);

	array_size = ARRAY_SIZE(dl_pga_gain);
	reg_minus_40db = PGA_MINUS_40_DB_REG_VAL;

	if (index >= array_size) {
		dev_warn(priv->dev, "return -EINVAL\n");
		return -EINVAL;
	}

	if (index == (array_size - 1))
		index = reg_minus_40db;	/* reg idx for -40dB*/

	switch (id) {
	case AUDIO_ANALOG_VOLUME_HPOUTL:
		snd_soc_update_bits(codec, ZCD_CON2, 0x001f, index);
		break;
	case AUDIO_ANALOG_VOLUME_HPOUTR:
		snd_soc_update_bits(codec, ZCD_CON2, 0x0f80, index << 7);
		break;
	case AUDIO_ANALOG_VOLUME_HSOUTL:
		snd_soc_update_bits(codec, ZCD_CON3, 0x001f, index);
		break;
	case AUDIO_ANALOG_VOLUME_LINEOUTL:
		snd_soc_update_bits(codec, ZCD_CON1, 0x001f, index);
		break;
	case AUDIO_ANALOG_VOLUME_LINEOUTR:
		snd_soc_update_bits(codec, ZCD_CON1, 0x0f10, index << 7);
		break;
	default:
		return 0;
	}

	priv->ana_gain[id] = index;
	return 0;
}

static const struct soc_enum dl_pga_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dl_pga_gain), dl_pga_gain),
};

#define MT_SOC_ENUM_EXT_ID(xname, xenum, xhandler_get, xhandler_put, id) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, .device = id,\
	.info = snd_soc_info_enum_double, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = (unsigned long)&xenum }

static const struct snd_kcontrol_new mt6358_snd_controls[] = {
	MT_SOC_ENUM_EXT_ID("Headset_PGAL_GAIN", dl_pga_enum[0],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_HPOUTL),
	MT_SOC_ENUM_EXT_ID("Headset_PGAR_GAIN", dl_pga_enum[0],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_HPOUTR),
	MT_SOC_ENUM_EXT_ID("Handset_PGA_GAIN", dl_pga_enum[0],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_HSOUTL),
	MT_SOC_ENUM_EXT_ID("Lineout_PGAL_GAIN", dl_pga_enum[0],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_LINEOUTL),
	MT_SOC_ENUM_EXT_ID("Lineout_PGAR_GAIN", dl_pga_enum[0],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_LINEOUTR),
};

/* ul pga gain */
static const char *const ul_pga_gain[] = {
	"0Db", "6Db", "12Db", "18Db", "24Db", "30Db"
};

static const struct soc_enum ul_pga_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ul_pga_gain), ul_pga_gain),
};

static int ul_pga_get(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] =
		priv->ana_gain[kcontrol->id.device];
	return 0;
}

static int ul_pga_set(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	int index = ucontrol->value.integer.value[0];
	unsigned int id = kcontrol->id.device;

	dev_info(priv->dev, "%s(), id %d, index %d\n", __func__, id, index);
	if (index > ARRAY_SIZE(ul_pga_gain)) {
		dev_warn(priv->dev, "return -EINVAL\n");
		return -EINVAL;
	}

	switch (id) {
	case AUDIO_ANALOG_VOLUME_MICAMP1:
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    0x7 << 8, index << 8);
		break;
	case AUDIO_ANALOG_VOLUME_MICAMP2:
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    0x7 << 8, index << 8);
		break;
	default:
		return 0;
	}

	priv->ana_gain[id] = index;
	return 0;
}

static const struct snd_kcontrol_new mt6358_snd_ul_controls[] = {
	MT_SOC_ENUM_EXT_ID("Audio_PGA1_Setting", ul_pga_enum[0],
			   ul_pga_get, ul_pga_set,
			   AUDIO_ANALOG_VOLUME_MICAMP1),
	MT_SOC_ENUM_EXT_ID("Audio_PGA2_Setting", ul_pga_enum[0],
			   ul_pga_get, ul_pga_set,
			   AUDIO_ANALOG_VOLUME_MICAMP2),
};

/* MUX */

/* LOL MUX */
static const char * const lo_in_mux_map[] = {
	"Open", "Mute", "Playback", "Test Mode"
};

static int lo_in_mux_map_value[] = {
	0x0, 0x1, 0x2, 0x3,
};

static SOC_VALUE_ENUM_SINGLE_DECL(lo_in_mux_map_enum,
				  AUDDEC_ANA_CON7,
				  RG_AUDLOLMUXINPUTSEL_VAUDP15_SFT,
				  RG_AUDLOLMUXINPUTSEL_VAUDP15_MASK,
				  lo_in_mux_map,
				  lo_in_mux_map_value);

static const struct snd_kcontrol_new lo_in_mux_control =
	SOC_DAPM_ENUM("In Select", lo_in_mux_map_enum);

/*HP MUX */
enum {
	HP_MUX_OPEN = 0,
	HP_MUX_HPSPK,
	HP_MUX_HP,
	HP_MUX_TEST_MODE,
	HP_MUX_MASK = 0x3,
};

static const char * const hp_in_mux_map[] = {
	"Open", "LoudSPK Playback", "Audio Playback", "Test Mode"
};

static int hp_in_mux_map_value[] = {
	HP_MUX_OPEN,
	HP_MUX_HPSPK,
	HP_MUX_HP,
	HP_MUX_TEST_MODE,
};

static SOC_VALUE_ENUM_SINGLE_DECL(hpl_in_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  HP_MUX_MASK,
				  hp_in_mux_map,
				  hp_in_mux_map_value);

static const struct snd_kcontrol_new hpl_in_mux_control =
	SOC_DAPM_ENUM("HPL Select", hpl_in_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(hpr_in_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  HP_MUX_MASK,
				  hp_in_mux_map,
				  hp_in_mux_map_value);

static const struct snd_kcontrol_new hpr_in_mux_control =
	SOC_DAPM_ENUM("HPR Select", hpr_in_mux_map_enum);

/* RCV MUX */
static const char * const rcv_in_mux_map[] = {
	"Open", "Mute", "Voice Playback", "Test Mode"
};

static int rcv_in_mux_map_value[] = {
	0x0, 0x1, 0x2, 0x3,
};

static SOC_VALUE_ENUM_SINGLE_DECL(rcv_in_mux_map_enum,
				  AUDDEC_ANA_CON6,
				  RG_AUDHSMUXINPUTSEL_VAUDP15_SFT,
				  RG_AUDHSMUXINPUTSEL_VAUDP15_MASK,
				  rcv_in_mux_map,
				  rcv_in_mux_map_value);

static const struct snd_kcontrol_new rcv_in_mux_control =
	SOC_DAPM_ENUM("RCV Select", rcv_in_mux_map_enum);

/* DAC In MUX */
static const char * const dac_in_mux_map[] = {
	"Normal Path", "Sgen"
};

static int dac_in_mux_map_value[] = {
	0x0, 0x1,
};

static SOC_VALUE_ENUM_SINGLE_DECL(dac_in_mux_map_enum,
				  PMIC_AFE_TOP_CON0,
				  DL_SINE_ON_SFT,
				  DL_SINE_ON_MASK,
				  dac_in_mux_map,
				  dac_in_mux_map_value);

static const struct snd_kcontrol_new dac_in_mux_control =
	SOC_DAPM_ENUM("DAC Select", dac_in_mux_map_enum);

/* AIF Out MUX */
static SOC_VALUE_ENUM_SINGLE_DECL(aif_out_mux_map_enum,
				  PMIC_AFE_TOP_CON0,
				  UL_SINE_ON_SFT,
				  UL_SINE_ON_MASK,
				  dac_in_mux_map,
				  dac_in_mux_map_value);

static const struct snd_kcontrol_new aif_out_mux_control =
	SOC_DAPM_ENUM("AIF Out Select", aif_out_mux_map_enum);

/* Mic Type MUX */
enum {
	MIC_TYPE_MUX_IDLE = 0,
	MIC_TYPE_MUX_ACC,
	MIC_TYPE_MUX_DMIC,
	MIC_TYPE_MUX_DCC,
	MIC_TYPE_MUX_DCC_ECM_DIFF,
	MIC_TYPE_MUX_DCC_ECM_SINGLE,
	MIC_TYPE_MUX_MASK = 0xf,
};

#define IS_DCC_BASE(x) (x == MIC_TYPE_MUX_DCC || \
			x == MIC_TYPE_MUX_DCC_ECM_DIFF || \
			x == MIC_TYPE_MUX_DCC_ECM_SINGLE)

static const char * const mic_type_mux_map[] = {
	"Idle",
	"ACC",
	"DMIC",
	"DCC",
	"DCC_ECM_DIFF",
	"DCC_ECM_SINGLE",
};

static int mic_type_mux_map_value[] = {
	MIC_TYPE_MUX_IDLE,
	MIC_TYPE_MUX_ACC,
	MIC_TYPE_MUX_DMIC,
	MIC_TYPE_MUX_DCC,
	MIC_TYPE_MUX_DCC_ECM_DIFF,
	MIC_TYPE_MUX_DCC_ECM_SINGLE,
};

static SOC_VALUE_ENUM_SINGLE_DECL(mic_type_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  MIC_TYPE_MUX_MASK,
				  mic_type_mux_map,
				  mic_type_mux_map_value);

static const struct snd_kcontrol_new mic_type_mux_control =
	SOC_DAPM_ENUM("Mic Type Select", mic_type_mux_map_enum);

/* ADC L MUX */
enum {
	ADC_MUX_IDLE = 0,
	ADC_MUX_AIN0,
	ADC_MUX_PREAMPLIFIER,
	ADC_MUX_IDLE1,
	ADC_MUX_MASK = 0x3,
};

static const char * const adc_left_mux_map[] = {
	"Idle", "AIN0", "Left Preamplifier", "Idle_1"
};

static int adc_mux_map_value[] = {
	ADC_MUX_IDLE,
	ADC_MUX_AIN0,
	ADC_MUX_PREAMPLIFIER,
	ADC_MUX_IDLE1,
};

static SOC_VALUE_ENUM_SINGLE_DECL(adc_left_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  ADC_MUX_MASK,
				  adc_left_mux_map,
				  adc_mux_map_value);

static const struct snd_kcontrol_new adc_left_mux_control =
	SOC_DAPM_ENUM("ADC L Select", adc_left_mux_map_enum);

/* ADC R MUX */
static const char * const adc_right_mux_map[] = {
	"Idle", "AIN0", "Right Preamplifier", "Idle_1"
};

static SOC_VALUE_ENUM_SINGLE_DECL(adc_right_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  ADC_MUX_MASK,
				  adc_right_mux_map,
				  adc_mux_map_value);

static const struct snd_kcontrol_new adc_right_mux_control =
	SOC_DAPM_ENUM("ADC R Select", adc_right_mux_map_enum);

/* PGA L MUX */
enum {
	PGA_MUX_NONE = 0,
	PGA_MUX_AIN0,
	PGA_MUX_AIN1,
	PGA_MUX_AIN2,
	PGA_MUX_MASK = 0x3,
};

static const char * const pga_mux_map[] = {
	"None", "AIN0", "AIN1", "AIN2"
};

static int pga_mux_map_value[] = {
	PGA_MUX_NONE,
	PGA_MUX_AIN0,
	PGA_MUX_AIN1,
	PGA_MUX_AIN2,
};

static SOC_VALUE_ENUM_SINGLE_DECL(pga_left_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  PGA_MUX_MASK,
				  pga_mux_map,
				  pga_mux_map_value);

static const struct snd_kcontrol_new pga_left_mux_control =
	SOC_DAPM_ENUM("PGA L Select", pga_left_mux_map_enum);

/* PGA R MUX */
static SOC_VALUE_ENUM_SINGLE_DECL(pga_right_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  PGA_MUX_MASK,
				  pga_mux_map,
				  pga_mux_map_value);

static const struct snd_kcontrol_new pga_right_mux_control =
	SOC_DAPM_ENUM("PGA R Select", pga_right_mux_map_enum);

static int mt_reg_set_clr_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol,
				int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	dev_dbg(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (w->on_val) {
			/* SET REG */
			snd_soc_update_bits(codec, w->reg + REG_STRIDE,
					    0x1 << w->shift,
					    0x1 << w->shift);
		} else {
			/* CLR REG */
			snd_soc_update_bits(codec, w->reg + REG_STRIDE * 2,
					    0x1 << w->shift,
					    0x1 << w->shift);
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (w->off_val) {
			/* SET REG */
			snd_soc_update_bits(codec, w->reg + REG_STRIDE,
					    0x1 << w->shift,
					    0x1 << w->shift);
		} else {
			/* CLR REG */
			snd_soc_update_bits(codec, w->reg + REG_STRIDE * 2,
					    0x1 << w->shift,
					    0x1 << w->shift);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int mt_sgen_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol,
			 int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	dev_dbg(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* sdm audio fifo clock power on */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0006);
		/* scrambler clock on enable */
		snd_soc_update_bits(codec, AFUNC_AUD_CON0, 0xffff, 0xCBA1);
		/* sdm power on */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0003);
		/* sdm fifo enable */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x000B);

		snd_soc_update_bits(codec, AFE_SGEN_CFG0,
				    0xff3f,
				    0x0000);
		snd_soc_update_bits(codec, AFE_SGEN_CFG1,
				    0xffff,
				    0x0001);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DL scrambler disabling sequence */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0000);
		snd_soc_update_bits(codec, AFUNC_AUD_CON0, 0xffff, 0xcba0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_aif_in_event(struct snd_soc_dapm_widget *w,
			   struct snd_kcontrol *kcontrol,
			   int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	dev_info(priv->dev, "%s(), event 0x%x, rate %d\n",
		 __func__, event, priv->dl_rate);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		set_playback_gpio(codec, true);

		/* enable aud_pad RX fifos */
		snd_soc_update_bits(codec, AFE_AUD_PAD_TOP, 0x00ff, 0x0031);

		/* sdm audio fifo clock power on */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0006);
		/* scrambler clock on enable */
		snd_soc_update_bits(codec, AFUNC_AUD_CON0, 0xffff, 0xCBA1);
		/* sdm power on */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0003);
		/* sdm fifo enable */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x000B);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DL scrambler disabling sequence */
		snd_soc_update_bits(codec, AFUNC_AUD_CON2, 0xffff, 0x0000);
		snd_soc_update_bits(codec, AFUNC_AUD_CON0, 0xffff, 0xcba0);

		/* disable aud_pad RX fifos */
		snd_soc_update_bits(codec, AFE_AUD_PAD_TOP, 0x00ff, 0x0000);

		set_playback_gpio(codec, false);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_hp_event(struct snd_soc_dapm_widget *w,
		       struct snd_kcontrol *kcontrol,
		       int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	int device = DEVICE_HP;

	dev_info(priv->dev, "%s(), event 0x%x, dev_counter[DEV_HP] %d, mux %u\n",
		 __func__,
		 event,
		 priv->dev_counter[device],
		 dapm_kcontrol_get_value(w->kcontrols[0]));

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		priv->dev_counter[device]++;
		if (priv->dev_counter[device] > 1)
			break;	/* already enabled, do nothing */
		else if (priv->dev_counter[device] <= 0)
			dev_warn(priv->dev, "%s(), dev_counter[DEV_HP] %d <= 0\n",
				 __func__,
				 priv->dev_counter[device]);

		/* Enable HP main CMFB Switch */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0200);
		/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON10, 0xffff, 0x00a8);

		/* Dsiable HP main CMFB Switch */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0000);
		/* Release HP CMFB pull down */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON10, 0xffff, 0x00a0);


		/* Pull-down HPL/R to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0x8000);

		/* Disable headphone short-circuit protection */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x3000);
		/* Disable handset short-circuit protection */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON6, 0xffff, 0x0010);
		/* Disable linout short-circuit protection */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON7, 0xffff, 0x0010);
		/* Reduce ESD resistance of AU_REFN */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0xc000);
		/* Set HPR/HPL gain as minimum (~ -40dB) */
		snd_soc_update_bits(codec, ZCD_CON2,
				    0xffff, DL_GAIN_N_40DB_REG);

		/* Turn on DA_600K_NCP_VA18 */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON1, 0xffff, 0x0001);
		/* Set NCP clock as 604kHz // 26MHz/43 = 604KHz */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON2, 0xffff, 0x002c);
		/* Toggle RG_DIVCKS_CHG */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON0, 0xffff, 0x0001);
		/* Set NCP soft start mode as default mode: 100us */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON4, 0xffff, 0x0003);
		/* Enable NCP */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON3, 0xffff, 0x0000);
		usleep_range(250, 270);

		/* Enable cap-less LDOs (1.5V) */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x1055, 0x1055);
		/* Enable NV regulator (-1.2V) */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON15, 0xffff, 0x0001);
		usleep_range(100, 120);

		/* Disable AUD_ZCD */
		hp_zcd_disable(codec);

		/* Enable IBIST */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON12, 0xffff, 0x0055);
		/* Set HP DR bias current optimization, 010: 6uA */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON11, 0xffff, 0x4900);
		/* Set HP & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON12, 0xffff, 0x0055);
		/* Set HPP/N STB enhance circuits */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0xc033);

		/* Enable HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x000c);
		/* Enable HP aux feedback loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x003c);
		/* Enable HP aux CMFB loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0c00);
		/* Enable HP driver bias circuits */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x30c0);
		/* Enable HP driver core circuits */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x30f0);
		/* Short HP main output to HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x00fc);

		/* Enable HP main CMFB loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0e00);
		/* Disable HP aux CMFB loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0200);
		/* Enable HP main output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x00ff);
		/* Enable HPR/L main output stage step by step */
		hp_main_output_ramp(codec, true);

		/* Reduce HP aux feedback loop gain */
		hp_aux_feedback_loop_gain_ramp(codec, true);
		/* Disable HP aux feedback loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3fcf);

		/* apply volume setting */
		headset_volume_ramp(codec,
				    DL_GAIN_N_40DB,
				    priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL]);

		/* Disable HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3fc3);
		/* Unshort HP main output to HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3f03);
		usleep_range(100, 120);

		/* Enable AUD_CLK */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON13, 0x1, 0x1);
		/* Enable Audio DAC  */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x30ff);
		/* Enable low-noise mode of DAC */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0xf201);
		usleep_range(100, 120);

		/* Switch HPL MUX to audio DAC */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x32ff);
		/* Switch HPR MUX to audio DAC */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0xffff, 0x3aff);

		/* Apply digital DC compensation value to DAC */
/*		SetDcCompenSation(true);*/

		/* No Pull-down HPL/R to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0x4033);

		break;
	case SND_SOC_DAPM_PRE_PMD:
		priv->dev_counter[device]--;
		if (priv->dev_counter[device] > 0) {
			break;	/* still being used, don't close */
		} else if (priv->dev_counter[device] < 0) {
			dev_warn(priv->dev, "%s(), dev_counter[DEV_HP] %d < 0\n",
				 __func__,
				 priv->dev_counter[device]);
			priv->dev_counter[device] = 0;
			break;
		}

		/* Pull-down HPL/R to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0xc033);

/*		SetDcCompenSation(false);*/

		/* HPR/HPL mux to open */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0x0f00, 0x0000);

		/* Disable low-noise mode of DAC */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0x0001, 0x0000);

		/* Disable Audio DAC */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0x000f, 0x0000);

		/* Disable AUD_CLK */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON13, 0x1, 0x0);

		/* Short HP main output to HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3fc3);
		/* Enable HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3fcf);

		/* decrease HPL/R gain to normal gain step by step */
		headset_volume_ramp(codec,
				    priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL],
				    DL_GAIN_N_40DB);

		/* Enable HP aux feedback loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0xffff, 0x3fff);

		/* Reduce HP aux feedback loop gain */
		hp_aux_feedback_loop_gain_ramp(codec, false);

		/* decrease HPR/L main output stage step by step */
		hp_main_output_ramp(codec, false);

		/* Disable HP main output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0x3, 0x0);

		/* Enable HP aux CMFB loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0e00);

		/* Disable HP main CMFB loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0c00);

		/* Unshort HP main output to HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0x3 << 6, 0x0);

		/* Disable HP driver core circuits */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0x3 << 4, 0x0);

		/* Disable HP driver bias circuits */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0x3 << 6, 0x0);

		/* Disable HP aux CMFB loop, Enable HP main CMFB for HP off state */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON9, 0xffff, 0x0200);

		/* Disable HP aux feedback loop */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0x3 << 4, 0x0);

		/* Disable HP aux output stage */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON1, 0x3 << 2, 0x0);

		/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON10, 0xff, 0xa8);

		/* Disable IBIST */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON12,
				    0x1 << 8, 0x1 << 8);

		/* Disable AUD_ZCD */
		hp_zcd_disable(codec);

		/* Disable NV regulator (-1.2V) */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON15, 0x1, 0x0);
		/* Disable cap-less LDOs (1.5V) */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x1055, 0x0);
		/* Disable NCP */
		snd_soc_update_bits(codec, AUDNCP_CLKDIV_CON3, 0x1, 0x1);

		/* Disable Pull-down HPL/R to AVSS28_AUD */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON2, 0xffff, 0x4033);

		break;
	default:
		break;
	}

	return 0;
}

static int mt_aif_out_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	dev_dbg(priv->dev, "%s(), event 0x%x, rate %d\n",
		__func__, event, priv->ul_rate);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		set_capture_gpio(codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		set_capture_gpio(codec, false);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_adc_supply_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	dev_dbg(priv->dev, "%s(), event 0x%x\n",
		__func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Enable audio ADC CLKGEN  */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON13,
				    0x1 << 5, 0x1 << 5);
		/* ADC CLK from CLKGEN (13MHz) */
		snd_soc_update_bits(codec, AUDENC_ANA_CON3, 0xffff, 0x0000);
		/* Enable  LCLDO_ENC 1P8V */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x2500, 0x0100);
		/* LCLDO_ENC remote sense */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x2500, 0x2500);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* LCLDO_ENC remote sense off */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x2500, 0x0100);
		/* disable LCLDO_ENC 1P8V */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON14, 0x2500, 0x0000);

		/* ADC CLK from CLKGEN (13MHz) */
		snd_soc_update_bits(codec, AUDENC_ANA_CON3, 0xffff, 0x0000);
		/* disable audio ADC CLKGEN  */
		snd_soc_update_bits(codec, AUDDEC_ANA_CON13,
				    0x1 << 5, 0x0 << 5);
		break;
	default:
		break;
	}

	return 0;
}

static int mt6358_enable_amic(struct snd_soc_codec *codec)
{
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];
	unsigned int mux_pga_l = priv->mux_select[MUX_PGA_L];
	unsigned int mux_pga_r = priv->mux_select[MUX_PGA_R];
	int mic_gain_l = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP1];
	int mic_gain_r = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP2];

	dev_info(priv->dev, "%s(), mux, mic %u, pga l %u, pga r %u, mic_gain l %d, r %d\n",
		 __func__, mic_type, mux_pga_l, mux_pga_r,
		 mic_gain_l, mic_gain_r);

	if (IS_DCC_BASE(mic_type)) {
		/* DCC 50k CLK (from 26M) */
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2062);
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2062);
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2060);
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2061);
		snd_soc_update_bits(codec, AFE_DCCLK_CFG1, 0xffff, 0x1105);
	}

	/* mic bias 0 */
	if (mux_pga_l == PGA_MUX_AIN0 || mux_pga_l == PGA_MUX_AIN2 ||
	    mux_pga_r == PGA_MUX_AIN0 || mux_pga_r == PGA_MUX_AIN2) {
		/* Enable MICBIAS0, MISBIAS0 = 1P9V */
		snd_soc_update_bits(codec, AUDENC_ANA_CON9, 0xffff, 0x0021);
	}

	/* mic bias 1 */
	if (mux_pga_l == PGA_MUX_AIN1 || mux_pga_r == PGA_MUX_AIN1) {
		/* Enable MICBIAS1, MISBIAS1 = 2P6V */
		if (mic_type == MIC_TYPE_MUX_DCC_ECM_SINGLE)
			snd_soc_update_bits(codec, AUDENC_ANA_CON10,
					    0xffff, 0x0161);
		else
			snd_soc_update_bits(codec, AUDENC_ANA_CON10,
					    0xffff, 0x0061);
	}

	/* set mic pga gain */
	snd_soc_update_bits(codec, AUDENC_ANA_CON0,
			    RG_AUDPREAMPLGAIN_MASK_SFT,
			    mic_gain_l << RG_AUDPREAMPLGAIN_SFT);
	snd_soc_update_bits(codec, AUDENC_ANA_CON1,
			    RG_AUDPREAMPRGAIN_MASK_SFT,
			    mic_gain_r << RG_AUDPREAMPRGAIN_SFT);

	if (IS_DCC_BASE(mic_type)) {
		/* Audio L/R preamplifier DCC precharge */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0xf8ff, 0x0004);
		snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0xf8ff, 0x0004);
	} else {
		/* reset reg */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0xf8ff, 0x0000);
		snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0xf8ff, 0x0000);
	}

	if (mux_pga_l != PGA_MUX_NONE) {
		/* L preamplifier input sel */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    RG_AUDPREAMPLINPUTSEL_MASK_SFT,
				    mux_pga_l << RG_AUDPREAMPLINPUTSEL_SFT);

		/* L preamplifier enable */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    RG_AUDPREAMPLON_MASK_SFT,
				    0x1 << RG_AUDPREAMPLON_SFT);

		if (IS_DCC_BASE(mic_type)) {
			/* L preamplifier DCCEN */
			snd_soc_update_bits(codec, AUDENC_ANA_CON0,
					    RG_AUDPREAMPLDCCEN_MASK_SFT,
					    0x1 << RG_AUDPREAMPLDCCEN_SFT);
		}

		/* L ADC input sel : L PGA. Enable audio L ADC */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    RG_AUDADCLINPUTSEL_MASK_SFT,
				    ADC_MUX_PREAMPLIFIER << RG_AUDADCLINPUTSEL_SFT);
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    RG_AUDADCLPWRUP_MASK_SFT,
				    0x1 << RG_AUDADCLPWRUP_SFT);
	}

	if (mux_pga_r != PGA_MUX_NONE) {
		/* R preamplifier input sel */
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    RG_AUDPREAMPRINPUTSEL_MASK_SFT,
				    mux_pga_r << RG_AUDPREAMPRINPUTSEL_SFT);

		/* R preamplifier enable */
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    RG_AUDPREAMPRON_MASK_SFT,
				    0x1 << RG_AUDPREAMPRON_SFT);

		if (IS_DCC_BASE(mic_type)) {
			/* R preamplifier DCCEN */
			snd_soc_update_bits(codec, AUDENC_ANA_CON1,
					    RG_AUDPREAMPRDCCEN_MASK_SFT,
					    0x1 << RG_AUDPREAMPRDCCEN_SFT);
		}

		/* R ADC input sel : R PGA. Enable audio R ADC */
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    RG_AUDADCRINPUTSEL_MASK_SFT,
				    ADC_MUX_PREAMPLIFIER << RG_AUDADCRINPUTSEL_SFT);
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    RG_AUDADCRPWRUP_MASK_SFT,
				    0x1 << RG_AUDADCRPWRUP_SFT);
	}

	if (IS_DCC_BASE(mic_type)) {
		usleep_range(100, 150);
		/* Audio L preamplifier DCC precharge off */
		snd_soc_update_bits(codec, AUDENC_ANA_CON0,
				    RG_AUDPREAMPLDCRPECHARGE_MASK_SFT, 0x0);
		/* Audio R preamplifier DCC precharge off */
		snd_soc_update_bits(codec, AUDENC_ANA_CON1,
				    RG_AUDPREAMPRDCRPECHARGE_MASK_SFT, 0x0);

		/* Short body to VCM in PGA */
		snd_soc_update_bits(codec, AUDENC_ANA_CON7, 0xffff, 0x0400);
	}

	/* here to set digital part */
	/* MTKAIF TX format setting */
	snd_soc_update_bits(codec, PMIC_AFE_ADDA_MTKAIF_CFG0, 0xffff, 0x0000);

	/* enable aud_pad TX fifos */
	snd_soc_update_bits(codec, AFE_AUD_PAD_TOP, 0xff00, 0x3100);

	/* UL dmic setting off */
	snd_soc_update_bits(codec, AFE_UL_SRC_CON0_H, 0xffff, 0x0000);

	/* UL turn on */
	snd_soc_update_bits(codec, AFE_UL_SRC_CON0_L, 0xffff, 0x0001);

	return 0;
}

static void mt6358_disable_amic(struct snd_soc_codec *codec)
{
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];
	unsigned int mux_pga_l = priv->mux_select[MUX_PGA_L];
	unsigned int mux_pga_r = priv->mux_select[MUX_PGA_R];
	int mic_gain_l = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP1];
	int mic_gain_r = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP2];

	dev_info(priv->dev, "%s(), mux, mic %u, pga l %u, pga r %u, mic_gain l %d, r %d\n",
		 __func__, mic_type, mux_pga_l, mux_pga_r,
		 mic_gain_l, mic_gain_r);

	/* UL turn off */
	snd_soc_update_bits(codec, AFE_UL_SRC_CON0_L, 0x0001, 0x0000);

	/* disable aud_pad TX fifos */
	snd_soc_update_bits(codec, AFE_AUD_PAD_TOP, 0xff00, 0x3000);

	if (IS_DCC_BASE(mic_type)) {
		/* Disable Short body to VCM in PGA */
		snd_soc_update_bits(codec, AUDENC_ANA_CON7, 0xffff, 0x0000);
	}

	/* L ADC input sel : off, disable L ADC */
	snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0xf000, 0x0000);
	/* L preamplifier DCCEN */
	snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0x1 << 1, 0x0);
	/* L preamplifier input sel : off, L PGA 0 dB gain */
	snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0xfffb, 0x0000);

	/* disable L preamplifier DCC precharge */
	snd_soc_update_bits(codec, AUDENC_ANA_CON0, 0x1 << 2, 0x0);

	/* R ADC input sel : off, disable R ADC */
	snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0xf000, 0x0000);
	/* R preamplifier DCCEN */
	snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0x1 << 1, 0x0);
	/* R preamplifier input sel : off, R PGA 0 dB gain */
	snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0x0ffb, 0x0000);

	/* disable R preamplifier DCC precharge */
	snd_soc_update_bits(codec, AUDENC_ANA_CON1, 0x1 << 2, 0x0);

	/* mic bias */
	/* Disable MICBIAS0, MISBIAS0 = 1P7V */
	snd_soc_update_bits(codec, AUDENC_ANA_CON9, 0xffff, 0x0000);

	/* Disable MICBIAS1, MISBIAS1 = 1P7V */
	if (mic_type == MIC_TYPE_MUX_DCC_ECM_SINGLE)
		snd_soc_update_bits(codec, AUDENC_ANA_CON10, 0xffff, 0x0100);
	else
		snd_soc_update_bits(codec, AUDENC_ANA_CON10, 0xffff, 0x0000);

	if (IS_DCC_BASE(mic_type)) {
		/* dcclk_gen_on=1'b0 */
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2060);
		/* dcclk_pdn=1'b1 */
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2062);
		/* dcclk_ref_ck_sel=2'b00 */
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2062);
		/* dcclk_div=11'b00100000011 */
		snd_soc_update_bits(codec, AFE_DCCLK_CFG0, 0xffff, 0x2062);
	}
}

static int mt6358_enable_dmic(struct snd_soc_codec *codec)
{
	return 0;
}

static void mt6358_disable_dmic(struct snd_soc_codec *codec)
{
}

static int mt_mic_type_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol,
			     int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_dbg(priv->dev, "%s(), event 0x%x, mux %u\n",
		__func__, event, mux);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		priv->mux_select[MUX_MIC_TYPE] = mux;

		if (mux != MIC_TYPE_MUX_DMIC)
			mt6358_enable_amic(codec);
		else
			mt6358_enable_dmic(codec);

		break;
	case SND_SOC_DAPM_POST_PMD:
		if (mux != MIC_TYPE_MUX_DMIC)
			mt6358_disable_amic(codec);
		else
			mt6358_disable_dmic(codec);

		priv->mux_select[MUX_MIC_TYPE] = mux;
		break;
	default:
		break;
	}

	return 0;
}

static int mt_adc_l_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_dbg(priv->dev, "%s(), event = 0x%x, mux %u\n",
		__func__, event, mux);

	priv->mux_select[MUX_ADC_L] = mux;

	return 0;
}

static int mt_adc_r_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_dbg(priv->dev, "%s(), event = 0x%x, mux %u\n",
		__func__, event, mux);

	priv->mux_select[MUX_ADC_R] = mux;

	return 0;
}

static int mt_pga_left_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol,
			     int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_dbg(priv->dev, "%s(), event = 0x%x, mux %u\n",
		__func__, event, mux);

	priv->mux_select[MUX_PGA_L] = mux;

	return 0;
}

static int mt_pga_right_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_dbg(priv->dev, "%s(), event = 0x%x, mux %u\n",
		__func__, event, mux);

	priv->mux_select[MUX_PGA_R] = mux;

	return 0;
}

static int mt_delay_250_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		usleep_range(250, 270);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		usleep_range(250, 270);
		break;
	default:
		break;
	}

	return 0;
}

/* DAPM Widgets */
static const struct snd_soc_dapm_widget mt6358_dapm_widgets[] = {
	/* Global Supply*/
	SND_SOC_DAPM_SUPPLY_S("CLK_BUF", SUPPLY_SEQ_CLK_BUF,
			      DCXO_CW14,
			      RG_XO_AUDIO_EN_M_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDGLB", SUPPLY_SEQ_AUD_GLB,
			      AUDDEC_ANA_CON13,
			      RG_AUDGLB_PWRDN_VA28_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("CLKSQ Audio", SUPPLY_SEQ_CLKSQ,
			      TOP_CLKSQ,
			      RG_CLKSQ_EN_AUD_SFT, 0,
			      mt_reg_set_clr_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUDNCP_CK", SUPPLY_SEQ_TOP_CK,
			      AUD_TOP_CKPDN_CON0,
			      RG_AUDNCP_CK_PDN_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ZCD13M_CK", SUPPLY_SEQ_TOP_CK,
			      AUD_TOP_CKPDN_CON0,
			      RG_ZCD13M_CK_PDN_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUD_CK", SUPPLY_SEQ_TOP_CK_LAST,
			      AUD_TOP_CKPDN_CON0,
			      RG_AUD_CK_PDN_SFT, 1,
			      mt_delay_250_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUDIF_CK", SUPPLY_SEQ_TOP_CK,
			      AUD_TOP_CKPDN_CON0,
			      RG_AUDIF_CK_PDN_SFT, 1, NULL, 0),

	/* Digital Clock */
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_CK_DIV_RST", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      AFE_CK_DIV_RST_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_AFE_CTL", SUPPLY_SEQ_AUD_TOP_LAST,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_AFE_CTL_SFT, 1,
			      mt_delay_250_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_DAC_CTL", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_DAC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_ADC_CTL", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_ADC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_BUCKOSC_CTL", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_BUCKOSC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_I2S_DL", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_I2S_DL_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PWR_CLK", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PWR_CLK_DIS_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PDN_AFE_TESTMODEL", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_AFE_TESTMODEL_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PDN_RESERVED", SUPPLY_SEQ_AUD_TOP,
			      PMIC_AUDIO_TOP_CON0,
			      PDN_RESERVED_SFT, 1, NULL, 0),

	SND_SOC_DAPM_SUPPLY("DL Digital Clock", SND_SOC_NOPM,
			    0, 0, NULL, 0),

	/* AFE ON */
	SND_SOC_DAPM_SUPPLY_S("AFE_ON", SUPPLY_SEQ_AFE,
			      AFE_UL_DL_CON0, AFE_ON_SFT, 0,
			      NULL, 0),

	/* AIF Rx*/
	SND_SOC_DAPM_AIF_IN_E("AIF_RX", "AIF1 Playback", 0, AFE_DL_SRC2_CON0_L,
			      DL_2_SRC_ON_TMP_CTL_PRE_SFT, 0,
			      mt_aif_in_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* DL Supply */
	SND_SOC_DAPM_SUPPLY("DL Power Supply", SND_SOC_NOPM,
			    0, 0, NULL, 0),

	/* DAC */
	SND_SOC_DAPM_MUX("DAC In Mux", SND_SOC_NOPM, 0, 0, &dac_in_mux_control),

	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	/* LOL */
	SND_SOC_DAPM_MUX("LOL Mux", SND_SOC_NOPM, 0, 0, &lo_in_mux_control),

	SND_SOC_DAPM_SUPPLY("LO Stability Enh", AUDDEC_ANA_CON7,
			    RG_LOOUTPUTSTBENH_VAUDP15_SFT, 0, NULL, 0),

	SND_SOC_DAPM_OUT_DRV("LOL Buffer", AUDDEC_ANA_CON7,
			     RG_AUDLOLPWRUP_VAUDP15_SFT, 0, NULL, 0),

	/* Headphone */
	SND_SOC_DAPM_MUX_E("HPL Mux", SND_SOC_NOPM, 0, 0,
			   &hpl_in_mux_control,
			   mt_hp_event,
			   SND_SOC_DAPM_PRE_PMU |
			   SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_MUX_E("HPR Mux", SND_SOC_NOPM, 0, 0,
			   &hpr_in_mux_control,
			   mt_hp_event,
			   SND_SOC_DAPM_PRE_PMU |
			   SND_SOC_DAPM_PRE_PMD),

	/* Receiver */
	SND_SOC_DAPM_MUX("RCV Mux", SND_SOC_NOPM, 0, 0, &rcv_in_mux_control),

	SND_SOC_DAPM_SUPPLY("RCV Stability Enh", AUDDEC_ANA_CON6,
			    RG_HSOUTPUTSTBENH_VAUDP15_SFT, 0, NULL, 0),

	SND_SOC_DAPM_OUT_DRV("RCV Buffer", AUDDEC_ANA_CON6,
			     RG_AUDHSPWRUP_VAUDP15_SFT, 0, NULL, 0),

	/* Outputs */
	SND_SOC_DAPM_OUTPUT("Receiver"),
	SND_SOC_DAPM_OUTPUT("Headphone L"),
	SND_SOC_DAPM_OUTPUT("Headphone R"),
	SND_SOC_DAPM_OUTPUT("LINEOUT L"),

	/* SGEN */
	SND_SOC_DAPM_SUPPLY("SGEN DL Enable", AFE_SGEN_CFG0,
			    C_DAC_EN_CTL_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("SGEN MUTE", AFE_SGEN_CFG0,
			    C_MUTE_SW_CTL_SFT, 1,
			    mt_sgen_event,
			    SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY("SGEN DL SRC", AFE_DL_SRC2_CON0_L,
			    DL_2_SRC_ON_TMP_CTL_PRE_SFT, 0, NULL, 0),
			/* tricky, same reg/bit as "AIF_RX", reconsider */

	SND_SOC_DAPM_INPUT("SGEN DL"),

	/* Uplinks */
	SND_SOC_DAPM_AIF_OUT_E("AIF1TX", "AIF1 Capture", 0,
			       SND_SOC_NOPM, 0, 0,
			       mt_aif_out_event,
			       SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("ADC Supply", SUPPLY_SEQ_ADC_SUPPLY,
			      SND_SOC_NOPM, 0, 0,
			      mt_adc_supply_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* Uplinks MUX */
	SND_SOC_DAPM_MUX("AIF Out Mux", SND_SOC_NOPM, 0, 0,
			 &aif_out_mux_control),

	SND_SOC_DAPM_MUX_E("Mic Type Mux", SND_SOC_NOPM, 0, 0,
			   &mic_type_mux_control,
			   mt_mic_type_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX_E("ADC L Mux", SND_SOC_NOPM, 0, 0,
			   &adc_left_mux_control,
			   mt_adc_l_event,
			   SND_SOC_DAPM_WILL_PMU),
	SND_SOC_DAPM_MUX_E("ADC R Mux", SND_SOC_NOPM, 0, 0,
			   &adc_right_mux_control,
			   mt_adc_r_event,
			   SND_SOC_DAPM_WILL_PMU),

	SND_SOC_DAPM_ADC("ADC L", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC R", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_MUX_E("PGA L Mux", SND_SOC_NOPM, 0, 0,
			   &pga_left_mux_control,
			   mt_pga_left_event,
			   SND_SOC_DAPM_WILL_PMU),
	SND_SOC_DAPM_MUX_E("PGA R Mux", SND_SOC_NOPM, 0, 0,
			   &pga_right_mux_control,
			   mt_pga_right_event,
			   SND_SOC_DAPM_WILL_PMU),

	SND_SOC_DAPM_PGA("PGA L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("PGA R", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* UL input */
	SND_SOC_DAPM_INPUT("AIN0"),
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIN2"),
};

static const struct snd_soc_dapm_route mt6358_dapm_routes[] = {
	/* Capture */
	{"AIF1TX", NULL, "AIF Out Mux"},
		{"AIF1TX", NULL, "CLK_BUF"},
		{"AIF1TX", NULL, "AUDGLB"},
		{"AIF1TX", NULL, "CLKSQ Audio"},

		{"AIF1TX", NULL, "AUD_CK"},
		{"AIF1TX", NULL, "AUDIF_CK"},

		{"AIF1TX", NULL, "AUDIO_TOP_AFE_CTL"},
		{"AIF1TX", NULL, "AUDIO_TOP_ADC_CTL"},
		{"AIF1TX", NULL, "AUDIO_TOP_BUCKOSC_CTL"},
		{"AIF1TX", NULL, "AUDIO_TOP_PWR_CLK"},
		{"AIF1TX", NULL, "AUDIO_TOP_PDN_RESERVED"},
		{"AIF1TX", NULL, "AUDIO_TOP_CK_DIV_RST"},
		{"AIF1TX", NULL, "AUDIO_TOP_I2S_DL"},

		{"AIF1TX", NULL, "AFE_ON"},

	{"AIF Out Mux", NULL, "Mic Type Mux"},

	{"Mic Type Mux", "ACC", "ADC L"},
	{"Mic Type Mux", "ACC", "ADC R"},
	{"Mic Type Mux", "DCC", "ADC L"},
	{"Mic Type Mux", "DCC", "ADC R"},
	{"Mic Type Mux", "DCC_ECM_DIFF", "ADC L"},
	{"Mic Type Mux", "DCC_ECM_DIFF", "ADC R"},
	{"Mic Type Mux", "DCC_ECM_SINGLE", "ADC L"},
	{"Mic Type Mux", "DCC_ECM_SINGLE", "ADC R"},

	{"ADC L", NULL, "ADC L Mux"},
		{"ADC L", NULL, "ADC Supply"},
	{"ADC R", NULL, "ADC R Mux"},
		{"ADC R", NULL, "ADC Supply"},

	{"ADC L Mux", "Left Preamplifier", "PGA L"},

	{"ADC R Mux", "Right Preamplifier", "PGA R"},

	{"PGA L", NULL, "PGA L Mux"},
	{"PGA R", NULL, "PGA R Mux"},

	{"PGA L Mux", "AIN0", "AIN0"},
	{"PGA L Mux", "AIN1", "AIN1"},
	{"PGA L Mux", "AIN2", "AIN2"},

	{"PGA R Mux", "AIN0", "AIN0"},
	{"PGA R Mux", "AIN1", "AIN1"},
	{"PGA R Mux", "AIN2", "AIN2"},

	/* DL Supply */
	{"DL Power Supply", NULL, "CLK_BUF"},
	{"DL Power Supply", NULL, "AUDGLB"},
	{"DL Power Supply", NULL, "CLKSQ Audio"},

	{"DL Power Supply", NULL, "AUDNCP_CK"},
	{"DL Power Supply", NULL, "ZCD13M_CK"},
	{"DL Power Supply", NULL, "AUD_CK"},
	{"DL Power Supply", NULL, "AUDIF_CK"},

	/* DL Digital Supply */
	{"DL Digital Clock", NULL, "AUDIO_TOP_AFE_CTL"},
	{"DL Digital Clock", NULL, "AUDIO_TOP_DAC_CTL"},
	{"DL Digital Clock", NULL, "AUDIO_TOP_PWR_CLK"},

	{"DL Digital Clock", NULL, "AFE_ON"},

	{"AIF_RX", NULL, "DL Digital Clock"},

	/* DL Path */
	{"DAC In Mux", "Normal Path", "AIF_RX"},

	{"DAC In Mux", "Sgen", "SGEN DL"},
		{"SGEN DL", NULL, "SGEN DL SRC"},
		{"SGEN DL", NULL, "SGEN MUTE"},
		{"SGEN DL", NULL, "SGEN DL Enable"},
		{"SGEN DL", NULL, "DL Digital Clock"},
		{"SGEN DL", NULL, "AUDIO_TOP_PDN_AFE_TESTMODEL"},

	{"DACL", NULL, "DAC In Mux"},
		{"DACL", NULL, "DL Power Supply"},

	{"DACR", NULL, "DAC In Mux"},
		{"DACR", NULL, "DL Power Supply"},

	/* Lineout Path */
	{"LOL Mux", "Playback", "DACL"},

	{"LOL Buffer", NULL, "LOL Mux"},
		{"LOL Buffer", NULL, "LO Stability Enh"},

	{"LINEOUT L", NULL, "LOL Buffer"},

	/* Headphone Path */
	{"HPL Mux", "Audio Playback", "DACL"},
	{"HPR Mux", "Audio Playback", "DACR"},

	{"Headphone L", NULL, "HPL Mux"},
	{"Headphone R", NULL, "HPR Mux"},

	/* Receiver Path */
	{"RCV Mux", "Voice Playback", "DACL"},

	{"RCV Buffer", NULL, "RCV Mux"},
		{"RCV Buffer", NULL, "RCV Stability Enh"},

	{"Receiver", NULL, "RCV Buffer"},
};

static int mt6358_codec_dai_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int rate = params_rate(params);


	dev_info(priv->dev, "%s(), substream->stream %d, rate %d, number %d\n",
		 __func__,
		 substream->stream,
		 rate,
		 substream->number);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		priv->dl_rate = rate;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		priv->ul_rate = rate;

	return 0;
}

static const struct snd_soc_dai_ops mt6358_codec_dai_ops = {
	.hw_params = mt6358_codec_dai_hw_params,
};

#define MT6358_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_U16_BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_U24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE |\
			SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_U32_BE)

static struct snd_soc_dai_driver mt6358_dai_driver[] = {
	{
		.name = "mt6358-snd-codec-aif1",
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = MT6358_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000,
			.formats = MT6358_FORMATS,
		},
		.ops = &mt6358_codec_dai_ops,
	},
};

static int mt6358_codec_init_reg(struct snd_soc_codec *codec)
{
	int ret = 0;

	/* enable clk buf */
	snd_soc_update_bits(codec, DCXO_CW14,
			    0x1 << RG_XO_AUDIO_EN_M_SFT,
			    0x1 << RG_XO_AUDIO_EN_M_SFT);

	/* set those not controlled by dapm widget */

	/* Disable CLKSQ 26MHz */
	snd_soc_update_bits(codec, TOP_CLKSQ_CLR, 0x1, 0x1);
	snd_soc_update_bits(codec, TOP_CLKSQ, 0x1, 0x0);
	/* disable AUDGLB */
	snd_soc_update_bits(codec, AUDDEC_ANA_CON13, 0x1 << 4, 0x1 << 4);
	/* Turn off AUDNCP_CLKDIV engine clock,Turn off AUD 26M */
	snd_soc_update_bits(codec, AUD_TOP_CKPDN_CON0, 0x66, 0x66);
	/* set pdn for golden setting */
	snd_soc_update_bits(codec, PMIC_AUDIO_TOP_CON0, 0xe0ff, 0xe0ff);

	/* Disable HeadphoneL/HeadphoneR short circuit protection */
	snd_soc_update_bits(codec, AUDDEC_ANA_CON0, 0x3 << 12, 0x3 << 12);
	/* Disable voice short circuit protection */
	snd_soc_update_bits(codec, AUDDEC_ANA_CON6, 0x1 << 4, 0x1 << 4);
	/* disable LO buffer left short circuit protection */
	snd_soc_update_bits(codec, AUDDEC_ANA_CON7, 0x1 << 4, 0x1 << 4);

	/* gpio miso driving set to default 6mA, 0xcccc */
	snd_soc_update_bits(codec, DRV_CON3, 0xffff, 0xcccc);

	/* set gpio */
	set_playback_gpio(codec, false);
	set_capture_gpio(codec, false);

	/* disable clk buf */
	snd_soc_update_bits(codec, DCXO_CW14,
			    0x1 << RG_XO_AUDIO_EN_M_SFT,
			    0x0);

	return ret;
}

static int mt6358_codec_probe(struct snd_soc_codec *codec)
{
	struct mt6358_priv *priv = snd_soc_codec_get_drvdata(codec);

	/* add codec controls */
	snd_soc_add_codec_controls(codec,
				   mt6358_snd_controls,
				   ARRAY_SIZE(mt6358_snd_controls));
	snd_soc_add_codec_controls(codec,
				   mt6358_snd_ul_controls,
				   ARRAY_SIZE(mt6358_snd_ul_controls));

	mt6358_codec_init_reg(codec);

	priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL] = 8;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTR] = 8;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;

	return 0;
}

static bool is_writeable_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static bool is_volatile_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static bool is_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case DRV_CON3:
	case GPIO_DIR0:
	case GPIO_MODE2:
	case GPIO_MODE3:
	case TOP_CKPDN_CON0:
	case TOP_CKHWEN_CON0:
	case TOP_CLKSQ:
	case OTP_CON0:
	case OTP_CON8:
	case OTP_CON11:
	case OTP_CON12:
	case OTP_CON13:
	case DCXO_CW14:
	case AUXADC_CON10:
		return true;
	default:
		break;
	};

	if (reg >= AUD_TOP_ID && reg <= ACCDET_ELR2)
		return true;

	return false;
}

static int reg_read(void *context, unsigned int reg, unsigned int *val)
{
	*val = ana_get_reg(reg);
	return 0;
}

static int reg_write(void *context, unsigned int reg, unsigned int val)
{
	ana_set_reg(reg, val, 0xffff);
	return 0;
}

static const struct regmap_config mt6358_regmap = {
	.reg_bits = 16,
	.val_bits = 16,
	.reg_stride = REG_STRIDE,

	.max_register = MT6358_MAX_REGISTER,
	.writeable_reg = is_writeable_reg,
	.volatile_reg = is_volatile_reg,
	.readable_reg = is_readable_reg,

	.reg_read = reg_read,
	.reg_write = reg_write,

	.cache_type = REGCACHE_NONE,
};

static struct snd_soc_codec_driver mt6358_soc_codec_driver = {
	.probe = mt6358_codec_probe,

	.dapm_widgets = mt6358_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt6358_dapm_widgets),
	.dapm_routes = mt6358_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(mt6358_dapm_routes),
};

static ssize_t mt6358_debugfs_write(struct file *f, const char __user *buf,
				    size_t count, loff_t *offset)
{
#define MAX_DEBUG_WRITE_INPUT 256
	int ret = 0;
	char input[MAX_DEBUG_WRITE_INPUT];
	char *token1 = NULL;
	char *token2 = NULL;
	char *temp = NULL;

	unsigned long reg_addr = 0;
	unsigned long reg_value = 0;
	char delim[] = " ,";

	memset((void *)input, 0, MAX_DEBUG_WRITE_INPUT);

	if (count > MAX_DEBUG_WRITE_INPUT)
		count = MAX_DEBUG_WRITE_INPUT;

	if (copy_from_user(input, buf, count))
		pr_warn("%s(), copy_from_user fail, count = %zu, temp = %s\n",
			__func__, count, input);

	temp = kstrndup(input, MAX_DEBUG_WRITE_INPUT, GFP_KERNEL);
	if (!temp) {
		pr_warn("%s(), kstrdup fail\n", __func__);
		goto exit;
	}

	pr_debug("%s(), copy_from_user, count = %zu, temp = %s, pointer = %p\n",
		 __func__, count, temp, temp);
	token1 = strsep(&temp, delim);
	pr_debug("token1 = %s\n", token1);
	token2 = strsep(&temp, delim);
	pr_debug("token2 = %s\n", token2);

	if ((token1 != NULL) && (token2 != NULL)) {
		ret = kstrtoul(token1, 16, &reg_addr);
		ret = kstrtoul(token2, 16, &reg_value);
		pr_debug("%s(), reg_addr = 0x%x, reg_value = 0x%x\n", __func__,
			 (unsigned int)reg_addr, (unsigned int)reg_value);
		ana_set_reg(reg_addr, reg_value, 0xffffffff);
		reg_value = ana_get_reg(reg_addr);
		pr_debug("%s(), reg_addr = 0x%x, reg_value = 0x%x\n", __func__,
			 (unsigned int)reg_addr, (unsigned int)reg_value);
	} else {
		pr_debug("token1 or token2 is NULL!\n");
	}

	kfree(temp);
exit:
	return count;
}

static const struct file_operations mt6358_debugfs_ops = {
	.write = mt6358_debugfs_write,
};

static int mt6358_platform_driver_probe(struct platform_device *pdev)
{
	struct mt6358_priv *priv;

	priv = devm_kzalloc(&pdev->dev,
			    sizeof(struct mt6358_priv),
			    GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, priv);

	priv->dev = &pdev->dev;

	priv->regmap = devm_regmap_init(&pdev->dev, NULL, NULL, &mt6358_regmap);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	/* create debugfs file */
	priv->debugfs = debugfs_create_file("mtksocanaaudio",
					    S_IFREG | S_IRUGO, NULL,
					    NULL, &mt6358_debugfs_ops);

	dev_info(priv->dev, "%s(), dev name %s\n",
		__func__, dev_name(&pdev->dev));

	return snd_soc_register_codec(&pdev->dev,
				      &mt6358_soc_codec_driver,
				      mt6358_dai_driver,
				      ARRAY_SIZE(mt6358_dai_driver));
}

static int mt6358_platform_driver_remove(struct platform_device *pdev)
{
	struct mt6358_priv *priv = dev_get_drvdata(&pdev->dev);

	dev_info(&pdev->dev, "%s()\n", __func__);

	debugfs_remove(priv->debugfs);

	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static const struct of_device_id mt6358_of_match[] = {
	{.compatible = "mediatek,mt6358-sound",},
	{}
};
MODULE_DEVICE_TABLE(of, mt6358_of_match);

static struct platform_driver mt6358_platform_driver = {
	.driver = {
		.name = "mt6358-sound",
		.of_match_table = mt6358_of_match,
	},
	.probe = mt6358_platform_driver_probe,
	.remove = mt6358_platform_driver_remove,
};

static int __init mt6358_module_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mt6358_platform_driver);
	if (ret) {
		pr_err("%s(), platform_driver_register failed, ret %d\n",
		       __func__, ret);
	}

	return ret;
}

module_init(mt6358_module_init);

static void __exit mt6358_module_exit(void)
{
	platform_driver_unregister(&mt6358_platform_driver);
}

module_exit(mt6358_module_exit);

/* Module information */
MODULE_DESCRIPTION("MT6358 ALSA SoC codec driver");
MODULE_AUTHOR("KaiChieh Chuang <kaichieh.chuang@mediatek.com>");
MODULE_LICENSE("GPL v2");
