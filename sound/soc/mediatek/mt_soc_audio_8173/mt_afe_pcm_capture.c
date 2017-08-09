/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#include "mt_afe_def.h"
#include "mt_afe_reg.h"
#include "mt_afe_clk.h"
#include "mt_afe_control.h"
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <sound/soc.h>


/*
 *    function implementation
 */
static int mt_pcm_capture_close(struct snd_pcm_substream *substream);

static void mt_pcm_capture_start_audio_hw(struct snd_pcm_substream *substream)
{
	struct mt_afe_irq_status irq_status;
	struct timespec curr_tstamp;

	pr_debug("%s\n", __func__);

	mt_afe_set_i2s_adc_in(substream->runtime->rate);

	mt_afe_set_memif_fetch_format(MT_AFE_DIGITAL_BLOCK_MEM_VUL, MT_AFE_MEMIF_16_BIT);
	mt_afe_set_out_conn_format(MT_AFE_CONN_OUTPUT_16BIT, INTER_CONN_O09);
	mt_afe_set_out_conn_format(MT_AFE_CONN_OUTPUT_16BIT, INTER_CONN_O10);

	if (mt_afe_get_memory_path_state(MT_AFE_DIGITAL_BLOCK_I2S_IN_ADC) == false) {
		mt_afe_enable_memory_path(MT_AFE_DIGITAL_BLOCK_I2S_IN_ADC);
		mt_afe_enable_i2s_adc();
	} else {
		mt_afe_enable_memory_path(MT_AFE_DIGITAL_BLOCK_I2S_IN_ADC);
	}

	mt_afe_set_sample_rate(MT_AFE_DIGITAL_BLOCK_MEM_VUL, substream->runtime->rate);
	mt_afe_set_channels(MT_AFE_DIGITAL_BLOCK_MEM_VUL, substream->runtime->channels);
	mt_afe_enable_memory_path(MT_AFE_DIGITAL_BLOCK_MEM_VUL);

	/* here to set interrupt */
	mt_afe_get_irq_state(MT_AFE_IRQ_MCU_MODE_IRQ2, &irq_status);
	if (likely(!irq_status.status)) {
		mt_afe_set_irq_counter(MT_AFE_IRQ_MCU_MODE_IRQ2, substream->runtime->period_size);
		mt_afe_set_irq_rate(MT_AFE_IRQ_MCU_MODE_IRQ2, substream->runtime->rate);
		mt_afe_set_irq_state(MT_AFE_IRQ_MCU_MODE_IRQ2, true);
		snd_pcm_gettime(substream->runtime, (struct timespec *)&curr_tstamp);
		pr_debug("%s curr_tstamp %ld %ld\n", __func__, curr_tstamp.tv_sec,
			 curr_tstamp.tv_nsec);

	} else {
		pr_debug("%s IRQ2_MCU_MODE is enabled, use original irq2 interrupt mode\n",
			 __func__);
	}
	/* here to turn off digital part */
	mt_afe_set_connection(INTER_CONNECT, INTER_CONN_I03, INTER_CONN_O09);
	mt_afe_set_connection(INTER_CONNECT, INTER_CONN_I04, INTER_CONN_O10);

	mt_afe_enable_afe(true);
}

static void mt_pcm_capture_stop_audio_hw(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);

	mt_afe_disable_memory_path(MT_AFE_DIGITAL_BLOCK_I2S_IN_ADC);
	if (mt_afe_get_memory_path_state(MT_AFE_DIGITAL_BLOCK_I2S_IN_ADC) == false)
		mt_afe_disable_i2s_adc();

	mt_afe_disable_memory_path(MT_AFE_DIGITAL_BLOCK_MEM_VUL);

	/* here to set interrupt */
	mt_afe_set_irq_state(MT_AFE_IRQ_MCU_MODE_IRQ2, false);

	/* here to turn off digital part */
	mt_afe_set_connection(INTER_DISCONNECT, INTER_CONN_I03, INTER_CONN_O09);
	mt_afe_set_connection(INTER_DISCONNECT, INTER_CONN_I04, INTER_CONN_O10);

	mt_afe_enable_afe(false);
}

static struct snd_pcm_hardware mt_pcm_capture_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SOC_NORMAL_USE_RATE,
	.rate_min = SOC_NORMAL_USE_RATE_MIN,
	.rate_max = SOC_NORMAL_USE_RATE_MAX,
	.channels_min = SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max = SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = UL1_MAX_BUFFER_SIZE,
	.period_bytes_min = SOC_NORMAL_USE_PERIOD_SIZE_MIN,
	.period_bytes_max = (UL1_MAX_BUFFER_SIZE / SOC_NORMAL_USE_PERIODS_MIN),
	.periods_min = SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max = SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size = 0,
};

static struct snd_pcm_hw_constraint_list mt_pcm_capture_constraints_rates = {
	.count = ARRAY_SIZE(soc_normal_supported_sample_rates),
	.list = soc_normal_supported_sample_rates,
};

