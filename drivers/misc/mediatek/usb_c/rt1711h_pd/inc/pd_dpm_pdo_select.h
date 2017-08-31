/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef PD_DPM_PDO_SELECT_H
#define PD_DPM_PDO_SELECT_H

#include "inc/tcpci.h"

struct dpm_pdo_info_t {
	uint8_t type;
	int vmin;
	int vmax;
	int uw;
	int ma;
};

struct dpm_rdo_info_t {
	uint8_t pos;
	uint8_t type;
	bool mismatch;

	int vmin;
	int vmax;

	union {
		uint32_t max_uw;
		uint32_t max_ma;
	};

	union {
		uint32_t oper_uw;
		uint32_t oper_ma;
	};
};

#define DPM_PDO_TYPE_FIXED	0
#define DPM_PDO_TYPE_BAT	1
#define DPM_PDO_TYPE_VAR	2
#define DPM_PDO_TYPE(pdo)	((pdo & PDO_TYPE_MASK) >> 30)

extern void dpm_extract_pdo_info(
			uint32_t pdo, struct dpm_pdo_info_t *info);

extern bool dpm_find_match_req_info(struct dpm_rdo_info_t *req_info,
	uint32_t snk_pdo, int cnt, uint32_t *src_pdos,
	int min_uw, uint32_t select_rule);

#endif	/* PD_DPM_PDO_SELECT_H */
