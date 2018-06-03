/*
 * cs4382a.c  --  CS4382A ALSA SoC codec driver
 *
 * Copyright (c) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

/*
 * The codec isn't really big-endian or little-endian, since the I2S
 * interface requires data to be sent serially with the MSbit first.
 * However, to support BE and LE I2S devices, we specify both here.  That
 * way, ALSA will always match the bit patterns.
 */
#define CS4382A_FORMATS (SNDRV_PCM_FMTBIT_S16_LE      | \
			SNDRV_PCM_FMTBIT_S24_LE      | \
			SNDRV_PCM_FMTBIT_S32_LE)

/* cs4382a registers addresses */
#define CS4382A_MODE1	0x01 /* Mode Control 1 */
#define CS4382A_MODE2	0x02 /* Mode Control 2 */
#define CS4382A_MODE3	0x03 /* Mode Control 3 */
#define CS4382A_FILTER	0x04 /* Filter Control */
#define CS4382A_INVERT	0x05 /* Invert Control */
#define CS4382A_MIXING1	0x06 /* Mixing Control Pair 1 */
#define CS4382A_MIXING2	0x09 /* Mixing Control Pair 2 */
#define CS4382A_MIXING3	0x0C /* Mixing Control Pair 3 */
#define CS4382A_MIXING4	0x0F /* Mixing Control Pair 4 */
#define CS4382A_VOLA1	0x07 /* Volume Control A1 */
#define CS4382A_VOLB1	0x08 /* Volume Control B1 */
#define CS4382A_VOLA2	0x0A /* Volume Control A2 */
#define CS4382A_VOLB2	0x0B /* Volume Control B2 */
#define CS4382A_VOLA3	0x0D /* Volume Control A3 */
#define CS4382A_VOLB3	0x0E /* Volume Control B3 */
#define CS4382A_VOLA4	0x10 /* Volume Control A4 */
#define CS4382A_VOLB4	0x11 /* Volume Control B4 */
#define CS4382A_REVISION	0x12 /* Chip Revision */

#define CS4382A_FIRSTREG	0x01
#define CS4382A_LASTREG	0x12
#define CS4382A_NUMREGS	(CS4382A_LASTREG - CS4382A_FIRSTREG + 1)

/* Bit masks for the cs4382a registers */
#define CS4382A_REVISION_ID	0xF8
#define CS4382A_REVISION_REV	0x07
#define CS4382A_MODE1_PDN	0x01
#define CS4382A_MODE1_CPEN	0x80
#define CS4382A_MIXING_FM	0x3
#define CS4382A_MODE2_FMT	0x70

#define CS4382A_MIXING_FM_SINGLE	0x0
#define CS4382A_MIXING_FM_DOUBLE	0x1
#define CS4382A_MIXING_FM_QUAD	0x2

#define CS4382A_MODE2_FMT_LJ24	(0<<4)
#define CS4382A_MODE2_FMT_I2S24	(1<<4)
#define CS4382A_MODE2_FMT_RJ16	(2<<4)
#define CS4382A_MODE2_FMT_RJ24	(3<<4)
#define CS4382A_MODE2_FMT_RJ20	(4<<4)
#define CS4382A_MODE2_FMT_RJ18	(5<<4)

/* Power-on default values for the registers
 *
 * This array contains the power-on default values of the registers
 */
static const struct reg_default cs4382a_reg_defaults[] = {
	{ 1, 0x01 },
	{ 2, 0x00 },
	{ 3, 0x84 },
	{ 4, 0x00 },
	{ 5, 0x00 },
	{ 6, 0x24 },
	{ 7, 0x00 },
	{ 8, 0x00 },
	{ 9, 0x24 },
	{ 10, 0x00 },
	{ 11, 0x00 },
	{ 12, 0x24 },
	{ 13, 0x00 },
	{ 14, 0x00 },
	{ 15, 0x24 },
	{ 16, 0x00 },
	{ 17, 0x00 },
	{ 18, 0x70 },
};

/* Private data for the cs4382a */
struct cs4382a_private {
	struct regmap *regmap;
	unsigned int mclk; /* Input frequency of the MCLK pin */
	unsigned int mode; /* The mode (I2S or left-justified) */
	unsigned int slave_mode;
	unsigned int manual_mute;
	int rst_gpio;
	/* power domain regulators */
	struct regulator *supply; /* power for va vd vlc vls */
};

