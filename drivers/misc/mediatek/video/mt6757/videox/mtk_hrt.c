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

#include "mtk_hrt.h"

static int emi_extreme_lower_bound;
static int emi_lower_bound;
static int emi_upper_bound;
static int larb_lower_bound;
static int larb_upper_bound;
static int primary_max_input_layer_num;
static int secondary_max_input_layer_num;

static int primary_fps = 60;

static disp_layer_info disp_info_hrt;
static int dal_enable;
static hrt_sort_entry *x_entry_list, *y_entry_list;

static int get_bpp(DISP_FORMAT format)
{
	int layerbpp;

	switch (format) {
	case DISP_FORMAT_YUV422:
		layerbpp = 2;
		break;
	case DISP_FORMAT_RGB565:
		layerbpp = 2;
		break;
	case DISP_FORMAT_RGB888:
		layerbpp = 3;
		break;
	case DISP_FORMAT_BGR888:
		layerbpp = 3;
		break;
	case DISP_FORMAT_ARGB8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_ABGR8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_RGBA8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_BGRA8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_XRGB8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_XBGR8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_RGBX8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_BGRX8888:
		layerbpp = 4;
		break;
	case DISP_FORMAT_UYVY:
		layerbpp = 2;
		break;
	case DISP_FORMAT_DIM:
		layerbpp = 0;
		break;

	default:
		DISPERR("Invalid color format: 0x%x\n", format);
		return -1;
	}

	return layerbpp;
}

static void dump_disp_info(disp_layer_info *disp_info)
{
	int i, j;
	layer_config *layer_info;

	for (i = 0 ; i < 2 ; i++) {
		DISPMSG("HRT D%d/M%d/LN%d/hrt_num:%d/G(%d,%d)/fps:%d/dal:%d\n",
			i, disp_info->disp_mode[i], disp_info->layer_num[i], disp_info->hrt_num,
			disp_info->gles_head[i], disp_info->gles_tail[i], primary_fps, dal_enable);

		for (j = 0 ; j < disp_info->layer_num[i] ; j++) {
			layer_info = &disp_info->input_config[i][j];
			DISPMSG("L%d->%d/of(%d,%d)/wh(%d,%d)/fmt:0x%x/ext:%d\n",
				j, layer_info->ovl_id, layer_info->dst_offset_x,
				layer_info->dst_offset_y, layer_info->dst_width, layer_info->dst_height,
				layer_info->src_fmt, layer_info->ext_sel_layer);
		}
	}
}

static void print_disp_info_to_log_buffer(disp_layer_info *disp_info)
{
	char *status_buf;
	int i, j, n;
	layer_config *layer_info;

	status_buf = get_dprec_status_ptr(0);
	if (status_buf == NULL)
		return;

	n = 0;
	n += snprintf(status_buf + n, LOGGER_BUFFER_SIZE - n,
		"Last hrt query data[start]\n");
	for (i = 0 ; i < 2 ; i++) {
		n += snprintf(status_buf + n, LOGGER_BUFFER_SIZE - n,
			"HRT D%d/M%d/LN%d/hrt_num:%d/G(%d,%d)/fps:%d\n",
			i, disp_info->disp_mode[i], disp_info->layer_num[i], disp_info->hrt_num,
			disp_info->gles_head[i], disp_info->gles_tail[i], primary_fps);

		for (j = 0 ; j < disp_info->layer_num[i] ; j++) {
			layer_info = &disp_info->input_config[i][j];
			n += snprintf(status_buf + n, LOGGER_BUFFER_SIZE - n,
				"L%d->%d/of(%d,%d)/wh(%d,%d)/fmt:0x%x\n",
				j, layer_info->ovl_id, layer_info->dst_offset_x,
				layer_info->dst_offset_y, layer_info->dst_width, layer_info->dst_height,
				layer_info->src_fmt);
		}
	}
	n += snprintf(status_buf + n, LOGGER_BUFFER_SIZE - n,
		"Last hrt query data[end]\n");

}

/**
 * Roll back all layers that overcome OVL HW capabilities to GPU
 *
 * \param disp_info all frame layers layout information
 */
static int filter_by_ovl_cnt(disp_layer_info *disp_info)
{
	int ovl_num_limit, disp_index, curr_ovl_num, available_ovl_num;
	int i;

	/* 0->primary display, 1->secondary display */
	for (disp_index = 0 ; disp_index < 2 ; disp_index++) {
		/* No need to considerate HRT in decouple mode */
		if (disp_info->disp_mode[disp_index] == DISP_SESSION_DECOUPLE_MIRROR_MODE ||
				disp_info->disp_mode[disp_index] == DISP_SESSION_DECOUPLE_MODE)
			continue;

		if (disp_index == 0)
			if (dal_enable)
				ovl_num_limit = primary_max_input_layer_num - 1;
			else
				ovl_num_limit = primary_max_input_layer_num;
		else
			ovl_num_limit = secondary_max_input_layer_num;

		if (disp_info->layer_num[disp_index] <= ovl_num_limit)
			continue;

		curr_ovl_num = disp_info->layer_num[disp_index] - (disp_info->gles_tail[disp_index] -
			disp_info->gles_head[disp_index]);
		/** reserve 1 layer for FBT if needed */
		available_ovl_num = (disp_info->gles_head[disp_index] == -1) ? ovl_num_limit : ovl_num_limit - 1;

		if (curr_ovl_num <= available_ovl_num)
			continue;

		curr_ovl_num = 0;
		for (i = 0 ; i < disp_info->layer_num[disp_index] ; i++) {
			/** OVL HW capability is overflowed */
			if (disp_info->gles_head[disp_index] == -1) {
				disp_info->gles_head[disp_index] = ovl_num_limit - 1;
				disp_info->gles_tail[disp_index] = disp_info->layer_num[disp_index] - 1;
			} else {
				if (disp_info->gles_head[disp_index] > ovl_num_limit) {
					disp_info->gles_head[disp_index] = ovl_num_limit - 1;
					disp_info->gles_tail[disp_index] = disp_info->layer_num[disp_index] - 1;
				} else {
					/* gles_tail = total_layer_num - ovl_num_limit + gles_head */
					int tmp_tail = 0;
					tmp_tail = disp_info->layer_num[disp_index] - ovl_num_limit +
						disp_info->gles_head[disp_index];
					if (tmp_tail > disp_info->gles_tail[disp_index])
						disp_info->gles_tail[disp_index] = tmp_tail;
				}
			}

		}
	}

#ifdef HRT_DEBUG
	DISPMSG("[%s result]\n", __func__);
	dump_disp_info(disp_info);
#endif
	return 0;
}

