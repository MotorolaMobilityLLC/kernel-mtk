/*
 * tfa_haptic.c   tfa_haptic codec module
 *
 *
 * Copyright (C) 2014-2020 NXP Semiconductors, All Rights Reserved.
 * Copyright 2020 GOODIX 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>

#include "tfa_haptic.h"


 /* Supported rates and data formats */
#define TFA98XX_RATES SNDRV_PCM_RATE_8000_48000
#define TFA98XX_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static int no_start = 0;
module_param(no_start, int, S_IRUGO);
MODULE_PARM_DESC(no_start, "do not start the work queue; for debugging via user\n");

static int no_reset = 0;
module_param(no_reset, int, S_IRUGO);

static uint16_t tfa_get_bf_value(const uint16_t bf, const uint16_t reg_value)
{
	uint16_t msk, value;

	/*
	 * bitfield enum:
	 * - 0..3  : len
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	uint8_t len = bf & 0x0f;
	uint8_t pos = (bf >> 4) & 0x0f;

	msk = ((1<<(len+1))-1)<<pos;
	value = (reg_value & msk) >> pos;

	return value;
}


static int tfa_hap_read_bf(struct regmap *regmap, uint16_t bitfield)
{
	int ret;
	uint32_t reg = (bitfield >> 8) & 0xff; /* extract register addess */
	uint32_t value;

	ret = regmap_read(regmap, reg, &value);
	if (ret < 0)
		return ret;

	return tfa_get_bf_value(bitfield, (uint16_t)value); /* extract the value */
}

static int tfa_hap_write_bf(struct regmap *regmap, uint16_t bitfield, uint16_t value)
{
	int ret;
	uint32_t regvalue, oldvalue;
	uint16_t msk;
	uint8_t len, pos, address;

	/*
	 * bitfield enum:
	 * - 0..3  : len-1
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	len = (bitfield & 0x0f) + 1;
	pos = (bitfield >> 4) & 0x0f;
	address = (bitfield >> 8) & 0xff;

	ret = regmap_read(regmap, address, &regvalue);
	if (ret < 0)
		return ret;

	oldvalue = regvalue;
	msk = ((1<<len)-1)<<pos;
	regvalue &= ~msk;
	regvalue |= value<<pos;

	/* Only write when the current register value is not the same as the new value */
	if (oldvalue != regvalue)
		return regmap_write(regmap, (bitfield >> 8) & 0xff, regvalue);

	return 0;
}

/******************************************************************************
 * TFA device register	functions
 */
static void tfa_haptic_optimal_settings(struct regmap *regmap, uint16_t revid)
{
	/* The optimal settings */
	switch (revid) {
	case 0x0c74:
		/* ----- generated code start ----- */
		/* V1.16 */
		regmap_write(regmap, 0x02, 0x22c8); //POR=0x25c8
		regmap_write(regmap, 0x52, 0x57dc); //POR=0x56dc
		regmap_write(regmap, 0x53, 0x003e); //POR=0x001e
		regmap_write(regmap, 0x56, 0x0400); //POR=0x0600
		regmap_write(regmap, 0x61, 0x0110); //POR=0x0198
		regmap_write(regmap, 0x6f, 0x00a5); //POR=0x01a3
		regmap_write(regmap, 0x70, 0x07f8); //POR=0x06f8
		regmap_write(regmap, 0x73, 0x0047); //POR=0x0045
		regmap_write(regmap, 0x74, 0x5098); //POR=0xcc80
		regmap_write(regmap, 0x75, 0x8d28); //POR=0x1138
		regmap_write(regmap, 0x80, 0x0000); //POR=0x0003
		regmap_write(regmap, 0x83, 0x0799); //POR=0x061a
		regmap_write(regmap, 0x84, 0x0081); //POR=0x0021
		/* ----- generated code end   ----- */

		break;
	case 0x0b73:
		/* -----  version 20 ----- */
		regmap_write(regmap, 0x02, 0x0628); //POR=0x0008
		regmap_write(regmap, 0x61, 0x0183); //POR=0x0182
		regmap_write(regmap, 0x63, 0x005a); //POR=0x055a
		regmap_write(regmap, 0x6f, 0x0082); //POR=0x00a5
		regmap_write(regmap, 0x70, 0xa3eb); //POR=0x23fb
		regmap_write(regmap, 0x71, 0x107e); //POR=0x007e
		regmap_write(regmap, 0x73, 0x0187); //POR=0x0107
		regmap_write(regmap, 0x83, 0x071c); //POR=0x0799
		regmap_write(regmap, 0x85, 0x0380); //POR=0x0382
		regmap_write(regmap, 0xd5, 0x004d); //POR=0x014d
		/* ----- generated code end   ----- */

		break;
	default:
		//pr_info("No optimal settings for 0x%.2x\n", data->revid);
		break;
	}
}

