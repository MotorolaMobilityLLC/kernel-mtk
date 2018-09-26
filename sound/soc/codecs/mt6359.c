// SPDX-License-Identifier: GPL-2.0
//
// mt6359.c  --  mt6359 ALSA SoC audio codec driver
//
// Copyright (c) 2018 MediaTek Inc.
// Author: KaiChieh Chuang <kaichieh.chuang@mediatek.com>

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include <sound/soc.h>

#include <mach/upmu_hw.h>

#ifdef CONFIG_MTK_ACCDET
#include "accdet.h"
#endif

#ifdef CONFIG_MTK_AUXADC_INTF
#include <mach/mtk_pmic.h>
#include <mt-plat/mtk_auxadc_intf.h>
#endif

#ifndef CONFIG_MTK_PMIC_WRAP	/* y: use regmap, else use legacy api */
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
#include <mach/mtk_pmic_wrap.h>
#endif
#endif

#ifdef CONFIG_MTK_PMIC_WRAP
#include <linux/soc/mediatek/pmic_wrap.h>
#endif

#include "mt6359.h"

enum {
	MT6359_AIF_1 = 0,	/* dl: hp, rcv, hp+lo */
	MT6359_AIF_2,		/* dl: lo only */
	MT6359_AIF_VOW,
	MT6359_AIF_NUM,
};

enum {
	AUDIO_ANALOG_VOLUME_HSOUTL,
	AUDIO_ANALOG_VOLUME_HSOUTR,
	AUDIO_ANALOG_VOLUME_HPOUTL,
	AUDIO_ANALOG_VOLUME_HPOUTR,
	AUDIO_ANALOG_VOLUME_LINEOUTL,
	AUDIO_ANALOG_VOLUME_LINEOUTR,
	AUDIO_ANALOG_VOLUME_MICAMP1,
	AUDIO_ANALOG_VOLUME_MICAMP2,
	AUDIO_ANALOG_VOLUME_MICAMP3,
	AUDIO_ANALOG_VOLUME_TYPE_MAX
};

enum {
	MUX_MIC_TYPE_0,	/* ain0/2, micbias 0 */
	MUX_MIC_TYPE_1,	/* ain1, micbias 1 */
	MUX_MIC_TYPE_2,	/* ain3, micbias 2 */
	MUX_PGA_L,
	MUX_PGA_R,
	MUX_PGA_3,
	MUX_HP_L,
	MUX_HP_R,
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

enum {
	HP_GAIN_CTL_ZCD = 0,
	HP_GAIN_CTL_NLE,
	HP_GAIN_CTL_NUM,
};

/* Supply widget subseq */
enum {
	/* common */
	SUPPLY_SEQ_CLK_BUF,
	SUPPLY_SEQ_AUD_GLB,
	SUPPLY_SEQ_DL_GPIO,
	SUPPLY_SEQ_UL_GPIO,
	SUPPLY_SEQ_HP_PULL_DOWN,
	SUPPLY_SEQ_CLKSQ,
	SUPPLY_SEQ_ADC_CLKGEN,
	SUPPLY_SEQ_VOW_AUD_LPW,
	SUPPLY_SEQ_AUD_VOW,
	SUPPLY_SEQ_VOW_CLK,
	SUPPLY_SEQ_VOW_LDO,
	SUPPLY_SEQ_TOP_CK,
	SUPPLY_SEQ_TOP_CK_LAST,
	SUPPLY_SEQ_DCC_CLK,
	SUPPLY_SEQ_MIC_BIAS,
	SUPPLY_SEQ_DMIC,
	SUPPLY_SEQ_AUD_TOP,
	SUPPLY_SEQ_AUD_TOP_LAST,
	SUPPLY_SEQ_DL_SDM_FIFO_CLK,
	SUPPLY_SEQ_DL_SDM,
	SUPPLY_SEQ_DL_NCP,
	SUPPLY_SEQ_AFE,
	/* playback */
	SUPPLY_SEQ_DL_SRC,
	SUPPLY_SEQ_DL_ESD_RESIST,
	SUPPLY_SEQ_HP_DAMPING_OFF_RESET_CMFB,
	SUPPLY_SEQ_HP_MUTE,
	SUPPLY_SEQ_DL_LDO_REMOTE_SENSE,
	SUPPLY_SEQ_DL_LDO,
	SUPPLY_SEQ_DL_NV,
	SUPPLY_SEQ_DL_IBIST,
	/* capture */
	SUPPLY_SEQ_UL_PGA,
	SUPPLY_SEQ_UL_ADC,
	SUPPLY_SEQ_UL_MTKAIF,
	SUPPLY_SEQ_UL_SRC_DMIC,
	SUPPLY_SEQ_UL_SRC,
};

enum {
	CH_L = 0,
	CH_R,
	NUM_CH,
};

/* Auxadc average resolution */
enum {
	AUXADC_AVG_1 = 0,
	AUXADC_AVG_4,
	AUXADC_AVG_8,
	AUXADC_AVG_16,
	AUXADC_AVG_32,
	AUXADC_AVG_64,
	AUXADC_AVG_128,
	AUXADC_AVG_256,
};

enum {
	DBG_DCTRIM_BYPASS_4POLE = 0x1 << 0,
	DBG_DCTRIM_4POLE_LOG = 0x1 << 1,
};

/* dl bias */
#define DRBIAS_MASK 0x7
#define DRBIAS_HP_SFT (RG_AUDBIASADJ_0_VAUDP32_SFT + 0)
#define DRBIAS_HP_MASK_SFT (DRBIAS_MASK << DRBIAS_HP_SFT)
#define DRBIAS_HS_SFT (RG_AUDBIASADJ_0_VAUDP32_SFT + 3)
#define DRBIAS_HS_MASK_SFT (DRBIAS_MASK << DRBIAS_HS_SFT)
#define DRBIAS_LO_SFT (RG_AUDBIASADJ_0_VAUDP32_SFT + 6)
#define DRBIAS_LO_MASK_SFT (DRBIAS_MASK << DRBIAS_LO_SFT)

enum {
	DRBIAS_4UA = 0,
	DRBIAS_5UA,
	DRBIAS_6UA,
	DRBIAS_7UA,
	DRBIAS_8UA,
	DRBIAS_9UA,
	DRBIAS_10UA,
	DRBIAS_11UA,
};

#define IBIAS_MASK 0x3
#define IBIAS_HP_SFT (RG_AUDBIASADJ_1_VAUDP32_SFT + 0)
#define IBIAS_HP_MASK_SFT (IBIAS_MASK << IBIAS_HP_SFT)
#define IBIAS_HS_SFT (RG_AUDBIASADJ_1_VAUDP32_SFT + 2)
#define IBIAS_HS_MASK_SFT (IBIAS_MASK << IBIAS_HS_SFT)
#define IBIAS_LO_SFT (RG_AUDBIASADJ_1_VAUDP32_SFT + 4)
#define IBIAS_LO_MASK_SFT (IBIAS_MASK << IBIAS_LO_SFT)
#define IBIAS_ZCD_SFT (RG_AUDBIASADJ_1_VAUDP32_SFT + 6)
#define IBIAS_ZCD_MASK_SFT (IBIAS_MASK << IBIAS_ZCD_SFT)

enum {
	IBIAS_4UA = 0,
	IBIAS_5UA,
	IBIAS_6UA,
	IBIAS_7UA,
};

enum {
	IBIAS_ZCD_3UA = 0,
	IBIAS_ZCD_4UA,
	IBIAS_ZCD_5UA,
	IBIAS_ZCD_6UA,
};

enum {
	MIC_BIAS_1P7 = 0,
	MIC_BIAS_1P8,
	MIC_BIAS_1P9,
	MIC_BIAS_2P0,
	MIC_BIAS_2P1,
	MIC_BIAS_2P5,
	MIC_BIAS_2P6,
	MIC_BIAS_2P7,
};

struct dc_trim_data {
	bool calibrated;
	int hp_offset[NUM_CH];
	int pre_comp_value[NUM_CH];
	int mic_vinp_mv;
};

struct mt6359_priv {
	struct device *dev;
	struct regmap *regmap;

	unsigned int dl_rate[MT6359_AIF_NUM];
	unsigned int ul_rate[MT6359_AIF_NUM];

	int ana_gain[AUDIO_ANALOG_VOLUME_TYPE_MAX];
	unsigned int mux_select[MUX_NUM];

	int dev_counter[DEVICE_NUM];

	int hp_gain_ctl;
	int hp_hifi_mode;

	struct mt6359_codec_ops ops;
	struct dc_trim_data dc_trim;
	int hp_plugged;

	/* hp impedance */
	int hp_impedance;
	int hp_current_calibrate_val;

	int mtkaif_protocol;

	struct dentry *debugfs;
	unsigned int debug_flag;

	/* vow control */
	int vow_enable;
	int reg_afe_vow_cfg0;
	int reg_afe_vow_cfg1;
	int reg_afe_vow_cfg2;
	int reg_afe_vow_cfg3;
	int reg_afe_vow_cfg4;
	int reg_afe_vow_cfg5;
	int reg_afe_vow_periodic;
};

/* static function declaration */
static int apply_dc_compensation(struct mt6359_priv *priv, bool enable);
static int dc_trim_thread(void *arg);

int mt6359_set_codec_ops(struct snd_soc_component *cmpnt,
			 struct mt6359_codec_ops *ops)
{
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	priv->ops.enable_dc_compensation = ops->enable_dc_compensation;
	priv->ops.set_lch_dc_compensation = ops->set_lch_dc_compensation;
	priv->ops.set_rch_dc_compensation = ops->set_rch_dc_compensation;
	return 0;
}

int mt6359_set_mtkaif_protocol(struct snd_soc_component *cmpnt,
			       int mtkaif_protocol)
{
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	priv->mtkaif_protocol = mtkaif_protocol;
	return 0;
}

static void playback_gpio_set(struct mt6359_priv *priv)
{
	/* set gpio mosi mode, clk / data mosi */
	regmap_write(priv->regmap, MT6359_GPIO_MODE2_CLR, 0x0ffe);
	regmap_write(priv->regmap, MT6359_GPIO_MODE2_SET, 0x0249);
	regmap_write(priv->regmap, MT6359_GPIO_MODE2, 0x0249);

	/* sync mosi */
	regmap_write(priv->regmap, MT6359_GPIO_MODE3_CLR, 0x6);
	regmap_write(priv->regmap, MT6359_GPIO_MODE3_SET, 0x1);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3,
			   0x7, 0x1);
}

static void playback_gpio_reset(struct mt6359_priv *priv)
{
	/* set pad_aud_*_mosi to GPIO mode and dir input
	 * reason:
	 * pad_aud_dat_mosi*, because the pin is used as boot strap
	 * don't clean clk/sync, for mtkaif protocol 2
	 */
	regmap_write(priv->regmap, MT6359_GPIO_MODE2_CLR, 0x0ff8);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE2,
			   0x0ff8, 0x0000);
	regmap_update_bits(priv->regmap, MT6359_GPIO_DIR0,
			   0x7 << 9, 0x0);
}

static void capture_gpio_set(struct mt6359_priv *priv)
{
	/* set gpio miso mode */
	regmap_write(priv->regmap, MT6359_GPIO_MODE3_CLR, 0x0ff8);
	regmap_write(priv->regmap, MT6359_GPIO_MODE3_SET, 0x0490);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3,
			   0x0ff8, 0x0490);

	regmap_write(priv->regmap, MT6359_GPIO_MODE4_CLR, 0x003f);
	regmap_write(priv->regmap, MT6359_GPIO_MODE4_SET, 0x0009);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE4,
			   0x03f, 0x0009);
}

static void capture_gpio_reset(struct mt6359_priv *priv)
{
	/* set pad_aud_*_miso to GPIO mode and dir input
	 * reason:
	 * pad_aud_clk_miso, because when playback only the miso_clk
	 * will also have 26m, so will have power leak
	 * pad_aud_dat_miso*, because the pin is used as boot strap
	 */
	regmap_write(priv->regmap, MT6359_GPIO_MODE3_CLR, 0x0ff8);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3,
			   0x0ff8, 0x0000);

	regmap_write(priv->regmap, MT6359_GPIO_MODE4_CLR, 0x003f);
	regmap_update_bits(priv->regmap, MT6359_GPIO_MODE4,
			   0x003f, 0x0000);

	regmap_update_bits(priv->regmap, MT6359_GPIO_DIR0,
			   0x7 << 13, 0x0);
	regmap_update_bits(priv->regmap, MT6359_GPIO_DIR1,
			   0x3 << 0, 0x0);
}

/* use only when not govern by DAPM */
static int mt6359_set_dcxo(struct mt6359_priv *priv, bool enable)
{
	regmap_update_bits(priv->regmap, MT6359_DCXO_CW12,
			   0x1 << RG_XO_AUDIO_EN_M_SFT,
			   (enable ? 1 : 0) << RG_XO_AUDIO_EN_M_SFT);
	return 0;
}

/* use only when not govern by DAPM */
static int mt6359_set_clksq(struct mt6359_priv *priv, bool enable)
{
	/* audio clk source from internal dcxo */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
			   RG_CLKSQ_IN_SEL_TEST_MASK_SFT,
			   0x0);

	/* Enable/disable CLKSQ 26MHz */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
			   RG_CLKSQ_EN_MASK_SFT,
			   (enable ? 1 : 0) << RG_CLKSQ_EN_SFT);
	return 0;
}

/* use only when not govern by DAPM */
static int mt6359_set_aud_global_bias(struct mt6359_priv *priv, bool enable)
{
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON13,
			   RG_AUDGLB_PWRDN_VA32_MASK_SFT,
			   (enable ? 0 : 1) << RG_AUDGLB_PWRDN_VA32_SFT);
	return 0;
}

/* use only when not govern by DAPM */
static int mt6359_set_topck(struct mt6359_priv *priv, bool enable)
{
	regmap_update_bits(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0,
			   0x0066, enable ? 0x0 : 0x66);
	return 0;
}

static int mt6359_set_decoder_clk(struct mt6359_priv *priv, bool enable)
{
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON13,
			   RG_RSTB_DECODER_VA32_MASK_SFT,
			   (enable ? 1 : 0) << RG_RSTB_DECODER_VA32_SFT);
	return 0;
}

static int mt6359_mtkaif_tx_enable(struct mt6359_priv *priv)
{
	switch (priv->mtkaif_protocol) {
	case MT6359_MTKAIF_PROTOCOL_2_CLK_P2:
		/* MTKAIF TX format setting */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_ADDA_MTKAIF_CFG0,
				   0xffff, 0x0210);
		/* enable aud_pad TX fifos */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_AUD_PAD_TOP,
				   0xff00, 0x3800);
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_AUD_PAD_TOP,
				   0xff00, 0x3900);
		break;
	case MT6359_MTKAIF_PROTOCOL_2:
		/* MTKAIF TX format setting */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_ADDA_MTKAIF_CFG0,
				   0xffff, 0x0210);
		/* enable aud_pad TX fifos */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_AUD_PAD_TOP,
				   0xff00, 0x3100);
		break;
	case MT6359_MTKAIF_PROTOCOL_1:
	default:
		/* MTKAIF TX format setting */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_ADDA_MTKAIF_CFG0,
				   0xffff, 0x0000);
		/* enable aud_pad TX fifos */
		regmap_update_bits(priv->regmap,
				   MT6359_AFE_AUD_PAD_TOP,
				   0xff00, 0x3100);
		break;
	}
	return 0;
}

static int mt6359_mtkaif_tx_disable(struct mt6359_priv *priv)
{
	/* disable aud_pad TX fifos */
	regmap_update_bits(priv->regmap, MT6359_AFE_AUD_PAD_TOP,
			   0xff00, 0x3000);
	return 0;
}

int mt6359_mtkaif_calibration_enable(struct snd_soc_component *cmpnt)
{
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	playback_gpio_set(priv);
	capture_gpio_set(priv);
	mt6359_mtkaif_tx_enable(priv);

	mt6359_set_dcxo(priv, true);
	mt6359_set_aud_global_bias(priv, true);
	mt6359_set_clksq(priv, true);
	mt6359_set_topck(priv, true);

	/* set dat_miso_loopback on */
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_DAT_MISO2_LOOPBACK_MASK_SFT,
			   1 << RG_AUD_PAD_TOP_DAT_MISO2_LOOPBACK_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_DAT_MISO_LOOPBACK_MASK_SFT,
			   1 << RG_AUD_PAD_TOP_DAT_MISO_LOOPBACK_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG1,
			   RG_AUD_PAD_TOP_DAT_MISO3_LOOPBACK_MASK_SFT,
			   1 << RG_AUD_PAD_TOP_DAT_MISO3_LOOPBACK_SFT);
	return 0;
}

int mt6359_mtkaif_calibration_disable(struct snd_soc_component *cmpnt)
{
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	/* set dat_miso_loopback off */
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_DAT_MISO2_LOOPBACK_MASK_SFT,
			   0 << RG_AUD_PAD_TOP_DAT_MISO2_LOOPBACK_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_DAT_MISO_LOOPBACK_MASK_SFT,
			   0 << RG_AUD_PAD_TOP_DAT_MISO_LOOPBACK_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG1,
			   RG_AUD_PAD_TOP_DAT_MISO3_LOOPBACK_MASK_SFT,
			   0 << RG_AUD_PAD_TOP_DAT_MISO3_LOOPBACK_SFT);

	mt6359_set_topck(priv, false);
	mt6359_set_clksq(priv, false);
	mt6359_set_aud_global_bias(priv, false);
	mt6359_set_dcxo(priv, false);

	mt6359_mtkaif_tx_disable(priv);
	playback_gpio_reset(priv);
	capture_gpio_reset(priv);
	return 0;
}

int mt6359_set_mtkaif_calibration_phase(struct snd_soc_component *cmpnt,
					int phase_1, int phase_2, int phase_3)
{
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_PHASE_MODE_MASK_SFT,
			   phase_1 << RG_AUD_PAD_TOP_PHASE_MODE_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG,
			   RG_AUD_PAD_TOP_PHASE_MODE2_MASK_SFT,
			   phase_2 << RG_AUD_PAD_TOP_PHASE_MODE2_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDIO_DIG_CFG1,
			   RG_AUD_PAD_TOP_PHASE_MODE3_MASK_SFT,
			   phase_3 << RG_AUD_PAD_TOP_PHASE_MODE3_SFT);
	return 0;
}

static int get_auxadc_audio(void)
{
#ifdef CONFIG_MTK_AUXADC_INTF
	return pmic_get_auxadc_value(AUXADC_LIST_HPOFS_CAL);
#else
	return 0;
#endif
}

static int get_accdet_auxadc(void)
{
#ifdef CONFIG_MTK_AUXADC_INTF
	return pmic_get_auxadc_value(AUXADC_LIST_ACCDET);
#else
	return 0;
#endif
}

/* dl pga gain */
enum {
	DL_GAIN_8DB = 0,
	DL_GAIN_0DB = 8,
	DL_GAIN_N_1DB = 9,
	DL_GAIN_N_10DB = 18,
	DL_GAIN_N_22DB = 30,
	DL_GAIN_N_40DB = 0x1f,
};
#define DL_GAIN_N_10DB_REG (DL_GAIN_N_10DB << 7 | DL_GAIN_N_10DB)
#define DL_GAIN_N_22DB_REG (DL_GAIN_N_22DB << 7 | DL_GAIN_N_22DB)
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

static const char *const hp_dl_pga_gain[] = {
	"8Db", "7Db", "6Db", "5Db", "4Db",
	"3Db", "2Db", "1Db", "0Db", "-1Db",
	"-2Db", "-3Db",	"-4Db", "-5Db", "-6Db",
	"-7Db", "-8Db", "-9Db", "-10Db", "-11Db",
	"-12Db", "-13Db", "-14Db", "-15Db", "-16Db",
	"-17Db", "-18Db", "-19Db", "-20Db", "-21Db",
	"-22Db", "-40Db"
};

static void zcd_disable(struct mt6359_priv *priv)
{
	regmap_write(priv->regmap, MT6359_ZCD_CON0, 0x0000);
}

static void hp_main_output_ramp(struct mt6359_priv *priv, bool up)
{
	int i = 0, stage = 0;
	int target = 7;

	/* Enable/Reduce HPL/R main output stage step by step */
	for (i = 0; i <= target; i++) {
		stage = up ? i : target - i;
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1,
				   RG_HPLOUTSTGCTRL_VAUDP32_MASK_SFT,
				   stage << RG_HPLOUTSTGCTRL_VAUDP32_SFT);
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1,
				   RG_HPROUTSTGCTRL_VAUDP32_MASK_SFT,
				   stage << RG_HPROUTSTGCTRL_VAUDP32_SFT);
		usleep_range(600, 650);
	}
}

static void hp_aux_feedback_loop_gain_ramp(struct mt6359_priv *priv, bool up)
{
	int i = 0, stage = 0;

	/* Reduce HP aux feedback loop gain step by step */
	for (i = 0; i <= 0xf; i++) {
		stage = up ? i : 0xf - i;
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
				   0xf << 12, stage << 12);
		usleep_range(600, 650);
	}
}

static void hp_in_pair_current(struct mt6359_priv *priv, bool increase)
{
	int i = 0, stage = 0;
	int target = 0x3;

	/* Set input diff pair bias select (Hi-Fi mode) */
	if (priv->hp_hifi_mode) {
		/* Reduce HP aux feedback loop gain step by step */
		for (i = 0; i <= target; i++) {
			stage = increase ? i : target - i;
			regmap_update_bits(priv->regmap,
					   MT6359_AUDDEC_ANA_CON10,
					   0x3 << 3, stage << 3);
			usleep_range(100, 150);
		}
	}
}

static void hp_pull_down(struct mt6359_priv *priv, bool enable)
{
	int i;

	if (enable) {
		for (i = 0x0; i <= 0x7; i++) {
			regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
					   RG_HPPSHORT2VCM_VAUDP32_MASK_SFT,
					   i << RG_HPPSHORT2VCM_VAUDP32_SFT);
			udelay(100);
		}
	} else {
		for (i = 0x7; i >= 0x0; i--) {
			regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
					   RG_HPPSHORT2VCM_VAUDP32_MASK_SFT,
					   i << RG_HPPSHORT2VCM_VAUDP32_SFT);
			udelay(100);
		}
	}
}

static int hp_gain_ctl_select(struct mt6359_priv *priv,
			       unsigned int hp_gain_ctl)
{
	if (hp_gain_ctl < HP_GAIN_CTL_NUM) {
		dev_warn(priv->dev, "%s(), hp_gain_ctl %d invalid\n",
			 __func__, hp_gain_ctl);
		return -EINVAL;
	}

	priv->hp_gain_ctl = hp_gain_ctl;
	regmap_update_bits(priv->regmap, MT6359_AFE_DL_NLE_CFG,
			   NLE_LCH_HPGAIN_SEL_MASK_SFT,
			   hp_gain_ctl << NLE_LCH_HPGAIN_SEL_SFT);
	regmap_update_bits(priv->regmap, MT6359_AFE_DL_NLE_CFG,
			   NLE_RCH_HPGAIN_SEL_MASK_SFT,
			   hp_gain_ctl << NLE_RCH_HPGAIN_SEL_SFT);

	return 0;
}

static bool is_valid_hp_pga_idx(int reg_idx)
{
	return (reg_idx >= DL_GAIN_8DB && reg_idx <= DL_GAIN_N_22DB) ||
	       reg_idx == DL_GAIN_N_40DB;
}

