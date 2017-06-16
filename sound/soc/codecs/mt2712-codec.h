/*
 * definitions for PCM1792A
 *
 * Copyright 2013 Amarula Solutions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT2712_CODEC_H__
#define __MT2712_CODEC_H__

struct mt2712_codec_priv {
	void __iomem *base_addr;
	struct regmap *regmap_dig;
	struct regmap *regmap_ana;
	struct snd_soc_codec *codec;
	int pga_gain;
};

enum {
	ANALOG_UL_PGA_GAIN_negtive_6dB = 0,
	ANALOG_UL_PGA_GAIN_0dB         = 1,
	ANALOG_UL_PGA_GAIN_6dB         = 2,
	ANALOG_UL_PGA_GAIN_12dB        = 3,
	ANALOG_UL_PGA_GAIN_18dB        = 4,
	ANALOG_UL_PGA_GAIN_24dB        = 5,
	ANALOG_UL_PGA_GAIN_NA          = 6,
};

#define ABB_AFE_CON0 0x00
#define ABB_AFE_CON1 0x04
#define ABB_AFE_CON11 0x2c
#define AFE_ADDA_UL_SRC_CON0 0x3c
#define AFE_ADDA_UL_SRC_CON1 0x40
#define AFE_ADDA_UL_DL_CON0 0x44

#define AFIFO_RATE (0x1f << 3)
#define AFIFO_RATE_SET(x) (x << 3)
#define AFIFO_SRPT (0x7)
#define AFIFO_SRPT_SET(x) (x)
#define ABB_UL_RATE (0x1 << 4)
#define ABB_UL_RATE_SET(x) (x << 4)
#define ABB_PDN_I2SO1 (0x1 << 3)
#define ABB_PDN_I2SO1_SET(x) (x << 3)
#define ABB_PDN_I2SI1 (0x1 << 2)
#define ABB_PDN_I2SI1_SET(x) (x << 2)
#define ABB_UL_EN (0x1 << 1)
#define ABB_UL_EN_SET(x) (x << 1)
#define ABB_AFE_EN (0x1)
#define ABB_AFE_EN_SET(x) (x)
#define ULSRC_VOICE_MODE (0x3 << 17)
#define ULSRC_VOICE_MODE_SET(x) (x << 17)
#define ULSRC_ON (0x1)
#define ULSRC_ON_SET(x) (x)

#define ADDA_END_ADDR 0x88

/* AFE_ADDA_UL_SRC_CON1: ul src sgen setting */
#define UL_SRC_SGEN_EN_POS		27
#define UL_SRC_SGEN_EN_MASK		(1 << UL_SRC_SGEN_EN_POS)
#define UL_SRC_MUTE_POS			26
#define UL_SRC_MUTE_MASK		(1 << UL_SRC_MUTE_POS)
#define UL_SRC_CH2_AMP_POS		21
#define UL_SRC_CH2_AMP_MASK		(0x7 << UL_SRC_CH2_AMP_POS)
#define UL_SRC_CH2_FREQ_POS		16
#define UL_SRC_CH2_FREQ_MASK		(0x1f << UL_SRC_CH2_FREQ_POS)
#define UL_SRC_CH2_MODE_POS		12
#define UL_SRC_CH2_MODE_MASK		(0xf << UL_SRC_CH2_MODE_POS)
#define UL_SRC_CH1_AMP_POS		9
#define UL_SRC_CH1_AMP_MASK		(0x7 << UL_SRC_CH1_AMP_POS)
#define UL_SRC_CH1_FREQ_POS		4
#define UL_SRC_CH1_FREQ_MASK		(0x1f << UL_SRC_CH1_FREQ_POS)
#define UL_SRC_CH1_MODE_POS		0
#define UL_SRC_CH1_MODE_MASK		(0xf << UL_SRC_CH1_MODE_POS)

/* AFE_ADDA_UL_DL_CON0 */
#define ADDA_adda_afe_on_POS		0
#define ADDA_adda_afe_on_MASK		(1 << ADDA_adda_afe_on_POS)

/* apmixedsys regmap - analog part reg */
#define APMIXED_OFFSET           (0)	/* 0x10209000 */
#define APMIXED_REG(reg)         (reg | APMIXED_OFFSET)

