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
 *   mt-soc-pcm-deep-buffer-dl.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio playback for deep buffer
 *
 * Author:
 * -------
 *   Shane Chien
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
#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Kernel.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_digital_type.h"
#include "mt_soc_pcm_common.h"
#include "AudDrv_Gpio.h"
#include <linux/ftrace.h>


#ifdef DEBUG_DEEP_BUFFER_DL
#define DEBUG_DEEP_BUFFER_DL(format, args...)  pr_debug(format, ##args)
#else
#define DEBUG_DEEP_BUFFER_DL(format, args...)
#endif

#define CLEAR_BUFFER_US         600
static int CLEAR_BUFFER_SIZE;

static AFE_MEM_CONTROL_T *pMemControl;
static struct snd_dma_buffer *deep_buffer_dl_dma_buf;
static int mPlaybackSramState;
static bool mPlaybackUseSram;


/*
 *    function implementation
 */
static bool mPrepareDone;
static const void *irq_user_id;
static unsigned int irq_cnt;
static struct device *mDev;

const char * const deep_buffer_dl_HD_output[] = {"Off", "On"};

static int Audio_Irqcnt_Get(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *ucontrol)
{
	AudDrv_Clk_On();
	ucontrol->value.integer.value[0] = Afe_Get_Reg(AFE_IRQ_MCU_CNT1);
	AudDrv_Clk_Off();

	return 0;
}

static int Audio_Irqcnt_Set(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), irq_user_id = %p, irq_cnt = %d, value = %ld\n",
		__func__,
		irq_user_id,
		irq_cnt,
		ucontrol->value.integer.value[0]);

	if (irq_cnt == ucontrol->value.integer.value[0])
		return 0;

	irq_cnt = ucontrol->value.integer.value[0];

	AudDrv_Clk_On();
	if (irq_user_id && irq_cnt)
		irq_update_user(irq_user_id,
				Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE,
				0,
				irq_cnt);
	else
		pr_debug("cannot update irq counter, user_id = %p, irq_cnt = %d\n",
			 irq_user_id, irq_cnt);

	AudDrv_Clk_Off();
	return 0;
}

static const struct snd_kcontrol_new deep_buffer_dl_controls[] = {
	SOC_SINGLE_EXT("deep_buffer_irq_cnt", SND_SOC_NOPM, 0, IRQ_MAX_RATE, 0,
		       Audio_Irqcnt_Get, Audio_Irqcnt_Set),
};

static struct snd_pcm_hardware mtk_deep_buffer_dl_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
	SNDRV_PCM_INFO_INTERLEAVED |
	SNDRV_PCM_INFO_RESUME |
	SNDRV_PCM_INFO_MMAP_VALID),
	.formats =   SND_SOC_ADV_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = Dl2_MAX_BUFFER_SIZE,
	.period_bytes_max = Dl2_MAX_BUFFER_SIZE,
	.periods_min =      SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max =     SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size =        0,
};


static int mtk_deep_buffer_dl_stop(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);

	irq_user_id = NULL;
	irq_remove_user(substream, Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE);

	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL2, false);

	ClearMemBlock(Soc_Aud_Digital_Block_MEM_DL2);

	return 0;
}

static snd_pcm_uframes_t mtk_deep_buffer_dl_pointer(struct snd_pcm_substream
						    *substream)
{
	unsigned int ptr_bytes = 0, hw_ptr;

	hw_ptr = Afe_Get_Reg(AFE_DL2_CUR);

	if (hw_ptr == 0)
		pr_debug("%s(), hw_ptr == 0\n", __func__);
	else
		ptr_bytes = hw_ptr - Afe_Get_Reg(AFE_DL2_BASE);

	return bytes_to_frames(substream->runtime, ptr_bytes);
}


static void set_deep_buffer_data_buffer(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	AFE_BLOCK_T *pblock = &pMemControl->rBlock;

	pblock->pucPhysBufAddr = runtime->dma_addr;
	pblock->pucVirtBufAddr = runtime->dma_area;
	pblock->u4BufferSize = runtime->dma_bytes;
	pblock->u4SampleNumMask = 0x001f;	/* 32 byte align */
	pblock->u4WriteIdx = 0;
	pblock->u4DMAReadIdx = 0;
	pblock->u4DataRemained = 0;
	pblock->u4fsyncflag = false;
	pblock->uResetFlag = true;
	pr_warn("SetDL2Buffer u4BufferSize = %d pucVirtBufAddr = %p pucPhysBufAddr = 0x%x\n",
	       pblock->u4BufferSize, pblock->pucVirtBufAddr, pblock->pucPhysBufAddr);
	/* set dram address top hardware */
	Afe_Set_Reg(AFE_DL2_BASE, pblock->pucPhysBufAddr, 0xffffffff);
	Afe_Set_Reg(AFE_DL2_END, pblock->pucPhysBufAddr + (pblock->u4BufferSize - 1), 0xffffffff);
	memset_io((void *)pblock->pucVirtBufAddr, 0, pblock->u4BufferSize);

}


