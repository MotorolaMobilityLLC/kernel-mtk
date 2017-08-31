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

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/string.h>
#include <mt-plat/mtk_chip.h>
#ifdef CONFIG_MTK_DCS
#include <mt-plat/mtk_meminfo.h>
#endif
#include "layering_rule.h"

static struct layering_rule_ops l_rule_ops;
static struct layering_rule_info_t l_rule_info;

static int emi_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_2K */
	{-1, 4, 4, 4},
	/* HRT_BOUND_TYPE_2K_DUAL */
	{1, 4, 4, 4},
	/* HRT_BOUND_TYPE_FHD */
	{2, 6, 6, 8},
	/* HRT_BOUND_TYPE_2K 4 channel*/
	{-1, 9, 9, 11},
	/* HRT_BOUND_TYPE_2K_DUAL 4 channel*/
	{2, 18, 18, 22},
	/* HRT_BOUND_TYPE_FHD 4 channel*/
	{4, 12, 12, 12},
	/* HRT_BOUND_TYPE_120HZ */
	{-1, -1, -1, -1},
	/* HRT_BOUND_TYPE_2K_E2 */
	{-1, 5, 6, 6},
	/* HRT_BOUND_TYPE_2K_DUAL_E2 */
	{3, 5, 6, 6},
	/* HRT_BOUND_TYPE_FHD_E2 */
	{2, 8, 12, 12},
	/* HRT_BOUND_TYPE_2K_E2 4 channel*/
	{-1, 9, 9, 11},
	/* HRT_BOUND_TYPE_2K_DUAL_E2 4 channel*/
	{6, 9, 9, 11},
	/* HRT_BOUND_TYPE_FHD_E2 4 channel*/
	{11, 12, 12, 12},
	/* HRT_BOUND_TYPE_2K_DSC_E2 4 channel*/
	{6, 9, 9, 11},
};

static int larb_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_2K */
	{-1, 4, 5, 5},
	/* HRT_BOUND_TYPE_2K_DUAL */
	{4, 8, 10, 10},
	/* HRT_BOUND_TYPE_FHD */
	{4, 7, 9, 9},
	/* HRT_BOUND_TYPE_2K 4 channel*/
	{-1, 4, 5, 5},
	/* HRT_BOUND_TYPE_2K_DUAL 4 channel*/
	{4, 8, 10, 10},
	/* HRT_BOUND_TYPE_FHD 4 channel*/
	{4, 7, 9, 9},
	/* HRT_BOUND_TYPE_120HZ */
	{-1, -1, -1, -1},
	/* HRT_BOUND_TYPE_2K_E2 */
	{-1, 4, 5, 6},
	/* HRT_BOUND_TYPE_2K_DUAL_E2 */
	{6, 8, 10, 12},
	/* HRT_BOUND_TYPE_FHD_E2 */
	{5, 7, 9, 10},
	/* HRT_BOUND_TYPE_2K_E2 4 channel*/
	{-1, 4, 5, 6},
	/* HRT_BOUND_TYPE_2K_DUAL_E2 4 channel*/
	{6, 8, 10, 12},
	/* HRT_BOUND_TYPE_FHD_E2 4 channel*/
	{5, 7, 9, 10},
	/* HRT_BOUND_TYPE_2K_DSC_E2 4 channel*/
	{3, 4, 5, 6},
};

/**
 * The layer mapping table define ovl layer dispatch rule for both
 * primary and secondary display.Each table has 16 elements which
 * represent the layer mapping rule by the number of input layers.
 */