static void headset_volume_ramp(struct mt6359_priv *priv,
				int from, int to)
{
	int offset = 0, count = 1, reg_idx;

	if (!is_valid_hp_pga_idx(from) || !is_valid_hp_pga_idx(to))
		dev_warn(priv->dev, "%s(), volume index is not valid, from %d, to %d\n",
			 __func__, from, to);

	dev_info(priv->dev, "%s(), from %d, to %d\n",
		 __func__, from, to);

	if (to > from)
		offset = to - from;
	else
		offset = from - to;

	while (offset > 0) {
		if (to > from)
			reg_idx = from + count;
		else
			reg_idx = from - count;

		if (is_valid_hp_pga_idx(reg_idx)) {
			regmap_update_bits(priv->regmap,
					   MT6359_ZCD_CON2,
					   DL_GAIN_REG_MASK,
					   (reg_idx << 7) | reg_idx);
			usleep_range(600, 650);
		}
		offset--;
		count++;
	}
}

static int dl_pga_get(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int id = kcontrol->id.device;
	int array_size, reg_minus_40db;


	if (id == AUDIO_ANALOG_VOLUME_HPOUTL ||
	    id == AUDIO_ANALOG_VOLUME_HPOUTR)
		array_size = ARRAY_SIZE(hp_dl_pga_gain);
	else
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
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int index = ucontrol->value.integer.value[0];
	unsigned int id = kcontrol->id.device;
	int array_size, reg_minus_40db;

	dev_info(priv->dev, "%s(), id %d, index %d\n", __func__, id, index);

	if (id == AUDIO_ANALOG_VOLUME_HPOUTL ||
	    id == AUDIO_ANALOG_VOLUME_HPOUTR)
		array_size = ARRAY_SIZE(hp_dl_pga_gain);
	else
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
		regmap_update_bits(priv->regmap, MT6359_ZCD_CON2,
				   RG_AUDHPLGAIN_MASK_SFT,
				   index << RG_AUDHPLGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_HPOUTR:
		regmap_update_bits(priv->regmap, MT6359_ZCD_CON2,
				   RG_AUDHPRGAIN_MASK_SFT,
				   index << RG_AUDHPRGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_HSOUTL:
		regmap_update_bits(priv->regmap, MT6359_ZCD_CON3,
				   RG_AUDHSGAIN_MASK_SFT,
				   index << RG_AUDHSGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_LINEOUTL:
		regmap_update_bits(priv->regmap, MT6359_ZCD_CON1,
				   RG_AUDLOLGAIN_MASK_SFT,
				   index << RG_AUDLOLGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_LINEOUTR:
		regmap_update_bits(priv->regmap, MT6359_ZCD_CON1,
				   RG_AUDLORGAIN_MASK_SFT,
				   index << RG_AUDLORGAIN_SFT);
		break;
	default:
		return 0;
	}

	priv->ana_gain[id] = index;
	return 0;
}

static const struct soc_enum dl_pga_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dl_pga_gain), dl_pga_gain),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hp_dl_pga_gain), hp_dl_pga_gain),
};

#define MT_SOC_ENUM_EXT_ID(xname, xenum, xhandler_get, xhandler_put, id) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, .device = id,\
	.info = snd_soc_info_enum_double, \
	.get = xhandler_get, .put = xhandler_put, \
	.private_value = (unsigned long)&xenum }

static const struct snd_kcontrol_new mt6359_snd_controls[] = {
	MT_SOC_ENUM_EXT_ID("Headset_PGAL_GAIN", dl_pga_enum[1],
			   dl_pga_get, dl_pga_set,
			   AUDIO_ANALOG_VOLUME_HPOUTL),
	MT_SOC_ENUM_EXT_ID("Headset_PGAR_GAIN", dl_pga_enum[1],
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
	"0Db", "6Db", "12Db", "18Db", "24Db"
};

static const struct soc_enum ul_pga_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ul_pga_gain), ul_pga_gain),
};

static int ul_pga_get(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	ucontrol->value.integer.value[0] = priv->ana_gain[kcontrol->id.device];
	return 0;
}

static int ul_pga_set(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int index = ucontrol->value.integer.value[0];
	unsigned int id = kcontrol->id.device;

	dev_info(priv->dev, "%s(), id %d, index %d\n", __func__, id, index);
	if (index > ARRAY_SIZE(ul_pga_gain)) {
		dev_warn(priv->dev, "return -EINVAL\n");
		return -EINVAL;
	}

	switch (id) {
	case AUDIO_ANALOG_VOLUME_MICAMP1:
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
				   RG_AUDPREAMPLGAIN_MASK_SFT,
				   index << RG_AUDPREAMPLGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_MICAMP2:
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
				   RG_AUDPREAMPRGAIN_MASK_SFT,
				   index << RG_AUDPREAMPRGAIN_SFT);
		break;
	case AUDIO_ANALOG_VOLUME_MICAMP3:
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   RG_AUDPREAMP3GAIN_MASK_SFT,
				   index << RG_AUDPREAMP3GAIN_SFT);
		break;
	default:
		return 0;
	}

	priv->ana_gain[id] = index;
	return 0;
}

/* Mic Type MUX */
enum {
	MIC_TYPE_MUX_IDLE = 0,
	MIC_TYPE_MUX_ACC,
	MIC_TYPE_MUX_DMIC,
	MIC_TYPE_MUX_DCC,
	MIC_TYPE_MUX_DCC_ECM_DIFF,
	MIC_TYPE_MUX_DCC_ECM_SINGLE,
	MIC_TYPE_MUX_VOW_ACC,
	MIC_TYPE_MUX_VOW_DMIC,
	MIC_TYPE_MUX_VOW_DMIC_LP,
	MIC_TYPE_MUX_VOW_DCC,
	MIC_TYPE_MUX_VOW_DCC_ECM_DIFF,
	MIC_TYPE_MUX_VOW_DCC_ECM_SINGLE,
};

#define IS_VOW_DCC_BASE(x) (x == MIC_TYPE_MUX_VOW_DCC || \
			    x == MIC_TYPE_MUX_VOW_DCC_ECM_DIFF || \
			    x == MIC_TYPE_MUX_VOW_DCC_ECM_SINGLE)

#define IS_DCC_BASE(x) (x == MIC_TYPE_MUX_DCC || \
			x == MIC_TYPE_MUX_DCC_ECM_DIFF || \
			x == MIC_TYPE_MUX_DCC_ECM_SINGLE || \
			IS_VOW_DCC_BASE(x))

#define IS_VOW_AMIC_BASE(x) (x == MIC_TYPE_MUX_VOW_ACC || IS_VOW_DCC_BASE(x))

static const char *const mic_type_mux_map[] = {
	"Idle",
	"ACC",
	"DMIC",
	"DCC",
	"DCC_ECM_DIFF",
	"DCC_ECM_SINGLE",
	"VOW_ACC",
	"VOW_DMIC",
	"VOW_DMIC_LP",
	"VOW_DCC",
	"VOW_DCC_ECM_DIFF",
	"VOW_DCC_ECM_SINGLE"
};

static const struct soc_enum mic_type_mux_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_type_mux_map), mic_type_mux_map),
};

static int mic_type_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	ucontrol->value.integer.value[0] =
		priv->mux_select[kcontrol->id.device];
	return 0;
}

static int mic_type_set(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int index = ucontrol->value.integer.value[0];
	unsigned int id = kcontrol->id.device;

	dev_info(priv->dev, "%s(), id %d, index %d\n", __func__, id, index);

	if (ucontrol->value.enumerated.item[0] >= e->items)
		return -EINVAL;

	priv->mux_select[id] = index;
	return 0;
}

static const struct snd_kcontrol_new mt6359_snd_ul_controls[] = {
	MT_SOC_ENUM_EXT_ID("Audio_PGA1_Setting", ul_pga_enum[0],
			   ul_pga_get, ul_pga_set,
			   AUDIO_ANALOG_VOLUME_MICAMP1),
	MT_SOC_ENUM_EXT_ID("Audio_PGA2_Setting", ul_pga_enum[0],
			   ul_pga_get, ul_pga_set,
			   AUDIO_ANALOG_VOLUME_MICAMP2),
	MT_SOC_ENUM_EXT_ID("Audio_PGA3_Setting", ul_pga_enum[0],
			   ul_pga_get, ul_pga_set,
			   AUDIO_ANALOG_VOLUME_MICAMP3),
	MT_SOC_ENUM_EXT_ID("Mic_Type_Mux_0", mic_type_mux_enum[0],
			   mic_type_get, mic_type_set,
			   MUX_MIC_TYPE_0),
	MT_SOC_ENUM_EXT_ID("Mic_Type_Mux_1", mic_type_mux_enum[0],
			   mic_type_get, mic_type_set,
			   MUX_MIC_TYPE_1),
	MT_SOC_ENUM_EXT_ID("Mic_Type_Mux_2", mic_type_mux_enum[0],
			   mic_type_get, mic_type_set,
			   MUX_MIC_TYPE_2),
};

/* MUX */

/* LOL MUX */
enum {
	LO_MUX_OPEN = 0,
	LO_MUX_L_DAC,
	LO_MUX_3RD_DAC,
	LO_MUX_TEST_MODE,
	LO_MUX_MASK = 0x3,
};

static const char * const lo_in_mux_map[] = {
	"Open", "Playback_L_DAC", "Playback", "Test Mode"
};

static int lo_in_mux_map_value[] = {
	0x0, 0x1, 0x2, 0x3,
};

static SOC_VALUE_ENUM_SINGLE_DECL(lo_in_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  LO_MUX_MASK,
				  lo_in_mux_map,
				  lo_in_mux_map_value);

static const struct snd_kcontrol_new lo_in_mux_control =
	SOC_DAPM_ENUM("LO Select", lo_in_mux_map_enum);

/*HP MUX */
enum {
	HP_MUX_OPEN = 0,
	HP_MUX_HPSPK,
	HP_MUX_HP,
	HP_MUX_TEST_MODE,
	HP_MUX_HP_IMPEDANCE,
	HP_MUX_MASK = 0x7,
};

static const char * const hp_in_mux_map[] = {
	"Open",
	"LoudSPK Playback",
	"Audio Playback",
	"Test Mode",
	"HP Impedance",
	"undefined1",
	"undefined2",
	"undefined3",
};

static int hp_in_mux_map_value[] = {
	HP_MUX_OPEN,
	HP_MUX_HPSPK,
	HP_MUX_HP,
	HP_MUX_TEST_MODE,
	HP_MUX_HP_IMPEDANCE,
	HP_MUX_OPEN,
	HP_MUX_OPEN,
	HP_MUX_OPEN,
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
enum {
	RCV_MUX_OPEN = 0,
	RCV_MUX_MUTE,
	RCV_MUX_VOICE_PLAYBACK,
	RCV_MUX_TEST_MODE,
	RCV_MUX_MASK = 0x3,
};

static const char * const rcv_in_mux_map[] = {
	"Open", "Mute", "Voice Playback", "Test Mode"
};

static int rcv_in_mux_map_value[] = {
	RCV_MUX_OPEN,
	RCV_MUX_MUTE,
	RCV_MUX_VOICE_PLAYBACK,
	RCV_MUX_TEST_MODE,
};

static SOC_VALUE_ENUM_SINGLE_DECL(rcv_in_mux_map_enum,
				  SND_SOC_NOPM,
				  0,
				  RCV_MUX_MASK,
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
				  MT6359_AFE_TOP_CON0,
				  DL_SINE_ON_SFT,
				  DL_SINE_ON_MASK,
				  dac_in_mux_map,
				  dac_in_mux_map_value);

static const struct snd_kcontrol_new dac_in_mux_control =
	SOC_DAPM_ENUM("DAC Select", dac_in_mux_map_enum);

/* AIF Out MUX */
static SOC_VALUE_ENUM_SINGLE_DECL(aif_out_mux_map_enum,
				  MT6359_AFE_TOP_CON0,
				  UL_SINE_ON_SFT,
				  UL_SINE_ON_MASK,
				  dac_in_mux_map,
				  dac_in_mux_map_value);

static const struct snd_kcontrol_new aif_out_mux_control =
	SOC_DAPM_ENUM("AIF Out Select", aif_out_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(aif2_out_mux_map_enum,
				  MT6359_AFE_TOP_CON0,
				  ADDA6_UL_SINE_ON_SFT,
				  ADDA6_UL_SINE_ON_MASK,
				  dac_in_mux_map,
				  dac_in_mux_map_value);

static const struct snd_kcontrol_new aif2_out_mux_control =
	SOC_DAPM_ENUM("AIF Out Select", aif2_out_mux_map_enum);

/* UL SRC MUX */
enum {
	UL_SRC_MUX_AMIC = 0,
	UL_SRC_MUX_DMIC,
};

static const char * const ul_src_mux_map[] = {
	"AMIC",
	"DMIC",
};

static int ul_src_mux_map_value[] = {
	UL_SRC_MUX_AMIC,
	UL_SRC_MUX_DMIC,
};

static SOC_VALUE_ENUM_SINGLE_DECL(ul_src_mux_map_enum,
				  MT6359_AFE_UL_SRC_CON0_L,
				  UL_SDM_3_LEVEL_CTL_SFT,
				  UL_SDM_3_LEVEL_CTL_MASK,
				  ul_src_mux_map,
				  ul_src_mux_map_value);

static const struct snd_kcontrol_new ul_src_mux_control =
	SOC_DAPM_ENUM("UL_SRC_MUX Select", ul_src_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(ul2_src_mux_map_enum,
				  MT6359_AFE_ADDA6_UL_SRC_CON0_L,
				  ADDA6_UL_SDM_3_LEVEL_CTL_SFT,
				  ADDA6_UL_SDM_3_LEVEL_CTL_MASK,
				  ul_src_mux_map,
				  ul_src_mux_map_value);

static const struct snd_kcontrol_new ul2_src_mux_control =
	SOC_DAPM_ENUM("UL_SRC_MUX Select", ul2_src_mux_map_enum);

/* MISO MUX */
enum {
	MISO_MUX_UL1_CH1 = 0,
	MISO_MUX_UL1_CH2,
	MISO_MUX_UL2_CH1,
	MISO_MUX_UL2_CH2,
};

static const char * const miso_mux_map[] = {
	"UL1_CH1",
	"UL1_CH2",
	"UL2_CH1",
	"UL2_CH2",
};

static int miso_mux_map_value[] = {
	MISO_MUX_UL1_CH1,
	MISO_MUX_UL1_CH2,
	MISO_MUX_UL2_CH1,
	MISO_MUX_UL2_CH2,
};

static SOC_VALUE_ENUM_SINGLE_DECL(miso0_mux_map_enum,
				  MT6359_AFE_MTKAIF_MUX_CFG,
				  RG_ADDA_CH1_SEL_SFT,
				  RG_ADDA_CH1_SEL_MASK,
				  miso_mux_map,
				  miso_mux_map_value);

static const struct snd_kcontrol_new miso0_mux_control =
	SOC_DAPM_ENUM("MISO_MUX Select", miso0_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(miso1_mux_map_enum,
				  MT6359_AFE_MTKAIF_MUX_CFG,
				  RG_ADDA_CH2_SEL_SFT,
				  RG_ADDA_CH2_SEL_MASK,
				  miso_mux_map,
				  miso_mux_map_value);

static const struct snd_kcontrol_new miso1_mux_control =
	SOC_DAPM_ENUM("MISO_MUX Select", miso1_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(miso2_mux_map_enum,
				  MT6359_AFE_MTKAIF_MUX_CFG,
				  RG_ADDA6_CH1_SEL_SFT,
				  RG_ADDA6_CH1_SEL_MASK,
				  miso_mux_map,
				  miso_mux_map_value);

static const struct snd_kcontrol_new miso2_mux_control =
	SOC_DAPM_ENUM("MISO_MUX Select", miso2_mux_map_enum);

/* AMIC MUX */
enum {
	AMIC_MUX_ADC_L = 0,
	AMIC_MUX_ADC_R,
	AMIC_MUX_ADC_T,
};

static const char * const amic_mux_map[] = {
	"ADC_L",
	"ADC_R",
	"ADC_T",
};

static int amic_mux_map_value[] = {
	AMIC_MUX_ADC_L,
	AMIC_MUX_ADC_R,
	AMIC_MUX_ADC_T,
};

static SOC_VALUE_ENUM_SINGLE_DECL(amic0_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_AMIC_ADC1_SOURCE_SEL_SFT,
				  RG_AMIC_ADC1_SOURCE_SEL_MASK,
				  amic_mux_map,
				  amic_mux_map_value);

static const struct snd_kcontrol_new amic0_mux_control =
	SOC_DAPM_ENUM("AMIC_MUX Select", amic0_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(amic1_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_AMIC_ADC2_SOURCE_SEL_SFT,
				  RG_AMIC_ADC2_SOURCE_SEL_MASK,
				  amic_mux_map,
				  amic_mux_map_value);

static const struct snd_kcontrol_new amic1_mux_control =
	SOC_DAPM_ENUM("AMIC_MUX Select", amic1_mux_map_enum);

static SOC_VALUE_ENUM_SINGLE_DECL(amic2_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_AMIC_ADC3_SOURCE_SEL_SFT,
				  RG_AMIC_ADC3_SOURCE_SEL_MASK,
				  amic_mux_map,
				  amic_mux_map_value);

static const struct snd_kcontrol_new amic2_mux_control =
	SOC_DAPM_ENUM("AMIC_MUX Select", amic2_mux_map_enum);

/* DMIC MUX */
enum {
	DMIC_MUX_DMIC_DATA0 = 0,
	DMIC_MUX_DMIC_DATA1_L,
	DMIC_MUX_DMIC_DATA1_L_1,
	DMIC_MUX_DMIC_DATA1_R,
};

static const char * const dmic_mux_map[] = {
	"DMIC_DATA0",
	"DMIC_DATA1_L",
	"DMIC_DATA1_L_1",
	"DMIC_DATA1_R",
};

static int dmic_mux_map_value[] = {
	DMIC_MUX_DMIC_DATA0,
	DMIC_MUX_DMIC_DATA1_L,
	DMIC_MUX_DMIC_DATA1_L_1,
	DMIC_MUX_DMIC_DATA1_R,
};

static SOC_VALUE_ENUM_SINGLE_DECL(dmic0_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_DMIC_ADC1_SOURCE_SEL_SFT,
				  RG_DMIC_ADC1_SOURCE_SEL_MASK,
				  dmic_mux_map,
				  dmic_mux_map_value);

static const struct snd_kcontrol_new dmic0_mux_control =
	SOC_DAPM_ENUM("DMIC_MUX Select", dmic0_mux_map_enum);

/* ul1 ch2 use RG_DMIC_ADC3_SOURCE_SEL */
static SOC_VALUE_ENUM_SINGLE_DECL(dmic1_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_DMIC_ADC3_SOURCE_SEL_SFT,
				  RG_DMIC_ADC3_SOURCE_SEL_MASK,
				  dmic_mux_map,
				  dmic_mux_map_value);

static const struct snd_kcontrol_new dmic1_mux_control =
	SOC_DAPM_ENUM("DMIC_MUX Select", dmic1_mux_map_enum);

/* ul2 ch1 use RG_DMIC_ADC2_SOURCE_SEL */
static SOC_VALUE_ENUM_SINGLE_DECL(dmic2_mux_map_enum,
				  MT6359_AFE_MIC_ARRAY_CFG,
				  RG_DMIC_ADC2_SOURCE_SEL_SFT,
				  RG_DMIC_ADC2_SOURCE_SEL_MASK,
				  dmic_mux_map,
				  dmic_mux_map_value);

static const struct snd_kcontrol_new dmic2_mux_control =
	SOC_DAPM_ENUM("DMIC_MUX Select", dmic2_mux_map_enum);

/* ADC L MUX */
enum {
	ADC_MUX_IDLE = 0,
	ADC_MUX_AIN0,
	ADC_MUX_PREAMPLIFIER,
	ADC_MUX_IDLE1,
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
				  MT6359_AUDENC_ANA_CON0,
				  RG_AUDADCLINPUTSEL_SFT,
				  RG_AUDADCLINPUTSEL_MASK,
				  adc_left_mux_map,
				  adc_mux_map_value);

static const struct snd_kcontrol_new adc_left_mux_control =
	SOC_DAPM_ENUM("ADC L Select", adc_left_mux_map_enum);

/* ADC R MUX */
static const char * const adc_right_mux_map[] = {
	"Idle", "AIN0", "Right Preamplifier", "Idle_1"
};

static SOC_VALUE_ENUM_SINGLE_DECL(adc_right_mux_map_enum,
				  MT6359_AUDENC_ANA_CON1,
				  RG_AUDADCRINPUTSEL_SFT,
				  RG_AUDADCRINPUTSEL_MASK,
				  adc_right_mux_map,
				  adc_mux_map_value);

static const struct snd_kcontrol_new adc_right_mux_control =
	SOC_DAPM_ENUM("ADC R Select", adc_right_mux_map_enum);

/* ADC 3 MUX */
static const char * const adc_3_mux_map[] = {
	"Idle", "AIN0", "Preamplifier", "Idle_1"
};

static SOC_VALUE_ENUM_SINGLE_DECL(adc_3_mux_map_enum,
				  MT6359_AUDENC_ANA_CON2,
				  RG_AUDADC3INPUTSEL_SFT,
				  RG_AUDADC3INPUTSEL_MASK,
				  adc_3_mux_map,
				  adc_mux_map_value);

static const struct snd_kcontrol_new adc_3_mux_control =
	SOC_DAPM_ENUM("ADC 3 Select", adc_3_mux_map_enum);

/* PGA L MUX */
enum {
	PGA_L_MUX_NONE = 0,
	PGA_L_MUX_AIN0,
	PGA_L_MUX_AIN1,
};

static const char * const pga_l_mux_map[] = {
	"None", "AIN0", "AIN1"
};

static int pga_l_mux_map_value[] = {
	PGA_L_MUX_NONE,
	PGA_L_MUX_AIN0,
	PGA_L_MUX_AIN1
};

static SOC_VALUE_ENUM_SINGLE_DECL(pga_left_mux_map_enum,
				  MT6359_AUDENC_ANA_CON0,
				  RG_AUDPREAMPLINPUTSEL_SFT,
				  RG_AUDPREAMPLINPUTSEL_MASK,
				  pga_l_mux_map,
				  pga_l_mux_map_value);

static const struct snd_kcontrol_new pga_left_mux_control =
	SOC_DAPM_ENUM("PGA L Select", pga_left_mux_map_enum);

/* PGA R MUX */
enum {
	PGA_R_MUX_NONE = 0,
	PGA_R_MUX_AIN2,
	PGA_R_MUX_AIN3,
	PGA_R_MUX_AIN0,
};

static const char * const pga_r_mux_map[] = {
	"None", "AIN2", "AIN3", "AIN0"
};

static int pga_r_mux_map_value[] = {
	PGA_R_MUX_NONE,
	PGA_R_MUX_AIN2,
	PGA_R_MUX_AIN3,
	PGA_R_MUX_AIN0
};

static SOC_VALUE_ENUM_SINGLE_DECL(pga_right_mux_map_enum,
				  MT6359_AUDENC_ANA_CON1,
				  RG_AUDPREAMPRINPUTSEL_SFT,
				  RG_AUDPREAMPRINPUTSEL_MASK,
				  pga_r_mux_map,
				  pga_r_mux_map_value);

static const struct snd_kcontrol_new pga_right_mux_control =
	SOC_DAPM_ENUM("PGA R Select", pga_right_mux_map_enum);

/* PGA 3 MUX */
enum {
	PGA_3_MUX_NONE = 0,
	PGA_3_MUX_AIN3,
	PGA_3_MUX_AIN2,
};

static const char * const pga_3_mux_map[] = {
	"None", "AIN3", "AIN2"
};

static int pga_3_mux_map_value[] = {
	PGA_3_MUX_NONE,
	PGA_3_MUX_AIN3,
	PGA_3_MUX_AIN2
};

static SOC_VALUE_ENUM_SINGLE_DECL(pga_3_mux_map_enum,
				  MT6359_AUDENC_ANA_CON2,
				  RG_AUDPREAMP3INPUTSEL_SFT,
				  RG_AUDPREAMP3INPUTSEL_MASK,
				  pga_3_mux_map,
				  pga_3_mux_map_value);

static const struct snd_kcontrol_new pga_3_mux_control =
	SOC_DAPM_ENUM("PGA 3 Select", pga_3_mux_map_enum);

static int mt_clksq_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_dbg(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* audio clk source from internal dcxo */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
				   RG_CLKSQ_IN_SEL_TEST_MASK_SFT,
				   0x0);
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
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_dbg(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* sdm audio fifo clock power on */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0006);
		/* scrambler clock on enable */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xCBA1);
		/* sdm power on */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0003);
		/* sdm fifo enable */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x000B);

		regmap_update_bits(priv->regmap, MT6359_AFE_SGEN_CFG0,
				   0xff3f,
				   0x0000);
		regmap_update_bits(priv->regmap, MT6359_AFE_SGEN_CFG1,
				   0xffff,
				   0x0001);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DL scrambler disabling sequence */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0000);
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xcba0);
		break;
	default:
		break;
	}

	return 0;
}