static int mtk_deep_buffer_dl_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	/* runtime->dma_bytes has to be set manually to allow mmap */
	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);
	substream->runtime->dma_area = deep_buffer_dl_dma_buf->area;
	substream->runtime->dma_addr = deep_buffer_dl_dma_buf->addr;
	set_deep_buffer_data_buffer(substream, hw_params);

	pr_debug("dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
		 substream->runtime->dma_bytes, substream->runtime->dma_area,
		 (long)substream->runtime->dma_addr);

	return ret;
}


static int mtk_deep_buffer_dl_hw_free(struct snd_pcm_substream *substream)
{
	return 0;
}

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	.mask = 0,
};

static int mtk_deep_buffer_dl_close(struct snd_pcm_substream *substream)
{
	pr_debug("%s\n", __func__);

	if (mPrepareDone == true) {
		SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I07,
			      Soc_Aud_InterConnectionOutput_O03);
		SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I08,
			      Soc_Aud_InterConnectionOutput_O04);

		/* stop DAC output */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, false);
		if (GetI2SDacEnable() == false)
			SetI2SDacEnable(false);

		RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL2, substream);

		EnableAfe(false);
		mPrepareDone = false;
	}

	/* reset irq counter */
	irq_cnt = 0;

	if (mPlaybackSramState == SRAM_STATE_PLAYBACKDRAM)
		AudDrv_Emi_Clk_Off();

	AfeControlSramLock();
	ClearSramState(mPlaybackSramState);
	mPlaybackSramState = GetSramState();
	AfeControlSramUnLock();

	AudDrv_Clk_Off();

	return 0;
}

static int mtk_deep_buffer_dl_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	mtk_deep_buffer_dl_hardware.buffer_bytes_max = GetPLaybackDramSize();
	mPlaybackSramState = SRAM_STATE_PLAYBACKDRAM;
	mPlaybackUseSram = false;
	if (mPlaybackSramState == SRAM_STATE_PLAYBACKDRAM)
		AudDrv_Emi_Clk_On();

	pr_warn("%s(), buffer_bytes_max = %zu mPlaybackSramState = %d\n",
		__func__, mtk_deep_buffer_dl_hardware.buffer_bytes_max,
		mPlaybackSramState);

	runtime->hw = mtk_deep_buffer_dl_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_deep_buffer_dl_hardware,
	       sizeof(struct snd_pcm_hardware));

	AudDrv_Clk_On();

	pMemControl = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_DL2);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);

	if (ret < 0) {
		pr_err("snd_pcm_hw_constraint_integer failed, close pcm\n");
		mtk_deep_buffer_dl_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_deep_buffer_dl_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	bool mI2SWLen;

	pr_debug("%s: mPrepareDone = %d, format = %d, sample rate = %d\n",
			       __func__,
			       mPrepareDone,
			       runtime->format,
			       substream->runtime->rate);

	if (mPrepareDone == false) {
		SetMemifSubStream(Soc_Aud_Digital_Block_MEM_DL2, substream);

		if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
		    runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL2,
						     AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
						  Soc_Aud_InterConnectionOutput_O03);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT,
						  Soc_Aud_InterConnectionOutput_O04);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		} else {
			SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_DL2, AFE_WLEN_16_BIT);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
						  Soc_Aud_InterConnectionOutput_O03);
			SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT,
						  Soc_Aud_InterConnectionOutput_O04);
			mI2SWLen = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		}

		SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I07,
			      Soc_Aud_InterConnectionOutput_O03);
		SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I08,
			      Soc_Aud_InterConnectionOutput_O04);


		/* start I2S DAC out */
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC) == false) {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
			SetI2SDacOut(substream->runtime->rate, 0, mI2SWLen);
			SetI2SDacEnable(true);
		} else {
			SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_OUT_DAC, true);
		}

		EnableAfe(true);
		mPrepareDone = true;

		CLEAR_BUFFER_SIZE = substream->runtime->rate * CLEAR_BUFFER_US *
				    audio_frame_to_bytes(substream, 1) / 1000000;
	}
	return 0;
}

static int mtk_deep_buffer_dl_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_debug("%s\n", __func__);

	/* here to set interrupt */
	irq_add_user(substream,
		     Soc_Aud_IRQ_MCU_MODE_IRQ1_MCU_MODE,
		     substream->runtime->rate,
		     irq_cnt ? irq_cnt : substream->runtime->period_size);
	irq_user_id = substream;

	SetSampleRate(Soc_Aud_Digital_Block_MEM_DL2, runtime->rate);
	SetChannels(Soc_Aud_Digital_Block_MEM_DL2, runtime->channels);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL2, true);

	EnableAfe(true);

	return 0;
}

