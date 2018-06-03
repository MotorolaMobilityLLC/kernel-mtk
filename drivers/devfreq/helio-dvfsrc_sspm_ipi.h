/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __HELIO_DVFSRC_SSPM_IPI_H
#define __HELIO_DVFSRC_SSPM_IPI_H

enum {
	QOS_IPI_QOS_ENABLE = 0,
	QOS_IPI_VCORE_OPP,
	QOS_IPI_DDR_OPP,
	QOS_IPI_ERROR_HANDLER,
	QOS_IPI_SWPM_INIT,
	QOS_IPI_UPOWER_DATA_TRANSFER,
	QOS_IPI_UPOWER_DUMP_TABLE,
	QOS_IPI_GET_GPU_BW,

	NR_QOS_IPI,
};


struct qos_data {
	unsigned int cmd;
	union {
		struct {
			unsigned int enable;
			unsigned int dvfs_en;
			unsigned int spm_dram_type;
		} qos_init;
		struct {
			unsigned int opp;
			unsigned int vcore_uv;
		} vcore_opp;
		struct {
			unsigned int opp;
			unsigned int ddr_khz;
		} ddr_opp;
		struct {
			unsigned int master_type;
			unsigned int emi_data;
			unsigned int predict_data;
		} error_handler;
		struct {
			unsigned int dram_addr;
			unsigned int dram_size;
			unsigned int dram_ch_num;
		} swpm_init;
		struct {
			unsigned int arg[3];
		} upower_data;
	} u;
};

static int qos_ipi_to_sspm_command(void *buffer, int slot)
{
#if 0
	int ack_data;

	return sspm_ipi_send_sync(IPI_ID_QOS, IPI_OPT_POLLING,
			buffer, slot, &ack_data, 1);
#endif
	return 0;
}

#endif

