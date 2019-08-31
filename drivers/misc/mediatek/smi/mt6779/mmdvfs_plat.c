/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/string.h>

#include "mmdvfs_plat.h"
#include "mtk_devinfo.h"
#include "mtk_qos_sram.h"

#undef pr_fmt
#define pr_fmt(fmt) "[mmdvfs][plat]" fmt

#define THERMAL_MASK 0x1	// 0b'001
#define THERMAL_OFFSET 0
#define CAM_MASK 0x6		// 0b'110
#define CAM_OFFSET 1
#define LIMIT_LEVEL1 0x7	// 0b'111
#define LIMIT_LEVEL2 0x5	// 0b'101
#define LIMIT_LEVEL3 0x3	// 0b'011

const char *mmdvfs_get_plat_name(void)
{
	u32 val = (get_devinfo_with_index(7) & 0xFF);

	if ((val == 0x09) || (val == 0x90) || (val == 0x08) || (val == 0x10)
	|| (val == 0x06) || (val == 0x60) || (val == 0x04) || (val == 0x20))
		return "_P95";

	return NULL;
}

void mmdvfs_update_limit_config(enum mmdvfs_limit_source source,
	u32 source_value, u32 *limit_value, u32 *limit_level)
{
	if (source == MMDVFS_LIMIT_THERMAL)
		*limit_value = (*limit_value & ~THERMAL_MASK) |
			((source_value << THERMAL_OFFSET) & THERMAL_MASK);
	else if (source == MMDVFS_LIMIT_CAM)
		*limit_value = (*limit_value & ~CAM_MASK) |
			((source_value << CAM_OFFSET) & CAM_MASK);

	if ((*limit_value & LIMIT_LEVEL1) == LIMIT_LEVEL1)
		*limit_level = 1;
	else if ((*limit_value & LIMIT_LEVEL2) == LIMIT_LEVEL2)
		*limit_level = 2;
	else if ((*limit_value & LIMIT_LEVEL3) == LIMIT_LEVEL3)
		*limit_level = 3;
	else
		*limit_level = 0;
}

#define MDP_START 5
#define LARB_MDP_ID 1
#define LARB_VENC_ID 3
#define LARB_IMG1_ID 5
#define LARB_IMG2_ID 8
#define LARB_CAM1_ID 9
#define LARB_CAM2_ID 10
#define COMM_MDP_PORT 1
#define COMM_VENC_PORT 3
#define COMM_IMG1_PORT 4
#define COMM_IMG2_PORT 5
#define COMM_CAM1_PORT 6
#define COMM_CAM2_PORT 7

void mmdvfs_update_qos_sram(struct mm_larb_request larb_req[], u32 larb_update)
{
	u32 bw;
	struct mm_qos_request *enum_req = NULL;

	if (larb_update & (1 << COMM_MDP_PORT)) {
		bw = larb_req[LARB_MDP_ID].total_bw_data;
		list_for_each_entry(enum_req,
			&larb_req[LARB_MDP_ID].larb_list, larb_node) {
			if (SMI_PORT_ID_GET(enum_req->master_id) < MDP_START) {
				bw -= get_comp_value(enum_req->bw_value,
					enum_req->comp_type, true);
			}
		}
		qos_sram_write(MM_SMI_MDP, bw);
	}

	if (larb_update & (1 << COMM_VENC_PORT)) {
		bw = larb_req[LARB_VENC_ID].total_bw_data;
		qos_sram_write(MM_SMI_VENC, bw);
	}

	if (larb_update & (1 << COMM_IMG1_PORT | 1 << COMM_IMG2_PORT)) {
		bw = larb_req[LARB_IMG1_ID].total_bw_data +
			larb_req[LARB_IMG2_ID].total_bw_data;
		qos_sram_write(MM_SMI_IMG, bw);
	}

	if (larb_update & (1 << COMM_CAM1_PORT | 1 << COMM_CAM2_PORT)) {
		bw = larb_req[LARB_CAM1_ID].total_bw_data +
			larb_req[LARB_CAM2_ID].total_bw_data;
		qos_sram_write(MM_SMI_CAM, bw);
	}
}

static u32 log_common_port_ids = (1 << 2) | (1 << 4);
static u32 log_larb_ids = (1 << 2) | (1 << 5);
bool mmdvfs_log_larb_mmp(s32 common_port_id, s32 larb_id)
{
	if (common_port_id >= 0)
		return (1 << common_port_id) & log_common_port_ids;
	if (larb_id >= 0)
		return (1 << larb_id) & log_larb_ids;
	return false;
}

#define MAX_OSTD_VALUE 40
void mmdvfs_update_plat_ostd(u32 larb, u32 hrt_port, u32 *ostd)
{
	if (larb <= LARB_MDP_ID && hrt_port)
		*ostd = MAX_OSTD_VALUE;
}

bool is_disp_larb(u32 larb)
{
	if (larb <= LARB_MDP_ID)
		return true;
	return false;
}

/* Return port number of CCU on SMI common */
inline u32 mmdvfs_get_ccu_smi_common_port(void)
{
	return 6;
}

s32 get_ccu_hrt_bw(struct mm_larb_request larb_req[])
{
	struct mm_qos_request *enum_req = NULL;
	s32 bw = larb_req[SMI_LARB_ID_GET(
				PORT_VIRTUAL_CCU_COMMON)].total_hrt_data;

	list_for_each_entry(enum_req,
		&larb_req[LARB_CAM1_ID].larb_list, larb_node) {
		if (enum_req->master_id == SMI_PORT_CCUI
			|| enum_req->master_id == SMI_PORT_CCUO)
			bw += enum_req->hrt_value;
	}
	return bw;
}


