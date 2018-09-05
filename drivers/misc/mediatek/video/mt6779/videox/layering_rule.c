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

#if defined(CONFIG_MTK_DRAMC)
#include "mtk_dramc.h"
#endif
#include "layering_rule.h"
#include "disp_drv_log.h"
#include "ddp_rsz.h"
#include "primary_display.h"
#include "disp_lowpower.h"
#include "mtk_disp_mgr.h"
#include "disp_rect.h"

static struct layering_rule_ops l_rule_ops;
static struct layering_rule_info_t l_rule_info;

int emi_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_LP4 */
	{100, 300, 500, 600},
	/* HRT_BOUND_TYPE_LP4_FHD_PLUS */
	{100, 300, 500, 600},
};

int larb_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_LP4 */
	{100, 300, 500, 600},
	/* HRT_BOUND_TYPE_LP4_FHD_PLUS */
	{100, 300, 500, 600},
};

/**
 * The layer mapping table define ovl layer dispatch rule for both
 * primary and secondary display.Each table has 16 elements which
 * represent the layer mapping rule by the number of input layers.
 */
static int layer_mapping_table[HRT_TB_NUM][TOTAL_OVL_LAYER_NUM] = {
	/* HRT_TB_TYPE_GENERAL */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003003F,
	0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F},
	/* HRT_TB_TYPE_RPO_L0 */
	{0x00010001, 0x00030005, 0x0003000D, 0x0003001D, 0x0003003D, 0x0003003D,
	0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D},
	/* HRT_TB_TYPE_RPO_L0L1 */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003003F,
	0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F},
};

/**
 * The larb mapping table represent the relation between LARB and OVL.
 */
static int larb_mapping_table[HRT_TB_NUM] = {
	0x00010010, 0x00010010, 0x00010010,
};

/**
 * The OVL mapping table is used to get the OVL index of correcponding layer.
 * The bit value 1 means the position of the last layer in OVL engine.
 */
static int ovl_mapping_table[HRT_TB_NUM] = {
	0x00020022, 0x00020022, 0x00020022,
};

#define GET_SYS_STATE(sys_state) \
	((l_rule_info.hrt_sys_state >> sys_state) & 0x1)

static bool has_rsz_layer(struct disp_layer_info *disp_info, int disp_idx)
{
	int i = 0;
	struct layer_config *c = NULL;

	for (i = 0; i < disp_info->layer_num[disp_idx]; i++) {
		c = &disp_info->input_config[disp_idx][i];

		if (is_gles_layer(disp_info, disp_idx, i))
			continue;

		if ((c->src_height != c->dst_height) ||
		    (c->src_width != c->dst_width))
			return true;
	}

	return false;
}

static bool is_rsz_valid(struct layer_config *c)
{
	if (c->src_width == c->dst_width &&
	    c->src_height == c->dst_height)
		return false;
	if (c->src_width > c->dst_width ||
	    c->src_height > c->dst_height)
		return false;
	/*
	 * HWC adjusts MDP layer alignment after query_valid_layer.
	 * This makes the decision of layering rule unreliable. Thus we
	 * add constraint to avoid frame_cfg becoming scale-down.
	 *
	 * TODO: If HWC adjusts MDP layer alignment before
	 * query_valid_layer, we could remove this if statement.
	 */
	if ((has_layer_cap(c, MDP_RSZ_LAYER) ||
	     has_layer_cap(c, MDP_ROT_LAYER)) &&
	    (c->dst_width - c->src_width <= MDP_ALIGNMENT_MARGIN ||
	     c->dst_height - c->src_height <= MDP_ALIGNMENT_MARGIN))
		return false;

	return true;
}

static int is_same_ratio(struct layer_config *ref, struct layer_config *c)
{
	int diff_w, diff_h;

	if (!ref->dst_width || !ref->dst_height) {
		DISP_PR_ERR("%s:ref dst(%dx%d)\n", __func__,
			    ref->dst_width, ref->dst_height);
		return -EINVAL;
	}

	diff_w = c->dst_width * ref->src_width / ref->dst_width - c->src_width;
	diff_h = c->dst_height * ref->src_height / ref->dst_height -
		 c->src_height;
	if (abs(diff_w) > 1 || abs(diff_h) > 1)
		return false;

	return true;
}