static int layer_mapping_table[HRT_TB_NUM][TOTAL_OVL_LAYER_NUM] = {
	/* HRT_TB_TYPE_GENERAL */
	{0x00000001, 0x00000011, 0x00000013, 0x00000033, 0x00000037, 0x0000003F,
	0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F, 0x0000003F},
	/* HRT_TB_TYPE_DUAL_DISP */
	{0x00010001, 0x00030003, 0x00070007, 0x000F000F, 0x001F0001F, 0x003F003F,
	0x003F003F, 0x003F003F, 0x003F003F, 0x003F003F, 0x003F003F, 0x003F003F},
	/* HRT_TB_TYPE_PARTIAL_RESIZE */
	{0x00010001, 0x00030011, 0x00070031, 0x000F0031, 0x001F0031, 0x003F0031,
	0x003F0031, 0x003F0031, 0x003F0031, 0x003F0031, 0x003F0031, 0x003F0031},
	/* HRT_TB_TYPE_PARTIAL_RESIZE_PMA */
	{0x00010001, 0x00030005, 0x0007000D, 0x000F001D, 0x001F003D, 0x003F003D,
	0x003F003D, 0x003F003D, 0x003F003D, 0x003F003D, 0x003F003D, 0x003F003D},
	/* HRT_TB_TYPE_MULTI_WINDOW_TUI */
	{0x00010001, 0x00030003, 0x00070007, 0x000F000F, 0x001F000F, 0x003F000F,
	0x003F000F, 0x003F000F, 0x003F000F, 0x003F000F, 0x003F000F, 0x003F000F},
};

/**
 * The larb mapping table represent the relation between LARB and OVL.
 */
static int larb_mapping_table[HRT_TB_NUM] = {
	0x00110010, 0x00110010, 0x00110010, 0x00110001, 0x00110001,
};

/**
 * The OVL mapping table is used to get the OVL index of correcponding layer.
 * The bit value 1 means the position of the last layer in OVL engine.
 */
static int ovl_mapping_table[HRT_TB_NUM] = {
	0x00280028, 0x00280028, 0x00280028, 0x00280022, 0x00280028,
};

#define GET_SYS_STATE(sys_state) ((l_rule_info.hrt_sys_state >> sys_state) & 0x1)

static bool can_switch_to_dual_pipe(struct disp_layer_info *disp_info)
{
#ifdef CONFIG_MTK_DCS
	int ret = 0, ch = 0;
	enum dcs_status status;

	ret = dcs_get_dcs_status_trylock(&ch, &status);
	if (ret < 0 || ch < 0) {
	DISPINFO("[DISP_HRT]DCS status is busy, ch:%d, dcs_status:%d\n", ch, status);
		mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_PULSE, ret, ch);
		ch = 2;
	} else {
		dcs_get_dcs_status_unlock();
	}
#endif

#ifdef CONFIG_MTK_DRE30_SUPPORT
	return false;
#endif


	if (disp_helper_get_option(DISP_OPT_DUAL_PIPE)) {
		int layer_limit;

		layer_limit = get_phy_layer_limit(layer_mapping_table[HRT_TB_TYPE_GENERAL][TOTAL_OVL_LAYER_NUM-1],
										HRT_PRIMARY);
		if (GET_SYS_STATE(DISP_HRT_FORCE_DUAL_OFF) ||
			is_ext_path(disp_info) ||
			(get_phy_ovl_layer_cnt(disp_info, HRT_PRIMARY) > 6 && layer_limit > 6) ||
			!is_max_lcm_resolution() ||
			disp_info->disp_mode[0] != 1 ||
#ifdef CONFIG_MTK_DCS
			ch == 2 ||
#endif
			l_rule_info.dal_enable ||
			primary_display_get_dsc_1slice_info())
			return false;

		return true;
	}
	return false;
}

