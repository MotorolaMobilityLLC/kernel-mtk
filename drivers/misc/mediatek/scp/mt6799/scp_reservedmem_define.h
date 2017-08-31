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

#ifndef __SCP_RESERVEDMEM_DEFINE_H__
#define __SCP_RESERVEDMEM_DEFINE_H__

static scp_reserve_mblock_t scp_reserve_mblock[] = {
	{
		.num = VOW_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1A000,/*104KB*/
	},
	{
		.num = SENS_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x800000,/*8MB*/
	},
	{
		.num = MP3_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x400000,/*4MB*/
	},
	{
		.num = RTOS_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x100000,/*1MB*/
	},
	{
		.num = OPENDSP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x200000,/*2MB*/
	},
	{
		.num = SENS_MEM_DIRECT_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x2000,/*8KB*/
	},
	{
		.num = SCP_A_LOGGER_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x200000,/*2MB*/
	},
	{
		.num = SCP_B_LOGGER_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x200000,/*2MB*/
	},
};


#endif
