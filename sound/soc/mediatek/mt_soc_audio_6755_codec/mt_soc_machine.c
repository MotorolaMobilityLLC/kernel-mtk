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
 *   mt_soc_machine.c
 *
 * Project:
 * --------
 *   Audio soc machine driver
 *
 * Description:
 * ------------
 *   Audio machine driver
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#define CONFIG_MTK_DEEP_IDLE
#ifdef CONFIG_MTK_DEEP_IDLE
#include "mt_idle.h"
#endif

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Kernel.h"
#include "mt_soc_afe_control.h"

//#include <mach/mt_clkbuf_ctl.h>
#include <sound/mt_soc_audio.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
/*#include <linux/xlog.h>*/
/*#include <mach/irqs.h>*/
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/div64.h>
/*
#include <mach/mt_reg_base.h>
#include <linux/aee.h>
#include <mach/pmic_mt6325_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/mt_gpio.h>
#include <mach/mt_typedefs.h>*/
#include <linux/types.h>
#include <stdarg.h>
#include <linux/module.h>


#include <linux/clk.h>

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
/* #include <asm/mach-types.h> */
#include <linux/debugfs.h>
#include "mt_soc_codec_63xx.h"

#if defined(CONFIG_SND_SOC_FLORIDA)
#include <linux/pm_runtime.h>
#include <linux/mfd/arizona/registers.h>
#include "../../codecs/florida.h"
#include <linux/pm_runtime.h>
#include <mt_gpio.h>
#include <linux/input.h>
#include <linux/mfd/arizona/core.h>
#include <sound/tlv.h>
#endif
static int mt_soc_lowjitter_control = 0;
static struct dentry *mt_sco_audio_debugfs;
static struct input_dev *ez2ctrl_input_dev;
#define DEBUG_FS_NAME "mtksocaudio"
#define DEBUG_ANA_FS_NAME "mtksocanaaudio"

#if defined(CONFIG_SND_SOC_FLORIDA)
#define FLORIDA_PLAT_CLK_HZ	  26000000
#define FLORIDA_MAX_SYSCLK_1  147456000  /*max sysclk for 4K family 49152000*/
#define FLORIDA_MAX_SYSCLK_2  135475200  /*max sysclk for 11.025K family florida only 45158400s*/
#define FLORIDA_RUN_MAINMIC 1
#define KLASSEN_RUN_HEADSETMIC 2
struct florida_drvdata {
    unsigned int pll_freq;
    int florida_hp_imp_compensate;
    enum snd_soc_bias_level previous_bias_level;
    struct wake_lock wake_lock; 
	bool ear_mic;
};
static int florida_active;
static void florida_ez2ctrl_cb(void);
static int mtk_florida_late_probe(struct snd_soc_card *card);
static int florida_check_clock_conditions(struct snd_soc_card *card);
static const DECLARE_TLV_DB_SCALE(digital_vol, -6400, 50, 0);

static struct snd_soc_codec *florida;
extern void Sound_Speaker_Turnon(void);
extern void Sound_Speaker_Turnoff(void);
extern int clk_monitor(int gpio_idx, int ckmon_value, int div);
#endif

static int mtmachine_startup(struct snd_pcm_substream *substream)
{
	/* pr_debug("mtmachine_startup\n"); */
	return 0;
}

static int mtmachine_prepare(struct snd_pcm_substream *substream)
{
	/* pr_debug("mtmachine_prepare\n"); */
	return 0;
}

static struct snd_soc_ops mt_machine_audio_ops = {
	.startup = mtmachine_startup,
	.prepare = mtmachine_prepare,
};

static int mtmachine_compr_startup(struct snd_compr_stream *stream)
{
	return 0;
}

static struct snd_soc_compr_ops mt_machine_audio_compr_ops = {
	.startup = mtmachine_compr_startup,
};

static int mtmachine_startupmedia2(struct snd_pcm_substream *substream)
{
	/* pr_debug("mtmachine_startupmedia2\n"); */
	return 0;
}

static int mtmachine_preparemedia2(struct snd_pcm_substream *substream)
{
	/* pr_debug("mtmachine_preparemedia2\n"); */
	return 0;
}

static struct snd_soc_ops mtmachine_audio_ops2 = {
	.startup = mtmachine_startupmedia2,
	.prepare = mtmachine_preparemedia2,
};

static int mt_soc_audio_init(struct snd_soc_pcm_runtime *rtd)
{
	pr_debug("mt_soc_audio_init\n");
	return 0;
}

static int mt_soc_audio_init2(struct snd_soc_pcm_runtime *rtd)
{
	pr_debug("mt_soc_audio_init2\n");
	return 0;
}

static int mt_soc_ana_debug_open(struct inode *inode, struct file *file)
{
	pr_debug("mt_soc_ana_debug_open\n");
	return 0;
}