#define AADC_CON0        APMIXED_REG(0x0910)
#define AADC_CON1        APMIXED_REG(0x0914)
#define AADC_CON2        APMIXED_REG(0x0918)
#define AADC_CON3        APMIXED_REG(0x091C)
#define AADC_CON4        APMIXED_REG(0x0920)

/* AADC_CON0 */
#define RG_AUDULR_VCM14_EN_POS			1
#define RG_AUDULR_VCM14_EN_MASK			(1 << RG_AUDULR_VCM14_EN_POS)
#define RG_AUDULR_VREF24_EN_POS			2
#define RG_AUDULR_VREF24_EN_MASK		(1 << RG_AUDULR_VREF24_EN_POS)
#define RG_AUDULR_VADC_DVREF_CAL_POS		3
#define RG_AUDULR_VADC_DVREF_CAL_MASK		(1 << RG_AUDULR_VADC_DVREF_CAL_POS)
#define RG_AUDULR_VUPG_POS			4
#define RG_AUDULR_VUPG_MASK			(7 << RG_AUDULR_VUPG_POS)
#define RG_AUDULR_VADC_DENB_POS			7
#define RG_AUDULR_VADC_DENB_MASK		(1 << RG_AUDULR_VADC_DENB_POS)
#define RG_AUDULR_VPWDB_ADC_POS			15
#define RG_AUDULR_VPWDB_ADC_MASK		(1 << RG_AUDULR_VPWDB_ADC_POS)
#define RG_AUDULR_VPWDB_PGA_POS			16
#define RG_AUDULR_VPWDB_PGA_MASK		(1 << RG_AUDULR_VPWDB_PGA_POS)
#define RG_AUDULL_VCM14_EN_POS			19
#define RG_AUDULL_VCM14_EN_MASK			(1 << RG_AUDULL_VCM14_EN_POS)
#define RG_AUDULL_VREF24_EN_POS			20
#define RG_AUDULL_VREF24_EN_MASK		(1 << RG_AUDULL_VREF24_EN_POS)
#define RG_AUDULL_VADC_DVREF_CAL_POS		21
#define RG_AUDULL_VADC_DVREF_CAL_MASK		(1 << RG_AUDULL_VADC_DVREF_CAL_POS)
#define RG_AUDULL_VADC_DENB_POS			22
#define RG_AUDULL_VADC_DENB_MASK		(1 << RG_AUDULL_VADC_DENB_POS)
#define RG_AUDULL_VPWDB_ADC_POS			23
#define RG_AUDULL_VPWDB_ADC_MASK		(1 << RG_AUDULL_VPWDB_ADC_POS)
#define RG_AUDULL_VPWDB_PGA_POS			24
#define RG_AUDULL_VPWDB_PGA_MASK		(1 << RG_AUDULL_VPWDB_PGA_POS)
#define RG_AUDULL_VUPG_POS			25
#define RG_AUDULL_VUPG_MASK			(7 << RG_AUDULL_VUPG_POS)
#define RG_AUDULL_VCFG_POS			28

/* AADC_CON3 */
#define RG_AUDUL_TEST_TIELOW_POS	1
#define RG_AUDUL_TEST_TIELOW_MASK	(1 << RG_AUDUL_TEST_TIELOW_POS)
#define RG_AUDMICBIASVREF_POS		12
#define RG_AUDMICBIASVREF_MASK		(3 << RG_AUDMICBIASVREF_POS)
#define RG_AUDPWDBMICBIAS_POS		14
#define RG_AUDPWDBMICBIAS_MASK		(1 << RG_AUDPWDBMICBIAS_POS)
#define RG_CLK_SEL_POS			16
#define RG_CLK_SEL_MASK			(1 << RG_CLK_SEL_POS)
#define RG_CLK_EN_POS			17
#define RG_CLK_EN_MASK			(1 << RG_CLK_EN_POS)
#define RG_ANAREG_DELAY_SEL_POS		29
#define RG_ANAREG_DELAY_SEL_MASK	(3 << RG_ANAREG_DELAY_SEL_POS)

#endif