static int mtk_hp_enable(struct mt6359_priv *priv)
{
	if (priv->hp_hifi_mode) {
		/* Set HP DR bias current optimization, 010: 6uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON11,
				   DRBIAS_HP_MASK_SFT,
				   DRBIAS_6UA << DRBIAS_HP_SFT);
		/* Set HP & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_ZCD_MASK_SFT,
				   IBIAS_ZCD_4UA << IBIAS_ZCD_SFT);
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_HP_MASK_SFT,
				   IBIAS_5UA << IBIAS_HP_SFT);
	} else {
		/* Set HP DR bias current optimization, 001: 5uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON11,
				   DRBIAS_HP_MASK_SFT,
				   DRBIAS_5UA << DRBIAS_HP_SFT);
		/* Set HP & ZCD bias current optimization */
		/* 00: ZCD: 3uA, HP/HS/LO: 4uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_ZCD_MASK_SFT,
				   IBIAS_ZCD_3UA << IBIAS_ZCD_SFT);
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_HP_MASK_SFT,
				   IBIAS_4UA << IBIAS_HP_SFT);
	}

	/* HP damp circuit enable */
	/*Enable HPRN/HPLN output 4K to VCM */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x0087);

	/* HP Feedback Cap select 2'b00: 15pF */
	/* for >= 96KHz sampling rate: 2'b01: 10.5pF */
	if (priv->dl_rate[MT6359_AIF_1] >= 96000)
		regmap_update_bits(priv->regmap,
				   MT6359_AUDDEC_ANA_CON4,
				   RG_AUDHPHFCOMPBUFGAINSEL_VAUDP32_MASK_SFT,
				   0x1 << RG_AUDHPHFCOMPBUFGAINSEL_VAUDP32_SFT);
	else
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON4, 0x0000);


	/* Set HPP/N STB enhance circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0x7133);

	/* Enable HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x000c);
	/* Enable HP aux feedback loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x003c);
	/* Enable HP aux CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0c00);
	/* Enable HP driver bias circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30c0);
	/* Enable HP driver core circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30f0);
	/* Short HP main output to HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x00fc);

	/* Increase HP input pair current to HPM step by step */
	hp_in_pair_current(priv, true);

	/* Enable HP main CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0e00);
	/* Disable HP aux CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0200);

	/* Enable HP main output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x00ff);
	/* Enable HPR/L main output stage step by step */
	hp_main_output_ramp(priv, true);

	/* Reduce HP aux feedback loop gain */
	hp_aux_feedback_loop_gain_ramp(priv, true);
	/* Disable HP aux feedback loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x77cf);

	/* apply volume setting */
	headset_volume_ramp(priv,
			    DL_GAIN_N_22DB,
			    priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL]);

	/* Disable HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x77c3);
	/* Unshort HP main output to HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x7703);
	usleep_range(100, 120);

	/* Enable AUD_CLK */
	mt6359_set_decoder_clk(priv, true);

	/* Enable Audio DAC  */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30ff);
	if (priv->hp_hifi_mode) {
		/* Enable low-noise mode of DAC */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0xf201);
	} else {
		/* Disable low-noise mode of DAC */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0xf200);
	}
	usleep_range(100, 120);

	/* Switch HPL MUX to audio DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x32ff);
	/* Switch HPR MUX to audio DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x3aff);

	/* Apply digital DC compensation value to DAC */
	apply_dc_compensation(priv, true);

	/* Disable Pull-down HPL/R to AVSS28_AUD */
	hp_pull_down(priv, false);

	return 0;
}

static int mtk_hp_disable(struct mt6359_priv *priv)
{
	/* Pull-down HPL/R to AVSS28_AUD */
	hp_pull_down(priv, true);

	apply_dc_compensation(priv, false);

	/* HPR/HPL mux to open */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x0f00, 0x0000);

	/* Disable low-noise mode of DAC */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0x0001, 0x0000);

	/* Disable Audio DAC */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x000f, 0x0000);

	/* Disable AUD_CLK */
	mt6359_set_decoder_clk(priv, false);

	/* Short HP main output to HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x77c3);
	/* Enable HP aux output stage */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x77cf);

	/* decrease HPL/R gain to normal gain step by step */
	headset_volume_ramp(priv,
			    priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL],
			    DL_GAIN_N_22DB);

	/* Enable HP aux feedback loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x77ff);

	/* Reduce HP aux feedback loop gain */
	hp_aux_feedback_loop_gain_ramp(priv, false);

	/* decrease HPR/L main output stage step by step */
	hp_main_output_ramp(priv, false);

	/* Disable HP main output stage */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x3, 0x0);

	/* Enable HP aux CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0e01);

	/* Disable HP main CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0c01);

	/* Decrease HP input pair current to 2'b00 step by step */
	hp_in_pair_current(priv, false);

	/* Unshort HP main output to HP aux output stage */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1,
			   0x3 << 6, 0x0);

	/* Disable HP driver core circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 4, 0x0);

	/* Disable HP driver bias circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 6, 0x0);

	/* Disable HP aux CMFB loop */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x201);

	/* Disable HP aux feedback loop */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1,
			   0x3 << 4, 0x0);

	/* Disable HP aux output stage */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1,
			   0x3 << 2, 0x0);
	return 0;
}

static int mtk_hp_impedance_enable(struct mt6359_priv *priv)
{
	/* release HP CMFB gate rstb */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON4,
			   0x1 << 6, 0x1 << 6);

	/* Disable headphone short-circuit protection */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x3000);

	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
			   RG_AUDREFN_DERES_EN_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDREFN_DERES_EN_VAUDP32_SFT);

	/* Enable cap-less LDOs (1.5V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0005);
	/* Enable NV regulator (-1.2V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0015);
	usleep_range(100, 120);

	/* Disable AUD_ZCD */
	zcd_disable(priv);

	/* Enable IBIST */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON12, 0x0055);
	/* Disable HPR/L STB enhance circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0x4000);

	/* Disable Pull-down HPL/R to AVSS28_AUD */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON4, 0x0000);

	/* Enable AUD_CLK */
	mt6359_set_decoder_clk(priv, true);

	/* Enable Audio L channel DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x3009);

	/* Enable Trim buffer VA28 reference */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0x00ff, 0x0002);

	/* Enable HPDET circuit, */
	/* select DACLP as HPDET input and HPR as HPDET output */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON8, 0x1900);

	/* Enable TRIMBUF circuit, select HPR as TRIMBUF input */
	/* Set TRIMBUF gain as 18dB */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON8, 0x1972);

	return 0;
}

static int mtk_hp_impedance_disable(struct mt6359_priv *priv)
{
	/* disable HPDET circuit */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON8,
			   0xff00, 0x0000);

	/* Disable Trim buffer VA28 reference */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0x00ff, 0x0000);

	/* Disable Audio DAC */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x000f, 0x0000);

	/* Disable AUD_CLK */
	mt6359_set_decoder_clk(priv, false);

	/* Disable HP main output stage */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x3, 0x0);

	/* Disable HP driver core circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 4, 0x0);

	/* Disable HP driver bias circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 6, 0x0);

	/* Disable HP aux CMFB loop */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0xff << 8, 0x0);

	/* Disable IBIST */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
			   RG_AUDIBIASPWRDN_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDIBIASPWRDN_VAUDP32_SFT);
	/* Disable NV regulator (-1.2V) */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON14,
			   RG_NVREG_EN_VAUDP32_MASK_SFT, 0x0);
	/* Disable cap-less LDOs (1.5V) */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x5, 0x0);

	/* Disable NCP */
	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc800);

	/* Set HPL/HPR gain to mute */
	regmap_write(priv->regmap, MT6359_ZCD_CON2, DL_GAIN_N_10DB_REG);

	/* Set HPP/N STB enhance circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0xff, 0x33);

	/* Increase ESD resistance of AU_REFN */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
			   RG_AUDREFN_DERES_EN_VAUDP32_MASK_SFT, 0x0);

	return 0;
}

static int mt_hp_event(struct snd_soc_dapm_widget *w,
		       struct snd_kcontrol *kcontrol,
		       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);
	int device = DEVICE_HP;

	dev_info(priv->dev, "%s(), event 0x%x, dev_counter[DEV_HP] %d, mux %u\n",
		 __func__,
		 event,
		 priv->dev_counter[device],
		 mux);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		priv->dev_counter[device]++;
		if (priv->dev_counter[device] > 1)
			break;	/* already enabled, do nothing */
		else if (priv->dev_counter[device] <= 0)
			dev_warn(priv->dev, "%s(), dev_counter[DEV_HP] %d <= 0\n",
				 __func__,
				 priv->dev_counter[device]);

		priv->mux_select[MUX_HP_L] = mux;

		if (mux == HP_MUX_HP)
			mtk_hp_enable(priv);
		else if (mux == HP_MUX_HP_IMPEDANCE)
			mtk_hp_impedance_enable(priv);
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

		if (priv->mux_select[MUX_HP_L] == HP_MUX_HP)
			mtk_hp_disable(priv);
		else if (priv->mux_select[MUX_HP_L] == HP_MUX_HP_IMPEDANCE)
			mtk_hp_impedance_disable(priv);

		priv->mux_select[MUX_HP_L] = mux;
		break;
	default:
		break;
	}

	return 0;
}

static int mt_rcv_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol,
			int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x, mux %u\n",
		 __func__,
		 event,
		 dapm_kcontrol_get_value(w->kcontrols[0]));

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Disable handset short-circuit protection */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0010);

		/* Set RCV DR bias current optimization, 010: 6uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON11,
				   DRBIAS_HS_MASK_SFT,
				   DRBIAS_6UA << DRBIAS_HS_SFT);
		/* Set RCV & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_ZCD_MASK_SFT,
				   IBIAS_ZCD_4UA << IBIAS_ZCD_SFT);
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_HS_MASK_SFT,
				   IBIAS_5UA << IBIAS_HS_SFT);

		/* Set HS STB enhance circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0090);

		/* Set HS output stage (3'b111 = 8x) */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x7000);

		/* Enable HS driver bias circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0092);
		/* Enable HS driver core circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0093);

		/* Set HS gain to normal gain step by step */
		regmap_write(priv->regmap, MT6359_ZCD_CON3,
			     priv->ana_gain[AUDIO_ANALOG_VOLUME_HSOUTL]);

		/* Enable AUD_CLK */
		mt6359_set_decoder_clk(priv, true);

		/* Enable Audio DAC  */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x0009);
		/* Enable low-noise mode of DAC */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0001);
		/* Switch HS MUX to audio DAC */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x009b);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* HS mux to open */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON6,
				   RG_AUDHSMUXINPUTSEL_VAUDP32_MASK_SFT,
				   RCV_MUX_OPEN);

		/* Disable Audio DAC */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
				   0x000f, 0x0000);

		/* Disable AUD_CLK */
		mt6359_set_decoder_clk(priv, false);

		/* decrease HS gain to minimum gain step by step */
		regmap_write(priv->regmap, MT6359_ZCD_CON3, DL_GAIN_N_40DB);

		/* Disable HS driver core circuits */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON6,
				   RG_AUDHSPWRUP_VAUDP32_MASK_SFT, 0x0);

		/* Disable HS driver bias circuits */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON6,
				   RG_AUDHSPWRUP_IBIAS_VAUDP32_MASK_SFT, 0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_lo_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol,
			int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x, mux %u\n",
		 __func__,
		 event,
		 dapm_kcontrol_get_value(w->kcontrols[0]));

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Disable handset short-circuit protection */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0010);

		/* Set LO DR bias current optimization, 010: 6uA */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON11,
				   DRBIAS_LO_MASK_SFT,
				   DRBIAS_6UA << DRBIAS_LO_SFT);
		/* Set LO & ZCD bias current optimization */
		/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
		if (priv->dev_counter[DEVICE_HP] == 0)
			regmap_update_bits(priv->regmap,
					   MT6359_AUDDEC_ANA_CON12,
					   IBIAS_ZCD_MASK_SFT,
					   IBIAS_ZCD_4UA << IBIAS_ZCD_SFT);

		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
				   IBIAS_LO_MASK_SFT,
				   IBIAS_5UA << IBIAS_LO_SFT);

		/* Set LO STB enhance circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0110);

		/* Enable LO driver bias circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0112);
		/* Enable LO driver core circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0113);

		/* Set LO gain to normal gain step by step */
		regmap_write(priv->regmap, MT6359_ZCD_CON1,
			     priv->ana_gain[AUDIO_ANALOG_VOLUME_LINEOUTL]);

		/* Enable AUD_CLK */
		mt6359_set_decoder_clk(priv, true);

		/* Enable Audio DAC (3rd DAC) */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x3113);
		/* Enable low-noise mode of DAC */
		if (priv->dev_counter[DEVICE_HP] == 0)
			regmap_write(priv->regmap,
				     MT6359_AUDDEC_ANA_CON9, 0x0001);
		/* Switch LOL MUX to audio 3rd DAC */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x311b);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* Switch LOL MUX to open */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON7,
				   RG_AUDLOLMUXINPUTSEL_VAUDP32_MASK_SFT,
				   LO_MUX_OPEN);

		/* Disable Audio DAC */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
				   0x000f, 0x0000);

		/* Disable AUD_CLK */
		mt6359_set_decoder_clk(priv, false);

		/* decrease LO gain to minimum gain step by step */
		regmap_write(priv->regmap, MT6359_ZCD_CON1, DL_GAIN_N_40DB);

		/* Disable LO driver core circuits */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON7,
				   RG_AUDLOLPWRUP_VAUDP32_MASK_SFT, 0x0);

		/* Disable LO driver bias circuits */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON7,
				   RG_AUDLOLPWRUP_IBIAS_VAUDP32_MASK_SFT, 0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_adc_clk_gen_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol,
				int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x, vow_enable %d\n",
		 __func__, event, priv->vow_enable);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (priv->vow_enable) {
			/* ADC CLK from CLKGEN (3.25MHz) */
			dev_info(priv->dev, "%s(), vow mode\n", __func__);
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKSOURCE_MASK_SFT,
					   0x1 << RG_AUDADCCLKSOURCE_SFT);
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKSEL_MASK_SFT,
					   0x1 << RG_AUDADCCLKSEL_SFT);
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKGENMODE_MASK_SFT, 0x0);
		} else {
			/* ADC CLK from CLKGEN (6.5MHz) */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKSOURCE_MASK_SFT, 0x0);
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKSEL_MASK_SFT, 0x0);
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
					   RG_AUDADCCLKGENMODE_MASK_SFT,
					   0x1 << RG_AUDADCCLKGENMODE_SFT);
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
				   RG_AUDADCCLKSOURCE_MASK_SFT, 0x0);
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
				   RG_AUDADCCLKSEL_MASK_SFT, 0x0);
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON5,
				   RG_AUDADCCLKGENMODE_MASK_SFT, 0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_dcc_clk_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* DCC 50k CLK (from 26M) */
		/* MT6359_AFE_DCCLK_CFG0, bit 3 for dm ck swap */
		regmap_update_bits(priv->regmap, MT6359_AFE_DCCLK_CFG0,
				   0xfff7, 0x2062);
		regmap_update_bits(priv->regmap, MT6359_AFE_DCCLK_CFG0,
				   0xfff7, 0x2060);
		regmap_update_bits(priv->regmap, MT6359_AFE_DCCLK_CFG0,
				   0xfff7, 0x2061);
		regmap_write(priv->regmap, MT6359_AFE_DCCLK_CFG1, 0x0100);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(priv->regmap, MT6359_AFE_DCCLK_CFG0,
				   0xfff7, 0x2060);
		regmap_update_bits(priv->regmap, MT6359_AFE_DCCLK_CFG0,
				   0xfff7, 0x2062);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_mic_bias_0_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE_0];

	dev_info(priv->dev, "%s(), event 0x%x, mic_type %d\n",
		 __func__, event, mic_type);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		switch (mic_type) {
		case MIC_TYPE_MUX_DCC_ECM_DIFF:
		case MIC_TYPE_MUX_VOW_DCC_ECM_DIFF:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON15,
					   0xff00, 0x7700);
			break;
		case MIC_TYPE_MUX_DCC_ECM_SINGLE:
		case MIC_TYPE_MUX_VOW_DCC_ECM_SINGLE:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON15,
					   0xff00, 0x1100);
			break;
		default:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON15,
					   0xff00, 0x0000);
			break;
		}

		/* MISBIAS0 = 1P9V */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON15,
				   RG_AUDMICBIAS0VREF_MASK_SFT,
				   MIC_BIAS_1P9 << RG_AUDMICBIAS0VREF_SFT);
		/* low power select */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON15,
				   RG_AUDMICBIAS0LOWPEN_MASK_SFT,
				   (IS_VOW_AMIC_BASE(mic_type) ? 1 : 0)
				   << RG_AUDMICBIAS0LOWPEN_SFT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Disable MICBIAS0, MISBIAS0 = 1P7V */
		regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON15, 0x0000);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_mic_bias_1_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE_1];

	dev_info(priv->dev, "%s(), event 0x%x, mic_type %d\n",
		 __func__, event, mic_type);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* MISBIAS1 = 2P6V */
		if (mic_type == MIC_TYPE_MUX_DCC_ECM_SINGLE)
			regmap_write(priv->regmap,
				     MT6359_AUDENC_ANA_CON16, 0x0160);
		else
			regmap_write(priv->regmap,
				     MT6359_AUDENC_ANA_CON16, 0x0060);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_mic_bias_2_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE_2];

	dev_info(priv->dev, "%s(), event 0x%x, mic_type %d\n",
		 __func__, event, mic_type);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		switch (mic_type) {
		case MIC_TYPE_MUX_DCC_ECM_DIFF:
		case MIC_TYPE_MUX_VOW_DCC_ECM_DIFF:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON17,
					   0xff00, 0x7700);
			break;
		case MIC_TYPE_MUX_DCC_ECM_SINGLE:
		case MIC_TYPE_MUX_VOW_DCC_ECM_SINGLE:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON17,
					   0xff00, 0x1100);
			break;
		default:
			regmap_update_bits(priv->regmap,
					   MT6359_AUDENC_ANA_CON17,
					   0xff00, 0x0000);
			break;
		}

		/* MISBIAS2 = 1P9V */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON17,
				   RG_AUDMICBIAS0VREF_MASK_SFT,
				   MIC_BIAS_1P9 << RG_AUDMICBIAS0VREF_SFT);
		/* low power select */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON17,
				   RG_AUDMICBIAS0LOWPEN_MASK_SFT,
				   (IS_VOW_AMIC_BASE(mic_type) ? 1 : 0)
				   << RG_AUDMICBIAS0LOWPEN_SFT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Disable MICBIAS2, MISBIAS0 = 1P7V */
		regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON17, 0x0000);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_vow_aud_lpw_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol,
				int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x\n", __func__, event);
#if 0
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Enable audio uplink LPW mode */
		/* Enable Audio ADC 1st Stage LPW */
		/* Enable Audio ADC 2nd & 3rd LPW */
		/* Enable Audio ADC flash Audio ADC flash */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   0x0039, 0x0039);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Disable audio uplink LPW mode */
		/* Disable Audio ADC 1st Stage LPW */
		/* Disable Audio ADC 2nd & 3rd LPW */
		/* Disable Audio ADC flash Audio ADC flash */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   0x0039, 0x0000);
		break;
	default:
		break;
	}
#endif
	return 0;
}

static int mt_vow_out_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_WILL_PMU:
		priv->vow_enable = 1;
		break;
	case SND_SOC_DAPM_PRE_PMU:
#if 0
		/* vow gpio set */
		regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3_CLR,
				   0xffff, 0xffff);
		regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3_SET,
				   0xffff, 0x0120);
		regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3,
				   0xffff, 0x0120);
#endif
		break;
	case SND_SOC_DAPM_POST_PMD:
#if 0
		/* vow gpio clear */
		regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3_CLR,
				   0xffff, 0xffff);
		regmap_update_bits(priv->regmap, MT6359_GPIO_MODE3,
				   0xffff, 0x0000);
#endif
		priv->vow_enable = 0;
		break;
	default:
		break;
	}

	return 0;
}

#if 0
static int mt6359_vow_cfg_enable(struct mt6359_priv *priv)
{
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];

	dev_info(priv->dev, "%s(), mic_type %d\n", __func__, mic_type);

	/* Enable vow cfg setting */
	regmap_write(priv->regmap, MT6359_AFE_VOW_CFG0,
		     priv->reg_afe_vow_cfg0);
	regmap_write(priv->regmap, MT6359_AFE_VOW_CFG1,
		     priv->reg_afe_vow_cfg1);
	regmap_write(priv->regmap, MT6359_AFE_VOW_CFG2,
		     priv->reg_afe_vow_cfg2);
	regmap_write(priv->regmap, MT6359_AFE_VOW_CFG3,
		     priv->reg_afe_vow_cfg3);
	regmap_update_bits(priv->regmap, MT6359_AFE_VOW_CFG4,
			   0x000f, priv->reg_afe_vow_cfg4);
	regmap_write(priv->regmap, MT6359_AFE_VOW_CFG5,
		     priv->reg_afe_vow_cfg5);

	switch (mic_type) {
	case MIC_TYPE_MUX_VOW_DMIC_LP:
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_CFG4,
				   0xfff0, 0x024e);
		/* vow posdiv and cic mode configure*/
		/* LP DMIC settings : 812.5k */
		regmap_write(priv->regmap, MT6359_AFE_VOW_POSDIV_CFG0, 0x0c0a);
		/* VOW_DIGMIC_ON */
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_TOP,
				   0x20c0, 0x20c0);
		break;
	case MIC_TYPE_MUX_VOW_DMIC:
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_CFG4,
				   0xfff0, 0x029e);
		/* vow posdiv and cic mode configure */
		/* DMIC settings : 1600k */
		regmap_write(priv->regmap, MT6359_AFE_VOW_POSDIV_CFG0, 0x0c00);
		/* VOW_DIGMIC_ON */
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_TOP,
				   0x20c0, 0x20c0);
		break;
	default:
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_CFG4,
				   0xfff0, 0x029e);
		/* vow posdiv and cic mode configure */
		/* AMIC settings : 1600k */
		regmap_write(priv->regmap, MT6359_AFE_VOW_POSDIV_CFG0, 0x0c00);
		/* VOW_DIGMIC_OFF */
		regmap_update_bits(priv->regmap, MT6359_AFE_VOW_TOP,
				   0x20c0, 0x0000);
		break;
	}
	return 0;
}

