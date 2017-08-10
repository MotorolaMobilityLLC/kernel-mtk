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
 *  mt6797_sound.c
 *
 * Project:
 * --------
 *   MT6797  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
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

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_afe_connection.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_digital_type.h"

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
#include <linux/uaccess.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <asm/div64.h>

#include <mt-plat/mt_lpae.h>

/*#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>*/

static const uint16_t kSideToneCoefficientTable16k[] = {
	0x049C, 0x09E8, 0x09E0, 0x089C,
	0xFF54, 0xF488, 0xEAFC, 0xEBAC,
	0xfA40, 0x17AC, 0x3D1C, 0x6028,
	0x7538
};


static const uint16_t kSideToneCoefficientTable32k[] = {
	0xff58, 0x0063, 0x0086, 0x00bf,
	0x0100, 0x013d, 0x0169, 0x0178,
	0x0160, 0x011c, 0x00aa, 0x0011,
	0xff5d, 0xfea1, 0xfdf6, 0xfd75,
	0xfd39, 0xfd5a, 0xfde8, 0xfeea,
	0x005f, 0x0237, 0x0458, 0x069f,
	0x08e2, 0x0af7, 0x0cb2, 0x0df0,
	0x0e96
};

/* Following structures may vary with chips!!!! */
typedef enum {
	AUDIO_APLL1_DIV0 = 0,
	AUDIO_APLL2_DIV0 = 1,
	AUDIO_APLL12_DIV0 = 2,
	AUDIO_APLL12_DIV1 = 3,
	AUDIO_APLL12_DIV2 = 4,
	AUDIO_APLL12_DIV3 = 5,
	AUDIO_APLL12_DIV4 = 6,
	AUDIO_APLL12_DIVB = 7,
	AUDIO_APLL_DIV_NUM
} AUDIO_APLL_DIVIDER_GROUP;

static const uint32 mMemIfSampleRate[Soc_Aud_Digital_Block_MEM_I2S+1][3] = { /* reg, bit position, bit mask */
	{AFE_DAC_CON1, 0, 0xf}, /* Soc_Aud_Digital_Block_MEM_DL1 */
	{AFE_DAC_CON1, 4, 0xf}, /* Soc_Aud_Digital_Block_MEM_DL2 */
	{AFE_DAC_CON1, 16, 0xf}, /* Soc_Aud_Digital_Block_MEM_VUL */
	{AFE_DAC_CON0, 24, 0x3}, /* Soc_Aud_Digital_Block_MEM_DAI */
	{AFE_DAC_CON0, 12, 0xf}, /* Soc_Aud_Digital_Block_MEM_DL3 */
	{AFE_DAC_CON1, 12, 0xf}, /* Soc_Aud_Digital_Block_MEM_AWB */
	{AFE_DAC_CON1, 30, 0x3}, /* Soc_Aud_Digital_Block_MEM_MOD_DAI */
	{AFE_DAC_CON0, 16, 0xf}, /* Soc_Aud_Digital_Block_MEM_DL1_DATA2 */
	{AFE_DAC_CON0, 20, 0xf}, /* Soc_Aud_Digital_Block_MEM_VUL_DATA2 */
	{AFE_DAC_CON1, 8, 0xf}, /* Soc_Aud_Digital_Block_MEM_I2S */
};

static const uint32 mMemIfChannels[Soc_Aud_Digital_Block_MEM_I2S+1][3] = { /* reg, bit position, bit mask */
	{AFE_DAC_CON1, 21, 0x1}, /* Soc_Aud_Digital_Block_MEM_DL1 */
	{AFE_DAC_CON1, 22, 0x1}, /* Soc_Aud_Digital_Block_MEM_DL2 */
	{AFE_DAC_CON1, 27, 0x1}, /* Soc_Aud_Digital_Block_MEM_VUL */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DAI */
	{AFE_DAC_CON1, 23, 0x1}, /* Soc_Aud_Digital_Block_MEM_DL3 */
	{AFE_DAC_CON1, 24, 0x1}, /* Soc_Aud_Digital_Block_MEM_AWB */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_MOD_DAI */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL1_DATA2 */
	{AFE_DAC_CON0, 10, 0x1}, /* Soc_Aud_Digital_Block_MEM_VUL_DATA2 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_I2S */
};

static const uint32 mMemIfMonoChSelect[Soc_Aud_Digital_Block_MEM_I2S+1][3] = { /* reg, bit position, bit mask */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL1 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL2 */
	{AFE_DAC_CON1, 28, 0x1}, /* Soc_Aud_Digital_Block_MEM_VUL */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DAI */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL3 */
	{AFE_DAC_CON1, 25, 0x1}, /* Soc_Aud_Digital_Block_MEM_AWB */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_MOD_DAI */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL1_DATA2 */
	{AFE_DAC_CON0, 11, 0x1}, /* Soc_Aud_Digital_Block_MEM_VUL_DATA2 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_I2S */
};

static const uint32 mMemDuplicateWrite[Soc_Aud_Digital_Block_MEM_I2S+1][3] = { /* reg, bit position, bit mask */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL1 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL2 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_VUL */
	{AFE_DAC_CON1, 29, 0x1}, /* Soc_Aud_Digital_Block_MEM_DAI */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL3 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_AWB */
	{AFE_DAC_CON0, 26, 0x1}, /* Soc_Aud_Digital_Block_MEM_MOD_DAI */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_DL1_DATA2 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_VUL_DATA2 */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_Digital_Block_MEM_I2S */
};

static const uint32 mMemAudioBlockEnableReg[][MEM_BLOCK_ENABLE_REG_INDEX_NUM] = { /* audio block, reg, bit position */
	{Soc_Aud_Digital_Block_MEM_DL1, AFE_DAC_CON0, 1},
	{Soc_Aud_Digital_Block_MEM_DL2, AFE_DAC_CON0, 2},
	{Soc_Aud_Digital_Block_MEM_VUL, AFE_DAC_CON0, 3},
	{Soc_Aud_Digital_Block_MEM_DAI, AFE_DAC_CON0, 4},
	{Soc_Aud_Digital_Block_MEM_DL3, AFE_DAC_CON0, 5},
	{Soc_Aud_Digital_Block_MEM_AWB, AFE_DAC_CON0, 6},
	{Soc_Aud_Digital_Block_MEM_MOD_DAI, AFE_DAC_CON0, 7},
	{Soc_Aud_Digital_Block_MEM_DL1_DATA2, AFE_DAC_CON0, 8},
	{Soc_Aud_Digital_Block_MEM_VUL_DATA2, AFE_DAC_CON0, 9},
};

