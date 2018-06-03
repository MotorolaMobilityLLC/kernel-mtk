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

#ifndef __PORT_SMEM_H__
#define __PORT_SMEM_H__

#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <mt-plat/mtk_ccci_common.h>
#include "ccci_config.h"
#include "ccci_core.h"

enum {
	TYPE_RAW = 0,
	TYPE_CCB,
};

enum {
	CCB_USER_INVALID = 0,
	CCB_USER_OK,
	CCB_USER_ERR,
};

struct tx_notify_task {
	struct hrtimer notify_timer;
	unsigned short core_id;
	struct ccci_port *port;
};

struct ccci_ccb_ctrl {
	unsigned char *ctrl_addr_phy;
	unsigned char *ctrl_addr_vir;
};

struct ccci_smem_port {
	SMEM_USER_ID user_id;
	unsigned char type;

	unsigned char state;
	unsigned int wakeup;
	unsigned char wk_cnt;
	phys_addr_t addr_phy;
	unsigned char *addr_vir;
	unsigned int length;
	struct ccci_port *port;
	wait_queue_head_t rx_wq;
	int ccb_ctrl_offset;

	spinlock_t write_lock;
};
#endif