int dump_entry_list(bool sort_by_y)
{
	hrt_sort_entry *temp;
	layer_config *layer_info;

	if (sort_by_y)
		temp = y_entry_list;
	else
		temp = x_entry_list;

	DISPMSG("%s, sort_by_y:%d, addr:0x%p\n", __func__, sort_by_y, temp);
	while (temp != NULL) {
			layer_info = temp->layer_info;
			DISPMSG("key:%d, offset(%d, %d), w/h(%d, %d), overlap_w:%d\n",
					temp->key, layer_info->dst_offset_x, layer_info->dst_offset_y,
					layer_info->dst_width, layer_info->dst_height, temp->overlap_w);
		temp = temp->tail;
	}
	DISPMSG("dump_entry_list end\n");
	return 0;
}

static int insert_entry(hrt_sort_entry **head, hrt_sort_entry *sort_entry)
{
	hrt_sort_entry *temp;
	temp = *head;
	while (temp != NULL) {
		if (sort_entry->key < temp->key ||
				((sort_entry->key == temp->key) && (sort_entry->overlap_w > 0))) {
			sort_entry->head = temp->head;
			sort_entry->tail = temp;
			if (temp->head != NULL)
				temp->head->tail = sort_entry;
			else
				*head = sort_entry;
			temp->head = sort_entry;
			break;
		}
		if (temp->tail == NULL) {
			temp->tail = sort_entry;
			sort_entry->head = temp;
			sort_entry->tail = NULL;
			break;
		}
		temp = temp->tail;
	}

	return 0;
}

static int add_layer_entry(layer_config *layer_info, bool sort_by_y, int overlay_w)
{
	hrt_sort_entry *begin_t, *end_t;
	hrt_sort_entry **p_entry;

	begin_t = kzalloc(sizeof(hrt_sort_entry), GFP_KERNEL);
	end_t = kzalloc(sizeof(hrt_sort_entry), GFP_KERNEL);

	begin_t->head = NULL;
	begin_t->tail = NULL;
	end_t->head = NULL;
	end_t->tail = NULL;

	if (sort_by_y) {
		begin_t->key = layer_info->dst_offset_y;
		end_t->key = layer_info->dst_offset_y + layer_info->dst_height;
		p_entry = &y_entry_list;
	} else {
		begin_t->key = layer_info->dst_offset_x;
		end_t->key = layer_info->dst_offset_x + layer_info->dst_width;
		p_entry = &x_entry_list;
	}

	begin_t->overlap_w = overlay_w;
	begin_t->layer_info = layer_info;
	end_t->overlap_w = -overlay_w;
	end_t->layer_info = layer_info;

	if (*p_entry == NULL) {
		*p_entry = begin_t;
		begin_t->head = NULL;
		begin_t->tail = end_t;
		end_t->head = begin_t;
		end_t->tail = NULL;
	} else {
	/* Inser begin entry */
		insert_entry(p_entry, begin_t);
#ifdef HRT_DEBUG
	DISPMSG("Insert key:%d\n", begin_t->key);
	dump_entry_list(sort_by_y);
#endif
	/* Inser end entry */
		insert_entry(p_entry, end_t);
#ifdef HRT_DEBUG
	DISPMSG("Insert key:%d\n", end_t->key);
	dump_entry_list(sort_by_y);
#endif
	}

	return 0;
}

static int remove_layer_entry(layer_config *layer_info, bool sort_by_y)
{
	hrt_sort_entry *temp, *free_entry;

	if (sort_by_y)
		temp = y_entry_list;
	else
		temp = x_entry_list;

	while (temp != NULL) {
		if (temp->layer_info == layer_info) {
			free_entry = temp;
			temp = temp->tail;
			if (free_entry->head == NULL) {
				if (temp != NULL)
					temp->head = NULL;
				if (sort_by_y)
					y_entry_list = temp;
				else
					x_entry_list = temp;
				kfree(free_entry);
			} else {
				free_entry->head->tail = free_entry->tail;
				if (temp)
					temp->head = free_entry->head;
				kfree(free_entry);
			}
		} else {
			temp = temp->tail;
		}
	}
	return 0;
}

static int free_all_layer_entry(bool sort_by_y)
{
	hrt_sort_entry *cur_entry, *next_entry;

	if (sort_by_y)
		cur_entry = y_entry_list;
	else
		cur_entry = x_entry_list;

	while (cur_entry != NULL) {
		next_entry = cur_entry->tail;
		kfree(cur_entry);
		cur_entry = next_entry;
		}
	if (sort_by_y)
		y_entry_list = NULL;
	else
		x_entry_list = NULL;

	return 0;
}

