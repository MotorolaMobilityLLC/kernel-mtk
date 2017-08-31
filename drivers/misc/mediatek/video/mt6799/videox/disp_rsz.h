/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __DISP_RSZ_H__
#define __DISP_RSZ_H__

#include "ddp_info.h"
#include "layering_rule.h"

struct rsz_lookuptable {
	unsigned int scaling_ratio;
	unsigned int in_w;
	unsigned int in_h;
};

struct rsz_lookuptable *primary_get_rsz_input_res_list(void);

extern char *HRT_path_name(enum HRT_PATH_SCENARIO path);
extern char *HRT_scale_name(enum HRT_SCALE_SCENARIO scale);
char *rsz_cfmt_name(enum RSZ_COLOR_FORMAT cfmt);

bool HRT_is_dual_path(int hrt_info);
bool HRT_is_pma_enabled(enum HRT_PATH_SCENARIO hrt_path);
bool HRT_is_resize_enabled(enum HRT_SCALE_SCENARIO scale);

enum DDP_SCENARIO_ENUM primary_get_DL_scenario(struct disp_ddp_path_config *pconfig);
enum DDP_SCENARIO_ENUM primary_get_OVL1to2_scenario(struct disp_ddp_path_config *pconfig);
enum DDP_SCENARIO_ENUM primary_get_RDMA_scenario(struct disp_ddp_path_config *pconfig);
enum DDP_SCENARIO_ENUM primary_get_OVLmemout_scenario(struct disp_ddp_path_config *pconfig);
enum HRT_PATH_SCENARIO disp_rsz_map_dual_to_single(enum HRT_PATH_SCENARIO hrt_path);
enum HRT_PATH_SCENARIO disp_rsz_map_single_to_dual(enum HRT_PATH_SCENARIO hrt_path);
enum HRT_PATH_SCENARIO disp_rsz_map_to_decouple_mirror(enum HRT_PATH_SCENARIO hrt_path);

/* debug function */
void disp_rsz_print_hrt_info(struct disp_ddp_path_config *pconfig, const char *func_name);

#endif