static int mt_pcm_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	pr_debug("%s\n", __func__);

	snd_soc_set_runtime_hwparams(substream, &mt_pcm_capture_hardware);

	pr_debug("%s runtime->hw->rates = 0x%x mt_pcm_capture_hardware = %p\n ",
		 __func__, runtime->hw.rates, &mt_pcm_capture_hardware);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &mt_pcm_capture_constraints_rates);
	if (unlikely(ret < 0))
		pr_err("%s snd_pcm_hw_constraint_list failed %d\n", __func__, ret);

	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (unlikely(ret < 0))
		pr_err("%s snd_pcm_hw_constraint_integer failed %d\n", __func__, ret);

	/* here open audio clocks */
	mt_afe_main_clk_on();
	mt_afe_adc_clk_on();
	mt_afe_emi_clk_on();

	/* print for hw pcm information */
	pr_info("%s device = %d\n", __func__, substream->pcm->device);

	if (unlikely(ret < 0)) {
		pr_err("%s mt_pcm_capture_close\n", __func__);
		mt_pcm_capture_close(substream);
		return ret;
	}

	pr_debug("%s return\n", __func__);
	return 0;
}

static int mt_pcm_capture_close(struct snd_pcm_substream *substream)
{
	pr_info("%s\n", __func__);

	mt_afe_adc_clk_off();
	mt_afe_main_clk_off();
	mt_afe_emi_clk_off();

	return 0;
}

static int mt_pcm_capture_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	pr_debug("%s\n", __func__);

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));

	if (likely(ret >= 0))
		mt_afe_init_dma_buffer(MT_AFE_MEM_CTX_VUL, runtime);
	else
		pr_err("%s snd_pcm_lib_malloc_pages fail %d\n", __func__, ret);

	pr_debug("%s dma_bytes = %zu dma_area = %p dma_addr = 0x%llx\n",
		 __func__, runtime->dma_bytes, runtime->dma_area,
		 (unsigned long long)runtime->dma_addr);
	return ret;
}

static int mt_pcm_capture_hw_free(struct snd_pcm_substream *substream)
{
	pr_info("%s\n", __func__);
	return snd_pcm_lib_free_pages(substream);
}

static int mt_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}

static int mt_pcm_capture_start(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);
	mt_afe_add_ctx_substream(MT_AFE_MEM_CTX_VUL, substream);
	mt_pcm_capture_start_audio_hw(substream);
	return 0;
}

static int mt_pcm_capture_stop(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);
	mt_pcm_capture_stop_audio_hw(substream);
	mt_afe_reset_dma_buffer(MT_AFE_MEM_CTX_VUL);
	mt_afe_remove_ctx_substream(MT_AFE_MEM_CTX_VUL);
	return 0;
}

static int mt_pcm_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
	pr_info("%s cmd=%d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mt_pcm_capture_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mt_pcm_capture_stop(substream);
	}
	return -EINVAL;
}

static snd_pcm_uframes_t mt_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
	return mt_afe_update_hw_ptr(MT_AFE_MEM_CTX_VUL);
}

static struct snd_pcm_ops mt_pcm_capture_ops = {
	.open = mt_pcm_capture_open,
	.close = mt_pcm_capture_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mt_pcm_capture_hw_params,
	.hw_free = mt_pcm_capture_hw_free,
	.prepare = mt_pcm_capture_prepare,
	.trigger = mt_pcm_capture_trigger,
	.pointer = mt_pcm_capture_pointer,
};

static int mt_pcm_capture_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	ret = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, card->dev,
						    UL1_MAX_BUFFER_SIZE, UL1_MAX_BUFFER_SIZE);
	return ret;
}

static void mt_pcm_capture_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static struct snd_soc_platform_driver mt_pcm_capture_platform = {
	.ops = &mt_pcm_capture_ops,
	.pcm_new = mt_pcm_capture_pcm_new,
	.pcm_free = mt_pcm_capture_pcm_free,
};

static int mt_pcm_capture_dev_probe(struct platform_device *pdev)
{
	pr_notice("%s dev name %s\n", __func__, dev_name(&pdev->dev));
	if (pdev->dev.of_node) {
		dev_set_name(&pdev->dev, "%s", MT_SOC_UL1_PCM);
		pr_notice("%s set dev name %s\n", __func__, dev_name(&pdev->dev));
	}
	return snd_soc_register_platform(&pdev->dev, &mt_pcm_capture_platform);
}

static int mt_pcm_capture_dev_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id mt_pcm_capture_dt_match[] = {
	{.compatible = "mediatek," MT_SOC_UL1_PCM,},
	{}
};

MODULE_DEVICE_TABLE(of, mt_pcm_capture_dt_match);

static struct platform_driver mt_pcm_capture_driver = {
	.driver = {
		   .name = MT_SOC_UL1_PCM,
		   .owner = THIS_MODULE,
		   .of_match_table = mt_pcm_capture_dt_match,
		   },
	.probe = mt_pcm_capture_dev_probe,
	.remove = mt_pcm_capture_dev_remove,
};

module_platform_driver(mt_pcm_capture_driver);

MODULE_DESCRIPTION("AFE PCM Capture platform driver");
MODULE_LICENSE("GPL");
