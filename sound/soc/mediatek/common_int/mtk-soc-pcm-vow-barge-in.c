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
 *   mtk-soc-pcm-vow-barge-in.c
 *
 * Project:
 * --------
 *   Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio echo reference to memif for VOW barge-in
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

#include "mtk-soc-speaker-amp.h"
#include <sound/pcm_params.h>
#include "scp_helper.h"

/* information about */
#define ECHOREF_SAMPLE_RATE (48000)
#define SWIP_SAMPLE_RATE (16000)

static struct afe_mem_control_t *vow_barge_in_mem_control_context;

static struct snd_dma_buffer vow_barge_in_dma_buf;
static int deep_buffer_mem_blk_io;
static int vow_barge_in_mem_blk_io;
static int vow_barge_in_mem_blk;
static int smart_pa_in_blk_io;
static unsigned int using_dram;
static bool is_use_smart_pa;
static bool is_use_general_src = true;

static void start_vow_barge_in_hardware(struct snd_pcm_substream *substream);
static void stop_vow_barge_in_hardware(struct snd_pcm_substream *substream);
static int mtk_vow_barge_in_probe(struct platform_device *pdev);
static int mtk_vow_barge_in_pcm_close(struct snd_pcm_substream *substream);
static int mtk_afe_vow_barge_in_probe(struct snd_soc_platform *platform);

#define MAX_PCM_DEVICES     4
#define MAX_PCM_SUBSTREAMS  128
#define MAX_MIDI_DEVICES

static struct snd_pcm_hardware mtk_vow_barge_in_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP |
	SNDRV_PCM_INFO_INTERLEAVED |
	SNDRV_PCM_INFO_RESUME |
	SNDRV_PCM_INFO_MMAP_VALID),
	.formats =      SND_SOC_STD_MT_FMTS,
	.rates =        SOC_HIGH_USE_RATE,
	.rate_min =     SOC_HIGH_USE_RATE_MIN,
	.rate_max =     SOC_HIGH_USE_RATE_MAX,
	.channels_min =     SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max =     SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = VOW_BARGE_IN_MAX_BUFFER_SIZE,
	.period_bytes_max = VOW_BARGE_IN_MAX_BUFFER_SIZE,
	.periods_min =      VOW_BARGE_IN_MIN_PERIOD_SIZE,
	.periods_max =      VOW_BARGE_IN_MAX_PERIOD_SIZE,
	.fifo_size =        0,
};

const char * const vow_barge_in_use_hw_src[] = {"Off", "On"};
const char * const vow_barge_in_use_smart_pa[] = {"Off", "On"};
const char * const vow_barge_in_pcm_dump[] = {"Off", "On"};

static const struct soc_enum vow_barge_in_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(vow_barge_in_use_hw_src), vow_barge_in_use_hw_src),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(vow_barge_in_use_smart_pa), vow_barge_in_use_smart_pa),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(vow_barge_in_pcm_dump), vow_barge_in_pcm_dump),
};

/*
 *    function implementation
 */
static int use_hw_src_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), is_use_general_src = %d\n", __func__, is_use_general_src);
	ucontrol->value.integer.value[0] = is_use_general_src;
	return 0;
}

static int use_hw_src_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(vow_barge_in_use_hw_src)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	is_use_general_src = ucontrol->value.integer.value[0];
	pr_debug("%s(), is_use_general_src = %d\n",
			__func__, is_use_general_src);
	return 0;
}

static int use_smart_pa_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s(), is_use_smart_pa = %d\n", __func__, is_use_smart_pa);
	ucontrol->value.integer.value[0] = is_use_smart_pa;
	return 0;
}

static int use_smart_pa_set(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.enumerated.item[0] > ARRAY_SIZE(vow_barge_in_use_smart_pa)) {
		pr_debug("return -EINVAL\n");
		return -EINVAL;
	}

	is_use_smart_pa = ucontrol->value.integer.value[0];
	pr_debug("%s(), is_use_smart_pa = %d\n",
			__func__, is_use_smart_pa);
	return 0;
}

