/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef BACKWARD_DRIVER_H
#define BACKWARD_DRIVER_H

#include "teei_common.h"

extern struct completion VFS_rd_comp;
extern struct completion VFS_wr_comp;
extern unsigned char *daulOS_VFS_share_mem;

struct create_vdrv_struct {
	unsigned int vdrv_type;
	unsigned int vdrv_phy_addr;
	unsigned int vdrv_size;
};

struct ack_vdrv_struct {
	unsigned int sysno;
};

void invoke_fastcall(void);
void secondary_invoke_fastcall(void *info);
long init_all_service_handlers(void);
int __vfs_handle(struct service_handler *handler);
int __reetime_handle(struct service_handler *handler);

#endif /* end of BACKWARD_DRIVER_H */
