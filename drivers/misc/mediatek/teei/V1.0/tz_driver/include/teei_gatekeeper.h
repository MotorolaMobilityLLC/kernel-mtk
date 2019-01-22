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

#ifndef __TEEI_GATEKEEPER_H__
#define __TEEI_GATEKEEPER_H__

struct gatekeeper_command_struct {
	unsigned long mem_size;
	int retVal;
};

extern unsigned long gatekeeper_buff_addr;
/*extern struct semaphore gatekeeper_lock;*/
extern struct mutex pm_mutex;

unsigned long create_gatekeeper_fdrv(int buff_size);
int __send_gatekeeper_command(unsigned long share_memory_size);

#endif /*__TEEI_GATEKEEPER_H__*/
