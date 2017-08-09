/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mtk_osc_pcm_i2s2_adc2.c
 *
 * Project:
 * --------
 *   Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio i2s2 to adc2 capture
 *
 * Author:
 * -------
 *
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
#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Kernel.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_pcm_common.h"

static AFE_MEM_CONTROL_T  *I2S2_ADC2_Control_context;
static struct snd_dma_buffer *Adc2_Capture_dma_buf;
static unsigned int mPlaybackDramState;

static DEFINE_SPINLOCK(auddrv_I2s2adc2InCtl_lock);
static kal_int32 Previous_Hw_cur;
static struct device *mDev;

/*
 *    function implementation
 */

static void StartAudioI2S2ADC2Hardware(struct snd_pcm_substream *substream);
static void StopAudioI2S2adc2Hardware(struct snd_pcm_substream *substream);
static int mtk_i2s2_adc2_probe(struct platform_device *pdev);
static int mtk_i2s2_adc2_pcm_close(struct snd_pcm_substream *substream);
static int mtk_asoc_i2s2_adc2_pcm_new(struct snd_soc_pcm_runtime *rtd);
static int mtk_i2s2_adc2_data_probe(struct snd_soc_platform *platform);

static struct snd_pcm_hardware mtk_I2S2_adc2_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED),
	.formats =      SND_SOC_ADV_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = AWB_MAX_BUFFER_SIZE,
	.period_bytes_max = AWB_MAX_BUFFER_SIZE,
	.periods_min =     AWB_MIN_PERIOD_SIZE,
	.periods_max =      AWB_MAX_PERIOD_SIZE,
	.fifo_size =        0,
};

static void StopAudioI2S2adc2Hardware(struct snd_pcm_substream *substream)
{
	pr_warn("StopAudioI2S2adc2Hardware\n");

	SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN, false);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_VUL_DATA2, false);
	SetExtI2SAdcInEnable(true);

	irq_remove_user(substream, Soc_Aud_IRQ_MCU_MODE_IRQ2_MCU_MODE);

	SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I25, Soc_Aud_InterConnectionOutput_O21);
	SetConnection(Soc_Aud_InterCon_DisConnect, Soc_Aud_InterConnectionInput_I26, Soc_Aud_InterConnectionOutput_O22);

	EnableAfe(false);
}

static void StartAudioI2S2ADC2Hardware(struct snd_pcm_substream *substream)
{
	AudioDigtalI2S DigtalI2SIn;

	pr_warn("+StartAudioI2S2ADC2Hardware\n");

	DigtalI2SIn.mLR_SWAP = Soc_Aud_LR_SWAP_NO_SWAP;
	DigtalI2SIn.mBuffer_Update_word = 8;
	DigtalI2SIn.mFpga_bit_test = 0;
	DigtalI2SIn.mFpga_bit = 0;
	DigtalI2SIn.mloopback = 0;
	DigtalI2SIn.mINV_LRCK = Soc_Aud_INV_LRCK_NO_INVERSE;
	DigtalI2SIn.mI2S_FMT = Soc_Aud_I2S_FORMAT_I2S;
	DigtalI2SIn.mI2S_WLEN = Soc_Aud_I2S_WLEN_WLEN_32BITS;
	DigtalI2SIn.mI2S_IN_PAD_SEL = true;
	DigtalI2SIn.mI2S_SAMPLERATE = (substream->runtime->rate);

	SetExtI2SAdcIn(&DigtalI2SIn);
	SetExtI2SAdcInEnable(true);

	if (substream->runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
		substream->runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
		pr_warn("+StartAudioI2S2ADC2Hardware SNDRV_PCM_FORMAT_U32_LE\n");
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_InterConnectionOutput_O21);
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_24BIT, Soc_Aud_InterConnectionOutput_O22);
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_VUL_DATA2,
			AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);

	} else {
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O21);
		SetoutputConnectionFormat(OUTPUT_DATA_FORMAT_16BIT, Soc_Aud_InterConnectionOutput_O22);
		SetMemIfFetchFormatPerSample(Soc_Aud_Digital_Block_MEM_VUL_DATA2, AFE_WLEN_16_BIT);
	}

	SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I25, Soc_Aud_InterConnectionOutput_O21);
	SetConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I26, Soc_Aud_InterConnectionOutput_O22);

	SetSampleRate(Soc_Aud_Digital_Block_MEM_VUL_DATA2, substream->runtime->rate);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_VUL_DATA2, true);

	/* here to set interrupt */
	irq_add_user(substream,
		     Soc_Aud_IRQ_MCU_MODE_IRQ2_MCU_MODE,
		     substream->runtime->rate,
		     substream->runtime->period_size);

	EnableAfe(true);
}