static ssize_t mt_soc_ana_debug_read(struct file *file, char __user *buf,
				     size_t count, loff_t *pos)
{
	const int size = 6020;
	/* char buffer[size]; */
	char *buffer = NULL; /* for reduce kernel stack */
	int n = 0;
	int ret = 0;

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer) {
		kfree(buffer);
		return -ENOMEM;
	}

	pr_debug("mt_soc_ana_debug_read count = %zu\n", count);
	AudDrv_Clk_On();
	/* audckbufEnable(true); */

	n += scnprintf(buffer + n, size - n, "AFE_UL_DL_CON0  = 0x%x\n",
		       Ana_Get_Reg(AFE_UL_DL_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC2_CON0_H  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SRC2_CON0_H));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC2_CON0_L  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SRC2_CON0_L));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_CON0  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SDM_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_CON1  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SDM_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC0_CON0_H  = 0x%x\n",
		       Ana_Get_Reg(AFE_UL_SRC0_CON0_H));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC0_CON0_L  = 0x%x\n",
		       Ana_Get_Reg(AFE_UL_SRC0_CON0_L));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC1_CON0_H  = 0x%x\n",
		       Ana_Get_Reg(AFE_UL_SRC1_CON0_H));
	n += scnprintf(buffer + n, size - n, "AFE_UL_SRC1_CON0_L  = 0x%x\n",
		       Ana_Get_Reg(AFE_UL_SRC1_CON0_L));
	n += scnprintf(buffer + n, size - n, "PMIC_AFE_TOP_CON0  = 0x%x\n",
		       Ana_Get_Reg(PMIC_AFE_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_AUDIO_TOP_CON0  = 0x%x\n",
		       Ana_Get_Reg(AFE_AUDIO_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "PMIC_AFE_TOP_CON0  = 0x%x\n",
		       Ana_Get_Reg(PMIC_AFE_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SRC_MON0  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SRC_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_SDM_TEST0  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_SDM_TEST0));
	n += scnprintf(buffer + n, size - n, "AFE_MON_DEBUG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_MON_DEBUG0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON0  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_CON0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON1  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_CON1));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON2  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_CON2));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON3  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_CON3));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_CON4  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_CON4));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_MON0  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_MON0));
	n += scnprintf(buffer + n, size - n, "AFUNC_AUD_MON1  = 0x%x\n",
		       Ana_Get_Reg(AFUNC_AUD_MON1));
	n += scnprintf(buffer + n, size - n, "AUDRC_TUNE_MON0  = 0x%x\n",
		       Ana_Get_Reg(AUDRC_TUNE_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_UP8X_FIFO_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_LOG_MON0  = 0x%x\n",
		       Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_UP8X_FIFO_LOG_MON1  = 0x%x\n",
		       Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_DC_COMP_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG1  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_DC_COMP_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_DL_DC_COMP_CFG2  = 0x%x\n",
		       Ana_Get_Reg(AFE_DL_DC_COMP_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_PMIC_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG1  = 0x%x\n",
		       Ana_Get_Reg(AFE_PMIC_NEWIF_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG2  = 0x%x\n",
		       Ana_Get_Reg(AFE_PMIC_NEWIF_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_PMIC_NEWIF_CFG3  = 0x%x\n",
		       Ana_Get_Reg(AFE_PMIC_NEWIF_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CFG0  = 0x%x\n",
			Ana_Get_Reg(AFE_SGEN_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CFG1  = 0x%x\n",
			Ana_Get_Reg(AFE_SGEN_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA2_UP8X_FIFO_LOG_MON0  = 0x%x\n",
			Ana_Get_Reg(AFE_ADDA2_UP8X_FIFO_LOG_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA2_UP8X_FIFO_LOG_MON1  = 0x%x\n",
			Ana_Get_Reg(AFE_ADDA2_UP8X_FIFO_LOG_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA2_PMIC_NEWIF_CFG0  = 0x%x\n",
			Ana_Get_Reg(AFE_ADDA2_PMIC_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA2_PMIC_NEWIF_CFG1  = 0x%x\n",
			Ana_Get_Reg(AFE_ADDA2_PMIC_NEWIF_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA2_PMIC_NEWIF_CFG2  = 0x%x\n",
			Ana_Get_Reg(AFE_ADDA2_PMIC_NEWIF_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_TOP  = 0x%x\n", Ana_Get_Reg(AFE_VOW_TOP));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG2  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG3  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG4  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG4));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_CFG5  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG5));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON0  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON1  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON2  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON3  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON3));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON4  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON4));
	n += scnprintf(buffer + n, size - n, "AFE_VOW_MON5  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON5));

	n += scnprintf(buffer + n, size - n, "AFE_DCCLK_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_DCCLK_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_DCCLK_CFG1  = 0x%x\n",
		       Ana_Get_Reg(AFE_DCCLK_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_HPANC_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_HPANC_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_NCP_CFG0  = 0x%x\n",
		       Ana_Get_Reg(AFE_NCP_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_NCP_CFG1  = 0x%x\n",
		       Ana_Get_Reg(AFE_NCP_CFG1));

	n += scnprintf(buffer + n, size - n, "TOP_CON  = 0x%x\n", Ana_Get_Reg(TOP_CON));
	n += scnprintf(buffer + n, size - n, "TOP_STATUS  = 0x%x\n", Ana_Get_Reg(TOP_STATUS));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON1  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON2  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON2));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON3  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON3));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON4  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON4));
	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON5  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON5));
	n += scnprintf(buffer + n, size - n, "TOP_CKSEL_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKSEL_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKSEL_CON1  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKSEL_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_CKSEL_CON2  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKSEL_CON2));
	n += scnprintf(buffer + n, size - n, "TOP_CKSEL_CON3  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKSEL_CON3));
	n += scnprintf(buffer + n, size - n, "TOP_CKDIVSEL_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKDIVSEL_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKDIVSEL_CON1  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKDIVSEL_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_CKHWEN_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKHWEN_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKHWEN_CON1  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKHWEN_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_CKHWEN_CON2  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKHWEN_CON2));
	n += scnprintf(buffer + n, size - n, "TOP_CKTST_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKTST_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_CKTST_CON1  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKTST_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_CKTST_CON2  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKTST_CON2));
	n += scnprintf(buffer + n, size - n, "TOP_CLKSQ  = 0x%x\n", Ana_Get_Reg(TOP_CLKSQ));
	n += scnprintf(buffer + n, size - n, "TOP_CLKSQ_RTC  = 0x%x\n",
			Ana_Get_Reg(TOP_CLKSQ_RTC));
	n += scnprintf(buffer + n, size - n, "TOP_CLK_TRIM  = 0x%x\n",
			Ana_Get_Reg(TOP_CLK_TRIM));
	n += scnprintf(buffer + n, size - n, "TOP_RST_CON0  = 0x%x\n",
			Ana_Get_Reg(TOP_RST_CON0));
	n += scnprintf(buffer + n, size - n, "TOP_RST_CON1  = 0x%x\n",
			Ana_Get_Reg(TOP_RST_CON1));
	n += scnprintf(buffer + n, size - n, "TOP_RST_CON2  = 0x%x\n",
			Ana_Get_Reg(TOP_RST_CON2));
	n += scnprintf(buffer + n, size - n, "TOP_RST_MISC  = 0x%x\n",
			Ana_Get_Reg(TOP_RST_MISC));
	n += scnprintf(buffer + n, size - n, "TOP_RST_STATUS  = 0x%x\n",
			Ana_Get_Reg(TOP_RST_STATUS));
	n += scnprintf(buffer + n, size - n, "TEST_CON0  = 0x%x\n", Ana_Get_Reg(TEST_CON0));
	n += scnprintf(buffer + n, size - n, "TEST_OUT  = 0x%x\n", Ana_Get_Reg(TEST_OUT));
	n += scnprintf(buffer + n, size - n, "AFE_MON_DEBUG0= 0x%x\n",
			Ana_Get_Reg(AFE_MON_DEBUG0));
	n += scnprintf(buffer + n, size - n, "ZCD_CON0  = 0x%x\n", Ana_Get_Reg(ZCD_CON0));
	n += scnprintf(buffer + n, size - n, "ZCD_CON1  = 0x%x\n", Ana_Get_Reg(ZCD_CON1));
	n += scnprintf(buffer + n, size - n, "ZCD_CON2  = 0x%x\n", Ana_Get_Reg(ZCD_CON2));
	n += scnprintf(buffer + n, size - n, "ZCD_CON3  = 0x%x\n", Ana_Get_Reg(ZCD_CON3));
	n += scnprintf(buffer + n, size - n, "ZCD_CON4  = 0x%x\n", Ana_Get_Reg(ZCD_CON4));
	n += scnprintf(buffer + n, size - n, "ZCD_CON5  = 0x%x\n", Ana_Get_Reg(ZCD_CON5));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON0  = 0x%x\n",
			Ana_Get_Reg(LDO_VA18_CON0));
	n += scnprintf(buffer + n, size - n, "LDO_VA18_CON1  = 0x%x\n",
			Ana_Get_Reg(LDO_VA18_CON1));
	n += scnprintf(buffer + n, size - n, "LDO_VUSB33_CON0  = 0x%x\n",
		       Ana_Get_Reg(LDO_VUSB33_CON0));
	n += scnprintf(buffer + n, size - n, "LDO_VUSB33_CON1  = 0x%x\n",
		       Ana_Get_Reg(LDO_VUSB33_CON1));

#if 0
	n += scnprintf(buffer + n, size - n, "SPK_CON0  = 0x%x\n", Ana_Get_Reg(SPK_CON0));
	n += scnprintf(buffer + n, size - n, "SPK_CON1  = 0x%x\n", Ana_Get_Reg(SPK_CON1));
	n += scnprintf(buffer + n, size - n, "SPK_CON2  = 0x%x\n", Ana_Get_Reg(SPK_CON2));
	n += scnprintf(buffer + n, size - n, "SPK_CON3  = 0x%x\n", Ana_Get_Reg(SPK_CON3));
	n += scnprintf(buffer + n, size - n, "SPK_CON4  = 0x%x\n", Ana_Get_Reg(SPK_CON4));
	n += scnprintf(buffer + n, size - n, "SPK_CON5  = 0x%x\n", Ana_Get_Reg(SPK_CON5));
	n += scnprintf(buffer + n, size - n, "SPK_CON6  = 0x%x\n", Ana_Get_Reg(SPK_CON6));
	n += scnprintf(buffer + n, size - n, "SPK_CON7  = 0x%x\n", Ana_Get_Reg(SPK_CON7));
	n += scnprintf(buffer + n, size - n, "SPK_CON8  = 0x%x\n", Ana_Get_Reg(SPK_CON8));
	n += scnprintf(buffer + n, size - n, "SPK_CON9  = 0x%x\n", Ana_Get_Reg(SPK_CON9));
	n += scnprintf(buffer + n, size - n, "SPK_CON10  = 0x%x\n", Ana_Get_Reg(SPK_CON10));
	n += scnprintf(buffer + n, size - n, "SPK_CON11  = 0x%x\n", Ana_Get_Reg(SPK_CON11));
	n += scnprintf(buffer + n, size - n, "SPK_CON12  = 0x%x\n", Ana_Get_Reg(SPK_CON12));
	n += scnprintf(buffer + n, size - n, "SPK_CON13  = 0x%x\n", Ana_Get_Reg(SPK_CON13));
	n += scnprintf(buffer + n, size - n, "SPK_CON14  = 0x%x\n", Ana_Get_Reg(SPK_CON14));
	n += scnprintf(buffer + n, size - n, "SPK_CON15  = 0x%x\n", Ana_Get_Reg(SPK_CON15));
	n += scnprintf(buffer + n, size - n, "SPK_CON16  = 0x%x\n", Ana_Get_Reg(SPK_CON16));
	n += scnprintf(buffer + n, size - n, "SPK_ANA_CON0  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON0));
	n += scnprintf(buffer + n, size - n, "SPK_ANA_CON1  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON1));
	n += scnprintf(buffer + n, size - n, "SPK_ANA_CON3  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON3));
#endif
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON0  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON0));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON1  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON1));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON2  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON2));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON3  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON3));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON4  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON4));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON5  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON5));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON6  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON6));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON7  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON7));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON8  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON8));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON9  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON9));
	n += scnprintf(buffer + n, size - n, "AUDDEC_ANA_CON10  = 0x%x\n",
		       Ana_Get_Reg(AUDDEC_ANA_CON10));

	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON0  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON0));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON1  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON1));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON2  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON2));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON3  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON3));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON4  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON4));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON5  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON5));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON6  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON6));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON7  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON7));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON8  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON8));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON9  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON9));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON10  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON10));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON11  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON11));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON12  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON12));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON13  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON13));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON14  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON14));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON15  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON15));
	n += scnprintf(buffer + n, size - n, "AUDENC_ANA_CON16  = 0x%x\n",
		       Ana_Get_Reg(AUDENC_ANA_CON16));

	n += scnprintf(buffer + n, size - n, "AUDNCP_CLKDIV_CON0  = 0x%x\n",
		       Ana_Get_Reg(AUDNCP_CLKDIV_CON0));
	n += scnprintf(buffer + n, size - n, "AUDNCP_CLKDIV_CON1  = 0x%x\n",
		       Ana_Get_Reg(AUDNCP_CLKDIV_CON1));
	n += scnprintf(buffer + n, size - n, "AUDNCP_CLKDIV_CON2  = 0x%x\n",
		       Ana_Get_Reg(AUDNCP_CLKDIV_CON2));
	n += scnprintf(buffer + n, size - n, "AUDNCP_CLKDIV_CON3  = 0x%x\n",
		       Ana_Get_Reg(AUDNCP_CLKDIV_CON3));
	n += scnprintf(buffer + n, size - n, "AUDNCP_CLKDIV_CON4  = 0x%x\n",
		       Ana_Get_Reg(AUDNCP_CLKDIV_CON4));
	n += scnprintf(buffer + n, size - n, "DCXO_CW00  = 0x%x\n",
		       Ana_Get_Reg(DCXO_CW00));
	n += scnprintf(buffer + n, size - n, "DCXO_CW01  = 0x%x\n",
		       Ana_Get_Reg(DCXO_CW01));
	n += scnprintf(buffer + n, size - n, "DCXO_CW02  = 0x%x\n",
		       Ana_Get_Reg(DCXO_CW02));
	n += scnprintf(buffer + n, size - n, "DCXO_CW03  = 0x%x\n",
		       Ana_Get_Reg(DCXO_CW03));

	n += scnprintf(buffer + n, size - n, "TOP_CKPDN_CON0  = 0x%x\n",
		       Ana_Get_Reg(TOP_CKPDN_CON0));
	n += scnprintf(buffer + n, size - n, "DRV_CON2  = 0x%x\n",
		       Ana_Get_Reg(DRV_CON2));
	n += scnprintf(buffer + n, size - n, "GPIO_MODE3  = 0x%x\n", Ana_Get_Reg(GPIO_MODE3));
	pr_debug("mt_soc_ana_debug_read len = %d\n", n);

	/* audckbufEnable(false); */
	AudDrv_Clk_Off();

	ret = simple_read_from_buffer(buf, count, pos, buffer, n);
	kfree(buffer);
	return ret;

}