static bool is_RPO(struct disp_layer_info *disp_info, int disp_idx,
		   int *rsz_idx)
{
	int i = 0;
	struct layer_config *c = NULL;
	int gpu_rsz_idx = 0;
	struct layer_config *ref_layer = NULL;
	struct disp_rect src_layer_roi = { 0 };
	struct disp_rect dst_layer_roi = { 0 };
	struct disp_rect src_roi = { 0 };
	struct disp_rect dst_roi = { 0 };

	if (disp_info->layer_num[disp_idx] <= 0)
		return false;

	*rsz_idx = -1;
	for (i = 0; i < disp_info->layer_num[disp_idx] && i < 2; i++) {
		c = &disp_info->input_config[disp_idx][i];

		if (i == 0 && c->src_fmt == DISP_FORMAT_DIM)
			continue;

		if (disp_info->gles_head[disp_idx] >= 0 &&
		    disp_info->gles_head[disp_idx] <= i)
			break;

		if (!is_rsz_valid(c))
			break;

		if (!ref_layer)
			ref_layer = c;
		else if (is_same_ratio(ref_layer, c) <= 0)
			break;

		rect_make(&src_layer_roi,
			  c->dst_offset_x * c->src_width / c->dst_width,
			  c->dst_offset_y * c->src_height / c->dst_height,
			  c->src_width, c->src_height);
		rect_make(&dst_layer_roi,
			  c->dst_offset_x * c->src_width / c->dst_width,
			  c->dst_offset_y * c->src_height / c->dst_height,
			  c->dst_width, c->dst_height);
		rect_join(&src_layer_roi, &src_roi, &src_roi);
		rect_join(&dst_layer_roi, &dst_roi, &dst_roi);
		if (src_roi.width > dst_roi.width ||
		    src_roi.height > dst_roi.height) {
			DISP_PR_ERR("L%d:scale down(%d,%d,%dx%d)->(%d,%d,%dx%d)\n",
				    i, src_roi.x, src_roi.y, src_roi.width,
				    src_roi.height, dst_roi.x, dst_roi.y,
				    dst_roi.width, dst_roi.height);
			break;
		}

		if (src_roi.width > RSZ_TILE_LENGTH - RSZ_ALIGNMENT_MARGIN ||
		    src_roi.height > RSZ_IN_MAX_HEIGHT)
			break;

		c->layer_caps |= DISP_RSZ_LAYER;
		*rsz_idx = i;
	}
	if (*rsz_idx == -1)
		return false;
	if (*rsz_idx == 2) {
		DISP_PR_ERR("%s:error: rsz layer idx >= 2\n", __func__);
		return false;
	}

	for (i = *rsz_idx + 1; i < disp_info->layer_num[disp_idx]; i++) {
		c = &disp_info->input_config[disp_idx][i];
		if (c->src_width != c->dst_width ||
		    c->src_height != c->dst_height) {
			if (has_layer_cap(c, MDP_RSZ_LAYER))
				continue;

			gpu_rsz_idx = i;
			break;
		}
	}

	if (gpu_rsz_idx)
		rollback_resize_layer_to_GPU_range(disp_info, disp_idx,
			gpu_rsz_idx, disp_info->layer_num[disp_idx] - 1);

	return true;
}

/* lr_rsz_layout - layering rule resize layer layout */
static bool lr_rsz_layout(struct disp_layer_info *disp_info)
{
	int disp_idx;

	if (is_ext_path(disp_info))
		rollback_all_resize_layer_to_GPU(disp_info, HRT_SECONDARY);

	for (disp_idx = 0; disp_idx < 2; disp_idx++) {
		int rsz_idx = 0;

		if (disp_info->layer_num[disp_idx] <= 0)
			continue;

		/* only support resize layer on Primary Display */
		if (disp_idx == HRT_SECONDARY)
			continue;

		if (!has_rsz_layer(disp_info, disp_idx)) {
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		} else if (is_RPO(disp_info, disp_idx, &rsz_idx)) {
			if (rsz_idx == 0)
				l_rule_info.disp_path = HRT_PATH_RPO_L0;
			else if (rsz_idx == 1)
				l_rule_info.disp_path = HRT_PATH_RPO_L0L1;
			else
				DISP_PR_ERR("%s:RPO but rsz_idx(%d) error\n",
					    __func__, rsz_idx);
		} else {
			rollback_all_resize_layer_to_GPU(disp_info,
							 HRT_PRIMARY);
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		}
	}

	return 0;
}

static bool
lr_unset_disp_rsz_attr(struct disp_layer_info *disp_info, int disp_idx)
{
	struct layer_config *c = &disp_info->input_config[disp_idx][0];

	if (l_rule_info.disp_path == HRT_PATH_RPO_L0 &&
	    has_layer_cap(c, MDP_RSZ_LAYER) &&
	    has_layer_cap(c, DISP_RSZ_LAYER)) {
		c->layer_caps &= ~DISP_RSZ_LAYER;
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
		return true;
	}
	return false;
}

