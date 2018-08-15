// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2018 MediaTek Inc.

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <sound/soc.h>

#include "../audio_dsp/mtk-dsp-platform-driver.h"
#include "../audio_dsp/mtk-base-dsp.h"

#include "mtk-dsp-common.h"
#include "audio_mem_control.h"

#define MTK_PCM_RATES (SNDRV_PCM_RATE_8000_48000 |\
		       SNDRV_PCM_RATE_88200 |\
		       SNDRV_PCM_RATE_96000 |\
		       SNDRV_PCM_RATE_176400 |\
		       SNDRV_PCM_RATE_192000)

#define MTK_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S24_LE |\
			 SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_pcm_hardware audio_dsp_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_NO_PERIOD_WAKEUP |
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_RESUME |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE |
		    SNDRV_PCM_FMTBIT_S24_LE |
		    SNDRV_PCM_FMTBIT_S32_LE),
	.rates = MTK_PCM_RATES,
	.period_bytes_min = 256,
	.period_bytes_max = 4 * 48 * 1024,
	.periods_min = 2,
	.periods_max = 4096,
	.buffer_bytes_max = 8 * 48 * 1024,
	.fifo_size = 0,
};

static int dsp_runtime_suspend(struct device *dev)
{
	return 0;
}

static int dsp_runtime_resume(struct device *dev)
{
	return 0;
}

static int dsp_pcm_dev_probe(struct platform_device *pdev)
{
	struct mtk_base_dsp *dsp;
	int ret = 0;
	struct device *dev;

	dsp = devm_kzalloc(&pdev->dev, sizeof(struct mtk_base_dsp), GFP_KERNEL);
	if (!dsp)
		return -ENOMEM;

	/*  register dsp dai driver*/
	dai_dsp_register(pdev, dsp);
	dsp->mtk_dsp_hardware = &audio_dsp_hardware;

	dsp->dev = &pdev->dev;
	dev = dsp->dev;

	dsp->runtime_resume = dsp_runtime_resume;
	dsp->runtime_suspend = dsp_runtime_suspend;

	dsp->request_dram_resource = dsp_dram_request;
	dsp->release_dram_resource = dsp_dram_release;

	platform_set_drvdata(pdev, dsp);
	pm_runtime_enable(&pdev->dev);

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", "snd_audio_dsp");

	ret = snd_soc_register_platform(&pdev->dev, &mtk_dsp_pcm_platform);
	if (ret) {
		dev_warn(&pdev->dev, "err_platform\n");
		goto err_platform;
	}
	ret = devm_snd_soc_register_component(&pdev->dev,
					      dsp->component_driver,
					      dsp->dai_drivers,
					      dsp->num_dai_drivers);
	init_mtk_adsp_dram_segment();
	set_ipi_recv_private((void *)dsp);
	set_dsp_base((void *)dsp);
	return 0;

err_platform:
	pr_err("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);

	return ret;
}

static int dsp_pcm_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dsp_pcm_dt_match[] = {
	{ .compatible = "mediatek,snd_audio_dsp", },
	{},
};
MODULE_DEVICE_TABLE(of, dsp_pcm_dt_match);

static const struct dev_pm_ops audio_dsp_pm_ops = {
	SET_RUNTIME_PM_OPS(dsp_runtime_suspend,
			   dsp_runtime_resume, NULL)
};

static struct platform_driver dsp_pcm_driver = {
	.driver = {
		   .name = "snd_audio_dsp",
		   .owner = THIS_MODULE,
		   .of_match_table = dsp_pcm_dt_match,
#ifdef CONFIG_PM
		   .pm = &audio_dsp_pm_ops,
#endif
	},
	.probe = dsp_pcm_dev_probe,
	.remove = dsp_pcm_dev_remove,
};

module_platform_driver(dsp_pcm_driver);

MODULE_DESCRIPTION("Mediatek ALSA SoC dsp platform driver for audio dsp");
MODULE_AUTHOR("Chipeng Chang <Chipeng.Chang@mediatek.com>");
MODULE_LICENSE("GPL v2");
