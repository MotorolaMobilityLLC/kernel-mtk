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

#include "disp_drv_log.h"
#include "mtk_hrt.h"
#include "disp_rsz.h"

char *HRT_path_name(enum HRT_PATH_SCENARIO path)
{
	switch (path) {
	case HRT_PATH_GENERAL:
		return "1p";
	case HRT_PATH_DUAL_PIPE:
		return "2p";
	case HRT_PATH_DUAL_DISP_MIRROR:
		return "2dM";
	case HRT_PATH_DUAL_DISP_EXT:
		return "2dE";
	case HRT_PATH_RESIZE_GENERAL:
		return "1p_rG";
	case HRT_PATH_RESIZE_PARTIAL:
		return "1p_rP";
	case HRT_PATH_RESIZE_PARTIAL_PMA:
		return "1p_rPP";
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return "2p_rG";
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
		return "2p_rP";
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return "2p_rPP";
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
		return "2dM_rG";
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		return "2dE_rG";
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
		return "2dM_rP";
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
		return "2dE_rP";
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
		return "2dM_rPP";
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		return "2dE_rPP";
	case HRT_PATH_NUM:
		return "HRT_PATH_NUM";
	case HRT_PATH_UNKNOWN:
		return "HRT_PATH_UNKNOWN";
	default:
		break;
	}
	return "HRT_PATH_UNKNOWN";
}

char *HRT_scale_name(enum HRT_SCALE_SCENARIO scale)
{
	switch (scale) {
	case HRT_SCALE_266:
		return "266";
	case HRT_SCALE_200:
		return "200";
	case HRT_SCALE_150:
		return "150";
	case HRT_SCALE_133:
		return "133";
	case HRT_SCALE_NONE:
		return "100";
	case HRT_SCALE_UNKNOWN:
		return "HRT_SCALE_UNKNOWN";
	default:
		break;
	}
	return "HRT_SCALE_UNKNOWN";
}

char *rsz_cfmt_name(enum RSZ_COLOR_FORMAT cfmt)
{
	switch (cfmt) {
	case ARGB8101010:
		return "RSZ_CFMT_ARGB8101010";
	case RGB999:
		return "RSZ_CFMT_RGB999";
	case RGB888:
		return "RSZ_CFMT_RGB888";
	case UNKNOWN_RSZ_CFMT:
		return "RSZ_CFMT_UNKNOWN";
	default:
		break;
	}
	return "RSZ_CFMT_UNKNOWN";
}

bool HRT_is_dual_path(int hrt_info)
{
	switch (HRT_GET_PATH_SCENARIO(hrt_info)) {
	case HRT_PATH_DUAL_PIPE:
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return true;
	default:
		break;
	}
	return false;
}

bool HRT_is_pma_enabled(int hrt_info)
{
	switch (HRT_GET_PATH_SCENARIO(hrt_info)) {
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		return true;
	default:
		break;
	}
	return false;
}

bool HRT_is_resize_enabled(enum HRT_SCALE_SCENARIO scale)
{
	switch (scale) {
	case HRT_SCALE_266:
	case HRT_SCALE_200:
	case HRT_SCALE_150:
	case HRT_SCALE_133:
		return true;
	default:
		break;
	}
	return false;
}

#define RSZ_RES_LIST_NUM 4

/* MUST: Sequence from small ratio to big. */
unsigned int rsz_support_scaling_ratio[HRT_SCALE_NUM] = {10000, 13333, 15000, 20000, 26666};
/* MUST: Sequence from big resolution to small. Synced with HWC. */
struct rsz_lookuptable rsz_lut[HRT_SCALE_NUM] = { { 0 } };
unsigned int rsz_support_res[RSZ_RES_LIST_NUM][2] = {
	{ 1440, 2560 }, /* WQHD */
	{ 1080, 1920 }, /* FHD */
	{ 720, 1280 }, /* HD */
	{ 540, 960 } /* qHD */
};

/* Get primary display resizer supported input scaling ratio list. */
struct rsz_lookuptable *primary_get_rsz_input_res_list(void)
{
	static unsigned int lcm_w, lcm_h;
	int i = 0, k = 0;

	if (lcm_w == primary_display_get_width() &&
	    lcm_h == primary_display_get_height())
		goto done;

	lcm_w = primary_display_get_width();
	lcm_h = primary_display_get_height();

	for (i = 0; i < HRT_SCALE_NUM; i++) {
		unsigned int ratio = rsz_support_scaling_ratio[i];
		unsigned int src_w = lcm_w * 10000 / ratio;
		unsigned int src_h = lcm_h * 10000 / ratio;

		for (k = 0; k < RSZ_RES_LIST_NUM; k++) {
			unsigned int rsz_in_w = rsz_support_res[k][0];
			unsigned int rsz_in_h = rsz_support_res[k][1];

			if (src_w != rsz_in_w)
				continue;

			if (src_h == rsz_in_h) {
				rsz_lut[i].scaling_ratio = ratio;
				rsz_lut[i].in_w = rsz_in_w;
				rsz_lut[i].in_h = rsz_in_h;
				break;
			}
		}
	}

	DISPDBG("rsz support input resolution list: LCM resolution %ux%u\n", lcm_w, lcm_h);
	for (i = 0; i < HRT_SCALE_NUM; i++)
		DISPDBG("(%ux%u), ratio:%u\n", rsz_lut[i].in_w, rsz_lut[i].in_h, rsz_lut[i].scaling_ratio);

done:
	return rsz_lut;
}