const struct Aud_IRQ_CTRL_REG mIRQCtrlRegs[Soc_Aud_IRQ_MCU_MODE_NUM] = {
	{	/*IRQ0*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ1*/
		{AFE_IRQ_MCU_CON, 0, 0x1}, /* irq on */
		{AFE_IRQ_MCU_CON, 4, 0xf}, /* irq mode */
		{AFE_IRQ_MCU_CNT1, 0, 0x3ffff}, /* irq count */
		{AFE_IRQ_MCU_CLR, 0, 0x1}, /* irq clear */
		{AFE_IRQ_MCU_CLR, 8, 0x1}, /* irq miss clear */
		{AFE_IRQ_MCU_STATUS, 0, 0x1}, /* irq status */
		{AFE_IRQ_MCU_EN, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ2*/
		{AFE_IRQ_MCU_CON, 1, 0x1}, /* irq on */
		{AFE_IRQ_MCU_CON, 8, 0xf}, /* irq mode */
		{AFE_IRQ_MCU_CNT2, 0, 0x3ffff}, /* irq count */
		{AFE_IRQ_MCU_CLR, 1, 0x1}, /* irq clear */
		{AFE_IRQ_MCU_CLR, 9, 0x1}, /* irq miss clear */
		{AFE_IRQ_MCU_STATUS, 1, 0x1}, /* irq status */
		{AFE_IRQ_MCU_EN, 1, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ3*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ4*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ5*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ6*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ7*/
		{AFE_IRQ_MCU_CON, 14, 0x1}, /* irq on */
		{AFE_IRQ_MCU_CON, 24, 0xf}, /* irq mode */
		{AFE_IRQ_MCU_CNT7, 0, 0x3ffff}, /* irq count */
		{AFE_IRQ_MCU_CLR, 6, 0x1}, /* irq clear */
		{AFE_IRQ_MCU_CLR, 14, 0x1}, /* irq miss clear */
		{AFE_IRQ_MCU_STATUS, 6, 0x1}, /* irq status */
		{AFE_IRQ_MCU_EN, 6, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ8*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ9*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ10*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ11*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ12*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ_ACC1*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
	{	/*IRQ_ACC2*/
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq on */
		{AFE_REG_UNDEFINED, 0, 0xf}, /* irq mode */
		{AFE_REG_UNDEFINED, 0, 0x3ffff}, /* irq count */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq miss clear */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq status */
		{AFE_REG_UNDEFINED, 0, 0x1}, /* irq enable */
		 Soc_Aud_IRQ_MCU /* irq use for specify purpose */
	},
};

const struct Aud_RegBitsInfo mIRQPurposeRegs[Soc_Aud_IRQ_PURPOSE_NUM] = {
	{AFE_IRQ_MCU_EN, 0, 0x7f}, /* Soc_Aud_IRQ_MCU */
	{AFE_IRQ_MCU_EN, 8, 0x3f}, /* Soc_Aud_IRQ_MD32 */
	{AFE_IRQ_MCU_EN, 16, 0x3f}, /* Soc_Aud_IRQ_MD32_H */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_IRQ_DSP */
	{AFE_REG_UNDEFINED, 0, 0x0}, /* Soc_Aud_IRQ_CM4 */
};

/*  Above structures may vary with chips!!!! */

static const int MEM_BLOCK_ENABLE_REG_NUM = sizeof(mMemAudioBlockEnableReg) / sizeof(mMemAudioBlockEnableReg[0]);

void Afe_Log_Print(void)
{
	AudDrv_Clk_On();
	pr_debug("+AudDrv Afe_Log_Print\n");
	pr_debug("AUDIO_TOP_CON0 = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON0));
	pr_debug("AUDIO_TOP_CON1 = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON1));
	pr_debug("AUDIO_TOP_CON3 = 0x%x\n", Afe_Get_Reg(AUDIO_TOP_CON3));
	pr_debug("AFE_DAC_CON0 = 0x%x\n", Afe_Get_Reg(AFE_DAC_CON0));
	pr_debug("AFE_DAC_CON1 = 0x%x\n", Afe_Get_Reg(AFE_DAC_CON1));
	pr_debug("AFE_I2S_CON = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON));
	pr_debug("AFE_DAIBT_CON0 = 0x%x\n", Afe_Get_Reg(AFE_DAIBT_CON0));
	pr_debug("AFE_CONN0 = 0x%x\n", Afe_Get_Reg(AFE_CONN0));
	pr_debug("AFE_CONN1 = 0x%x\n", Afe_Get_Reg(AFE_CONN1));
	pr_debug("AFE_CONN2 = 0x%x\n", Afe_Get_Reg(AFE_CONN2));
	pr_debug("AFE_CONN3 = 0x%x\n", Afe_Get_Reg(AFE_CONN3));
	pr_debug("AFE_CONN4 = 0x%x\n", Afe_Get_Reg(AFE_CONN4));
	pr_debug("AFE_I2S_CON1 = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON1));
	pr_debug("AFE_I2S_CON2 = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON2));
	pr_debug("AFE_MRGIF_CON = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_CON));
	pr_debug("AFE_DL1_BASE = 0x%x\n", Afe_Get_Reg(AFE_DL1_BASE));
	pr_debug("AFE_DL1_CUR = 0x%x\n", Afe_Get_Reg(AFE_DL1_CUR));
	pr_debug("AFE_DL1_END = 0x%x\n", Afe_Get_Reg(AFE_DL1_END));
	pr_debug("AFE_I2S_CON3 = 0x%x\n", Afe_Get_Reg(AFE_I2S_CON3));
	pr_debug("AFE_DL2_BASE = 0x%x\n", Afe_Get_Reg(AFE_DL2_BASE));
	pr_debug("AFE_DL2_CUR = 0x%x\n", Afe_Get_Reg(AFE_DL2_CUR));
	pr_debug("AFE_DL2_END = 0x%x\n", Afe_Get_Reg(AFE_DL2_END));
	pr_debug("AFE_CONN5 = 0x%x\n", Afe_Get_Reg(AFE_CONN5));
	pr_debug("AFE_CONN_24BIT = 0x%x\n", Afe_Get_Reg(AFE_CONN_24BIT));
	pr_debug("AFE_AWB_BASE = 0x%x\n", Afe_Get_Reg(AFE_AWB_BASE));
	pr_debug("AFE_AWB_END = 0x%x\n", Afe_Get_Reg(AFE_AWB_END));
	pr_debug("AFE_AWB_CUR = 0x%x\n", Afe_Get_Reg(AFE_AWB_CUR));
	pr_debug("AFE_VUL_BASE = 0x%x\n", Afe_Get_Reg(AFE_VUL_BASE));
	pr_debug("AFE_VUL_END = 0x%x\n", Afe_Get_Reg(AFE_VUL_END));
	pr_debug("AFE_VUL_CUR = 0x%x\n", Afe_Get_Reg(AFE_VUL_CUR));
	pr_debug("AFE_DAI_BASE = 0x%x\n", Afe_Get_Reg(AFE_DAI_BASE));
	pr_debug("AFE_DAI_END = 0x%x\n", Afe_Get_Reg(AFE_DAI_END));
	pr_debug("AFE_DAI_CUR = 0x%x\n", Afe_Get_Reg(AFE_DAI_CUR));
	pr_debug("AFE_CONN6 = 0x%x\n", Afe_Get_Reg(AFE_CONN6));
	pr_debug("AFE_MEMIF_MSB = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MSB));
	pr_debug("AFE_MEMIF_MON0 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON0));
	pr_debug("AFE_MEMIF_MON1 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON1));
	pr_debug("AFE_MEMIF_MON2 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON2));
	pr_debug("AFE_MEMIF_MON4 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON4));
	pr_debug("AFE_MEMIF_MON5 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON5));
	pr_debug("AFE_MEMIF_MON6 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON6));
	pr_debug("AFE_MEMIF_MON7 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON7));
	pr_debug("AFE_MEMIF_MON8 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON8));
	pr_debug("AFE_MEMIF_MON9 = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MON9));
	pr_debug("AFE_ADDA_DL_SRC2_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON0));
	pr_debug("AFE_ADDA_DL_SRC2_CON1 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_DL_SRC2_CON1));
	pr_debug("AFE_ADDA_UL_SRC_CON0  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_SRC_CON0));
	pr_debug("AFE_ADDA_UL_SRC_CON1  = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_SRC_CON1));
	pr_debug("AFE_ADDA_TOP_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_TOP_CON0));
	pr_debug("AFE_ADDA_UL_DL_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_UL_DL_CON0));
	pr_debug("AFE_ADDA_SRC_DEBUG = 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG));
	pr_debug("AFE_ADDA_SRC_DEBUG_MON0= 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON0));
	pr_debug("AFE_ADDA_SRC_DEBUG_MON1= 0x%x\n", Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON1));
	pr_debug("AFE_ADDA_NEWIF_CFG0 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_NEWIF_CFG0));
	pr_debug("AFE_ADDA_NEWIF_CFG1 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_NEWIF_CFG1));
	pr_debug("AFE_SIDETONE_DEBUG = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_DEBUG));
	pr_debug("AFE_SIDETONE_MON = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_MON));
	pr_debug("AFE_SIDETONE_CON0 = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_CON0));
	pr_debug("AFE_SIDETONE_COEFF = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_COEFF));
	pr_debug("AFE_SIDETONE_CON1 = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_CON1));
	pr_debug("AFE_SIDETONE_GAIN = 0x%x\n", Afe_Get_Reg(AFE_SIDETONE_GAIN));
	pr_debug("AFE_SGEN_CON0 = 0x%x\n", Afe_Get_Reg(AFE_SGEN_CON0));
	pr_debug("AFE_TOP_CON0 = 0x%x\n", Afe_Get_Reg(AFE_TOP_CON0));
	pr_debug("AFE_BUS_CFG = 0x%x\n", Afe_Get_Reg(AFE_BUS_CFG));
	pr_debug("AFE_BUS_MON = 0x%x\n", Afe_Get_Reg(AFE_BUS_MON));
	pr_debug("AFE_ADDA_PREDIS_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_PREDIS_CON0));
	pr_debug("AFE_ADDA_PREDIS_CON1 = 0x%x\n", Afe_Get_Reg(AFE_ADDA_PREDIS_CON1));
	pr_debug("AFE_MRGIF_MON0 = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON0));
	pr_debug("AFE_MRGIF_MON1 = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON1));
	pr_debug("AFE_MRGIF_MON2 = 0x%x\n", Afe_Get_Reg(AFE_MRGIF_MON2));
	pr_debug("AFE_DAC_CON2 = 0x%x\n", Afe_Get_Reg(AFE_DAC_CON2));
	pr_debug("AFE_VUL2_BASE = 0x%x\n", Afe_Get_Reg(AFE_VUL2_BASE));
	pr_debug("AFE_VUL2_END = 0x%x\n", Afe_Get_Reg(AFE_VUL2_END));
	pr_debug("AFE_VUL2_CUR = 0x%x\n", Afe_Get_Reg(AFE_VUL2_CUR));
	pr_debug("AFE_MOD_DAI_BASE = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_BASE));
	pr_debug("AFE_MOD_DAI_END = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_END));
	pr_debug("AFE_MOD_DAI_CUR = 0x%x\n", Afe_Get_Reg(AFE_MOD_DAI_CUR));
	pr_debug("AFE_VUL12_BASE = 0x%x\n", Afe_Get_Reg(AFE_VUL12_BASE));
	pr_debug("AFE_VUL12_END = 0x%x\n", Afe_Get_Reg(AFE_VUL12_END));
	pr_debug("AFE_VUL12_CUR = 0x%x\n", Afe_Get_Reg(AFE_VUL12_CUR));
	pr_debug("AFE_IRQ_MCU_CON = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CON));
	pr_debug("AFE_IRQ_MCU_STATUS = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_STATUS));
	pr_debug("AFE_IRQ_MCU_CLR = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CLR));
	pr_debug("AFE_IRQ_MCU_CNT1 = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CNT1));
	pr_debug("AFE_IRQ_MCU_CNT2 = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CNT2));
	pr_debug("AFE_IRQ_MCU_EN = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_EN));
	pr_debug("AFE_IRQ_MCU_MON2 = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_MON2));
	pr_debug("AFE_IRQ1_MCU_CNT_MON = 0x%x\n", Afe_Get_Reg(AFE_IRQ1_MCU_CNT_MON));
	pr_debug("AFE_IRQ2_MCU_CNT_MON = 0x%x\n", Afe_Get_Reg(AFE_IRQ2_MCU_CNT_MON));
	pr_debug("AFE_IRQ1_MCU_EN_CNT_MON= 0x%x\n", Afe_Get_Reg(AFE_IRQ1_MCU_EN_CNT_MON));
	pr_debug("AFE_MEMIF_MAXLEN = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_MAXLEN));
	pr_debug("AFE_MEMIF_PBUF_SIZE = 0x%x\n", Afe_Get_Reg(AFE_MEMIF_PBUF_SIZE));
	pr_debug("AFE_IRQ_MCU_CNT7 = 0x%x\n", Afe_Get_Reg(AFE_IRQ_MCU_CNT7));
	pr_debug("AFE_APLL1_TUNER_CFG = 0x%x\n", Afe_Get_Reg(AFE_APLL1_TUNER_CFG));
	pr_debug("AFE_APLL2_TUNER_CFG = 0x%x\n", Afe_Get_Reg(AFE_APLL2_TUNER_CFG));
	pr_debug("AFE_CONN33 = 0x%x\n", Afe_Get_Reg(AFE_CONN33));
	pr_debug("AFE_GAIN1_CON0 = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON0));
	pr_debug("AFE_GAIN1_CON1 = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON1));
	pr_debug("AFE_GAIN1_CON2 = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON2));
	pr_debug("AFE_GAIN1_CON3 = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CON3));
	pr_debug("AFE_CONN7 = 0x%x\n", Afe_Get_Reg(AFE_CONN7));
	pr_debug("AFE_GAIN1_CUR = 0x%x\n", Afe_Get_Reg(AFE_GAIN1_CUR));
	pr_debug("AFE_GAIN2_CON0 = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON0));
	pr_debug("AFE_GAIN2_CON1 = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON1));
	pr_debug("AFE_GAIN2_CON2 = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON2));
	pr_debug("AFE_GAIN2_CON3 = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CON3));
	pr_debug("AFE_CONN8 = 0x%x\n", Afe_Get_Reg(AFE_CONN8));
	pr_debug("AFE_GAIN2_CUR = 0x%x\n", Afe_Get_Reg(AFE_GAIN2_CUR));
	pr_debug("AFE_CONN9 = 0x%x\n", Afe_Get_Reg(AFE_CONN9));
	pr_debug("AFE_CONN10 = 0x%x\n", Afe_Get_Reg(AFE_CONN10));
	pr_debug("AFE_CONN11 = 0x%x\n", Afe_Get_Reg(AFE_CONN11));
	pr_debug("AFE_CONN12 = 0x%x\n", Afe_Get_Reg(AFE_CONN12));
	pr_debug("AFE_CONN13 = 0x%x\n", Afe_Get_Reg(AFE_CONN13));
	pr_debug("AFE_CONN14 = 0x%x\n", Afe_Get_Reg(AFE_CONN14));
	pr_debug("AFE_CONN15 = 0x%x\n", Afe_Get_Reg(AFE_CONN15));
	pr_debug("AFE_CONN16 = 0x%x\n", Afe_Get_Reg(AFE_CONN16));
	pr_debug("AFE_CONN17 = 0x%x\n", Afe_Get_Reg(AFE_CONN17));
	pr_debug("AFE_CONN18 = 0x%x\n", Afe_Get_Reg(AFE_CONN18));
	pr_debug("AFE_CONN19 = 0x%x\n", Afe_Get_Reg(AFE_CONN19));
	pr_debug("AFE_CONN20 = 0x%x\n", Afe_Get_Reg(AFE_CONN20));
	pr_debug("AFE_CONN21 = 0x%x\n", Afe_Get_Reg(AFE_CONN21));
	pr_debug("AFE_CONN22 = 0x%x\n", Afe_Get_Reg(AFE_CONN22));
	pr_debug("AFE_CONN23 = 0x%x\n", Afe_Get_Reg(AFE_CONN23));
	pr_debug("AFE_CONN24 = 0x%x\n", Afe_Get_Reg(AFE_CONN24));
	pr_debug("AFE_CONN_RS = 0x%x\n", Afe_Get_Reg(AFE_CONN_RS));
	pr_debug("AFE_CONN_DI = 0x%x\n", Afe_Get_Reg(AFE_CONN_DI));
	pr_debug("AFE_CONN25 = 0x%x\n", Afe_Get_Reg(AFE_CONN25));
	pr_debug("AFE_CONN26 = 0x%x\n", Afe_Get_Reg(AFE_CONN26));
	pr_debug("AFE_CONN27 = 0x%x\n", Afe_Get_Reg(AFE_CONN27));
	pr_debug("AFE_CONN28 = 0x%x\n", Afe_Get_Reg(AFE_CONN28));
	pr_debug("AFE_CONN29 = 0x%x\n", Afe_Get_Reg(AFE_CONN29));
	pr_debug("AFE_CONN30 = 0x%x\n", Afe_Get_Reg(AFE_CONN30));
	pr_debug("AFE_CONN31 = 0x%x\n", Afe_Get_Reg(AFE_CONN31));
	pr_debug("AFE_CONN32 = 0x%x\n", Afe_Get_Reg(AFE_CONN32));
	pr_debug("AFE_ASRC_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON0));
	pr_debug("AFE_ASRC_CON1 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON1));
	pr_debug("AFE_ASRC_CON2 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON2));
	pr_debug("AFE_ASRC_CON3 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON3));
	pr_debug("AFE_ASRC_CON4 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON4));
	pr_debug("AFE_ASRC_CON5 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON5));
	pr_debug("AFE_ASRC_CON6 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON6));
	pr_debug("AFE_ASRC_CON7 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON7));
	pr_debug("AFE_ASRC_CON8 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON8));
	pr_debug("AFE_ASRC_CON9 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON9));
	pr_debug("AFE_ASRC_CON10 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON10));
	pr_debug("AFE_ASRC_CON11 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON11));
	pr_debug("PCM_INTF_CON  = 0x%x\n", Afe_Get_Reg(PCM_INTF_CON1));
	pr_debug("PCM_INTF_CON2 = 0x%x\n", Afe_Get_Reg(PCM_INTF_CON2));
	pr_debug("PCM2_INTF_CON = 0x%x\n", Afe_Get_Reg(PCM2_INTF_CON));
	pr_debug("AFE_ASRC_CON13 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON13));
	pr_debug("AFE_ASRC_CON14 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON14));
	pr_debug("AFE_ASRC_CON15 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON15));
	pr_debug("AFE_ASRC_CON16 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON16));
	pr_debug("AFE_ASRC_CON17 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON17));
	pr_debug("AFE_ASRC_CON18 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON18));
	pr_debug("AFE_ASRC_CON19 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON19));
	pr_debug("AFE_ASRC_CON20 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON20));
	pr_debug("AFE_ASRC_CON21 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CON21));
	pr_debug("CLK_AUDDIV_0 = 0x%x\n", Afe_Get_Reg(CLK_AUDDIV_0));
	pr_debug("CLK_AUDDIV_1 = 0x%x\n", Afe_Get_Reg(CLK_AUDDIV_1));
	pr_debug("CLK_AUDDIV_2 = 0x%x\n", Afe_Get_Reg(CLK_AUDDIV_2));
	pr_debug("CLK_AUDDIV_3 = 0x%x\n", Afe_Get_Reg(CLK_AUDDIV_3));
	pr_debug("FPGA_CFG0 = 0x%x\n", Afe_Get_Reg(FPGA_CFG0));
	pr_debug("FPGA_CFG1 = 0x%x\n", Afe_Get_Reg(FPGA_CFG1));
	pr_debug("FPGA_CFG2 = 0x%x\n", Afe_Get_Reg(FPGA_CFG2));
	pr_debug("FPGA_CFG3 = 0x%x\n", Afe_Get_Reg(FPGA_CFG3));
	pr_debug("AFE_ASRC4_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON0));
	pr_debug("AFE_ASRC4_CON1 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON1));
	pr_debug("AFE_ASRC4_CON2 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON2));
	pr_debug("AFE_ASRC4_CON3 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON3));
	pr_debug("AFE_ASRC4_CON4 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON4));
	pr_debug("AFE_ASRC4_CON5 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON5));
	pr_debug("AFE_ASRC4_CON6 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON6));
	pr_debug("AFE_ASRC4_CON7 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON7));
	pr_debug("AFE_ASRC4_CON8 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON8));
	pr_debug("AFE_ASRC4_CON9 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON9));
	pr_debug("AFE_ASRC4_CON10 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON10));
	pr_debug("AFE_ASRC4_CON11 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON11));
	pr_debug("AFE_ASRC4_CON12 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON12));
	pr_debug("AFE_ASRC4_CON13 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON13));
	pr_debug("AFE_ASRC4_CON14 = 0x%x\n", Afe_Get_Reg(AFE_ASRC4_CON14));
	pr_debug("AFE_CONN_RS1 = 0x%x\n", Afe_Get_Reg(AFE_CONN_RS1));
	pr_debug("AFE_CONN_DI1 = 0x%x\n", Afe_Get_Reg(AFE_CONN_DI1));
	pr_debug("AFE_CONN_24BIT_1 = 0x%x\n", Afe_Get_Reg(AFE_CONN_24BIT_1));
	pr_debug("AFE_CONN_REG = 0x%x\n", Afe_Get_Reg(AFE_CONN_REG));
	pr_debug("AFE_CONNSYS_I2S_CON = 0x%x\n", Afe_Get_Reg(AFE_CONNSYS_I2S_CON));
	pr_debug("AFE_ASRC_CONNSYS_CON0 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON0));
	pr_debug("AFE_ASRC_CONNSYS_CON1 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON1));
	pr_debug("AFE_ASRC_CONNSYS_CON2 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON2));
	pr_debug("AFE_ASRC_CONNSYS_CON3 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON3));
	pr_debug("AFE_ASRC_CONNSYS_CON4 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON4));
	pr_debug("AFE_ASRC_CONNSYS_CON5 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON5));
	pr_debug("AFE_ASRC_CONNSYS_CON6 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON6));
	pr_debug("AFE_ASRC_CONNSYS_CON7 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON7));
	pr_debug("AFE_ASRC_CONNSYS_CON8 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON8));
	pr_debug("AFE_ASRC_CONNSYS_CON9 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON9));
	pr_debug("AFE_ASRC_CONNSYS_CON10 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON10));
	pr_debug("AFE_ASRC_CONNSYS_CON11 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON11));
	pr_debug("AFE_ASRC_CONNSYS_CON13 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON13));
	pr_debug("AFE_ASRC_CONNSYS_CON14 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON14));
	pr_debug("AFE_ASRC_CONNSYS_CON15 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON15));
	pr_debug("AFE_ASRC_CONNSYS_CON16 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON16));
	pr_debug("AFE_ASRC_CONNSYS_CON17 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON17));
	pr_debug("AFE_ASRC_CONNSYS_CON18 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON18));
	pr_debug("AFE_ASRC_CONNSYS_CON19 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON19));
	pr_debug("AFE_ASRC_CONNSYS_CON20 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON20));
	pr_debug("AFE_ASRC_CONNSYS_CON21 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON21));
	pr_debug("AFE_ASRC_CONNSYS_CON23 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON23));
	pr_debug("AFE_ASRC_CONNSYS_CON24 = 0x%x\n", Afe_Get_Reg(AFE_ASRC_CONNSYS_CON24));
	pr_debug("AUDIO_CLK_CFG_4 = 0x%x\n", GetClkCfg(AUDIO_CLK_CFG_4));
	pr_debug("AUDIO_CLK_CFG_6 = 0x%x\n", GetClkCfg(AUDIO_CLK_CFG_6));
	AudDrv_Clk_Off();
	pr_debug("-AudDrv Afe_Log_Print\n");
}
/* export symbols for other module using */
EXPORT_SYMBOL(Afe_Log_Print);

void Enable4pin_I2S0_I2S3(uint32 SampleRate, uint32 wLenBit)
{
	/*wLenBit : 0:Soc_Aud_I2S_WLEN_WLEN_32BITS /1:Soc_Aud_I2S_WLEN_WLEN_16BITS */
	uint32 Audio_I2S0 = 0;
	uint32 Audio_I2S3 = 0;

	/*Afe_Set_Reg(AUDIO_TOP_CON1, 0x2,  0x2);*/  /* I2S_SOFT_Reset  4 wire i2s mode*/
	Afe_Set_Reg(AUDIO_TOP_CON1, 0x1 << 4,  0x1 << 4); /* I2S0 clock-gated */
	Afe_Set_Reg(AUDIO_TOP_CON1, 0x1 << 7,  0x1 << 7); /* I2S3 clock-gated */

	/* Set I2S0 configuration */
	Audio_I2S0 |= (Soc_Aud_I2S_IN_PAD_SEL_I2S_IN_FROM_IO_MUX << 28);/* I2S in from io_mux */
	Audio_I2S0 |= Soc_Aud_LOW_JITTER_CLOCK << 12; /* Low jitter mode */
	Audio_I2S0 |= (Soc_Aud_INV_LRCK_NO_INVERSE << 5);
	Audio_I2S0 |= (Soc_Aud_I2S_FORMAT_I2S << 3);
	Audio_I2S0 |= (wLenBit << 1);
	Afe_Set_Reg(AFE_I2S_CON, Audio_I2S0, MASK_ALL);
	pr_debug("Audio_I2S0= 0x%x\n", Audio_I2S0);

	SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S, SampleRate); /* set I2S0 sample rate */

	/* Set I2S3 configuration */
	Audio_I2S3 |= Soc_Aud_LOW_JITTER_CLOCK << 12; /* Low jitter mode */
	Audio_I2S3 |= SampleRateTransform(SampleRate, Soc_Aud_Digital_Block_I2S_IN_2) << 8;
	Audio_I2S3 |= Soc_Aud_I2S_FORMAT_I2S << 3; /*  I2s format */
	Audio_I2S3 |= wLenBit << 1; /* WLEN */
	Afe_Set_Reg(AFE_I2S_CON3, Audio_I2S3, AFE_MASK_ALL);
	pr_debug("Audio_I2S3= 0x%x\n", Audio_I2S3);

	Afe_Set_Reg(AUDIO_TOP_CON1, 0 << 4,  0x1 << 4); /* Clear I2S0 clock-gated */
	Afe_Set_Reg(AUDIO_TOP_CON1, 0 << 7,  0x1 << 7); /* Clear I2S3 clock-gated */

	udelay(200);

	/*Afe_Set_Reg(AUDIO_TOP_CON1, 0,  0x2);*/  /* Clear I2S_SOFT_Reset  4 wire i2s mode*/

	Afe_Set_Reg(AFE_I2S_CON, 0x1, 0x1); /* Enable I2S0 */

	Afe_Set_Reg(AFE_I2S_CON3, 0x1, 0x1); /* Enable I2S3 */
}

void SetChipModemPcmConfig(int modem_index, AudioDigitalPCM p_modem_pcm_attribute)
{
	uint32 reg_pcm2_intf_con = 0;
	uint32 reg_pcm_intf_con1 = 0;

	pr_debug("+%s()\n", __func__);

	if (modem_index == MODEM_1) {
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mTxLchRepeatSel & 0x1) << 13;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mVbt16kModeSel & 0x1) << 12;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mSingelMicSel & 0x1) << 7;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mAsyncFifoSel & 0x1) << 6;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mPcmWordLength & 0x1) << 5;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mPcmModeWidebandSel & 0x3) << 3;
		reg_pcm2_intf_con |= (p_modem_pcm_attribute.mPcmFormat & 0x3) << 1;
		pr_debug("%s(), PCM2_INTF_CON(0x%lx) = 0x%x\n", __func__, PCM2_INTF_CON,
			 reg_pcm2_intf_con);
		Afe_Set_Reg(PCM2_INTF_CON, reg_pcm2_intf_con, MASK_ALL);
	} else if (modem_index == MODEM_2 || modem_index == MODEM_EXTERNAL) {
		/* MODEM_2 use PCM_INTF_CON1 (0x530) !!! */
		if (p_modem_pcm_attribute.mPcmModeWidebandSel == Soc_Aud_PCM_MODE_PCM_MODE_8K) {
			Afe_Set_Reg(AFE_ASRC_CON1, 0x00065900, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON4, 0x00065900, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON7, 0x00032C80, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON1, 0x00065900, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON4, 0x00065900, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON7, 0x00032C80, 0xffffffff);
		} else if (p_modem_pcm_attribute.mPcmModeWidebandSel ==
			   Soc_Aud_PCM_MODE_PCM_MODE_16K) {
			Afe_Set_Reg(AFE_ASRC_CON1, 0x00032C80, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON4, 0x00032C80, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON7, 0x00019640, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON1, 0x00032C80, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON4, 0x00032C80, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON7, 0x00019640, 0xffffffff);
		} else if (p_modem_pcm_attribute.mPcmModeWidebandSel ==
			   Soc_Aud_PCM_MODE_PCM_MODE_32K) {
			Afe_Set_Reg(AFE_ASRC_CON1, 0x00019640, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON4, 0x00019640, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC_CON7, 0x0000CB20, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON1, 0x00019640, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON2, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON3, 0x00400000, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON4, 0x00019640, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON6, 0x007F188F, 0xffffffff);
			Afe_Set_Reg(AFE_ASRC4_CON7, 0x0000CB20, 0xffffffff);
		}

		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mBclkOutInv & 0x01) << 22;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mTxLchRepeatSel & 0x01) << 19;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mVbt16kModeSel & 0x01) << 18;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mExtModemSel & 0x01) << 17;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mExtendBckSyncLength & 0x1F) << 9;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mExtendBckSyncTypeSel & 0x01) << 8;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mSingelMicSel & 0x01) << 7;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mAsyncFifoSel & 0x01) << 6;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mSlaveModeSel & 0x01) << 5;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mPcmModeWidebandSel & 0x03) << 3;
		reg_pcm_intf_con1 |= (p_modem_pcm_attribute.mPcmFormat & 0x03) << 1;
		pr_debug("%s(), PCM_INTF_CON1(0x%lx) = 0x%x", __func__, PCM_INTF_CON1,
			reg_pcm_intf_con1);
		Afe_Set_Reg(PCM_INTF_CON1, reg_pcm_intf_con1, MASK_ALL);
	}
}

bool SetChipModemPcmEnable(int modem_index, bool modem_pcm_on)
{
	uint32 dNeedDisableASM = 0, mPcm1AsyncFifo;

	pr_debug("+%s(), modem_index = %d, modem_pcm_on = %d\n", __func__, modem_index,
		 modem_pcm_on);

	if (modem_index == MODEM_1) {	/* MODEM_1 use PCM2_INTF_CON (0x53C) !!! */
		/* todo:: temp for use fifo */
		Afe_Set_Reg(PCM2_INTF_CON, modem_pcm_on, 0x1);
	} else if (modem_index == MODEM_2 || modem_index == MODEM_EXTERNAL) {
		/* MODEM_2 use PCM_INTF_CON1 (0x530) !!! */
		if (modem_pcm_on == true) {	/* turn on ASRC before Modem PCM on */
			Afe_Set_Reg(PCM_INTF_CON2, (modem_index - 1) << 8, 0x100);
			/* selects internal MD2/MD3 PCM interface (0x538[8]) */
			mPcm1AsyncFifo = (Afe_Get_Reg(PCM_INTF_CON1) & 0x0040) >> 6;
			if (mPcm1AsyncFifo == 0) {
				/* Afe_Set_Reg(AFE_ASRC_CON6, 0x005f188f, MASK_ALL);   // denali marked */
				Afe_Set_Reg(AFE_ASRC_CON0, 0x86083031, MASK_ALL);
				/* Afe_Set_Reg(AFE_ASRC4_CON6, 0x005f188f, MASK_ALL);   // denali marked */
				Afe_Set_Reg(AFE_ASRC4_CON0, 0x06003031, MASK_ALL);
			}
			Afe_Set_Reg(PCM_INTF_CON1, 0x1, 0x1);
		} else if (modem_pcm_on == false) {	/* turn off ASRC after Modem PCM off */
			Afe_Set_Reg(PCM_INTF_CON1, 0x0, 0x1);
			Afe_Set_Reg(AFE_ASRC_CON6, 0x00000000, MASK_ALL);
			dNeedDisableASM = (Afe_Get_Reg(AFE_ASRC_CON0) & 0x1) ? 1 : 0;
			Afe_Set_Reg(AFE_ASRC_CON0, 0, (1 << 4 | 1 << 5 | dNeedDisableASM));
			Afe_Set_Reg(AFE_ASRC_CON0, 0x0, 0x1);
			Afe_Set_Reg(AFE_ASRC4_CON6, 0x00000000, MASK_ALL);
			Afe_Set_Reg(AFE_ASRC4_CON0, 0, (1 << 4 | 1 << 5));
			Afe_Set_Reg(AFE_ASRC4_CON0, 0x0, 0x1);
		}
	} else {
		pr_err("%s(), no such modem_index: %d!!", __func__, modem_index);
		return false;
	}

	return true;
}

bool SetI2SASRCConfig(bool bIsUseASRC, unsigned int dToSampleRate)
{
	pr_debug("+%s() bIsUseASRC [%d] dToSampleRate [%d]\n", __func__, bIsUseASRC, dToSampleRate);

	if (true == bIsUseASRC) {
		BUG_ON(!(dToSampleRate == 44100 || dToSampleRate == 48000));
		Afe_Set_Reg(AFE_CONN4, 0, 1 << 30);
		SetSampleRate(Soc_Aud_Digital_Block_MEM_I2S, dToSampleRate);	/* To target sample rate */

		if (dToSampleRate == 44100)
			Afe_Set_Reg(AFE_ASRC_CON14, 0x001B9000, AFE_MASK_ALL);
		 else
			Afe_Set_Reg(AFE_ASRC_CON14, 0x001E0000, AFE_MASK_ALL);

		Afe_Set_Reg(AFE_ASRC_CON15, 0x00140000, AFE_MASK_ALL);
		Afe_Set_Reg(AFE_ASRC_CON16, 0x00FF5987, AFE_MASK_ALL);
		Afe_Set_Reg(AFE_ASRC_CON17, 0x00007EF4, AFE_MASK_ALL);
		Afe_Set_Reg(AFE_ASRC_CON16, 0x00FF5986, AFE_MASK_ALL);
		Afe_Set_Reg(AFE_ASRC_CON16, 0x00FF5987, AFE_MASK_ALL);

		Afe_Set_Reg(AFE_ASRC_CON13, 0, 1 << 16);	/* 0:Stereo 1:Mono */

		Afe_Set_Reg(AFE_ASRC_CON20, 0x00036000, AFE_MASK_ALL);	/* Calibration setting */
		Afe_Set_Reg(AFE_ASRC_CON21, 0x0002FC00, AFE_MASK_ALL);
	} else {
		Afe_Set_Reg(AFE_CONN4, 1 << 30, 1 << 30);
	}

	return true;
}

bool SetI2SASRCEnable(bool bEnable)
{
	if (true == bEnable) {
		Afe_Set_Reg(AFE_ASRC_CON0, ((1 << 6) | (1 << 0)), ((1 << 6) | (1 << 0)));
	} else {
		uint32 dNeedDisableASM = (Afe_Get_Reg(AFE_ASRC_CON0) & 0x0030) ? 1 : 0;

		Afe_Set_Reg(AFE_ASRC_CON0, 0, (1 << 6 | dNeedDisableASM));
	}

	return true;
}

bool EnableSideToneFilter(bool stf_on)
{
/* MD support 16K/32K sampling rate */
	uint8_t kSideToneHalfTapNum;
	const uint16_t *kSideToneCoefficientTable;
	uint32 eSamplingRate = (Afe_Get_Reg(AFE_ADDA_UL_SRC_CON0) & 0x60000) >> 17;
	uint32 eSamplingRate2 = (Afe_Get_Reg(AFE_ADDA_UL_SRC_CON0) >> 17) & 0x3;

	pr_debug("+%s(), eSamplingRate = %d, eSamplingRate2=%d\n", __func__, eSamplingRate, eSamplingRate2);
	if (eSamplingRate == Soc_Aud_ADDA_UL_SAMPLERATE_32K) {
		kSideToneHalfTapNum = sizeof(kSideToneCoefficientTable32k) / sizeof(uint16_t);
		kSideToneCoefficientTable = kSideToneCoefficientTable32k;
	} else {
	kSideToneHalfTapNum = sizeof(kSideToneCoefficientTable16k) / sizeof(uint16_t);
	kSideToneCoefficientTable = kSideToneCoefficientTable16k;
	}
	pr_debug("+%s(), stf_on = %d, kSTFCoef[0]=0x%x\n", __func__, stf_on, kSideToneCoefficientTable[0]);
	AudDrv_Clk_On();

	if (stf_on == false) {
		/* bypass STF result & disable */
		const bool bypass_stf_on = true;
		uint32_t reg_value = (bypass_stf_on << 31) | (bypass_stf_on << 30) | (stf_on << 8);

		Afe_Set_Reg(AFE_SIDETONE_CON1, reg_value, MASK_ALL);
		pr_debug("%s(), AFE_SIDETONE_CON1[0x%lx] = 0x%x\n", __func__, AFE_SIDETONE_CON1,
			 reg_value);
		/* set side tone gain = 0 */
		Afe_Set_Reg(AFE_SIDETONE_GAIN, 0, MASK_ALL);
		pr_debug("%s(), AFE_SIDETONE_GAIN[0x%lx] = 0x%x\n", __func__, AFE_SIDETONE_GAIN, 0);
	} else {
		const bool bypass_stf_on = false;
		/* using STF result & enable & set half tap num */
		uint32_t write_reg_value =
		    (bypass_stf_on << 31) | (bypass_stf_on << 30) | (stf_on << 8) | kSideToneHalfTapNum;
		/* set side tone coefficient */
		const bool enable_read_write = true;	/* enable read/write side tone coefficient */
		const bool read_write_sel = true;	/* for write case */
		const bool sel_ch2 = false;	/* using uplink ch1 as STF input */
		uint32_t read_reg_value = Afe_Get_Reg(AFE_SIDETONE_CON0);
		size_t coef_addr = 0;

		pr_debug("%s(), AFE_SIDETONE_GAIN[0x%lx] = 0x%x\n", __func__, AFE_SIDETONE_GAIN, 0);

		/* set side tone gain */
		Afe_Set_Reg(AFE_SIDETONE_GAIN, 0, MASK_ALL);
		Afe_Set_Reg(AFE_SIDETONE_CON1, write_reg_value, MASK_ALL);
		pr_debug("%s(), AFE_SIDETONE_CON1[0x%lx] = 0x%x\n", __func__, AFE_SIDETONE_CON1,
			 write_reg_value);

		for (coef_addr = 0; coef_addr < kSideToneHalfTapNum; coef_addr++) {
			bool old_write_ready = (read_reg_value >> 29) & 0x1;
			bool new_write_ready = 0;
			int try_cnt = 0;

			write_reg_value = enable_read_write << 25 |
			read_write_sel	<< 24 |
			sel_ch2		<< 23 |
			coef_addr	<< 16 |
			kSideToneCoefficientTable[coef_addr];
			Afe_Set_Reg(AFE_SIDETONE_CON0, write_reg_value, 0x39FFFFF);
			pr_warn("%s(), AFE_SIDETONE_CON0[0x%lx] = 0x%x\n", __func__, AFE_SIDETONE_CON0,
				write_reg_value);

			/* wait until flag write_ready changed (means write done) */
			for (try_cnt = 0; try_cnt < 10; try_cnt++) { /* max try 10 times */
				/* msleep(3); */
				/* usleep_range(3 * 1000, 20 * 1000); */
				read_reg_value = Afe_Get_Reg(AFE_SIDETONE_CON0);
				new_write_ready = (read_reg_value >> 29) & 0x1;
				if (new_write_ready == old_write_ready) { /* flip => ok */
					udelay(3);
					if (try_cnt == 9) {
						BUG_ON(new_write_ready == old_write_ready);
						AudDrv_Clk_Off();
						return false;
					}
				} else {
					break;
				}

			}
		}

	}

	AudDrv_Clk_Off();
	pr_debug("-%s(), stf_on = %d\n", __func__, stf_on);

	return true;
}

bool CleanPreDistortion(void)
{
	/* printk("%s\n", __FUNCTION__); */
	Afe_Set_Reg(AFE_ADDA_PREDIS_CON0, 0, MASK_ALL);
	Afe_Set_Reg(AFE_ADDA_PREDIS_CON1, 0, MASK_ALL);

	return true;
}

/* Follow 6755 */
bool SetDLSrc2(uint32 SampleRate)
{
	uint32 AfeAddaDLSrc2Con0, AfeAddaDLSrc2Con1;

	if (SampleRate == 8000)
		AfeAddaDLSrc2Con0 = 0;
	else if (SampleRate == 11025)
		AfeAddaDLSrc2Con0 = 1;
	else if (SampleRate == 12000)
		AfeAddaDLSrc2Con0 = 2;
	else if (SampleRate == 16000)
		AfeAddaDLSrc2Con0 = 3;
	else if (SampleRate == 22050)
		AfeAddaDLSrc2Con0 = 4;
	else if (SampleRate == 24000)
		AfeAddaDLSrc2Con0 = 5;
	else if (SampleRate == 32000)
		AfeAddaDLSrc2Con0 = 6;
	else if (SampleRate == 44100)
		AfeAddaDLSrc2Con0 = 7;
	else if (SampleRate == 48000)
		AfeAddaDLSrc2Con0 = 8;
	else
		AfeAddaDLSrc2Con0 = 7;	/* Default 44100 */

	/* ASSERT(0); */
	if (AfeAddaDLSrc2Con0 == 0 || AfeAddaDLSrc2Con0 == 3) {
		/* 8k or 16k voice mode */
		AfeAddaDLSrc2Con0 =
		    (AfeAddaDLSrc2Con0 << 28) | (0x03 << 24) | (0x03 << 11) | (0x01 << 5);
	} else {
		AfeAddaDLSrc2Con0 = (AfeAddaDLSrc2Con0 << 28) | (0x03 << 24) | (0x03 << 11);
	}

		/* SA suggest apply -0.3db to audio/speech path */
	AfeAddaDLSrc2Con0 = AfeAddaDLSrc2Con0 | (0x01 << 1);	/* 2013.02.22 for voice mode degrade 0.3 db */
	AfeAddaDLSrc2Con1 = 0xf74f0000;
	Afe_Set_Reg(AFE_ADDA_DL_SRC2_CON0, AfeAddaDLSrc2Con0, MASK_ALL);
	Afe_Set_Reg(AFE_ADDA_DL_SRC2_CON1, AfeAddaDLSrc2Con1, MASK_ALL);

	return true;
}

uint32 SampleRateTransformI2s(uint32 SampleRate)
{
	switch (SampleRate) {
	case 8000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_8K;
	case 11025:
		return Soc_Aud_I2S_SAMPLERATE_I2S_11K;
	case 12000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_12K;
	case 16000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_16K;
	case 22050:
		return Soc_Aud_I2S_SAMPLERATE_I2S_22K;
	case 24000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_24K;
	case 32000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_32K;
	case 44100:
		return Soc_Aud_I2S_SAMPLERATE_I2S_44K;
	case 48000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_48K;
	case 88200:
		return Soc_Aud_I2S_SAMPLERATE_I2S_88K;
	case 96000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_96K;
	case 176400:
		return Soc_Aud_I2S_SAMPLERATE_I2S_174K;
	case 192000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_192K;
	case 260000:
		return Soc_Aud_I2S_SAMPLERATE_I2S_260K;
	default:
		break;
	}

	return Soc_Aud_I2S_SAMPLERATE_I2S_44K;
}

bool SetChipI2SAdcIn(AudioDigtalI2S *DigtalI2S, bool audioAdcI2SStatus)
{
	uint32 Audio_I2S_Adc = 0;
	AudioDigtalI2S *audioAdcI2S = kzalloc(sizeof(AudioDigtalI2S), GFP_KERNEL);

	memcpy((void *)audioAdcI2S, (void *)DigtalI2S, sizeof(AudioDigtalI2S));
	if (false == audioAdcI2SStatus) {
		uint32 eSamplingRate = SampleRateTransformI2s(audioAdcI2S->mI2S_SAMPLERATE);
		uint32 dVoiceModeSelect = 0;

		Afe_Set_Reg(AFE_ADDA_TOP_CON0, 0, 0x1); /* Using Internal ADC */
		if (eSamplingRate == Soc_Aud_I2S_SAMPLERATE_I2S_8K)
			dVoiceModeSelect = 0;
		else if (eSamplingRate == Soc_Aud_I2S_SAMPLERATE_I2S_16K)
			dVoiceModeSelect = 1;
		else if (eSamplingRate == Soc_Aud_I2S_SAMPLERATE_I2S_32K)
			dVoiceModeSelect = 2;
		else if (eSamplingRate == Soc_Aud_I2S_SAMPLERATE_I2S_48K)
			dVoiceModeSelect = 3;

		Afe_Set_Reg(AFE_ADDA_UL_SRC_CON0,
				(dVoiceModeSelect << 19) | (dVoiceModeSelect << 17), 0x001E0000);
		Afe_Set_Reg(AFE_ADDA_NEWIF_CFG0, 0x03F87201, 0xFFFFFFFF);	/* up8x txif sat on */
		Afe_Set_Reg(AFE_ADDA_NEWIF_CFG1, ((dVoiceModeSelect < 3) ? 1 : 3) << 10,
				0x00000C00);
	} else {
		Afe_Set_Reg(AFE_ADDA_TOP_CON0, 1, 0x1); /* Using External ADC */
		Audio_I2S_Adc |= (audioAdcI2S->mLR_SWAP << 31);
		Audio_I2S_Adc |= (audioAdcI2S->mBuffer_Update_word << 24);
		Audio_I2S_Adc |= (audioAdcI2S->mINV_LRCK << 23);
		Audio_I2S_Adc |= (audioAdcI2S->mFpga_bit_test << 22);
		Audio_I2S_Adc |= (audioAdcI2S->mFpga_bit << 21);
		Audio_I2S_Adc |= (audioAdcI2S->mloopback << 20);
		Audio_I2S_Adc |= (SampleRateTransformI2s(audioAdcI2S->mI2S_SAMPLERATE) << 8);
		Audio_I2S_Adc |= (audioAdcI2S->mI2S_FMT << 3);
		Audio_I2S_Adc |= (audioAdcI2S->mI2S_WLEN << 1);
		pr_debug("%s Audio_I2S_Adc = 0x%x", __func__, Audio_I2S_Adc);
		Afe_Set_Reg(AFE_I2S_CON2, Audio_I2S_Adc, MASK_ALL);
	}
	kfree(audioAdcI2S);

	return true;
}

bool setChipDmicPath(bool _enable, uint32 sample_rate)
{
	return true;
}

bool SetSampleRate(uint32 Aud_block, uint32 SampleRate)
{
	/* pr_warn("%s Aud_block = %d SampleRate = %d\n", __func__, Aud_block, SampleRate); */
	SampleRate = SampleRateTransform(SampleRate, Aud_block);

	switch (Aud_block) {
	case Soc_Aud_Digital_Block_MEM_DL1:
	case Soc_Aud_Digital_Block_MEM_DL1_DATA2:
	case Soc_Aud_Digital_Block_MEM_DL2:
	case Soc_Aud_Digital_Block_MEM_DL3:
	case Soc_Aud_Digital_Block_MEM_I2S:
	case Soc_Aud_Digital_Block_MEM_AWB:
	case Soc_Aud_Digital_Block_MEM_VUL:
	case Soc_Aud_Digital_Block_MEM_DAI:
	case Soc_Aud_Digital_Block_MEM_MOD_DAI:
	case Soc_Aud_Digital_Block_MEM_VUL_DATA2:
		Afe_Set_Reg(mMemIfSampleRate[Aud_block][0], SampleRate << mMemIfSampleRate[Aud_block][1],
			mMemIfSampleRate[Aud_block][2] << mMemIfSampleRate[Aud_block][1]);
		break;
	default:
		pr_err("audio_error: %s(): given Aud_block is not valid!!!!\n", __func__);
		return false;
	}
	return true;
}


bool SetChannels(uint32 Memory_Interface, uint32 channel)
{
	const bool bMono = (channel == 1) ? true : false;
	/* pr_warn("SetChannels Memory_Interface = %d channels = %d\n", Memory_Interface, channel); */
	switch (Memory_Interface) {
	case Soc_Aud_Digital_Block_MEM_DL1:
	case Soc_Aud_Digital_Block_MEM_DL2:
	case Soc_Aud_Digital_Block_MEM_DL3:
	case Soc_Aud_Digital_Block_MEM_AWB:
	case Soc_Aud_Digital_Block_MEM_VUL:
	case Soc_Aud_Digital_Block_MEM_VUL_DATA2:
		Afe_Set_Reg(mMemIfChannels[Memory_Interface][0], bMono << mMemIfChannels[Memory_Interface][1],
			mMemIfChannels[Memory_Interface][2] << mMemIfChannels[Memory_Interface][1]);
		break;
	default:
		pr_warn
		    ("[AudioWarn] SetChannels  Memory_Interface = %d, channel = %d, bMono = %d\n",
		     Memory_Interface, channel, bMono);
		return false;
	}
	return true;
}

int SetMemifMonoSel(uint32 Memory_Interface, bool mono_use_r_ch)
{
	switch (Memory_Interface) {
	case Soc_Aud_Digital_Block_MEM_AWB:
	case Soc_Aud_Digital_Block_MEM_VUL:
	case Soc_Aud_Digital_Block_MEM_VUL_DATA2:
		Afe_Set_Reg(mMemIfMonoChSelect[Memory_Interface][0],
			mono_use_r_ch << mMemIfMonoChSelect[Memory_Interface][1],
			mMemIfMonoChSelect[Memory_Interface][2] << mMemIfMonoChSelect[Memory_Interface][1]);
		break;
	default:
		pr_warn("[AudioWarn] %s(), invalid Memory_Interface = %d\n",
			__func__, Memory_Interface);
		return -EINVAL;
	}
	return 0;
}

bool SetMemDuplicateWrite(uint32 InterfaceType, int dupwrite)
{
	switch (InterfaceType) {
	case Soc_Aud_Digital_Block_MEM_DAI:
	case Soc_Aud_Digital_Block_MEM_MOD_DAI:
		Afe_Set_Reg(mMemDuplicateWrite[InterfaceType][0], dupwrite << mMemDuplicateWrite[InterfaceType][1],
			mMemDuplicateWrite[InterfaceType][2] << mMemDuplicateWrite[InterfaceType][1]);
		break;
	default:
		return false;
	}

	return true;
}

uint32 GetEnableAudioBlockRegInfo(uint32 Aud_block, int index)
{
	int i = 0;

	for (i = 0; i < MEM_BLOCK_ENABLE_REG_NUM; i++) {
		if (mMemAudioBlockEnableReg[i][MEM_BLOCK_ENABLE_REG_INDEX_AUDIO_BLOCK] == Aud_block)
			return mMemAudioBlockEnableReg[i][index];
	}
	return 0; /* 0: no such bit */
}

uint32 GetEnableAudioBlockRegAddr(uint32 Aud_block)
{
	return GetEnableAudioBlockRegInfo(Aud_block, MEM_BLOCK_ENABLE_REG_INDEX_REG);
}

uint32 GetEnableAudioBlockRegOffset(uint32 Aud_block)
{
	return GetEnableAudioBlockRegInfo(Aud_block, MEM_BLOCK_ENABLE_REG_INDEX_OFFSET);
}

bool SetMemIfFormatReg(uint32 InterfaceType, uint32 eFetchFormat)
{
	switch (InterfaceType) {
	case Soc_Aud_Digital_Block_MEM_DL1:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 16,
				    0x00030000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_DL1_DATA2:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 12,
				    0x00003000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_DL2:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 18,
				    0x000c0000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_I2S:{
			/* Afe_Set_Reg(AFE_DAC_CON1, mAudioMEMIF[InterfaceType].mSampleRate << 8 , 0x00000f00); */
			pr_warn("Unsupport MEM_I2S");
			break;
		}
	case Soc_Aud_Digital_Block_MEM_AWB:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
					eFetchFormat << 20,
				    0x00300000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_VUL:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 22,
				    0x00C00000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_VUL_DATA2:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 14,
				    0x0000C000);
			break;
		}
	case Soc_Aud_Digital_Block_MEM_DAI:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 24,
				    0x03000000);
			break;
		}

	case Soc_Aud_Digital_Block_MEM_MOD_DAI:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 26,
				    0x0C000000);
			break;
		}

	case Soc_Aud_Digital_Block_MEM_HDMI:{
			Afe_Set_Reg(AFE_MEMIF_PBUF_SIZE,
				    eFetchFormat << 28,
				    0x30000000);
			break;
		}
	default:
		return false;
	}

	return true;
}

ssize_t AudDrv_Reg_Dump(char *buffer, int size)
{
	int n = 0;

	pr_debug("mt_soc_debug_read\n");
	n = scnprintf(buffer + n, size - n, "AUDIO_TOP_CON0  = 0x%x\n",
			Afe_Get_Reg(AUDIO_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON1		   = 0x%x\n",
			Afe_Get_Reg(AUDIO_TOP_CON1));
	n += scnprintf(buffer + n, size - n, "AUDIO_TOP_CON3		   = 0x%x\n",
			Afe_Get_Reg(AUDIO_TOP_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON		   = 0x%x\n",
			Afe_Get_Reg(AFE_I2S_CON));
	n += scnprintf(buffer + n, size - n, "AFE_DAIBT_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAIBT_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_CONN0			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN0));
	n += scnprintf(buffer + n, size - n, "AFE_CONN1			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN1));
	n += scnprintf(buffer + n, size - n, "AFE_CONN2			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN2));
	n += scnprintf(buffer + n, size - n, "AFE_CONN3			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN3));
	n += scnprintf(buffer + n, size - n, "AFE_CONN4			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN4));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_I2S_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_I2S_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_CON		   = 0x%x\n",
			Afe_Get_Reg(AFE_MRGIF_CON));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL1_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL1_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL1_END));
	n += scnprintf(buffer + n, size - n, "AFE_I2S_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_I2S_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DL2_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_DL2_END));
	n += scnprintf(buffer + n, size - n, "AFE_CONN5			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN5));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_24BIT		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_24BIT));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_AWB_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_AWB_END));
	n += scnprintf(buffer + n, size - n, "AFE_AWB_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_AWB_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAI_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAI_END));
	n += scnprintf(buffer + n, size - n, "AFE_DAI_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_DAI_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_CONN6			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN6));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MSB		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MSB));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON4		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON4));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON5		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON5));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON6		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON6));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON7		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON7));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON8		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON8));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MON9		   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MON9));
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
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG	   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_SRC_DEBUG));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG_MON0= 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_SRC_DEBUG_MON1= 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_SRC_DEBUG_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_NEWIF_CFG0    = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_NEWIF_CFG1    = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_NEWIF_CFG1));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_DEBUG	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_DEBUG));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_MON	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_MON));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_CON0	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_COEFF	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_COEFF));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_CON1	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_SIDETONE_GAIN	   = 0x%x\n",
			Afe_Get_Reg(AFE_SIDETONE_GAIN));
	n += scnprintf(buffer + n, size - n, "AFE_SGEN_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_SGEN_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_TOP_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_BUS_CFG		   = 0x%x\n",
			Afe_Get_Reg(AFE_BUS_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_BUS_MON		   = 0x%x\n",
			Afe_Get_Reg(AFE_BUS_MON));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_PREDIS_CON0   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_PREDIS_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA_PREDIS_CON1   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA_PREDIS_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_MRGIF_MON0));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_MRGIF_MON1));
	n += scnprintf(buffer + n, size - n, "AFE_MRGIF_MON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_MRGIF_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_DAC_CON2   = 0x%x\n",
			Afe_Get_Reg(AFE_DAC_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_VUL2_BASE		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL2_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL2_END		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL2_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL2_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_BASE	   = 0x%x\n",
			Afe_Get_Reg(AFE_MOD_DAI_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_END	   = 0x%x\n",
			Afe_Get_Reg(AFE_MOD_DAI_END));
	n += scnprintf(buffer + n, size - n, "AFE_MOD_DAI_CUR	   = 0x%x\n",
			Afe_Get_Reg(AFE_MOD_DAI_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_VUL12_BASE	   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL12_BASE));
	n += scnprintf(buffer + n, size - n, "AFE_VUL12_END	   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL12_END));
	n += scnprintf(buffer + n, size - n, "AFE_VUL12_CUR	   = 0x%x\n",
			Afe_Get_Reg(AFE_VUL12_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CON	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_CON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_STATUS	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_STATUS));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CLR	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_CLR));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT1	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_CNT1));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT2	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_CNT2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_EN		   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_EN));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_MON2	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_MON2));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_MCU_CNT_MON   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ1_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ2_MCU_CNT_MON   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ2_MCU_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ1_MCU_EN_CNT_MON= 0x%x\n",
			Afe_Get_Reg(AFE_IRQ1_MCU_EN_CNT_MON));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_MAXLEN	   = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_MAXLEN));
	n += scnprintf(buffer + n, size - n, "AFE_MEMIF_PBUF_SIZE    = 0x%x\n",
			Afe_Get_Reg(AFE_MEMIF_PBUF_SIZE));
	n += scnprintf(buffer + n, size - n, "AFE_IRQ_MCU_CNT7	   = 0x%x\n",
			Afe_Get_Reg(AFE_IRQ_MCU_CNT7));
	n += scnprintf(buffer + n, size - n, "AFE_APLL1_TUNER_CFG    = 0x%x\n",
			Afe_Get_Reg(AFE_APLL1_TUNER_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_APLL2_TUNER_CFG    = 0x%x\n",
			Afe_Get_Reg(AFE_APLL2_TUNER_CFG));
	n += scnprintf(buffer + n, size - n, "AFE_CONN33    = 0x%x\n",
			Afe_Get_Reg(AFE_CONN33));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN1_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN1_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN1_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN1_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_CONN7		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN7));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN1_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN1_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN2_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN2_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN2_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN2_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_CONN8		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN8));
	n += scnprintf(buffer + n, size - n, "AFE_GAIN2_CUR		   = 0x%x\n",
			Afe_Get_Reg(AFE_GAIN2_CUR));
	n += scnprintf(buffer + n, size - n, "AFE_CONN9			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN9));
	n += scnprintf(buffer + n, size - n, "AFE_CONN10		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN10));
	n += scnprintf(buffer + n, size - n, "AFE_CONN11		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN11));
	n += scnprintf(buffer + n, size - n, "AFE_CONN12		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN12));
	n += scnprintf(buffer + n, size - n, "AFE_CONN13		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN13));
	n += scnprintf(buffer + n, size - n, "AFE_CONN14		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN14));
	n += scnprintf(buffer + n, size - n, "AFE_CONN15		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN15));
	n += scnprintf(buffer + n, size - n, "AFE_CONN16		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN16));
	n += scnprintf(buffer + n, size - n, "AFE_CONN17		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN17));
	n += scnprintf(buffer + n, size - n, "AFE_CONN18		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN18));
	n += scnprintf(buffer + n, size - n, "AFE_CONN19			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN19));
	n += scnprintf(buffer + n, size - n, "AFE_CONN20		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN20));
	n += scnprintf(buffer + n, size - n, "AFE_CONN21		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN21));
	n += scnprintf(buffer + n, size - n, "AFE_CONN22		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN22));
	n += scnprintf(buffer + n, size - n, "AFE_CONN23			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN23));
	n += scnprintf(buffer + n, size - n, "AFE_CONN24		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN24));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_RS			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_RS));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_DI		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_DI));
	n += scnprintf(buffer + n, size - n, "AFE_CONN25			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN25));
	n += scnprintf(buffer + n, size - n, "AFE_CONN26		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN26));
	n += scnprintf(buffer + n, size - n, "AFE_CONN27		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN27));
	n += scnprintf(buffer + n, size - n, "AFE_CONN28		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN28));
	n += scnprintf(buffer + n, size - n, "AFE_CONN29			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN29));
	n += scnprintf(buffer + n, size - n, "AFE_CONN30		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN30));
	n += scnprintf(buffer + n, size - n, "AFE_CONN31			   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN31));
	n += scnprintf(buffer + n, size - n, "AFE_CONN32		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN32));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON4		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON5		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON5));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON6		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON6));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON7		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON7));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON8		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON8));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON9		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON9));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON10		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON10));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON11		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON11));
	n += scnprintf(buffer + n, size - n, "PCM_INTF_CON1		   = 0x%x\n",
			Afe_Get_Reg(PCM_INTF_CON1));
	n += scnprintf(buffer + n, size - n, "PCM_INTF_CON2		   = 0x%x\n",
			Afe_Get_Reg(PCM_INTF_CON2));
	n += scnprintf(buffer + n, size - n, "PCM2_INTF_CON		   = 0x%x\n",
			Afe_Get_Reg(PCM2_INTF_CON));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON13		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON13));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON14		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON14));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON15		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON15));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON16		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON16));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON17		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON17));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON18		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON18));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON19		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON19));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON20		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON20));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CON21		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CON21));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_0		   = 0x%x\n",
			Afe_Get_Reg(CLK_AUDDIV_0));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_1		   = 0x%x\n",
			Afe_Get_Reg(CLK_AUDDIV_1));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_2		   = 0x%x\n",
			Afe_Get_Reg(CLK_AUDDIV_2));
	n += scnprintf(buffer + n, size - n, "CLK_AUDDIV_3		   = 0x%x\n",
			Afe_Get_Reg(CLK_AUDDIV_3));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG0		   = 0x%x\n",
			Afe_Get_Reg(FPGA_CFG0));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG1		   = 0x%x\n",
			Afe_Get_Reg(FPGA_CFG1));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG2		   = 0x%x\n",
			Afe_Get_Reg(FPGA_CFG2));
	n += scnprintf(buffer + n, size - n, "FPGA_CFG3		   = 0x%x\n",
			Afe_Get_Reg(FPGA_CFG3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON4		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON5		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON5));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON6		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON6));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON7		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON7));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON8		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON8));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON9		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON9));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON10		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON10));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON11		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON11));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON12		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON12));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON13		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON13));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC4_CON14		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC4_CON14));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_RS1		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_RS1));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_DI1		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_DI1));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_24BIT_1		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_24BIT_1));
	n += scnprintf(buffer + n, size - n, "AFE_CONN_REG		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONN_REG));
	n += scnprintf(buffer + n, size - n, "AFE_CONNSYS_I2S_CON		   = 0x%x\n",
			Afe_Get_Reg(AFE_CONNSYS_I2S_CON));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON0		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON1		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON2		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON2));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON3		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON3));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON4		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON4));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON5		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON5));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON6		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON6));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON7		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON7));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON8		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON8));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON9		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON9));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON10		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON10));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON11		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON11));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON13		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON13));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON14		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON14));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON15		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON15));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON16		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON16));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON17		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON17));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON18		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON18));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON19		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON19));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON20		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON20));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON21		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON21));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON23		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON23));
	n += scnprintf(buffer + n, size - n, "AFE_ASRC_CONNSYS_CON24		   = 0x%x\n",
			Afe_Get_Reg(AFE_ASRC_CONNSYS_CON24));

	/*n += scnprintf(buffer + n, size - n, "AFE_ADDA4_TOP_CON0	   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA4_TOP_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA4_UL_SRC_CON0  = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA4_UL_SRC_CON0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA4_UL_SRC_CON1  = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA4_UL_SRC_CON1));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA4_NEWIF_CFG0   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA4_NEWIF_CFG0));
	n += scnprintf(buffer + n, size - n, "AFE_ADDA4_NEWIF_CFG1   = 0x%x\n",
			Afe_Get_Reg(AFE_ADDA4_NEWIF_CFG1));*/

	n += scnprintf(buffer + n, size - n, "CLK_CFG_4  = 0x%x\n",
			GetClkCfg(AUDIO_CLK_CFG_4));
	n += scnprintf(buffer + n, size - n, "CLK_CFG_6  = 0x%x\n",
			GetClkCfg(AUDIO_CLK_CFG_6));
	n += scnprintf(buffer + n, size - n, "CLK_CFG_8  = 0x%x\n",
			GetClkCfg(AUDIO_CLK_CFG_8));

	n += scnprintf(buffer + n, size - n, "APLL1_CON0  = 0x%x\n",
			GetApmixedCfg(APLL1_CON0));
	n += scnprintf(buffer + n, size - n, "APLL1_CON1  = 0x%x\n",
			GetApmixedCfg(APLL1_CON1));
	n += scnprintf(buffer + n, size - n, "APLL1_CON2  = 0x%x\n",
			GetApmixedCfg(APLL1_CON2));
	n += scnprintf(buffer + n, size - n, "APLL1_CON3  = 0x%x\n",
			GetApmixedCfg(APLL1_CON3));

	n += scnprintf(buffer + n, size - n, "APLL2_CON0  = 0x%x\n",
			GetApmixedCfg(APLL2_CON0));
	n += scnprintf(buffer + n, size - n, "APLL2_CON1  = 0x%x\n",
			GetApmixedCfg(APLL2_CON1));
	n += scnprintf(buffer + n, size - n, "APLL2_CON2  = 0x%x\n",
			GetApmixedCfg(APLL2_CON2));
	n += scnprintf(buffer + n, size - n, "APLL2_CON3  = 0x%x\n",
			GetApmixedCfg(APLL2_CON3));

	n += scnprintf(buffer + n, size - n, "0x1f8  = 0x%x\n",
			Afe_Get_Reg(AFE_BASE + 0x1f8));

	return n;
}

bool SetFmI2sConnection(uint32 ConnectionState)
{
	SetIntfConnection(ConnectionState,
			  Soc_Aud_AFE_IO_Block_I2S_CONNSYS,
			  Soc_Aud_AFE_IO_Block_HW_GAIN1_OUT);
	SetIntfConnection(ConnectionState,
			  Soc_Aud_AFE_IO_Block_HW_GAIN1_IN,
			  Soc_Aud_AFE_IO_Block_I2S1_DAC);
	SetIntfConnection(ConnectionState,
			  Soc_Aud_AFE_IO_Block_HW_GAIN1_IN,
			  Soc_Aud_AFE_IO_Block_I2S1_DAC_2);
	SetIntfConnection(ConnectionState,
			  Soc_Aud_AFE_IO_Block_HW_GAIN1_IN,
			  Soc_Aud_AFE_IO_Block_I2S3);
	return true;
}

bool SetFmAwbConnection(uint32 ConnectionState)
{
	SetIntfConnection(ConnectionState,
			Soc_Aud_AFE_IO_Block_I2S_CONNSYS, Soc_Aud_AFE_IO_Block_MEM_AWB);
	return true;
}

int SetFmI2sInEnable(bool enable)
{
	return setConnsysI2SInEnable(enable);
}

int SetFmI2sIn(AudioDigtalI2S *mDigitalI2S)
{
	return setConnsysI2SIn(mDigitalI2S);
}

bool GetFmI2sInPathEnable(void)
{
	return GetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_CONNSYS);
}

bool SetFmI2sInPathEnable(bool bEnable)
{
	return SetMemoryPathEnable(Soc_Aud_Digital_Block_I2S_IN_CONNSYS, bEnable);
}

int SetFmI2sAsrcEnable(bool enable)
{
	return setConnsysI2SEnable(enable);
}

int SetFmI2sAsrcConfig(bool bIsUseASRC, unsigned int dToSampleRate)
{
	return setConnsysI2SAsrc(bIsUseASRC, dToSampleRate);
}

bool SetAncRecordReg(uint32 value, uint32 mask)
{
	return false;
}

const struct Aud_IRQ_CTRL_REG *GetIRQCtrlReg(enum Soc_Aud_IRQ_MCU_MODE irqIndex)
{
	return &mIRQCtrlRegs[irqIndex];
}

const struct Aud_RegBitsInfo *GetIRQPurposeReg(enum Soc_Aud_IRQ_PURPOSE irqPurpose)
{
	return &mIRQPurposeRegs[irqPurpose];
}

/*Irq handler function array*/
static void Aud_IRQ1_Handler(void)
{
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1) &&
	    is_irq_from_ext_module() == false)
		Auddrv_DL1_Interrupt_Handler();
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL2))
		Auddrv_DL2_Interrupt_Handler();
}
static void Aud_IRQ2_Handler(void)
{
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_VUL))
		Auddrv_UL1_Interrupt_Handler();
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_AWB))
		Auddrv_AWB_Interrupt_Handler();
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DAI))
		Auddrv_DAI_Interrupt_Handler();
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_VUL_DATA2))
		Auddrv_UL2_Interrupt_Handler();
	if (GetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_MOD_DAI))
		Auddrv_MOD_DAI_Interrupt_Handler();
}

