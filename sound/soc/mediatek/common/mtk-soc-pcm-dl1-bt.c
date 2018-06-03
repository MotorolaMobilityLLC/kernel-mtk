/*
* Copyright (C) 2015 MediaTek Inc.
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
 *   mt_soc_dl1_bt.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio dl1 data1 playback
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include <linux/dma-mapping.h>
#include "mtk-auddrv-common.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-clk.h"
#include "mtk-auddrv-kernel.h"
#include "mtk-soc-afe-control.h"
#include "mtk-soc-pcm-platform.h"
#include "mtk-soc-analog-type.h"

static AFE_MEM_CONTROL_T *pdl1btMemControl;
static struct snd_dma_buffer *dl1bt_Playback_dma_buf;
static unsigned int mPlaybackDramState;
static struct device *mDev;

/*
 *    function implementation
 */

static int mtk_dl1bt_probe(struct platform_device *pdev);
static int mtk_Dl1Bt_close(struct snd_pcm_substream *substream);
static int mtk_asoc_Dl1Bt_pcm_new(struct snd_soc_pcm_runtime *rtd);
static int mtk_asoc_dl1bt_probe(struct snd_soc_platform *platform);

static struct snd_pcm_hardware mtk_dl1bt_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
	SNDRV_PCM_INFO_INTERLEAVED |
	SNDRV_PCM_INFO_RESUME |
	SNDRV_PCM_INFO_MMAP_VALID),
	.formats =      SND_SOC_ADV_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = Dl1_MAX_BUFFER_SIZE,
	.period_bytes_max = MAX_PERIOD_SIZE,
	.periods_min =      SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max =      SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size =        0,
};

static int mtk_pcm_dl1Bt_stop(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("mtk_pcm_dl1Bt_stop\n");

	/* here to turn off digital part */
	SetIntfConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, false);

	irq_remove_user(substream, Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE);

	SetMemoryPathEnable(Soc_Aud_Digital_Block_DAI_BT, false);

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_DAI_BT) == false)
		SetDaiBtEnable(false);

	EnableAfe(false);
	RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

	return 0;
}

static snd_pcm_uframes_t mtk_dl1bt_pcm_pointer(struct snd_pcm_substream *substream)
{
	return get_mem_frame_index(substream,
		pdl1btMemControl, Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_dl1bt_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	PRINTK_AUDDRV("mtk_pcm_dl1bt_hw_params\n");

	/* runtime->dma_bytes has to be set manually to allow mmap */
	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);

	if (AllocateAudioSram(&substream->runtime->dma_addr,	 &substream->runtime->dma_area,
		substream->runtime->dma_bytes, substream) == 0) {
		AudDrv_Allocate_DL1_Buffer(mDev, substream->runtime->dma_bytes,
			substream->runtime->dma_addr, substream->runtime->dma_area);
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, false, substream->runtime->dma_addr);
		/* pr_warn("mtk_pcm_hw_params dma_bytes = %d\n",substream->runtime->dma_bytes); */
	} else {
		substream->runtime->dma_area = dl1bt_Playback_dma_buf->area;
		substream->runtime->dma_addr = dl1bt_Playback_dma_buf->addr;
		SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, true, substream->runtime->dma_addr);
		set_mem_block(substream, hw_params,
			pdl1btMemControl, Soc_Aud_Digital_Block_MEM_DL1);
		mPlaybackDramState = true;
		AudDrv_Emi_Clk_On();
	}

	PRINTK_AUDDRV(" dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
		      substream->runtime->dma_bytes, substream->runtime->dma_area, (long)substream->runtime->dma_addr);
	return ret;
}

static int mtk_pcm_dl1bt_hw_free(struct snd_pcm_substream *substream)
{
	pr_warn("%s substream = %p\n", __func__, substream);
	if (mPlaybackDramState == true) {
		AudDrv_Emi_Clk_Off();
		mPlaybackDramState = false;
	} else
		freeAudioSram((void *)substream);
	return 0;
}

static struct snd_pcm_hw_constraint_list constraints_dl1_sample_rates = {
	.count = ARRAY_SIZE(soc_voice_supported_sample_rates),
	.list = soc_voice_supported_sample_rates,
	.mask = 0,
};

