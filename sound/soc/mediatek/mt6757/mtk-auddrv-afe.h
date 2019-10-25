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
 */

/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudioAfe.h
 *
 * Project:
 * --------
 *   MT6757  Audio Driver Afe Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 *   Ir Lian (mtk00976)
 *   Harvey Huang (mtk03996)
 *   Chipeng Chang (mtk02308)
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

#ifndef _AUDDRV_AFE_H_
#define _AUDDRV_AFE_H_

#include "mtk-auddrv-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-soc-digital-type.h"
#include <linux/types.h>

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/

#define AUDIO_VCOREFS_SUPPORT

/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

enum mem_interface {
	AFE_MEM_NONE = 0,
	AFE_MEM_DL1,
	AFE_MEM_DL1_DATA2,
	AFE_MEM_DL2,
	AFE_MEM_VUL,
	AFE_MEM_DAI,
	AFE_MEM_I2S,
	AFE_MEM_AWB,
	AFE_MEM_MOD_PCM,
};

enum sample_rate {
	AFE_8000HZ = 0,
	AFE_11025HZ = 1,
	AFE_12000HZ = 2,
	AFE_16000HZ = 3,
	AFE_22050HZ = 4,
	AFE_24000HZ = 5,
	AFE_32000HZ = 6,
	AFE_44100HZ = 7,
	AFE_48000HZ = 8
};

enum daimod_sample_rate {
	AFE_DAIMOD_8000HZ = 0x0,
	AFE_DAIMOD_16000HZ = 0x1,
};

enum memif_ch_cfg { AFE_STEREO = 0x0, AFE_MONO = 0x1 };

enum memif_mono_sel { AFE_MONO_USE_L = 0x0, AFE_MONO_USE_R = 0x1 };

enum memif_dup_write { AFE_DUP_WR_DISABLE = 0x0, AFE_DUP_WR_ENABLE = 0x1 };

struct memif_buf_t {
	unsigned int u4AFE_MEMIF_BUF_BASE;
	unsigned int u4AFE_MEMIF_BUF_END;
	unsigned int u4AFE_MEMIF_BUF_WP;
	unsigned int u4AFE_MEMIF_BUF_RP;
};

struct memif_config_t {
	enum mem_interface eMemInterface;
	enum sample_rate eSamplingRate;
	enum daimod_sample_rate eDaiModSamplingRate;
	enum memif_ch_cfg eChannelConfig;
	enum memif_mono_sel
	    eMonoSelect; /* Used when AWB and VUL and data is mono */
	enum memif_dup_write eDupWrite; /* Used when MODPCM and DAI */
	struct memif_buf_t rBufferSetting;
};

/* I2S related */
enum i2s_fmt { I2S_EIAJ = 0x0, I2S_I2S = 0x1 };

enum i2s_wlen { I2S_16BIT = 0x0, I2S_32BIT = 0x1 };

enum i2s_swap { I2S_NOSWAP = 0x0, I2S_LRSWAP = 0x1 };

enum i2s_en { I2S_DISABLE = 0x0, I2S_ENABLE = 0x1 };

enum i2s_src { I2S_MASTER = 0x0, I2S_SLAVE = 0x1 };

enum i2s_dir { I2S_OUT = 0x0, I2S_IN = 0x1 };

/* PCM related */
enum pcm_module {
	PCM_1 = 0x0, /* (O7, O8, I9) */
	PCM_2 = 0x1  /* (O17, O18, I14) */
};

enum pcm_en { PCM_DISABLE = 0x0, PCM_ENABLE = 0x1 };

enum pcm_fmt {
	PCM_I2S = 0x0,
	PCM_EIAJ = 0x1,
	PCM_MODEA = 0x2,
	PCM_MODEB = 0x3
};

enum pcm_mode { PCM_8K = 0x0, PCM_16K = 0x1 };

enum pcm_wlen { PCM_16BIT = 0x0, PCM_32BIT = 0x1 };

