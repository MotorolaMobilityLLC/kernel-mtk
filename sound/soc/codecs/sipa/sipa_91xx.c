/*
 * Copyright (C) 2018, SI-IN, Yun Shi (yun.shi@si-in.com).
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

#define DEBUG
// #define HAVE_BACKTRACE_SUPPORT
#define LOG_FLAG	"sipa_91xx"

#include <linux/regmap.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>
#include <linux/interrupt.h>
#include <sound/pcm_params.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include "sipa_common.h"
#include "sipa_regmap.h"
#include "sipa_91xx.h"


#define SIPA_DRV_CHANGE_I2C_I2S_TIME_SEQ		(0)

static struct snd_soc_dapm_widget sia91xx_dapm_widgets_common[] = {
	/* Stream widgets */
	SND_SOC_DAPM_AIF_IN("AIF IN", "AIF Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF OUT", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_OUTPUT("OUTL"),
	SND_SOC_DAPM_INPUT("AEC Loopback"),
};

static const struct snd_soc_dapm_route sia91xx_dapm_routes_common[] = {
	{ "OUTL", NULL, "AIF IN" },
	{ "AIF OUT", NULL, "AEC Loopback" },
};

const struct sia91xx_irq_desc irq_range[] = {
	{0x1, "PORI"},
	{0x4, "NOCLK"},
	{0x8, "OTPI"},
	{0x20, "UVPI"},
	{0x40, "OCPI"},
	{0x80, "TDMERR"},
};

static void sia91xx_interrupt(struct work_struct *work)
{
	sipa_dev_t *si_pa = container_of(work, sipa_dev_t, interrupt_work.work);
	uint32_t IntReg, IntEn, IntVal, regVal;

	pr_info("[ info][%s] %s: %s irq ch(%d) trigger \r\n",
		LOG_FLAG, __func__, si_pa->name, si_pa->channel_num);

	sipa_read_reg(si_pa->regmap, SIA91XX_REG_INT_REG, &IntReg);
	sipa_read_reg(si_pa->regmap, SIA91XX_REG_INT_EN, &IntEn);

	pr_info("[ info][%s] %s: IntReg(0x%x) IntEn(0x%x) \r\n",
		LOG_FLAG, __func__, IntReg, IntEn);

	if (0 == (IntEn & IntReg)) {
		pr_err("[  err][%s] %s: interrupt source error !!! \r\n",LOG_FLAG, __func__);
		return;
	}
	IntVal = IntReg & 0xffff;

	sipa_read_reg(si_pa->regmap, SIA91XX_REG_INT_STAT, &regVal);
	pr_info("[ info][%s] %s: Int_Stat(0x%x) \r\n",
		LOG_FLAG, __func__, regVal);

	usleep_range(1000, 2000);
	sipa_write_reg(si_pa->regmap, SIA91XX_REG_INT_WC_REG, IntVal);
}

static irqreturn_t sia91xx_irq(int irq, void *data)
{
	sipa_dev_t *si_pa = (sipa_dev_t *)data;
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL !!! \r\n", LOG_FLAG, __func__);
		return IRQ_HANDLED;
	}

	if (si_pa->sipa_wq != NULL) {
		queue_delayed_work(si_pa->sipa_wq, &si_pa->interrupt_work, 0);
	}
	return IRQ_HANDLED;
}


static void sia91xx_register_interrupt(sipa_dev_t *si_pa)
{
	int irq_flags;
	int ret = 0;
	char *sipa_irq_name = NULL;

	if (gpio_is_valid(si_pa->irq_pin)) {

		INIT_DELAYED_WORK(&si_pa->interrupt_work, sia91xx_interrupt);

		gpio_direction_input(si_pa->irq_pin);
		irq_flags = IRQF_TRIGGER_FALLING;
		sipa_irq_name = devm_kzalloc(&(si_pa->pdev->dev), 50, GFP_KERNEL);
		if (!sipa_irq_name) {
			pr_err("[  err][%s] %s: sipa_irq_name point error ! \r\n", LOG_FLAG, __func__);
			return;
		}

		snprintf(sipa_irq_name, 50, "sipa_irq_ch_%d", si_pa->channel_num);

		ret = devm_request_irq(&(si_pa->pdev->dev), gpio_to_irq(si_pa->irq_pin), sia91xx_irq,
				irq_flags|IRQF_ONESHOT|IRQF_SHARED, sipa_irq_name, si_pa);
		if (ret) {
			pr_err("[  err][%s] %s: devm_request_irq error ! \r\n", LOG_FLAG, __func__);
			return;
		}

		pr_info("[ info][%s] %s: %s irq_pin(%d) int_num(%d): %s irq register \r\n",
			LOG_FLAG, __func__, si_pa->name, si_pa->irq_pin, gpio_to_irq(si_pa->irq_pin), sipa_irq_name);

	} else {
		pr_info("[ info][%s] %s: irq pin not provided ! \r\n", LOG_FLAG, __func__);
	}
	return;
}