static int scan_x_overlap(disp_layer_info *disp_info, int disp_index, int ovl_overlap_limit_w)
{
	hrt_sort_entry *tmp_entry;
	int overlap_w_sum, max_overlap;
	overlap_w_sum = 0;
	max_overlap = 0;
		tmp_entry = x_entry_list;
	while (tmp_entry != NULL) {
		overlap_w_sum += tmp_entry->overlap_w;
		max_overlap = (overlap_w_sum > max_overlap) ? overlap_w_sum : max_overlap;
		tmp_entry = tmp_entry->tail;
	}
	return max_overlap;
}

static int scan_y_overlap(disp_layer_info *disp_info, int disp_index, int ovl_overlap_limit_w)
{
	hrt_sort_entry *tmp_entry;
	int overlap_w_sum, tmp_overlap, max_overlap;

	overlap_w_sum = 0;
	tmp_overlap = 0;
	max_overlap = 0;
	tmp_entry = y_entry_list;
	while (tmp_entry != NULL) {
		overlap_w_sum += tmp_entry->overlap_w;
		if (tmp_entry->overlap_w > 0)
			add_layer_entry(tmp_entry->layer_info, false, tmp_entry->overlap_w);
		else
			remove_layer_entry(tmp_entry->layer_info, false);

		if (overlap_w_sum > ovl_overlap_limit_w && overlap_w_sum > max_overlap)
			tmp_overlap = scan_x_overlap(disp_info, disp_index, ovl_overlap_limit_w);
		else
			tmp_overlap = overlap_w_sum;

		max_overlap = (tmp_overlap > max_overlap) ? tmp_overlap : max_overlap;
		tmp_entry = tmp_entry->tail;
	}

	return max_overlap;
}
static int get_hrt_level(int sum_overlap_w, int is_larb)
{
	if (is_larb) {
		if (sum_overlap_w <= larb_lower_bound * BYTES_PER_SECOND_FULLSCREEN)
			return HRT_LEVEL_LOW;
		else if (sum_overlap_w <= larb_upper_bound * BYTES_PER_SECOND_FULLSCREEN)
			return HRT_LEVEL_HIGH;
		else
			return HRT_OVER_LIMIT;
	} else {
		if (sum_overlap_w <= emi_extreme_lower_bound * BYTES_PER_SECOND_FULLSCREEN && primary_fps == 60)
			return HRT_LEVEL_EXTREME_LOW;

		if (sum_overlap_w <= emi_lower_bound * BYTES_PER_SECOND_FULLSCREEN)
			return HRT_LEVEL_LOW;
		else if (sum_overlap_w <= emi_upper_bound * BYTES_PER_SECOND_FULLSCREEN)
			return HRT_LEVEL_HIGH;
		else
			return HRT_OVER_LIMIT;
	}

	return HRT_LEVEL_HIGH;
}

static int get_ovl_layer_cnt(disp_layer_info *disp_info, int disp_idx)
{
	int total_cnt = 0;

	if (disp_info->layer_num[disp_idx] != -1) {
		total_cnt += disp_info->layer_num[disp_idx];
		/** OVL layer = total layer - GPU layer */
		if (disp_info->gles_head[disp_idx] >= 0)
			total_cnt -= disp_info->gles_tail[disp_idx] - disp_info->gles_head[disp_idx];
	}
	return total_cnt;
}

static bool has_hrt_limit(disp_layer_info *disp_info, int disp_idx)
{
	if (disp_info->layer_num[disp_idx] <= 0 ||
		disp_info->disp_mode[disp_idx] == DISP_SESSION_DECOUPLE_MIRROR_MODE ||
		disp_info->disp_mode[disp_idx] == DISP_SESSION_DECOUPLE_MODE)
		return false;

	return true;
}

/**
 * Get modulated fps
 *   for primary display, just return 60
 *   for HDMI Display (Everest)
 *     2160x3840@30fps  --> 120fps
 *     1440x2560@60fps  --> 60fps
 *     1080x1920@120fps --> 60fps
 *     1080x1920@60fps  --> 30fps
 * \todo Maybe step down one level for Olympus?
 */
static int get_layer_weight(int disp_idx)
{
#ifdef CONFIG_MTK_HDMI_SUPPORT
	if (disp_idx == HRT_SECONDARY) {
		int weight = 0;
		disp_session_info dispif_info;

		/* For seconary display, set the wight 4K@30 as 2K@60.	*/
		hdmi_get_dev_info(true, &dispif_info);

		if (dispif_info.displayWidth >= 2160)
			weight = 120;
		else if (dispif_info.displayWidth >= 1440)
			weight = 60;
		else if (dispif_info.displayWidth >= 1080)
			weight = 30;
		else
			weight = 30;

		if (dispif_info.vsyncFPS <= 30)
			weight /= 2;

		return weight;
	}
#endif
	return 60;
}