enum pcm_clk_src { PCM_MASTER = 0x0, PCM_SLAVE = 0x1 };

enum pcm_bypass_src {
	PCM_GO_ASRC = 0x0, /* (ASRC)       Set to 0 when source & destination
			      uses different crystal */
	PCM_GO_ASYNC_FIFO = 0x1 /* (Async FIFO) Set to 1 when source &
				   destination uses same crystal */
};

enum pcm_bt_mode {
	PCM_DMTX = 0x0, /* dual mic on TX */
	PCM_SMTX = 0x1  /* single mic on TX (In BT mode, only L channel data is
			   sent on PCM TX.) */
};

enum pcm_sync_type { PCM_SYNC_LEN_1_BCK = 0x0, PCM_SYNC_LEN_N_BCK = 0x1 };

enum pcm_ext_modem { PCM_INT_MD = 0x0, PCM_EXT_MD = 0x1 };

enum pcm_vbt_16k_mode {
	PCM_VBT_16K_MODE_DISABLE = 0x0,
	PCM_VBT_16K_MODE_ENABLE = 0x1
};

enum pcm_clk_inv { PCM_NOINV = 0x0, PCM_INV = 0x1 };

enum pcm_loop_ena { PCM_LB_DISABLE = 0x0, PCM_LB_ENABLE = 0x1 };

enum pcm_txfix_en { PCM_TXFIX_OFF = 0x0, PCM_TXFIX_ON = 0x1 };

struct pcm_info_t {
	enum pcm_fmt ePcmFmt;
	enum pcm_mode ePcm8k16kmode;
	enum pcm_wlen ePcmWlen;
	enum pcm_clk_src ePcmClkSrc;
	enum pcm_bypass_src ePcmBypassASRC;
	enum pcm_ext_modem ePcmModemSel;
	enum pcm_vbt_16k_mode ePcmVbt16kSel;
};

/* BT PCM */
enum bt_pcm_en { BTPCM_DISABLE = 0x0, BTPCM_ENABLE = 0x1 };

enum bt_pcm_mode { BTPCM_8K = 0x0, BTPCM_16K = 0x1 };

/* Interconnection related */
enum interconnection_in {
	I00 = 0,
	I01 = 1,
	I02 = 2,
	I03 = 3,
	I04 = 4,
	I05 = 5,
	I06 = 6,
	I07 = 7,
	I08 = 8,
	I09 = 9,
	IN_MAX
};

enum interconnection_out {
	O00 = 0,
	O01 = 1,
	O02 = 2,
	O03 = 3,
	O04 = 4,
	O05 = 5,
	O06 = 6,
	O07 = 7,
	O08 = 8,
	O09 = 9,
	O010 = 10,
	O011 = 11,
	O012 = 12,
	OUT_MAX
};

/* Side tone filter related */
enum stf_src {
	I3I4 = 0,
	HW_SINE = 1,
	I5I6 = 2,
};

/* Sine wave generator related */
enum sine_tone_ch {
	SINE_TONE_CH1 = 0,
	SINE_TONE_CH2 = 1,
	SINE_TONE_STEREO = 2
};

enum sine_tone_amp {
	SINE_TONE_128 = 0,
	SINE_TONE_64 = 1,
	SINE_TONE_32 = 2,
	SINE_TONE_16 = 3,
	SINE_TONE_8 = 4,
	SINE_TONE_4 = 5,
	SINE_TONE_2 = 6,
	SINE_TONE_1 = 7
};

enum sine_tone_mode {
	SINE_TONE_8K = 0,
	SINE_TONE_11K = 1,
	SINE_TONE_12K = 2,
	SINE_TONE_16K = 3,
	SINE_TONE_22K = 4,
	SINE_TONE_24K = 5,
	SINE_TONE_32K = 6,
	SINE_TONE_44K = 7,
	SINE_TONE_48K = 8,
	SINE_TONE_LOOPBACK = 9
};