struct cs4382a_mode_ratios {
	unsigned int min_rate;
	unsigned int max_rate;
	u8 speed_mode;
};

static const struct cs4382a_mode_ratios cs4382a_mode_ratios[] = {
	{4000, 50000, CS4382A_MIXING_FM_SINGLE},
	{50000, 100000, CS4382A_MIXING_FM_DOUBLE},
	{100000, 200000, CS4382A_MIXING_FM_QUAD},
};
/* The number of MCLK/LRCK ratios supported by the cs4382a */
#define NUM_MCLK_RATIOS		ARRAY_SIZE(cs4382a_mode_ratios)

static const unsigned int cs4382a_mixing_controls[] = {
	CS4382A_MIXING1, CS4382A_MIXING2, CS4382A_MIXING3, CS4382A_MIXING4
};
#define NUM_MIXING_CONTROL		ARRAY_SIZE(cs4382a_mixing_controls)

static bool cs4382a_reg_is_readable(struct device *dev, unsigned int reg)
{
	return (reg >= CS4382A_FIRSTREG) && (reg <= CS4382A_LASTREG);
}

static bool cs4382a_reg_is_volatile(struct device *dev, unsigned int reg)
{
	/* Unreadable registers are considered volatile */
	if ((reg < CS4382A_FIRSTREG) || (reg > CS4382A_LASTREG))
		return 1;

	return reg == CS4382A_REVISION;
}

/**
 * cs4382a_set_dai_sysclk - determine the cs4382a samples rates.
 * @codec_dai: the codec DAI
 * @clk_id: the clock ID (ignored)
 * @freq: the MCLK input frequency
 * @dir: the clock direction (ignored)
 *
 * This function is used to tell the codec driver what the input MCLK
 * frequency is.
 *
 * The value of MCLK is used to determine which sample rates are supported
 * by the cs4382a.  The ratio of MCLK / Fs must be equal to one of nine
 * supported values - 64, 96, 128, 192, 256, 384, 512, 768, and 1024.
 *
 * This function calculates the nine ratios and determines which ones match
 * a standard sample rate.  If there's a match, then it is added to the list
 * of supported sample rates.
 *
 * This function must be called by the machine driver's 'startup' function,
 * otherwise the list of supported sample rates will not be available in
 * time for ALSA.
 *
 * For setups with variable MCLKs, pass 0 as 'freq' argument. This will cause
 * theoretically possible sample rates to be enabled. Call it again with a
 * proper value set one the external clock is set (most probably you would do
 * that from a machine's driver 'hw_param' hook.
 */
static int cs4382a_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				 int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);

	cs4382a->mclk = freq;
	return 0;
}

/**
 * cs4382a_set_dai_fmt - configure the codec for the selected audio format
 * @codec_dai: the codec DAI
 * @format: a SND_SOC_DAIFMT_x value indicating the data format
 *
 * This function takes a bitmask of SND_SOC_DAIFMT_x bits and programs the
 * codec accordingly.
 *
 * Currently, this function only supports SND_SOC_DAIFMT_I2S and
 * SND_SOC_DAIFMT_LEFT_J.  The cs4382a codec also supports right-justified
 * data for playback only, but ASoC currently does not support different
 * formats for playback vs. record.
 */