static int mt_soc_debug_open(struct inode *inode, struct file *file)
{
	pr_debug("mt_soc_debug_open\n");
	return 0;
}

static ssize_t mt_soc_debug_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	const int size = 6144;
	/* char buffer[size]; */
	char *buffer = NULL; /* for reduce kernel stack */
	int n = 0;
	int ret = 0;

	buffer = kmalloc(size, GFP_KERNEL);
	if (!buffer) {
		kfree(buffer);
		return -ENOMEM;
	}

	AudDrv_Clk_On();

	pr_debug("mt_soc_debug_read\n");
	n = scnprintf(buffer + n, size - n, "AUDIO_TOP_CON0	   = 0x%x\n",
		      Afe_Get_Reg(AUDIO_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AUDIO_TOP_CON1));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AUDIO_TOP_CON2));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AUDIO_TOP_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON	   = 0x%x\n",
		       Afe_Get_Reg(AFE_I2S_CON));
	n += scnprintf(buffer + n, size - n, "AFE_DAIBT_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAIBT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_CONN0		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN0));
	n += scnprintf(buffer + n, size - n, "AFE_CONN1		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN1));
	n += scnprintf(buffer + n, size - n, "AFE_CONN2		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN2));
	n += scnprintf(buffer + n, size - n, "AFE_CONN3		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN3));
	n += scnprintf(buffer + n, size - n, "AFE_CONN4		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN4));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_I2S_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_I2S_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_CON	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MRGIF_CON));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_END));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_D2_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_D2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_D2_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_D2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_D2_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL1_D2_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_D2_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_D2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_D2_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_D2_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_D2_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_D2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_I2S_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DL2_END));
	n += scnprintf(buffer + n, size - n, "AFE_CONN5		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN5));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_24BIT	   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN_24BIT));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_AWB_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_AWB_END));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_AWB_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_VUL_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAI_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAI_END));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_DAI_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_CONN6		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN6));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MSB	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MSB));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON4	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MON4));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_DL_SRC2_CON0  = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_DL_SRC2_CON1  = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_UL_SRC_CON0   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_UL_SRC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_UL_SRC_CON1   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_UL_SRC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_TOP_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_UL_DL_CON0    = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_UL_DL_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_SRC_DEBUG));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG_MON0= 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG_MON1= 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_NEWIF_CFG0    = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_NEWIF_CFG1    = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_NEWIF_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_DEBUG   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_DEBUG));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_MON	   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_MON));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_COEFF   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_COEFF));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_GAIN	   = 0x%x\n",
		       Afe_Get_Reg(AFE_SIDETONE_GAIN));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_SGEN_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_TOP_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_PREDIS_CON0   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_PREDIS_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_PREDIS_CON1   = 0x%x\n",
		       Afe_Get_Reg(AFE_ADDA_PREDIS_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MRGIF_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MRGIF_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MRGIF_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_BASE	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MOD_DAI_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_END	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MOD_DAI_END));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MOD_DAI_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CON	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_CON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_STATUS   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_STATUS));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CLR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_CLR));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_CNT1));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_CNT2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_EN	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_EN));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_MON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_CNT5	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_CNT5));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_MCU_CNT_MON   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ1_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ2_MCU_CNT_MON   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ2_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_MCU_EN_CNT_MON= 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ1_MCU_EN_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_DEBUG	   = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_DEBUG));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MAXLEN	   = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_MAXLEN));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_PBUF_SIZE    = 0x%x\n",
		       Afe_Get_Reg(AFE_MEMIF_PBUF_SIZE));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT7    = 0x%x\n",
		       Afe_Get_Reg(AFE_IRQ_MCU_CNT7));
	n += scnprintf(buffer + n, size - n, "AFE_APLL1_TUNER_CFG    = 0x%x\n",
		       Afe_Get_Reg(AFE_APLL1_TUNER_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_APLL2_TUNER_CFG    = 0x%x\n",
		       Afe_Get_Reg(AFE_APLL2_TUNER_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CONN	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CONN));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CONN	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CONN));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CUR	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CONN2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CONN2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CONN3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN2_CONN3));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CONN2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CONN2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CONN3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_GAIN1_CONN3));
	n += scnprintf(buffer + n, size - n, "AFE_CONN7		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN7));
	n += scnprintf(buffer + n, size - n, "AFE_CONN8		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN8));
	n += scnprintf(buffer + n, size - n, "AFE_CONN9		   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN9));
	n += scnprintf(buffer + n, size - n, "AFE_CONN10	   = 0x%x\n",
		       Afe_Get_Reg(AFE_CONN10));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG0		   = 0x%x\n",
		       Afe_Get_Reg(FPGA_CFG0));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG1		   = 0x%x\n",
		       Afe_Get_Reg(FPGA_CFG1));
	n += scnprintf(buffer + n, size - n, "FPGA_VER		   = 0x%x\n",
		       Afe_Get_Reg(FPGA_VER));
	n += scnprintf(buffer + n, size - n, "FPGA_STC		   = 0x%x\n",
		       Afe_Get_Reg(FPGA_STC));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON4	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON5	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON5));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON6	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON6));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON7	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON7));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON8	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON8));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON9	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON9));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON10	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON10));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON11	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON11));
	n += scnprintf(buffer + n, size - n, "PCM_INTF_CON	   = 0x%x\n",
		       Afe_Get_Reg(PCM_INTF_CON));
	n += scnprintf(buffer + n, size - n, "PCM_INTF_CON2	   = 0x%x\n",
		       Afe_Get_Reg(PCM_INTF_CON2));
	n += scnprintf(buffer + n, size - n, "PCM2_INTF_CON	   = 0x%x\n",
		       Afe_Get_Reg(PCM2_INTF_CON));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON13	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON13));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON14	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON14));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON15	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON15));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON16	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON16));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON17	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON17));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON18	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON18));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON19	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON19));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON20	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON20));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON21	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC_CON21));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON0	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON1	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON2	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON3	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON4	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON5	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON5));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON6	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON6));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON7	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON7));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON8	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON8));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON9	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON9));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON10	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON10));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON11	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON11));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON12	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON12));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON13	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON13));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON14	   = 0x%x\n",
		       Afe_Get_Reg(AFE_ASRC4_CON14));

	n += scnprintf(buffer + n, size - n, "AUDIO_CLK_AUDDIV_0  = 0x%x\n",
		       Afe_Get_Reg(AUDIO_CLK_AUDDIV_0));
	n += scnprintf(buffer + n, size - n, "AUDIO_CLK_AUDDIV_1  = 0x%x\n",
		       Afe_Get_Reg(AUDIO_CLK_AUDDIV_1));
	pr_debug("mt_soc_debug_read len = %d\n", n);
	AudDrv_Clk_Off();

	ret = simple_read_from_buffer(buf, count, pos, buffer, n);
	kfree(buffer);
	return ret;
}

static char const ParSetkeyAfe[] = "Setafereg";
static char const ParSetkeyAna[] = "Setanareg";
static char const ParSetkeyCfg[] = "Setcfgreg";
static char const PareGetkeyAfe[] = "Getafereg";
static char const PareGetkeyAna[] = "Getanareg";
/* static char ParGetkeyCfg[] = "Getcfgreg"; */
/* static char ParSetAddr[] = "regaddr"; */
/* static char ParSetValue[] = "regvalue"; */