static void tfa_haptic_cold_start(struct regmap *regmap, uint16_t revid)
{
	tfa_haptic_optimal_settings(regmap, revid);

	/* The optimal settings */
	switch (revid) {
	case 0x0c74:
		tfa_hap_write_bf(regmap, TFA9874_BF_AUDFS, 8);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDME, 1);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMSLOTS, 1);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMNBCK, 0);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMSSIZE, 15);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMSLLN, 15);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMVSS, 0);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMCSS, 1);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMVSE, 1);
		tfa_hap_write_bf(regmap, TFA9874_BF_TDMCSE, 1);
		//tfa_hap_write_bf(regmap, TFA9874_BF_INPLEV, 1);
		//tfa_hap_write_bf(regmap, TFA9874_BF_TDMSPKG, 5);
		tfa_hap_write_bf(regmap, TFA9874_BF_MANSCONF,1);

		break;
	case 0x0b73:
		tfa_hap_write_bf(regmap, TFA9873_BF_AUDFS, 8);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDME, 1);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMSLOTS, 1);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMNBCK, 0);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMSSIZE, 15);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMSLLN, 15);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMVSS, 0);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMCSS, 1);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMVSE, 1);
		tfa_hap_write_bf(regmap, TFA9873_BF_TDMCSE, 1);
		//tfa_hap_write_bf(regmap, TFA9873_BF_INPLEV, 1);
		//tfa_hap_write_bf(regmap, TFA9873_BF_TDMSPKG, 5);
		tfa_hap_write_bf(regmap, TFA9873_BF_MANSCONF,1);

		break;
	default:
		//pr_info("No optimal settings for 0x%.2x\n", data->revid);
		break;
	}

}

extern int send_tfa_cal_apr(int8_t *buffer, 
				size_t size, bool read) __attribute__((weak));

int send_tfa_cal_apr(int8_t *buffer, size_t size, bool read)
{
	(void)buffer;
	(void)size;
	(void)read;
	return 0;
}

static ssize_t tfa_haptic_rpc_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t off, size_t count)
{
	return count;
}

static ssize_t tfa_haptic_rpc_send(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t off, size_t count)
{

	return count;
}

static void *tfa_haptic_devm_kstrdup(struct device *dev, char *buf)
{
	char *str = devm_kzalloc(dev, strlen(buf) + 1, GFP_KERNEL);
	if (!str)
		return str;
	memcpy(str, buf, strlen(buf));
	return str;
}