static int mtk_dl1bt_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;

	mPlaybackDramState = false;
	mtk_dl1bt_pcm_hardware.buffer_bytes_max = GetPLaybackSramFullSize();
	AudDrv_Clk_On();

	/* get dl1 memconptrol and record substream */
	pdl1btMemControl = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_DL1);
	runtime->hw = mtk_dl1bt_pcm_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_dl1bt_pcm_hardware, sizeof(struct snd_pcm_hardware));

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_dl1_sample_rates);
	if (ret < 0)
		PRINTK_AUDDRV("snd_pcm_hw_constraint_list failed\n");

	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (ret < 0)
		PRINTK_AUDDRV("snd_pcm_hw_constraint_integer failed\n");

	/* print for hw pcm information */
	PRINTK_AUDDRV("mtk_dl1bt_pcm_open runtime rate = %d channels = %d substream->pcm->device = %d\n",
		      runtime->rate, runtime->channels, substream->pcm->device);

	if (ret < 0) {
		PRINTK_AUDDRV("mtk_Dl1Bt_close\n");
		mtk_Dl1Bt_close(substream);
		return ret;
	}
	return 0;
}

static int mtk_Dl1Bt_close(struct snd_pcm_substream *substream)
{
	PRINTK_AUDDRV("%s\n", __func__);
	AudDrv_Clk_Off();
	return 0;
}

static int mtk_dl1bt_pcm_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}

static bool SetVoipDAIBTAttribute(int sample_rate)
{
	AudioDigitalDAIBT daibt_attribute;

	memset_io((void *)&daibt_attribute, 0, sizeof(daibt_attribute));

#if 0 /* temp for merge only support */
	daibt_attribute.mUSE_MRGIF_INPUT = Soc_Aud_BT_DAI_INPUT_FROM_BT;
#else
	daibt_attribute.mUSE_MRGIF_INPUT = Soc_Aud_BT_DAI_INPUT_FROM_MGRIF;
#endif
	daibt_attribute.mDAI_BT_MODE = (sample_rate == 8000) ? Soc_Aud_DATBT_MODE_Mode8K : Soc_Aud_DATBT_MODE_Mode16K;
	daibt_attribute.mDAI_DEL = Soc_Aud_DAI_DEL_HighWord; /* suggest always HighWord */
	daibt_attribute.mBT_LEN  = 0;
	daibt_attribute.mDATA_RDY = true;
	daibt_attribute.mBT_SYNC = Soc_Aud_BTSYNC_Short_Sync;
	daibt_attribute.mBT_ON = true;
	daibt_attribute.mDAIBT_ON = false;
	SetDaiBt(&daibt_attribute);
	return true;
}


static int mtk_pcm_dl1bt_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	SetMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE || runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_AFE_IO_Block_DAI_BT_OUT);
		/* BT SCO only support 16 bit */
	} else {
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_16_BIT);
		SetConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_AFE_IO_Block_DAI_BT_OUT);
	}

	/* here start digital part */
	SetIntfConnection(Soc_Aud_InterCon_Connection,
			Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT);
	SetIntfConnection(Soc_Aud_InterCon_ConnectionShift,
			Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT);

	/* set dl1 sample ratelimit_state */
	SetSampleRate(Soc_Aud_Digital_Block_MEM_DL1, runtime->rate);
	SetChannels(Soc_Aud_Digital_Block_MEM_DL1, runtime->channels);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, true);

	/* here to set interrupt */
	irq_add_user(substream,
		     Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE,
		     substream->runtime->rate,
		     substream->runtime->period_size >> 1);

	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_DAI_BT) == false) {
		/* set merge interface */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_DAI_BT, true);
	} else {
		SetMemoryPathEnable(Soc_Aud_Digital_Block_DAI_BT, true);
	}

	SetVoipDAIBTAttribute(runtime->rate);
	SetDaiBtEnable(true);

	EnableAfe(true);

	return 0;
}

static int mtk_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	PRINTK_AUDDRV("mtk_pcm_trigger cmd = %d\n", cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_dl1bt_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_dl1Bt_stop(substream);
	}
	return -EINVAL;
}