static ssize_t mt_soc_debug_write(struct file *f, const char __user *buf,
				  size_t count, loff_t *offset)
{
	int ret = 0;
	char InputString[256];
	char *token1 = NULL;
	char *token2 = NULL;
	char *token3 = NULL;
	char *token4 = NULL;
	char *token5 = NULL;
	char *temp = NULL;

	unsigned int long regaddr = 0;
	unsigned int long regvalue = 0;
	char delim[] = " ,";

	memset((void *)InputString, 0, 256);
	if (copy_from_user((InputString), buf, count))
		pr_debug("copy_from_user mt_soc_debug_write count = %zu temp = %s\n", count,
			 InputString);

	temp = kstrdup(InputString, GFP_KERNEL);
	pr_debug("copy_from_user mt_soc_debug_write count = %zu temp = %s pointer = %p\n",
		 count, InputString, InputString);
	token1 = strsep(&temp, delim);
	pr_debug("token1\n");
	pr_debug("token1 = %s\n", token1);
	token2 = strsep(&temp, delim);
	pr_debug("token2 = %s\n", token2);
	token3 = strsep(&temp, delim);
	pr_debug("token3 = %s\n", token3);
	token4 = strsep(&temp, delim);
	pr_debug("token4 = %s\n", token4);
	token5 = strsep(&temp, delim);
	pr_debug("token5 = %s\n", token5);

	AudDrv_Clk_On();
	if (strcmp(token1, ParSetkeyAfe) == 0) {
		pr_debug("strcmp (token1,ParSetkeyAfe)\n");
		ret = kstrtoul(token3, 16, &regaddr);
		ret = kstrtoul(token5, 16, &regvalue);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyAfe, regaddr, regvalue);
		Afe_Set_Reg(regaddr, regvalue, 0xffffffff);
		regvalue = Afe_Get_Reg(regaddr);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyAfe, regaddr, regvalue);
	}
	if (strcmp(token1, ParSetkeyAna) == 0) {
		pr_debug("strcmp (token1,ParSetkeyAna)\n");
		ret = kstrtoul(token3, 16, &regaddr);
		ret = kstrtoul(token5, 16, &regvalue);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyAna, regaddr, regvalue);
		/* audckbufEnable(true); */
		Ana_Set_Reg(regaddr, regvalue, 0xffffffff);
		regvalue = Ana_Get_Reg(regaddr);
		/* audckbufEnable(false); */
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyAna, regaddr, regvalue);
	}
	if (strcmp(token1, ParSetkeyCfg) == 0) {
		pr_debug("strcmp (token1,ParSetkeyCfg)\n");
		ret = kstrtoul(token3, 16, &regaddr);
		ret = kstrtoul(token5, 16, &regvalue);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyCfg, regaddr, regvalue);
		SetClkCfg(regaddr, regvalue, 0xffffffff);
		regvalue = GetClkCfg(regaddr);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", ParSetkeyCfg, regaddr, regvalue);
	}
	if (strcmp(token1, PareGetkeyAfe) == 0) {
		pr_debug("strcmp (token1,PareGetkeyAfe)\n");
		ret = kstrtoul(token3, 16, &regaddr);
		regvalue = Afe_Get_Reg(regaddr);
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", PareGetkeyAfe, regaddr, regvalue);
	}
	if (strcmp(token1, PareGetkeyAna) == 0) {
		pr_debug("strcmp (token1,PareGetkeyAna)\n");
		/* audckbufEnable(true); */
		ret = kstrtoul(token3, 16, &regaddr);
		regvalue = Ana_Get_Reg(regaddr);
		/* audckbufEnable(false); */
		pr_debug("%s regaddr = 0x%lu regvalue = 0x%lu\n", PareGetkeyAna, regaddr, regvalue);
	}

	/* AudDrv_Clk_Off(); */
	return count;
}

static const struct file_operations mtaudio_debug_ops = {
	.open = mt_soc_debug_open,
	.read = mt_soc_debug_read,
	.write = mt_soc_debug_write,
};


static const struct file_operations mtaudio_ana_debug_ops = {
	.open = mt_soc_ana_debug_open,
	.read = mt_soc_ana_debug_read,
};

#if defined(CONFIG_SND_SOC_FLORIDA)
#define GPIO_ARIZONA_EXT_SPKEN_PIN 99

static const char *const spk_function[] = {"Off", "On","Voice-On"};
static const struct soc_enum k5_snd_enum[] = {
	SOC_ENUM_SINGLE_EXT(3, spk_function),
};
static int florida_set_spk(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
  //  if (gpio_get_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN) == ucontrol->value.integer.value[0])
    {
        printk("%s spk vdd already %s \n",__func__,ucontrol->value.integer.value[0] ? "On":"Off");
        return 0;
    }
    printk("%s: set spk vdd %s\n", __func__,ucontrol->value.integer.value[0] ? "On":"Off");
//	gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, ucontrol->value.integer.value[0]);
	return 1;
}

static int florida_get_spk(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
        //ucontrol->value.integer.value[0] = gpio_get_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN);
	    printk("%s spk vdd state %s \n",__func__,ucontrol->value.integer.value[0] ? "On":"Off");
	    return 0;
}
struct hp_imp_gain_tab{
	int min;           /* Minimum impedance */
	int max;           /* Maximum impedance */
	int gain; /* Register value to set for this measurement */
};
static struct hp_imp_gain_tab florida_hp_gain_table[] = {
	{   0,      10, -12},
	{   11,     19, -8 },
	{   20,     36, -4 },
	{  37,     70,   0 },
	{  71,    250,   0 },
	{ 251, INT_MAX,  0 },
};
void florida_set_hp_imp_compensate(int gain)
{
       struct florida_drvdata *pdata = NULL;
	   WARN_ON(!florida);
		if (!florida)
		return;
        printk("%s florida_hp_imp_compensate %d\n", __func__,gain);
        pdata = snd_soc_card_get_drvdata(florida->component.card);    
        pdata->florida_hp_imp_compensate = gain;
}
void k5_arizona_hpdet_cb(unsigned int meas)
{
	int i;  
    printk("arizona %s enter meas = %d\n",__func__, meas);   
	for (i = 0; i < ARRAY_SIZE(florida_hp_gain_table); i++) {
		 if (meas < florida_hp_gain_table[i].min || meas > florida_hp_gain_table[i].max)
			continue;
		    printk("set florida=%d for %d ohms\n",florida_hp_gain_table[i].gain, meas);
            florida_set_hp_imp_compensate(florida_hp_gain_table[i].gain);
            break;
	}
	if(meas == 0)
	{
		florida_set_hp_imp_compensate(0);
	}
}
int florida_hp1_volume_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
    WARN_ON(!florida);
	if (!florida)
		return -1;          
	ucontrol->value.integer.value[0] =
		(snd_soc_read(florida, reg) >> shift) & mask;
	if (invert)
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
	if (snd_soc_volsw_is_stereo(mc)) {
		if (reg == reg2)
			ucontrol->value.integer.value[1] =
				(snd_soc_read(florida, reg) >> rshift) & mask;
		else
			ucontrol->value.integer.value[1] =
				(snd_soc_read(florida, reg2) >> shift) & mask;
		if (invert)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
	}
	return 0;
}
static int florida_hp1_volume_put(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
	struct florida_drvdata *pdata = NULL;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
        unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	int err;
	unsigned int val, val_mask;
    WARN_ON(!florida);
	if (!florida)
		return -1;    
        pdata = snd_soc_card_get_drvdata(florida->component.card);
	val = (ucontrol->value.integer.value[0] & mask);
	val += pdata->florida_hp_imp_compensate;
	printk("florida->dev, SET GAIN %d according to impedance, moved %d step\n",
			 val, pdata->florida_hp_imp_compensate);
	if (invert)
		val = max - val;
	val_mask = mask << shift;
	val = val << shift;
	err = snd_soc_update_bits(florida, reg, val_mask, val);
    err = snd_soc_update_bits(florida, reg2, val_mask, val);
	return err;
}

static int Ext_AW_set_spk(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
        if ((gpio_get_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN) == ucontrol->value.integer.value[0])
	||( (gpio_get_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN)  > 0) && (ucontrol->value.integer.value[0] > 0)))		
        {
            printk("%s AE PA already %s \n",__func__,ucontrol->value.integer.value[0] ? "On":"Off");
            return 0;
        }
        /* lenovo-sw zhangrc2 media use mode1 voice usemode7 as kp demand begin */
	    printk("%s: AW PA vdd %ld\n", __func__,ucontrol->value.integer.value[0] );
		if(ucontrol->value.integer.value[0] == 1) {			
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);			
		}else if (ucontrol->value.integer.value[0] == 2)  {
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);		
            #if  0 
          		  gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
			udelay(3);         
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);      
          		  gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
			udelay(3);
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);        
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
			udelay(3);
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);     
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0); 
			udelay(3);
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
			udelay(3);
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
			udelay(3);
          	gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
            udelay(3);
            gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 1);
           	udelay(3); 
            #endif 	  
	  	}
		else  {		
			gpio_set_value_cansleep(GPIO_ARIZONA_EXT_SPKEN_PIN, 0);
          	         udelay(3);
		}
		  /* lenovo-sw zhangrc2 media use mode2 voice usemode7 as kp demand end */
	    return 1;
}

static int Ext_AW_get_spk(struct snd_kcontrol *kcontrol,
		       struct snd_ctl_elem_value *ucontrol)
{
             ucontrol->value.integer.value[0] = mt_get_gpio_in(GPIO_ARIZONA_EXT_SPKEN_PIN | 0x80000000);
	    printk("%s AD PA state %s\n",__func__,ucontrol->value.integer.value[0] ? "On":"Off");
	    return 0;
}


