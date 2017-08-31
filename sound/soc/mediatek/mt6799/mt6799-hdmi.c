/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program
 * If not, see <http://www.gnu.org/licenses/>.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mt6797_hdmi.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio hdmi playback
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
#include "mtk-soc-afe-control.h"
#include "mtk-soc-digital-type.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-soc-pcm-platform.h"
#include "mt6799-hdmi.h"
#include "mtk-soc-hdmi-type.h"

void mtk_Hdmi_Configuration_Init(void *hdmi_stream_format)
{
	memset_io(hdmi_stream_format, 0, sizeof(AudioHDMIFormat));
}

void mtk_Hdmi_Clock_Set(void *hdmi_stream_format)
{
	AudioHDMIFormat *hdmi_stream = (AudioHDMIFormat *) hdmi_stream_format;

	pr_warn("[+%s]\n", __func__);

	hdmi_stream->mI2Snum = Soc_Aud_I2S4;
	hdmi_stream->mI2S_MCKDIV = Soc_Aud_I2S4_MCKDIV;
	hdmi_stream->mI2S_BCKDIV = Soc_Aud_I2S4_BCKDIV;

	pr_warn("[-%s]\n", __func__);

}

void mtk_Hdmi_Configuration_Set(void *hdmi_stream_format, int rate,
		int channels, snd_pcm_format_t format, int displaytype)
{
	AudioHDMIFormat *hdmi_stream = (AudioHDMIFormat *) hdmi_stream_format;

	pr_warn("[%s] format=%d, displaytype=%d\n", __func__, format, displaytype);

	hdmi_stream->mHDMI_DisplayType = displaytype;
	hdmi_stream->mHDMI_Samplerate = rate;
	hdmi_stream->mHDMI_Channels = channels;


	/* SET hdmi channels , samplerate and formats */

	if (format == SNDRV_PCM_FORMAT_S32_LE
	    || format == SNDRV_PCM_FORMAT_U32_LE) {
		hdmi_stream->mMemIfFetchFormatPerSample = AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA;
		hdmi_stream->mHDMI_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		hdmi_stream->mTDM_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		hdmi_stream->mClock_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		hdmi_stream->mTDM_LRCK = ((Soc_Aud_I2S_WLEN_WLEN_32BITS + 1) * 16) - 1;
	} else {
		hdmi_stream->mMemIfFetchFormatPerSample = AFE_WLEN_16_BIT;
		hdmi_stream->mHDMI_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		hdmi_stream->mTDM_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		hdmi_stream->mClock_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_16BITS;
		hdmi_stream->mTDM_LRCK = ((Soc_Aud_I2S_WLEN_WLEN_16BITS + 1) * 16) - 1;
	}



	if (displaytype == HDMI_DISPLAY_SILMPORT) {	/* ANX7805 only I2s  */
		hdmi_stream->mHDMI_Channels = _ANX7805_SLIMPORT_CHANNEL;
		hdmi_stream->mClock_Data_Lens = Soc_Aud_I2S_WLEN_WLEN_32BITS;
		/* overwrite 16 bits settings for ANX7805 */
		hdmi_stream->mTDM_LRCK = ((Soc_Aud_I2S_WLEN_WLEN_32BITS + 1) * 16) - 1;
		/* overwrite 16 bits settings for ANX7805 */
		hdmi_stream->msDATA_Channels = _ANX7805_SLIMPORT_CHANNEL;
		hdmi_stream->mSdata0 = true;
		hdmi_stream->mSdata1 = true;
		hdmi_stream->mSdata2 = true;
		hdmi_stream->mSdata3 = true;
	} else {
		hdmi_stream->msDATA_Channels = channels;
		hdmi_stream->mSdata0 = true;
		hdmi_stream->mSdata1 = false;
		hdmi_stream->mSdata2 = false;
		hdmi_stream->mSdata3 = false;
	}
}

