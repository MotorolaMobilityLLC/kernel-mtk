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
 *   mtk_codec_speaker_63xx
 *
 * Project:
 * --------
 *
 *
 * Description:
 * ------------
 *   Audio codec stub file
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

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_analog_type.h"
#if 0
#include <mach/pmic_mt6323_sw.h>	/* no this */
#endif

#include "mt_soc_pcm_common.h"

/* static DEFINE_MUTEX(Speaker_Ctrl_Mutex); */
/* static DEFINE_SPINLOCK(Speaker_lock); */
/* static int Speaker_Counter = 0; */
/* static bool  Speaker_Trim_init = false; */

void Speaker_ClassD_Open(void)
{
	pr_debug("%s\n", __func__);
	Ana_Set_Reg(SPK_CON2, 0x0214, 0xffff);	/* enable classAB OC function */
	Ana_Set_Reg(SPK_CON9, 0x0400, 0xffff);	/* Set Spk 6dB gain */

	/* enable SPK-Amp with 0dB gain, enable SPK amp offset triming, select class D mode */
	Ana_Set_Reg(SPK_CON0, 0x3008, 0xffff);

	Ana_Set_Reg(SPK_CON0, 0x3009, 0xffff);	/* Enable Class ABD */
	mdelay(5);
	Ana_Set_Reg(SPK_CON0, 0x3001, 0xffff);	/* enable SPK AMP with 0dB gain, select Class D. enable Amp. */
}

void Speaker_ClassD_close(void)
{
	pr_debug("%s\n", __func__);
	/* Mute Spk amp, select to original class AB mode. disable class-D Amp */
	Ana_Set_Reg(SPK_CON0, 0x0004, 0xffff);
}


void Speaker_ClassAB_Open(void)
{
	pr_debug("%s\n", __func__);
	Ana_Set_Reg(SPK_CON2, 0x0214, 0xffff);	/* enable classAB OC function */
	Ana_Set_Reg(SPK_CON9, 0x0400, 0xffff);	/* Set Spk 6dB gain */

	/* enable SPK-Amp with 0dB gain, enable SPK amp offset triming, select class D mode */
	Ana_Set_Reg(SPK_CON0, 0x3008, 0xffff);

	Ana_Set_Reg(SPK_CON0, 0x3009, 0xffff);	/* Enable Class ABD */
	mdelay(5);
	Ana_Set_Reg(SPK_CON0, 0x3000, 0xffff);	/* disable amp before switch to ClassAB */
	Ana_Set_Reg(SPK_CON0, 0x3005, 0xffff);	/* enable SPK AMP with 0dB gain, select Class AB. enable Amp. */
}

void Speaker_ClassAB_close(void)
{
	pr_debug("%s\n", __func__);
	/* Mute Spk amp, select to original class D mode. disable class-AB Amp */
	Ana_Set_Reg(SPK_CON0, 0x0000, 0xffff);
}

void Speaker_ReveiverMode_Open(void)
{
	pr_debug("%s\n", __func__);
	/* enable classAB OC function, enable speaker L receiver mode[6] */
	Ana_Set_Reg(SPK_CON2, 0x0614, 0xffff);

	Ana_Set_Reg(SPK_CON9, 0x0100, 0xffff);
	/* Ana_Set_Reg(SPK_CON9, 0x0400, 0xffff); // Set Spk 6dB gain */

	/* enable SPK AMP with -6dB gain for 2in1 speaker, select Class AB. enable Amp. */
	Ana_Set_Reg(SPK_CON0, 0x1005, 0xffff);
}

void Speaker_ReveiverMode_close(void)
{
	/* Mute Spk amp, select to original class D mode. disable class-AB Amp */
	Ana_Set_Reg(SPK_CON0, 0x0000, 0xffff);
}

bool GetSpeakerOcFlag(void)
{
	unsigned int OCregister = 0;
	unsigned int bitmask = 1;
	bool DmodeFlag = false;
	bool ABmodeFlag = false;
#if 0				/* no this */
	Ana_Set_Reg(TOP_CKPDN_CON2_CLR, 0x3, 0xffff);
#endif
	OCregister = Ana_Get_Reg(SPK_CON6);
	DmodeFlag = OCregister & (bitmask << 14);	/* no.14 bit is SPK_D_OC_L_DEG */
	ABmodeFlag = OCregister & (bitmask << 15);	/* no.15 bit is SPK_AB_OC_L_DEG */
	pr_debug("OCregister = %d\n", OCregister);
	return (DmodeFlag | ABmodeFlag);
}