static const struct snd_soc_dapm_widget florida_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Int Mic", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
    SND_SOC_DAPM_SPK("Receive", NULL),	
	SND_SOC_DAPM_MIC("Sec Mic", NULL),
	SND_SOC_DAPM_MIC("ANC Mic", NULL),
	SND_SOC_DAPM_SPK("DSP Virtual Out Pin",NULL),
	SND_SOC_DAPM_SPK("DRC2 Signal Activity Pin",NULL),
};
static const struct snd_soc_dapm_route florida_audio_map[] = {
	{"Headphone", NULL, "HPOUT1L"},
	{"Headphone", NULL, "HPOUT1R"},
	{"Ext Spk", NULL, "SPKOUTLP"},
	{"Ext Spk", NULL, "SPKOUTLN"},
	{"Ext Spk", NULL, "SPKOUTRP"},
	{"Ext Spk", NULL, "SPKOUTRN"},
	/*Add for Speaker output*/
	{"Ext Spk", NULL, "HPOUT3L"},
	{"Ext Spk", NULL, "HPOUT3R"},
	/*Add for HPOUT2 receive*/
	{"Receive", NULL, "HPOUT2L"},
	{"Receive", NULL, "HPOUT2R"},
	//TODO: Check this mapping
	{"Headset Mic", NULL, "MICBIAS1"},
	{"Headset Mic", NULL, "MICBIAS3"},
	{"IN1R", NULL, "Headset Mic"},
	{"Int Mic", NULL, "MICBIAS2"},
	{"IN2L", NULL, "Int Mic"},
	{"Sec Mic", NULL, "MICBIAS2"},
	{"IN1L", NULL, "Sec Mic"},
	{"ANC Mic", NULL, "MICBIAS2"},
	{"IN2R", NULL, "ANC Mic"},
	{"DSP Virtual Out Pin", NULL, "DSP Virtual Output"},
	{"DRC2 Signal Activity Pin", NULL, "DRC2 Signal Activity"},
};
static const struct snd_kcontrol_new florida_mc_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Headset Mic"),
	SOC_DAPM_PIN_SWITCH("Int Mic"),
	SOC_DAPM_PIN_SWITCH("Ext Spk"),
	SOC_DAPM_PIN_SWITCH("Sec Mic"),
	SOC_DAPM_PIN_SWITCH("ANC Mic"),
	SOC_DAPM_PIN_SWITCH("Receive"),
	SOC_DAPM_PIN_SWITCH("DSP Virtual Out Pin"),
	SOC_DAPM_PIN_SWITCH("DRC2 Signal Activity Pin"),
	SOC_ENUM_EXT("Speaker VDD", k5_snd_enum[0], florida_get_spk,
			florida_set_spk),
	SOC_DOUBLE_R_EXT_TLV("Auto Gain HP Volume", ARIZONA_DAC_DIGITAL_VOLUME_1L,
		 ARIZONA_DAC_DIGITAL_VOLUME_1R, ARIZONA_OUT1L_VOL_SHIFT,
		 0xbf, 0, florida_hp1_volume_get, florida_hp1_volume_put, digital_vol),
	SOC_ENUM_EXT("EXT_AW PA", k5_snd_enum[0], Ext_AW_get_spk,
         		Ext_AW_set_spk),
};
static int florida_set_bias_level(struct snd_soc_card *card,
				struct snd_soc_dapm_context *dapm,
				enum snd_soc_bias_level level)
{
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct florida_drvdata *data = card->drvdata;
	int ret;
	if (!florida)
		return 0;
	if (dapm->dev != florida->dev)
		return 0;
    printk("%s: level=%d, pll=%d data->previous_bias_level=%d\n",__func__,level,data->pll_freq,data->previous_bias_level);
	switch (level) {
		case SND_SOC_BIAS_PREPARE:
        	if (data->previous_bias_level != SND_SOC_BIAS_STANDBY)
				break;
			/*enable 26Mhz clock */
			clk_monitor(1,1,0);
			mdelay(5);
			ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1_REFCLK,
					ARIZONA_FLL_SRC_NONE, 0, 0);
		if (ret < 0) {
			pr_err("Failed to stop FLL: %d\n", ret);
			return ret;
		}
		ret =snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
					ARIZONA_FLL_SRC_NONE, 0, 0);
		ret = snd_soc_codec_set_sysclk(florida, ARIZONA_CLK_SYSCLK, ARIZONA_CLK_SRC_FLL1,
            data->pll_freq, SND_SOC_CLOCK_IN);
         if(ret < 0) {
            dev_err(codec->dev, "Failed to set sysclk: %d, %d\n", data->pll_freq, ret);
            return ret;
         }
		 ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
								ARIZONA_CLK_SRC_MCLK1, //TODO Check if clock from AP is connected to MCLK1
								FLORIDA_PLAT_CLK_HZ,
									(data->pll_freq));
			if (ret != 0) {
				printk("%s Failed to enable FLL1 with Ref Clock Loop: %d\n",__func__,ret); 	 
				return ret;
			}
		florida_active=1;
			break;
	default:
		break;
	}
	return 0;
}
static int florida_set_bias_level_post(struct snd_soc_card *card,
				     struct snd_soc_dapm_context *dapm,
				     enum snd_soc_bias_level level)
{
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct florida_drvdata *data = card->drvdata;
	int ret;
	if (!florida)
		return 0;
	if (dapm->dev != florida->dev)
		return 0;
    printk("%s: level is %d data->previous_bias_level is %d, set fll to %d\n",__func__,level,data->previous_bias_level,data->pll_freq);
	switch (level) {
		case SND_SOC_BIAS_STANDBY:
			if(data->previous_bias_level < SND_SOC_BIAS_PREPARE)
				break;
			ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1_REFCLK,
									ARIZONA_FLL_SRC_NONE, 0, 0);
			if (ret < 0) {
				pr_err("Failed to stop FLL: %d\n", ret);
				return ret;
			}
		ret =snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
					ARIZONA_FLL_SRC_NONE, 0, 0);
			if(ret < 0) {
				dev_err(codec->dev, "Failed to set sysclk: %d, %d\n", data->pll_freq, ret);
				return ret;
			}
			florida_active=0;
			/*disable 26Mhz clock */
			clk_monitor(1,0,0);
			mdelay(1);
			break;
	default:
		break;
	}
	data->previous_bias_level = level;
	return 0;
}
static const struct snd_soc_pcm_runtime *k5_find_dai_link(
					const struct snd_soc_card *card,
					const char *dai_link_name)
{
	int i;
	for (i = 0; i < card->num_rtd; ++i)
		if (strcmp(card->rtd[i].dai_link->name, dai_link_name) == 0)
			return &card->rtd[i];
	return NULL;
}
struct florida_drvdata drvdata =
{
    .pll_freq = FLORIDA_MAX_SYSCLK_1,
};

static int florida_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret;
	struct snd_soc_codec *codec = runtime->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = runtime->card;
	printk("Enter:%s\n", __func__);

	if (GPIO_ARIZONA_EXT_SPKEN_PIN >= 0) {
		ret = gpio_request_one(GPIO_ARIZONA_EXT_SPKEN_PIN, GPIOF_DIR_OUT | GPIOF_INIT_LOW,
				"ext-spk-enable");
		if (ret != 0) {
			pr_err("%s: failed to request ext spkena gpio %d\n", __func__, ret);
			return ret;
		}
	}

	florida = codec;
	florida_set_bias_level(card, dapm, SND_SOC_BIAS_OFF);
	card->dapm.idle_bias_off = true;
	ret = snd_soc_add_card_controls(card, florida_mc_controls,
					ARRAY_SIZE(florida_mc_controls));
	if (ret) {
		printk("unable to add card controls\n");
		return ret;
	}
	florida_active=0;
	snd_soc_dapm_sync(dapm);
	return ret;
}