static int sia91xx_ext_reset(sipa_dev_t *si_pa)
{
	if (si_pa && gpio_is_valid(si_pa->rst_pin)) {

		if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
			gpio_set_value_cansleep(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
			usleep_range(10000, 12000);
		} else {
			gpio_set_value_cansleep(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
			usleep_range(5000, 6000);
			gpio_set_value_cansleep(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
			usleep_range(5000, 6000);
		}
	}
	return 0;
}

static void *sia91xx_devm_kstrdup(struct device *dev, char *buf)
{
	char *str = devm_kzalloc(dev, strlen(buf) + 1, GFP_KERNEL);
	if (!str)
		return str;

	memcpy(str, buf, strlen(buf));
	return str;
}

int sia91xx_append_i2c_address(
	struct device *dev,
	struct i2c_client *i2c,
	struct snd_soc_dapm_widget *widgets,
	int num_widgets,
	struct snd_soc_dai_driver *dai_drv,
	int num_dai)
{
	char buf[50];
	int i;
	int i2cbus = i2c->adapter->nr;
	int addr = i2c->addr;

	if (dai_drv && num_dai > 0) {
		for (i = 0; i < num_dai; i++) {
			snprintf(buf, 50, "%s-%x-%x", dai_drv[i].name, i2cbus, addr);
			dai_drv[i].name = sia91xx_devm_kstrdup(dev, buf);

			snprintf(buf, 50, "%s-%x-%x",
						dai_drv[i].playback.stream_name,
						i2cbus, addr);
			dai_drv[i].playback.stream_name = sia91xx_devm_kstrdup(dev, buf);

			snprintf(buf, 50, "%s-%x-%x",
						dai_drv[i].capture.stream_name,
						i2cbus,
						addr);
			dai_drv[i].capture.stream_name = sia91xx_devm_kstrdup(dev, buf);
		}
	}

	if (widgets && num_widgets > 0) {
		for (i = 0; i < num_widgets; i++) {
			if (!widgets[i].sname)
				continue;

			if ((widgets[i].id == snd_soc_dapm_aif_in)
				|| (widgets[i].id == snd_soc_dapm_aif_out)) {
				snprintf(buf, 50, "%s-%x-%x", widgets[i].sname, i2cbus, addr);
				widgets[i].sname = sia91xx_devm_kstrdup(dev, buf);
			}
		}
	}
	return 0;
}

static void sia91xx_get_rst_value(sipa_dev_t *si_pa)
{
	unsigned int gpio_value = 0xff;
	if (0 == si_pa->disable_pin) {
		gpio_value = gpio_get_value(si_pa->rst_pin);
		pr_debug("[debug][%s] %s: reset pin num:%u, value:%u \r\n",
			LOG_FLAG, __func__, si_pa->rst_pin, gpio_value);
	}
}

int sia91xx_detect_chip(
	sipa_dev_t *si_pa)
{
	int ret;

	/* Power up! */
	sia91xx_ext_reset(si_pa);
	sia91xx_get_rst_value(si_pa);
	ret = sipa_regmap_check_chip_id(si_pa->regmap,
		si_pa->channel_num, si_pa->chip_type, true);
	if (ret < 0) {
		pr_err("[  err][%s] %s: Failed to read Revision register: %d \r\n",
			LOG_FLAG, __func__, ret);
		return -EIO;
	}

	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
		gpio_set_value(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
	}
	return 0;
}

int sia91xx_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	//struct snd_soc_codec *codec = dai->codec;
	//sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
	unsigned int rate, width, channels;

	if (SNDRV_PCM_STREAM_CAPTURE == substream->stream) {
		return 0;
	}

	rate = params_rate(params);
	pr_info("[ info][%s] %s: dai:%s %s i2s rate: %u !!! \r\n",
		LOG_FLAG, __func__, dai->name, substream->name, rate);

	channels = params_channels(params);
	pr_info("[ info][%s] %s: i2s channel: %u !!! \r\n",
		LOG_FLAG, __func__, channels);
	if (channels < CHANNEL_NUM_MIN || channels > CHANNEL_NUM_MAX) {
		pr_err("[  err][%s] %s: i2s channel not match !!! \r\n",
			LOG_FLAG, __func__);
		return -SIPA_ERROR_I2S_UNMATCH;
	}

	width = snd_pcm_format_physical_width(params_format(params));
	pr_info("[ info][%s] %s: i2s width: %u !!! \r\n",
		LOG_FLAG, __func__, width);

	pr_debug("[debug][%s] %s: stream = %d, requested rate = %d, sample size = %d,"
						"physical size = %d, channel num = %d\n",
					LOG_FLAG, __func__, substream->stream, rate,
					snd_pcm_format_width(params_format(params)), width, channels);
	return 0;
}

int sia91xx_smartpa_soft_mute(sipa_dev_t *si_pa)
{
	if (sipa_regmap_set_chip_off(si_pa))
		return 0;

	return -1;
}

int sia91xx_smartpa_start(sipa_dev_t *si_pa, int stream)
{
	uint8_t start_count = 0;

	while (start_count < SIA91XX_MAX_PA_START_TRY_COUNT) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK
			&& sipa_regmap_set_chip_on(si_pa)) {
			break;
		}
		start_count++;
	}

	if (start_count >= SIA91XX_MAX_PA_START_TRY_COUNT &&
			stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_err("[  err][%s] %s: Failed starting device \n",LOG_FLAG, __func__);
		return -SIPA_ERROR_DEV_START;
	} else {
		pr_debug("[debug][%s] %s: device start success! \n", LOG_FLAG, __func__);
	}
	return 0;
}

