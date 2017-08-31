#ifndef __CCU_MAILBOX_H__
#define __CCU_MAILBOX_H__

#include "ccu_ext_interface/ccu_mailbox_extif.h"

typedef enum {
		MAILBOX_OK = 0,
		MAILBOX_QUEUE_FULL,
		MAILBOX_QUEUE_EMPTY
} mb_result;

mb_result mailbox_init(ccu_mailbox_t *apmcu_mb_addr, ccu_mailbox_t *ccu_mb_addr);

mb_result mailbox_send_cmd(ccu_msg_t *task);

mb_result mailbox_receive_cmd(ccu_msg_t *task);

#endif