bool florida_power_status = false;
int florida_powerdown_prepare(struct device *dev)
{
    struct snd_soc_card *card =dev_get_drvdata(dev);
	struct florida_drvdata *data = card->drvdata;
    int ret;
	printk("Enter:%s pll_out=%d\n", __func__,data->pll_freq );
	ret = florida_check_clock_conditions(card); 
	if (ret == KLASSEN_RUN_HEADSETMIC) {
            //because clk will not closed by bias_level when headset in, so close here when suspend
             printk("%s disable clk",__func__);
	   florida_power_status = true;		
            ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1_REFCLK,
                ARIZONA_FLL_SRC_NONE, 0, 0);
            if (ret < 0) {
                pr_err("Failed to stop FLL: %d\n", ret);
                return ret;
            }
            ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
                ARIZONA_FLL_SRC_NONE, 0, 0);
            if(ret < 0) {
                pr_err("%s arizona Failed to set sysclk ret = %d\n",__func__, ret);
                return ret;
            }
		  
           /*disable 26Mhz clock */
			clk_monitor(1,0,0);
      }else if (ret == FLORIDA_RUN_MAINMIC) {
        	/* if CODEC is running then switch to the 32k clock */
                printk("%s set mclk2\n",__func__);
		           ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
			            ARIZONA_CLK_SRC_MCLK2,
			            32000, data->pll_freq);
		  		if (ret != 0) {
		   			 printk("Failed to switch to 32K clock: %d\n", ret);
		    		 return ret;
	             }
			/*disable 26Mhz clock */
			clk_monitor(1,0,0);
	  }
       return 0;
}
void florida_powerdown_complete(struct device *dev)
{
	struct snd_soc_card *card =dev_get_drvdata(dev);
	struct florida_drvdata *data = card->drvdata;
    int ret;
     ret = florida_check_clock_conditions(card);  
	 printk("%s enter ret %d \n",__func__,ret);
      if ((ret == KLASSEN_RUN_HEADSETMIC) || (florida_power_status == true)) {
	     florida_power_status = false;	
              printk("%s reenable clk",__func__);
            /*enable 26Mhz clock */
			 clk_monitor(1,1,0);
			 mdelay(5);
             snd_soc_codec_set_pll(florida, FLORIDA_FLL1_REFCLK,
					    ARIZONA_FLL_SRC_NONE, 0, 0);
             snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
						ARIZONA_FLL_SRC_NONE, 0, 0);
             ret = snd_soc_codec_set_sysclk( florida, ARIZONA_CLK_SYSCLK,
						  ARIZONA_CLK_SRC_FLL1,
						  data->pll_freq, 
						  SND_SOC_CLOCK_IN);
			ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
								ARIZONA_CLK_SRC_MCLK1, 
								FLORIDA_PLAT_CLK_HZ,
									(data->pll_freq));
            if (ret != 0) {
                printk("arizona Failed to start SYSCLK: %d\n", ret);
              
	  		  }
		}
			else if (ret == FLORIDA_RUN_MAINMIC) {
        	/* if CODEC is running then switch to the 26M clock */
                              pr_info("%s set mclk1\n",__func__);
			 /*enable 26Mhz clock */
			    clk_monitor(1,1,0);
			    mdelay(5);
			
				ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
						ARIZONA_CLK_SRC_MCLK1,
						FLORIDA_PLAT_CLK_HZ, data->pll_freq);
				if (ret != 0) {
			    	printk("Failed to switch to MCLK1 clock: %d\n", ret);
			    	
		        }
	    }
  
}
static unsigned int Florida_supported_high_sample_rates[] =
{
	16000,
	44100, 
	48000,
	192000, 
};
static struct snd_pcm_hw_constraint_list florida_constraints_sample_rates =
{
	.count = ARRAY_SIZE(Florida_supported_high_sample_rates),
	.list = Florida_supported_high_sample_rates,
	.mask = 0,
};
static int florida_aif1_startup(struct snd_pcm_substream *substream)
{
	int ret ; 
	ret= snd_pcm_hw_constraint_list(substream->runtime, 0,
							SNDRV_PCM_HW_PARAM_RATE,
							&florida_constraints_sample_rates);
	printk("Enter:%s  ret=%d\n", __func__,ret); 
	return ret;  
}
static int florida_aif1_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
//	struct snd_soc_codec *codec = codec_dai->codec;
	struct snd_soc_card *card = codec_dai->card;
	struct florida_drvdata *data = card->drvdata;
	unsigned int pll_out;
	int ret = 0;
	printk("%s arizona aif1_hw start  \n",__func__);
	pll_out = (params_rate(params) % 4000 == 0) ? (FLORIDA_MAX_SYSCLK_1) : (FLORIDA_MAX_SYSCLK_2);
	if (data->pll_freq != pll_out) {
	    data->pll_freq = pll_out;
		 snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
					ARIZONA_FLL_SRC_NONE, 0, 0);
	    ret = snd_soc_codec_set_sysclk(florida, ARIZONA_CLK_SYSCLK, ARIZONA_CLK_SRC_FLL1,
				data->pll_freq, SND_SOC_CLOCK_IN);
		if (ret < 0) {
			printk("arizona Failed to set sysclk: %d, %d\n", pll_out, ret);
			return ret;
		}
		ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
			   ARIZONA_CLK_SRC_MCLK1, FLORIDA_PLAT_CLK_HZ, data->pll_freq);
		if (ret < 0) {
			printk("arizona Failed to set FLL1: %d, %d\n", pll_out, ret);
			return ret;
		}
	}
	printk("%s arizona aif1_hw done  \n",__func__);
	return ret;
}
static int florida_aif_free(struct snd_pcm_substream *substream) 
{ 
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = codec_dai->codec;
	int ret;	   
	printk("%s arizona aif1_free \n",__func__);
	ret = snd_soc_codec_set_sysclk(codec, ARIZONA_CLK_SYSCLK, ARIZONA_CLK_SRC_FLL1,
							0, SND_SOC_CLOCK_IN);
	return ret;
 } 
