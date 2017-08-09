#include "audio_messenger_ipi.h"

#include <linux/spinlock.h>
#include <scp_ipi.h>

#include "audio_log.h"
#include "audio_assert.h"



/*==============================================================================
 *                     MACRO
 *============================================================================*/

#define MAX_IPI_MSG_QUEUE_SIZE (8)

/* DEBUG FLAGS */
/*#define ENABLE_DUMP_IPI_MSG*/

/*==============================================================================
 *                     private global members
 *============================================================================*/

static DEFINE_SPINLOCK(audio_ipi_queue_lock);

static ipi_msg_t gIpiMsgQueue[MAX_IPI_MSG_QUEUE_SIZE];
/*static uint16_t  gIpiMsgSize = 0; TODO: Queue*/

recv_message_t recv_message_array[TASK_SCENE_SIZE];


/*==============================================================================
 *                     private functions - declaration
 *============================================================================*/

/* queue related */
static ipi_msg_t *getIpiMsg(task_scene_t task_scene);

static void audio_ipi_msg_dispatcher(int id, void *data, unsigned int len);
static uint16_t current_idx;


/*==============================================================================
 *                     private functions - implementation
 *============================================================================*/

static ipi_msg_t *getIpiMsg(task_scene_t task_scene)
{
	uint16_t msg_idx = 0xFFFF;

	unsigned long flags = 0;

	spin_lock_irqsave(&audio_ipi_queue_lock, flags);

#if 0
	gIpiMsgSize++;
	if (gIpiMsgSize >= MAX_IPI_MSG_QUEUE_SIZE) {
		AUD_LOG_E("queue size errors\n");
		return 0xFFFF;
	}
#endif

	msg_idx = current_idx;
	current_idx++;
	if (current_idx == MAX_IPI_MSG_QUEUE_SIZE)
		current_idx = 0;

	spin_unlock_irqrestore(&audio_ipi_queue_lock, flags);

	if (msg_idx >= MAX_IPI_MSG_QUEUE_SIZE) {
		AUD_LOG_E("%s(), idx(%d) > MAX_IPI_MSG_QUEUE_SIZE\n", __func__, msg_idx);
		return NULL;
	}

	return &gIpiMsgQueue[msg_idx];
}


uint16_t get_message_buf_size(const ipi_msg_t *ipi_msg)
{
	if (ipi_msg->data_type == AUDIO_IPI_MSG_ONLY)
		return IPI_MSG_HEADER_SIZE;
	else if (ipi_msg->data_type == AUDIO_IPI_PAYLOAD)
		return (IPI_MSG_HEADER_SIZE + ipi_msg->param1);
	else if (ipi_msg->data_type == AUDIO_IPI_DMA)
		return (IPI_MSG_HEADER_SIZE + 8); /* 64-bits addr */
	else
		return 0;
}


void dump_msg(const ipi_msg_t *ipi_msg)
{
#ifdef ENABLE_DUMP_IPI_MSG
	int i = 0;
	int payload_size = 0;

	AUD_LOG_D("%s(), sizeof(ipi_msg_t) = %lu\n", __func__, sizeof(ipi_msg_t));

	AUD_LOG_D("%s(), magic = 0x%x\n", __func__, ipi_msg->magic);
	AUD_LOG_D("%s(), task_scene = 0x%x\n", __func__, ipi_msg->task_scene);
	AUD_LOG_D("%s(), msg_layer = 0x%x\n", __func__, ipi_msg->msg_layer);
	AUD_LOG_D("%s(), data_type = 0x%x\n", __func__, ipi_msg->data_type);
	AUD_LOG_D("%s(), ack_type = 0x%x\n", __func__, ipi_msg->ack_type);
	AUD_LOG_D("%s(), msg_id = 0x%x\n", __func__, ipi_msg->msg_id);
	AUD_LOG_D("%s(), param1 = 0x%x\n", __func__, ipi_msg->param1);
	AUD_LOG_D("%s(), param2 = 0x%x\n", __func__, ipi_msg->param2);

	if (ipi_msg->data_type == AUDIO_IPI_PAYLOAD) {
		payload_size = ipi_msg->param1;
		for (i = 0; i < payload_size; i++)
			AUD_LOG_D("%s(), payload[%d] = 0x%x\n", __func__, i, ipi_msg->payload[i]);
	} else if (ipi_msg->data_type == AUDIO_IPI_DMA)
		AUD_LOG_D("%s(), dma_addr = %p\n", __func__, ipi_msg->dma_addr);
#endif
}


void check_msg_format(const ipi_msg_t *ipi_msg, unsigned int len)
{
	dump_msg(ipi_msg); /* TODO: remove it later */

	AUD_ASSERT(ipi_msg->magic == IPI_MSG_MAGIC_NUMBER);
	AUD_ASSERT(get_message_buf_size(ipi_msg) == len);
}