static void get_bound_type(struct disp_layer_info *disp_info, int ch)
{
	unsigned int ver = mt_get_chip_sw_ver();
	unsigned int ver_offset = 0;

	if (ver != CHIP_SW_VER_01)
		ver_offset = HRT_BOUND_TYPE_2K_2CHANNEL_E2;

	if (primary_display_get_lcm_refresh_rate() == 120) {
		/* TODO: 120 condition */
		l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_120HZ;
	} else if (GET_SYS_STATE(DISP_HRT_MULTI_TUI_ON)) {
		if (is_max_lcm_resolution()) {
			if (ch == 4)
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL;
			else
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_2CHANNEL;
		} else {
			if (ch == 4)
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_4CHANNEL;
			else
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_2CHANNEL;
		}
	} else if (can_switch_to_dual_pipe(disp_info)) {
		if (ch == 4)
			l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_DUAL_4CHANNEL;
		else
			l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_DUAL_2CHANNEL;
	} else if (is_ext_path(disp_info)) {
		if (is_max_lcm_resolution()) {
			if (ch == 4) {
				if (primary_display_get_dsc_1slice_info())
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL_DSC_E2;
				else
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL;
			} else {
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_2CHANNEL;
			}
		} else {
			if (ch == 4)
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_4CHANNEL;
			else
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_2CHANNEL;
		}
	} else if (is_decouple_path(disp_info)) {
		if (is_max_lcm_resolution()) {
			if (ch == 4) {
				if (primary_display_get_dsc_1slice_info())
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL_DSC_E2;
				else
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL;
			} else {
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_2CHANNEL;
			}
		} else {
			if (ch == 4)
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_4CHANNEL;
			else
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_2CHANNEL;
		}
	} else {
		if (is_max_lcm_resolution()) {
			if (ch == 4) {
				if (primary_display_get_dsc_1slice_info())
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL_DSC_E2;
				else
					l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_4CHANNEL;
			} else {
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_2K_2CHANNEL;
			}
		} else {
			if (ch == 4)
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_4CHANNEL;
			else
				l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_FHD_2CHANNEL;
		}
	}

	if (!primary_display_get_dsc_1slice_info())
		l_rule_info.bound_tb_idx += ver_offset;
}

static void layering_rule_senario_decision(struct disp_layer_info *disp_info)
{
	int ret = 0, ch = 0;

#ifdef CONFIG_MTK_DCS
	enum dcs_status status;

	ret = dcs_get_dcs_status_trylock(&ch, &status);
	if (ret < 0 || ch < 0) {
		DISPINFO("[DISP_HRT]DCS status is busy, ch:%d, dcs_status:%d\n", ch, status);
		mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_PULSE, ret, ch);
		ch = 2;
	} else {
		dcs_get_dcs_status_unlock();
	}
#endif

	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_START, ret, ch);

	if (primary_display_get_lcm_refresh_rate() == 120) {
		/* TODO: 120 condition */
		l_rule_info.primary_fps = 120;
	} else if (GET_SYS_STATE(DISP_HRT_MULTI_TUI_ON)) {
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_MULTI_WINDOW_TUI;
		l_rule_info.primary_fps = 60;
	} else if (can_switch_to_dual_pipe(disp_info)) {
		switch (l_rule_info.disp_path) {
		case HRT_PATH_RESIZE_GENERAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_PIPE_RESIZE_GENERAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
			break;
		case HRT_PATH_RESIZE_PARTIAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE;
			break;
		case HRT_PATH_RESIZE_PARTIAL_PMA:
			l_rule_info.disp_path = HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE_PMA;
			break;
		default:
			l_rule_info.disp_path = HRT_PATH_DUAL_PIPE;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		}
		l_rule_info.primary_fps = 60;
	} else if (is_ext_path(disp_info)) {
		switch (l_rule_info.disp_path) {
		case HRT_PATH_RESIZE_GENERAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
			break;
		case HRT_PATH_RESIZE_PARTIAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE;
			break;
		case HRT_PATH_RESIZE_PARTIAL_PMA:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE_PMA;
			break;
		default:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_EXT;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		}
		l_rule_info.primary_fps = 60;
	} else if (is_decouple_path(disp_info)) {
		switch (l_rule_info.disp_path) {
		case HRT_PATH_RESIZE_GENERAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
			break;
		case HRT_PATH_RESIZE_PARTIAL:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE;
			break;
		case HRT_PATH_RESIZE_PARTIAL_PMA:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE_PMA;
			break;
		default:
			l_rule_info.disp_path = HRT_PATH_DUAL_DISP_MIRROR;
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		}
		l_rule_info.primary_fps = 60;
	} else {
		switch (l_rule_info.disp_path) {
		case HRT_PATH_RESIZE_GENERAL:
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
			break;
		case HRT_PATH_RESIZE_PARTIAL:
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE;
			break;
		case HRT_PATH_RESIZE_PARTIAL_PMA:
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_PARTIAL_RESIZE_PMA;
			break;
		default:
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
			l_rule_info.disp_path = HRT_PATH_GENERAL;
		}
		l_rule_info.primary_fps = 60;
	}

	get_bound_type(disp_info, ch);
	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_END, l_rule_info.disp_path,
		l_rule_info.layer_tb_idx | (l_rule_info.bound_tb_idx << 16));

}