enum sine_tone_loopback {
	SINE_TONE_LOOPBACK_I0_I1 = 0,
	SINE_TONE_LOOPBACK_I2 = 1,
	SINE_TONE_LOOPBACK_I3_I4 = 2,
	SINE_TONE_LOOPBACK_I5_I6 = 3,
	SINE_TONE_LOOPBACK_I7_I8 = 4,
	SINE_TONE_LOOPBACK_I9_I10 = 5,
	SINE_TONE_LOOPBACK_I11_I12 = 6,
	SINE_TONE_LOOPBACK_O0_O1 = 7,
	SINE_TONE_LOOPBACK_O2 = 8,
	SINE_TONE_LOOPBACK_O3_O4 = 9,
	SINE_TONE_LOOPBACK_O5_O6 = 10,
	SINE_TONE_LOOPBACK_O7_O8 = 11,
	SINE_TONE_LOOPBACK_O9_O10 = 12,
	SINE_TONE_LOOPBACK_O11 = 13,
	SINE_TONE_LOOPBACK_O12 = 14
};

struct afe_sinegen_info_t {
	unsigned int u4ch1_freq_div; /* 64/n sample/period */
	enum sine_tone_amp rch1_amp_div;
	enum sine_tone_mode rch1_sine_mode;
	unsigned int u4ch2_freq_div; /* 64/n sample/period */
	enum sine_tone_amp rch2_amp_div;
	enum sine_tone_mode rch2_sine_mode;
	enum sine_tone_loopback rloopback_mode;
};

/*****************************************************************************
 *                          C O N S T A N T S
 *****************************************************************************/
#define AUDIO_HW_PHYSICAL_BASE (0x11220000L)
#define AUDIO_CLKCFG_PHYSICAL_BASE (0x10000000L)
/* need enable this register before access all register */
#define AUDIO_POWER_TOP (0x1000629cL)
#define AUDIO_INFRA_BASE (0x10001000L)
#define AUDIO_HW_VIRTUAL_BASE (0xF1220000L)

#define APMIXEDSYS_BASE (0x1000C000L)
/* Register TOP */
/* We need to write AP_PLL_CON5 to set mux, thus we can set APLL Tuner in AFE
 * setting */
#define AP_PLL_CON5 (0x0014)

#ifdef AUDIO_MEM_IOREMAP
#define AFE_BASE (0L)
#else
#define AFE_BASE (AUDIO_HW_VIRTUAL_BASE)
#endif

/* Internal sram */
#define AFE_INTERNAL_SRAM_PHY_BASE (0x11221000L)
#define AFE_INTERNAL_SRAM_VIR_BASE (AUDIO_HW_VIRTUAL_BASE - 0x70000 + 0x8000)
#define AFE_INTERNAL_SRAM_NORMAL_SIZE (0xC000) /* 48k, for normal mode */
#define AFE_INTERNAL_SRAM_COMPACT_SIZE (0x8000)
#define AFE_INTERNAL_SRAM_SIZE AFE_INTERNAL_SRAM_NORMAL_SIZE

/* Dram */
#define AFE_EXTERNAL_DRAM_SIZE (0xC000) /* 48k */

/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/

#define AUD_GPIO_BASE (0xF0005000L)
#define AUD_GPIO_MODE39 (0x860)
#define AUD_DRV_SEL4 (0xB40)

#define APLL_PHYSICAL_BASE (0x10209000L)

#define AUDIO_CLK_CFG_4 (0x0080)
#define AUDIO_CLK_CFG_6 (0x00A0)
#define AUDIO_CLK_CFG_7 (0x00B0)
#define AUDIO_CLK_CFG_8 (0x00C0)
#define AUDIO_CG_SET (0x88)
#define AUDIO_CG_CLR (0x8c)
#define AUDIO_CG_STATUS (0x94)

/* apmixed sys */
#define APLL1_CON0 0x02a0
#define APLL1_CON1 0x02a4
#define APLL1_CON2 0x02a8
#define APLL1_CON3 0x02ac