static int tfa_haptic_append_i2c_address(struct device *dev,
	struct i2c_client *i2c,
	struct snd_soc_dapm_widget *widgets,
	int num_widgets,
	struct snd_soc_dai_driver *dai_drv,
	int num_dai)
{
	char buf[50] = {0};
	int i;
	int i2cbus = i2c->adapter->nr;
	int addr = i2c->addr;
	if (dai_drv && num_dai > 0)
		for (i = 0; i < num_dai; i++) {
			snprintf(buf, 50, "%s-%x-%x", dai_drv[i].name, i2cbus,
				addr);
			dai_drv[i].name = tfa_haptic_devm_kstrdup(dev, (char*)buf);

			snprintf(buf, 50, "%s-%x-%x",
				dai_drv[i].playback.stream_name,
				i2cbus, addr);
			dai_drv[i].playback.stream_name = tfa_haptic_devm_kstrdup(dev, (char*)buf);

			snprintf(buf, 50, "%s-%x-%x",
				dai_drv[i].capture.stream_name,
				i2cbus, addr);
			dai_drv[i].capture.stream_name = tfa_haptic_devm_kstrdup(dev, (char*)buf);
		}

	/* the idea behind this is convert:
	 * SND_SOC_DAPM_AIF_IN("AIF IN", "AIF Playback", 0, SND_SOC_NOPM, 0, 0),
	 * into:
	 * SND_SOC_DAPM_AIF_IN("AIF IN", "AIF Playback-2-36", 0, SND_SOC_NOPM, 0, 0),
	 */
	if (widgets && num_widgets > 0)
		for (i = 0; i < num_widgets; i++) {
			if (!widgets[i].sname)
				continue;
			if ((widgets[i].id == snd_soc_dapm_aif_in)
				|| (widgets[i].id == snd_soc_dapm_aif_out)) {
				snprintf(buf, 50, "%s-%x-%x", widgets[i].sname,
					i2cbus, addr);
				widgets[i].sname = tfa_haptic_devm_kstrdup(dev, (char*)buf);
			}
		}

	return 0;
}

static int tfa_haptic_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	(void) params;
	(void) dai;

	return 0;
}

static int tfa_haptic_mute(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *codec = dai->component;
	struct tfa_haptic *tfa_haptic = snd_soc_component_get_drvdata(codec);

	if (mute) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			switch (tfa_haptic->rev) {
			case 0x0c74:
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9874_BF_PWDN, 1);
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9874_BF_AMPE, 0);
				break;

			case 0x0b73:
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_PWDN, 1);
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_AMPE, 0);		   
				break;

			default:
				break;
			}
		}
	}
	else {
		if (tfa_hap_read_bf(tfa_haptic->regmap, status_flag_por) == 1)
			tfa_haptic_cold_start(tfa_haptic->regmap, tfa_haptic->rev);

		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			switch (tfa_haptic->rev) {
			case 0x0c74:
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9874_BF_PWDN, 0);
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9874_BF_MANSCONF, 1);
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9874_BF_AMPE, 1);
				break;

			case 0x0b73:
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_PWDN, 0);
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_MANSCONF, 1); 
				tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_AMPE, 1);		   
				break;

			default:
				break;
			}
		}
	}

	return 0;
}

static const struct snd_soc_dai_ops tfa_haptic_dai_ops = {
	.hw_params = tfa_haptic_hw_params,
	.mute_stream = tfa_haptic_mute,
};

static struct snd_soc_dai_driver tfa_haptic_dai[] = {
	{
		.name = "tfa_haptic-aif",
		.id = 1,
		.playback = {
			.stream_name = "AIF Playback",
			.channels_min = 1,
			.channels_max = 4,
			.rates = TFA98XX_RATES,
			.formats = TFA98XX_FORMATS,
		},
		.capture = {
			 .stream_name = "AIF Capture",
			 .channels_min = 1,
			 .channels_max = 4,
			 .rates = TFA98XX_RATES,
			 .formats = TFA98XX_FORMATS,
		 },
		.ops = &tfa_haptic_dai_ops,
	},
};