static int mtk_i2s2_adc2_pcm_prepare(struct snd_pcm_substream *substream)
{
	pr_debug("mtk_i2s2_adc2_pcm_prepare substream->rate = %d  substream->channels = %d\n",
		substream->runtime->rate, substream->runtime->channels);
	return 0;
}

static int mtk_i2s2_adc2_alsa_stop(struct snd_pcm_substream *substream)
{
	pr_warn("mtk_i2s2_adc2_alsa_stop\n");
	StopAudioI2S2adc2Hardware(substream);
	RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_VUL_DATA2, substream);
	return 0;
}

static snd_pcm_uframes_t mtk_i2s2_adc2_pcm_pointer(struct snd_pcm_substream *substream)
{
	kal_int32 HW_memory_index = 0;
	kal_int32 HW_Cur_ReadIdx = 0;
	kal_uint32 Frameidx = 0;
	AFE_BLOCK_T *adc2_Block = &(I2S2_ADC2_Control_context->rBlock);

	PRINTK_AUD_UL2("mtk_i2s2_adc2_pcm_pointer adc2_Block->u4WriteIdx;= 0x%x\n", adc2_Block->u4WriteIdx);
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_VUL_DATA2) == true) {
		Frameidx = audio_bytes_to_frame(substream, adc2_Block->u4WriteIdx);
		return Frameidx;

		HW_Cur_ReadIdx = Align64ByteSize(Afe_Get_Reg(AFE_AWB_CUR));
		if (HW_Cur_ReadIdx == 0) {
			pr_err("[Auddrv] mtk_awb_pcm_pointer  HW_Cur_ReadIdx ==0\n");
			HW_Cur_ReadIdx = adc2_Block->pucPhysBufAddr;
		}
		HW_memory_index = (HW_Cur_ReadIdx - adc2_Block->pucPhysBufAddr);
		Previous_Hw_cur = HW_memory_index;
		pr_debug("[Auddrv] mtk_i2s2_adc2_pcm_pointer =0x%x HW_memory_index = 0x%x\n",
			HW_Cur_ReadIdx, HW_memory_index);
		return audio_bytes_to_frame(substream, Previous_Hw_cur);
	}
	return 0;
}

static void SetADC2Buffer(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *hw_params)
{
	AFE_BLOCK_T *pblock = &I2S2_ADC2_Control_context->rBlock;
	struct snd_pcm_runtime *runtime = substream->runtime;

	pblock->pucPhysBufAddr =  runtime->dma_addr;
	pblock->pucVirtBufAddr =  runtime->dma_area;
	pblock->u4BufferSize = runtime->dma_bytes;
	pblock->u4SampleNumMask = 0x001f;
	pblock->u4WriteIdx     = 0;
	pblock->u4DMAReadIdx    = 0;
	pblock->u4DataRemained  = 0;
	pblock->u4fsyncflag     = false;
	pblock->uResetFlag      = true;
	pr_debug("dma_bytes = %d dma_area = %p dma_addr = 0x%x\n",
	       pblock->u4BufferSize, pblock->pucVirtBufAddr, pblock->pucPhysBufAddr);

	Afe_Set_Reg(AFE_VUL_D2_BASE , pblock->pucPhysBufAddr , 0xffffffff);
	Afe_Set_Reg(AFE_VUL_D2_END  , pblock->pucPhysBufAddr + (pblock->u4BufferSize - 1), 0xffffffff);
}