static void lr_gpu_change_rsz_info(void)
{
}

static void layering_rule_senario_decision(struct disp_layer_info *disp_info)
{
	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_START,
			 l_rule_info.disp_path, l_rule_info.layer_tb_idx |
			 (l_rule_info.bound_tb_idx << 16));

	if (GET_SYS_STATE(DISP_HRT_MULTI_TUI_ON)) {
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
	} else {
		if (l_rule_info.disp_path == HRT_PATH_RPO_L0) {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_RPO_L0;
		} else if (l_rule_info.disp_path == HRT_PATH_RPO_L0L1) {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_RPO_L0L1;
		} else {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
			l_rule_info.disp_path = HRT_PATH_GENERAL;
		}
	}

	l_rule_info.primary_fps = 60;

	if (primary_display_get_height() > 1920)
		l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_LP4_FHD_PLUS;
	else
		l_rule_info.bound_tb_idx = HRT_BOUND_TYPE_LP4;

	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_END,
			 l_rule_info.disp_path, l_rule_info.layer_tb_idx |
			 (l_rule_info.bound_tb_idx << 16));
}

static bool filter_by_hw_limitation(struct disp_layer_info *disp_info)
{
	return true;
}

static int get_hrt_bound(int is_larb, int hrt_level)
{
	if (is_larb)
		return larb_bound_table[l_rule_info.bound_tb_idx][hrt_level];

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
		break;
	}
	return NULL;
}

static int get_mapping_table(enum DISP_HW_MAPPING_TB_TYPE tb_type, int param)
{
	int layer_tb_idx = l_rule_info.layer_tb_idx;

	switch (tb_type) {
	case DISP_HW_OVL_TB:
		return ovl_mapping_table[layer_tb_idx];
	case DISP_HW_LARB_TB:
		return larb_mapping_table[layer_tb_idx];
	case DISP_HW_LAYER_TB:
		if (param < MAX_PHY_OVL_CNT && param >= 0)
			return layer_mapping_table[layer_tb_idx][param];
		else
			return -1;
	default:
		break;
	}
	return -1;
}

void layering_rule_init(void)
{
	l_rule_info.primary_fps = 60;
	register_layering_rule_ops(&l_rule_ops, &l_rule_info);

	set_layering_opt(LYE_OPT_RPO, disp_helper_get_option(DISP_OPT_RPO));
	set_layering_opt(LYE_OPT_EXT_LAYER,
			 disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER));
}

static bool _adaptive_dc_enabled(void)
{
#ifdef CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW
	/* RDMA does not support rotation */
	return false;
#endif

	if (disp_mgr_has_mem_session() ||
	    !disp_helper_get_option(DISP_OPT_DC_BY_HRT) || is_DAL_Enabled())
		return false;

	return true;
}

static bool _rollback_all_to_GPU_for_idle(void)
{
	if (disp_mgr_has_mem_session() ||
	    !disp_helper_get_option(DISP_OPT_IDLEMGR_BY_REPAINT) ||
	    !atomic_read(&idle_need_repaint)) {
		atomic_set(&idle_need_repaint, 0);
		return false;
	}

	atomic_set(&idle_need_repaint, 0);

	return true;
}

void update_layering_opt_by_disp_opt(enum DISP_HELPER_OPT opt, int value)
{
	switch (opt) {
	case DISP_OPT_OVL_EXT_LAYER:
		set_layering_opt(LYE_OPT_EXT_LAYER, value);
		break;
	case DISP_OPT_RPO:
		set_layering_opt(LYE_OPT_RPO, value);
		break;
	default:
		break;
	}
}

static struct layering_rule_ops l_rule_ops = {
	.resizing_rule = lr_rsz_layout,
	.rsz_by_gpu_info_change = lr_gpu_change_rsz_info,
	.scenario_decision = layering_rule_senario_decision,
	.get_bound_table = get_bound_table,
	.get_hrt_bound = get_hrt_bound,
	.get_mapping_table = get_mapping_table,
	.rollback_to_gpu_by_hw_limitation = filter_by_hw_limitation,
	.unset_disp_rsz_attr = lr_unset_disp_rsz_attr,
	.adaptive_dc_enabled = _adaptive_dc_enabled,
	.rollback_all_to_GPU_for_idle = _rollback_all_to_GPU_for_idle,
};