#define APLL2_CON0 0x02b4
#define APLL2_CON1 0x02b8
#define APLL2_CON2 0x02bc
#define APLL2_CON3 0x02c0

#define AUDIO_TOP_CON0 (AFE_BASE + 0x0000)
#define AUDIO_TOP_CON1 (AFE_BASE + 0x0004)
#define AUDIO_TOP_CON3 (AFE_BASE + 0x000c)
#define AFE_DAC_CON0 (AFE_BASE + 0x0010)
#define AFE_DAC_CON1 (AFE_BASE + 0x0014)
#define AFE_I2S_CON (AFE_BASE + 0x0018)
#define AFE_DAIBT_CON0 (AFE_BASE + 0x001c)
#define AFE_CONN0 (AFE_BASE + 0x0020)
#define AFE_CONN1 (AFE_BASE + 0x0024)
#define AFE_CONN2 (AFE_BASE + 0x0028)
#define AFE_CONN3 (AFE_BASE + 0x002c)
#define AFE_CONN4 (AFE_BASE + 0x0030)
#define AFE_I2S_CON1 (AFE_BASE + 0x0034)
#define AFE_I2S_CON2 (AFE_BASE + 0x0038)
#define AFE_MRGIF_CON (AFE_BASE + 0x003c)
#define AFE_DL1_BASE (AFE_BASE + 0x0040)
#define AFE_DL1_CUR (AFE_BASE + 0x0044)
#define AFE_DL1_END (AFE_BASE + 0x0048)
#define AFE_I2S_CON3 (AFE_BASE + 0x004c)
#define AFE_DL2_BASE (AFE_BASE + 0x0050)
#define AFE_DL2_CUR (AFE_BASE + 0x0054)
#define AFE_DL2_END (AFE_BASE + 0x0058)
#define AFE_CONN5 (AFE_BASE + 0x005c)
#define AFE_CONN_24BIT (AFE_BASE + 0x006c)
#define AFE_AWB_BASE (AFE_BASE + 0x0070)
#define AFE_AWB_END (AFE_BASE + 0x0078)
#define AFE_AWB_CUR (AFE_BASE + 0x007c)
#define AFE_VUL_BASE (AFE_BASE + 0x0080)
#define AFE_VUL_END (AFE_BASE + 0x0088)
#define AFE_VUL_CUR (AFE_BASE + 0x008c)
#define AFE_DAI_BASE (AFE_BASE + 0x0090)
#define AFE_DAI_END (AFE_BASE + 0x0098)
#define AFE_DAI_CUR (AFE_BASE + 0x009c)
#define AFE_CONN6 (AFE_BASE + 0x00bc)
#define AFE_MEMIF_MSB (AFE_BASE + 0x00cc)
#define AFE_MEMIF_MON0 (AFE_BASE + 0x00d0)
#define AFE_MEMIF_MON1 (AFE_BASE + 0x00d4)
#define AFE_MEMIF_MON2 (AFE_BASE + 0x00d8)
#define AFE_MEMIF_MON4 (AFE_BASE + 0x00e0)
#define AFE_MEMIF_MON5 (AFE_BASE + 0x00e4)
#define AFE_MEMIF_MON6 (AFE_BASE + 0x00e8)
#define AFE_MEMIF_MON7 (AFE_BASE + 0x00ec)
#define AFE_MEMIF_MON8 (AFE_BASE + 0x00f0)
#define AFE_MEMIF_MON9 (AFE_BASE + 0x00f4)
#define AFE_ADDA_DL_SRC2_CON0 (AFE_BASE + 0x0108)
#define AFE_ADDA_DL_SRC2_CON1 (AFE_BASE + 0x010c)
#define AFE_ADDA_UL_SRC_CON0 (AFE_BASE + 0x0114)
#define AFE_ADDA_UL_SRC_CON1 (AFE_BASE + 0x0118)
#define AFE_ADDA_TOP_CON0 (AFE_BASE + 0x0120)
#define AFE_ADDA_UL_DL_CON0 (AFE_BASE + 0x0124)
#define AFE_ADDA_SRC_DEBUG (AFE_BASE + 0x012c)
#define AFE_ADDA_SRC_DEBUG_MON0 (AFE_BASE + 0x0130)
#define AFE_ADDA_SRC_DEBUG_MON1 (AFE_BASE + 0x0134)
#define AFE_ADDA_NEWIF_CFG0 (AFE_BASE + 0x0138)
#define AFE_ADDA_NEWIF_CFG1 (AFE_BASE + 0x013c)
#define AFE_SIDETONE_DEBUG (AFE_BASE + 0x01d0)
#define AFE_SIDETONE_MON (AFE_BASE + 0x01d4)
#define AFE_SIDETONE_CON0 (AFE_BASE + 0x01e0)
#define AFE_SIDETONE_COEFF (AFE_BASE + 0x01e4)
#define AFE_SIDETONE_CON1 (AFE_BASE + 0x01e8)
#define AFE_SIDETONE_GAIN (AFE_BASE + 0x01ec)
#define AFE_SGEN_CON0 (AFE_BASE + 0x01f0)
#define AFE_TOP_CON0 (AFE_BASE + 0x0200)
#define AFE_BUS_CFG (AFE_BASE + 0x0240)
#define AFE_BUS_MON (AFE_BASE + 0x0244)
#define AFE_ADDA_PREDIS_CON0 (AFE_BASE + 0x0260)
#define AFE_ADDA_PREDIS_CON1 (AFE_BASE + 0x0264)
#define AFE_MRGIF_MON0 (AFE_BASE + 0x0270)
#define AFE_MRGIF_MON1 (AFE_BASE + 0x0274)
#define AFE_MRGIF_MON2 (AFE_BASE + 0x0278)
#define AFE_DAC_CON2 (AFE_BASE + 0x02e0)
#define AFE_VUL2_BASE (AFE_BASE + 0x02f0)
#define AFE_VUL2_END (AFE_BASE + 0x02f8)
#define AFE_VUL2_CUR (AFE_BASE + 0x02fc)
#define AFE_MOD_DAI_BASE (AFE_BASE + 0x0330)
#define AFE_MOD_DAI_END (AFE_BASE + 0x0338)
#define AFE_MOD_DAI_CUR (AFE_BASE + 0x033c)
#define AFE_VUL_D2_BASE (AFE_BASE + 0x0350)
#define AFE_VUL_D2_END (AFE_BASE + 0x0358)
#define AFE_VUL_D2_CUR (AFE_BASE + 0x035c)
#define AFE_VUL12_BASE (AFE_VUL_D2_BASE) /* AFE_VUL_D2 in MT6797*/
#define AFE_VUL12_END (AFE_VUL_D2_END)
#define AFE_VUL12_CUR (AFE_VUL_D2_CUR)
#define AFE_IRQ_MCU_CON (AFE_BASE + 0x03a0)
#define AFE_IRQ_MCU_STATUS (AFE_BASE + 0x03a4)
#define AFE_IRQ_MCU_CLR (AFE_BASE + 0x03a8)
#define AFE_IRQ_MCU_CNT1 (AFE_BASE + 0x03ac)
#define AFE_IRQ_MCU_CNT2 (AFE_BASE + 0x03b0)
#define AFE_IRQ_MCU_EN (AFE_BASE + 0x03b4)
#define AFE_IRQ_MCU_MON2 (AFE_BASE + 0x03b8)
#define AFE_IRQ1_MCU_CNT_MON (AFE_BASE + 0x03c0)
#define AFE_IRQ2_MCU_CNT_MON (AFE_BASE + 0x03c4)
#define AFE_IRQ1_MCU_EN_CNT_MON (AFE_BASE + 0x03c8)
#define AFE_MEMIF_MINLEN (AFE_BASE + 0x03d0)
#define AFE_MEMIF_MAXLEN (AFE_BASE + 0x03d4)
#define AFE_MEMIF_PBUF_SIZE (AFE_BASE + 0x03d8)
#define AFE_IRQ_MCU_CNT7 (AFE_BASE + 0x03dc)
#define AFE_APLL1_TUNER_CFG (AFE_BASE + 0x03f0)
#define AFE_APLL2_TUNER_CFG (AFE_BASE + 0x03f4)
#define AFE_CONN33 (AFE_BASE + 0x0408)
#define AFE_GAIN1_CON0 (AFE_BASE + 0x0410)
#define AFE_GAIN1_CON1 (AFE_BASE + 0x0414)
#define AFE_GAIN1_CON2 (AFE_BASE + 0x0418)
#define AFE_GAIN1_CON3 (AFE_BASE + 0x041c)
#define AFE_CONN7 (AFE_BASE + 0x0420)
#define AFE_GAIN1_CUR (AFE_BASE + 0x0424)
#define AFE_GAIN2_CON0 (AFE_BASE + 0x0428)
#define AFE_GAIN2_CON1 (AFE_BASE + 0x042c)
#define AFE_GAIN2_CON2 (AFE_BASE + 0x0430)
#define AFE_GAIN2_CON3 (AFE_BASE + 0x0434)
#define AFE_CONN8 (AFE_BASE + 0x0438)
#define AFE_GAIN2_CUR (AFE_BASE + 0x043c)
#define AFE_CONN9 (AFE_BASE + 0x0440)
#define AFE_CONN10 (AFE_BASE + 0x0444)
#define AFE_CONN11 (AFE_BASE + 0x0448)
#define AFE_CONN12 (AFE_BASE + 0x044c)
#define AFE_CONN13 (AFE_BASE + 0x0450)
#define AFE_CONN14 (AFE_BASE + 0x0454)
#define AFE_CONN15 (AFE_BASE + 0x0458)
#define AFE_CONN16 (AFE_BASE + 0x045c)
#define AFE_CONN17 (AFE_BASE + 0x0460)
#define AFE_CONN18 (AFE_BASE + 0x0464)
#define AFE_CONN19 (AFE_BASE + 0x0468)
#define AFE_CONN20 (AFE_BASE + 0x046c)
#define AFE_CONN21 (AFE_BASE + 0x0470)
#define AFE_CONN22 (AFE_BASE + 0x0474)
#define AFE_CONN23 (AFE_BASE + 0x0478)
#define AFE_CONN24 (AFE_BASE + 0x047c)
#define AFE_CONN_RS (AFE_BASE + 0x0494)
#define AFE_CONN_DI (AFE_BASE + 0x0498)
#define AFE_CONN25 (AFE_BASE + 0x04b0)
#define AFE_CONN26 (AFE_BASE + 0x04b4)
#define AFE_CONN27 (AFE_BASE + 0x04b8)
#define AFE_CONN28 (AFE_BASE + 0x04bc)
#define AFE_CONN29 (AFE_BASE + 0x04c0)
#define AFE_CONN30 (AFE_BASE + 0x04c4)
#define AFE_CONN31 (AFE_BASE + 0x04c8)
#define AFE_CONN32 (AFE_BASE + 0x04cc)
#define AFE_ASRC_CON0 (AFE_BASE + 0x0500)
#define AFE_ASRC_CON1 (AFE_BASE + 0x0504)
#define AFE_ASRC_CON2 (AFE_BASE + 0x0508)
#define AFE_ASRC_CON3 (AFE_BASE + 0x050c)
#define AFE_ASRC_CON4 (AFE_BASE + 0x0510)
#define AFE_ASRC_CON5 (AFE_BASE + 0x0514)
#define AFE_ASRC_CON6 (AFE_BASE + 0x0518)
#define AFE_ASRC_CON7 (AFE_BASE + 0x051c)
#define AFE_ASRC_CON8 (AFE_BASE + 0x0520)
#define AFE_ASRC_CON9 (AFE_BASE + 0x0524)
#define AFE_ASRC_CON10 (AFE_BASE + 0x0528)
#define AFE_ASRC_CON11 (AFE_BASE + 0x052c)
#define PCM_INTF_CON1 (AFE_BASE + 0x0530)
#define PCM_INTF_CON2 (AFE_BASE + 0x0538)
#define PCM2_INTF_CON (AFE_BASE + 0x053c)
#define AFE_ASRC_CON13 (AFE_BASE + 0x0550)
#define AFE_ASRC_CON14 (AFE_BASE + 0x0554)
#define AFE_ASRC_CON15 (AFE_BASE + 0x0558)
#define AFE_ASRC_CON16 (AFE_BASE + 0x055c)
#define AFE_ASRC_CON17 (AFE_BASE + 0x0560)
#define AFE_ASRC_CON18 (AFE_BASE + 0x0564)
#define AFE_ASRC_CON19 (AFE_BASE + 0x0568)
#define AFE_ASRC_CON20 (AFE_BASE + 0x056c)
#define AFE_ASRC_CON21 (AFE_BASE + 0x0570)
#define CLK_AUDDIV_0 (AFE_BASE + 0x05a0)
#define CLK_AUDDIV_1 (AFE_BASE + 0x05a4)
#define CLK_AUDDIV_2 (AFE_BASE + 0x05a8)
#define CLK_AUDDIV_3 (AFE_BASE + 0x05ac)
#define FPGA_CFG0 (AFE_BASE + 0x05b0)
#define FPGA_CFG1 (AFE_BASE + 0x05b4)
#define FPGA_CFG2 (AFE_BASE + 0x05c0)
#define FPGA_CFG3 (AFE_BASE + 0x05c4)
#define AFE_ASRC4_CON0 (AFE_BASE + 0x06c0)
#define AFE_ASRC4_CON1 (AFE_BASE + 0x06c4)
#define AFE_ASRC4_CON2 (AFE_BASE + 0x06c8)
#define AFE_ASRC4_CON3 (AFE_BASE + 0x06cc)
#define AFE_ASRC4_CON4 (AFE_BASE + 0x06d0)
#define AFE_ASRC4_CON5 (AFE_BASE + 0x06d4)
#define AFE_ASRC4_CON6 (AFE_BASE + 0x06d8)
#define AFE_ASRC4_CON7 (AFE_BASE + 0x06dc)
#define AFE_ASRC4_CON8 (AFE_BASE + 0x06e0)
#define AFE_ASRC4_CON9 (AFE_BASE + 0x06e4)
#define AFE_ASRC4_CON10 (AFE_BASE + 0x06e8)
#define AFE_ASRC4_CON11 (AFE_BASE + 0x06ec)
#define AFE_ASRC4_CON12 (AFE_BASE + 0x06f0)
#define AFE_ASRC4_CON13 (AFE_BASE + 0x06f4)
#define AFE_ASRC4_CON14 (AFE_BASE + 0x06f8)
#define AFE_CONN_RS1 (AFE_BASE + 0x098c)
#define AFE_CONN_DI1 (AFE_BASE + 0x0990)
#define AFE_CONN_24BIT_1 (AFE_BASE + 0x0994)
#define AFE_CONN_REG (AFE_BASE + 0x0998)
#define AFE_CONNSYS_I2S_CON (AFE_BASE + 0x0c78)
#define AFE_ASRC_CONNSYS_CON0 (AFE_BASE + 0x0c80)
#define AFE_ASRC_CONNSYS_CON1 (AFE_BASE + 0x0c84)
#define AFE_ASRC_CONNSYS_CON2 (AFE_BASE + 0x0c88)
#define AFE_ASRC_CONNSYS_CON3 (AFE_BASE + 0x0c8c)
#define AFE_ASRC_CONNSYS_CON4 (AFE_BASE + 0x0c90)
#define AFE_ASRC_CONNSYS_CON5 (AFE_BASE + 0x0c94)
#define AFE_ASRC_CONNSYS_CON6 (AFE_BASE + 0x0c98)
#define AFE_ASRC_CONNSYS_CON7 (AFE_BASE + 0x0c9c)
#define AFE_ASRC_CONNSYS_CON8 (AFE_BASE + 0x0ca0)
#define AFE_ASRC_CONNSYS_CON9 (AFE_BASE + 0x0ca4)
#define AFE_ASRC_CONNSYS_CON10 (AFE_BASE + 0x0ca8)
#define AFE_ASRC_CONNSYS_CON11 (AFE_BASE + 0x0cac)
#define AFE_ASRC_CONNSYS_CON13 (AFE_BASE + 0x0cd0)
#define AFE_ASRC_CONNSYS_CON14 (AFE_BASE + 0x0cd4)
#define AFE_ASRC_CONNSYS_CON15 (AFE_BASE + 0x0cd8)
#define AFE_ASRC_CONNSYS_CON16 (AFE_BASE + 0x0cdc)
#define AFE_ASRC_CONNSYS_CON17 (AFE_BASE + 0x0ce0)
#define AFE_ASRC_CONNSYS_CON18 (AFE_BASE + 0x0ce4)
#define AFE_ASRC_CONNSYS_CON19 (AFE_BASE + 0x0ce8)
#define AFE_ASRC_CONNSYS_CON20 (AFE_BASE + 0x0cec)
#define AFE_ASRC_CONNSYS_CON21 (AFE_BASE + 0x0cf0)
#define AFE_ASRC_CONNSYS_CON23 (AFE_BASE + 0x0cf8)
#define AFE_ASRC_CONNSYS_CON24 (AFE_BASE + 0x0cfc)