static int mtk_i2s2_adc2_pcm_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);
	if (AllocateAudioSram(&substream->runtime->dma_addr,	&substream->runtime->dma_area,
		substream->runtime->dma_bytes, substream) == 0) {
		mPlaybackDramState = false;
		/* pr_warn("mtk_pcm_hw_params dma_bytes = %d\n",substream->runtime->dma_bytes); */
	} else {
		substream->runtime->dma_area = Adc2_Capture_dma_buf->area;
		substream->runtime->dma_addr = Adc2_Capture_dma_buf->addr;
		mPlaybackDramState = true;
		AudDrv_Emi_Clk_On();
	}

	pr_warn("mtk_i2s2_adc2_pcm_hw_params dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
	       runtime->dma_bytes, runtime->dma_area, (long)runtime->dma_addr);
	SetADC2Buffer(substream, hw_params);
	return ret;
}

static int mtk_i2s2_adc2_capture_pcm_hw_free(struct snd_pcm_substream *substream)
{
	pr_warn("mtk_i2s2_adc2_capture_pcm_hw_free\n");
	if (mPlaybackDramState == true) {
		AudDrv_Emi_Clk_Off();
		mPlaybackDramState = false;
	} else
		freeAudioSram((void *)substream);
	return 0;
}

static struct snd_pcm_hw_constraint_list i2s2_adc2_constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
};

static int mtk_i2s2_adc2_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	mPlaybackDramState = false;

	pr_warn("mtk_i2s2_adc2_pcm_open\n");
	I2S2_ADC2_Control_context = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_VUL_DATA2);
	runtime->hw = mtk_I2S2_adc2_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_I2S2_adc2_hardware , sizeof(struct snd_pcm_hardware));

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &i2s2_adc2_constraints_sample_rates);
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (ret < 0)
		pr_err("snd_pcm_hw_constraint_integer failed\n");

	runtime->hw.info |= SNDRV_PCM_INFO_INTERLEAVED;
	runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	runtime->hw.info |= SNDRV_PCM_INFO_MMAP_VALID;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		pr_debug("SNDRV_PCM_STREAM_CAPTURE\n");

	if (ret < 0) {
		pr_err("mtk_i2s2_adc2_pcm_close\n");
		mtk_i2s2_adc2_pcm_close(substream);
		return ret;
	}
	AudDrv_Clk_On();
	pr_debug("mtk_i2s2_adc2_pcm_open return\n");
	return 0;
}

static int mtk_i2s2_adc2_pcm_close(struct snd_pcm_substream *substream)
{
	AudDrv_Clk_Off();
	return 0;
}

static int mtk_i2s2_adc2_alsa_start(struct snd_pcm_substream *substream)
{
	pr_debug("mtk_i2s2_adc2_alsa_start\n");
	SetMemifSubStream(Soc_Aud_Digital_Block_MEM_VUL_DATA2, substream);
	StartAudioI2S2ADC2Hardware(substream);
	return 0;
}

static int mtk_i2s2_adc2_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	pr_debug("mtk_i2s2_adc2_pcm_trigger cmd = %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_i2s2_adc2_alsa_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_i2s2_adc2_alsa_stop(substream);
	}
	return -EINVAL;
}

static bool CheckNullPointer(void *pointer)
{
	if (pointer == NULL) {
		pr_err("CheckNullPointer pointer = NULL");
		return true;
	}
	return false;
}