enum DDP_SCENARIO_ENUM primary_get_DL_scenario(struct disp_ddp_path_config *pconfig)
{
	switch (pconfig->hrt_path) {
	case HRT_PATH_DUAL_PIPE:
		return DDP_SCENARIO_PRIMARY_DISP_LEFT;
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP_LEFT;
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_DISP_LEFT;

	case HRT_PATH_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR:
	case HRT_PATH_DUAL_DISP_EXT:
		return DDP_SCENARIO_PRIMARY_DISP;
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP;
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_DISP;
	case HRT_PATH_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_DISP;

	default:
		break;
	}
	return DDP_SCENARIO_MAX;
}

enum DDP_SCENARIO_ENUM primary_get_OVL1to2_scenario(struct disp_ddp_path_config *pconfig)
{
	switch (pconfig->hrt_path) {
	case HRT_PATH_DUAL_PIPE:
		return DDP_SCENARIO_PRIMARY_ALL_LEFT;
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_OVL0_RSZ0_1TO2_LEFT;
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_1TO2_LEFT;

	case HRT_PATH_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR:
	case HRT_PATH_DUAL_DISP_EXT:
		return DDP_SCENARIO_PRIMARY_ALL;
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_OVL0_RSZ0_1TO2;
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_1TO2;
	case HRT_PATH_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_1TO2;

	default:
		break;
	}
	return DDP_SCENARIO_MAX;
}

enum DDP_SCENARIO_ENUM primary_get_RDMA_scenario(struct disp_ddp_path_config *pconfig)
{
	switch (pconfig->hrt_path) {
	case HRT_PATH_DUAL_PIPE:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_RDMA_COLOR_DISP_LEFT;
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_RDMA0_CCORR0_RSZ0_DISP_LEFT;

	case HRT_PATH_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR:
	case HRT_PATH_DUAL_DISP_EXT:
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP;
	case HRT_PATH_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_RDMA0_CCORR0_RSZ0_DISP;

	default:
		break;
	}
	return DDP_SCENARIO_MAX;
}

enum DDP_SCENARIO_ENUM primary_get_OVLmemout_scenario(struct disp_ddp_path_config *pconfig)
{
	/*
	 * Special case for OVL_MEMOUT of dual pipe.
	 * These path selections should follow HRT and dual pipe design.
	 */
	switch (pconfig->hrt_path) {
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:

	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		return DDP_SCENARIO_PRIMARY_OVL0_RSZ0_MEMOUT;

	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		return DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_MEMOUT;

	case HRT_PATH_DUAL_PIPE:
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:

	case HRT_PATH_GENERAL:
	case HRT_PATH_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR:
	case HRT_PATH_DUAL_DISP_EXT:
		return DDP_SCENARIO_PRIMARY_OVL_MEMOUT;

	default:
		break;
	}
	return DDP_SCENARIO_MAX;
}

enum HRT_PATH_SCENARIO disp_rsz_map_dual_to_single(enum HRT_PATH_SCENARIO hrt_path)
{
	switch (hrt_path) {
	case HRT_PATH_DUAL_PIPE:
		return HRT_PATH_GENERAL;
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return HRT_PATH_RESIZE_GENERAL;
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
		return HRT_PATH_RESIZE_PARTIAL;
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return HRT_PATH_RESIZE_PARTIAL_PMA;
	default:
		break;
	}
	return hrt_path;
}

enum HRT_PATH_SCENARIO disp_rsz_map_single_to_dual(enum HRT_PATH_SCENARIO hrt_path)
{
	switch (hrt_path) {
	case HRT_PATH_GENERAL:
		return HRT_PATH_DUAL_PIPE;
	case HRT_PATH_RESIZE_GENERAL:
		return HRT_PATH_DUAL_PIPE_RESIZE_GENERAL;
	case HRT_PATH_RESIZE_PARTIAL:
		return HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL;
	case HRT_PATH_RESIZE_PARTIAL_PMA:
		return HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA;
	default:
		break;
	}
	return hrt_path;
}

enum HRT_PATH_SCENARIO disp_rsz_map_to_decouple_mirror(enum HRT_PATH_SCENARIO hrt_path)
{
	switch (hrt_path) {
	case HRT_PATH_GENERAL:
	case HRT_PATH_DUAL_PIPE:
		return HRT_PATH_DUAL_DISP_MIRROR;
	case HRT_PATH_RESIZE_GENERAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		return HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL;
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
		return HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL;
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		return HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA;
	default:
		DISPMSG("%s:%s no map to decouple mirror path\n",
			__func__, HRT_path_name(hrt_path));
		break;
	}
	return hrt_path;
}

void disp_rsz_print_hrt_info(struct disp_ddp_path_config *pconfig, const char *func)
{
	disp_path_handle phandle = (disp_path_handle)pconfig->path_handle;

	DISPDBG("%s:dst_dirty:%d,en:%d,r:%s,path:%s,dual:%d on scn:%s(%p)\n",
		func, pconfig->dst_dirty, pconfig->rsz_enable,
		HRT_scale_name(pconfig->hrt_scale), HRT_path_name(pconfig->hrt_path),
		pconfig->is_dual, ddp_get_scenario_name(dpmgr_get_scenario(phandle)), phandle);
}