int sia91xx_startup(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 17, 19))
	struct snd_soc_component *component = dai->component;
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = dai->codec;
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	if (SNDRV_PCM_STREAM_CAPTURE == substream->stream) {
		return 0;
	}

	pr_debug("[debug][%s] %s: dai:%s, substream:%s, startup, stream = %d \r\n",
			LOG_FLAG, __func__, dai->name, substream->name, substream->stream);

	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
		gpio_set_value(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
		usleep_range(5000, 6000);
	} else {
		if (SIA91XX_HIGH_LEVEL == gpio_get_value(si_pa->rst_pin)) {
			gpio_set_value(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
			usleep_range(5000, 6000);
		}
	}

	if (si_pa->mute == SIPA_DEVICE_MUTE_ON) {
		pr_info("[debug][%s] %s: pa is mute on, direct return!\n",LOG_FLAG, __func__);
		return 0;
	}

#if SIPA_DRV_CHANGE_I2C_I2S_TIME_SEQ
	if (si_pa->power_mode == true) {
		pr_info("[debug][%s] %s: power_mode is true, direct return!\r\n",
			LOG_FLAG, __func__);
		return 0;
	}
	//at the startup intf, I2C starts earlier than I2S.
	sipa_reg_init(si_pa);
	if (sia91xx_smartpa_start(si_pa, substream->stream))
		return -SIPA_ERROR_SOFT_MUTE;
	sipa_regmap_check_trimming(si_pa);
	sipa_regmap_write_sram(si_pa);
	si_pa->power_mode = true;
#endif
	return 0;
}

int sia91xx_mute(
	struct snd_soc_dai *dai,
	int mute,
	int stream)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 17, 19))
	struct snd_soc_component *component = dai->component;
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = dai->codec;
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	pr_debug("[debug][%s] %s: dai:%s,%s mute = %d stream = %d\n",
		LOG_FLAG, __func__, dai->name, dai->driver->name, mute, stream);

	if (si_pa->mute == SIPA_DEVICE_MUTE_ON) {
		pr_info("[debug][%s] %s: pa is mute on, direct return!\n",LOG_FLAG, __func__);
		return 0;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (mute) {
			//at the mute intf, I2S ends earlier than I2C.
			if(si_pa->power_mode == false) {
				pr_info("[debug][%s] %s: power_mode is false, direct return!\n",
						LOG_FLAG, __func__);
				return 0;
			}
			si_pa->power_mode = false;
			if (0 != sia91xx_smartpa_soft_mute(si_pa)) {
				if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
					gpio_set_value(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
				} else {
					gpio_set_value(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
				}
				usleep_range(5000, 6000);
				pr_err("[  err][%s] %s: Failed mute smartpa \n", LOG_FLAG, __func__);
				return -SIPA_ERROR_SOFT_MUTE;
			}
			pr_debug("[debug][%s] %s: device mute success \n", LOG_FLAG, __func__);
			// if multiple pa share rst, direct operation on the rst is not allowed.
			// if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
			// 	gpio_set_value(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
			// } else {
			// 	gpio_set_value(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
			// }
		} else {
#if !SIPA_DRV_CHANGE_I2C_I2S_TIME_SEQ
			if (si_pa->power_mode == true) {
				pr_info("[debug][%s] %s: power_mode is true, direct return!\n",
							LOG_FLAG, __func__);
				return 0;
			}

			//at the unmute intf, I2S starts earlier than I2C.
			sipa_reg_init(si_pa);
			if (sia91xx_smartpa_start(si_pa, stream))
				return -SIPA_ERROR_SOFT_MUTE;

			sipa_regmap_check_trimming(si_pa);
			sipa_regmap_write_sram(si_pa);
			si_pa->power_mode = true;
#endif
		}
	}
	return 0;
}

