#ifndef AUDIO_MESSENGER_IPI_H
#define AUDIO_MESSENGER_IPI_H

#include "audio_type.h"

#define IPI_MSG_HEADER_SIZE      (16)
#define MAX_IPI_MSG_PAYLOAD_SIZE (32)
#define MAX_IPI_MSG_BUF_SIZE     (IPI_MSG_HEADER_SIZE + MAX_IPI_MSG_PAYLOAD_SIZE)

#define IPI_MSG_MAGIC_NUMBER     (0x8888)


typedef enum {
	TASK_SCENE_PHONE_CALL,
	TASK_SCENE_VOW,
	TASK_SCENE_PLAYBACK_MP3,
#if 0
	TASK_SCENE_RECORD,
	TASK_SCENE_VOIP,
	TASK_SCENE_SPEAKER_PROTECTION,
#endif
	TASK_SCENE_SIZE /* the size of tasks */
} task_scene_t;


typedef enum {
	AUDIO_IPI_LAYER_HAL,     /* HAL    <-> SCP */
	AUDIO_IPI_LAYER_KERNEL,  /* kernel <-> SCP */
	AUDIO_IPI_LAYER_MODEM,   /* MODEM  <-> SCP */
} audio_ipi_msg_layer_t;


typedef enum {
	AUDIO_IPI_MSG_ONLY, /* param1: defined by user,       param2: defined by user */
	AUDIO_IPI_PAYLOAD,  /* param1: payload length (<=32), param2: defined by user */
	AUDIO_IPI_DMA,      /* param1: dma data length,       param2: defined by user */
} audio_ipi_msg_data_t;


typedef enum {
	AUDIO_IPI_MSG_BYPASS_ACK = 0,
	AUDIO_IPI_MSG_NEED_ACK   = 1,
	AUDIO_IPI_MSG_ACK_BACK   = 2,
	AUDIO_IPI_MSG_CANCELED   = 8
} audio_ipi_msg_ack_t;


typedef struct ipi_msg_t {
	uint16_t magic;      /* IPI_MSG_MAGIC_NUMBER */
	uint8_t  task_scene; /* see task_scene_t */
	uint8_t  msg_layer;  /* see audio_ipi_msg_layer_t */
	uint8_t  data_type;  /* see audio_ipi_msg_data_t */
	uint8_t  ack_type;   /* see audio_ipi_msg_ack_t */
	uint16_t msg_id;     /* defined by user */
	uint32_t param1;     /* see audio_ipi_msg_data_t */
	uint32_t param2;     /* see audio_ipi_msg_data_t */
	union {
		char payload[MAX_IPI_MSG_PAYLOAD_SIZE];
		char *dma_addr;  /* for AUDIO_IPI_DMA only */
	};
} ipi_msg_t;


typedef void (*recv_message_t)(struct ipi_msg_t *ipi_msg);

/*==============================================================================
 *                     public functions - declaration
 *============================================================================*/

void audio_messenger_ipi_init(void);

void audio_reg_recv_message(uint8_t task_scene, recv_message_t recv_message);

uint16_t get_message_buf_size(const ipi_msg_t *ipi_msg);

void dump_msg(const ipi_msg_t *ipi_msg);

void check_msg_format(const ipi_msg_t *ipi_msg, unsigned int len);


void audio_send_ipi_msg(
	task_scene_t task_scene,
	audio_ipi_msg_data_t data_type,
	audio_ipi_msg_ack_t ack_type,
	uint16_t msg_id,
	uint32_t param1,
	uint32_t param2,
	char    *payload);

void audio_send_ipi_msg_to_scp(const ipi_msg_t *ipi_msg);

void audio_send_ipi_msg_ack_back(const ipi_msg_t *ipi_msg);

#endif /* end of AUDIO_MESSENGER_IPI_H */