static int cs4382a_set_dai_fmt(struct snd_soc_dai *codec_dai,
			      unsigned int format)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);

	/* set DAI format */
	switch (format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_LEFT_J:
		cs4382a->mode = format & SND_SOC_DAIFMT_FORMAT_MASK;
		break;
	default:
		dev_err(codec->dev, "invalid dai format\n");
		return -EINVAL;
	}

	/* set master/slave audio interface */
	switch (format & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		cs4382a->slave_mode = format & SND_SOC_DAIFMT_MASTER_MASK;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		cs4382a->slave_mode = format & SND_SOC_DAIFMT_MASTER_MASK;
		break;
	default:
		/* all other modes are unsupported by the hardware */
		dev_err(codec->dev, "Unknown master/slave configuration\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * cs4382a_hw_params - program the cs4382a with the given hardware parameters.
 * @substream: the audio stream
 * @params: the hardware parameters to set
 * @dai: the SOC DAI (ignored)
 *
 * This function programs the hardware with the values provided.
 * Specifically, the sample rate and the data format.
 *
 * The .ops functions are used to provide board-specific data, like input
 * frequencies, to this driver.  This function takes that information,
 * combines it with the hardware parameters provided, and programs the
 * hardware accordingly.
 */
static int cs4382a_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{

	struct snd_soc_codec *codec = dai->codec;
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);
	int ret;
	unsigned int i;
	u8 speed_mode;
	unsigned int rate;
	snd_pcm_format_t stream_fmt;
	int val = 0;

	rate = params_rate(params);	/* Sampling rate, in Hz */
	stream_fmt = params_format(params);

	for (i = 0; i < NUM_MCLK_RATIOS; i++) {
		if (rate > cs4382a_mode_ratios[i].min_rate && rate <= cs4382a_mode_ratios[i].max_rate) {
			speed_mode = cs4382a_mode_ratios[i].speed_mode;
			break;
		}
	}

	if (i == NUM_MCLK_RATIOS) {
		/* We did not find a matching ratio */
		dev_err(codec->dev, "could not find matching ratio\n");
		return -EINVAL;
	}

	/* Set the sample rate speed mode*/
	for (i = 0; i < NUM_MIXING_CONTROL; i++) {
		ret = regmap_update_bits(cs4382a->regmap, cs4382a_mixing_controls[i], CS4382A_MIXING_FM, speed_mode);
		if (ret < 0) {
			dev_err(codec->dev, "i2c write mixing control failed(%d %d)\n", i, ret);
			return ret;
		}
	}

	/* Set the DAI format */
	switch (cs4382a->mode) {
	case SND_SOC_DAIFMT_I2S:
		val = CS4382A_MODE2_FMT_I2S24;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val = CS4382A_MODE2_FMT_LJ24;
		break;
	default:
		dev_err(codec->dev, "unknown dai format\n");
		return -EINVAL;
	}
	ret = regmap_update_bits(cs4382a->regmap, CS4382A_MODE2, CS4382A_MODE2_FMT, val);
	if (ret < 0) {
		dev_err(codec->dev, "i2c write fmt failed(%d)\n", ret);
		return ret;
	}

	return ret;
}

/**
 * cs4382a_dai_mute - enable/disable the cs4382a external mute
 * @dai: the SOC DAI
 * @mute: 0 = disable mute, 1 = enable mute
 *
 * This function toggles the mute bits in the MUTE register.  The cs4382a's
 * mute capability is intended for external muting circuitry, so if the
 * board does not have the MUTEA or MUTEB pins connected to such circuitry,
 * then this function will do nothing.
 */
static int cs4382a_dai_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static const struct snd_soc_dai_ops cs4382a_dai_ops = {
	.hw_params	= cs4382a_hw_params,
	.set_sysclk	= cs4382a_set_dai_sysclk,
	.set_fmt	= cs4382a_set_dai_fmt,
	.digital_mute	= cs4382a_dai_mute,
};

static struct snd_soc_dai_driver cs4382a_dai = {
	.name = "cs4382a-i2s",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min = 4000,
		.rate_max = 216000,
		.formats = CS4382A_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = 4000,
		.formats = 216000,
		.formats = CS4382A_FORMATS,
	},
	.ops = &cs4382a_dai_ops,
};

static int cs4382a_reset(struct device *dev, struct cs4382a_private *cs4382a)
{
	int ret;
	int val = 0;

	/* do codec reset */
	if (gpio_is_valid(cs4382a->rst_gpio))
		gpio_set_value(cs4382a->rst_gpio, 0);

	udelay(80);

	if (gpio_is_valid(cs4382a->rst_gpio))
		gpio_set_value(cs4382a->rst_gpio, 1);

	regcache_cache_bypass(cs4382a->regmap, true);
	ret = regmap_read(cs4382a->regmap, CS4382A_REVISION, &val);
	if (ret < 0) {
		dev_err(dev, "%s failed(%d) to read i2c at cs4382a version\n",
				__func__, ret);
		return ret;
	}
	regcache_cache_bypass(cs4382a->regmap, false);

	/* The top five bits of the chip ID should be 01110. */
	if ((val & CS4382A_REVISION_ID) != 0x70) {
		dev_err(dev, "device id %x is not a cs4382a\n", val);
		return -ENODEV;
	}

	return 0;
}