static int _calc_hrt_num(disp_layer_info *disp_info, int disp_index,
				int start_layer, int end_layer, bool force_scan_y, bool has_dal_layer)
{
	int i, bpp, sum_overlap_w, overlap_lower_bound, overlay_w, weight;
	bool has_gles = false;
	layer_config *layer_info;

	if (start_layer > end_layer || end_layer >= disp_info->layer_num[disp_index]) {
		DISPERR("%s input layer index incorrect, start:%d, end:%d\n",
			__func__, start_layer, end_layer);
		dump_disp_info(disp_info);
	}

	/* 1.Initial overlap conditions. */
	/* modulate fps */
	weight = get_layer_weight(disp_index);

	/* total bytes per second */
	sum_overlap_w = 0;
	overlap_lower_bound = emi_lower_bound * BYTES_PER_SECOND_FULLSCREEN;

	/* To check if has GPU layer for input layers */
	if (disp_info->gles_head[disp_index] != -1 &&
		disp_info->gles_head[disp_index] >= start_layer &&
		disp_info->gles_head[disp_index] <= end_layer)
		has_gles = true;

	/*
	2.Add each layer info to layer list and sort it by yoffset.
	Also add up each layer overlap weight.
	*/
	for (i = start_layer ; i <= end_layer ; i++) {
		layer_info = &disp_info->input_config[disp_index][i];

		if (!has_gles ||
			(i < disp_info->gles_head[disp_index] ||
			i > disp_info->gles_tail[disp_index])) {
			bpp = get_bpp(layer_info->src_fmt);
			overlay_w = bpp * weight;
			sum_overlap_w += overlay_w;
			add_layer_entry(layer_info, true, overlay_w);
		}
	}
	/* Add overlap weight of Gles layer and Assert layer. */
	if (has_gles)
		sum_overlap_w += (4 * weight);
	if (has_dal_layer)
		sum_overlap_w += 120;

	/*
	3.Calculate the HRT bound if the total layer weight over the lower bound
	or has secondary display.
	*/
	if (sum_overlap_w > overlap_lower_bound ||
		has_hrt_limit(disp_info, HRT_SECONDARY) ||
		force_scan_y) {
		sum_overlap_w = scan_y_overlap(disp_info, disp_index, overlap_lower_bound);
		/* Add overlap weight of Gles layer and Assert layer. */
		if (has_gles)
			sum_overlap_w += (4 * weight);
		if (has_dal_layer)
			sum_overlap_w += 120;
	}

#ifdef HRT_DEBUG
	DISPMSG("%s disp_index:%d, start_layer:%d, end_layer:%d, sum_overlap_w:%d\n",
		__func__, disp_index, start_layer, end_layer, sum_overlap_w);
#endif

	free_all_layer_entry(true);
	return sum_overlap_w;
}

static int _get_larb0_idx(disp_layer_info *disp_info)
{
	int primary_ovl_cnt = 0, larb_idx = 0;

	primary_ovl_cnt = get_ovl_layer_cnt(disp_info, HRT_PRIMARY);

	if (primary_fps == 120) {
		if (primary_ovl_cnt < 3)
			larb_idx = primary_ovl_cnt - 1;
		else if (primary_ovl_cnt < 5)
			larb_idx = 1;
		else if (primary_ovl_cnt < 7)
			larb_idx = 2;
		else
			larb_idx = 3;
	} else {
		if (primary_ovl_cnt < 4)
			larb_idx = primary_ovl_cnt - 1;
		else if (primary_ovl_cnt < 7)
			larb_idx = 2;
		else
			larb_idx = 3;
	}
	if (disp_info->gles_head[0] != -1 &&
		larb_idx > disp_info->gles_head[0])
		larb_idx += (disp_info->gles_tail[0] - disp_info->gles_head[0]);

	return larb_idx;
}

static bool _calc_larb0(disp_layer_info *disp_info, int emi_hrt_w)
{
	int larb_idx = 0, sum_overlap_w = 0;
	bool is_over_bound = true;

	if (!has_hrt_limit(disp_info, HRT_PRIMARY)) {
		is_over_bound = false;
		return is_over_bound;
	}

	larb_idx = _get_larb0_idx(disp_info);
	sum_overlap_w = _calc_hrt_num(disp_info, 0, 0, larb_idx, false, false);

	if (get_hrt_level(sum_overlap_w, true) > HRT_LEVEL_LOW)
		is_over_bound = true;
	else
		is_over_bound = false;

	return is_over_bound;
}

static bool _calc_larb5(disp_layer_info *disp_info, int emi_hrt_w)
{
	int primary_ovl_cnt = 0, larb5_idx = 0, sum_overlap_w = 0;
	bool is_over_bound = true;

	primary_ovl_cnt = get_ovl_layer_cnt(disp_info, HRT_PRIMARY);
	if (primary_ovl_cnt > 3 && has_hrt_limit(disp_info, HRT_PRIMARY)) {

		larb5_idx = _get_larb0_idx(disp_info) + 1;
		sum_overlap_w += _calc_hrt_num(disp_info, HRT_PRIMARY,
					larb5_idx, disp_info->layer_num[0] - 1, false, dal_enable);

	}

	if (has_hrt_limit(disp_info, HRT_SECONDARY)) {
		sum_overlap_w += _calc_hrt_num(disp_info, HRT_SECONDARY,
					0, disp_info->layer_num[1] - 1, false, false);
	}

	if (get_hrt_level(sum_overlap_w, true) > HRT_LEVEL_LOW)
		is_over_bound = true;
	else
		is_over_bound = false;

	return is_over_bound;
}

static bool calc_larb_hrt(disp_layer_info *disp_info, int emi_hrt_w)
{
	bool is_over_bound = true;

	is_over_bound = _calc_larb0(disp_info, emi_hrt_w);
	if (!is_over_bound)
		is_over_bound = _calc_larb5(disp_info, emi_hrt_w);

	return is_over_bound;
}

