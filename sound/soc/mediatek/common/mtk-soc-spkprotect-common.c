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
 *   mt_soc_spkprotect_common.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio playback
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

#include <linux/compat.h>
#include <linux/wakelock.h>
#include "mtk-auddrv-scp-spkprotect-common.h"

#ifdef CONFIG_MTK_AUDIO_SCP_SPKPROTECT_SUPPORT
#include "audio_ipi_client_spkprotect.h"
#include <audio_task_manager.h>
#include <audio_ipi_client_spkprotect.h>
#include <audio_dma_buf_control.h>
#endif

static Aud_Spk_Message_t mAud_Spk_Message;

void spkprocservice_ipicmd_received(ipi_msg_t *ipi_msg)
{
	switch (ipi_msg->msg_id) {
	case SPK_PROTECT_IRQDL:
		mAud_Spk_Message.msg_id = ipi_msg->msg_id;
		mAud_Spk_Message.param1 = ipi_msg->param1;
		mAud_Spk_Message.param2 = ipi_msg->param2;
		mAud_Spk_Message.payload = ipi_msg->payload;
		AudDrv_DSP_IRQ_handler((void *)&mAud_Spk_Message);
		break;
	case SPK_PROTECT_NEEDDATA:
		break;
	default:
		break;
	}
}

void spkprocservice_ipicmd_send(audio_ipi_msg_data_t data_type,
						audio_ipi_msg_ack_t ack_type, uint16_t msg_id, uint32_t param1,
						uint32_t param2, char *payload)
{
	ipi_msg_t ipi_msg;

	memset((void *)&ipi_msg, 0, sizeof(ipi_msg_t));
	audio_send_ipi_msg(&ipi_msg, TASK_SCENE_SPEAKER_PROTECTION, AUDIO_IPI_LAYER_KERNEL_TO_SCP,
		data_type, ack_type, msg_id, param1, param2, (char *)payload);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek SpkPotect");