static int mtk_i2s2_adc2_pcm_copy(struct snd_pcm_substream *substream,
				  int channel, snd_pcm_uframes_t pos,
				  void __user *dst, snd_pcm_uframes_t count)
{
	AFE_MEM_CONTROL_T *pADC_Data2_MEM_ConTrol = NULL;
	AFE_BLOCK_T  *adc2_Block = NULL;
	char *Read_Data_Ptr = (char *)dst;
	int DMA_Read_Ptr = 0 , read_size = 0, read_count = 0;
	unsigned long flags;

	count = Align64ByteSize(audio_frame_to_bytes(substream , count));

	pADC_Data2_MEM_ConTrol = I2S2_ADC2_Control_context;
	adc2_Block = &(pADC_Data2_MEM_ConTrol->rBlock);

	if (pADC_Data2_MEM_ConTrol == NULL) {
		pr_err("cannot find MEM control !!!!!!!\n");
		msleep(50);
		return 0;
	}

	if (adc2_Block->u4BufferSize <= 0) {
		msleep(50);
		return 0;
	}

	if (CheckNullPointer((void *)adc2_Block->pucVirtBufAddr)) {
		pr_err("CheckNullPointer  pucVirtBufAddr = %p\n", adc2_Block->pucVirtBufAddr);
		return 0;
	}

	spin_lock_irqsave(&auddrv_I2s2adc2InCtl_lock, flags);
	if (adc2_Block->u4DataRemained >  adc2_Block->u4BufferSize) {
		pr_err("AudDrv_MEMIF_Read u4DataRemained=%x > u4BufferSize=%x",
			adc2_Block->u4DataRemained, adc2_Block->u4BufferSize);
		adc2_Block->u4DataRemained = 0;
		adc2_Block->u4DMAReadIdx   = adc2_Block->u4WriteIdx;
	}
	if (count >  adc2_Block->u4DataRemained)
		read_size = adc2_Block->u4DataRemained;
	else
		read_size = count;

	DMA_Read_Ptr = adc2_Block->u4DMAReadIdx;
	spin_unlock_irqrestore(&auddrv_I2s2adc2InCtl_lock, flags);

	PRINTK_AUD_UL2("finish0 read_count:0x%x read_size:0x%x,Remained:0x%x, ReadIdx:0x%x, WriteIdx:0x%x\n",
		 read_count, read_size, adc2_Block->u4DataRemained, adc2_Block->u4DMAReadIdx, adc2_Block->u4WriteIdx);

	if (DMA_Read_Ptr + read_size < adc2_Block->u4BufferSize) {
		if (DMA_Read_Ptr != adc2_Block->u4DMAReadIdx) {
			pr_err("read_size:%d, DataRemained:0x%x, DMA_Read_Ptr:%d, DMAReadIdx:0x%x\n",
			       read_size, adc2_Block->u4DataRemained, DMA_Read_Ptr, adc2_Block->u4DMAReadIdx);
		}

		if (copy_to_user((void __user *)Read_Data_Ptr,
			(adc2_Block->pucVirtBufAddr + DMA_Read_Ptr), read_size)) {
			pr_err("Fail copy Data_Ptr:%p VirtBufAddr:%p, ReadIdx:0x%x Read_Ptr:%d,read_size:%d",
				Read_Data_Ptr, adc2_Block->pucVirtBufAddr, adc2_Block->u4DMAReadIdx,
					DMA_Read_Ptr, read_size);
			return 0;
		}

		read_count += read_size;
		spin_lock(&auddrv_I2s2adc2InCtl_lock);
		adc2_Block->u4DataRemained -= read_size;
		adc2_Block->u4DMAReadIdx += read_size;
		adc2_Block->u4DMAReadIdx %= adc2_Block->u4BufferSize;
		DMA_Read_Ptr = adc2_Block->u4DMAReadIdx;
		spin_unlock(&auddrv_I2s2adc2InCtl_lock);

		Read_Data_Ptr += read_size;
		count -= read_size;

		PRINTK_AUD_UL2("finish1, copy size:0x%x, u4DMAReadIdx:0x%x, u4WriteIdx:0x%x, u4DataRemained:0x%x\n",
			 read_size, adc2_Block->u4DMAReadIdx, adc2_Block->u4WriteIdx, adc2_Block->u4DataRemained);
	}

	else {
		uint32 size_1 = adc2_Block->u4BufferSize - DMA_Read_Ptr;
		uint32 size_2 = read_size - size_1;

		if (DMA_Read_Ptr != adc2_Block->u4DMAReadIdx) {

			pr_err("2 read_size1:0x%x, DataRemained:0x%x, DMA_Read_Ptr:%d, DMAReadIdx:0x%x\n",
			       size_1, adc2_Block->u4DataRemained, DMA_Read_Ptr, adc2_Block->u4DMAReadIdx);
		}
		if (copy_to_user((void __user *)Read_Data_Ptr, (adc2_Block->pucVirtBufAddr + DMA_Read_Ptr), size_1)) {
			pr_err("Fail 2 copy user Read_Ptr:%p, BufAddr:%p, ReadIdx:0x%x, Read_Ptr:%d,read_size:%d",
			       Read_Data_Ptr, adc2_Block->pucVirtBufAddr,
			       adc2_Block->u4DMAReadIdx, DMA_Read_Ptr, read_size);
			return 0;
		}

		read_count += size_1;
		spin_lock(&auddrv_I2s2adc2InCtl_lock);
		adc2_Block->u4DataRemained -= size_1;
		adc2_Block->u4DMAReadIdx += size_1;
		adc2_Block->u4DMAReadIdx %= adc2_Block->u4BufferSize;
		DMA_Read_Ptr = adc2_Block->u4DMAReadIdx;
		spin_unlock(&auddrv_I2s2adc2InCtl_lock);

		PRINTK_AUD_UL2("finish2, copy size_1:0x%x, ReadIdx:0x%x, riteIdx:0x%x, DataRemained:0x%x\n",
			 size_1, adc2_Block->u4DMAReadIdx, adc2_Block->u4WriteIdx, adc2_Block->u4DataRemained);

		if (DMA_Read_Ptr != adc2_Block->u4DMAReadIdx) {

			pr_err("3 read_size2:%x, DataRemained:%x, DMA_Read_Ptr:%d, DMAReadIdx:%x\n",
			       size_2, adc2_Block->u4DataRemained, DMA_Read_Ptr, adc2_Block->u4DMAReadIdx);
		}
		if (copy_to_user((void __user *)(Read_Data_Ptr + size_1),
			(adc2_Block->pucVirtBufAddr + DMA_Read_Ptr), size_2)) {
			pr_err("Fail 3 Read_Data_Ptr:%p, VirtBufAddr:%p, ReadIdx:0x%x ,Read_Ptr:%d, read_size:%d",
			       Read_Data_Ptr, adc2_Block->pucVirtBufAddr, adc2_Block->u4DMAReadIdx,
			       DMA_Read_Ptr, read_size);
			return read_count << 2;
		}

		read_count += size_2;
		spin_lock(&auddrv_I2s2adc2InCtl_lock);
		adc2_Block->u4DataRemained -= size_2;
		adc2_Block->u4DMAReadIdx += size_2;
		DMA_Read_Ptr = adc2_Block->u4DMAReadIdx;
		spin_unlock(&auddrv_I2s2adc2InCtl_lock);

		count -= read_size;
		Read_Data_Ptr += read_size;

		PRINTK_AUD_UL2("finish3, copy size_2:0x%x, ReadIdx:0x%x, WriteIdx:0x%x Remained:0x%x\n",
			 size_2, adc2_Block->u4DMAReadIdx, adc2_Block->u4WriteIdx, adc2_Block->u4DataRemained);
	}

	return read_count >> 2;
}