void mtk_Hdmi_Set_Interconnection(void *hdmi_stream_format)
{
	/*follow CEA861E (0x13)*/
	AudioHDMIFormat *hdmi_stream = (AudioHDMIFormat *) hdmi_stream_format;

	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I30,
			  Soc_Aud_InterConnectionOutput_O30);
	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I31,
			  Soc_Aud_InterConnectionOutput_O31);
	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I34,
			  Soc_Aud_InterConnectionOutput_O34);
	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I35,
			  Soc_Aud_InterConnectionOutput_O35);
	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I36,
			  Soc_Aud_InterConnectionOutput_O36);
	SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I37,
			  Soc_Aud_InterConnectionOutput_O37);

	if (hdmi_stream->mHDMI_DisplayType == HDMI_DISPLAY_SILMPORT) {
		SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I32,
			Soc_Aud_InterConnectionOutput_O32);
		SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I33,
			Soc_Aud_InterConnectionOutput_O33);
	} else {
		SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I32,
			Soc_Aud_InterConnectionOutput_O33);	/* O33 center */
		SetHDMIConnection(Soc_Aud_InterCon_Connection, Soc_Aud_InterConnectionInput_I33,
			Soc_Aud_InterConnectionOutput_O32);	/* O32 LFE */
	}
}

void mtk_Hdmi_Set_Sdata(void *hdmi_stream_format)
{
	AudioHDMIFormat *hdmi_stream = (AudioHDMIFormat *) hdmi_stream_format;

	if (hdmi_stream->mSdata0)
		Afe_Set_Reg(AFE_TDM_CON2, 0, 0x0000000f);	/*  0: Channel starts from O30/O31. */
	else
		Afe_Set_Reg(AFE_TDM_CON2, 4, 0x0000000f);	/*  0: Channel 0 */

	if (hdmi_stream->mSdata1)
		Afe_Set_Reg(AFE_TDM_CON2, 1 << 4, 0x000000f0);	/*  1: Channel O32/O33 */
	else
		Afe_Set_Reg(AFE_TDM_CON2, 4 << 4, 0x000000f0);	/*  1: Channel 0 */

	if (hdmi_stream->mSdata2)
		Afe_Set_Reg(AFE_TDM_CON2, 2 << 8, 0x00000f00);	/*  2: Channel O34/O35 */
	else
		Afe_Set_Reg(AFE_TDM_CON2, 4 << 8, 0x00000f00);	/*  2: Channel 0 */

	if (hdmi_stream->mSdata3)
		Afe_Set_Reg(AFE_TDM_CON2, 3 << 12, 0x0000f000); /*  3: Channel O36/O37 */
	else
		Afe_Set_Reg(AFE_TDM_CON2, 4 << 12, 0x0000f000); /*  3: Channel 0 */
}

void SetHDMIClockInverse(void)
{
	Afe_Set_Reg(AUDIO_TOP_CON3, 1 << 3, 1 << 3);	/* must inverse HDMI BCLK */
}

void SetHDMIDebugEnable(bool enable)
{
	if (enable) {
		Afe_Set_Reg(AFE_TDM_CON2, 0, 0x00600000);	/*  select loopback sdata0 */
		Afe_Set_Reg(AFE_TDM_CON2, 1 << 20, 0x00100000); /* enable TDM to I2S path */
	} else {
		Afe_Set_Reg(AFE_TDM_CON2, 0, 0x00100000);	/* disable TDM to I2S path */
		Afe_Set_Reg(AFE_I2S_CON, 0, 0x00000001);	/* I2S disable */
	}
}

