/*
* Copyright (C) 2019 Cirrus Logic Inc.
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
 *   mtk-cirrus-machine-ops.c
 *
 * Project:
 * --------
 *   Audio soc machine vendor ops
 *
 * Description:
 * ------------
 *   Audio machine driver
 *
 * Author:
 * -------
 * Vlad Karpovich
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

static int cirrus_amp_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int cirrus_amp_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd[0].codec;
	struct snd_soc_dai *codec_dai = rtd[0].codec_dai;
	struct snd_soc_dai_link *dai_link = rtd[0].dai_link;
	int ret;
	unsigned int clk_freq;

	clk_freq = params_rate(params) *
		   params_channels(params) *
		   params_width(params);

	ret = snd_soc_dai_set_fmt(codec_dai, dai_link->dai_fmt);
	if (ret != 0) {
		dev_err(codec_dai->dev, "Failed to set %s's dai_fmt: ret = %d",
			codec_dai->name, ret);
		return ret;
	}
		dev_err(codec_dai->dev, "MTK - cs35l41 :Set %s's clock %d\n",
			codec_dai->name,clk_freq);
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, clk_freq, SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(codec_dai->dev, "Failed to set %s's clock: ret = %d\n",
			codec_dai->name, ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, 0, 0, clk_freq, SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(codec_dai->dev, "Failed to set %s's clock: ret = %d\n",
			codec_dai->name, ret);
		return ret;
	}
	return ret;
}

static int cirrus_amp_free(struct snd_pcm_substream *substream)
{
	return 0;
}

const struct snd_soc_ops cirrus_amp_ops = {
	.startup = cirrus_amp_startup,
	.hw_params = cirrus_amp_hw_params,
	.hw_free = cirrus_amp_free,
};
EXPORT_SYMBOL_GPL(cirrus_amp_ops);