static int calc_hrt_num(disp_layer_info *disp_info)
{
	int hrt_level = HRT_OVER_LIMIT;
	int sum_overlay_w = 0;

	/* Calculate HRT for EMI level */
	if (has_hrt_limit(disp_info, HRT_PRIMARY))
		sum_overlay_w = _calc_hrt_num(disp_info, 0, 0, disp_info->layer_num[0] - 1,
					false, dal_enable);
	if (has_hrt_limit(disp_info, HRT_SECONDARY))
		sum_overlay_w += _calc_hrt_num(disp_info, 1, 0, disp_info->layer_num[1] - 1,
					false, false);

	hrt_level = get_hrt_level(sum_overlay_w, false);
	/*
	The larb bound always meet the limit for HRT_LEVEL_LOW in 8+4 ovl architecture.
	So calculate larb bound only for HRT_LEVEL_LOW.
	*/
	disp_info->hrt_num = hrt_level;
#ifdef HRT_DEBUG
	DISPMSG("EMI hrt level:%d\n", hrt_level);
#endif

	/* Need to calculate larb hrt for HRT_LEVEL_LOW level. */
	if (hrt_level != HRT_LEVEL_LOW)
		return hrt_level;

	/* Check Larb Bound here */
	if (calc_larb_hrt(disp_info, sum_overlay_w))
		hrt_level = HRT_LEVEL_HIGH;
	else
		hrt_level = HRT_LEVEL_LOW;
	disp_info->hrt_num = hrt_level;

#ifdef HRT_DEBUG
	DISPMSG("Larb hrt level:%d\n", hrt_level);
#endif
	return hrt_level;
}

static int is_overlap_on_yaxis(layer_config *lhs, layer_config *rhs)
{
	if ((lhs->dst_offset_y + lhs->dst_height < rhs->dst_offset_y) ||
			(rhs->dst_offset_y + rhs->dst_height < lhs->dst_offset_y))
		return 0;
	return 1;
}

/**
 * check if continuous ext layers is overlapped with each other
 * also need to check the below nearest phy layer which these ext layers will be attached to
 * 1. check all ext layers, if overlapped with any one, change it to phy layer
 * 2. if more than 1 ext layer exist, need to check the phy layer
 */
static int is_continuous_ext_layer_overlap(layer_config *configs, int curr)
{
	int overlapped, need_check_phy_layer = 0;
	layer_config *src_info, *dst_info;
	int i;

	overlapped = 0;
	dst_info = &configs[curr];
	for (i = curr-1; i > 0; i--) {
		src_info = &configs[i];
		if (src_info->ext_sel_layer != -1) {
			overlapped |= is_overlap_on_yaxis(src_info, dst_info);
			if (overlapped)
				break;
			need_check_phy_layer = 1;
		} else {
			if (need_check_phy_layer)
				overlapped |= is_overlap_on_yaxis(src_info, dst_info);
			else
				break;
		}
	}
	return overlapped;
}

/**
 * dispatch which one layer could be ext layer
 *
 * find all subset of un-overlapped layers, mark the z-order smallest one as phy layer
 * and others as ext layer that will be attached to the first phy layer.
 *
 * \note
 * Just do this after HRT and layer layout is ready, because ext layer DO NOT
 * affect HRT overlapped layer number calculations
 *
 * PREREQUISITES:
 * 1. if more than 2 continuous layers are non-overlap, every two of them must be non-overlap
 * 2. MUST NOT exceed the HW ext layers capability of each OVL HW
 * 3. MUST BE called after HRT calculations and dispatch_ovl_id(), to make sure that
 *    a). only 1 fbt layer exist if has GPU layers (layers with the same layer id)
 *    b). total layer num do not exceed hw capabilities
 *
 * \todo
 * 1. consider SIM LARB layout
 * 2. affect the max ovl layer number returned to HWC
 */
static int dispatch_ext_layer(disp_layer_info *disp_info)
{
	int ext_layer_num_on_OVL, ext_layer_num_on_OVL_2L;
	int phy_layer_num_on_OVL, phy_layer_num_on_OVL_2L;
	int hw_layer_num;	/* tmp variable */
	int is_on_OVL, is_ext_layer;
	int disp_idx, i;
	layer_config *src_info, *dst_info;

	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {
		ext_layer_num_on_OVL = ext_layer_num_on_OVL_2L = 0;
		phy_layer_num_on_OVL = 1;
		phy_layer_num_on_OVL_2L = 0;
		is_on_OVL = 1;

		/** initialize ext layer info */
		for (i = 0 ; i < disp_info->layer_num[disp_idx]; i++)
			disp_info->input_config[disp_idx][i].ext_sel_layer = -1;

		for (i = 1 ; i < disp_info->layer_num[disp_idx]; i++) {
			dst_info = &disp_info->input_config[disp_idx][i];
			src_info = &disp_info->input_config[disp_idx][i-1];
			/** skip other GPU layers */
			if (dst_info->ovl_id == src_info->ovl_id)
				continue;

			is_ext_layer = !is_overlap_on_yaxis(src_info, dst_info);

			is_ext_layer &= !is_continuous_ext_layer_overlap(dst_info, i);
			/**
			 * update current ext_layer_num and phy_layer_num
			 */
			if (is_ext_layer) {
				if (is_on_OVL)
					++ext_layer_num_on_OVL;
				else
					++ext_layer_num_on_OVL_2L;
			} else {
				if (is_on_OVL)
					++phy_layer_num_on_OVL;
				else
					++phy_layer_num_on_OVL_2L;
			}
			/**
			 * If OVL is full, put the current phy layer onto OVL_2L
			 */
			hw_layer_num = (disp_idx == 0) ? PRIMARY_HW_OVL_LAYER_NUM : EXTERNAL_HW_OVL_LAYER_NUM;
			if (is_on_OVL && (phy_layer_num_on_OVL > hw_layer_num)) {
				is_on_OVL = 0;
				++phy_layer_num_on_OVL_2L;
			}
			/**
			 * If ext layer num exceed HW capability on OVL, then put the current ext layer
			 * on OVL_2L and change it to phy layer
			 */
			if (is_on_OVL && (ext_layer_num_on_OVL > DISP_HW_OVL_EXT_LAYER_NUM)) {
				is_on_OVL = 0;
				is_ext_layer = 0;
				++phy_layer_num_on_OVL_2L;
			}

			/** DO NOT need to check phy layer num on OVL_2L */

			/**
			 * If the ext layer num exceed HW capability on OVL_2L, then change it to phy layer
			 */
			if (!is_on_OVL && (ext_layer_num_on_OVL_2L > DISP_HW_OVL_EXT_LAYER_NUM)) {
				is_ext_layer = 0;
				++phy_layer_num_on_OVL_2L;
			}
			if (is_ext_layer) {
				if (is_on_OVL)
					dst_info->ext_sel_layer = phy_layer_num_on_OVL - 1;
				else
					dst_info->ext_sel_layer = phy_layer_num_on_OVL_2L - 1;
			}
		}
	}

	return 0;
}