static void (*Aud_IRQ_Handler_Funcs[Soc_Aud_IRQ_MCU_MODE_NUM])(void) = {
	NULL,
	Aud_IRQ1_Handler,
	Aud_IRQ2_Handler,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* Reserved */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

void RunIRQHandler(enum Soc_Aud_IRQ_MCU_MODE irqIndex)
{
	if (Aud_IRQ_Handler_Funcs[irqIndex] != NULL)
		Aud_IRQ_Handler_Funcs[irqIndex]();
	else
		pr_aud("%s(), Aud_IRQ%d_Handler is Null", __func__, irqIndex);
}

bool IsNeedToSetHighAddr(bool usingdram, dma_addr_t addr)
{
	return (enable_4G() & usingdram & (0 < (addr >> 32)));
}

bool SetHighAddr(Soc_Aud_Digital_Block MemBlock, bool usingdram, dma_addr_t addr)
{
	bool highBitEnable = IsNeedToSetHighAddr(usingdram, addr);

	/*pr_debug("%s MemBlock = %d usingdram = %d\n",
		 __func__,
		 MemBlock,
		 usingdram);*/

	switch (MemBlock) {
	case Soc_Aud_Digital_Block_MEM_DL1:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 0, 0x1 << 0);
		break;
	case Soc_Aud_Digital_Block_MEM_DL2:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 1, 0x1 << 1);
		break;
	case Soc_Aud_Digital_Block_MEM_VUL:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 6, 0x1 << 6);
		break;
	case Soc_Aud_Digital_Block_MEM_DAI:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 5, 0x1 << 5);
		break;
	case Soc_Aud_Digital_Block_MEM_AWB:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 3, 0x1 << 3);
		break;
	case Soc_Aud_Digital_Block_MEM_MOD_DAI:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 4, 0x1 << 4);
		break;
	case Soc_Aud_Digital_Block_MEM_DL1_DATA2:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 2, 0x1 << 2);
		break;
	case Soc_Aud_Digital_Block_MEM_VUL_DATA2:
		Afe_Set_Reg(AFE_MEMIF_MSB, highBitEnable << 7, 0x1 << 7);
		break;
	default:
		break;
	}
	return true;
}