void SetHDMIDumpReg(void)
{
	kal_uint32 volatile u4RegValue;
	kal_uint32 volatile u4tmpValue;
	kal_uint32 volatile u4tmpValue1;
	kal_uint32 volatile u4tmpValue2;
	kal_uint32 volatile u4tmpClkdiv0;
	kal_uint32 volatile u4tmpClkdiv1;
	kal_uint32 volatile u4tmpClkdiv2;
	kal_uint32 volatile u4tmpHDMode;
	kal_uint32 volatile u4tmpHDAlign;
	kal_uint32 volatile u4RegDAC;

	u4RegValue = Afe_Get_Reg(AFE_IRQ_MCU_STATUS);
	u4RegValue &= 0xffffffff;

	u4tmpValue = Afe_Get_Reg(AFE_HDMI_OUT_CON0);
	u4tmpValue &= 0xffffffff;

	u4tmpValue1 = Afe_Get_Reg(AFE_TDM_CON1);
	u4tmpValue1 &= 0xffffffff;

	u4tmpValue2 = Afe_Get_Reg(AFE_TDM_CON2);
	u4tmpValue2 &= 0xffffffff;

	u4tmpClkdiv0 = Afe_Get_Reg(CLK_AUDDIV_0);
	u4tmpClkdiv1 = Afe_Get_Reg(CLK_AUDDIV_1);
	u4tmpClkdiv2 = Afe_Get_Reg(CLK_AUDDIV_2);

	u4tmpHDMode = Afe_Get_Reg(AFE_MEMIF_HD_MODE);
	u4tmpHDAlign = Afe_Get_Reg(AFE_MEMIF_HDALIGN);
	u4RegDAC = Afe_Get_Reg(AFE_DAC_CON0);

	pr_warn
	    ("%s IRQ =0x%x, HDMI_CON0= 0x%x, TDM_CON1=0x%x, TDM_CON2 =0x%x\n",
		__func__, u4RegValue, u4tmpValue, u4tmpValue1, u4tmpValue2);

	pr_warn
	    ("%s DIV0=0x%x,DIV1=0x%x,DIV2=0x%x,Mode=0x%x,Align=0x%x, DAC_CON0=0x%x\n",
		__func__, u4tmpClkdiv0, u4tmpClkdiv1, u4tmpClkdiv2, u4tmpHDMode, u4tmpHDAlign, u4RegDAC);


}

#if 0

bool SetHDMIMCLK(void)
{
	uint32 mclksamplerate = mHDMIOutput->mSampleRate * 256;
	uint32 hdmi_APll = GetHDMIApLLSource();
	uint32 hdmi_mclk_div = 0;

	pr_debug("%s\n", __func__);

	if (hdmi_APll == APLL_SOURCE_24576)
		hdmi_APll = 24576000;
	else
		hdmi_APll = 22579200;

	pr_warn("%s hdmi_mclk_div = %d mclksamplerate = %d\n", __func__, hdmi_mclk_div,
		mclksamplerate);
	hdmi_mclk_div = (hdmi_APll / mclksamplerate / 2) - 1;
	mHDMIOutput->mHdmiMckDiv = hdmi_mclk_div;
	pr_debug("%s hdmi_mclk_div = %d\n", __func__, hdmi_mclk_div);
	SetCLkMclk(Soc_Aud_I2S4, mHDMIOutput->mSampleRate);

	return true;
}

bool SetHDMIBCLK(void)
{
	mHDMIOutput->mBckSamplerate = mHDMIOutput->mSampleRate * mHDMIOutput->mChannels;
	pr_debug("%s mBckSamplerate = %d mSampleRate = %d mChannels = %d\n", __func__,
		mHDMIOutput->mBckSamplerate, mHDMIOutput->mSampleRate, mHDMIOutput->mChannels);
	mHDMIOutput->mBckSamplerate *= (mHDMIOutput->mI2S_WLEN + 1) * 16;
	pr_debug("%s mBckSamplerate = %d mApllSamplerate = %d\n", __func__,
		mHDMIOutput->mBckSamplerate, mHDMIOutput->mApllSamplerate);
	mHDMIOutput->mHdmiBckDiv =
	    (mHDMIOutput->mApllSamplerate / mHDMIOutput->mBckSamplerate / 2) - 1;
	pr_debug("%s mHdmiBckDiv = %d\n", __func__, mHDMIOutput->mHdmiBckDiv);

	return true;
}