static void mt6359_vow_cfg_disable(struct mt6359_priv *priv)
{
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];

	dev_info(priv->dev, "%s(), mic_type 0x%x\n", __func__, mic_type);

	/* VOW_DIGMIC_OFF */
	regmap_update_bits(priv->regmap, MT6359_AFE_VOW_TOP,
			   0x20c0, 0x0000);
}

static int mt6359_dmic_enable(struct mt6359_priv *priv)
{
	dev_info(priv->dev, "%s()\n", __func__);

	/* mic bias */
	/* Enable MICBIAS0, MISBIAS0 = 1P9V */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON9, 0x0021);

	/* RG_BANDGAPGEN=1'b0 */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON10,
			   0x1 << 12, 0x0);

	/* DMIC enable */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON8, 0x0005);

	/* here to set digital part */
	mt6359_mtkaif_tx_enable(priv);

	/* UL dmic setting */
	regmap_write(priv->regmap, MT6359_AFE_UL_SRC_CON0_H, 0x0080);

	/* UL turn on */
	regmap_write(priv->regmap, MT6359_AFE_UL_SRC_CON0_L, 0x0003);
	return 0;
}

static void mt6359_dmic_disable(struct mt6359_priv *priv)
{
	dev_info(priv->dev, "%s()\n", __func__);

	/* UL turn off */
	regmap_update_bits(priv->regmap, MT6359_AFE_UL_SRC_CON0_L,
			   0x0003, 0x0000);

	/* disable aud_pad TX fifos */
	mt6359_mtkaif_tx_disable(priv);

	/* DMIC disable */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON8, 0x0000);

	/* mic bias */
	/* MISBIAS0 = 1P7V */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON9, 0x0001);

	/* RG_BANDGAPGEN=1'b0 */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON10,
			   0x1 << 12, 0x0);

	/* MICBIA0 disable */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON9, 0x0000);
}

static int mt6359_vow_amic_enable(struct mt6359_priv *priv)
{
	/* TODO: please use mix ctl set gain in user space */
	/* set mic pga gain : Audio L PGA 18 dB gain*/
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
			   RG_AUDPREAMPLGAIN_MASK_SFT,
			   0x03 << RG_AUDPREAMPLGAIN_SFT);
	return 0;
}

static int mt6359_vow_amic_disable(struct mt6359_priv *priv)
{
	return 0;
}

static int mt6359_vow_dmic_enable(struct mt6359_priv *priv)
{
	dev_info(priv->dev, "%s()\n", __func__);
	/* mic bias */
	/* Enable MICBIAS0, MISBIAS0 = 1P9V */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON9, 0x0025);

	/* DMIC enable */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON8, 0x0005);

	return 0;
}

static int mt6359_vow_dmic_disable(struct mt6359_priv *priv)
{
	dev_info(priv->dev, "%s()\n", __func__);
	/* DMIC disable */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON8, 0x0004);

	/* mic bias */
	/* MISBIAS0 = 1P7V */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON9, 0x0000);
	return 0;
}
#endif
static int mt_mtkaif_tx_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mt6359_mtkaif_tx_enable(priv);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mt6359_mtkaif_tx_disable(priv);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_ul_src_dmic_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol,
				int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* default two wire, 3.25M */
		regmap_write(priv->regmap, MT6359_AFE_UL_SRC_CON0_H, 0x0080);
		regmap_update_bits(priv->regmap, MT6359_AFE_UL_SRC_CON0_L,
				   0xfffc, 0x0000);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(priv->regmap,
			     MT6359_AFE_UL_SRC_CON0_H, 0x0000);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_ul_src_34_dmic_event(struct snd_soc_dapm_widget *w,
				   struct snd_kcontrol *kcontrol,
				   int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* default two wire, 3.25M */
		regmap_write(priv->regmap,
			     MT6359_AFE_ADDA6_L_SRC_CON0_H, 0x0080);
		regmap_update_bits(priv->regmap, MT6359_AFE_ADDA6_UL_SRC_CON0_L,
				   0xfffc, 0x0000);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(priv->regmap,
			     MT6359_AFE_ADDA6_L_SRC_CON0_H, 0x0000);
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
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		usleep_range(100, 120);
		/* Audio L preamplifier DCC precharge off */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
				   RG_AUDPREAMPLDCPRECHARGE_MASK_SFT,
				   0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_adc_r_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		usleep_range(100, 120);
		/* Audio R preamplifier DCC precharge off */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
				   RG_AUDPREAMPRDCPRECHARGE_MASK_SFT,
				   0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_adc_3_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	dev_info(priv->dev, "%s(), event = 0x%x\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		usleep_range(100, 120);
		/* Audio R preamplifier DCC precharge off */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   RG_AUDPREAMP3DCPRECHARGE_MASK_SFT,
				   0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_pga_l_mux_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_info(priv->dev, "%s(), mux %d\n", __func__, mux);
	priv->mux_select[MUX_PGA_L] = mux >> RG_AUDPREAMPLINPUTSEL_SFT;
	return 0;
}

static int mt_pga_r_mux_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_info(priv->dev, "%s(), mux %d\n", __func__, mux);
	priv->mux_select[MUX_PGA_R] = mux >> RG_AUDPREAMPRINPUTSEL_SFT;
	return 0;
}

static int mt_pga_3_mux_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mux = dapm_kcontrol_get_value(w->kcontrols[0]);

	dev_info(priv->dev, "%s(), mux %d\n", __func__, mux);
	priv->mux_select[MUX_PGA_3] = mux >> RG_AUDPREAMP3INPUTSEL_SFT;
	return 0;
}

static int mt_pga_l_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int mic_gain_l = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP1];
	unsigned int mux_pga = priv->mux_select[MUX_PGA_L];
	unsigned int mic_type;

	switch (mux_pga) {
	case PGA_L_MUX_AIN0:
		mic_type = priv->mux_select[MUX_MIC_TYPE_0];
		break;
	case PGA_L_MUX_AIN1:
		mic_type = priv->mux_select[MUX_MIC_TYPE_1];
		break;
	default:
		dev_err(priv->dev, "%s(), invalid pga mux %d\n",
			__func__, mux_pga);
		return -EINVAL;
	}

	dev_info(priv->dev, "%s(), event = 0x%x, mic_type %d, mic_gain_l %d, mux_pga %d\n",
		 __func__, event, mic_type, mic_gain_l, mux_pga);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (IS_DCC_BASE(mic_type)) {
			/* Audio L preamplifier DCC precharge */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
					   RG_AUDPREAMPLDCPRECHARGE_MASK_SFT,
					   0x1 << RG_AUDPREAMPLDCPRECHARGE_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* set mic pga gain */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
				   RG_AUDPREAMPLGAIN_MASK_SFT,
				   mic_gain_l << RG_AUDPREAMPLGAIN_SFT);

		if (IS_DCC_BASE(mic_type)) {
			/* L preamplifier DCCEN */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
					   RG_AUDPREAMPLDCCEN_MASK_SFT,
					   0x1 << RG_AUDPREAMPLDCCEN_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* L preamplifier DCCEN */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON0,
				   RG_AUDPREAMPLDCCEN_MASK_SFT,
				   0x0 << RG_AUDPREAMPLDCCEN_SFT);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_pga_r_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int mic_gain_r = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP2];
	unsigned int mux_pga = priv->mux_select[MUX_PGA_R];
	unsigned int mic_type;

	switch (mux_pga) {
	case PGA_R_MUX_AIN0:
	case PGA_R_MUX_AIN2:
		mic_type = priv->mux_select[MUX_MIC_TYPE_0];
		break;
	case PGA_R_MUX_AIN3:
		mic_type = priv->mux_select[MUX_MIC_TYPE_2];
		break;
	default:
		dev_err(priv->dev, "%s(), invalid pga mux %d\n",
			__func__, mux_pga);
		return -EINVAL;
	}

	dev_info(priv->dev, "%s(), event = 0x%x, mic_type %d, mic_gain_r %d, mux_pga %d\n",
		 __func__, event, mic_type, mic_gain_r, mux_pga);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (IS_DCC_BASE(mic_type)) {
			/* Audio R preamplifier DCC precharge */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
					   RG_AUDPREAMPRDCPRECHARGE_MASK_SFT,
					   0x1 << RG_AUDPREAMPRDCPRECHARGE_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* set mic pga gain */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
				   RG_AUDPREAMPRGAIN_MASK_SFT,
				   mic_gain_r << RG_AUDPREAMPRGAIN_SFT);

		if (IS_DCC_BASE(mic_type)) {
			/* R preamplifier DCCEN */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
					   RG_AUDPREAMPRDCCEN_MASK_SFT,
					   0x1 << RG_AUDPREAMPRDCCEN_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* R preamplifier DCCEN */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON1,
				   RG_AUDPREAMPRDCCEN_MASK_SFT,
				   0x0 << RG_AUDPREAMPRDCCEN_SFT);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_pga_3_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol,
			  int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int mic_gain_3 = priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP3];
	unsigned int mux_pga = priv->mux_select[MUX_PGA_3];
	unsigned int mic_type;

	switch (mux_pga) {
	case PGA_3_MUX_AIN2:
		mic_type = priv->mux_select[MUX_MIC_TYPE_0];
		break;
	case PGA_3_MUX_AIN3:
		mic_type = priv->mux_select[MUX_MIC_TYPE_2];
		break;
	default:
		dev_err(priv->dev, "%s(), invalid pga mux %d\n",
			__func__, mux_pga);
		return -EINVAL;
	}

	dev_info(priv->dev, "%s(), event = 0x%x, mic_type %d, mic_gain_3 %d, mux_pga %d\n",
		 __func__, event, mic_type, mic_gain_3, mux_pga);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (IS_DCC_BASE(mic_type)) {
			/* Audio 3 preamplifier DCC precharge */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
					   RG_AUDPREAMP3DCPRECHARGE_MASK_SFT,
					   0x1 << RG_AUDPREAMP3DCPRECHARGE_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		/* set mic pga gain */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   RG_AUDPREAMP3GAIN_MASK_SFT,
				   mic_gain_3 << RG_AUDPREAMP3GAIN_SFT);

		if (IS_DCC_BASE(mic_type)) {
			/* 3 preamplifier DCCEN */
			regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
					   RG_AUDPREAMP3DCCEN_MASK_SFT,
					   0x1 << RG_AUDPREAMP3DCCEN_SFT);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* 3 preamplifier DCCEN */
		regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON2,
				   RG_AUDPREAMP3DCCEN_MASK_SFT,
				   0x0 << RG_AUDPREAMP3DCCEN_SFT);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_delay_250_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
	case SND_SOC_DAPM_PRE_PMD:
		usleep_range(250, 270);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_delay_100_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
	case SND_SOC_DAPM_PRE_PMD:
		usleep_range(100, 120);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_hp_pull_down_event(struct snd_soc_dapm_widget *w,
				 struct snd_kcontrol *kcontrol,
				 int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hp_pull_down(priv, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hp_pull_down(priv, false);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_hp_mute_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Set HPR/HPL gain to -22dB */
		regmap_write(priv->regmap, MT6359_ZCD_CON2, DL_GAIN_N_22DB_REG);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Set HPL/HPR gain to mute */
		regmap_write(priv->regmap, MT6359_ZCD_CON2, DL_GAIN_N_40DB_REG);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_hp_damp_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_POST_PMD:
		/* Disable HP damping circuit & HPN 4K load */
		/* reset CMFB PW level */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x0000);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_esd_resist_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol,
			       int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* Reduce ESD resistance of AU_REFN */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
				   RG_AUDREFN_DERES_EN_VAUDP32_MASK_SFT,
				   0x1 << RG_AUDREFN_DERES_EN_VAUDP32_SFT);
		usleep_range(250, 270);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Increase ESD resistance of AU_REFN */
		regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
				   RG_AUDREFN_DERES_EN_VAUDP32_MASK_SFT, 0x0);
		break;
	default:
		break;
	}

	return 0;
}

static int mt_sdm_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol,
			int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* sdm audio fifo clock power on */
		regmap_update_bits(priv->regmap, MT6359_AFUNC_AUD_CON2,
				   0xfffd, 0x0006);
		/* scrambler clock on enable */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xCBA1);
		/* sdm power on */
		regmap_update_bits(priv->regmap, MT6359_AFUNC_AUD_CON2,
				   0xfffd, 0x0003);
		/* sdm fifo enable */
		regmap_update_bits(priv->regmap, MT6359_AFUNC_AUD_CON2,
				   0xfffd, 0x000B);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DL scrambler disabling sequence */
		regmap_update_bits(priv->regmap, MT6359_AFUNC_AUD_CON2,
				   0xfffd, 0x0000);
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xcba0);
		break;
	default:
		break;
	}
	return 0;
}

static int mt_sdm_3rd_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* sdm audio fifo clock power on */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON11, 0x0006);
		/* scrambler clock on enable */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON9, 0xCBA1);
		/* sdm power on */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON11, 0x0003);
		/* sdm fifo enable */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON11, 0x000B);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* DL scrambler disabling sequence */
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON11, 0x0000);
		regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON9, 0xcba0);
		break;
	default:
		break;
	}
	return 0;
}

static int mt_ncp_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol,
			int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc800);
		break;
	default:
		break;
	}
	return 0;
}

static int mt_dl_gpio_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		playback_gpio_set(priv);
		break;
	case SND_SOC_DAPM_POST_PMD:
		playback_gpio_reset(priv);
		break;
	default:
		break;
	}
	return 0;
}

static int mt_ul_gpio_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol,
			    int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		capture_gpio_set(priv);
		break;
	case SND_SOC_DAPM_POST_PMD:
		capture_gpio_reset(priv);
		break;
	default:
		break;
	}
	return 0;
}

static int mt_dc_trim_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol,
			      int event)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	struct dc_trim_data *dc_trim = &priv->dc_trim;

	dev_dbg(priv->dev, "%s(), event = 0x%x, dc_trim->calibrated %u\n",
		__func__, event, dc_trim->calibrated);

	if (dc_trim->calibrated)
		return 0;

	kthread_run(dc_trim_thread, priv, "dc_trim_thread");
	return 0;
}