static struct snd_soc_ops byt_aif1_ops = {
	.startup = florida_aif1_startup,
	.hw_params = florida_aif1_hw_params,
	.hw_free = florida_aif_free, 
};
static unsigned int Florida_supported_high_sample_rates_record[] =
{
	48000, 
};
static struct snd_pcm_hw_constraint_list florida_constraints_sample_rates_record =
{
	.count = ARRAY_SIZE(Florida_supported_high_sample_rates_record),
	.list = Florida_supported_high_sample_rates_record,
	.mask = 0,
};
static int florida_aif2_startup(struct snd_pcm_substream *substream)
{
	int ret ; 
	ret= snd_pcm_hw_constraint_list(substream->runtime, 0,
							SNDRV_PCM_HW_PARAM_RATE,
							&florida_constraints_sample_rates);
	printk("Enter:%s  ret=%d\n", __func__,ret); 
	return ret;  
}
static int florida_aif2_record_startup(struct snd_pcm_substream *substream)
{
	int ret ; 
	printk("Enter:%s  substream->runtime->rate=%d\n", __func__,substream->runtime->rate);
	ret= snd_pcm_hw_constraint_list(substream->runtime, 0,
							SNDRV_PCM_HW_PARAM_RATE,
							&florida_constraints_sample_rates_record);
	printk("Enter:%s  ret=%d\n", __func__,ret); 
	return ret;  
}
static int florida_aif2_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
//	struct snd_soc_codec *codec = codec_dai->codec;
	struct snd_soc_card *card = codec_dai->card;
	struct florida_drvdata *data = card->drvdata;
	unsigned int pll_out;
	int ret = 0;
	pll_out = (params_rate(params) % 4000 == 0) ? (FLORIDA_MAX_SYSCLK_1) : (FLORIDA_MAX_SYSCLK_2);
	if (data->pll_freq != pll_out) {
	    data->pll_freq = pll_out;
		 snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
					ARIZONA_FLL_SRC_NONE, 0, 0);
	    ret = snd_soc_codec_set_sysclk(florida, ARIZONA_CLK_SYSCLK, ARIZONA_CLK_SRC_FLL1,
				data->pll_freq, SND_SOC_CLOCK_IN);
		if (ret < 0) {
			printk("arizona Failed to set sysclk: %d, %d\n", pll_out, ret);
			return ret;
		}
		ret = snd_soc_codec_set_pll(florida, FLORIDA_FLL1,
			   ARIZONA_CLK_SRC_MCLK1, FLORIDA_PLAT_CLK_HZ, data->pll_freq);
		if (ret < 0) {
			printk("arizona Failed to set FLL1: %d, %d\n", pll_out, ret);
			return ret;
		}
	}
	return ret;
}
static int florida_aif2_free(struct snd_pcm_substream *substream) 
{ 
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = codec_dai->codec;
	int ret;	   
	printk("%s arizona aif2_free \n",__func__);
	ret = snd_soc_codec_set_sysclk(codec, ARIZONA_CLK_SYSCLK, ARIZONA_CLK_SRC_FLL1,
							0, SND_SOC_CLOCK_IN);
	return ret;
}
static struct snd_soc_ops byt_aif2_record_ops = {
	.startup = florida_aif2_record_startup,
	.hw_params = florida_aif2_hw_params,
	.hw_free = florida_aif2_free, 
};
#endif  /* CONFIG_SND_SOC_FLORIDA */
static struct snd_soc_ops byt_aif2_ops = {
	.startup = florida_aif2_startup,
	.hw_params = florida_aif2_hw_params,
	.hw_free = florida_aif_free, 
};
/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link mt_soc_dai_common[] = {
	/* FrontEnd DAI Links */
	{
	 .name = "MultiMedia1",
	 .stream_name = MT_SOC_DL1_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_DL1DAI_NAME,
	 .platform_name = MT_SOC_DL1_PCM,
	 .codec_dai_name = MT_SOC_CODEC_TXDAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "MultiMedia2",
	 .stream_name = MT_SOC_UL1_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_UL1DAI_NAME,
	 .platform_name = MT_SOC_UL1_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_RXDAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "Voice_MD1",
	 .stream_name = MT_SOC_VOICE_MD1_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOICE_MD1_NAME,
	 .platform_name = MT_SOC_VOICE_MD1,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_VOICE_MD1DAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "HDMI_OUT",
	 .stream_name = MT_SOC_HDMI_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_HDMI_NAME,
	 .platform_name = MT_SOC_HDMI_PCM,
	 .codec_dai_name = MT_SOC_CODEC_HDMI_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "ULDLOOPBACK",
	 .stream_name = MT_SOC_ULDLLOOPBACK_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_ULDLLOOPBACK_NAME,
	 .platform_name = MT_SOC_ULDLLOOPBACK_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_ULDLLOOPBACK_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "I2S0OUTPUT",
	 .stream_name = MT_SOC_I2S0_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_I2S0_NAME,
	 .platform_name = MT_SOC_I2S0_PCM,
	 .codec_dai_name = MT_SOC_CODEC_I2S0_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "MRGRX",
	 .stream_name = MT_SOC_MRGRX_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_MRGRX_NAME,
	 .platform_name = MT_SOC_MRGRX_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_MRGRX_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "MRGRXCAPTURE",
	 .stream_name = MT_SOC_MRGRX_CAPTURE_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_MRGRX_NAME,
	 .platform_name = MT_SOC_MRGRX_AWB_PCM,
	 .codec_dai_name = MT_SOC_CODEC_MRGRX_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "I2S0DL1OUTPUT",
	 .stream_name = MT_SOC_I2SDL1_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_I2S0DL1_NAME,
	 .platform_name = MT_SOC_I2S0DL1_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ignore_pmdown_time = 1,
        .ops = &byt_aif1_ops,
#else
	 .codec_dai_name = MT_SOC_CODEC_I2S0TXDAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "DL1AWBCAPTURE",
	 .stream_name = MT_SOC_DL1_AWB_RECORD_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_DL1AWB_NAME,
	 .platform_name = MT_SOC_DL1_AWB_PCM,
	 .codec_dai_name = MT_SOC_CODEC_DL1AWBDAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "Voice_MD1_BT",
	 .stream_name = MT_SOC_VOICE_MD1_BT_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOICE_MD1_BT_NAME,
	 .platform_name = MT_SOC_VOICE_MD1_BT,
	 .codec_dai_name = MT_SOC_CODEC_VOICE_MD1_BTDAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "VOIP_CALL_BT_PLAYBACK",
	 .stream_name = MT_SOC_VOIP_BT_OUT_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOIP_CALL_BT_OUT_NAME,
	 .platform_name = MT_SOC_VOIP_BT_OUT,
	 .codec_dai_name = MT_SOC_CODEC_VOIPCALLBTOUTDAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "VOIP_CALL_BT_CAPTURE",
	 .stream_name = MT_SOC_VOIP_BT_IN_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOIP_CALL_BT_IN_NAME,
	 .platform_name = MT_SOC_VOIP_BT_IN,
	 .codec_dai_name = MT_SOC_CODEC_VOIPCALLBTINDAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "TDM_Debug_CAPTURE",
	 .stream_name = MT_SOC_TDM_CAPTURE_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_TDMRX_NAME,
	 .platform_name = MT_SOC_TDMRX_PCM,
	 .codec_dai_name = MT_SOC_CODEC_TDMRX_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "FM_MRG_TX",
	 .stream_name = MT_SOC_FM_MRGTX_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_FM_MRGTX_NAME,
	 .platform_name = MT_SOC_FM_MRGTX_PCM,
	 .codec_dai_name = MT_SOC_CODEC_FMMRGTXDAI_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "MultiMedia3",
	 .stream_name = MT_SOC_UL1DATA2_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_UL2DAI_NAME,
	 .platform_name = MT_SOC_UL2_PCM,
	 .codec_dai_name = MT_SOC_CODEC_RXDAI2_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "I2S0_AWB_CAPTURE",
	 .stream_name = MT_SOC_I2S0AWB_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_I2S0AWBDAI_NAME,
	 .platform_name = MT_SOC_I2S0_AWB_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
		.codec_dai_name = "florida-aif2",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif2_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_I2S0AWB_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif 
	 },

	{
	 .name = "Voice_MD2",
	 .stream_name = MT_SOC_VOICE_MD2_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOICE_MD2_NAME,
	 .platform_name = MT_SOC_VOICE_MD2,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_VOICE_MD2DAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "PLATOFRM_CONTROL",
	 .stream_name = MT_SOC_ROUTING_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_ROUTING_DAI_NAME,
	 .platform_name = MT_SOC_ROUTING_PCM,
	 .codec_dai_name = MT_SOC_CODEC_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init2,
	 .ops = &mtmachine_audio_ops2,
	 },
	{
	 .name = "Voice_MD2_BT",
	 .stream_name = MT_SOC_VOICE_MD2_BT_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_VOICE_MD2_BT_NAME,
	 .platform_name = MT_SOC_VOICE_MD2_BT,
	 .codec_dai_name = MT_SOC_CODEC_VOICE_MD2_BTDAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "HP_IMPEDANCE",
	 .stream_name = MT_SOC_HP_IMPEDANCE_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_HP_IMPEDANCE_NAME,
	 .platform_name = MT_SOC_HP_IMPEDANCE_PCM,
	 .codec_dai_name = MT_SOC_CODEC_HP_IMPEDANCE_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "FM_I2S_RX_Playback",
	 .stream_name = MT_SOC_FM_I2S_PLAYBACK_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_FM_I2S_NAME,
	 .platform_name = MT_SOC_FM_I2S_PCM,
#if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
#else
	 .codec_dai_name = MT_SOC_CODEC_FM_I2S_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
#endif
	 },
	{
	 .name = "FM_I2S_RX_Capture",
	 .stream_name = MT_SOC_FM_I2S_CAPTURE_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_FM_I2S_NAME,
	 .platform_name = MT_SOC_FM_I2S_AWB_PCM,
	 .codec_dai_name = MT_SOC_CODEC_FM_I2S_DUMMY_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	{
	 .name = "OFFLOAD_GDMA_OUT",
	 .stream_name = MT_SOC_OFFLOAD_GDMA_STREAM_NAME,
	 .cpu_dai_name = MT_SOC_OFFLOAD_GDMA_NAME,
	 .platform_name = MT_SOC_OFFLOAD_GDMA_PCM,
	 .codec_dai_name = MT_SOC_CODEC_OFFLOAD_GDMA_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 /* .ops = &mt_machine_audio_ops, */
	 .compr_ops = &mt_machine_audio_compr_ops,
	 },
	{
	 .name = "MultiMedia_DL2",
	 .stream_name = MT_SOC_DL2_STREAM_NAME,
	 .cpu_dai_name   = MT_SOC_DL2DAI_NAME,
	 .platform_name  = MT_SOC_DL2_PCM,
	 #if defined(CONFIG_SND_SOC_FLORIDA)  
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
          #else
	 .codec_dai_name = MT_SOC_CODEC_TXDAI2_NAME,
	 .codec_name = MT_SOC_CODEC_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 #endif
	 },
#ifdef CONFIG_MTK_BTCVSD_ALSA
	 {
	 .name = "BTCVSD_RX",
	 .stream_name = MT_SOC_BTCVSD_CAPTURE_STREAM_NAME,
	 .cpu_dai_name   = MT_SOC_BTCVSD_RX_DAI_NAME,
	 .platform_name  = MT_SOC_BTCVSD_RX_PCM,
	 .codec_dai_name = MT_SOC_CODEC_BTCVSD_RX_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
	 {
	 .name = "BTCVSD_TX",
	 .stream_name = MT_SOC_BTCVSD_PLAYBACK_STREAM_NAME,
	 .cpu_dai_name   = MT_SOC_BTCVSD_TX_DAI_NAME,
	 .platform_name  = MT_SOC_BTCVSD_TX_PCM,
	 .codec_dai_name = MT_SOC_CODEC_BTCVSD_TX_DAI_NAME,
	 .codec_name = MT_SOC_CODEC_DUMMY_NAME,
	 .init = mt_soc_audio_init,
	 .ops = &mt_machine_audio_ops,
	 },
#endif
#if defined(CONFIG_SND_SOC_FLORIDA)
	{
        .name = "mtk-florida-i2s",
        .stream_name = MT_SOC_DUMMY_I2S_STREAM_PCM,
        .cpu_dai_name   = MT_SOC_DUMMY_I2S_DAI_NAME,
       // .platform_name  = MT_SOC_DUMMY_I2S_PCM,
        .codec_dai_name = "florida-aif1",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .init = florida_init,
        .ignore_suspend = 1,
        .ops = &byt_aif1_ops,
        .ignore_pmdown_time = 1,
       },
       {
        .name = "mtk-florida-i2s-record",
        .stream_name = MT_SOC_DUMMY_I2S_RECORD_STREAM_PCM,
        .cpu_dai_name   = MT_SOC_DUMMY_I2S_RECORD_DAI_NAME,
        //.platform_name  = MT_SOC_DUMMY_I2S_PCM,
        .codec_dai_name = "florida-aif2",
        .codec_name = "florida-codec",
        .dai_fmt	= SND_SOC_DAIFMT_I2S
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBS_CFS,
        .ignore_suspend = 1,
        .ops = &byt_aif2_record_ops,
        .ignore_pmdown_time = 1,
       },   
  {
         .name = "CPU-DSP Voice Control",
         .stream_name = "CPU-DSP Voice Control",
         .cpu_dai_name = "florida-cpu-voicectrl",
         .codec_name = "florida-codec",
         .codec_dai_name = "florida-dsp-voicectrl",
         .platform_name = "florida-codec",
         .ignore_suspend = 1,
         .dynamic = 0,
         },

	 {
	.name = "CPU-DSP Trace Debug",
	.stream_name = "CPU-DSP Trace Debug",
	.cpu_dai_name = "florida-cpu-trace",
	.codec_name = "florida-codec",
	.codec_dai_name = "florida-dsp-trace",
	.platform_name = "florida-codec",
	.ignore_suspend = 1,
	.dynamic = 0,
         }     

#endif

};

static const char const *I2S_low_jittermode[] = { "Off", "On" };

static const struct soc_enum mt_soc_machine_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(I2S_low_jittermode), I2S_low_jittermode),
};


