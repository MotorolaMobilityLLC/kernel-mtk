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
 *  mt_sco_afe_connection.c
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
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/div64.h>

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

/* mutex lock */
static DEFINE_MUTEX(afe_connection_mutex);

/**
* connection of register
*/
const uint32 mConnectionReg[Soc_Aud_InterConnectionOutput_Num_Output] = {
	AFE_CONN0, AFE_CONN1, AFE_CONN2, AFE_CONN3, AFE_CONN4,
	AFE_CONN5, AFE_CONN6, AFE_CONN7, AFE_CONN8, AFE_CONN9,
	AFE_CONN10, AFE_CONN11, AFE_CONN12, AFE_CONN13, AFE_CONN14,
	AFE_CONN15, AFE_CONN16, AFE_CONN17, AFE_CONN18, AFE_CONN19,
	AFE_CONN20, AFE_CONN21, AFE_CONN22, AFE_CONN23, AFE_CONN24,
	AFE_CONN25, AFE_CONN26, AFE_CONN27, AFE_CONN28, AFE_CONN29,
	};

/**
* connection state of register
*/
static char mConnectionState[Soc_Aud_InterConnectionInput_Num_Input]
	[Soc_Aud_InterConnectionOutput_Num_Output] = { {0} };

static bool CheckBitsandReg(short regaddr, char bits)
{
	if (regaddr <= 0 || bits < 0) {
		pr_debug("regaddr = %x bits = %d\n", regaddr, bits);
		return false;
	}
	return true;
}

bool SetConnectionState(uint32 ConnectionState, uint32 Input, uint32 Output)
{
	/* printk("SetinputConnection ConnectionState = %d
	Input = %d Output = %d\n", ConnectionState, Input, Output); */
	int connectReg = 0;

	switch (ConnectionState) {
	case Soc_Aud_InterCon_DisConnect:
	{
		/* printk("nConnectionState = %d\n", ConnectionState); */
		if ((mConnectionState[Input][Output] & Soc_Aud_InterCon_Connection)
			== Soc_Aud_InterCon_Connection) {

			/* here to disconnect connect bits */
			connectReg = mConnectionReg[Output];
			if (CheckBitsandReg(connectReg, Input)) {
				Afe_Set_Reg(connectReg, 0, 1 << Input);
				mConnectionState[Input][Output] &= ~(Soc_Aud_InterCon_Connection);
			}
		}
		if ((mConnectionState[Input][Output] & Soc_Aud_InterCon_ConnectionShift)
			== Soc_Aud_InterCon_ConnectionShift) {

			/* here to disconnect connect shift bits */
			if (CheckBitsandReg(AFE_CONN_RS, Input)) {
				Afe_Set_Reg(AFE_CONN_RS, 0, 1 << Input);
				mConnectionState[Input][Output] &= ~(Soc_Aud_InterCon_ConnectionShift);
			}
		}
		break;
	}
	case Soc_Aud_InterCon_Connection:
	{
		/* printk("nConnectionState = %d\n", ConnectionState); */
		connectReg = mConnectionReg[Output];
		if (CheckBitsandReg(connectReg, Input)) {
			Afe_Set_Reg(connectReg, 1 << Input, 1 << Input);
			mConnectionState[Input][Output] |= Soc_Aud_InterCon_Connection;
		}
		break;
	}
	case Soc_Aud_InterCon_ConnectionShift:
	{
		/* printk("nConnectionState = %d\n", ConnectionState); */
		if (CheckBitsandReg(AFE_CONN_RS, Input)) {
			Afe_Set_Reg(AFE_CONN_RS, 1 << Input, 1 << Input);
			mConnectionState[Input][Output] |= Soc_Aud_InterCon_ConnectionShift;
		}
		break;
	}
	default:
		pr_err("no this state ConnectionState = %d\n", ConnectionState);
		break;
	}

	return true;
}
EXPORT_SYMBOL(SetConnectionState);
