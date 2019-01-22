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

#ifndef TEEI_FUNC_H
#define TEEI_FUNC_H
#define IOC_MAGIC                   'T'
#define TEEI_IOC_MAXNR		(4)
#define MICROTRUST_FP_SIZE	(0x80000)
#define FP_BUFFER_OFFSET	(0x10)
#define FP_LEN_MAX		(MICROTRUST_FP_SIZE - FP_BUFFER_OFFSET)
#define FP_LEN_MIN		0
#define CMD_MEM_CLEAR		_IO(IOC_MAGIC, 0x1)
#define CMD_FP_CMD			_IO(IOC_MAGIC, 0x2)
#define CMD_GATEKEEPER_CMD	_IO(IOC_MAGIC, 0x3)
#define CMD_LOAD_TEE		_IO(IOC_MAGIC, 0x4)
#define FP_MAJOR		   254
#define SHMEM_ENABLE       0
#define SHMEM_DISABLE      1
#define DEV_NAME			"teei_fp"
#define FP_DRIVER_ID		100
#define GK_DRIVER_ID		120

extern wait_queue_head_t __fp_open_wq;
extern int enter_tui_flag;

int send_fp_command(unsigned long share_memory_size);

#endif /* end of TEEI_FUNC_H*/