static int mtk_i2s_adc2_pcm_silence(struct snd_pcm_substream *substream,
				    int channel, snd_pcm_uframes_t pos,
				    snd_pcm_uframes_t count)
{
	pr_debug("dummy_pcm_silence\n");
	return 0; /* do nothing */
}


static void *dummy_page[2];

static struct page *mtk_i2s2_adc2_pcm_page(struct snd_pcm_substream *substream,
					   unsigned long offset)
{
	pr_debug("dummy_pcm_page\n");
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}


static struct snd_pcm_ops mtk_i2s2_adc2_ops = {
	.open =     mtk_i2s2_adc2_pcm_open,
	.close =    mtk_i2s2_adc2_pcm_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params =    mtk_i2s2_adc2_pcm_hw_params,
	.hw_free =  mtk_i2s2_adc2_capture_pcm_hw_free,
	.prepare =  mtk_i2s2_adc2_pcm_prepare,
	.trigger =  mtk_i2s2_adc2_pcm_trigger,
	.pointer =  mtk_i2s2_adc2_pcm_pointer,
	.copy =     mtk_i2s2_adc2_pcm_copy,
	.silence =  mtk_i2s_adc2_pcm_silence,
	.page =     mtk_i2s2_adc2_pcm_page,
};

static struct snd_soc_platform_driver mtk_soc_platform = {
	.ops        = &mtk_i2s2_adc2_ops,
	.pcm_new    = mtk_asoc_i2s2_adc2_pcm_new,
	.probe      = mtk_i2s2_adc2_data_probe,
};