static const struct snd_kcontrol_new vow_barge_in_controls[] = {
	SOC_ENUM_EXT("vow_barge_in_use_hw_src", vow_barge_in_enum[0],
			use_hw_src_get, use_hw_src_set),
	SOC_ENUM_EXT("vow_barge_in_use_smart_pa", vow_barge_in_enum[1],
			use_smart_pa_get, use_smart_pa_set),
/*	SOC_ENUM_EXT("vow_barge_in_pcm_dump", vow_barge_in_enum[2], */
/*		     mtk_vow_barge_in_pcm_dump_get, mtk_vow_barge_in_pcm_dump_set), */
};

bool set_interconnection(unsigned int connection_state, bool use_smart_pa, bool use_general_src)
{
	if (use_general_src) {
		if (use_smart_pa) {
			SetIntfConnection(connection_state, smart_pa_in_blk_io,
					  Soc_Aud_AFE_IO_Block_GENERAL_SRC_1_OUT);
		} else {
			SetIntfConnection(connection_state, Soc_Aud_AFE_IO_Block_MEM_DL1,
					  Soc_Aud_AFE_IO_Block_GENERAL_SRC_1_OUT);
			SetIntfConnection(connection_state, Soc_Aud_AFE_IO_Block_MEM_DL2,
					  Soc_Aud_AFE_IO_Block_GENERAL_SRC_1_OUT);
			if (deep_buffer_mem_blk_io >= 0) {
				pr_debug("%s(), deep_buffer_mem_blk_io %d, set intercon between awb and deep buffer\n",
						__func__, deep_buffer_mem_blk_io);
				SetIntfConnection(connection_state, deep_buffer_mem_blk_io,
						  Soc_Aud_AFE_IO_Block_GENERAL_SRC_1_OUT);
			}
		}
		SetIntfConnection(connection_state, Soc_Aud_AFE_IO_Block_GENERAL_SRC_1_IN, vow_barge_in_mem_blk_io);
	} else {
		if (use_smart_pa) {
			SetIntfConnection(connection_state, smart_pa_in_blk_io, vow_barge_in_mem_blk_io);
		} else {
			SetIntfConnection(connection_state, Soc_Aud_AFE_IO_Block_MEM_DL1, vow_barge_in_mem_blk_io);
			SetIntfConnection(connection_state, Soc_Aud_AFE_IO_Block_MEM_DL2, vow_barge_in_mem_blk_io);
			if (deep_buffer_mem_blk_io >= 0) {
				pr_debug("%s(), deep_buffer_mem_blk_io %d, set intercon between awb and deep buffer\n",
						__func__, deep_buffer_mem_blk_io);
				SetIntfConnection(connection_state, deep_buffer_mem_blk_io, vow_barge_in_mem_blk_io);
			}
		}
	}
	return true;
}
static void stop_vow_barge_in_hardware(struct snd_pcm_substream *substream)
{
	pr_debug("stop_vow_barge_in_hardware()\n");

	/* Set interconnection */
	set_interconnection(Soc_Aud_InterCon_DisConnect, is_use_smart_pa, is_use_general_src);

	/* Set smart pa i2s */
	if (is_use_smart_pa) {
		/* stop default i2s in: i2s0 */
		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_2, false);
		if (GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_2) == false) {
			Set2ndI2SEnable(false);
			udelay(20);
			Afe_Set_Reg(AUDIO_TOP_CON1, 0x1 << 4,  0x1 << 4); /* I2S0 clock-gated */
			pr_debug("%s(), AFE_I2S_CON = 0x%x\n", __func__, Afe_Get_Reg(AFE_I2S_CON));
		}
	}

	if (is_use_general_src)
		set_general_asrc_enable(AUDIO_GENERAL_ASRC_1, false);

	/* SetMemoryPathEnable(vow_barge_in_mem_blk, false); */

	/* here to set interrupt */
	/* irq_remove_user(substream, Soc_Aud_IRQ_MCU_MODE_IRQ7_MCU_MODE); */

	EnableAfe(false);
}