static int dispatch_ovl_id(disp_layer_info *disp_info)
{
	int disp_idx, i;
	layer_config *layer_info;

	/* Dispatch gles range if necessary */
	if (disp_info->hrt_num > HRT_LEVEL_HIGH) {
		int valid_ovl_cnt = emi_upper_bound;

		if (dal_enable)
			valid_ovl_cnt = emi_upper_bound - 1;

		/*
		Arrange 4 ovl layers to secondary display, so no need to
		redistribute gles layer since it's already meet the larb
		limit.
		*/
		if (has_hrt_limit(disp_info, HRT_SECONDARY))
			valid_ovl_cnt -= get_ovl_layer_cnt(disp_info, HRT_SECONDARY);

		if (has_hrt_limit(disp_info, HRT_PRIMARY)) {
			/** put all overflow with big z-order to gles */
			if (disp_info->gles_head[HRT_PRIMARY] == -1) {
				disp_info->gles_head[HRT_PRIMARY] = valid_ovl_cnt - 1;
				disp_info->gles_tail[HRT_PRIMARY] = disp_info->layer_num[HRT_PRIMARY] - 1;
			} else {
				/** overflow is below gles head, just extend it to include all */
				if (disp_info->gles_head[HRT_PRIMARY] > valid_ovl_cnt) {
					disp_info->gles_head[HRT_PRIMARY] = valid_ovl_cnt;
					disp_info->gles_tail[HRT_PRIMARY] = disp_info->layer_num[HRT_PRIMARY] - 1;
				} else {
					/* gles_tail = total_layer_num - ovl_num_limit + gles_head */
					int tmp_tail = 0;

					tmp_tail = disp_info->layer_num[HRT_PRIMARY] - valid_ovl_cnt +
						disp_info->gles_head[HRT_PRIMARY];
					if (tmp_tail > disp_info->gles_tail[HRT_PRIMARY])
						disp_info->gles_tail[HRT_PRIMARY] = tmp_tail;
				}
			}
		}
		disp_info->hrt_num = HRT_LEVEL_HIGH;
	}

	/* Dispatch OVL id */
	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {
		int gles_count, ovl_cnt;

		ovl_cnt = get_ovl_layer_cnt(disp_info, disp_idx);

		gles_count = disp_info->gles_tail[disp_idx] - disp_info->gles_head[disp_idx] + 1;

		for (i = 0 ; i < disp_info->layer_num[disp_idx] ; i++) {
			layer_info = &disp_info->input_config[disp_idx][i];

			if (i < disp_info->gles_head[disp_idx])
				layer_info->ovl_id = i;
			else if (i > disp_info->gles_tail[disp_idx])
				layer_info->ovl_id = i - gles_count + 1;
			else {
				/** all GPU layer has the same layer id, a.k.a. fbt */
				layer_info->ovl_id = disp_info->gles_head[disp_idx];

				/**
				 * reset layer info for calculating ext layer
				 * \warning Expect HWC do not use this info any more
				 */
				layer_info->dst_offset_x = layer_info->dst_offset_y = 0;
				layer_info->dst_width = DISP_GetScreenWidth();
				layer_info->dst_height = DISP_GetScreenHeight();
			}
		}
	}
	return 0;
}

int check_disp_info(disp_layer_info *disp_info)
{
	int disp_idx;

	if (disp_info == NULL) {
		DISPERR("[HRT]disp_info is empty\n");
		return -1;
	}

	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {

		if (disp_info->layer_num[disp_idx] > 0 &&
			disp_info->input_config[disp_idx] == NULL) {
			DISPERR("[HRT]Has input layer, but input config is empty, disp_idx:%d, layer_num:%d\n",
				disp_idx, disp_info->layer_num[disp_idx]);
			return -1;
		}

		if ((disp_info->gles_head[disp_idx] < 0 && disp_info->gles_tail[disp_idx] >= 0) ||
			(disp_info->gles_tail[disp_idx] < 0 && disp_info->gles_head[disp_idx] >= 0)) {
			DISPERR("[HRT]gles layer invalid, disp_idx:%d, head:%d, tail:%d\n",
				disp_idx, disp_info->gles_head[disp_idx], disp_info->gles_tail[disp_idx]);
			return -1;
		}
	}

	return 0;
}