static bool has_rsz_layer(struct disp_layer_info *disp_info, int disp_idx)
{
	int i;
	struct layer_config *layer_info;

	for (i = 0 ; i < disp_info->layer_num[disp_idx]; i++) {
		layer_info = &disp_info->input_config[disp_idx][i];

		if (i >= disp_info->gles_head[disp_idx] && i <= disp_info->gles_tail[disp_idx])
			continue;
		if ((layer_info->src_height != layer_info->dst_height) ||
			(layer_info->src_width != layer_info->dst_width))
			return true;
	}

	return false;
}

static int get_scale_scenario(struct layer_config *layer_info)
{
	int scale_diff;

	scale_diff = layer_info->dst_width * 3 / 8 - layer_info->src_width;
	if (scale_diff >= -1 && scale_diff <= 1) {
		scale_diff = layer_info->dst_height * 3 / 8 - layer_info->src_height;
		if (scale_diff >= -1 && scale_diff <= 1)
			return HRT_SCALE_266;
		else
			return HRT_SCALE_UNKNOWN;
	}

	scale_diff = layer_info->dst_width / 2 - layer_info->src_width;
	if (scale_diff >= -1 && scale_diff <= 1) {
		scale_diff = layer_info->dst_height / 2 - layer_info->src_height;
		if (scale_diff >= -1 && scale_diff <= 1)
			return HRT_SCALE_200;
		else
			return HRT_SCALE_UNKNOWN;
	}

	scale_diff = layer_info->dst_width * 2 / 3 - layer_info->src_width;
	if (scale_diff >= -1 && scale_diff <= 1) {
		scale_diff = layer_info->dst_height * 2 / 3 - layer_info->src_height;
		if (scale_diff >= -1 && scale_diff <= 1)
			return HRT_SCALE_150;
		else
			return HRT_SCALE_UNKNOWN;
	}

	scale_diff = layer_info->dst_width * 3 / 4 - layer_info->src_width;
	if (scale_diff >= -1 && scale_diff <= 1) {
		scale_diff = layer_info->dst_height * 3 / 4 - layer_info->src_height;
		if (scale_diff >= -1 && scale_diff <= 1)
			return HRT_SCALE_133;
		else
			return HRT_SCALE_UNKNOWN;
	}

	return HRT_SCALE_UNKNOWN;
}

static bool is_scale_ratio_valid(struct layer_config *layer_info, int target_scale_ratio)
{
	if (target_scale_ratio != HRT_SCALE_UNKNOWN &&
		get_scale_scenario(layer_info) == target_scale_ratio)
		return true;

	return false;
}

static bool is_rsz_general(struct disp_layer_info *disp_info, int disp_idx, int *scale_ratio)
{
	int i;
	int tmp_scale_ratio = HRT_SCALE_UNKNOWN;
	struct layer_config *layer_info;

	if (disp_info->gles_head[disp_idx] != -1 || disp_info->gles_tail[disp_idx] != -1)
		return false;

	for (i = 0 ; i < disp_info->layer_num[disp_idx]; i++) {
		layer_info = &disp_info->input_config[disp_idx][i];

		if ((layer_info->src_height != layer_info->dst_height) ||
			(layer_info->src_width != layer_info->dst_width)) {

			if (tmp_scale_ratio == HRT_SCALE_UNKNOWN)
				tmp_scale_ratio = get_scale_scenario(layer_info);
			if (!is_scale_ratio_valid(layer_info, tmp_scale_ratio))
				return false;

		} else {
			return false;
		}
	}
#ifdef RSZ_SCALE_RATIO_ROLLBACK
	if (primary_display_is_video_mode() && l_rule_info.scale_rate != 0 && l_rule_info.scale_rate != tmp_scale_ratio)
		return false;
#endif
	*scale_ratio = tmp_scale_ratio;
	return true;
}

