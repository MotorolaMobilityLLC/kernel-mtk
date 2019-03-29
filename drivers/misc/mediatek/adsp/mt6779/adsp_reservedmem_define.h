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

#ifndef __ADSP_RESERVEDMEM_DEFINE_H__
#define __ADSP_RESERVEDMEM_DEFINE_H__

//#define FPGA_EARLY_DEVELOPMENT

static struct adsp_reserve_mblock adsp_reserve_mblock[] = {
	{
		.num = ADSP_A_SYSTEM_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
#ifdef FPGA_EARLY_DEVELOPMENT
		.size = 0x40000,/* 256KB */
#else
		.size = 0x700000,/* 7MB */
#endif
	},
	{
		.num = ADSP_A_IPI_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
#ifdef FPGA_EARLY_DEVELOPMENT
		.size = 0x80000,/* 512KB */
#else
		.size = 0x500000,/* 5MB */
#endif
	},
	{
		.num = ADSP_A_LOGGER_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
#ifdef FPGA_EARLY_DEVELOPMENT
		.size = 0x80000,/* 512KB */
#else
		.size = 0x200000,/* 2MB */
#endif
	},
#ifndef FPGA_EARLY_DEVELOPMENT
	{
		.num = ADSP_A_TRAX_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1000,/*4KB*/
	},
	{
		.num = ADSP_SPK_PROTECT_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x20000,/*128KB*/
	},
	{
		.num = ADSP_VOIP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000,/*192KB*/
	},
	{
		.num = ADSP_A2DP_PLAYBACK_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x40000,/*256KB*/
	},
	{
		.num = ADSP_OFFLOAD_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x400000,/*4MB*/
	},
	{
		.num = ADSP_EFFECT_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x60000,/*384KB*/
	},
	{
		.num = ADSP_VOICE_CALL_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x60000,/*384KB*/
	},
	{
		.num = ADSP_AFE_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x40000,/*256KB*/
	},
	{
		.num = ADSP_PLAYBACK_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000,/*192KB*/
	},
	{
		.num = ADSP_DEEPBUF_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000,/*192KB*/
	},
	{
		.num = ADSP_PRIMARY_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000,/*192KB*/
	},
	{
		.num = ADSP_CAPTURE_UL1_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x20000,/*128KB*/
	},
	{
		.num = ADSP_DATAPROVIDER_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000,/*192KB*/
	},
#endif
	{
		.num = ADSP_A_DEBUG_DUMP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x80000,/*512KB*/
	},
	{
		.num = ADSP_A_CORE_DUMP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x400,/*1KB*/
	},
};

#endif