static void audio_ipi_msg_dispatcher(int id, void *data, unsigned int len)
{
	ipi_msg_t *ipi_msg = NULL;

	AUD_LOG_V("%s(), data = %p, len = %u\n", __func__, data, len);

	if (data == NULL) {
		AUD_LOG_W("%s(), drop msg due to data = NULL\n", __func__);
		return;
	}
	if (len < IPI_MSG_HEADER_SIZE || len > MAX_IPI_MSG_BUF_SIZE) {
		AUD_LOG_W("%s(), drop msg due to len(%u) error!!\n", __func__, len);
		return;
	}

	ipi_msg = (ipi_msg_t *)data;
	check_msg_format(ipi_msg, len);

	/* TODO: kernel or HAL */
	if (recv_message_array[ipi_msg->task_scene] == NULL) {
		AUD_LOG_W("%s(), recv_message_array[%d] = NULL, drop msg. msg_id = 0x%x\n",
			  __func__, ipi_msg->task_scene, ipi_msg->msg_id);
		return;
	}

	recv_message_array[ipi_msg->task_scene](ipi_msg);
}


/*==============================================================================
 *                     public functions - implementation
 *============================================================================*/

void audio_messenger_ipi_init(void)
{
	int i = 0;
	ipi_status retval = ERROR;

	current_idx = 0;

	retval = scp_ipi_registration(IPI_AUDIO, audio_ipi_msg_dispatcher, "audio");
	if (retval != DONE)
		AUD_LOG_E("%s(), scp_ipi_registration fail!!\n", __func__);

	for (i = 0; i < TASK_SCENE_SIZE; i++)
		recv_message_array[i] = NULL;
}


void audio_reg_recv_message(uint8_t task_scene, recv_message_t recv_message)
{
	if (task_scene >= TASK_SCENE_SIZE)
		AUD_LOG_W("%s(), not support task_scene %d!!\n", __func__, task_scene);

	recv_message_array[task_scene] = recv_message;
}


void audio_send_ipi_msg(
	task_scene_t task_scene,
	audio_ipi_msg_data_t data_type,
	audio_ipi_msg_ack_t ack_type,
	uint16_t msg_id,
	uint32_t param1,
	uint32_t param2,
	char    *payload)
{
	ipi_msg_t *ipi_msg = NULL;
	uint32_t ipi_msg_len = 0;

	AUD_LOG_D("send, task: %d, id: 0x%x, p1: 0x%x, p2: 0x%x\n",
		  ipi_msg->task_scene, ipi_msg->msg_id, ipi_msg->param1, ipi_msg->param2);

	ipi_msg = getIpiMsg(task_scene);
	if (ipi_msg == NULL) {
		AUD_LOG_E("%s(), ipi_msg = NULL, return\n", __func__);
		return;
	}

	memset_io(ipi_msg, 0, MAX_IPI_MSG_BUF_SIZE);

	ipi_msg->magic      = IPI_MSG_MAGIC_NUMBER;
	ipi_msg->task_scene = task_scene;
	ipi_msg->msg_layer  = AUDIO_IPI_LAYER_KERNEL;
	ipi_msg->data_type  = data_type;
	ipi_msg->ack_type   = ack_type;
	ipi_msg->msg_id     = msg_id;
	ipi_msg->param1     = param1;
	ipi_msg->param2     = param2;

	if (ipi_msg->data_type == AUDIO_IPI_PAYLOAD) {
		AUD_ASSERT(payload != NULL);

		if (ipi_msg->param1 > MAX_IPI_MSG_PAYLOAD_SIZE) {
			AUD_LOG_E("%s(), payload size(%d) > MAX_IPI_MSG_PAYLOAD_SIZE(%d)\n",
				  __func__, ipi_msg->param1, MAX_IPI_MSG_PAYLOAD_SIZE);
			ipi_msg->param1 = MAX_IPI_MSG_PAYLOAD_SIZE;
		}
		memcpy(ipi_msg->payload, payload, ipi_msg->param1);
	} else if (ipi_msg->data_type == AUDIO_IPI_DMA) {
		AUD_ASSERT(payload != NULL);
		ipi_msg->dma_addr = payload;
	}

	ipi_msg_len = get_message_buf_size(ipi_msg);
	check_msg_format(ipi_msg, ipi_msg_len);

	audio_send_ipi_msg_to_scp(ipi_msg);
}


void audio_send_ipi_msg_to_scp(const ipi_msg_t *ipi_msg)
{
	ipi_status send_status = ERROR;

	AUD_LOG_D("send, task: %d, id: 0x%x, p1: 0x%x, p2: 0x%x\n",
		  ipi_msg->task_scene, ipi_msg->msg_id, ipi_msg->param1, ipi_msg->param2);

	send_status = scp_ipi_send(
			      IPI_AUDIO,
			      (void *)ipi_msg,
			      get_message_buf_size(ipi_msg),
			      1);  /* default wait */

	if (send_status != DONE) {
		AUD_LOG_E("%s(), scp_ipi_send error\n", __func__);
		return;
	}
}


void audio_send_ipi_msg_ack_back(const ipi_msg_t *ipi_msg)
{
	if (ipi_msg->ack_type == AUDIO_IPI_MSG_NEED_ACK) {
		AUD_LOG_V("%s(), msg_id = 0x%x\n", __func__, ipi_msg->msg_id);
		audio_send_ipi_msg(ipi_msg->task_scene,
				   ipi_msg->data_type, AUDIO_IPI_MSG_ACK_BACK,
				   ipi_msg->msg_id, ipi_msg->param1, ipi_msg->param2,
				   ipi_msg->dma_addr);
	}
}