/* DAPM Widgets */
static const struct snd_soc_dapm_widget mt6359_dapm_widgets[] = {
	/* Global Supply*/
	SND_SOC_DAPM_SUPPLY_S("CLK_BUF", SUPPLY_SEQ_CLK_BUF,
			      MT6359_DCXO_CW12,
			      RG_XO_AUDIO_EN_M_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDGLB", SUPPLY_SEQ_AUD_GLB,
			      MT6359_AUDDEC_ANA_CON13,
			      RG_AUDGLB_PWRDN_VA32_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("CLKSQ Audio", SUPPLY_SEQ_CLKSQ,
			      MT6359_AUDENC_ANA_CON23,
			      RG_CLKSQ_EN_SFT, 0,
			      mt_clksq_event,
			      SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_SUPPLY_S("AUDNCP_CK", SUPPLY_SEQ_TOP_CK,
			      MT6359_AUD_TOP_CKPDN_CON0,
			      RG_AUDNCP_CK_PDN_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("ZCD13M_CK", SUPPLY_SEQ_TOP_CK,
			      MT6359_AUD_TOP_CKPDN_CON0,
			      RG_ZCD13M_CK_PDN_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUD_CK", SUPPLY_SEQ_TOP_CK_LAST,
			      MT6359_AUD_TOP_CKPDN_CON0,
			      RG_AUD_CK_PDN_SFT, 1,
			      mt_delay_250_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUDIF_CK", SUPPLY_SEQ_TOP_CK,
			      MT6359_AUD_TOP_CKPDN_CON0,
			      RG_AUDIF_CK_PDN_SFT, 1, NULL, 0),
	/* vow */
	SND_SOC_DAPM_SUPPLY_S("VOW_AUD_LPW", SUPPLY_SEQ_VOW_AUD_LPW,
			      SND_SOC_NOPM, 0, 0,
			      mt_vow_aud_lpw_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUD_VOW", SUPPLY_SEQ_AUD_VOW,
			      MT6359_AUDENC_ANA_CON23,
			      RG_AUDIO_VOW_EN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("VOW_CLK", SUPPLY_SEQ_VOW_CLK,
			      MT6359_DCXO_CW11,
			      RG_XO_VOW_EN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("VOW_LDO", SUPPLY_SEQ_VOW_LDO,
			      MT6359_AUDENC_ANA_CON23,
			      RG_CLKSQ_EN_VOW_SFT, 0, NULL, 0),

	/* Digital Clock */
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_AFE_CTL", SUPPLY_SEQ_AUD_TOP_LAST,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_AFE_CTL_SFT, 1,
			      mt_delay_250_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_DAC_CTL", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_DAC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_ADC_CTL", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_ADC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_ADDA6_ADC_CTL", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_ADDA6_ADC_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_I2S_DL", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_I2S_DL_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PWR_CLK", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PWR_CLK_DIS_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PDN_AFE_TESTMODEL", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_AFE_TESTMODEL_CTL_SFT, 1, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("AUDIO_TOP_PDN_RESERVED", SUPPLY_SEQ_AUD_TOP,
			      MT6359_AUDIO_TOP_CON0,
			      PDN_RESERVED_SFT, 1, NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("SDM", SUPPLY_SEQ_DL_SDM,
			      SND_SOC_NOPM, 0, 0,
			      mt_sdm_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("SDM_3RD", SUPPLY_SEQ_DL_SDM,
			      SND_SOC_NOPM, 0, 0,
			      mt_sdm_3rd_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* ch123 share SDM FIFO CLK */
	SND_SOC_DAPM_SUPPLY_S("SDM_FIFO_CLK", SUPPLY_SEQ_DL_SDM_FIFO_CLK,
			      MT6359_AFUNC_AUD_CON2,
			      CCI_AFIFO_CLK_PWDB_SFT, 0,
			      NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("NCP", SUPPLY_SEQ_DL_NCP,
			      MT6359_AFE_NCP_CFG0,
			      RG_NCP_ON_SFT, 0,
			      mt_ncp_event,
			      SND_SOC_DAPM_PRE_PMU),

	SND_SOC_DAPM_SUPPLY("DL Digital Clock", SND_SOC_NOPM,
			    0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DL Digital Clock CH_1_2", SND_SOC_NOPM,
			    0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("DL Digital Clock CH_3", SND_SOC_NOPM,
			    0, 0, NULL, 0),

	/* AFE ON */
	SND_SOC_DAPM_SUPPLY_S("AFE_ON", SUPPLY_SEQ_AFE,
			      MT6359_AFE_UL_DL_CON0, AFE_ON_SFT, 0,
			      NULL, 0),

	/* GPIO */
	SND_SOC_DAPM_SUPPLY_S("DL_GPIO", SUPPLY_SEQ_DL_GPIO,
			      SND_SOC_NOPM, 0, 0,
			      mt_dl_gpio_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("UL_GPIO", SUPPLY_SEQ_UL_GPIO,
			      SND_SOC_NOPM, 0, 0,
			      mt_ul_gpio_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* AIF Rx*/
	SND_SOC_DAPM_AIF_IN("AIF_RX", "AIF1 Playback", 0,
			    SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_IN("AIF2_RX", "AIF2 Playback", 0,
			    SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SUPPLY_S("AFE_DL_SRC", SUPPLY_SEQ_DL_SRC,
			      MT6359_AFE_DL_SRC2_CON0_L,
			      DL_2_SRC_ON_TMP_CTL_PRE_SFT, 0,
			      NULL, 0),

	/* DL Supply */
	SND_SOC_DAPM_SUPPLY("DL Power Supply", SND_SOC_NOPM,
			    0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("ESD_RESIST", SUPPLY_SEQ_DL_ESD_RESIST,
			      SND_SOC_NOPM,
			      0, 0,
			      mt_esd_resist_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("LDO", SUPPLY_SEQ_DL_LDO,
			      MT6359_AUDDEC_ANA_CON14,
			      RG_LCLDO_DEC_EN_VA32_SFT, 0,
			      NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("LDO_REMOTE", SUPPLY_SEQ_DL_LDO_REMOTE_SENSE,
			      MT6359_AUDDEC_ANA_CON14,
			      RG_LCLDO_DEC_REMOTE_SENSE_VA18_SFT, 0,
			      NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("NV_REGULATOR", SUPPLY_SEQ_DL_NV,
			      MT6359_AUDDEC_ANA_CON14,
			      RG_NVREG_EN_VAUDP32_SFT, 0,
			      mt_delay_100_event, SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY_S("IBIST", SUPPLY_SEQ_DL_IBIST,
			      MT6359_AUDDEC_ANA_CON12,
			      RG_AUDIBIASPWRDN_VAUDP32_SFT, 1,
			      NULL, 0),

	/* DAC */
	SND_SOC_DAPM_MUX("DAC In Mux", SND_SOC_NOPM, 0, 0, &dac_in_mux_control),

	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_DAC("DAC_3RD", NULL, SND_SOC_NOPM, 0, 0),

	/* Headphone */
	SND_SOC_DAPM_MUX_E("HPL Mux", SND_SOC_NOPM, 0, 0,
			   &hpl_in_mux_control,
			   mt_hp_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_MUX_E("HPR Mux", SND_SOC_NOPM, 0, 0,
			   &hpr_in_mux_control,
			   mt_hp_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_SUPPLY("HP_Supply", SND_SOC_NOPM,
			    0, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("HP_PULL_DOWN", SUPPLY_SEQ_HP_PULL_DOWN,
			      SND_SOC_NOPM,
			      0, 0,
			      mt_hp_pull_down_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("HP_MUTE", SUPPLY_SEQ_HP_MUTE,
			      SND_SOC_NOPM,
			      0, 0,
			      mt_hp_mute_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("HP_DAMP", SUPPLY_SEQ_HP_DAMPING_OFF_RESET_CMFB,
			      SND_SOC_NOPM,
			      0, 0,
			      mt_hp_damp_event,
			      SND_SOC_DAPM_POST_PMD),

	/* Receiver */
	SND_SOC_DAPM_MUX_E("RCV Mux", SND_SOC_NOPM, 0, 0,
			   &rcv_in_mux_control,
			   mt_rcv_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	/* LOL */
	SND_SOC_DAPM_MUX_E("LOL Mux", SND_SOC_NOPM, 0, 0,
			   &lo_in_mux_control,
			   mt_lo_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	/* Outputs */
	SND_SOC_DAPM_OUTPUT("Receiver"),
	SND_SOC_DAPM_OUTPUT("Headphone L"),
	SND_SOC_DAPM_OUTPUT("Headphone R"),
	SND_SOC_DAPM_OUTPUT("Headphone L Ext Spk Amp"),
	SND_SOC_DAPM_OUTPUT("Headphone R Ext Spk Amp"),
	SND_SOC_DAPM_OUTPUT("LINEOUT L"),

	/* SGEN */
	SND_SOC_DAPM_SUPPLY("SGEN DL Enable", MT6359_AFE_SGEN_CFG0,
			    SGEN_DAC_EN_CTL_SFT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("SGEN MUTE", MT6359_AFE_SGEN_CFG0,
			    SGEN_MUTE_SW_CTL_SFT, 1,
			    mt_sgen_event,
			    SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY("SGEN DL SRC", MT6359_AFE_DL_SRC2_CON0_L,
			    DL_2_SRC_ON_TMP_CTL_PRE_SFT, 0, NULL, 0),
			/* tricky, same reg/bit as "AIF_RX", reconsider */

	SND_SOC_DAPM_INPUT("SGEN DL"),

	/* Uplinks */
	SND_SOC_DAPM_AIF_OUT("AIF1TX", "AIF1 Capture", 0,
			     SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF2TX", "AIF2 Capture", 0,
			     SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SUPPLY_S("ADC_CLKGEN", SUPPLY_SEQ_ADC_CLKGEN,
			      MT6359_AUDENC_ANA_CON5, RG_AUDADCCLKRSTB_SFT, 0,
			      mt_adc_clk_gen_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_SUPPLY_S("DCC_CLK", SUPPLY_SEQ_DCC_CLK,
			      SND_SOC_NOPM, 0, 0,
			      mt_dcc_clk_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* Uplinks MUX */
	SND_SOC_DAPM_MUX("AIF Out Mux", SND_SOC_NOPM, 0, 0,
			 &aif_out_mux_control),

	SND_SOC_DAPM_MUX("AIF2 Out Mux", SND_SOC_NOPM, 0, 0,
			 &aif2_out_mux_control),

	SND_SOC_DAPM_SUPPLY("AIFTX_Supply", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("MTKAIF_TX", SUPPLY_SEQ_UL_MTKAIF,
			      SND_SOC_NOPM, 0, 0,
			      mt_mtkaif_tx_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("UL_SRC", SUPPLY_SEQ_UL_SRC,
			      MT6359_AFE_UL_SRC_CON0_L,
			      UL_SRC_ON_TMP_CTL_SFT, 0,
			      NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("UL_SRC_DMIC", SUPPLY_SEQ_UL_SRC_DMIC,
			      SND_SOC_NOPM, 0, 0,
			      mt_ul_src_dmic_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SUPPLY_S("UL_SRC_34", SUPPLY_SEQ_UL_SRC,
			      MT6359_AFE_ADDA6_UL_SRC_CON0_L,
			      ADDA6_UL_SRC_ON_TMP_CTL_SFT, 0,
			      NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("UL_SRC_34_DMIC", SUPPLY_SEQ_UL_SRC_DMIC,
			      SND_SOC_NOPM, 0, 0,
			      mt_ul_src_34_dmic_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("MISO0_MUX", SND_SOC_NOPM, 0, 0, &miso0_mux_control),
	SND_SOC_DAPM_MUX("MISO1_MUX", SND_SOC_NOPM, 0, 0, &miso1_mux_control),
	SND_SOC_DAPM_MUX("MISO2_MUX", SND_SOC_NOPM, 0, 0, &miso2_mux_control),

	SND_SOC_DAPM_MUX("UL_SRC_MUX", SND_SOC_NOPM, 0, 0,
			 &ul_src_mux_control),
	SND_SOC_DAPM_MUX("UL2_SRC_MUX", SND_SOC_NOPM, 0, 0,
			 &ul2_src_mux_control),

	SND_SOC_DAPM_MUX("AMIC0_MUX", SND_SOC_NOPM, 0, 0, &amic0_mux_control),
	SND_SOC_DAPM_MUX("AMIC1_MUX", SND_SOC_NOPM, 0, 0, &amic1_mux_control),
	SND_SOC_DAPM_MUX("AMIC2_MUX", SND_SOC_NOPM, 0, 0, &amic2_mux_control),
	SND_SOC_DAPM_MUX("DMIC0_MUX", SND_SOC_NOPM, 0, 0, &dmic0_mux_control),
	SND_SOC_DAPM_MUX("DMIC1_MUX", SND_SOC_NOPM, 0, 0, &dmic1_mux_control),
	SND_SOC_DAPM_MUX("DMIC2_MUX", SND_SOC_NOPM, 0, 0, &dmic2_mux_control),

	SND_SOC_DAPM_MUX_E("ADC_L_Mux", SND_SOC_NOPM, 0, 0,
			   &adc_left_mux_control, NULL, 0),
	SND_SOC_DAPM_MUX_E("ADC_R_Mux", SND_SOC_NOPM, 0, 0,
			   &adc_right_mux_control, NULL, 0),
	SND_SOC_DAPM_MUX_E("ADC_3_Mux", SND_SOC_NOPM, 0, 0,
			   &adc_3_mux_control, NULL, 0),

	SND_SOC_DAPM_ADC("ADC_L", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC_R", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC_3", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SUPPLY_S("ADC_L_EN", SUPPLY_SEQ_UL_ADC,
			      MT6359_AUDENC_ANA_CON0,
			      RG_AUDADCLPWRUP_SFT, 0,
			      mt_adc_l_event,
			      SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY_S("ADC_R_EN", SUPPLY_SEQ_UL_ADC,
			      MT6359_AUDENC_ANA_CON1,
			      RG_AUDADCRPWRUP_SFT, 0,
			      mt_adc_r_event,
			      SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY_S("ADC_3_EN", SUPPLY_SEQ_UL_ADC,
			      MT6359_AUDENC_ANA_CON2,
			      RG_AUDADC3PWRUP_SFT, 0,
			      mt_adc_3_event,
			      SND_SOC_DAPM_POST_PMU),

	SND_SOC_DAPM_MUX_E("PGA_L_Mux", SND_SOC_NOPM, 0, 0,
			   &pga_left_mux_control,
			   mt_pga_l_mux_event,
			   SND_SOC_DAPM_WILL_PMU),
	SND_SOC_DAPM_MUX_E("PGA_R_Mux", SND_SOC_NOPM, 0, 0,
			   &pga_right_mux_control,
			   mt_pga_r_mux_event,
			   SND_SOC_DAPM_WILL_PMU),
	SND_SOC_DAPM_MUX_E("PGA_3_Mux", SND_SOC_NOPM, 0, 0,
			   &pga_3_mux_control,
			   mt_pga_3_mux_event,
			   SND_SOC_DAPM_WILL_PMU),

	SND_SOC_DAPM_PGA("PGA_L", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("PGA_R", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("PGA_3", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY_S("PGA_L_EN", SUPPLY_SEQ_UL_PGA,
			      MT6359_AUDENC_ANA_CON0,
			      RG_AUDPREAMPLON_SFT, 0,
			      mt_pga_l_event,
			      SND_SOC_DAPM_PRE_PMU |
			      SND_SOC_DAPM_POST_PMU |
			      SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("PGA_R_EN", SUPPLY_SEQ_UL_PGA,
			      MT6359_AUDENC_ANA_CON1,
			      RG_AUDPREAMPRON_SFT, 0,
			      mt_pga_r_event,
			      SND_SOC_DAPM_PRE_PMU |
			      SND_SOC_DAPM_POST_PMU |
			      SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("PGA_3_EN", SUPPLY_SEQ_UL_PGA,
			      MT6359_AUDENC_ANA_CON2,
			      RG_AUDPREAMP3ON_SFT, 0,
			      mt_pga_3_event,
			      SND_SOC_DAPM_PRE_PMU |
			      SND_SOC_DAPM_POST_PMU |
			      SND_SOC_DAPM_POST_PMD),

	/* UL input */
	SND_SOC_DAPM_INPUT("AIN0"),
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIN2"),
	SND_SOC_DAPM_INPUT("AIN3"),

	SND_SOC_DAPM_INPUT("AIN0_DMIC"),
	SND_SOC_DAPM_INPUT("AIN2_DMIC"),
	SND_SOC_DAPM_INPUT("AIN3_DMIC"),

	/* mic bias */
	SND_SOC_DAPM_SUPPLY_S("MIC_BIAS_0", SUPPLY_SEQ_MIC_BIAS,
			      MT6359_AUDENC_ANA_CON15,
			      RG_AUDPWDBMICBIAS0_SFT, 0,
			      mt_mic_bias_0_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY_S("MIC_BIAS_1", SUPPLY_SEQ_MIC_BIAS,
			      MT6359_AUDENC_ANA_CON16,
			      RG_AUDPWDBMICBIAS1_SFT, 0,
			      mt_mic_bias_1_event,
			      SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_SUPPLY_S("MIC_BIAS_2", SUPPLY_SEQ_MIC_BIAS,
			      MT6359_AUDENC_ANA_CON17,
			      RG_AUDPWDBMICBIAS2_SFT, 0,
			      mt_mic_bias_2_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* dmic */
	SND_SOC_DAPM_SUPPLY_S("DMIC_0", SUPPLY_SEQ_DMIC,
			      MT6359_AUDENC_ANA_CON13,
			      RG_AUDDIGMICEN_SFT, 0,
			      NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("DMIC_1", SUPPLY_SEQ_DMIC,
			      MT6359_AUDENC_ANA_CON14,
			      RG_AUDDIGMIC1EN_SFT, 0,
			      NULL, 0),

	/* DC trim : trigger dc trim flow because set the reg when init_reg */
	/* this must be at the last widget */
	SND_SOC_DAPM_SUPPLY("DC Trim", MT6359_AUDDEC_ANA_CON8,
			    RG_AUDTRIMBUF_EN_VAUDP32_SFT, 0,
			    mt_dc_trim_event, SND_SOC_DAPM_POST_PMD),

	/* VOW */
	SND_SOC_DAPM_AIF_OUT_E("VOW TX", "VOW Capture", 0,
			       SND_SOC_NOPM, 0, 0,
			       mt_vow_out_event,
			       SND_SOC_DAPM_WILL_PMU |
			       SND_SOC_DAPM_PRE_PMU |
			       SND_SOC_DAPM_POST_PMD),
};
#if 0
static int mt_vow_amic_dcc_connect(struct snd_soc_dapm_widget *source,
				   struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];

	return (IS_VOW_DCC_BASE(mic_type)) ? 1 : 0;
}

static int mt_vow_amic_connect(struct snd_soc_dapm_widget *source,
			       struct snd_soc_dapm_widget *sink)
{

	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int mic_type = priv->mux_select[MUX_MIC_TYPE];

	return (IS_VOW_AMIC_BASE(mic_type)) ? 1 : 0;
}
#endif
static int mt_dcc_clk_connect(struct snd_soc_dapm_widget *source,
			      struct snd_soc_dapm_widget *sink)
{
	struct snd_soc_dapm_widget *w = sink;
	struct snd_soc_component *cmpnt = snd_soc_dapm_to_component(w->dapm);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	if (IS_DCC_BASE(priv->mux_select[MUX_MIC_TYPE_0]) ||
	    IS_DCC_BASE(priv->mux_select[MUX_MIC_TYPE_1]) ||
	    IS_DCC_BASE(priv->mux_select[MUX_MIC_TYPE_2]))
		return 1;
	else
		return 0;
}

static const struct snd_soc_dapm_route mt6359_dapm_routes[] = {
	/* Capture */
	{"AIFTX_Supply", NULL, "CLK_BUF"},
	{"AIFTX_Supply", NULL, "AUDGLB"},
	{"AIFTX_Supply", NULL, "CLKSQ Audio"},

	{"AIFTX_Supply", NULL, "AUD_CK"},
	{"AIFTX_Supply", NULL, "AUDIF_CK"},

	{"AIFTX_Supply", NULL, "AUDIO_TOP_AFE_CTL"},
	{"AIFTX_Supply", NULL, "AUDIO_TOP_PWR_CLK"},
	{"AIFTX_Supply", NULL, "AUDIO_TOP_PDN_RESERVED"},
	{"AIFTX_Supply", NULL, "AUDIO_TOP_I2S_DL"},
	/*
	 * *_ADC_CTL should enable only if UL_SRC in use,
	 * but dm ck may be needed even UL_SRC_x not in use
	 */
	{"AIFTX_Supply", NULL, "AUDIO_TOP_ADC_CTL"},
	{"AIFTX_Supply", NULL, "AUDIO_TOP_ADDA6_ADC_CTL"},

	{"AIFTX_Supply", NULL, "AFE_ON"},

	/* ul ch 12 */
	{"AIF1TX", NULL, "AIF Out Mux"},
		{"AIF1TX", NULL, "AIFTX_Supply"},
		{"AIF1TX", NULL, "UL_GPIO"},
		{"AIF1TX", NULL, "MTKAIF_TX"},

	{"AIF2TX", NULL, "AIF2 Out Mux"},
		{"AIF2TX", NULL, "AIFTX_Supply"},
		{"AIF2TX", NULL, "UL_GPIO"},
		{"AIF2TX", NULL, "MTKAIF_TX"},

	{"AIF Out Mux", "Normal Path", "MISO0_MUX"},
	{"AIF Out Mux", "Normal Path", "MISO1_MUX"},

	{"AIF2 Out Mux", "Normal Path", "MISO2_MUX"},

	{"MISO0_MUX", "UL1_CH1", "UL_SRC_MUX"},
	{"MISO0_MUX", "UL1_CH2", "UL_SRC_MUX"},
	{"MISO0_MUX", "UL2_CH1", "UL2_SRC_MUX"},
	{"MISO0_MUX", "UL2_CH2", "UL2_SRC_MUX"},

	{"MISO1_MUX", "UL1_CH1", "UL_SRC_MUX"},
	{"MISO1_MUX", "UL1_CH2", "UL_SRC_MUX"},
	{"MISO1_MUX", "UL2_CH1", "UL2_SRC_MUX"},
	{"MISO1_MUX", "UL2_CH2", "UL2_SRC_MUX"},

	{"MISO2_MUX", "UL1_CH1", "UL_SRC_MUX"},
	{"MISO2_MUX", "UL1_CH2", "UL_SRC_MUX"},
	{"MISO2_MUX", "UL2_CH1", "UL2_SRC_MUX"},
	{"MISO2_MUX", "UL2_CH2", "UL2_SRC_MUX"},

	{"UL_SRC_MUX", "AMIC", "AMIC0_MUX"},
	{"UL_SRC_MUX", "AMIC", "AMIC1_MUX"},
	{"UL_SRC_MUX", "DMIC", "DMIC0_MUX"},
	{"UL_SRC_MUX", "DMIC", "DMIC1_MUX"},

		{"UL_SRC_MUX", NULL, "UL_SRC"},

	{"UL2_SRC_MUX", "AMIC", "AMIC2_MUX"},
	{"UL2_SRC_MUX", "DMIC", "DMIC2_MUX"},

		{"UL2_SRC_MUX", NULL, "UL_SRC_34"},

	{"AMIC0_MUX", "ADC_L", "ADC_L"},
	{"AMIC0_MUX", "ADC_R", "ADC_R"},
	{"AMIC0_MUX", "ADC_T", "ADC_3"},
	{"AMIC1_MUX", "ADC_L", "ADC_L"},
	{"AMIC1_MUX", "ADC_R", "ADC_R"},
	{"AMIC1_MUX", "ADC_T", "ADC_3"},
	{"AMIC2_MUX", "ADC_L", "ADC_L"},
	{"AMIC2_MUX", "ADC_R", "ADC_R"},
	{"AMIC2_MUX", "ADC_T", "ADC_3"},

	{"DMIC0_MUX", "DMIC_DATA0", "AIN0_DMIC"},
	{"DMIC0_MUX", "DMIC_DATA1_L", "AIN2_DMIC"},
	{"DMIC0_MUX", "DMIC_DATA1_L_1", "AIN2_DMIC"},
	{"DMIC0_MUX", "DMIC_DATA1_R", "AIN3_DMIC"},
	{"DMIC1_MUX", "DMIC_DATA0", "AIN0_DMIC"},
	{"DMIC1_MUX", "DMIC_DATA1_L", "AIN2_DMIC"},
	{"DMIC1_MUX", "DMIC_DATA1_L_1", "AIN2_DMIC"},
	{"DMIC1_MUX", "DMIC_DATA1_R", "AIN3_DMIC"},
	{"DMIC2_MUX", "DMIC_DATA0", "AIN0_DMIC"},
	{"DMIC2_MUX", "DMIC_DATA1_L", "AIN2_DMIC"},
	{"DMIC2_MUX", "DMIC_DATA1_L_1", "AIN2_DMIC"},
	{"DMIC2_MUX", "DMIC_DATA1_R", "AIN3_DMIC"},

		{"DMIC0_MUX", NULL, "UL_SRC_DMIC"},
		{"DMIC1_MUX", NULL, "UL_SRC_DMIC"},
		{"DMIC2_MUX", NULL, "UL_SRC_34_DMIC"},

	{"AIN0_DMIC", NULL, "DMIC_0"},

	{"AIN2_DMIC", NULL, "DMIC_1"},
	{"AIN3_DMIC", NULL, "DMIC_1"},

	{"AIN2_DMIC", NULL, "MIC_BIAS_2"},
	{"AIN3_DMIC", NULL, "MIC_BIAS_2"},
	/* adc */
	{"ADC_L", NULL, "ADC_L_Mux"},
		{"ADC_L", NULL, "ADC_CLKGEN"},
		{"ADC_L", NULL, "ADC_L_EN"},
	{"ADC_R", NULL, "ADC_R_Mux"},
		{"ADC_R", NULL, "ADC_CLKGEN"},
		{"ADC_R", NULL, "ADC_R_EN"},
	{"ADC_3", NULL, "ADC_3_Mux"},
		{"ADC_3", NULL, "ADC_CLKGEN"},
		{"ADC_3", NULL, "ADC_3_EN"},

	{"ADC_L_Mux", "Left Preamplifier", "PGA_L"},

	{"ADC_R_Mux", "Right Preamplifier", "PGA_R"},

	{"ADC_3_Mux", "Preamplifier", "PGA_3"},

	{"PGA_L", NULL, "PGA_L_Mux"},
		{"PGA_L", NULL, "PGA_L_EN"},

	{"PGA_R", NULL, "PGA_R_Mux"},
		{"PGA_R", NULL, "PGA_R_EN"},

	{"PGA_3", NULL, "PGA_3_Mux"},
		{"PGA_3", NULL, "PGA_3_EN"},

		{"PGA_L", NULL, "DCC_CLK", mt_dcc_clk_connect},
		{"PGA_R", NULL, "DCC_CLK", mt_dcc_clk_connect},
		{"PGA_3", NULL, "DCC_CLK", mt_dcc_clk_connect},

	{"PGA_L_Mux", "AIN0", "AIN0"},
	{"PGA_L_Mux", "AIN1", "AIN1"},

	{"PGA_R_Mux", "AIN0", "AIN0"},
	{"PGA_R_Mux", "AIN2", "AIN2"},
	{"PGA_R_Mux", "AIN3", "AIN3"},

	{"PGA_3_Mux", "AIN2", "AIN2"},
	{"PGA_3_Mux", "AIN3", "AIN3"},

	{"AIN0", NULL, "MIC_BIAS_0"},
	{"AIN1", NULL, "MIC_BIAS_1"},
	{"AIN2", NULL, "MIC_BIAS_0"},
	{"AIN3", NULL, "MIC_BIAS_2"},

	/* DL Supply */
	{"DL Power Supply", NULL, "CLK_BUF"},
	{"DL Power Supply", NULL, "AUDGLB"},
	{"DL Power Supply", NULL, "CLKSQ Audio"},

	{"DL Power Supply", NULL, "AUDNCP_CK"},
	{"DL Power Supply", NULL, "ZCD13M_CK"},
	{"DL Power Supply", NULL, "AUD_CK"},
	{"DL Power Supply", NULL, "AUDIF_CK"},

	{"DL Power Supply", NULL, "ESD_RESIST"},
	{"DL Power Supply", NULL, "LDO"},
	{"DL Power Supply", NULL, "LDO_REMOTE"},
	{"DL Power Supply", NULL, "NV_REGULATOR"},
	{"DL Power Supply", NULL, "IBIST"},

	/* DL Digital Supply */
	{"DL Digital Clock", NULL, "AUDIO_TOP_AFE_CTL"},
	{"DL Digital Clock", NULL, "AUDIO_TOP_DAC_CTL"},
	{"DL Digital Clock", NULL, "AUDIO_TOP_PWR_CLK"},

	{"DL Digital Clock", NULL, "SDM_FIFO_CLK"},
	{"DL Digital Clock", NULL, "NCP"},

	{"DL Digital Clock", NULL, "AFE_ON"},
	{"DL Digital Clock", NULL, "AFE_DL_SRC"},

	{"DL Digital Clock CH_1_2", NULL, "DL Digital Clock"},
	{"DL Digital Clock CH_1_2", NULL, "SDM"},

	{"DL Digital Clock CH_3", NULL, "DL Digital Clock"},
	{"DL Digital Clock CH_3", NULL, "SDM_3RD"},

	{"AIF_RX", NULL, "DL Digital Clock CH_1_2"},
	{"AIF_RX", NULL, "DL_GPIO"},

	{"AIF2_RX", NULL, "DL Digital Clock CH_3"},
	{"AIF2_RX", NULL, "DL_GPIO"},

	/* DL Path */
	{"DAC In Mux", "Normal Path", "AIF_RX"},

	{"DAC In Mux", "Sgen", "SGEN DL"},
		{"SGEN DL", NULL, "SGEN DL SRC"},
		{"SGEN DL", NULL, "SGEN MUTE"},
		{"SGEN DL", NULL, "SGEN DL Enable"},
		{"SGEN DL", NULL, "DL Digital Clock CH_1_2"},
		{"SGEN DL", NULL, "DL Digital Clock CH_3"},
		{"SGEN DL", NULL, "AUDIO_TOP_PDN_AFE_TESTMODEL"},

	{"DACL", NULL, "DAC In Mux"},
		{"DACL", NULL, "DL Power Supply"},

	{"DACR", NULL, "DAC In Mux"},
		{"DACR", NULL, "DL Power Supply"},

	/* DAC 3RD */
	{"DAC In Mux", "Normal Path", "AIF2_RX"},
	{"DAC_3RD", NULL, "DAC In Mux"},
		{"DAC_3RD", NULL, "DL Power Supply"},

	/* Lineout Path */
	{"LOL Mux", "Playback", "DAC_3RD"},
	{"LINEOUT L", NULL, "LOL Mux"},

	/* Headphone Path */
	{"HP_Supply", NULL, "HP_PULL_DOWN"},
	{"HP_Supply", NULL, "HP_MUTE"},
	{"HPL Mux", NULL, "HP_Supply"},
	{"HPR Mux", NULL, "HP_Supply"},

	{"HPL Mux", "Audio Playback", "DACL"},
	{"HPR Mux", "Audio Playback", "DACR"},
	{"HPL Mux", "HP Impedance", "DACL"},
	{"HPR Mux", "HP Impedance", "DACR"},
	{"HPL Mux", "LoudSPK Playback", "DACL"},
	{"HPR Mux", "LoudSPK Playback", "DACR"},

	{"Headphone L", NULL, "HPL Mux"},
	{"Headphone R", NULL, "HPR Mux"},
	{"Headphone L Ext Spk Amp", NULL, "HPL Mux"},
	{"Headphone R Ext Spk Amp", NULL, "HPR Mux"},

	/* Receiver Path */
	{"RCV Mux", "Voice Playback", "DACL"},
	{"Receiver", NULL, "RCV Mux"},

	/* VOW */
#if 0
	{"VOW TX", NULL, "Mic_Type_Mux"},
		{"VOW TX", NULL, "CLK_BUF"},
		{"VOW TX", NULL, "AUDGLB"},
		{"VOW TX", NULL, "AUD_CK", mt_vow_amic_connect},
		{"VOW TX", NULL, "CLKSQ Audio", mt_vow_amic_dcc_connect},
		{"VOW TX", NULL, "VOW_AUD_LPW", mt_vow_amic_connect},
		{"VOW TX", NULL, "VOW_CLK"},
		{"VOW TX", NULL, "AUD_VOW"},
		{"VOW TX", NULL, "VOW_LDO", mt_vow_amic_connect},

	/* mic typ mux is deprecated */
	{"Mic_Type_Mux", "VOW_ACC", "ADC_L"},
	{"Mic_Type_Mux", "VOW_DCC", "ADC_L"},
	{"Mic_Type_Mux", "VOW_DCC_ECM_DIFF", "ADC_L"},
	{"Mic_Type_Mux", "VOW_DCC_ECM_SINGLE", "ADC_L"},
	{"Mic_Type_Mux", "VOW_DMIC", "AIN0"},
	{"Mic_Type_Mux", "VOW_DMIC_LP", "AIN0"},
#endif
};

static int mt6359_codec_dai_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_component *cmpnt = dai->component;
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int rate = params_rate(params);
	int id = dai->id;


	dev_info(priv->dev, "%s(), id %d, substream->stream %d, rate %d, number %d\n",
		 __func__,
		 id,
		 substream->stream,
		 rate,
		 substream->number);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		priv->dl_rate[id] = rate;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		priv->ul_rate[id] = rate;

	return 0;
}

static const struct snd_soc_dai_ops mt6359_codec_dai_ops = {
	.hw_params = mt6359_codec_dai_hw_params,
};

#define MT6359_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_U16_BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_U24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE |\
			SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_U32_BE)

static struct snd_soc_dai_driver mt6359_dai_driver[] = {
	{
		.id = MT6359_AIF_1,
		.name = "mt6359-snd-codec-aif1",
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = MT6359_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = MT6359_FORMATS,
		},
		.ops = &mt6359_codec_dai_ops,
	},
	{
		.id = MT6359_AIF_2,
		.name = "mt6359-snd-codec-aif2",
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000 |
				 SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = MT6359_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000,
			.formats = MT6359_FORMATS,
		},
		.ops = &mt6359_codec_dai_ops,
	},
	{
		.id = MT6359_AIF_VOW,
		.name = "mt6359-snd-codec-vow",
		.capture = {
			.stream_name = "VOW Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 |
				 SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_32000 |
				 SNDRV_PCM_RATE_48000,
			.formats = MT6359_FORMATS,
		},
	},
};

/* trim buffer */
enum {
	TRIM_BUF_MUX_OPEN = 0,
	TRIM_BUF_MUX_HPL,
	TRIM_BUF_MUX_HPR,
	TRIM_BUF_MUX_HSP,
	TRIM_BUF_MUX_HSN,
	TRIM_BUF_MUX_LOLP,
	TRIM_BUF_MUX_LOLN,
	TRIM_BUF_MUX_LORP,
	TRIM_BUF_MUX_LORN,
	TRIM_BUF_MUX_AVSS32,
	TRIM_BUF_MUX_GROUND,
};

enum {
	TRIM_BUF_GAIN_0DB = 0,
	TRIM_BUF_GAIN_6DB,
	TRIM_BUF_GAIN_12DB,
	TRIM_BUF_GAIN_18DB,
};

static void set_trim_buf_in_mux(struct mt6359_priv *priv, int mux)
{
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON8,
			    RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP32_MASK,
			    mux);
}

static void set_trim_buf_gain(struct mt6359_priv *priv, unsigned int gain)
{
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON8,
			   RG_AUDTRIMBUF_GAINSEL_VAUDP32_MASK_SFT,
			   gain << RG_AUDTRIMBUF_GAINSEL_VAUDP32_SFT);
}

static void enable_trim_buf(struct mt6359_priv *priv, bool enable)
{
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON8,
			   RG_AUDTRIMBUF_EN_VAUDP32_MASK_SFT,
			   (enable ? 1 : 0) << RG_AUDTRIMBUF_EN_VAUDP32_SFT);
}

/* 1 / (10 ^ (dB / 20)) * db_denominator */
static const int db_denominator = 8192;
static const int db_numerator[32] = {
	3261, 3659, 4106, 4607,
	5169, 5799, 6507, 7301,
	8192, 9192, 10313, 11572,
	12983, 14568, 16345, 18340,
	20577, 23088, 25905, 819200,
	819200, 819200, 819200, 819200,
	819200, 819200, 819200, 819200,
	819200, 819200, 819200, 819200,
};

static int get_mic_bias_mv(struct mt6359_priv *priv)
{
	unsigned int mic_bias;

	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON10, &mic_bias);
	mic_bias = (mic_bias >> 4) & 0x7;

	switch (mic_bias) {
	case MIC_BIAS_1P7:
		return 1700;
	case MIC_BIAS_1P8:
		return 1800;
	case MIC_BIAS_1P9:
		return 1900;
	case MIC_BIAS_2P0:
		return 2000;
	case MIC_BIAS_2P1:
		return 2100;
	case MIC_BIAS_2P5:
		return 2500;
	case MIC_BIAS_2P6:
		return 2600;
	case MIC_BIAS_2P7:
		return 2700;
	default:
		dev_warn(priv->dev, "%s(), invalid mic_bias %d\n",
			 __func__, mic_bias);
		return 2600;
	};
}

#define MIC_VINP_4POLE_THRES_MV 283
#define VINP_NORMALIZED_TO_MV 1700
static int convert_offset_to_comp(struct mt6359_priv *priv,
				  int offset, int vol_type)
{
	int gain = priv->ana_gain[vol_type];
	int mic_vinp_mv = priv->dc_trim.mic_vinp_mv;
	int real_mic_vinp_mv;
	int mic_bias_mv;
	int offset_scale = DIV_ROUND_CLOSEST(offset * db_numerator[gain],
					     db_denominator);

	if (mic_vinp_mv > MIC_VINP_4POLE_THRES_MV &&
	    ((priv->debug_flag & DBG_DCTRIM_BYPASS_4POLE) == 0)) {
		int v_diff_bias_vinp;
		int v_diff_bias_vinp_scale;

		/* refine mic bias influence on 4 pole headset */
		mic_bias_mv = get_mic_bias_mv(priv);
		real_mic_vinp_mv = DIV_ROUND_CLOSEST(mic_vinp_mv * mic_bias_mv,
						     VINP_NORMALIZED_TO_MV);

		v_diff_bias_vinp = mic_bias_mv - real_mic_vinp_mv;
		v_diff_bias_vinp_scale = DIV_ROUND_CLOSEST((v_diff_bias_vinp) *
							   db_numerator[gain],
							   db_denominator);

		if ((priv->debug_flag & DBG_DCTRIM_4POLE_LOG) != 0) {
			dev_info(priv->dev, "%s(), mic_bias_mv %d, mic_vinp_mv %d, real_mic_vinp_mv %d\n",
				 __func__,
				 mic_bias_mv, mic_vinp_mv, real_mic_vinp_mv);

			dev_info(priv->dev, "%s(), a %d, b %d\n", __func__,
				 DIV_ROUND_CLOSEST(offset_scale * 2804225,
						   32768),
				 DIV_ROUND_CLOSEST(v_diff_bias_vinp_scale *
						   1782,
						   1800));
		}

		return DIV_ROUND_CLOSEST(offset_scale * 2804225, 32768) -
		       DIV_ROUND_CLOSEST(v_diff_bias_vinp_scale * 1782, 1800);
	} else {
		/* The formula is from DE programming guide */
		/* should be mantain by pmic owner */
		return DIV_ROUND_CLOSEST(offset_scale * 2804225, 32768);
	}
}

static int get_dc_ramp_step(int gain)
{
	/* each step should be smaller than 100uV */
	/* 1 pcm of dc compensation = 0.0808uV HP buffer voltage @ 0dB*/
	/* 80uV / 0.0808uV(0dB) = 990.099 */
	int step_0db = 990;

	/* scale for specific gain */
	return step_0db * db_numerator[gain] / db_denominator;
}

static int apply_dc_compensation(struct mt6359_priv *priv, bool enable)
{
	struct mt6359_codec_ops *ops = &priv->ops;
	struct dc_trim_data *dc_trim = &priv->dc_trim;

	int lch_value = 0, rch_value = 0, tmp_ramp = 0;
	int times = 0, i = 0;
	int sign_lch = 0, sign_rch = 0;
	int abs_lch = 0, abs_rch = 0;
	int diff_lch = 0, diff_rch = 0, ramp_l = 0, ramp_r = 0;
	int index_lgain = priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL];
	int ramp_step = get_dc_ramp_step(index_lgain);
	int *pre_lch_comp = &dc_trim->pre_comp_value[CH_L];
	int *pre_rch_comp = &dc_trim->pre_comp_value[CH_R];

	if (ops->enable_dc_compensation == NULL ||
	    ops->set_lch_dc_compensation == NULL ||
	    ops->set_rch_dc_compensation == NULL) {
		dev_warn(priv->dev, "%s(), function not ready, enable %p, lch %p, rch %p\n",
			 __func__,
			 ops->enable_dc_compensation,
			 ops->set_lch_dc_compensation,
			 ops->set_rch_dc_compensation);
		return -EFAULT;
	}

	if (enable && index_lgain == DL_GAIN_N_40DB) {
		dev_info(priv->dev, "%s(), -40dB skip dc compensation\n",
			 __func__);
		return 0;
	}

	lch_value = convert_offset_to_comp(priv, dc_trim->hp_offset[CH_L],
					   AUDIO_ANALOG_VOLUME_HPOUTL);
	rch_value = convert_offset_to_comp(priv, dc_trim->hp_offset[CH_R],
					   AUDIO_ANALOG_VOLUME_HPOUTR);

	diff_lch = enable ? lch_value - *pre_lch_comp : lch_value;
	diff_rch = enable ? rch_value - *pre_rch_comp : rch_value;
	sign_lch = diff_lch < 0 ? -1 : 1;
	sign_rch = diff_rch < 0 ? -1 : 1;
	abs_lch = sign_lch * diff_lch;
	abs_rch = sign_rch * diff_rch;
	times = abs_lch > abs_rch ?
		(abs_lch / ramp_step) : (abs_rch / ramp_step);

	dev_info(priv->dev, "%s(), enable = %d, index_gain = %d, times = %d, lch_value = %d -> %d, rch_value = %d -> %d, ramp_step %d, mic_vinp_mv %d\n",
		 __func__, enable, index_lgain, times,
		 *pre_lch_comp, lch_value,
		 *pre_rch_comp, rch_value, ramp_step, dc_trim->mic_vinp_mv);

	if (enable) {
		ops->enable_dc_compensation(true);
		for (i = 1; i <= times; i++) {
			tmp_ramp = i * ramp_step;
			if (tmp_ramp < abs_lch) {
				ramp_l = *pre_lch_comp + sign_lch * tmp_ramp;
				ops->set_lch_dc_compensation(ramp_l << 8);
			}
			if (tmp_ramp < abs_rch) {
				ramp_r = *pre_rch_comp + sign_rch * tmp_ramp;
				ops->set_rch_dc_compensation(ramp_r << 8);
			}
			udelay(600);
		}
		ops->set_lch_dc_compensation(lch_value << 8);
		ops->set_rch_dc_compensation(rch_value << 8);
		*pre_lch_comp = lch_value;
		*pre_rch_comp = rch_value;
	} else {
		for (i = times; i >= 0; i--) {
			tmp_ramp = i * ramp_step;
			if (tmp_ramp < abs_lch) {
				ramp_l = sign_lch * tmp_ramp;
				ops->set_lch_dc_compensation(ramp_l << 8);
			}

			if (tmp_ramp < abs_rch) {
				ramp_r = sign_rch * tmp_ramp;
				ops->set_rch_dc_compensation(ramp_r << 8);
			}
			udelay(600);
		}
		ops->set_lch_dc_compensation(0);
		ops->set_rch_dc_compensation(0);
		ops->enable_dc_compensation(false);
		*pre_lch_comp = 0;
		*pre_rch_comp = 0;
	}

	return 0;
}

#ifndef CONFIG_FPGA_EARLY_PORTING
static void start_trim_hardware(struct mt6359_priv *priv, bool buffer_on)
{
#if 0
	/* enable clk buf */
	mt6359_set_dcxo(priv, true);

	/* Enable HP main CMFB Switch */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0200);
	/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x00a8);
	/* Dsiable HP main CMFB Switch */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0000);
	/* Release HP CMFB pull down */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x00a0);

	/* Enable AUDGLB */
	mt6359_set_aud_global_bias(priv, true);

	/* Enable CLKSQ 26MHz */
	mt6359_set_clksq(priv, true);

	/* Enable ZCD13M_CK, AUD_CK, AUDIF_CK, AUDNCP_CK*/
	mt6359_set_topck(priv, true);
	usleep_range(250, 270);

	regmap_update_bits(priv->regmap, MT6359_AUDIO_TOP_CON0, 0x00c4, 0x0);
	usleep_range(250, 270);

	/* sdm audio fifo clock power on */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0006);
	/* scrambler clock on enable */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xCBA1);
	/* sdm power on */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0003);
	/* sdm fifo enable */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x000B);

	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc800);
	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc801);

	/* afe enable */
	regmap_update_bits(priv->regmap, MT6359_AFE_UL_DL_CON0,
			   AFE_ON_MASK_SFT, 0x1);
	/* turn on dl */
	regmap_write(priv->regmap, MT6359_AFE_DL_SRC2_CON0_L, 0x1);
	/* set DL in normal path, not from sine gen table */
	regmap_write(priv->regmap, MT6359_AFE_TOP_CON0, 0x0);

	/* sdm output mute enable */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON1, 0x0000);
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xcbad);

	/* Disable headphone short-circuit protection */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x3000);
	/* Disable handset short-circuit protection */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0010);
	/* Disable linout short-circuit protection */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0010);
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
			   RG_HPPSHORT2VCM_VAUDP32_MASK_SFT,
			   0x1 << RG_HPPSHORT2VCM_VAUDP32_SFT);
	/* Set HPR/HPL gain as minimum (~ -40dB) */
	regmap_write(priv->regmap, MT6359_ZCD_CON2, DL_GAIN_N_40DB_REG);

	/* Enable cap-less LDOs (1.5V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0005);
	/* Enable NV regulator (-1.2V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0015);
	usleep_range(100, 120);

	/* Disable AUD_ZCD */
	zcd_disable(priv);

	/* Enable IBIST */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON12, 0x0055);
	/* Set HP DR bias current optimization, 010: 6uA */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON11, 0x4900);
	/* Set HP & ZCD bias current optimization */
	/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON12, 0x0055);
	/* Set HPP/N STB enhance circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0xc033);
	if (buffer_on) {
		/* No Pull-down HPL/R to AVSS28_AUD */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0x4033);
		/* Enable HP driver bias circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30c0);
		/* Enable HP driver core circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30f0);
		/* Enable HP main CMFB loop */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0200);
		/* Enable HP main output stage */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x0003);
		/* Enable HPR/L main output stage step by step */
		hp_main_output_ramp(priv, true);
	} else {
		/* Pull-down HPL/R to AVSS28_AUD */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0xc033);
		/* Enable HP driver bias circuits */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x30c0);
		/* Disable HP main output stage */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x0000);
		/* Enable HS driver bias circuits */
		/* Disable HS main output stage */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0012);
		/* Enable LO driver bias circuits */
		/* Disable LO main output stage */
		regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0012);
	}

	/* apply volume setting */
	headset_volume_ramp(priv, DL_GAIN_N_40DB, DL_GAIN_0DB);
	/* Enable AUD_CLK */
	mt6359_set_decoder_clk(priv, true);
	/* Enable low-noise mode of DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0203);
#endif
}

static void stop_trim_hardware(struct mt6359_priv *priv)
{
#if 0
	/* HPR/HPL mux to open */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x0f00, 0x0000);
	/* Disable Audio DAC */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x000f, 0x0000);
	/* Disable AUD_CLK */
	mt6359_set_decoder_clk(priv, false);

	/* decrease HPL/R gain to normal gain step by step */
	headset_volume_ramp(priv, DL_GAIN_0DB, DL_GAIN_N_40DB);
	/* decrease HPR/L main output stage step by step */
	hp_main_output_ramp(priv, false);

	/* disable HS driver bias circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0010);
	/* disable LO driver bias circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON7, 0x0010);

	/* Disable HP main output stage */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON1, 0x3, 0x0);
	/* Disable HP driver core circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 4, 0x0);
	/* Disable HP driver bias circuits */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   0x3 << 6, 0x0);
	/* Disable HP aux CMFB loop */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0xff << 8, 0x0);
	/* Enable HP main CMFB Switch */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON9,
			   0xff << 8, 0x2 << 8);

	/* Pull-down HPL/R, HS, LO to AVSS28_AUD */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0xff, 0xa8);

	/* Disable IBIST */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON12,
			   RG_AUDIBIASPWRDN_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDIBIASPWRDN_VAUDP32_SFT);
	/* Disable NV regulator (-1.2V) */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON14,
			   RG_NVREG_EN_VAUDP32_MASK_SFT, 0x0);
	/* Disable cap-less LDOs (1.5V) */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x5, 0x0);

	/* Disable NCP */
	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc800);

	/* Disable Pull-down HPL/R to AVSS28_AUD */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON2, 0x4033);

	/* Turn off down-link */
	regmap_write(priv->regmap, MT6359_AFE_DL_SRC2_CON0_L, 0x0);

	/* DL scrambler disabling sequence */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0);
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xcba0);

	/* turn off afe */
	regmap_update_bits(priv->regmap, MT6359_AFE_UL_DL_CON0,
			   AFE_ON_MASK_SFT, 0x0);
	/* all power down */
	regmap_update_bits(priv->regmap, MT6359_AUDIO_TOP_CON0,
			   0x00ff, 0x00ff);

	usleep_range(250, 270);

	/* disable ZCD13M_CK, AUD_CK, AUDIF_CK, AUDNCP_CK*/
	mt6359_set_topck(priv, false);
	/* disable CLKSQ 26MHz */
	mt6359_set_clksq(priv, false);

	/* disable AUDGLB */
	mt6359_set_aud_global_bias(priv, false);

	/* disable clk buf */
	mt6359_set_dcxo(priv, false);
