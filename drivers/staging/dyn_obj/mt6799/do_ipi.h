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

#include "scp_helper.h"
#include "scp_ipi.h"
#include "scp_excep.h"

#define IPI_BUFF_SIZE           SHARE_BUF_SIZE	/* max: 288 */
#define IPI_WAIT_INTERVAL_MS    1
#define IPI_MAX_RETRY_COUNT     500

void do_ipi_handler(int id, void *data, unsigned int len);
int do_ipi_send(u32 type, void *buf, unsigned int bufsize, scp_core_id id, int wait);
int do_ipi_send_dram_addr(u32 addr, scp_core_id scp, int in_isr);
int do_ipi_send_do_name(char *name, int load, scp_core_id scp);
