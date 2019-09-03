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
#include <linux/atomic.h>

static atomic_t cs35l41_mclk_rsc_ref;

int cs35l41_snd_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_codec *spk_cdc = rtd->codec_dais[0]->codec;
	struct snd_soc_dapm_context *spk_dapm = snd_soc_codec_get_dapm(spk_cdc);
#if defined(CONFIG_SND_SOC_DUAL_CS35L41)
	struct snd_soc_codec *rcv_cdc = rtd->codec_dais[1]->codec;
	struct snd_soc_dapm_context *rcv_dapm = snd_soc_codec_get_dapm(rcv_cdc);
#endif
	dev_info(card->dev, "%s: set cs35l41_mclk_rsc_ref to 0 \n", __func__);
	atomic_set(&cs35l41_mclk_rsc_ref, 0);

	dev_info(card->dev, "%s: found codec[%s]\n", __func__, dev_name(spk_cdc->dev));
	snd_soc_dapm_ignore_suspend(spk_dapm, "SPK AMP Playback");
	snd_soc_dapm_ignore_suspend(spk_dapm, "SPK SPK");
	snd_soc_dapm_sync(spk_dapm);
#if defined(CONFIG_SND_SOC_DUAL_CS35L41)
	dev_info(card->dev, "%s: found codec[%s]\n", __func__, dev_name(rcv_cdc->dev));
	snd_soc_dapm_ignore_suspend(rcv_dapm, "RCV AMP Playback");
	snd_soc_dapm_ignore_suspend(rcv_dapm, "RCV SPK");
	snd_soc_dapm_sync(rcv_dapm);
#endif
	return 0;
}

static int cirrus_amp_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int cirrus_amp_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai **codec_dais = rtd->codec_dais;
	int ret, i;
	unsigned int clk_freq;

	dev_info(card->dev, "+%s, mclk refcount = %d\n", __func__,
		atomic_read(&cs35l41_mclk_rsc_ref));

	if (atomic_inc_return(&cs35l41_mclk_rsc_ref) == 1) {
		clk_freq = params_rate(params) *
		params_channels(params) *
		params_width(params);
	
		for (i = 0; i < rtd->num_codecs; i++) {
			ret = snd_soc_dai_set_fmt(codec_dais[i],
							SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S);
			if (ret < 0) {
				dev_err(card->dev, "%s: Failed to set fmt codec dai: %d\n",
					__func__, ret);
				return ret;
			}

			ret = snd_soc_dai_set_sysclk(codec_dais[i], 0,
							clk_freq,
							SND_SOC_CLOCK_IN);
			if (ret < 0) {
				dev_err(card->dev, "%s: Failed to set dai sysclk: %d\n",
						__func__, ret);
				return ret;
			}

			ret = snd_soc_codec_set_sysclk(codec_dais[i]->codec, 0, 0,
							clk_freq,
							SND_SOC_CLOCK_IN);
			if (ret < 0) {
				dev_err(card->dev, "%s: Failed to set codec sysclk: %d\n",
						__func__, ret);
				return ret;
			}
		}
	}
	return ret;
}

void cirrus_amp_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "+%s, mclk refcount = %d \n", __func__,
		atomic_read(&cs35l41_mclk_rsc_ref));

	if (atomic_read(&cs35l41_mclk_rsc_ref) > 0) {
		atomic_dec_return(&cs35l41_mclk_rsc_ref);
	}
}

const struct snd_soc_ops cirrus_amp_ops = {
	.startup = cirrus_amp_startup,
	.hw_params = cirrus_amp_hw_params,
	.shutdown = cirrus_amp_shutdown,
};
EXPORT_SYMBOL_GPL(cirrus_amp_ops);