#define AFE_MAXLENGTH (AFE_BASE + 0x0cfc)
#define AFE_REG_UNDEFINED (AFE_MAXLENGTH + 0x1)

/* do afe register ioremap */
int Auddrv_Reg_map(struct device *pdev);

void Afe_Set_Reg(unsigned int offset, unsigned int value, unsigned int mask);
unsigned int Afe_Get_Reg(unsigned int offset);

/* function to apmixed */
unsigned int GetApmixedCfg(unsigned int offset);
void SetApmixedCfg(unsigned int offset, unsigned int value, unsigned int mask);

/* for debug usage */
void Afe_Log_Print(void);

/* function to get pointer */
unsigned int Get_Afe_Sram_Length(void);
dma_addr_t Get_Afe_Sram_Phys_Addr(void);
dma_addr_t Get_Afe_Sram_Capture_Phys_Addr(void);
void *Get_Afe_SramBase_Pointer(void);
void *Get_Afe_SramCaptureBase_Pointer(void);

void *Get_Afe_Powertop_Pointer(void);
void *Get_AudClk_Pointer(void);
void *Get_Afe_Infra_Pointer(void);

void SetChipModemPcmConfig(int modem_index,
			   struct audio_digital_pcm p_modem_pcm_attribute);
bool SetChipModemPcmEnable(int modem_index, bool modem_pcm_on);
bool SetI2SASRCConfig(bool bIsUseASRC, unsigned int dToSampleRate);
bool SetI2SASRCEnable(bool bEnable);
bool EnableSideToneFilter(bool stf_on);
bool CleanPreDistortion(void);
bool SetDLSrc2(unsigned int SampleRate);

bool SetSampleRate(unsigned int Aud_block, unsigned int SampleRate);
bool SetChannels(unsigned int Memory_Interface, unsigned int channel);
int SetMemifMonoSel(unsigned int Memory_Interface, bool mono_use_r_ch);

bool SetMemDuplicateWrite(unsigned int InterfaceType, int dupwrite);

ssize_t AudDrv_Reg_Dump(char *buffer, int size);

#endif