static int mtk_deep_buffer_dl_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_deep_buffer_dl_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_deep_buffer_dl_stop(substream);
	}
	return -EINVAL;
}

static void *dummy_page[2];
static struct page *mtk_deep_buffer_dl_page(struct snd_pcm_substream *substream,
				       unsigned long offset)
{
	pr_debug("%s\n", __func__);
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static int mtk_deep_buffer_dl_ack(struct snd_pcm_substream *substream)
{
	int size_per_frame = audio_frame_to_bytes(substream, 1);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int copy_size = snd_pcm_playback_avail(runtime) * size_per_frame;

	if (copy_size > CLEAR_BUFFER_SIZE)
		copy_size = CLEAR_BUFFER_SIZE;

	if (copy_size > 0) {
		AFE_BLOCK_T *afe_block_t = &pMemControl->rBlock;
		snd_pcm_uframes_t appl_ofs = runtime->control->appl_ptr % runtime->buffer_size;
		int32_t u4WriteIdx = appl_ofs * size_per_frame;

		if (u4WriteIdx + copy_size < afe_block_t->u4BufferSize) {
			memset_io(afe_block_t->pucVirtBufAddr + u4WriteIdx, 0, copy_size);
			/*
			* pr_debug("%s A, offset %d, clear buffer %d, copy_size %d\n",
			*          __func__, u4WriteIdx, copy_size, copy_size);
			*/
		} else {
			int32_t size_1 = 0, size_2 = 0;

			size_1 = afe_block_t->u4BufferSize - u4WriteIdx;
			size_2 = copy_size - size_1;

			memset_io(afe_block_t->pucVirtBufAddr + u4WriteIdx, 0, size_1);
			memset_io(afe_block_t->pucVirtBufAddr, 0, size_2);
			/*
			 * pr_debug("%s B-1, offset %d, clear buffer %d, copy_size %d\n",
			 *          __func__, u4WriteIdx, size_1, copy_size);
			 * pr_debug("%s B-2, offset %d, clear buffer %d, copy_size %d\n",
			 *          __func__, 0, size_2, copy_size);
			 */
		}
	}

	return 0;
}

static struct snd_pcm_ops mtk_deep_buffer_dl_ops = {
	.open =     mtk_deep_buffer_dl_open,
	.close =    mtk_deep_buffer_dl_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params = mtk_deep_buffer_dl_hw_params,
	.hw_free =  mtk_deep_buffer_dl_hw_free,
	.prepare =  mtk_deep_buffer_dl_prepare,
	.trigger =  mtk_deep_buffer_dl_trigger,
	.pointer =  mtk_deep_buffer_dl_pointer,
	.page =     mtk_deep_buffer_dl_page,
	.ack =      mtk_deep_buffer_dl_ack,
};

static int mtk_deep_buffer_dl_platform_probe(struct snd_soc_platform *platform)
{
	snd_soc_add_platform_controls(platform, deep_buffer_dl_controls,
				      ARRAY_SIZE(deep_buffer_dl_controls));
	/* allocate dram */
	AudDrv_Allocate_mem_Buffer(platform->dev, Soc_Aud_Digital_Block_MEM_DL2,
				   Dl2_MAX_BUFFER_SIZE);
	deep_buffer_dl_dma_buf = Get_Mem_Buffer(Soc_Aud_Digital_Block_MEM_DL2);

	pr_debug("area = %p\n", deep_buffer_dl_dma_buf->area);

	return 0;
}

static struct snd_soc_platform_driver mtk_deep_buffer_dl_soc_platform = {
	.ops        = &mtk_deep_buffer_dl_ops,
	.probe      = mtk_deep_buffer_dl_platform_probe,
};

static int mtk_deep_buffer_dl_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_DEEP_BUFFER_DL_PCM);

	pr_debug("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev,
					 &mtk_deep_buffer_dl_soc_platform);
}

static int mtk_deep_buffer_dl_remove(struct platform_device *pdev)
{
	pr_info("%s()\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_deep_buffer_dl_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_deep_buffer_dl", },
	{}
};
#endif

static struct platform_driver mtk_deep_buffer_dl_driver = {
	.driver = {
		.name = MT_SOC_DEEP_BUFFER_DL_PCM,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_deep_buffer_dl_of_ids,
#endif
	},
	.probe = mtk_deep_buffer_dl_probe,
	.remove = mtk_deep_buffer_dl_remove,
};

module_platform_driver(mtk_deep_buffer_dl_driver);

MODULE_DESCRIPTION("AFE PCM module platform driver");
MODULE_LICENSE("GPL");