static int mtk_pcm_dl1bt_copy(struct snd_pcm_substream *substream,
			      int channel, snd_pcm_uframes_t pos,
			      void __user *dst, snd_pcm_uframes_t count)
{
	return mtk_memblk_copy(substream, channel, pos, dst, count, pdl1btMemControl, Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_dl1bt_silence(struct snd_pcm_substream *substream,
				 int channel, snd_pcm_uframes_t pos,
				 snd_pcm_uframes_t count)
{
	PRINTK_AUDDRV("%s\n", __func__);
	return 0; /* do nothing */
}

static void *dummy_page[2];

static struct page *mtk_pcm_page(struct snd_pcm_substream *substream,
				 unsigned long offset)
{
	PRINTK_AUDDRV("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static struct snd_pcm_ops mtk_d1lbt_ops = {
	.open =     mtk_dl1bt_pcm_open,
	.close =    mtk_Dl1Bt_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params =    mtk_pcm_dl1bt_hw_params,
	.hw_free =  mtk_pcm_dl1bt_hw_free,
	.prepare =  mtk_dl1bt_pcm_prepare,
	.trigger =  mtk_pcm_trigger,
	.pointer =  mtk_dl1bt_pcm_pointer,
	.copy =     mtk_pcm_dl1bt_copy,
	.silence =  mtk_pcm_dl1bt_silence,
	.page =     mtk_pcm_page,
};

static struct snd_soc_platform_driver mtk_soc_dl1bt_platform = {
	.ops        = &mtk_d1lbt_ops,
	.pcm_new    = mtk_asoc_Dl1Bt_pcm_new,
	.probe      = mtk_asoc_dl1bt_probe,
};

static int mtk_dl1bt_probe(struct platform_device *pdev)
{
	PRINTK_AUDDRV("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (pdev->dev.dma_mask == NULL)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_VOIP_BT_OUT);

	PRINTK_AUDDRV("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev,
					 &mtk_soc_dl1bt_platform);
}

static int mtk_asoc_Dl1Bt_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;

	PRINTK_AUDDRV("%s\n", __func__);
	return ret;
}


static int mtk_asoc_dl1bt_probe(struct snd_soc_platform *platform)
{
	PRINTK_AUDDRV("mtk_asoc_dl1bt_probe\n");
	AudDrv_Allocate_mem_Buffer(platform->dev, Soc_Aud_Digital_Block_MEM_DL1,
				   Dl1_MAX_BUFFER_SIZE);
	dl1bt_Playback_dma_buf =  Get_Mem_Buffer(Soc_Aud_Digital_Block_MEM_DL1);
	return 0;
}

static int mtk_asoc_dl1bt_remove(struct platform_device *pdev)
{
	PRINTK_AUDDRV("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_dl1_bt_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_dl1_bt", },
	{}
};
#endif

static struct platform_driver mtk_dl1bt_driver = {
	.driver = {
		.name = MT_SOC_VOIP_BT_OUT,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_dl1_bt_of_ids,
#endif
	},
	.probe = mtk_dl1bt_probe,
	.remove = mtk_asoc_dl1bt_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtk_dl1bt_dev;
#endif

static int __init mtk_soc_dl1bt_platform_init(void)
{
	int ret;

	PRINTK_AUDDRV("%s\n", __func__);
#ifndef CONFIG_OF
	soc_mtk_dl1bt_dev = platform_device_alloc(MT_SOC_VOIP_BT_OUT, -1);
	if (!soc_mtk_dl1bt_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_mtk_dl1bt_dev);
	if (ret != 0) {
		platform_device_put(soc_mtk_dl1bt_dev);
		return ret;
	}
#endif
	ret = platform_driver_register(&mtk_dl1bt_driver);
	return ret;

}
module_init(mtk_soc_dl1bt_platform_init);

static void __exit mtk_soc_dl1bt_platform_exit(void)
{
	PRINTK_AUDDRV("%s\n", __func__);

	platform_driver_unregister(&mtk_dl1bt_driver);
}
module_exit(mtk_soc_dl1bt_platform_exit);

MODULE_DESCRIPTION("AFE dl1bt module platform driver");
MODULE_LICENSE("GPL");