static bool is_rsz_partial(struct disp_layer_info *disp_info, int disp_idx, int *scale_ratio)
{
	int i;
	int tmp_scale_ratio = HRT_SCALE_UNKNOWN;
	struct layer_config *layer_info;

	if (disp_info->gles_head[disp_idx] == 0 || disp_info->layer_num[disp_idx] <= 0)
		return false;

	layer_info = &disp_info->input_config[disp_idx][0];

#ifdef PARTIAL_L0_YUV_ONLY
	if (!is_yuv(layer_info->src_fmt))
		return false;
#endif

	if ((layer_info->src_height == layer_info->dst_height) ||
		(layer_info->src_width == layer_info->dst_width))
		return false;

	tmp_scale_ratio = get_scale_scenario(layer_info);
	if (tmp_scale_ratio == HRT_SCALE_UNKNOWN)
		return false;

	for (i = 1 ; i < disp_info->layer_num[disp_idx]; i++) {
		layer_info = &disp_info->input_config[disp_idx][i];
		if ((layer_info->src_height != layer_info->dst_height) ||
			(layer_info->src_width != layer_info->dst_width))
			return false;
	}
#ifdef RSZ_SCALE_RATIO_ROLLBACK
	if (primary_display_is_video_mode() && l_rule_info.scale_rate != 0 && l_rule_info.scale_rate != tmp_scale_ratio)
		return false;
#endif

	*scale_ratio = tmp_scale_ratio;
	return true;
}

static bool is_rsz_partial_pma(struct disp_layer_info *disp_info, int disp_idx, int *scale_ratio)
{
	int i;
	int tmp_scale_ratio = HRT_SCALE_UNKNOWN;
	struct layer_config *layer_info;

	if (disp_info->gles_head[disp_idx] != -1 || disp_info->gles_tail[disp_idx] != -1)
		return false;

	layer_info = &disp_info->input_config[disp_idx][0];

#ifdef PARTIAL_PMA_L0_YUV_ONLY
	if (!is_yuv(layer_info->src_fmt))
		return false;
#endif

	if ((layer_info->src_height != layer_info->dst_height) ||
		(layer_info->src_width != layer_info->dst_width))
		return false;

	for (i = 1 ; i < disp_info->layer_num[disp_idx]; i++) {
		layer_info = &disp_info->input_config[disp_idx][i];
		if (tmp_scale_ratio == HRT_SCALE_UNKNOWN)
			tmp_scale_ratio = get_scale_scenario(layer_info);
		if (!is_scale_ratio_valid(layer_info, tmp_scale_ratio))
			return false;
		if (tmp_scale_ratio == HRT_SCALE_NONE)
			return false;
		if (is_argb_fmt(layer_info->src_fmt))
			return false;
	}
#ifdef RSZ_SCALE_RATIO_ROLLBACK
	if (primary_display_is_video_mode() && l_rule_info.scale_rate != 0 && l_rule_info.scale_rate != tmp_scale_ratio)
		return false;
#endif

	*scale_ratio = tmp_scale_ratio;
	return true;
}