int sia91xx_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	pr_debug("[debug][%s] %s: fmt = 0x%x\n", LOG_FLAG, __func__, fmt);

	/* Supported mode: regular I2S, slave, or PDM */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) != SND_SOC_DAIFMT_CBS_CFS) {
			pr_err("[  err][%s] %s: Invalid Codec master mode\n",LOG_FLAG, __func__);
			return -EINVAL;
		}
		break;
	default:
		pr_err("[  err][%s] %s: Unsupported DAI format %d \n",
				LOG_FLAG, __func__, fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
static struct snd_soc_dapm_context *snd_soc_codec_get_dapm(
	struct snd_soc_codec *codec)
{
	return &codec->dapm;
}
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
int sia91xx_component_probe(struct snd_soc_component *component)
{
	struct snd_soc_dapm_widget *widgets;
	struct snd_soc_dapm_context *dapm;
	unsigned int num_dapm_widgets = ARRAY_SIZE(sia91xx_dapm_widgets_common);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	si_pa->codec = component;
	dapm = snd_soc_component_get_dapm(component);
	sia91xx_register_interrupt(si_pa);
	widgets = devm_kzalloc(&si_pa->client->dev,
							sizeof(struct snd_soc_dapm_widget) *
							ARRAY_SIZE(sia91xx_dapm_widgets_common),
							GFP_KERNEL);
	if (!widgets)
			return 0;

	memcpy(widgets, sia91xx_dapm_widgets_common,
					sizeof(struct snd_soc_dapm_widget) *
							ARRAY_SIZE(sia91xx_dapm_widgets_common));

	sia91xx_append_i2c_address(&si_pa->client->dev,
					si_pa->client,
					widgets,
					num_dapm_widgets,
					NULL,
					0);

	snd_soc_dapm_new_controls(dapm, widgets,
					ARRAY_SIZE(sia91xx_dapm_widgets_common));
	snd_soc_dapm_add_routes(dapm, sia91xx_dapm_routes_common,
					ARRAY_SIZE(sia91xx_dapm_routes_common));
	return 0;
}

#else

int sia91xx_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_widget *widgets;
	struct snd_soc_dapm_context *dapm;
	unsigned int num_dapm_widgets = ARRAY_SIZE(sia91xx_dapm_widgets_common);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	si_pa->codec = codec;
	dapm = snd_soc_codec_get_dapm(codec);
	sia91xx_register_interrupt(si_pa);
	widgets = devm_kzalloc(&si_pa->client->dev,
				sizeof(struct snd_soc_dapm_widget) *
				ARRAY_SIZE(sia91xx_dapm_widgets_common),
				GFP_KERNEL);
	if (!widgets)
		return 0;

	memcpy(widgets, sia91xx_dapm_widgets_common,
			sizeof(struct snd_soc_dapm_widget) *
				ARRAY_SIZE(sia91xx_dapm_widgets_common));

	sia91xx_append_i2c_address(&si_pa->client->dev,
			si_pa->client,
			widgets,
			num_dapm_widgets,
			NULL,
			0);

	snd_soc_dapm_new_controls(dapm, widgets,
			ARRAY_SIZE(sia91xx_dapm_widgets_common));
	snd_soc_dapm_add_routes(dapm, sia91xx_dapm_routes_common,
			ARRAY_SIZE(sia91xx_dapm_routes_common));
	return 0;
}
#endif