#endif
}
#endif

static int hp_trim_offset(struct mt6359_priv *priv, int channel)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#define TRIM_TIMES 26
#define TRIM_DISCARD_NUM 3
#define TRIM_USEFUL_NUM (TRIM_TIMES - (TRIM_DISCARD_NUM * 2))

	int on_value[TRIM_TIMES];
	int off_value[TRIM_TIMES];
	int offset = 0;
	int i, j, tmp;

	if (channel != TRIM_BUF_MUX_HPL &&
	    channel != TRIM_BUF_MUX_HPR){
		dev_warn(priv->dev, "%s(), channel %d not support\n",
			 __func__, channel);
		return 0;
	}

	regmap_update_bits(priv->regmap, MT6359_AUXADC_CON10,
			   0x7, AUXADC_AVG_256);

	/* trimming buffer gain selection 18db*/
	set_trim_buf_gain(priv, TRIM_BUF_GAIN_18DB);
	/* trimming buffer mux selection */
	set_trim_buf_in_mux(priv, channel);

	/* get buffer on auxadc value  */
	start_trim_hardware(priv, true);

	enable_trim_buf(priv, true);
	usleep_range(1 * 1000, 10 * 1000);

	for (i = 0; i < TRIM_TIMES; i++)
		on_value[i] = get_auxadc_audio();

	enable_trim_buf(priv, false);

	stop_trim_hardware(priv);

	/* get buffer off auxadc value */
	start_trim_hardware(priv, false);

	enable_trim_buf(priv, true);

	usleep_range(1 * 1000, 10 * 1000);

	for (i = 0; i < TRIM_TIMES; i++)
		off_value[i] = get_auxadc_audio();

	enable_trim_buf(priv, false);

	stop_trim_hardware(priv);

	/* reset trimming buffer mux to ground */
	set_trim_buf_in_mux(priv, TRIM_BUF_MUX_GROUND);

	/* sort */
	for (i = 0; i < TRIM_TIMES - 1; i++) {
		for (j = 0; j < TRIM_TIMES - 1 - i; j++) {
			if (on_value[j] > on_value[j + 1]) {
				tmp = on_value[j + 1];
				on_value[j + 1] = on_value[j];
				on_value[j] = tmp;
			}
			if (off_value[j] > off_value[j + 1]) {
				tmp = off_value[j + 1];
				off_value[j + 1] = off_value[j];
				off_value[j] = tmp;
			}
		}
	}


	/* calculate result */
	for (i = TRIM_DISCARD_NUM; i < TRIM_TIMES - TRIM_DISCARD_NUM; i++) {
		offset += on_value[i] - off_value[i];
		dev_info(priv->dev, "%s(), offset diff %d, on %d, off %d\n",
			 __func__,
			 on_value[i] - off_value[i], on_value[i], off_value[i]);
	}

	offset = DIV_ROUND_CLOSEST(offset, TRIM_USEFUL_NUM);

	dev_info(priv->dev, "%s(), channel = %d, offset = %d\n",
		 __func__, channel, offset);

	return offset;
#else
	return 0;
#endif

}

static void get_hp_trim_offset(struct mt6359_priv *priv, bool force)
{
	struct dc_trim_data *dc_trim = &priv->dc_trim;

	if (dc_trim->calibrated && !force)
		return;

	dev_info(priv->dev, "%s(), Start DCtrim Calibrating", __func__);
	dc_trim->calibrated = true;
	dc_trim->hp_offset[CH_L] = hp_trim_offset(priv, TRIM_BUF_MUX_HPL);
	dc_trim->hp_offset[CH_R] = hp_trim_offset(priv, TRIM_BUF_MUX_HPR);
	dev_info(priv->dev, "%s(), End DCtrim Calibrating, L:%d, R:%d",
		 __func__,
		 dc_trim->hp_offset[CH_L], dc_trim->hp_offset[CH_R]);
}

static int dc_trim_thread(void *arg)
{
	struct mt6359_priv *priv = arg;

	get_hp_trim_offset(priv, false);
#ifdef CONFIG_MTK_ACCDET
	accdet_late_init(0);
#endif
	do_exit(0);
	return 0;
}

/* Headphone Impedance Detection */
static int mtk_calculate_impedance_formula(int pcm_offset, int aux_diff)
{
	/* The formula is from DE programming guide */
	/* should be mantain by pmic owner */
	/* R = V /I */
	/* V = auxDiff * (1800mv /auxResolution)  /TrimBufGain */
	/* I =  pcmOffset * DAC_constant * Gsdm * Gibuf */
	return DIV_ROUND_CLOSEST(3600000 / pcm_offset * aux_diff, 7832);
}

static int calculate_impedance(struct mt6359_priv *priv,
			       int dc_init, int dc_input,
			       short pcm_offset,
			       const unsigned int detect_times)
{
	int dc_value;
	int r_tmp = 0;

	if (dc_input < dc_init) {
		dev_warn(priv->dev, "%s(), Wrong[%d] : dc_input(%d) < dc_init(%d)\n",
			 __func__, pcm_offset, dc_input, dc_init);
		return 0;
	}

	dc_value = dc_input - dc_init;
	r_tmp = mtk_calculate_impedance_formula(pcm_offset, dc_value);
	r_tmp = DIV_ROUND_CLOSEST(r_tmp, detect_times);

	/* Efuse calibration */
	if ((priv->hp_current_calibrate_val != 0) && (r_tmp != 0)) {
		dev_info(priv->dev, "%s(), Before Calibration from EFUSE: %d, R: %d\n",
			 __func__, priv->hp_current_calibrate_val, r_tmp);
		r_tmp = DIV_ROUND_CLOSEST(
				r_tmp * 128 + priv->hp_current_calibrate_val,
				128);
	}

	dev_dbg(priv->dev, "%s(), pcm_offset %d dcoffset %d detected resistor is %d\n",
		__func__, pcm_offset, dc_value, r_tmp);

	return r_tmp;
}

