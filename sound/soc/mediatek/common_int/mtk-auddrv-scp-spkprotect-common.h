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

/******************************************************************************
*
 *
 * Filename:
 * ---------
 *   mtk-auddrv_scp_spkprotect_common.h
 *
 * Project:
 * --------
 *   None
 *
 * Description:
 * ------------
 *   Audio Spk Protect Kernel Definitions
 *
 * Author:
 * -------
 *   Chipeng Chang
 *
 *---------------------------------------------------------------------------
---
 *

*******************************************************************************/


#ifndef AUDIO_SPKPROCT_COMMON_H
#define AUDIO_SPKPROCT_COMMON_H

#include <linux/kernel.h>
#include "mtk-auddrv-common.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-kernel.h"
#include "mtk-soc-afe-control.h"


#ifdef CONFIG_MTK_AUDIO_SCP_SPKPROTECT_SUPPORT
#include <audio_task_manager.h>
#include <audio_ipi_client_spkprotect.h>
#include <audio_dma_buf_control.h>
#endif


enum { /* IPI_RECEIVED_SPK_PROTECTION */
	SPK_PROTECT_OPEN = 0x1,
	SPK_PROTECT_CLOSE,
	SPK_PROTECT_PREPARE,
	SPK_PROTECT_PLATMEMPARAM,
	SPK_PROTECT_DLMEMPARAM,
	SPK_PROTECT_IVMEMPARAM,
	SPK_PROTECT_HWPARAM,
	SPK_PROTECT_DLCOPY,
	SPK_PROTECT_START,
	SPK_PROTECT_STOP,
	SPK_PROTECT_SETPRAM,
	SPK_PROTECT_NEEDDATA,
	SPK_PROTTCT_PCMDUMP_ON,
	SPK_PROTTCT_PCMDUMP_OFF,
	SPK_PROTECT_PCMDUMP_OK,
	SPK_PROTECT_IRQDL = 0x100,
	SPK_PROTECT_IRQUL,
};

struct Aud_Spk_Message_t {
	uint16_t msg_id;
	uint32_t param1;
	uint32_t param2;
	char *payload;
};


void spkprocservice_ipicmd_received(struct ipi_msg_t *ipi_msg);
void spkprocservice_ipicmd_send(uint8_t data_type,
						uint8_t ack_type, uint16_t msg_id, uint32_t param1,
						uint32_t param2, char *payload);


#endif