static void start_vow_barge_in_hardware(struct snd_pcm_substream *substream)
{
	unsigned int memif_rate = 0;

	pr_debug("start_vow_barge_in_hardware()\n");

	if (is_use_smart_pa) {
		unsigned int u32Audio2ndI2sIn = 0;
		/* default i2s in for echo reference is i2s0 */
		Afe_Set_Reg(AUDIO_TOP_CON1, 0x1 << 4,  0x1 << 4); /* I2S0 clock-gated */

		SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S, ECHOREF_SAMPLE_RATE);

		SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_2, true);
		u32Audio2ndI2sIn |= (Soc_Aud_LR_SWAP_NO_SWAP << 31);
		u32Audio2ndI2sIn |= (Soc_Aud_LOW_JITTER_CLOCK << 12);
		u32Audio2ndI2sIn |= (Soc_Aud_I2S_IN_PAD_SEL_I2S_IN_FROM_IO_MUX << 28);
		u32Audio2ndI2sIn |= (Soc_Aud_INV_LRCK_NO_INVERSE << 5);
		u32Audio2ndI2sIn |= (Soc_Aud_I2S_FORMAT_I2S << 3);
		u32Audio2ndI2sIn |= (Soc_Aud_I2S_WLEN_WLEN_32BITS << 1);
		Afe_Set_Reg(AFE_I2S_CON, u32Audio2ndI2sIn, MASK_ALL);

		Afe_Set_Reg(AUDIO_TOP_CON1, 0 << 4,  0x1 << 4); /* Clear I2S0 clock-gated */
		Set2ndI2SEnable(true);				 /* Enable I2S0 */

		pr_debug("%s(), AFE_I2S_CON = 0x%x, AFE_DAC_CON1 = 0x%x\n",
				__func__, Afe_Get_Reg(AFE_I2S_CON), Afe_Get_Reg(AFE_DAC_CON1));
	}

	set_interconnection(Soc_Aud_InterCon_Connection, is_use_smart_pa, is_use_general_src);

	if (is_use_general_src) {
		set_general_asrc_parameter(AUDIO_GENERAL_ASRC_1, ECHOREF_SAMPLE_RATE, SWIP_SAMPLE_RATE);
		set_general_asrc_enable(AUDIO_GENERAL_ASRC_1, true);
	}

	memif_rate = (is_use_general_src?SWIP_SAMPLE_RATE : ECHOREF_SAMPLE_RATE);
	SetSampleRate(vow_barge_in_mem_blk, memif_rate);

	EnableAfe(true);
}

static int mtk_vow_barge_in_pcm_prepare(struct snd_pcm_substream *substream)
{
	pr_debug("mtk_vow_barge_in_pcm_prepare(), substream->rate = %d, substream->channels = %d\n",
			substream->runtime->rate, substream->runtime->channels);
	return 0;
}

static int mtk_vow_barge_in_alsa_stop(struct snd_pcm_substream *substream)
{
	pr_debug("%s()\n", __func__);
	stop_vow_barge_in_hardware(substream);
	RemoveMemifSubStream(vow_barge_in_mem_blk, substream);
	return 0;
}

static int mtk_vow_barge_in_pcm_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *hw_params)
{
	struct snd_dma_buffer *dma_buf = &substream->dma_buffer;
	int ret = 0;
	unsigned int buffer_size = 0;

	buffer_size = params_buffer_bytes(hw_params);

	pr_debug("mtk_vow_barge_in_pcm_hw_params()\n");

	dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
	dma_buf->dev.dev = substream->pcm->card->dev;
	dma_buf->private_data = NULL;

	vow_barge_in_dma_buf.bytes = buffer_size;
	if (AllocateAudioSram(&vow_barge_in_dma_buf.addr,
			&vow_barge_in_dma_buf.area,
			vow_barge_in_dma_buf.bytes,
			substream,
			params_format(hw_params), false) == 0) {
		/* Using SRAM */
		SetHighAddr(vow_barge_in_mem_blk, false, vow_barge_in_dma_buf.addr);
	} else {
		/* Using DRAM, use SCP reserved DRAM that can be access by SCP */
		pr_debug("%s(), use DRAM\n", __func__);
		vow_barge_in_dma_buf.addr = scp_get_reserve_mem_phys(VOW_BARGEIN_MEM_ID);
		vow_barge_in_dma_buf.area = (uint8_t *)scp_get_reserve_mem_virt(VOW_BARGEIN_MEM_ID);
		vow_barge_in_dma_buf.bytes = buffer_size;
		SetHighAddr(vow_barge_in_mem_blk, true, vow_barge_in_dma_buf.addr);
		using_dram = true;
		AudDrv_Emi_Clk_On();
	}
	set_memif_addr(vow_barge_in_mem_blk,
		vow_barge_in_dma_buf.addr,
		vow_barge_in_dma_buf.bytes);
	pr_debug("%s(), addr = %llx, area = %p, bytes = %zu, using_dram = %u\n",
			__func__, vow_barge_in_dma_buf.addr, vow_barge_in_dma_buf.area,
			vow_barge_in_dma_buf.bytes, using_dram);

	return ret;
}