uint32 GetHDMIApLLSource(void)
{
	pr_debug("%s ApllSource = %d\n", __func__, mHDMIOutput->mApllSource);
	return mHDMIOutput->mApllSource;
}

bool SetHDMIApLL(uint32 ApllSource)
{
	pr_warn("%s ApllSource = %d", __func__, ApllSource);

	if (ApllSource == APLL_SOURCE_24576) {
		mHDMIOutput->mApllSource = APLL_SOURCE_24576;
		mHDMIOutput->mApllSamplerate = 24576000;
	} else if (ApllSource == APLL_SOURCE_225792) {
		mHDMIOutput->mApllSource = APLL_SOURCE_225792;
		mHDMIOutput->mApllSamplerate = 22579200;
	}

	return true;
}

bool SetHDMIsamplerate(uint32 samplerate)
{
	uint32 SampleRateinedx = SampleRateTransform(samplerate, Soc_Aud_Digital_Block_MEM_HDMI);

	mHDMIOutput->mSampleRate = samplerate;
	pr_debug("%s samplerate = %d\n", __func__, samplerate);
	switch (SampleRateinedx) {
	case Soc_Aud_I2S_SAMPLERATE_I2S_8K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_11K:
		SetHDMIApLL(APLL_SOURCE_225792);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_12K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_16K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_22K:
		SetHDMIApLL(APLL_SOURCE_225792);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_24K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_32K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_44K:
		SetHDMIApLL(APLL_SOURCE_225792);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_48K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_88K:
		SetHDMIApLL(APLL_SOURCE_225792);
		break;
	case Soc_Aud_I2S_SAMPLERATE_I2S_96K:
		SetHDMIApLL(APLL_SOURCE_24576);
		break;
	default:
		break;
	}

	return true;
}
#endif

bool SetHDMIdatalength(uint32 length)
{

	pr_warn("%s length = %d\n ", __func__, length);
	Afe_Set_Reg(AFE_HDMI_OUT_CON0, (length << 1), 1 << 1);

	return true;
}

bool SetTDMLrckWidth(uint32 cycles)
{
	pr_warn("%s cycles = %d", __func__, cycles);
	Afe_Set_Reg(AFE_TDM_CON1, cycles << 24, 0xff000000);
	return true;
}

bool SetTDMbckcycle(uint32 cycles)
{
	uint32 index = 0;

	pr_warn("%s cycles = %d\n", __func__, cycles);
	switch (cycles) {
	case Soc_Aud_I2S_WLEN_WLEN_16BITS: {
		index = 0;
	}
	break;
	case Soc_Aud_I2S_WLEN_WLEN_32BITS: {
		index = 2;
	}
	break;
	default:
		index = 2;
		break;
	}
	Afe_Set_Reg(AFE_TDM_CON1, index << 12, 0x00003000);
	return true;
}

bool SetTDMChannelsSdata(uint32 channels)
{
	uint32 index = 0;

	pr_warn("%s channels = %d", __func__, channels);
	switch (channels) {
	case 2:
		index = 0;
		break;
	case 4:
		index = 1;
		break;
	case 8:
		index = 2;
		break;
	}
	Afe_Set_Reg(AFE_TDM_CON1, index << 10, 0x00000c00);
	return true;
}

bool SetTDMDatalength(uint32 length)
{
	pr_warn("%s length = %d\n", __func__, length);
	if (length == Soc_Aud_I2S_WLEN_WLEN_16BITS)
		Afe_Set_Reg(AFE_TDM_CON1, 1 << 8, 0x00000300);
	else if (length == Soc_Aud_I2S_WLEN_WLEN_32BITS)
		Afe_Set_Reg(AFE_TDM_CON1, 2 << 8, 0x00000300);
	return true;
}

bool SetTDMI2Smode(uint32 mode)
{
	pr_warn("%s mode = %d", __func__, mode);
	if (mode == Soc_Aud_I2S_FORMAT_EIAJ)
		Afe_Set_Reg(AFE_TDM_CON1, 0 << 3, 1 << 3);
	else if (mode == Soc_Aud_I2S_FORMAT_I2S)
		Afe_Set_Reg(AFE_TDM_CON1, 1 << 3, 1 << 3);

	Afe_Set_Reg(AFE_TDM_CON1, 1 << 4, 1 << 4);
	return true;
}