static int mtk_i2s2_adc2_probe(struct platform_device *pdev)
{
	pr_debug("mtk_i2s2_adc2_probe\n");

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_I2S2_ADC2_PCM);

	pr_err("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	mDev = &pdev->dev;
	return snd_soc_register_platform(&pdev->dev,
					 &mtk_soc_platform);
}

static int mtk_asoc_i2s2_adc2_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	pr_debug("mtk_asoc_i2s2_adc2_pcm_new\n");
	return 0;
}

static int mtk_i2s2_adc2_data_probe(struct snd_soc_platform *platform)
{
	pr_warn("mtk_i2s2_adc2_data_probe\n");
	AudDrv_Allocate_mem_Buffer(platform->dev, Soc_Aud_Digital_Block_MEM_VUL_DATA2, UL2_MAX_BUFFER_SIZE);
	Adc2_Capture_dma_buf =  Get_Mem_Buffer(Soc_Aud_Digital_Block_MEM_VUL_DATA2);
	return 0;
}

static int mtk_i2s2_adc2_remove(struct platform_device *pdev)
{
	pr_warn("%s\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_i2s2_adc2_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_i2s2_adc2", },
	{}
};
#endif

static struct platform_driver mtk_i2s2_adc2_capture_driver = {
	.driver = {
		.name = MT_SOC_I2S2_ADC2_PCM,
		.owner = THIS_MODULE,
	},
	.probe = mtk_i2s2_adc2_probe,
	.remove = mtk_i2s2_adc2_remove,
};

static struct platform_device *soc_i2s2_adc2_capture_dev;

static int __init mtk_soc_i2s2_adc2_platform_init(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
	soc_i2s2_adc2_capture_dev = platform_device_alloc(MT_SOC_I2S2_ADC2_PCM, -1);
	if (!soc_i2s2_adc2_capture_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_i2s2_adc2_capture_dev);
	if (ret != 0) {
		platform_device_put(soc_i2s2_adc2_capture_dev);
		return ret;
	}

	ret = platform_driver_register(&mtk_i2s2_adc2_capture_driver);
	return ret;
}

static void __exit mtk_soc_i2s2_adc2_platform_exit(void)
{
	pr_debug("%s\n", __func__);
	platform_driver_unregister(&mtk_i2s2_adc2_capture_driver);
}

module_init(mtk_soc_i2s2_adc2_platform_init);
module_exit(mtk_soc_i2s2_adc2_platform_exit);

MODULE_DESCRIPTION("I2S2 ADC2 module platform driver");
MODULE_LICENSE("GPL");