static int mtk_vow_barge_in_pcm_hw_free(struct snd_pcm_substream *substream)
{
	pr_debug("%s()\n", __func__);
	if (using_dram == true) {
		AudDrv_Emi_Clk_Off();
		using_dram = false;
	} else {
		freeAudioSram((void *)substream);
	}
	return 0;
}

static struct snd_pcm_hw_constraint_list vow_barge_in_constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
};

static int mtk_vow_barge_in_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	using_dram = false;
	vow_barge_in_mem_control_context = Get_Mem_ControlT(vow_barge_in_mem_blk);
	runtime->hw = mtk_vow_barge_in_hardware;
	memcpy((void *)(&(runtime->hw)), (void *)&mtk_vow_barge_in_hardware, sizeof(struct snd_pcm_hardware));

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
			&vow_barge_in_constraints_sample_rates);
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	if (ret < 0)
		pr_debug("snd_pcm_hw_constraint_integer failed\n");

	/* print for hw pcm information */
	pr_debug("mtk_vow_barge_in_pcm_open(), runtime rate = %d, channels = %d, substream->stream = %d\n",
			runtime->rate, runtime->channels, substream->stream);

	/* here open audio clocks */
	AudDrv_Clk_On();

	runtime->hw.info |= SNDRV_PCM_INFO_INTERLEAVED;
	runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	runtime->hw.info |= SNDRV_PCM_INFO_MMAP_VALID;

	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		pr_debug("substream->stream != SNDRV_PCM_STREAM_CAPTURE, return -1\n");

	if (ret < 0) {
		pr_debug("mtk_vow_barge_in_pcm_close\n");
		mtk_vow_barge_in_pcm_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_vow_barge_in_pcm_close(struct snd_pcm_substream *substream)
{
	pr_debug("mtk_vow_barge_in_pcm_close\n");
	AudDrv_Clk_Off();
	return 0;
}

static int mtk_vow_barge_in_alsa_start(struct snd_pcm_substream *substream)
{
	pr_debug("%s()\n", __func__);
	SetMemifSubStream(vow_barge_in_mem_blk, substream);
	start_vow_barge_in_hardware(substream);
	return 0;
}

static int mtk_vow_barge_in_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_vow_barge_in_alsa_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_vow_barge_in_alsa_stop(substream);
	}
	return -EINVAL;
}

static int mtk_vow_barge_in_pcm_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	return 0; /* do nothing */
}

static int mtk_vow_barge_in_pcm_silence(struct snd_pcm_substream *substream,
				   int channel, snd_pcm_uframes_t pos,
				   snd_pcm_uframes_t count)
{
	pr_debug("%s()\n", __func__);
	return 0; /* do nothing */
}

static void *dummy_page[2];