static bool gles_layer_adjustment_resize(struct disp_layer_info *disp_info)
{
	int disp_idx;
	int scale_ratio = HRT_SCALE_UNKNOWN;

	if (l_rule_info.dal_enable) {
		rollback_all_resize_layer_to_GPU(disp_info, HRT_PRIMARY);
		rollback_all_resize_layer_to_GPU(disp_info, HRT_SECONDARY);
	} else if (is_ext_path(disp_info)) {
		rollback_all_resize_layer_to_GPU(disp_info, HRT_SECONDARY);
	}
#ifdef DECOUPLE_RSZ_OFF
	if (disp_info->disp_mode[0] != 1)
		rollback_all_resize_layer_to_GPU(disp_info, HRT_PRIMARY);
#endif
	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {

		if (disp_info->layer_num[disp_idx] <= 0) {
			if (disp_idx == HRT_PRIMARY)
				l_rule_info.scale_rate = HRT_SCALE_NONE;
			continue;
		}

		/* Now we only support resize layer on Primary Display */
		if (disp_idx == HRT_SECONDARY)
			continue;

		/**
		 * Currently display system only support 3 kind of scale cases:
		 * RESIZE ALL, RESIZE PARTIAL and RESIZE PARTIAL with PMA.
		 *
		 * RESIZE ALL: All the input layers have same scale ratio.
		 * RESIZE PARTIAL: Only the input layer 0 need to be scaled.
		 * RESIZE PARTIAL PMA: Layer 0 is non-resizing and the other layers need to be scaled
		 * with the equivalent scale ratio.
		 **/
		if (!has_rsz_layer(disp_info, disp_idx)) {
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		} else if (is_rsz_general(disp_info, disp_idx, &scale_ratio)) {
			l_rule_info.scale_rate = scale_ratio;
			l_rule_info.disp_path = HRT_PATH_RESIZE_GENERAL;
		} else if (is_rsz_partial(disp_info, disp_idx, &scale_ratio)) {
			l_rule_info.scale_rate = scale_ratio;
			l_rule_info.disp_path = HRT_PATH_RESIZE_PARTIAL;
		} else if (is_rsz_partial_pma(disp_info, disp_idx, &scale_ratio)) {
			l_rule_info.scale_rate = scale_ratio;
			l_rule_info.disp_path = HRT_PATH_RESIZE_PARTIAL_PMA;
		} else {
			rollback_all_resize_layer_to_GPU(disp_info, HRT_PRIMARY);
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		}
	}

#ifdef HRT_DEBUG_LEVEL1
	DISPMSG("[%s] scale_rate:%d scale_ratio:%d\n", __func__, l_rule_info->scale_rate, scale_ratio);
	dump_disp_info(disp_info, DISP_DEBUG_LEVEL_INFO);
#endif

	return 0;
}


static int get_hrt_bound(int is_larb, int hrt_level)
{
	if (is_larb)
		return larb_bound_table[l_rule_info.bound_tb_idx][hrt_level];
	else
		return emi_bound_table[l_rule_info.bound_tb_idx][hrt_level];
}

static int *get_bound_table(enum DISP_HW_MAPPING_TB_TYPE tb_type)
{
	switch (tb_type) {
	case DISP_HW_EMI_BOUND_TB:
		return emi_bound_table[l_rule_info.bound_tb_idx];
	case DISP_HW_LARB_BOUND_TB:
		return larb_bound_table[l_rule_info.bound_tb_idx];
	default:
		return NULL;
	}
}

static int get_mapping_table(enum DISP_HW_MAPPING_TB_TYPE tb_type, int param)
{
	switch (tb_type) {
	case DISP_HW_OVL_TB:
		return ovl_mapping_table[l_rule_info.layer_tb_idx];
	case DISP_HW_LARB_TB:
		return larb_mapping_table[l_rule_info.layer_tb_idx];
	case DISP_HW_LAYER_TB:
		return layer_mapping_table[l_rule_info.layer_tb_idx][param];
	default:
		return -1;
	}
}

void layering_rule_init(void)
{
	l_rule_info.primary_fps = 60;
	register_layering_rule_ops(&l_rule_ops, &l_rule_info);
}

void layering_rule_gpu_rsz_info_change(void)
{
	switch (l_rule_info.disp_path) {
	case HRT_PATH_RESIZE_GENERAL:
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
		break;
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
		l_rule_info.disp_path = HRT_PATH_DUAL_PIPE;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		break;
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
		l_rule_info.disp_path = HRT_PATH_DUAL_DISP_MIRROR;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		break;
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		l_rule_info.disp_path = HRT_PATH_DUAL_DISP_EXT;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_DUAL_DISP;
		break;
	}
	l_rule_info.scale_rate = HRT_SCALE_NONE;

}

static struct layering_rule_ops l_rule_ops = {
	.resizing_rule = gles_layer_adjustment_resize,
	.scenario_decision = layering_rule_senario_decision,
	.get_bound_table = get_bound_table,
	.get_hrt_bound = get_hrt_bound,
	.get_mapping_table = get_mapping_table,
	.rsz_by_gpu_info_change = layering_rule_gpu_rsz_info_change,
};

