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

#ifndef __CCU_MAILBOX_H__
#define __CCU_MAILBOX_H__

#include "ccu_ext_interface/ccu_mailbox_extif.h"

typedef enum {
	MAILBOX_OK = 0,
	MAILBOX_QUEUE_FULL,
	MAILBOX_QUEUE_EMPTY
} mb_result;

mb_result mailbox_init(struct ccu_mailbox_s *apmcu_mb_addr,
		       struct ccu_mailbox_s *ccu_mb_addr);

mb_result mailbox_send_cmd(ccu_msg_t *task);

mb_result mailbox_receive_cmd(ccu_msg_t *task);

#endif
