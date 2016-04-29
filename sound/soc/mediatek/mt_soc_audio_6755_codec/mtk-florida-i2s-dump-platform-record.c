/*
 * cpu-i2s.c  --  ALSA Soc Audio Layer dump platform
 *
 * (c) 2014 Wolfson Microelectronics PLC.
 * karl.sun@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/pcm.h>

#include <sound/core.h>
#include "mt_soc_pcm_common.h"

/*
 * Set dummy I2S DAI format
 */
static unsigned int cpu_i2s_record_audio_ap_supported_high_sample_rates[] =
{
    8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
    96000, 192000
};
static struct snd_pcm_hw_constraint_list cpu_i2s_record_constraints_sample_rates =
{
    .count = ARRAY_SIZE(cpu_i2s_record_audio_ap_supported_high_sample_rates),
    .list = cpu_i2s_record_audio_ap_supported_high_sample_rates,
    .mask = 0,
};
static int cpu_i2s2_record_startup(struct snd_pcm_substream *substream,
                              struct snd_soc_dai *dai)
{
    printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
    snd_pcm_hw_constraint_list(substream->runtime, 0,
                               SNDRV_PCM_HW_PARAM_RATE,
                               &cpu_i2s_record_constraints_sample_rates);
    return 0;
}
static int cpu_i2s2_record_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{

	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s2_record_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{

	printk(KERN_ERR "WM florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s2_record_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	int ret = 0;
	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return ret;
}

/*
 * Set dummy Clock source
 */
static int cpu_i2s2_record_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{

	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

/*
 * Set dummy Clock dividers
 */
static int cpu_i2s2_record_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	return 0;
}

static int cpu_i2s2_record_probe(struct snd_soc_dai *dai)
{
	printk(KERN_ERR "Florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}

#ifdef CONFIG_PM
static int cpu_i2s2_record_suspend(struct snd_soc_dai *cpu_dai)
{
	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}

static int cpu_i2s2_record_resume(struct snd_soc_dai *cpu_dai)
{
	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	return 0;
}
#else
#define cpu_i2s2_record_suspend NULL
#define cpu_i2s2_record_resume NULL
#endif


#define CPU_I2S2_RECORD_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

static const struct snd_soc_dai_ops cpu_i2s2_record_dai_ops = {
	.startup    = cpu_i2s2_record_startup,
	.trigger	= cpu_i2s2_record_trigger,
	.hw_params	= cpu_i2s2_record_hw_params,
	.set_fmt	= cpu_i2s2_record_set_fmt,
	.set_clkdiv	= cpu_i2s2_record_set_clkdiv,
	.set_sysclk	= cpu_i2s2_record_set_sysclk,
};

static struct snd_soc_dai_driver cpu_i2s2_record_dai = {
	.name = "mtk-florida-i2s-record.0",
	.probe = cpu_i2s2_record_probe,
	.suspend = cpu_i2s2_record_suspend,
	.resume = cpu_i2s2_record_resume,
	.playback = {
		.stream_name = MT_SOC_DUMMY_I2S_RECORD_STREAM_PCM,
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.rate_min = 8000,
                .rate_max = 192000,
		.formats = (SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
            		    SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_S16_LE |
                            SNDRV_PCM_FMTBIT_U16_BE | SNDRV_PCM_FMTBIT_S16_BE |
            		    SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE |
                            SNDRV_PCM_FMTBIT_U24_BE | SNDRV_PCM_FMTBIT_S24_BE |
                            SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_S24_3LE |
                            SNDRV_PCM_FMTBIT_U24_3BE | SNDRV_PCM_FMTBIT_S24_3BE |
                            SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_S32_LE |
                            SNDRV_PCM_FMTBIT_U32_BE | SNDRV_PCM_FMTBIT_S32_BE),
                },
	.capture = {
		.stream_name = MT_SOC_DUMMY_I2S_RECORD_STREAM_PCM,
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
	        .rate_min = 8000,
                .rate_max = 192000,
		.formats = (SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S8 |
            		    SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_S16_LE |
                            SNDRV_PCM_FMTBIT_U16_BE | SNDRV_PCM_FMTBIT_S16_BE |
            		    SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE |
                            SNDRV_PCM_FMTBIT_U24_BE | SNDRV_PCM_FMTBIT_S24_BE |
                            SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_S24_3LE |
                            SNDRV_PCM_FMTBIT_U24_3BE | SNDRV_PCM_FMTBIT_S24_3BE |
                            SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_S32_LE |
                            SNDRV_PCM_FMTBIT_U32_BE | SNDRV_PCM_FMTBIT_S32_BE),
		},
	.ops = &cpu_i2s2_record_dai_ops,
};

static const struct snd_soc_component_driver mt_dai_component_record = {
	.name = "mtk-florida-i2s-record"
};
static int cpu_iis2_record_dev_probe(struct platform_device *pdev)
{

	int rc = 0;

	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);

	rc = snd_soc_register_component(&pdev->dev, &mt_dai_component_record, &cpu_i2s2_record_dai,
					1);

	return rc;
}

static int cpu_iis2_record_dev_remove(struct platform_device *pdev)
{
	printk(KERN_ERR "florida-I2S(%d) >>>>: %s \n", __LINE__,__func__);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver mtk_florida_i2s2_record_driver = {
	.probe  = cpu_iis2_record_dev_probe,
	.remove = cpu_iis2_record_dev_remove,
	.driver = {
		.name = "mtk-florida-i2s-record",
		.owner = THIS_MODULE,
	},
};

static int __init mtk_florida_i2s2_record_dai_init(void)
{
    printk("%s:\n", __func__);
    return platform_driver_register(&mtk_florida_i2s2_record_driver);
}
module_init(mtk_florida_i2s2_record_dai_init);

static void __exit mtk_florida_i2s2_record_dai_exit(void)
{
    printk("%s:\n", __func__);

    platform_driver_unregister(&mtk_florida_i2s2_record_driver);
}

module_exit(mtk_florida_i2s2_record_dai_exit);

/* Module information */
MODULE_AUTHOR("karl.sun karl.sun@wolfsonmicro.com>");
MODULE_DESCRIPTION("cpu I2S sedcod record SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cpu-iis2");
