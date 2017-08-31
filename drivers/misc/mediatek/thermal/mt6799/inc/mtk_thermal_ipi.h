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

#ifndef __MTK_THERMAL_IPI_H__
#define __MTK_THERMAL_IPI_H__

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#define THERMAL_ENABLE_TINYSYS_SSPM (1)
#else
#define THERMAL_ENABLE_TINYSYS_SSPM (0)
#endif

#if THERMAL_ENABLE_TINYSYS_SSPM
#include "sspm_ipi.h"
#include <sspm_reservedmem_define.h>

#define THERMAL_SLOT_NUM (4)

/* IPI Msg type */
enum {
	THERMAL_IPI_INIT_GRP1,
	THERMAL_IPI_INIT_GRP2,
	THERMAL_IPI_INIT_GRP3,
	THERMAL_IPI_INIT_GRP4,
	THERMAL_IPI_INIT_GRP5,
	THERMAL_IPI_INIT_GRP6,
	THERMAL_IPI_GET_TEMP,
	NR_THERMAL_IPI
};

/* IPI Msg data structure */
struct thermal_ipi_data {
	unsigned int cmd;
	union {
		struct {
			int arg[THERMAL_SLOT_NUM - 1];
		} data;
	} u;
};
extern unsigned int thermal_to_sspm(unsigned int cmd, struct thermal_ipi_data *thermal_data);
#endif /* THERMAL_ENABLE_TINYSYS_SSPM */
#endif
