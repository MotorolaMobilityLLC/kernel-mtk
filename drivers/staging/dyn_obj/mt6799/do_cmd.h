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

#include<linux/types.h>
#include "do_part_header.h"

enum DO_MSG_TYPE {
	DO_MSG_INFO = 1,
	DO_MSG_GET_INFO,
	DO_MSG_LOAD_MODULE,
	DO_MSG_UNLOAD_MODULE,
	DO_MSG_UPDATE_CURRENT_DO,
	DO_MSG_DRAM_ADDR,
	DO_MSG_DO_DISABLED = 99
};

struct do_msg {
	u32 scp_num;
	u32 type;
	u32 len;
	char payload;
};

struct do_header {
	char id[DO_ID_LENGTH];
	u32 dram_addr;
	u32 sram_addr;
	u32 size;
	u32 featlist_len;
};

struct do_info {
	u32 num_of_do;
	struct do_header headers;
};