/**
 * cs4382a_probe - ASoC probe function
 * @pdev: platform device
 *
 * This function is called when ASoC has all the pieces it needs to
 * instantiate a sound driver.
 */
static int cs4382a_probe(struct snd_soc_codec *codec)
{
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);
	int ret;

	ret = regulator_enable(cs4382a->supply);
	if (ret != 0) {
		dev_err(codec->dev, "%s supply enable fail:%d\n", __func__, ret);
		return ret;
	}

	ret = cs4382a_reset(codec->dev, cs4382a);
	if (ret != 0) {
		dev_err(codec->dev, "%s codec reset fail:%d\n", __func__, ret);
		return ret;
	}

	/* Set the control port enable*/
	ret = regmap_update_bits(cs4382a->regmap, CS4382A_MODE1, CS4382A_MODE1_CPEN, CS4382A_MODE1_CPEN);
	if (ret < 0) {
		dev_err(codec->dev, "set cs4382a control port enable fail(%d)\n", ret);
		return ret;
	}

	ret = regmap_update_bits(cs4382a->regmap, CS4382A_MODE1, CS4382A_MODE1_PDN, 0);
	if (ret < 0)
		dev_err(codec->dev, "set cs4382a pdn disable fail(%d)\n", ret);

	return ret;
}

/**
 * cs4382a_remove - ASoC remove function
 * @pdev: platform device
 *
 * This function is the counterpart to cs4382a_probe().
 */
static int cs4382a_remove(struct snd_soc_codec *codec)
{
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);

	regmap_update_bits(cs4382a->regmap, CS4382A_MODE1, CS4382A_MODE1_PDN, CS4382A_MODE1_PDN);
	regulator_disable(cs4382a->supply);

	return 0;
};

#ifdef CONFIG_PM

/* This suspend/resume implementation can handle both - a simple standby
 * where the codec remains powered, and a full suspend, where the voltage
 * domain the codec is connected to is teared down and/or any other hardware
 * reset condition is asserted.
 *
 * The codec's own power saving features are enabled in the suspend callback,
 * and all registers are written back to the hardware when resuming.
 */

static int cs4382a_soc_suspend(struct snd_soc_codec *codec)
{
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);

	regmap_update_bits(cs4382a->regmap, CS4382A_MODE1, CS4382A_MODE1_PDN, CS4382A_MODE1_PDN);
	regulator_disable(cs4382a->supply);

	return 0;
}

static int cs4382a_soc_resume(struct snd_soc_codec *codec)
{
	struct cs4382a_private *cs4382a = snd_soc_codec_get_drvdata(codec);
	int ret;

	ret = regulator_enable(cs4382a->supply);
	if (ret != 0)
		return ret;

	/* In case the device was put to hard reset during sleep, we need to
	 * wait 500ns here before any I2C communication.
	 */
	ndelay(500);

	/* first restore the entire register cache ... */
	regcache_sync(cs4382a->regmap);

	/* ... then disable the power-down bits */
	ret = regmap_update_bits(cs4382a->regmap, CS4382A_MODE1, CS4382A_MODE1_PDN, 0);

	return ret;
}
#else
#define cs4382a_soc_suspend	NULL
#define cs4382a_soc_resume	NULL
#endif /* CONFIG_PM */

/*
 * ASoC codec driver structure
 */
static const struct snd_soc_codec_driver soc_codec_device_cs4382a = {
	.probe =		cs4382a_probe,
	.remove =		cs4382a_remove,
	.suspend =		cs4382a_soc_suspend,
	.resume =		cs4382a_soc_resume,
};

/*
 * cs4382a_of_match - the device tree bindings
 */
static const struct of_device_id cs4382a_of_match[] = {
	{ .compatible = "cirrus,cs4382a", },
	{ }
};
MODULE_DEVICE_TABLE(of, cs4382a_of_match);