#define PARALLEL_OHM 0
static int detect_impedance(struct mt6359_priv *priv)
{
	const unsigned int num_detect = 8;
	int i;
	int dc_sum = 0, detect_sum = 0;
	int pick_impedance = 0, impedance = 0, phase_flag = 0;
	int cur_dc = 0;
	unsigned int value;

	/* params by chip */
	int auxcable_impedance = 5000;
	/* should little lower than auxadc max resolution */
	int auxadc_upper_bound = 32630;
	/* Dc ramp up and ramp down step */
	int dc_step = 96;
	/* Phase 0 : high impedance with worst resolution */
	int dc_phase0 = 288;
	/* Phase 1 : median impedance with normal resolution */
	int dc_phase1 = 1440;
	/* Phase 2 : low impedance with better resolution */
	int dc_phase2 = 6048;
	/* Resistance Threshold of phase 2 and phase 1 */
	int resistance_1st_threshold = 250;
	/* Resistance Threshold of phase 1 and phase 0 */
	int resistance_2nd_threshold = 1000;

	if (priv->ops.enable_dc_compensation &&
	    priv->ops.set_lch_dc_compensation &&
	    priv->ops.set_rch_dc_compensation) {
		priv->ops.set_lch_dc_compensation(0);
		priv->ops.set_rch_dc_compensation(0);
		priv->ops.enable_dc_compensation(true);
	} else {
		dev_warn(priv->dev, "%s(), dc compensation ops not ready\n",
			 __func__);
		return 0;
	}

	regmap_update_bits(priv->regmap, MT6359_AUXADC_CON10,
			   0x7, AUXADC_AVG_64);

	set_trim_buf_in_mux(priv, TRIM_BUF_MUX_HPR);
	set_trim_buf_gain(priv, TRIM_BUF_GAIN_18DB);
	enable_trim_buf(priv, true);

	/* set hp gain 0dB */
	regmap_update_bits(priv->regmap, MT6359_ZCD_CON2,
			   RG_AUDHPRGAIN_MASK_SFT,
			   DL_GAIN_0DB << RG_AUDHPRGAIN_SFT);
	regmap_update_bits(priv->regmap, MT6359_ZCD_CON2,
			   RG_AUDHPLGAIN_MASK_SFT, DL_GAIN_0DB);

	for (cur_dc = 0; cur_dc <= dc_phase2; cur_dc += dc_step) {
		/* apply dc by dc compensation: 16bit MSB and negative value */
		priv->ops.set_lch_dc_compensation(-cur_dc << 16);
		priv->ops.set_rch_dc_compensation(-cur_dc << 16);

		/* save for DC = 0 offset */
		if (cur_dc == 0) {
			usleep_range(1 * 1000, 1 * 1000);
			dc_sum = 0;
			for (i = 0; i < num_detect; i++)
				dc_sum += get_auxadc_audio();

			if ((dc_sum / num_detect) > auxadc_upper_bound) {
				dev_info(priv->dev, "%s(), cur_dc == 0, auxadc value %d > auxadc_upper_bound %d\n",
					 __func__,
					 dc_sum / num_detect,
					 auxadc_upper_bound);
				impedance = auxcable_impedance;
				break;
			}
		}

		/* start checking */
		if (cur_dc == dc_phase0) {
			usleep_range(1 * 1000, 1 * 1000);
			detect_sum = 0;
			detect_sum = get_auxadc_audio();

			if ((dc_sum / num_detect) == detect_sum) {
				dev_info(priv->dev, "%s(), dc_sum / num_detect %d == detect_sum %d\n",
					 __func__,
					 dc_sum / num_detect, detect_sum);
				impedance = auxcable_impedance;
				break;
			}

			pick_impedance = calculate_impedance(
						priv,
						dc_sum / num_detect,
						detect_sum, cur_dc, 1);

			if (pick_impedance < resistance_1st_threshold) {
				phase_flag = 2;
				continue;
			} else if (pick_impedance < resistance_2nd_threshold) {
				phase_flag = 1;
				continue;
			}

			/* Phase 0 : detect range 1kohm to 5kohm impedance */
			for (i = 1; i < num_detect; i++)
				detect_sum += get_auxadc_audio();

			/* if auxadc > 32630 , the hpImpedance is over 5k ohm */
			if ((detect_sum / num_detect) > auxadc_upper_bound)
				impedance = auxcable_impedance;
			else
				impedance = calculate_impedance(priv,
								dc_sum,
								detect_sum,
								cur_dc,
								num_detect);
			break;
		}

		/* Phase 1 : detect range 250ohm to 1000ohm impedance */
		if (phase_flag == 1 && cur_dc == dc_phase1) {
			usleep_range(1 * 1000, 1 * 1000);
			detect_sum = 0;
			for (i = 0; i < num_detect; i++)
				detect_sum += get_auxadc_audio();

			impedance = calculate_impedance(priv,
							dc_sum, detect_sum,
							cur_dc, num_detect);
			break;
		}

		/* Phase 2 : detect under 250ohm impedance */
		if (phase_flag == 2 && cur_dc == dc_phase2) {
			usleep_range(1 * 1000, 1 * 1000);
			detect_sum = 0;
			for (i = 0; i < num_detect; i++)
				detect_sum += get_auxadc_audio();

			impedance = calculate_impedance(priv,
							dc_sum, detect_sum,
							cur_dc, num_detect);
			break;
		}
		usleep_range(1 * 200, 1 * 200);
	}

	if (PARALLEL_OHM != 0) {
		if (impedance < PARALLEL_OHM) {
			impedance = DIV_ROUND_CLOSEST(impedance * PARALLEL_OHM,
						      PARALLEL_OHM - impedance);
		} else {
			dev_warn(priv->dev, "%s(), PARALLEL_OHM %d <= impedance %d\n",
				 __func__, PARALLEL_OHM, impedance);
		}
	}

	regmap_read(priv->regmap, MT6359_AUXADC_CON10, &value);
	dev_info(priv->dev, "%s(), phase %d [dc,detect]Sum %d times [%d,%d], hp_impedance %d, pick_impedance %d, AUXADC_CON10 0x%x\n",
		 __func__, phase_flag, num_detect, dc_sum, detect_sum,
		 impedance, pick_impedance, value);

	/* Ramp-Down */
	while (cur_dc > 0) {
		cur_dc -= dc_step;
		/* apply dc by dc compensation: 16bit MSB and negative value */
		priv->ops.set_lch_dc_compensation(-cur_dc << 16);
		priv->ops.set_rch_dc_compensation(-cur_dc << 16);
		usleep_range(1 * 200, 1 * 200);
	}

	priv->ops.set_lch_dc_compensation(0);
	priv->ops.set_rch_dc_compensation(0);
	priv->ops.enable_dc_compensation(false);

	set_trim_buf_in_mux(priv, TRIM_BUF_MUX_GROUND);
	enable_trim_buf(priv, false);

	return impedance;
}

static int hp_impedance_get(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	if (priv->dev_counter[DEVICE_HP] <= 0 ||
	    priv->mux_select[MUX_HP_L] != HP_MUX_HP_IMPEDANCE) {
		dev_warn(priv->dev, "%s(), counter %d <= 0 || mux_select[MUX_HP_L] %d != HP_MUX_HP_IMPEDANCE\n",
			 __func__,
			 priv->dev_counter[DEVICE_HP],
			 priv->mux_select[MUX_HP_L]);
		ucontrol->value.integer.value[0] = priv->hp_impedance;
		return 0;
	}

	priv->hp_impedance = detect_impedance(priv);

	ucontrol->value.integer.value[0] = priv->hp_impedance;

	dev_info(priv->dev, "%s(), hp_impedance = %d, efuse = %d\n",
		 __func__, priv->hp_impedance, priv->hp_current_calibrate_val);

	return 0;
}

/* vow control */
static int *get_vow_coeff_by_name(struct mt6359_priv *priv,
				 const char *name)
{
	if (strcmp(name, "Audio VOWCFG0 Data") == 0)
		return &(priv->reg_afe_vow_cfg0);
	else if (strcmp(name, "Audio VOWCFG1 Data") == 0)
		return &(priv->reg_afe_vow_cfg1);
	else if (strcmp(name, "Audio VOWCFG2 Data") == 0)
		return &(priv->reg_afe_vow_cfg2);
	else if (strcmp(name, "Audio VOWCFG3 Data") == 0)
		return &(priv->reg_afe_vow_cfg3);
	else if (strcmp(name, "Audio VOWCFG4 Data") == 0)
		return &(priv->reg_afe_vow_cfg4);
	else if (strcmp(name, "Audio VOWCFG5 Data") == 0)
		return &(priv->reg_afe_vow_cfg5);
	else if (strcmp(name, "Audio_VOW_Periodic") == 0)
		return &(priv->reg_afe_vow_periodic);
	else
		return NULL;
}

static int audio_vow_cfg_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int *vow_cfg;

	vow_cfg = get_vow_coeff_by_name(priv, kcontrol->id.name);
	if (!vow_cfg) {
		dev_err(priv->dev, "%s(), vow_cfg == NULL\n", __func__);
		return -EINVAL;
	}
	dev_info(priv->dev, "%s(), %s = %d\n",
		 __func__, kcontrol->id.name, *vow_cfg);

	ucontrol->value.integer.value[0] = *vow_cfg;
	return 0;
}

static int audio_vow_cfg_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	int index = ucontrol->value.integer.value[0];
	int *vow_cfg;

	vow_cfg = get_vow_coeff_by_name(priv, kcontrol->id.name);
	if (!vow_cfg) {
		dev_err(priv->dev, "%s(), vow_cfg == NULL\n", __func__);
		return -EINVAL;
	}
	dev_info(priv->dev, "%s(), %s = %d\n",
		 __func__, kcontrol->id.name, index);

	*vow_cfg = index;
	return 0;
}

/* misc control */
static const char *const off_on_function[] = {"Off", "On"};

static int hp_plugged_in_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	ucontrol->value.integer.value[0] = priv->hp_plugged;
	return 0;
}

static int hp_plugged_in_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(off_on_function)) {
		dev_warn(priv->dev, "%s(), return -EINVAL\n", __func__);
		return -EINVAL;
	}

	if (ucontrol->value.integer.value[0] == 1) {
		priv->dc_trim.mic_vinp_mv = get_accdet_auxadc();
		dev_info(priv->dev, "%s(), mic_vinp_mv = %d\n",
			 __func__, priv->dc_trim.mic_vinp_mv);
	}

	priv->hp_plugged = ucontrol->value.integer.value[0];

	return 0;
}