static int mt6595_get_lowjitter(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s:  mt_soc_lowjitter_control = %d\n", __func__, mt_soc_lowjitter_control);
	ucontrol->value.integer.value[0] = mt_soc_lowjitter_control;
	return 0;
}

static int mt6595_set_lowjitter(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s()\n", __func__);

	mt_soc_lowjitter_control = ucontrol->value.integer.value[0];
	return 0;
}


static const struct snd_kcontrol_new mt_soc_controls[] = {
	SOC_ENUM_EXT("I2S low Jitter function", mt_soc_machine_enum[0], mt6595_get_lowjitter,
		     mt6595_set_lowjitter),
};

static struct snd_soc_card snd_soc_card_mt = {
	.name = "mt-snd-card",
	.dai_link = mt_soc_dai_common,
	.num_links = ARRAY_SIZE(mt_soc_dai_common),
	.controls = mt_soc_controls,
	.num_controls = ARRAY_SIZE(mt_soc_controls),
#if defined(CONFIG_SND_SOC_FLORIDA)   
	.late_probe = mtk_florida_late_probe,
    .set_bias_level = florida_set_bias_level,
    .set_bias_level_post = florida_set_bias_level_post,
	.dapm_widgets = florida_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(florida_dapm_widgets),	
    .dapm_routes = florida_audio_map,
	.num_dapm_routes = ARRAY_SIZE(florida_audio_map),	
	.drvdata = & drvdata,
#endif	
};

#if 0
static struct platform_device *mt_snd_device;
#endif

#if defined(CONFIG_SND_SOC_FLORIDA)
void k5_arizona_micd_cb(bool mic)
{
       struct florida_drvdata *pdata = NULL;
       WARN_ON(!florida);
       if (!florida)
               return;
        pdata = snd_soc_card_get_drvdata(florida->component.card);
        pdata->ear_mic = mic;
        printk("%s: ear_mic = %d\n", __func__, pdata->ear_mic);
}
static int florida_check_clock_conditions(struct snd_soc_card *card)
{
       struct florida_drvdata *pdata = snd_soc_card_get_drvdata(card);
       int mainmic_state = 0;
#ifdef CONFIG_MFD_FLORIDA
       mainmic_state = snd_soc_dapm_get_pin_status(&card->dapm, "Int Mic");
#endif
       printk("%s: florida->component.active=%d \n",__func__,florida->component.active);
       if (!florida->component.active && mainmic_state) {
               printk("%s: MAIN_MIC is running without input stream\n",__func__);
               return FLORIDA_RUN_MAINMIC;
       }
       if (!florida->component.active && pdata->ear_mic && !mainmic_state) {
               printk("%s: EAR_MIC is running without input stream\n",__func__);
               return KLASSEN_RUN_HEADSETMIC;
       }
       return 0;
}
static void florida_ez2ctrl_cb(void)
{
	struct snd_soc_card *card = &snd_soc_card_mt;
	struct florida_drvdata *data = card->drvdata;
	printk("==============  The Trigger had happened =================\n");
	wake_lock_timeout(&data->wake_lock, 5000);
	if (ez2ctrl_input_dev) {
		input_report_key(ez2ctrl_input_dev, KEY_VOICE_WAKEUP, 1);
		input_report_key(ez2ctrl_input_dev, KEY_VOICE_WAKEUP, 0);
		input_sync(ez2ctrl_input_dev);
	}
}
static int mtk_florida_late_probe(struct snd_soc_card *card)
{
	struct florida_drvdata *data = card->drvdata;
	const struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_codec *codec = NULL;
	rtd = k5_find_dai_link(card, "mtk-florida-i2s-record");
	if (rtd==NULL)
	{
	 printk("Failed to find florida device");
	 return 0;
	}
	codec = rtd->codec_dai->codec;
	snd_soc_dapm_ignore_suspend(&card->dapm, "Int Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Sec Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "ANC Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Headset Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Ext Spk");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Headphone");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Receive");

	
	snd_soc_dapm_sync(&card->dapm);
	snd_soc_dapm_ignore_suspend(&codec->dapm, "IN4L");
    snd_soc_dapm_ignore_suspend(&codec->dapm, "HPOUT2L");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "HPOUT2R");
    snd_soc_dapm_ignore_suspend(&codec->dapm, "HPOUT3L");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "HPOUT3R");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1 Capture");

	snd_soc_dapm_ignore_suspend(&codec->dapm, "SPKOUTLP");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DSP Virtual Out Pin");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DRC2 Signal Activity Pin");

	snd_soc_dapm_sync(&codec->dapm);

	wake_lock_init(&data->wake_lock, WAKE_LOCK_SUSPEND,
				"k5-voicewakeup");
	ez2ctrl_input_dev = input_allocate_device();
	if (!ez2ctrl_input_dev) {
		printk("Failed to allocate Ez2Control input device");
	} else {
		ez2ctrl_input_dev->evbit[0] = BIT_MASK(EV_KEY);
		ez2ctrl_input_dev->keybit[BIT_WORD(KEY_VOICE_WAKEUP)] = BIT_MASK(KEY_VOICE_WAKEUP);
		ez2ctrl_input_dev->keybit[BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER);
		ez2ctrl_input_dev->name = "voice-wakeup";
		if (input_register_device(ez2ctrl_input_dev) != 0) {
			printk("ez2ctrl input device register fail\n");
			input_free_device(ez2ctrl_input_dev);
			ez2ctrl_input_dev = NULL;
		} else {
			arizona_set_ez2ctrl_cb(florida,florida_ez2ctrl_cb);
		}
	}
	arizona_set_hpdet_cb(codec, k5_arizona_hpdet_cb);
	arizona_set_micd_cb(codec, k5_arizona_micd_cb);
	printk("arizona success to register device");
    return 0;
}
#endif
static int mt_soc_snd_init(struct platform_device *pdev)
{
	int ret=0;
	struct snd_soc_card *card = &snd_soc_card_mt;
         printk("arizona mt_soc_snd_init card addr = %p \n", card);

	card->dev = &pdev->dev;
	platform_set_drvdata(pdev, card);
	ret = snd_soc_register_card(card);
	if (ret) {
		pr_debug("arizona snd_soc_register_card failed %d\n", ret);
	}
#if 0 /*move sound card register to register card */
	mt_snd_device = platform_device_alloc("soc-audio", -1);
    if (!mt_snd_device)
    {
        printk("arizona mt6589_probe  platform_device_alloc fail\n");
		return -ENOMEM;
	}
	card->dev = &mt_snd_device->dev;
	platform_set_drvdata(mt_snd_device, &snd_soc_card_mt);
	ret = platform_device_add(mt_snd_device);

    if (ret != 0)
    {
        printk("arizona mt_soc_snd_init goto put_device fail\n");
		goto put_device;
	}
#endif

#if 0
    printk("mt_soc_snd_init dai_link = %p \n", snd_soc_card_mt.dai_link);
#endif
	/* create debug file */
	mt_sco_audio_debugfs = debugfs_create_file(DEBUG_FS_NAME,
						   S_IFREG | S_IRUGO, NULL, (void *)DEBUG_FS_NAME,
						   &mtaudio_debug_ops);


	/* create analog debug file */
	mt_sco_audio_debugfs = debugfs_create_file(DEBUG_ANA_FS_NAME,
						   S_IFREG | S_IRUGO, NULL,
						   (void *)DEBUG_ANA_FS_NAME,
						   &mtaudio_ana_debug_ops);

	return ret;
#if 0
put_device:
	platform_device_put(mt_snd_device);
	return ret;
#endif
}

static int mt_soc_snd_exit(struct platform_device *pdev)
{
	int ret=0;
    	snd_soc_unregister_card(&snd_soc_card_mt);
	return ret;
}

const struct dev_pm_ops florida_pm_ops = {
       .suspend = snd_soc_suspend,
       .resume = snd_soc_resume,
       .freeze = snd_soc_suspend,
       .thaw = snd_soc_resume,
       .poweroff = snd_soc_poweroff,
       .restore = snd_soc_resume,
       .prepare = florida_powerdown_prepare,
       .complete = florida_powerdown_complete,
};
static struct platform_driver florida_sound_card = {
	.driver = {
		.name = "mt-snd-card",
		.owner = THIS_MODULE,
		.pm = &florida_pm_ops,
	},
	.probe = mt_soc_snd_init,
	.remove = mt_soc_snd_exit,
};
static int __init florida_sound_card_init(void)
{
 	printk("arizona florida_sound_card_init \n");
	clk_monitor(1,1,0);
	return platform_driver_register(&florida_sound_card);
}
static void __exit florida_sound_card_exit(void)
{
	struct snd_soc_card *card = &snd_soc_card_mt;
	struct florida_drvdata *data = card->drvdata;
	wake_lock_destroy(&data->wake_lock);
	platform_driver_unregister(&florida_sound_card);
}
#if defined(CONFIG_SND_SOC_FLORIDA)
late_initcall(florida_sound_card_init);
#else
//module_init(mt_soc_snd_init);
#endif
module_exit(florida_sound_card_exit);

/* Module information */
MODULE_AUTHOR("ChiPeng <chipeng.chang@mediatek.com>");
MODULE_DESCRIPTION("ALSA SoC driver ");

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt-snd-card");
