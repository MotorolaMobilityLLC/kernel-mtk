/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include "audio_ipi_message.h"

#include "audio_assert.h"
#include "audio_log.h"


/*
 * =============================================================================
 *                     MACRO
 * =============================================================================
 */

/* DEBUG FLAGS */
/*#define ENABLE_DUMP_IPI_MSG*/


/*
 * =============================================================================
 *                     typedef
 * =============================================================================
 */


/*
 * =============================================================================
 *                     private global members
 * =============================================================================
 */


/*
 * =============================================================================
 *                     private function declaration
 * =============================================================================
 */


/*
 * =============================================================================
 *                     utilities
 * =============================================================================
 */


/*
 * =============================================================================
 *                     main function implementation
 * =============================================================================
 */

uint16_t get_message_buf_size(const ipi_msg_t *p_ipi_msg)
{
	if (p_ipi_msg->data_type == AUDIO_IPI_MSG_ONLY)
		return IPI_MSG_HEADER_SIZE;
	else if (p_ipi_msg->data_type == AUDIO_IPI_PAYLOAD)
		return (IPI_MSG_HEADER_SIZE + p_ipi_msg->param1);
	else if (p_ipi_msg->data_type == AUDIO_IPI_DMA)
		return (IPI_MSG_HEADER_SIZE + 8); /* 64-bits addr */
	else
		return 0;
}


void dump_msg(const ipi_msg_t *p_ipi_msg)
{
#ifdef ENABLE_DUMP_IPI_MSG
	int i = 0;
	int payload_size = 0;

	AUD_LOG_D("%s(), sizeof(ipi_msg_t) = %lu\n", __func__, sizeof(ipi_msg_t));

	AUD_LOG_D("%s(), magic = 0x%x\n", __func__, p_ipi_msg->magic);
	AUD_LOG_D("%s(), task_scene = 0x%x\n", __func__, p_ipi_msg->task_scene);
	AUD_LOG_D("%s(), msg_layer = 0x%x\n", __func__, p_ipi_msg->msg_layer);
	AUD_LOG_D("%s(), data_type = 0x%x\n", __func__, p_ipi_msg->data_type);
	AUD_LOG_D("%s(), ack_type = 0x%x\n", __func__, p_ipi_msg->ack_type);
	AUD_LOG_D("%s(), msg_id = 0x%x\n", __func__, p_ipi_msg->msg_id);
	AUD_LOG_D("%s(), param1 = 0x%x\n", __func__, p_ipi_msg->param1);
	AUD_LOG_D("%s(), param2 = 0x%x\n", __func__, p_ipi_msg->param2);

	if (p_ipi_msg->data_type == AUDIO_IPI_PAYLOAD) {
		payload_size = p_ipi_msg->param1;
		for (i = 0; i < payload_size; i++)
			AUD_LOG_D("%s(), payload[%d] = 0x%x\n", __func__, i, p_ipi_msg->payload[i]);
	} else if (p_ipi_msg->data_type == AUDIO_IPI_DMA)
		AUD_LOG_D("%s(), dma_addr = %p\n", __func__, p_ipi_msg->dma_addr);
#endif
}


void check_msg_format(const ipi_msg_t *p_ipi_msg, unsigned int len)
{
	dump_msg(p_ipi_msg); /* TODO: remove it later */

	AUD_ASSERT(p_ipi_msg->magic == IPI_MSG_MAGIC_NUMBER);
	AUD_ASSERT(get_message_buf_size(p_ipi_msg) == len);
}


void print_msg_info(
	const char *func_name,
	const char *description,
	const ipi_msg_t *p_ipi_msg)
{
	/* error handling */
	if (func_name == NULL || description == NULL || p_ipi_msg == NULL)
		return;

	AUD_LOG_D("%s(), %s, task: %d, id: 0x%x, ack: %d, p1: 0x%x, p2: 0x%x\n",
		  func_name,
		  description,
		  p_ipi_msg->task_scene,
		  p_ipi_msg->msg_id,
		  p_ipi_msg->ack_type,
		  p_ipi_msg->param1,
		  p_ipi_msg->param2);
}


/*
 * =============================================================================
 *                     utilities implementation
 * =============================================================================
 */