static int mt6359_codec_debug_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int mt6359_codec_debug_set(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);
	unsigned int value;

	regmap_read(priv->regmap, MT6359_AUD_TOP_ID, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_REV0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_DBI, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_DXI, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_TPM0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKPDN_TPM0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_TPM1, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKPDN_TPM1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKPDN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKPDN_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKPDN_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKSEL_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKSEL_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKSEL_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKTST_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CKTST_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CLK_HWEN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CLK_HWEN_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_CLK_HWEN_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_RST_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_RST_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_RST_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_BANK_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_RST_BANK_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_MASK_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0_SET, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_MASK_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0_CLR, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_MASK_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_STATUS0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_STATUS0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_RAW_STATUS0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_RAW_STATUS0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MISC_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_INT_MISC_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_MON_CON0, &value);
	dev_info(priv->dev, "MT6359_AUD_TOP_MON_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_ID, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_REV0, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_DBI, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_DXI, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_DSN_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_DL_CON0, &value);
	dev_info(priv->dev, "MT6359_AFE_UL_DL_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_SRC2_CON0_L, &value);
	dev_info(priv->dev, "MT6359_AFE_DL_SRC2_CON0_L = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_SRC_CON0_H, &value);
	dev_info(priv->dev, "MT6359_AFE_UL_SRC_CON0_H = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_SRC_CON0_L, &value);
	dev_info(priv->dev, "MT6359_AFE_UL_SRC_CON0_L = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_TOP_CON0, &value);
	dev_info(priv->dev, "MT6359_AFE_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_TOP_CON0, &value);
	dev_info(priv->dev, "MT6359_AUDIO_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_MON_DEBUG0, &value);
	dev_info(priv->dev, "MT6359_AFE_MON_DEBUG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON0, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON1, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON2, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON3, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON4, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON5, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON6, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_MON0, &value);
	dev_info(priv->dev, "MT6359_AFUNC_AUD_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDRC_TUNE_MON0, &value);
	dev_info(priv->dev, "MT6359_AUDRC_TUNE_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_FIFO_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_FIFO_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_FIFO_LOG_MON1, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_FIFO_LOG_MON1 = 0x%x\n",
		 value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON0, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON1, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON2, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_RX_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_RX_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG2, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_RX_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG3, &value);
	dev_info(priv->dev, "MT6359_AFE_ADDA_MTKAIF_RX_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_SGEN_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_SGEN_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_SGEN_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_SGEN_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADC_ASYNC_FIFO_CFG, &value);
	dev_info(priv->dev, "MT6359_AFE_ADC_ASYNC_FIFO_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DCCLK_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_DCCLK_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DCCLK_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_DCCLK_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_CFG, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP, &value);
	dev_info(priv->dev, "MT6359_AFE_AUD_PAD_TOP = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP_MON, &value);
	dev_info(priv->dev, "MT6359_AFE_AUD_PAD_TOP_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP_MON1, &value);
	dev_info(priv->dev, "MT6359_AFE_AUD_PAD_TOP_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_NLE_CFG, &value);
	dev_info(priv->dev, "MT6359_AFE_DL_NLE_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_NLE_MON, &value);
	dev_info(priv->dev, "MT6359_AFE_DL_NLE_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_CG_EN_MON, &value);
	dev_info(priv->dev, "MT6359_AFE_CG_EN_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_ID, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_2ND_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_REV0, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_2ND_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_DBI, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_2ND_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_DXI, &value);
	dev_info(priv->dev, "MT6359_AUDIO_DIG_2ND_DSN_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_PMIC_NEWIF_CFG3, &value);
	dev_info(priv->dev, "MT6359_AFE_PMIC_NEWIF_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON2, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON3, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON4, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_MON0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TOP_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG2, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG3, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG4, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG5, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG6, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG7, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG8, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG9, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG10, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG11, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG12, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_CFG12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON2, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON3, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON4, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON5, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON6, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON7, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON8, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON9, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON10, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON11, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_VAD_MON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TGEN_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TGEN_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TGEN_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_TGEN_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_HPF_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_HPF_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_HPF_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_HPF_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG2, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG3, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG4, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG5, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG6, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG7, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG8, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG9, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG10, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG11, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG12, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG13, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG13 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG14, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG14 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG15, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG15 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG16, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG16 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG17, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG17 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG18, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG18 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG19, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG19 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG20, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG20 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG21, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG21 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG22, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG22 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG23, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG23 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG24, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG24 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG25, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG25 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG26, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG26 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG27, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG27 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG28, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG28 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG29, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG29 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG30, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG30 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG31, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG31 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG32, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG32 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG33, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG33 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG34, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG34 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG35, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG35 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG36, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG36 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG37, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG37 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG38, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG38 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG39, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_CFG39 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON0, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON1, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON2, &value);
	dev_info(priv->dev, "MT6359_AFE_VOW_PERIODIC_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG0, &value);
	dev_info(priv->dev, "MT6359_AFE_NCP_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG1, &value);
	dev_info(priv->dev, "MT6359_AFE_NCP_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG2, &value);
	dev_info(priv->dev, "MT6359_AFE_NCP_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_ID, &value);
	dev_info(priv->dev, "MT6359_AUDENC_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_REV0, &value);
	dev_info(priv->dev, "MT6359_AUDENC_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_DBI, &value);
	dev_info(priv->dev, "MT6359_AUDENC_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_FPI, &value);
	dev_info(priv->dev, "MT6359_AUDENC_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON0, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON1, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON2, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON3, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON4, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON5, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON6, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON7, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON8, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON9, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON10, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON11, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON12, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON13, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON13 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON14, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON14 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON15, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON15 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON16, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON16 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON17, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON17 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON18, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON18 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON19, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON19 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON20, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON20 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON21, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON21 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON22, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON22 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON23, &value);
	dev_info(priv->dev, "MT6359_AUDENC_ANA_CON23 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_ID, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_REV0, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_DBI, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_FPI, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON0, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON1, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON2, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON3, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON4, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON5, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON6, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON7, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON8, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON9, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON10, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON11, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON12, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON13, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON13 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON14, &value);
	dev_info(priv->dev, "MT6359_AUDDEC_ANA_CON14 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_ID, &value);
	dev_info(priv->dev, "MT6359_AUDZCD_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_REV0, &value);
	dev_info(priv->dev, "MT6359_AUDZCD_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_DBI, &value);
	dev_info(priv->dev, "MT6359_AUDZCD_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_FPI, &value);
	dev_info(priv->dev, "MT6359_AUDZCD_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON0, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON1, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON2, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON3, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON4, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON5, &value);
	dev_info(priv->dev, "MT6359_ZCD_CON5 = 0x%x\n", value);

	regmap_read(priv->regmap, MT6359_DRV_CON3, &value);
	dev_info(priv->dev, "MT6359_DRV_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_DIR0, &value);
	dev_info(priv->dev, "MT6359_GPIO_DIR0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_DIR1, &value);
	dev_info(priv->dev, "MT6359_GPIO_DIR1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE2, &value);
	dev_info(priv->dev, "MT6359_GPIO_MODE2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE3, &value);
	dev_info(priv->dev, "MT6359_GPIO_MODE3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE4, &value);
	dev_info(priv->dev, "MT6359_GPIO_MODE4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_TOP_CKPDN_CON0, &value);
	dev_info(priv->dev, "MT6359_TOP_CKPDN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_TOP_CKHWEN_CON0, &value);
	dev_info(priv->dev, "MT6359_TOP_CKHWEN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_DCXO_CW11, &value);
	dev_info(priv->dev, "MT6359_DCXO_CW11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_DCXO_CW12, &value);
	dev_info(priv->dev, "MT6359_DCXO_CW12 = 0x%x\n", value);

	return 0;
}

static const struct soc_enum misc_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(off_on_function), off_on_function),
};

static int mt6359_rcv_dcc_set(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	/* receiver downlink */
	playback_gpio_set(priv);
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON13,
			   RG_AUDGLB_PWRDN_VA32_MASK_SFT, 0x0);
	regmap_update_bits(priv->regmap, MT6359_DCXO_CW12,
			   0x1 << RG_XO_AUDIO_EN_M_SFT,
			   0x1 << RG_XO_AUDIO_EN_M_SFT);
	/* audio clk source from internal dcxo */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
			   RG_CLKSQ_IN_SEL_TEST_MASK_SFT,
			   0x0);

	/* Enable/disable CLKSQ 26MHz */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
			   RG_CLKSQ_EN_MASK_SFT,
			   1 << RG_CLKSQ_EN_SFT);

	regmap_update_bits(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0,
			   0x66, 0x0);
	usleep_range(250, 270);
	/* Audio system digital clock power down release */
	regmap_update_bits(priv->regmap, MT6359_AUDIO_TOP_CON0,
			   0x00ff, 0x0000);
	usleep_range(250, 270);

	/* sdm audio fifo clock power on */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0006);
	/* scrambler clock on enable */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON0, 0xCBA1);
	/* sdm power on */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x0003);
	/* sdm fifo enable */
	regmap_write(priv->regmap, MT6359_AFUNC_AUD_CON2, 0x000B);

	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc800);
	regmap_write(priv->regmap, MT6359_AFE_NCP_CFG0, 0xc801);

	/* afe enable, dl_lr_swap = 0 */
	regmap_update_bits(priv->regmap, MT6359_AFE_UL_DL_CON0,
			   0xc001, 0x0001);

	/* turn on dl */
	regmap_write(priv->regmap, MT6359_AFE_DL_SRC2_CON0_L, 0x0001);

	/* set DL in normal path, not from sine gen table */
	regmap_write(priv->regmap, MT6359_AFE_TOP_CON0, 0x0000);

	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON2,
			   RG_AUDREFN_DERES_EN_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDREFN_DERES_EN_VAUDP32_SFT);
	usleep_range(250, 270);

	/* Enable cap-less LDOs (1.5V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0005);
	/* Enable NV regulator (-1.2V) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON14, 0x0015);
	usleep_range(100, 120);

	/* Disable AUD_ZCD */
	zcd_disable(priv);

	/* Disable handset short-circuit protection */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0010);
	/* Enable IBIST */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON12, 0x0055);
	/* Set HP DR bias current optimization, 010: 6uA */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON11, 0x4900);
	/* Set HP & ZCD bias current optimization */
	/* 01: ZCD: 4uA, HP/HS/LO: 5uA */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON12, 0x0055);
	/* Set HS STB enhance circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0090);

	/* Set HS output stage (3'b111 = 8x) */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON10, 0x7000);

	/* Enable HS driver bias circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0092);
	/* Enable HS driver core circuits */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x0093);

	/* Set HS gain to normal gain step by step */
	regmap_write(priv->regmap, MT6359_ZCD_CON3, 0x0);

	/* Enable AUD_CLK */
	mt6359_set_decoder_clk(priv, true);
	/* Enable Audio DAC  */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON0, 0x0009);
	/* Enable low-noise mode of DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON9, 0x0001);
	/* Switch HS MUX to audio DAC */
	regmap_write(priv->regmap, MT6359_AUDDEC_ANA_CON6, 0x009b);

	/* phone mic dcc */

	/* Enable audio ADC CLKGEN  */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON5, 0x0001);
	/* ADC CLK from CLKGEN (13MHz) */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON5, 0x0021);

	/* DCC 50k CLK (from 26M) */
	regmap_write(priv->regmap, MT6359_AFE_DCCLK_CFG0, 0x2062);
	regmap_write(priv->regmap, MT6359_AFE_DCCLK_CFG0, 0x2060);
	regmap_write(priv->regmap, MT6359_AFE_DCCLK_CFG0, 0x2061);
	regmap_write(priv->regmap, MT6359_AFE_DCCLK_CFG1, 0x0100);

	/* phone mic */
	/* Enable MICBIAS0, MISBIAS0 = 1P9V */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON15, 0x0021);

	/* dcc precharge */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x0004);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x0004);

	/* preamplifier input sel, enable pga */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x0045);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x0045);

	/* pga gain 18 dB */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x0345);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x0345);

	/* preamplifier dcc en */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x0347);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x0347);

	/* adc in sel, enable adc */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x5347);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x5347);

	usleep_range(100, 120);

	/* preamplifier dcc precharge off */
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON1, 0x5343);
	regmap_write(priv->regmap, MT6359_AUDENC_ANA_CON0, 0x5343);

	/* here to set digital part */

	/* set gpio miso mode */
	capture_gpio_set(priv);

	/* power on clock */
	regmap_update_bits(priv->regmap, MT6359_AUDIO_TOP_CON0,
			   0x00ff, 0x0000);

	/* configure ADC setting */
	regmap_write(priv->regmap, MT6359_AFE_TOP_CON0, 0x0000);

	/* [0] afe enable */
	regmap_update_bits(priv->regmap, MT6359_AFE_UL_DL_CON0,
			   0x0001, 0x0001);

	mt6359_mtkaif_tx_enable(priv);

	/* UL dmic setting */
	regmap_write(priv->regmap, MT6359_AFE_UL_SRC_CON0_H, 0x0000);

	/* UL turn on */
	regmap_write(priv->regmap, MT6359_AFE_UL_SRC_CON0_L, 0x0001);

	return 0;
}

static int mt6359_rcv_dcc_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

/* vow control */
static const struct snd_kcontrol_new mt6359_snd_vow_controls[] = {
	SOC_SINGLE_EXT("Audio VOWCFG0 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio VOWCFG1 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio VOWCFG2 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio VOWCFG3 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio VOWCFG4 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio VOWCFG5 Data",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
	SOC_SINGLE_EXT("Audio_VOW_Periodic",
		       SND_SOC_NOPM, 0, 0x80000, 0,
		       audio_vow_cfg_get, audio_vow_cfg_set),
};

static const struct snd_kcontrol_new mt6359_snd_misc_controls[] = {
	SOC_ENUM_EXT("Headphone Plugged In", misc_control_enum[0],
		     hp_plugged_in_get, hp_plugged_in_set),
	SOC_SINGLE_EXT("Audio HP ImpeDance Setting",
		       SND_SOC_NOPM, 0, 0x10000, 0,
		       hp_impedance_get, NULL),
	SOC_ENUM_EXT("Audio_Codec_Debug_Setting", misc_control_enum[0],
		     mt6359_codec_debug_get, mt6359_codec_debug_set),
	SOC_ENUM_EXT("PMIC_REG_CLEAR", misc_control_enum[0],
		     mt6359_rcv_dcc_get, mt6359_rcv_dcc_set),
};

static int mt6359_codec_init_reg(struct mt6359_priv *priv)
{
	int ret = 0;

	/* enable clk buf */
	regmap_update_bits(priv->regmap, MT6359_DCXO_CW12,
			   0x1 << RG_XO_AUDIO_EN_M_SFT,
			   0x1 << RG_XO_AUDIO_EN_M_SFT);

	/* set those not controlled by dapm widget */

	/* audio clk source from internal dcxo */
	regmap_update_bits(priv->regmap, MT6359_AUDENC_ANA_CON23,
			   RG_CLKSQ_IN_SEL_TEST_MASK_SFT,
			   0x0);

	/* Disable HeadphoneL/HeadphoneR short circuit protection */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   RG_AUDHPLSCDISABLE_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDHPLSCDISABLE_VAUDP32_SFT);
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON0,
			   RG_AUDHPRSCDISABLE_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDHPRSCDISABLE_VAUDP32_SFT);
	/* Disable voice short circuit protection */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON6,
			   RG_AUDHSSCDISABLE_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDHSSCDISABLE_VAUDP32_SFT);
	/* disable LO buffer left short circuit protection */
	regmap_update_bits(priv->regmap, MT6359_AUDDEC_ANA_CON7,
			   RG_AUDLOLSCDISABLE_VAUDP32_MASK_SFT,
			   0x1 << RG_AUDLOLSCDISABLE_VAUDP32_SFT);

	/* set gpio */
	playback_gpio_reset(priv);
	capture_gpio_reset(priv);

	/* hp gain ctl default choose ZCD */
	priv->hp_gain_ctl = HP_GAIN_CTL_ZCD;
	hp_gain_ctl_select(priv, priv->hp_gain_ctl);

	/* hp hifi mode, default normal mode */
	priv->hp_hifi_mode = 0;

	/* Disable AUD_ZCD */
	zcd_disable(priv);

	/* disable clk buf */
	regmap_update_bits(priv->regmap, MT6359_DCXO_CW12,
			   0x1 << RG_XO_AUDIO_EN_M_SFT,
			   0x0);

	/* this will trigger dctrim widgat power down event */
	enable_trim_buf(priv, true);
	return ret;
}

static int get_hp_current_calibrate_val(struct mt6359_priv *priv)
{
#if 0
	int ret = 0;
	int value, sign;

	/* 1. enable efuse ctrl engine clock */
	regmap_update_bits(priv->regmap, MT6359_TOP_CKHWEN_CON0_CLR,
			   0x1 << 2, 0x1 << 2);
	regmap_update_bits(priv->regmap, MT6359_TOP_CKPDN_CON0_CLR,
			   0x1 << 4, 0x1 << 4);

	/* 2. set RG_OTP_RD_SW */
	regmap_update_bits(priv->regmap, MT6359_OTP_CON11, 0x0001, 0x0001);

	/* 3. set EFUSE addr */
	/* HPDET_COMP[6:0] @ efuse bit 1696 ~ 1702 */
	/* HPDET_COMP_SIGN @ efuse bit 1703 */
	/* 1696 / 8 = 212 --> 0xd4 */
	regmap_update_bits(priv->regmap, MT6359_OTP_CON0, 0xff, 0xd4);

	/* 4. Toggle RG_OTP_RD_TRIG */
	regmap_read(priv->regmap, MT6359_OTP_CON8, &ret);
	if (ret == 0)
		regmap_update_bits(priv->regmap, MT6359_OTP_CON8,
				   0x0001, 0x0001);
	else
		regmap_update_bits(priv->regmap, MT6359_OTP_CON8,
				   0x0001, 0x0000);

	/* 5. Polling RG_OTP_RD_BUSY */
	do {
		regmap_read(priv->regmap, MT6359_OTP_CON13, &ret);
		ret = ret & 0x0001;
		usleep_range(100, 200);
		dev_dbg(priv->dev, "%s(), polling MT6359_OTP_CON13 = 0x%x\n",
			__func__, ret);
	} while (ret == 1);

	/* Need to delay at least 1ms for 0xC1A and than can read */
	usleep_range(500, 1000);

	/* 6. Read RG_OTP_DOUT_SW */
	regmap_read(priv->regmap, MT6359_OTP_CON12, &ret);
	dev_dbg(priv->dev, "%s(), efuse = 0x%x\n",
		__func__, ret);

	sign = (ret >> 7) & 0x1;
	value = ret & 0x7f;
	value = sign ? -value : value;

	/* 7. Disables efuse_ctrl egine clock */
	regmap_update_bits(priv->regmap, MT6359_OTP_CON11, 0x0001, 0x0000);
	regmap_update_bits(priv->regmap, MT6359_TOP_CKPDN_CON0_SET,
			   0x1 << 4, 0x1 << 4);
	regmap_update_bits(priv->regmap, MT6359_TOP_CKHWEN_CON0_SET,
			   0x1 << 2, 0x1 << 2);

	dev_dbg(priv->dev, "%s(), efuse: %d\n", __func__, value);
	return value;
#else
	return 0;
#endif
}

static int mt6359_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_component *cmpnt = &codec->component;
	struct mt6359_priv *priv = snd_soc_component_get_drvdata(cmpnt);

	snd_soc_component_init_regmap(cmpnt, priv->regmap);

	/* add codec controls */
	snd_soc_add_component_controls(cmpnt,
				       mt6359_snd_controls,
				       ARRAY_SIZE(mt6359_snd_controls));
	snd_soc_add_component_controls(cmpnt,
				       mt6359_snd_ul_controls,
				       ARRAY_SIZE(mt6359_snd_ul_controls));
	snd_soc_add_component_controls(cmpnt,
				       mt6359_snd_misc_controls,
				       ARRAY_SIZE(mt6359_snd_misc_controls));
	snd_soc_add_component_controls(cmpnt,
				       mt6359_snd_vow_controls,
				       ARRAY_SIZE(mt6359_snd_vow_controls));

	mt6359_codec_init_reg(priv);

	priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTL] = 8;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_HPOUTR] = 8;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP1] = 3;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP2] = 3;
	priv->ana_gain[AUDIO_ANALOG_VOLUME_MICAMP3] = 3;

	priv->hp_current_calibrate_val = get_hp_current_calibrate_val(priv);

	return 0;
}

static struct snd_soc_codec_driver mt6359_soc_codec_driver = {
	.probe = mt6359_codec_probe,
	.component_driver = {
		.dapm_widgets = mt6359_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(mt6359_dapm_widgets),
		.dapm_routes = mt6359_dapm_routes,
		.num_dapm_routes = ARRAY_SIZE(mt6359_dapm_routes),
	},
};

static void debug_write_reg(struct file *file, void *arg)
{
	struct mt6359_priv *priv = file->private_data;
	char *token1 = NULL;
	char *token2 = NULL;
	char *temp = arg;
	char delim[] = " ,";
	unsigned int reg_addr = 0;
	unsigned int reg_value = 0;
	int ret = 0;

	token1 = strsep(&temp, delim);
	token2 = strsep(&temp, delim);
	dev_info(priv->dev, "%s(), token1 = %s, token2 = %s, temp = %s\n",
		 __func__, token1, token2, temp);

	if ((token1 != NULL) && (token2 != NULL)) {
		ret = kstrtouint(token1, 16, &reg_addr);
		ret = kstrtouint(token2, 16, &reg_value);
		dev_info(priv->dev, "%s(), reg_addr = 0x%x, reg_value = 0x%x\n",
			 __func__,
			 reg_addr, reg_value);
		regmap_write(priv->regmap, reg_addr, reg_value);
		regmap_read(priv->regmap, reg_addr, &reg_value);
		dev_info(priv->dev, "%s(), reg_addr = 0x%x, reg_value = 0x%x\n",
			 __func__,
			 reg_addr, reg_value);
	} else {
		dev_err(priv->dev, "token1 or token2 is NULL!\n");
	}
}

static void debug_re_trim_offset(struct file *file,
				 void *arg __attribute__((unused)))
{
	struct mt6359_priv *priv = file->private_data;

	dev_info(priv->dev, "%s(), start\n", __func__);
	get_hp_trim_offset(priv, true);
	dev_info(priv->dev, "%s(), end\n", __func__);
}

static void debug_set_debug_flag(struct file *file, void *arg)
{
	struct mt6359_priv *priv = file->private_data;
	char *token1 = NULL;
	char *temp = arg;
	char delim[] = " ,";
	int ret = 0;
	unsigned int value;

	token1 = strsep(&temp, delim);
	dev_info(priv->dev, "%s(), token1 = %s, temp = %s\n",
		 __func__, token1, temp);

	if (token1 != NULL) {
		ret = kstrtouint(token1, 16, &value);
		priv->debug_flag = value;
	} else {
		dev_warn(priv->dev, "%s(), token1 is NULL!\n", __func__);
	}
}

struct command_function {
	const char *cmd;
	void (*fn)(struct file *file, void *arg);
};

#define CMD_FN(_cmd, _fn) {	\
	.cmd = _cmd,		\
	.fn = _fn,		\
}

static const struct command_function debug_cmds[] = {
	CMD_FN("write_reg", debug_write_reg),
	CMD_FN("re_trim_offset", debug_re_trim_offset),
	CMD_FN("set_debug_flag", debug_set_debug_flag),
	{}
};

static int mt6359_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t mt6359_debugfs_read(struct file *file, char __user *buf,
				     size_t count, loff_t *pos)
{
	struct mt6359_priv *priv = file->private_data;
	const int size = 12288;
	char *buffer = NULL; /* for reduce kernel stack */
	int n = 0;
	unsigned int value;
	int ret = 0;

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	n += scnprintf(buffer + n, size - n, "mtkaif_protocol = %d\n",
		       priv->mtkaif_protocol);
	n += scnprintf(buffer + n, size - n, "dc_trim_data:\n");
	n += scnprintf(buffer + n, size - n, "\tcalibrated = %d\n",
		       priv->dc_trim.calibrated);
	n += scnprintf(buffer + n, size - n, "\toffset_L = %d, offset_R = %d\n",
		       priv->dc_trim.hp_offset[CH_L],
		       priv->dc_trim.hp_offset[CH_R]);
	n += scnprintf(buffer + n, size - n, "\tmic_vinp_mv = %d\n",
		       priv->dc_trim.mic_vinp_mv);

	n += scnprintf(buffer + n, size - n, "codec_ops:\n");
	n += scnprintf(buffer + n, size - n, "\tenable_dc_compensation = %p\n",
		       priv->ops.enable_dc_compensation);
	n += scnprintf(buffer + n, size - n, "\tset_lch_dc_compensation = %p\n",
		       priv->ops.set_lch_dc_compensation);
	n += scnprintf(buffer + n, size - n, "\tset_rch_dc_compensation = %p\n",
		       priv->ops.set_rch_dc_compensation);

	n += scnprintf(buffer + n, size - n, "debug_flag = 0x%x\n",
		       priv->debug_flag);

	n += scnprintf(buffer + n, size - n, "hp_impedance = %d\n",
		       priv->hp_impedance);
	n += scnprintf(buffer + n, size - n, "hp_current_calibrate_val = %d\n",
		       priv->hp_current_calibrate_val);

	regmap_read(priv->regmap, MT6359_DRV_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_DRV_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_DIR0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_GPIO_DIR0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_DIR1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_GPIO_DIR1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_GPIO_MODE2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_GPIO_MODE3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_GPIO_MODE4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_GPIO_MODE4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_TOP_CKPDN_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_TOP_CKPDN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_TOP_CKHWEN_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_TOP_CKHWEN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_DCXO_CW11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_DCXO_CW11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_DCXO_CW12, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_DCXO_CW12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUXADC_CON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUXADC_CON10 = 0x%x\n", value);

	regmap_read(priv->regmap, MT6359_AUD_TOP_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_DXI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_TPM0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKPDN_TPM0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_TPM1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKPDN_TPM1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKPDN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKPDN_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKPDN_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKPDN_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKSEL_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKSEL_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKSEL_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKSEL_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CKTST_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CKTST_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CLK_HWEN_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CLK_HWEN_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_CLK_HWEN_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_CLK_HWEN_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_RST_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_RST_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_RST_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_RST_BANK_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_RST_BANK_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_MASK_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0_SET, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_MASK_CON0_SET = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MASK_CON0_CLR, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_MASK_CON0_CLR = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_STATUS0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_STATUS0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_RAW_STATUS0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_RAW_STATUS0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_INT_MISC_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_INT_MISC_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUD_TOP_MON_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUD_TOP_MON_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_DSN_DXI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_DSN_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_DL_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_UL_DL_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_SRC2_CON0_L, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_DL_SRC2_CON0_L = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_SRC_CON0_H, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_UL_SRC_CON0_H = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_UL_SRC_CON0_L, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_UL_SRC_CON0_L = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_MON_DEBUG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_MON_DEBUG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFUNC_AUD_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFUNC_AUD_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDRC_TUNE_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDRC_TUNE_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_FIFO_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_FIFO_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_FIFO_LOG_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_FIFO_LOG_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_MON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_RX_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_RX_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_RX_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADDA_MTKAIF_RX_CFG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADDA_MTKAIF_RX_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_SGEN_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_SGEN_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_SGEN_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_SGEN_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_ADC_ASYNC_FIFO_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_ADC_ASYNC_FIFO_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DCCLK_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_DCCLK_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DCCLK_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_DCCLK_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_AUD_PAD_TOP = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_AUD_PAD_TOP_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_AUD_PAD_TOP_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_AUD_PAD_TOP_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_NLE_CFG, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_DL_NLE_CFG = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_DL_NLE_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_DL_NLE_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_CG_EN_MON, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_CG_EN_MON = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_2ND_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_2ND_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_2ND_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDIO_DIG_2ND_DSN_DXI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDIO_DIG_2ND_DSN_DXI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_PMIC_NEWIF_CFG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_PMIC_NEWIF_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TOP_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TOP_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG7, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG8, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG9, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_CFG12, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_CFG12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON9, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_VAD_MON11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_VAD_MON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TGEN_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TGEN_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_TGEN_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_TGEN_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_HPF_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_HPF_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_HPF_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_HPF_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG7, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG8, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG9, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG12, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG13, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG13 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG14, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG14 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG15, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG15 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG16, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG16 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG17, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG17 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG18, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG18 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG19, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG19 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG20, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG20 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG21, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG21 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG22, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG22 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG23, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG23 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG24, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG24 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG25, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG25 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG26, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG26 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG27, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG27 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG28, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG28 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG29, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG29 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG30, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG30 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG31, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG31 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG32, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG32 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG33, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG33 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG34, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG34 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG35, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG35 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG36, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG36 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG37, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG37 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG38, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG38 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_CFG39, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_CFG39 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_MON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_MON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_VOW_PERIODIC_MON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_VOW_PERIODIC_MON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_NCP_CFG0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_NCP_CFG1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AFE_NCP_CFG2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AFE_NCP_CFG2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_DSN_FPI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON9, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON12, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_DSN_FPI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON5 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON6, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON6 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON7, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON7 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON8, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON8 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON9, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON9 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON10, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON10 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON11, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON11 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON12, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON12 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON13, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON13 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDDEC_ANA_CON14, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDDEC_ANA_CON14 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON15, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON15 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON16, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON16 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON17, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON17 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON18, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON18 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON19, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON19 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON20, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON20 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON21, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON21 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON22, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON22 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDENC_ANA_CON23, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDENC_ANA_CON23 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_ID, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDZCD_DSN_ID = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_REV0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDZCD_DSN_REV0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_DBI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDZCD_DSN_DBI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_AUDZCD_DSN_FPI, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_AUDZCD_DSN_FPI = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON0, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON0 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON1, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON1 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON2, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON2 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON3, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON3 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON4, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON4 = 0x%x\n", value);
	regmap_read(priv->regmap, MT6359_ZCD_CON5, &value);
	n += scnprintf(buffer + n, size - n,
		       "MT6359_ZCD_CON5 = 0x%x\n", value);

	ret = simple_read_from_buffer(buf, count, pos, buffer, n);
	kfree(buffer);
	return ret;
}

static ssize_t mt6359_debugfs_write(struct file *f, const char __user *buf,
				    size_t count, loff_t *offset)
{
#define MAX_DEBUG_WRITE_INPUT 256
	struct mt6359_priv *priv = f->private_data;
	char input[MAX_DEBUG_WRITE_INPUT];
	char *temp = input;
	char *command = NULL;
	char delim[] = " ,\n";
	const struct command_function *cf;

	memset((void *)input, 0, MAX_DEBUG_WRITE_INPUT);

	if (count > MAX_DEBUG_WRITE_INPUT)
		count = MAX_DEBUG_WRITE_INPUT;

	if (copy_from_user(input, buf, count))
		dev_warn(priv->dev, "%s(), copy_from_user fail, count = %zu\n",
			 __func__, count);

	command = strsep(&temp, delim);

	for (cf = debug_cmds; cf->cmd; cf++) {
		if (strcmp(cf->cmd, command) == 0) {
			cf->fn(f, temp);
			break;
		}
	}

	return count;
}

static const struct file_operations mt6359_debugfs_ops = {
	.open = mt6359_debugfs_open,
	.write = mt6359_debugfs_write,
	.read = mt6359_debugfs_read,
};

#ifndef CONFIG_MTK_PMIC_WRAP
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
static DEFINE_SPINLOCK(ana_set_reg_lock);
#endif
static unsigned int ana_get_reg(unsigned int offset)
{
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
	int ret = 0;
	unsigned int data = 0;

	ret = pwrap_read(offset, &data);

	return data;
#else
	return 0;
#endif
}

static void ana_set_reg(unsigned int offset,
			unsigned int value,
			unsigned int mask)
{
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
	int ret = 0;
	unsigned int reg_value;
	unsigned long flags = 0;

	spin_lock_irqsave(&ana_set_reg_lock, flags);
	reg_value = ana_get_reg(offset);
	reg_value &= (~mask);
	reg_value |= (value & mask);
	ret = pwrap_write(offset, reg_value);
	spin_unlock_irqrestore(&ana_set_reg_lock, flags);

	reg_value = ana_get_reg(offset);
	if ((reg_value & mask) != (value & mask))
		pr_warn("%s(), offset 0x%x, mask = 0x%x ret = %d reg_value = 0x%x\n",
			__func__, offset, mask, ret, reg_value);
#endif
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
	case MT6359_DRV_CON3:
	case MT6359_GPIO_DIR0:
	case MT6359_GPIO_DIR1:
	case MT6359_GPIO_MODE2:
	case MT6359_GPIO_MODE3:
	case MT6359_GPIO_MODE4:
	case MT6359_TOP_CKPDN_CON0:
	case MT6359_TOP_CKHWEN_CON0:
	case MT6359_OTP_CON0:
	case MT6359_OTP_CON8:
	case MT6359_OTP_CON11:
	case MT6359_OTP_CON12:
	case MT6359_OTP_CON13:
	case MT6359_DCXO_CW11:
	case MT6359_DCXO_CW12:
	case MT6359_AUXADC_CON10:
		return true;
	default:
		break;
	};

	if (reg >= MT6359_AUD_TOP_ID && reg <= MT6359_ZCD_CON5)
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

#define REG_STRIDE 2
static const struct regmap_config mt6359_regmap = {
	.reg_bits = 16,
	.val_bits = 16,
	.reg_stride = REG_STRIDE,

	.max_register = MT6359_MAX_REGISTER,
	.writeable_reg = is_writeable_reg,
	.volatile_reg = is_volatile_reg,
	.readable_reg = is_readable_reg,

	.reg_read = reg_read,
	.reg_write = reg_write,

	.cache_type = REGCACHE_NONE,
};
#endif

static int mt6359_platform_driver_probe(struct platform_device *pdev)
{
	struct mt6359_priv *priv;
#ifdef CONFIG_MTK_PMIC_WRAP
	struct device_node *pwrap_node;
#endif

	priv = devm_kzalloc(&pdev->dev,
			    sizeof(struct mt6359_priv),
			    GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, priv);

	priv->dev = &pdev->dev;

#ifndef CONFIG_MTK_PMIC_WRAP
	priv->regmap = devm_regmap_init(&pdev->dev, NULL, NULL, &mt6359_regmap);
#else
	pwrap_node = of_parse_phandle(pdev->dev.of_node,
				      "mediatek,pwrap-regmap", 0);
	if (pwrap_node) {
		priv->regmap = pwrap_node_to_regmap(pwrap_node);
		if (IS_ERR(priv->regmap))
			return PTR_ERR(priv->regmap);
	} else {
		dev_err(&pdev->dev, "get pwrap node fail\n");
		return -EINVAL;
	}
#endif
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	/* create debugfs file */
	priv->debugfs = debugfs_create_file("mtksocanaaudio",
					    S_IFREG | 0444, NULL,
					    priv, &mt6359_debugfs_ops);

	dev_info(priv->dev, "%s(), dev name %s\n",
		__func__, dev_name(&pdev->dev));

	return snd_soc_register_codec(&pdev->dev,
				      &mt6359_soc_codec_driver,
				      mt6359_dai_driver,
				      ARRAY_SIZE(mt6359_dai_driver));
}

static int mt6359_platform_driver_remove(struct platform_device *pdev)
{
	struct mt6359_priv *priv = dev_get_drvdata(&pdev->dev);

	dev_info(&pdev->dev, "%s()\n", __func__);

	debugfs_remove(priv->debugfs);

	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static const struct of_device_id mt6359_of_match[] = {
	{.compatible = "mediatek,mt6359-sound",},
	{}
};
MODULE_DEVICE_TABLE(of, mt6359_of_match);

static struct platform_driver mt6359_platform_driver = {
	.driver = {
		.name = "mt6359-sound",
		.of_match_table = mt6359_of_match,
	},
	.probe = mt6359_platform_driver_probe,
	.remove = mt6359_platform_driver_remove,
};

module_platform_driver(mt6359_platform_driver)

/* Module information */
MODULE_DESCRIPTION("MT6359 ALSA SoC codec driver");
MODULE_AUTHOR("KaiChieh Chuang <kaichieh.chuang@mediatek.com>");
MODULE_LICENSE("GPL v2");