int gen_hrt_pattern(void)
{
#ifdef HRT_DEBUG
	disp_layer_info disp_info;
	layer_config *layer_info;
	int i;

	/* Primary Display */
	disp_info.disp_mode[0] = DISP_SESSION_DIRECT_LINK_MODE;
	disp_info.layer_num[0] = 7;
	disp_info.gles_head[0] = -1;
	disp_info.gles_tail[0] = -1;
	disp_info.input_config[0] = kzalloc(sizeof(layer_config) * 10, GFP_KERNEL);
	layer_info = disp_info.input_config[0];
	for (i = 0 ; i < disp_info.layer_num[0] ; i++)
		layer_info[i].src_fmt = DISP_FORMAT_ARGB8888;

	layer_info = disp_info.input_config[0];
	layer_info[0].dst_offset_x = 0;
	layer_info[0].dst_offset_y = 0;
	layer_info[0].dst_width = 1080;
	layer_info[0].dst_height = 1920;
	layer_info[1].dst_offset_x = 0;
	layer_info[1].dst_offset_y = 0;
	layer_info[1].dst_width = 1080;
	layer_info[1].dst_height = 1920;
	layer_info[2].dst_offset_x = 269;
	layer_info[2].dst_offset_y = 72;
	layer_info[2].dst_width = 657;
	layer_info[2].dst_height = 612;
	layer_info[3].dst_offset_x = 0;
	layer_info[3].dst_offset_y = 0;
	layer_info[3].dst_width = 1080;
	layer_info[3].dst_height = 72;
	layer_info[4].dst_offset_x = 1079;
	layer_info[4].dst_offset_y = 72;
	layer_info[4].dst_width = 1;
	layer_info[4].dst_height = 1704;
	layer_info[5].dst_offset_x = 0;
	layer_info[5].dst_offset_y = 1776;
	layer_info[5].dst_width = 1080;
	layer_info[5].dst_height = 144;
	layer_info[6].dst_offset_x = 0;
	layer_info[6].dst_offset_y = 72;
	layer_info[6].dst_width = 1080;
	layer_info[6].dst_height = 113;
	layer_info[7].dst_offset_x = 0;
	layer_info[7].dst_offset_y = 72;
	layer_info[7].dst_width = 1080;
	layer_info[7].dst_height = 113;
	layer_info[8].dst_offset_x = 0;
	layer_info[8].dst_offset_y = 550;
	layer_info[8].dst_width = 1080;
	layer_info[8].dst_height = 100;
	layer_info[9].dst_offset_x = 0;
	layer_info[9].dst_offset_y = 550;
	layer_info[9].dst_width = 1080;
	layer_info[9].dst_height = 100;

	/* Secondary Display */
	disp_info.disp_mode[1] = DISP_SESSION_DIRECT_LINK_MODE;
	disp_info.layer_num[1] = 0;
	disp_info.gles_head[1] = -1;
	disp_info.gles_tail[1] = -1;

	disp_info.input_config[1] = kzalloc(sizeof(layer_config) * 10, GFP_KERNEL);
	layer_info = disp_info.input_config[1];
	for (i = 0 ; i < disp_info.layer_num[1] ; i++)
		layer_info[i].src_fmt = DISP_FORMAT_ARGB8888;

	layer_info = disp_info.input_config[1];
	layer_info[0].dst_offset_x = 0;
	layer_info[0].dst_offset_y = 0;
	layer_info[0].dst_width = 1080;
	layer_info[0].dst_height = 1920;
	layer_info[1].dst_offset_x = 0;
	layer_info[1].dst_offset_y = 0;
	layer_info[1].dst_width = 1080;
	layer_info[1].dst_height = 100;
	layer_info[2].dst_offset_x = 0;
	layer_info[2].dst_offset_y = 1820;
	layer_info[2].dst_width = 1080;
	layer_info[2].dst_height = 100;
	layer_info[3].dst_offset_x = 0;
	layer_info[3].dst_offset_y = 500;
	layer_info[3].dst_width = 1080;
	layer_info[3].dst_height = 100;
	layer_info[4].dst_offset_x = 0;
	layer_info[4].dst_offset_y = 500;
	layer_info[4].dst_width = 1080;
	layer_info[4].dst_height = 100;
	layer_info[5].dst_offset_x = 550;
	layer_info[5].dst_offset_y = 0;
	layer_info[5].dst_width = 100;
	layer_info[5].dst_height = 1920;
	layer_info[6].dst_offset_x = 0;
	layer_info[6].dst_offset_y = 550;
	layer_info[6].dst_width = 1080;
	layer_info[6].dst_height = 100;
	layer_info[7].dst_offset_x = 0;
	layer_info[7].dst_offset_y = 550;
	layer_info[7].dst_width = 1080;
	layer_info[7].dst_height = 100;
	layer_info[8].dst_offset_x = 0;
	layer_info[8].dst_offset_y = 550;
	layer_info[8].dst_width = 1080;
	layer_info[8].dst_height = 100;
	layer_info[9].dst_offset_x = 0;
	layer_info[9].dst_offset_y = 550;
	layer_info[9].dst_width = 1080;
	layer_info[9].dst_height = 100;

	dispsys_hrt_calc(&disp_info);

	DISPMSG("free test pattern\n");
	kfree(disp_info.input_config[0]);
	kfree(disp_info.input_config[1]);
	msleep(50);
#endif
	return 0;
}