bool SetTDMLrckInverse(bool enable)
{
	pr_warn("%s enable = %d", __func__, enable);
	if (enable)
		Afe_Set_Reg(AFE_TDM_CON1, 1 << 2, 1 << 2);
	else
		Afe_Set_Reg(AFE_TDM_CON1, 0, 1 << 2);

	return true;
}

bool SetTDMBckInverse(bool enable)
{
	pr_warn("%s enable = %d", __func__, enable);
	if (enable)
		Afe_Set_Reg(AFE_TDM_CON1, 1 << 1, 1 << 1);
	else
		Afe_Set_Reg(AFE_TDM_CON1, 0, 1 << 1);

	return true;
}

bool SetTDMEnable(bool enable)
{
	pr_warn("%s enable = %d", __func__, enable);
	if (enable)
		Afe_Set_Reg(AFE_TDM_CON1, 1, 1);
	else
		Afe_Set_Reg(AFE_TDM_CON1, 0, 1);

	return true;
}

bool SetTDMDataChannels(uint32 SData, uint32 SDataChannels)
{
	int index = 0;

	pr_warn("%s SData = %d SDataChannels = %d", __func__, SData, SDataChannels);
	switch (SData) {
	case HDMI_SDATA0:
		index = 0;
		break;
	case HDMI_SDATA1:
		index = 4;
		break;
	case HDMI_SDATA2:
		index = 8;
		break;
	case HDMI_SDATA3:
		index = 12;
		break;
	default:
		break;
	}
	Afe_Set_Reg(AFE_TDM_CON2, SDataChannels << index, 1 << 0x7 << index);
	return true;
}

bool SetTDMtoI2SEnable(bool enable)
{
	pr_warn("%s enable = %d", __func__, enable);
	if (enable)
		Afe_Set_Reg(AFE_TDM_CON2, 1 << 20, 1 << 20);
	else
		Afe_Set_Reg(AFE_TDM_CON2, 0, 1 << 20);

	return true;
}

bool SetHDMIChannels(uint32 Channels)
{
	pr_warn("+%s(), Channels = %d\n", __func__, Channels);
	Afe_Set_Reg(AFE_HDMI_OUT_CON0, (Channels << 4), 0x00f0);

	return true;
}

bool SetHDMIEnable(bool bEnable)
{
	pr_warn("+%s(), bEnable = %d\n", __func__, bEnable);
	Afe_Set_Reg(AFE_HDMI_OUT_CON0, bEnable, 0x0001);

	return true;
}


bool SetHDMIConnection(uint32 ConnectionState, uint32 Input, uint32 Output)
{
	pr_warn("+%s(), Input = %d, Output = %d\n", __func__, Input, Output);
	switch (Output) {
	case Soc_Aud_InterConnectionOutput_O30:
		Afe_Set_Reg(AFE_HDMI_CONN0, Input, 0x0007);
		break;
	case Soc_Aud_InterConnectionOutput_O31:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 3), (0x0007 << 3));
		break;
	case Soc_Aud_InterConnectionOutput_O32:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 6), (0x0007 << 6));
		break;
	case Soc_Aud_InterConnectionOutput_O33:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 9), (0x0007 << 9));
		break;
	case Soc_Aud_InterConnectionOutput_O34:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 12), (0x0007 << 12));
		break;
	case Soc_Aud_InterConnectionOutput_O35:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 15), (0x0007 << 15));
		break;
	case Soc_Aud_InterConnectionOutput_O36:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 18), (0x0007 << 18));
		break;
	case Soc_Aud_InterConnectionOutput_O37:
		Afe_Set_Reg(AFE_HDMI_CONN0, (Input << 21), (0x0007 << 21));
		break;
	default:
		break;
	}
	return true;
}


