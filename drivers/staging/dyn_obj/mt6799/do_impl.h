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

#include <linux/printk.h>
#include "do_cmd.h"
#include "do.h"

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[DO]"fmt
#endif

struct do_item {
	struct do_header *header;
	struct do_list_node *feat_list;
	struct do_item *next;
};

void scp_update_current_do(char *str, uint32_t len, uint32_t scp);
uint32_t mt_do_init_infos(struct do_info *info, uint32_t len, uint32_t scp);
void deinit_do_infos(void);
void set_do_api_enable(void);
void reset_do_api(void);
struct do_item *get_detail_do_infos(void);
int do_send_dram_start_addr(int in_isr);