static int tfa_haptic_probe(struct snd_soc_component *codec)
{
	struct tfa_haptic *tfa_haptic = snd_soc_component_get_drvdata(codec);
	uint32_t value;
	int ret;

	ret = tfa_hap_write_bf(tfa_haptic->regmap, TFA9873_BF_I2CR, 1);
	if (ret < 0)
		return ret;

	snd_soc_component_init_regmap(codec, tfa_haptic->regmap);
	/*Unlock*/
	regmap_write(tfa_haptic->regmap, 0x0f, 0x5a6b);
	regmap_read(tfa_haptic->regmap, 0xfb, &value);
	regmap_write(tfa_haptic->regmap, 0xa0, (value^0x5a) & 0xffff);
	regmap_write(tfa_haptic->regmap, 0xa1, 0x5a);

	tfa_haptic_cold_start(tfa_haptic->regmap, tfa_haptic->rev);

	dev_info(codec->dev, "tfa_haptic codec registered");

	return ret;
}


static void tfa_haptic_remove(struct snd_soc_component *codec)
{
	(void) codec;
}

static struct snd_soc_component_driver soc_codec_dev_tfa_haptic = {
	.probe =	tfa_haptic_probe,
	.remove =	tfa_haptic_remove,

};

static bool tfa_haptic_writeable_register(struct device *dev, unsigned int reg)
{
	/* enable read access for all registers */
	return 1;
}

static bool tfa_haptic_readable_register(struct device *dev, unsigned int reg)
{
	/* enable read access for all registers */
	return 1;
}

static bool tfa_haptic_volatile_register(struct device *dev, unsigned int reg)
{
	/* enable read access for all registers */
	return 1;
}

static const struct regmap_config tfa_haptic_regmap = {
	.reg_bits = 8,
	.val_bits = 16,

	.max_register = 0xff,
	.writeable_reg = tfa_haptic_writeable_register,
	.readable_reg = tfa_haptic_readable_register,
	.volatile_reg = tfa_haptic_volatile_register,
	.cache_type = REGCACHE_NONE,
};

static int tfa_haptic_ext_reset(struct tfa_haptic *tfa_haptic)
{
	if (tfa_haptic && gpio_is_valid(tfa_haptic->reset_gpio)) {
		gpio_set_value_cansleep(tfa_haptic->reset_gpio, 1);
		mdelay(1);
		gpio_set_value_cansleep(tfa_haptic->reset_gpio, 0);
		mdelay(1);
	}
	return 0;
}

static int tfa_haptic_parse_dt(struct tfa_haptic *tfa_haptic,
	struct device_node *np) {

	tfa_haptic->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (tfa_haptic->reset_gpio < 0)
		pr_warn("No reset GPIO provided, will not HW reset device\n");

	return 0;
}

static struct bin_attribute dev_attr_rpc = {
	.attr = {
		.name = "rpc",
		.mode = S_IRUSR | S_IWUSR,
	},
	.size = 0,
	.read = tfa_haptic_rpc_read,
	.write = tfa_haptic_rpc_send,
};

