#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/rtpm_prio.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "mtk_hrt.h"

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
		DISPMSG("Disp_Id:%d, disp_mode:%d, layer_num:%d, hrt_num:%d, gles_head:%d, gles_tail:%d\n",
			i, disp_info->disp_mode[i], disp_info->layer_num[i], disp_info->hrt_num,
			disp_info->gles_head[i], disp_info->gles_tail[i]);

		for (j = 0 ; j < disp_info->layer_num[i] ; j++) {
			layer_info = &disp_info->input_config[i][j];
			DISPMSG("[L->O]:%d->%d, offset(%d, %d), w/h(%d, %d), fmt:0x%x\n",
				j, layer_info->ovl_id, layer_info->dst_offset_x,
				layer_info->dst_offset_y, layer_info->dst_width, layer_info->dst_height,
				layer_info->src_fmt);
		}
	}
}

static int filter_by_ovl_cnt(disp_layer_info *disp_info)
{
	int ovl_num_limit, disp_index, curr_ovl_num;
	int i;
	layer_config *layer_info;

	/* 0->primary display, 1->secondary display */
	for (disp_index = 0 ; disp_index < 2 ; disp_index++) {

		/* No need to considerate HRT in decouple mode */
		if (disp_info->disp_mode[disp_index] == DISP_SESSION_DECOUPLE_MIRROR_MODE ||
			disp_info->disp_mode[disp_index] == DISP_SESSION_DECOUPLE_MODE)
			continue;

		if (disp_index == 0)
			ovl_num_limit = PRIMARY_OVL_LAYER_NUM;
		else
			ovl_num_limit = SECONDARY_OVL_LAYER_NUM;

		if (disp_info->layer_num[disp_index] <= ovl_num_limit)
			continue;

		curr_ovl_num = 0;
		for (i = 0 ; i < disp_info->layer_num[disp_index] ; i++) {
			layer_info = &disp_info->input_config[disp_index][i];

			if (disp_info->gles_head[disp_index] == -1) {
				disp_info->gles_head[disp_index] = ovl_num_limit - 1;
				disp_info->gles_tail[disp_index] = disp_info->layer_num[disp_index] - 1;
			} else {
				if (ovl_num_limit > disp_info->gles_head[disp_index]) {
					int tmp_tail = 0;

					curr_ovl_num = ovl_num_limit - disp_info->gles_head[disp_index] - 1;
					tmp_tail = disp_info->layer_num[disp_index] - curr_ovl_num - 1;
					if (tmp_tail > disp_info->gles_tail[disp_index])
						disp_info->gles_tail[disp_index] = tmp_tail;
				} else {
					disp_info->gles_head[disp_index] = ovl_num_limit - 1;
					disp_info->gles_tail[disp_index] = disp_info->layer_num[disp_index] - 1;
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

hrt_sort_entry *x_entry_list, *y_entry_list;

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
				/* Free head entry */
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
		if (sum_overlap_w <= LARB_LOWER_BOUND * 240)
			return HRT_LEVEL_LOW;
		else if (sum_overlap_w <= LARB_UPPER_BOUND * 240)
			return HRT_LEVEL_HIGH;
		else
			return HRT_OVER_LIMIT;
	} else {
		if (sum_overlap_w <= OVERLAP_LOWER_BOUND * 240)
			return HRT_LEVEL_LOW;
		else if (sum_overlap_w <= OVERLAP_UPPER_BOUND * 240)
			return HRT_LEVEL_HIGH;
		else
			return HRT_OVER_LIMIT;
	}

	return 2;
}

static int get_ovl_layer_cnt(disp_layer_info *disp_info, int disp_idx)
{
	int total_cnt = 0;

	if (disp_info->layer_num[disp_idx] != -1) {
		total_cnt += disp_info->layer_num[disp_idx];

		if (disp_info->gles_head[disp_idx] >= 0)
			total_cnt = total_cnt + disp_info->gles_head[disp_idx]
					- disp_info->gles_tail[disp_idx];
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

static int get_layer_weight(int disp_idx)
{
#ifdef CONFIG_MTK_HDMI_SUPPORT
	if (disp_idx == HRT_SECONDARY) {
		int weight = 0;
		disp_session_info dispif_info;

		/* For seconary display, set the wight 4K@30 as 2K@60.	*/
		hdmi_get_dev_info(true, &dispif_info);

		if (dispif_info.displayWidth > 2560)
			weight = 120;
		else if (dispif_info.displayWidth > 1920)
			weight = 60;
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
				int start_layer, int end_layer, bool force_scan_y)
{
	int i, bpp, sum_overlap_w, overlap_lower_bound, overlay_w, weight;
	bool has_gles = false;
	layer_config *layer_info;

	/* TODO: get display fps here */
	weight = get_layer_weight(disp_index);

	sum_overlap_w = 0;
	overlap_lower_bound = OVERLAP_LOWER_BOUND * 240;
	if (disp_info->gles_head[disp_index] != -1 &&
		disp_info->gles_head[disp_index] >= start_layer &&
		disp_info->gles_head[disp_index] <= end_layer)
		has_gles = true;

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

	if (has_gles)
		sum_overlap_w += (4 * weight);

	if (sum_overlap_w > overlap_lower_bound ||
		has_hrt_limit(disp_info, HRT_SECONDARY) ||
		force_scan_y) {
		sum_overlap_w = scan_y_overlap(disp_info, disp_index, overlap_lower_bound);
		if (has_gles)
			sum_overlap_w += (4 * weight);
	}

#ifdef HRT_DEBUG
	DISPMSG("%s disp_index:%d, start_layer:%d, end_layer:%d, sum_overlap_w:%d\n",
		__func__, disp_index, start_layer, end_layer, sum_overlap_w);
#endif

	free_all_layer_entry(true);
	return sum_overlap_w;
}

static bool _calc_larb0(disp_layer_info *disp_info, int emi_hrt_w)
{
	int primary_ovl_cnt = 0, larb_idx = 0, sum_overlap_w = 0;
	bool is_over_bound = true;

	if (!has_hrt_limit(disp_info, HRT_PRIMARY)) {
		is_over_bound = false;
		return is_over_bound;
	}

	primary_ovl_cnt = get_ovl_layer_cnt(disp_info, HRT_PRIMARY);
	if (primary_ovl_cnt < 4)
		larb_idx = primary_ovl_cnt - 1;
	else if (primary_ovl_cnt < 7)
		larb_idx = 2;
	else
		larb_idx = 3;

	if (disp_info->gles_head[0] != -1 &&
		larb_idx > disp_info->gles_head[0])
		larb_idx += (disp_info->gles_tail[0] - disp_info->gles_head[0]);

	sum_overlap_w = _calc_hrt_num(disp_info, 0, 0, larb_idx, false);

	if (get_hrt_level(sum_overlap_w, true) > HRT_LEVEL_LOW)
		is_over_bound = true;
	else
		is_over_bound = false;

	return is_over_bound;
}

static bool _calc_larb5(disp_layer_info *disp_info, int emi_hrt_w)
{
	int primary_ovl_cnt = 0, larb_idx = 0, sum_overlap_w = 0;
	bool is_over_bound = true;

	primary_ovl_cnt = get_ovl_layer_cnt(disp_info, HRT_PRIMARY);
	if (primary_ovl_cnt > 3 && has_hrt_limit(disp_info, HRT_PRIMARY)) {
		if (primary_ovl_cnt < 4)
			larb_idx = primary_ovl_cnt;
		else if (primary_ovl_cnt < 7)
			larb_idx = 3;
		else
			larb_idx = 4;

		if (disp_info->gles_head[0] != -1 &&
			larb_idx > disp_info->gles_head[0])
			larb_idx += (disp_info->gles_tail[0] - disp_info->gles_head[0]);

		/* HRTMSG("%s larb_idx:%d\n", __func__, larb_idx); */
		sum_overlap_w += _calc_hrt_num(disp_info, HRT_PRIMARY,
					larb_idx, disp_info->layer_num[0] - 1, false);
	}

	if (has_hrt_limit(disp_info, HRT_SECONDARY)) {
		sum_overlap_w += _calc_hrt_num(disp_info, HRT_SECONDARY,
					0, disp_info->layer_num[1] - 1, false);
	}

	/* HRTMSG("%s, sum_overlap_w:%d\n", __func__, sum_overlap_w); */
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
	bool single_ovl = false;

	/*
	This is a temporary solution for support single ovl for primary display.
	Remove it if primary display can support multiple ovl.
	*/
	if (PRIMARY_OVL_LAYER_NUM < 8)
		single_ovl = true;

	/* Calculate HRT for EMI level */
	if (has_hrt_limit(disp_info, HRT_PRIMARY))
		sum_overlay_w = _calc_hrt_num(disp_info, 0, 0, disp_info->layer_num[0] - 1, false);
	if (has_hrt_limit(disp_info, HRT_SECONDARY))
		sum_overlay_w += _calc_hrt_num(disp_info, 1, 0, disp_info->layer_num[1] - 1, false);


	hrt_level = get_hrt_level(sum_overlay_w, false);

	/*
	The larb bound always meet the limit for HRT_LEVEL2 in 8+4 ovl architecture.
	So calculate larb bound only for HRT_LEVEL2.
	*/
	disp_info->hrt_num = hrt_level;
#ifdef HRT_DEBUG
	DISPMSG("EMI hrt level:%d\n", hrt_level);
#endif
	if (hrt_level > HRT_LEVEL_LOW)
		return hrt_level;

	if (single_ovl) {
		/* In single ovl mode, larb0->primary, larb5->secondary */

		if (has_hrt_limit(disp_info, HRT_PRIMARY)) {
			sum_overlay_w = _calc_hrt_num(disp_info, 0, 0, disp_info->layer_num[0] - 1, true);
			hrt_level = get_hrt_level(sum_overlay_w, true);
			if (hrt_level > HRT_LEVEL_LOW) {
				disp_info->hrt_num = hrt_level;
				return hrt_level;
			}
		}
		if (has_hrt_limit(disp_info, HRT_SECONDARY)) {
			sum_overlay_w += _calc_hrt_num(disp_info, 1, 0, disp_info->layer_num[1] - 1, true);
			hrt_level = get_hrt_level(sum_overlay_w, true);
			if (hrt_level > HRT_LEVEL_LOW) {
				disp_info->hrt_num = hrt_level;
				return hrt_level;
			}
		}
	} else {
		/* Check Larb Bound here */
		if (calc_larb_hrt(disp_info, sum_overlay_w))
			hrt_level = HRT_LEVEL_HIGH;
		else
			hrt_level = HRT_LEVEL_LOW;
		disp_info->hrt_num = hrt_level;
	}
#ifdef HRT_DEBUG
	DISPMSG("Larb hrt level:%d\n", hrt_level);
#endif
	return hrt_level;
}

static int dispatch_ovl_id(disp_layer_info *disp_info, int hrt_level)
{
	int disp_idx, i, ovl_id_offset, overlap_upper_bound;
	layer_config *layer_info;
	bool has_gles, has_second_disp;

	if (disp_info->layer_num[1] > 0)
		has_second_disp = true;
	else
		has_second_disp = false;

	overlap_upper_bound = OVERLAP_UPPER_BOUND;

	/* Dispatch gles range if necessary */
	if (hrt_level > HRT_LEVEL_HIGH) {
		int valid_ovl_cnt = OVERLAP_UPPER_BOUND;

		/*
		Arrange 4 ovl layers to secondary display, so no need to
		redistribute gles layer since it's already meet the larb
		limit.
		*/
		if (has_hrt_limit(disp_info, HRT_SECONDARY))
			valid_ovl_cnt -= get_ovl_layer_cnt(disp_info, HRT_SECONDARY);

		if (has_hrt_limit(disp_info, HRT_PRIMARY)) {

			if (disp_info->gles_head[HRT_PRIMARY] == -1) {
				disp_info->gles_head[HRT_PRIMARY] = valid_ovl_cnt - 1;
				disp_info->gles_tail[HRT_PRIMARY] = disp_info->layer_num[HRT_PRIMARY] - 1;
			} else {
				if (disp_info->gles_head[HRT_PRIMARY] + 1 - valid_ovl_cnt >= 0) {
					disp_info->gles_head[HRT_PRIMARY] =
						disp_info->gles_head[HRT_PRIMARY] + 1 - valid_ovl_cnt;
					disp_info->gles_tail[HRT_PRIMARY] = disp_info->layer_num[HRT_PRIMARY] - 1;
				} else {
					int tail_ovl_num = valid_ovl_cnt - disp_info->gles_head[HRT_PRIMARY] + 1;

					if (disp_info->gles_tail[HRT_PRIMARY] <
						disp_info->layer_num[HRT_PRIMARY] - tail_ovl_num) {
						disp_info->gles_tail[HRT_PRIMARY] =
							disp_info->layer_num[HRT_PRIMARY] - tail_ovl_num;
					} else {
						valid_ovl_cnt = disp_info->gles_tail[disp_idx] -
							(disp_info->layer_num[HRT_PRIMARY] - tail_ovl_num);
					}
				}
			}
		}
	}

	/* Dispatch OVL id */
	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {
		int gles_count, ovl_cnt;

		ovl_cnt = get_ovl_layer_cnt(disp_info, disp_idx);
		if (ovl_cnt <= LARB_LOWER_BOUND * 2 && disp_idx == HRT_PRIMARY &&
			PRIMARY_OVL_LAYER_NUM > 4)
			ovl_id_offset = 1;
		else
			ovl_id_offset = 0;
		gles_count = disp_info->gles_tail[disp_idx] - disp_info->gles_head[disp_idx] + 1;
		has_gles = false;
		if (disp_info->gles_head[disp_idx] != -1)
			has_gles = true;

		for (i = 0 ; i < disp_info->layer_num[disp_idx] ; i++) {
			layer_info = &disp_info->input_config[disp_idx][i];

			if (i < disp_info->gles_head[disp_idx])
				layer_info->ovl_id = i + ovl_id_offset;
			else if (i > disp_info->gles_tail[disp_idx])
				layer_info->ovl_id = i - gles_count + 1 + ovl_id_offset;
			else
				layer_info->ovl_id = disp_info->gles_head[disp_idx] + ovl_id_offset;
		}
	}
	return 0;
}

int check_disp_info(disp_layer_info *disp_info)
{
	int disp_idx;
	/*layer_config *layer_info;*/

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

int dispsys_hrt_calc(disp_layer_info *disp_info)
{
	int ret, hrt_level;

	if (check_disp_info(disp_info) < 0) {
		DISPERR("check_disp_info fail\n");
		return -1;
	}
#ifdef HRT_DEBUG
	DISPMSG("[Input data]\n");
	dump_disp_info(disp_info);
#endif

	ret = filter_by_ovl_cnt(disp_info);

	/*
	Calculate overlap number of available input layers.
	If the overlap number is out of bound, then decrease the number of available layers
	to overlap number.
	*/
	hrt_level = calc_hrt_num(disp_info);

	/*
	Fill layer id for each input layers. All the gles layer set as same layer id.
	*/
	ret = dispatch_ovl_id(disp_info, hrt_level);
	dump_disp_info(disp_info);

	return ret;
}