static int set_hrt_bound(void)
{
	if (primary_display_get_lcm_refresh_rate() == 120) {
		emi_lower_bound = OD_EMI_LOWER_BOUND;
		emi_upper_bound = OD_EMI_UPPER_BOUND;
		larb_lower_bound = OD_LARB_LOWER_BOUND;
		larb_upper_bound = OD_LARB_UPPER_BOUND;
#ifdef HRT_DEBUG
		DISPMSG("120hz hrt bound\n");
#endif
		return 120;
	}

	emi_lower_bound = EMI_LOWER_BOUND;
	emi_upper_bound = EMI_UPPER_BOUND;
	larb_lower_bound = LARB_LOWER_BOUND;
	larb_upper_bound = LARB_UPPER_BOUND;
	if (primary_display_get_width() > 1080)
		emi_extreme_lower_bound = EMI_EXTREME_LOWER_BOUND;
	else
		emi_extreme_lower_bound = EMI_EXTREME_LOWER_BOUND + 1;

	dal_enable = is_DAL_Enabled();

	if (disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER)) {
		primary_max_input_layer_num = PRIMARY_HW_OVL_LAYER_NUM + PRIMARY_HW_OVL_2L_LAYER_NUM +
			DISP_HW_OVL_EXT_LAYER_NUM * 2;
#if 0
		/* external and memory session no need more than 4 layers now */
		secondary_max_input_layer_num = EXTERNAL_HW_OVL_LAYER_NUM + EXTERNAL_HW_OVL_2L_LAYER_NUM +
			DISP_HW_OVL_EXT_LAYER_NUM * 2;
#else
		secondary_max_input_layer_num = EXTERNAL_HW_OVL_LAYER_NUM + EXTERNAL_HW_OVL_2L_LAYER_NUM;
#endif
	} else {
		primary_max_input_layer_num = PRIMARY_HW_OVL_LAYER_NUM + PRIMARY_HW_OVL_2L_LAYER_NUM;
		secondary_max_input_layer_num = EXTERNAL_HW_OVL_LAYER_NUM + EXTERNAL_HW_OVL_2L_LAYER_NUM;
	}

#ifdef HRT_DEBUG
	DISPMSG("60hz hrt bound\n");
#endif
	return 60;

}

int set_disp_info(disp_layer_info *disp_info_user)
{
	memcpy(&disp_info_hrt, disp_info_user, sizeof(disp_layer_info));

	if (disp_info_hrt.layer_num[0]) {
		disp_info_hrt.input_config[0] =
			kzalloc(sizeof(layer_config) * disp_info_hrt.layer_num[0], GFP_KERNEL);
		if (disp_info_hrt.input_config[0] == NULL) {
			DISPERR("[HRT]: alloc input config 0 fail, layer_num:%d\n",
				disp_info_hrt.layer_num[0]);
			return -EFAULT;
		}

		if (copy_from_user(disp_info_hrt.input_config[0], disp_info_user->input_config[0],
			sizeof(layer_config) * disp_info_hrt.layer_num[0])) {
			DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}
	}

	if (disp_info_hrt.layer_num[1]) {
		disp_info_hrt.input_config[1] =
			kzalloc(sizeof(layer_config) * disp_info_hrt.layer_num[1], GFP_KERNEL);
		if (disp_info_hrt.input_config[1] == NULL) {
			DISPERR("[HRT]: alloc input config 1 fail, layer_num:%d\n",
				disp_info_hrt.layer_num[1]);
			return -EFAULT;
		}

		if (copy_from_user(disp_info_hrt.input_config[1], disp_info_user->input_config[1],
			sizeof(layer_config) * disp_info_hrt.layer_num[1])) {
			DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}
	}

	return 0;
}

int copy_layer_info_to_user(disp_layer_info *disp_info_user)
{
	int ret = 0;

	disp_info_user->hrt_num = disp_info_hrt.hrt_num;

	if (disp_info_hrt.layer_num[0]) {
		disp_info_user->gles_head[0] = disp_info_hrt.gles_head[0];
		disp_info_user->gles_tail[0] = disp_info_hrt.gles_tail[0];
		if (copy_to_user(disp_info_user->input_config[0], disp_info_hrt.input_config[0],
			sizeof(layer_config) * disp_info_hrt.layer_num[0])) {
			DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			ret = -EFAULT;
		}
		kfree(disp_info_hrt.input_config[0]);
	}

	if (disp_info_hrt.layer_num[1]) {
		disp_info_user->gles_head[1] = disp_info_hrt.gles_head[1];
		disp_info_user->gles_tail[1] = disp_info_hrt.gles_tail[1];
		if (copy_to_user(disp_info_user->input_config[1], disp_info_hrt.input_config[1],
			sizeof(layer_config) * disp_info_hrt.layer_num[1])) {
			DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			ret = -EFAULT;
		}
		kfree(disp_info_hrt.input_config[1]);
	}

	return ret;
}

int dispsys_hrt_calc(disp_layer_info *disp_info_user)
{
	int ret;

	if (check_disp_info(disp_info_user) < 0) {
		DISPERR("check_disp_info fail\n");
		return -EFAULT;
	}

	if (set_disp_info(disp_info_user))
		return -EFAULT;

	print_disp_info_to_log_buffer(&disp_info_hrt);

#ifdef HRT_DEBUG
	DISPMSG("[Input data]\n");
	dump_disp_info(&disp_info_hrt);
#endif

	/* Set corresponding hrt bound for 60HZ and 120HZ. */
	primary_fps = set_hrt_bound();

	/*
	If the number of input layr over the real layer number OVL hw can support,
	set some of these layers as GLES layer to meet the hw capability.
	*/
	ret = filter_by_ovl_cnt(&disp_info_hrt);

	/*
	Calculate overlap number of available input layers.
	If the overlap number is out of bound, then decrease the number of available layers
	to overlap number.
	*/
	calc_hrt_num(&disp_info_hrt);

	/* Fill layer id for each input layers. All the gles layer set as same layer id. */
	ret = dispatch_ovl_id(&disp_info_hrt);

	/* check each layer to see if it can be as ext layer */
	if (disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER))
		ret = dispatch_ext_layer(&disp_info_hrt);

	dump_disp_info(&disp_info_hrt);

#ifdef CONFIG_MTK_DISPLAY_120HZ_SUPPORT
	/* Make hrt number with primary fps. */
	disp_info_hrt.hrt_num = MAKE_HRT_NUM(primary_fps, disp_info_hrt.hrt_num);
#endif

	ret = copy_layer_info_to_user(disp_info_user);
	return ret;
}