static int tfa_haptic_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{
	struct snd_soc_dai_driver *dai	= NULL;
	struct tfa_haptic *tfa_haptic	= NULL;
	struct device_node *np = i2c->dev.of_node;
	unsigned int reg;
	int ret;

	pr_info("addr=0x%x\n", i2c->addr);

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		dev_err(&i2c->dev, "check_functionality failed\n");
		return -EIO;
	}

	tfa_haptic = devm_kzalloc(&i2c->dev, sizeof(struct tfa_haptic), GFP_KERNEL);
	if (tfa_haptic == NULL)
		return -ENOMEM;

	tfa_haptic->i2c = i2c;

	tfa_haptic->regmap = devm_regmap_init_i2c(i2c, &tfa_haptic_regmap);
	if (IS_ERR(tfa_haptic->regmap)) {
		ret = PTR_ERR(tfa_haptic->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	i2c_set_clientdata(i2c, tfa_haptic);

	if (np) {
		ret = tfa_haptic_parse_dt(tfa_haptic, np);
		if (ret) {
			dev_err(&i2c->dev, "Failed to parse DT node\n");
			return ret;
		}

		if (no_reset)
			tfa_haptic->reset_gpio = -1;
	}
	else {
		tfa_haptic->reset_gpio = -1;
	}

	if (gpio_is_valid(tfa_haptic->reset_gpio)) {
		ret = devm_gpio_request_one(&i2c->dev, tfa_haptic->reset_gpio,
			GPIOF_OUT_INIT_LOW, "TFA98XX_RST");
		if (ret)
			return ret;
	}

	/* Power up! */
	tfa_haptic_ext_reset(tfa_haptic);

	if ((no_start == 0) && (no_reset == 0)) {
		ret = regmap_read(tfa_haptic->regmap, 0x03, &reg);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to read Revision register: %d\n",
				ret);
			return -EIO;
		}

		tfa_haptic->rev = reg & 0xffff;

		switch (reg & 0xff) {
		case 0x73: /* tfa9873 */
			pr_info("TFA9873 detected\n");
			break;
		case 0x74: /* tfa9874 */
			pr_info("TFA9874 detected\n");
			break;
		default:
			pr_info("Unsupported device revision (0x%x)\n", reg & 0xff);
			if (gpio_is_valid(tfa_haptic->reset_gpio))
				devm_gpio_free(&i2c->dev, tfa_haptic->reset_gpio);		   
			return -EINVAL;
		}
	}


	/* Modify the stream names, by appending the i2c device address.
	 * This is used with multicodec, in order to discriminate the devices.
	 * Stream names appear in the dai definition and in the stream		 .
	 * We create copies of original structures because each device will
	 * have its own instance of this structure, with its own address.
	 */
	dai = devm_kzalloc(&i2c->dev, sizeof(tfa_haptic_dai), GFP_KERNEL);
	if (!dai)
		return -ENOMEM;
	memcpy(dai, tfa_haptic_dai, sizeof(tfa_haptic_dai));

	tfa_haptic_append_i2c_address(&i2c->dev,
		i2c,
		NULL,
		0,
		dai,
		ARRAY_SIZE(tfa_haptic_dai));

	ret = devm_snd_soc_register_component(&i2c->dev,
				&soc_codec_dev_tfa_haptic, dai,
				ARRAY_SIZE(tfa_haptic_dai));
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to register TFA98xx: %d\n", ret);
		return ret;
	}

	ret = device_create_bin_file(&i2c->dev, &dev_attr_rpc);
	if (ret)
		dev_info(&i2c->dev, "error creating sysfs files\n");

	pr_info("%s Probe completed successfully!\n", __func__);

	return 0;
}

static int tfa_haptic_i2c_remove(struct i2c_client *i2c)
{
	struct tfa_haptic *tfa_haptic = i2c_get_clientdata(i2c);

	pr_debug("addr=0x%x\n", i2c->addr);

	snd_soc_unregister_component(&i2c->dev);

	if (gpio_is_valid(tfa_haptic->reset_gpio))
		devm_gpio_free(&i2c->dev, tfa_haptic->reset_gpio);

	return 0;
}

static const struct i2c_device_id tfa_haptic_i2c_id[] = {
	{ "tfa_haptic", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tfa_haptic_i2c_id);

#ifdef CONFIG_OF
static struct of_device_id tfa_haptic_dt_match[] = {
	{.compatible = "tfa,tfa_haptic" },
	{ },
};
#endif

static struct i2c_driver tfa_haptic_i2c_driver = {
	.driver = {
		.name = "tfa_haptic",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(tfa_haptic_dt_match),
	},
	.probe = tfa_haptic_i2c_probe,
	.remove = tfa_haptic_i2c_remove,
	.id_table = tfa_haptic_i2c_id,
};

static int __init tfa_haptic_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&tfa_haptic_i2c_driver);

	return ret;
}

static void __exit tfa_haptic_i2c_exit(void)
{
	i2c_del_driver(&tfa_haptic_i2c_driver);
}

module_init(tfa_haptic_i2c_init);
module_exit(tfa_haptic_i2c_exit);

MODULE_DESCRIPTION("ASoC TFA_HAPTIC driver");
MODULE_LICENSE("GPL");