static const struct regmap_config cs4382a_regmap = {
	.reg_bits =		8,
	.val_bits =		8,
	.max_register =		CS4382A_LASTREG,
	.reg_defaults =		cs4382a_reg_defaults,
	.num_reg_defaults =	ARRAY_SIZE(cs4382a_reg_defaults),
	.cache_type =		REGCACHE_RBTREE,

	.readable_reg =		cs4382a_reg_is_readable,
	.volatile_reg =		cs4382a_reg_is_volatile,
};

static int cs4382a_gpio_probe(struct device *dev, struct cs4382a_private *cs4382a)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

	cs4382a->rst_gpio = of_get_named_gpio(np, "rst-gpio", 0);
	if (!gpio_is_valid(cs4382a->rst_gpio)) {
		dev_warn(dev, "%s get invalid extamp-power-gpio %d\n", __func__, cs4382a->rst_gpio);
		ret = -EINVAL;
	}

	ret = devm_gpio_request_one(dev, cs4382a->rst_gpio, GPIOF_OUT_INIT_LOW, "cs4382a reset");
	if (ret < 0)
		return ret;

	return ret;
}

/**
 * cs4382a_i2c_probe - initialize the I2C interface of the cs4382a
 * @i2c_client: the I2C client object
 * @id: the I2C device ID (ignored)
 *
 * This function is called whenever the I2C subsystem finds a device that
 * matches the device ID given via a prior call to i2c_add_driver().
 */
static int cs4382a_i2c_probe(struct i2c_client *i2c_client,
	const struct i2c_device_id *id)
{
	struct cs4382a_private *cs4382a;
	int ret;

	cs4382a = devm_kzalloc(&i2c_client->dev, sizeof(struct cs4382a_private),
			      GFP_KERNEL);
	if (!cs4382a)
		return -ENOMEM;

	cs4382a->supply =  devm_regulator_get(&i2c_client->dev, "cs4382a");
	if (IS_ERR(cs4382a->supply)) {
		ret = PTR_ERR(cs4382a->supply);
		return ret;
	}
	if (regulator_is_enabled(cs4382a->supply) == 1)
		regulator_disable(cs4382a->supply);
	ret = regulator_set_voltage(cs4382a->supply, 3300000, 3300000);
	if (ret != 0) {
		dev_err(&i2c_client->dev, "%s failed to set supply to 3.3v %d\n",
			__func__, ret);
		return ret;
	}

	cs4382a_gpio_probe(&i2c_client->dev, cs4382a);

	cs4382a->regmap = devm_regmap_init_i2c(i2c_client, &cs4382a_regmap);
	if (IS_ERR(cs4382a->regmap))
		return PTR_ERR(cs4382a->regmap);

	i2c_set_clientdata(i2c_client, cs4382a);
	ret = snd_soc_register_codec(&i2c_client->dev,
			&soc_codec_device_cs4382a, &cs4382a_dai, 1);

	dev_err(&i2c_client->dev, "%s cs4382a codec register %d\n",
			__func__, ret);
	return ret;
}

/**
 * cs4382a_i2c_remove - remove an I2C device
 * @i2c_client: the I2C client object
 *
 * This function is the counterpart to cs4382a_i2c_probe().
 */
static int cs4382a_i2c_remove(struct i2c_client *i2c_client)
{
	snd_soc_unregister_codec(&i2c_client->dev);
	return 0;
}

/*
 * cs4382a_id - I2C device IDs supported by this driver
 */
static const struct i2c_device_id cs4382a_id[] = {
	{"cs4382a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cs4382a_id);

/*
 * cs4382a_i2c_driver - I2C device identification
 *
 * This structure tells the I2C subsystem how to identify and support a
 * given I2C device type.
 */
static struct i2c_driver cs4382a_i2c_driver = {
	.driver = {
		.name = "cs4382a",
		.of_match_table = cs4382a_of_match,
	},
	.id_table = cs4382a_id,
	.probe = cs4382a_i2c_probe,
	.remove = cs4382a_i2c_remove,
};

module_i2c_driver(cs4382a_i2c_driver);

MODULE_DESCRIPTION("CS4382A ALSA SoC Codec Driver");
MODULE_LICENSE("GPL");