static struct page *mtk_vow_barge_in_pcm_page(struct snd_pcm_substream *substream,
					     unsigned long offset)
{
	pr_debug("%s()\n", __func__);
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static struct snd_pcm_ops mtk_vow_barge_in_ops = {
	.open =     mtk_vow_barge_in_pcm_open,
	.close =    mtk_vow_barge_in_pcm_close,
	.ioctl =    snd_pcm_lib_ioctl,
	.hw_params =    mtk_vow_barge_in_pcm_hw_params,
	.hw_free =  mtk_vow_barge_in_pcm_hw_free,
	.prepare =  mtk_vow_barge_in_pcm_prepare,
	.trigger =  mtk_vow_barge_in_pcm_trigger,
	.copy =     mtk_vow_barge_in_pcm_copy,
	.silence =  mtk_vow_barge_in_pcm_silence,
	.page =     mtk_vow_barge_in_pcm_page,
};

static struct snd_soc_platform_driver mtk_vow_barge_in_soc_platform = {
	.ops        = &mtk_vow_barge_in_ops,
	.probe      = mtk_afe_vow_barge_in_probe,
};

static int mtk_vow_barge_in_probe(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	deep_buffer_mem_blk_io = get_usage_digital_block_io(AUDIO_USAGE_DEEPBUFFER_PLAYBACK);
	if (deep_buffer_mem_blk_io < 0) {
		pr_debug("%s(), invalid mem blk io %d, no need to set intercon between memif and deep buffer output\n",
				__func__, deep_buffer_mem_blk_io);
	}
	smart_pa_in_blk_io = get_usage_digital_block_io(AUDIO_USAGE_SMART_PA_IN);
	if (smart_pa_in_blk_io < 0) {
		pr_debug("%s(), invalid smart pa io %d, no need to set intercon between memif and smart pa in\n",
				__func__, smart_pa_in_blk_io);
	}
	vow_barge_in_mem_blk_io = get_usage_digital_block_io(AUDIO_USAGE_VOW_BARGE_IN);
	if (vow_barge_in_mem_blk_io < 0) {
		pr_debug("%s(), invalid mem blk io %d, no need to set intercon between memif and smart pa in\n",
				__func__, vow_barge_in_mem_blk_io);
	}
	vow_barge_in_mem_blk = get_usage_digital_block(AUDIO_USAGE_VOW_BARGE_IN);
	if (vow_barge_in_mem_blk < 0) {
		pr_debug("%s(), invalid mem blk io %d, no memif support\n",
				__func__, vow_barge_in_mem_blk);
	}
	is_use_smart_pa = (mtk_spk_get_type() != MTK_SPK_NOT_SMARTPA);


	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);

	if (pdev->dev.dma_mask == NULL)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_VOW_BARGE_IN_PCM);

	pr_debug("%s(), dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_platform(&pdev->dev,
					 &mtk_vow_barge_in_soc_platform);
}

static int mtk_afe_vow_barge_in_probe(struct snd_soc_platform *platform)
{
	int ret = 0;

	pr_debug("%s()\n", __func__);
	vow_barge_in_dma_buf.area = dma_alloc_coherent(platform->dev,
							VOW_BARGE_IN_MAX_BUFFER_SIZE,
							&vow_barge_in_dma_buf.addr,
							GFP_KERNEL | GFP_DMA);
	if (!vow_barge_in_dma_buf.area)
		return -ENOMEM;
	vow_barge_in_dma_buf.bytes = VOW_BARGE_IN_MAX_BUFFER_SIZE;

	ret = snd_soc_add_platform_controls(platform, vow_barge_in_controls,
				      ARRAY_SIZE(vow_barge_in_controls));
	pr_debug("%s(), area = %p, register kcontrol ret = %d\n", __func__, vow_barge_in_dma_buf.area, ret);
	return 0;
}

static int mtk_vow_barge_in_remove(struct platform_device *pdev)
{
	pr_debug("%s()\n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_vow_barge_in_of_ids[] = {
	{ .compatible = "mediatek,mt_soc_pcm_vow_barge_in", },
	{}
};
#endif

static struct platform_driver mtk_vow_barge_in_driver = {
	.driver = {
		.name = MT_SOC_VOW_BARGE_IN_PCM,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_soc_pcm_vow_barge_in_of_ids,
#endif
	},
	.probe = mtk_vow_barge_in_probe,
	.remove = mtk_vow_barge_in_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_vow_barge_in_dev;
#endif

static int __init mtk_soc_vow_barge_in_platform_init(void)
{
	int ret = 0;

	pr_debug("%s()\n", __func__);
#ifndef CONFIG_OF
	soc_vow_barge_in_dev = platform_device_alloc(MT_SOC_VOW_BARGE_IN_PCM, -1);
	if (!soc_vow_barge_in_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_vow_barge_in_dev);
	if (ret != 0) {
		platform_device_put(soc_vow_barge_in_dev);
		return ret;
	}
#endif
	ret = platform_driver_register(&mtk_vow_barge_in_driver);
	return ret;
}

static void __exit mtk_soc_vow_barge_in_platform_exit(void)
{
	pr_debug("%s()\n", __func__);
	platform_driver_unregister(&mtk_vow_barge_in_driver);
}

module_init(mtk_soc_vow_barge_in_platform_init);
module_exit(mtk_soc_vow_barge_in_platform_exit);

MODULE_DESCRIPTION("VOW Barge-In module platform driver");
MODULE_LICENSE("GPL");
