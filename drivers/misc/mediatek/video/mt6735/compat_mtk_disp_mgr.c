/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include "compat_mtk_disp_mgr.h"

#include "mtk_sync.h"

#include "disp_debug.h"
#include "disp_lcm.h"
#include "disp_utils.h"

#include "primary_display.h"

#include "ddp_manager.h"
#include "disp_drv_platform.h"
#include "disp_recorder.h"

#include "mtk_disp_mgr.h"

#include "disp_session.h"
#include "mtk_ovl.h"
#include "mtkfb_fence.h"


#ifdef CONFIG_COMPAT
static int compat_get_disp_session_config(compat_disp_session_config __user *data32,
					  disp_session_config __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data32->type));
	err |= put_user(u, &(data->type));

	err |= get_user(u, &(data32->device_id));
	err |= put_user(u, &(data->device_id));

	err |= get_user(u, &(data32->mode));
	err |= put_user(u, &(data->mode));

	err |= get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->user));
	err |= put_user(u, &(data->user));

	err |= get_user(u, &(data32->present_fence_idx));
	err |= put_user(u, &(data->present_fence_idx));

	err |= get_user(u, &(data32->dc_type));
	err |= put_user(u, &(data->dc_type));

	err |= get_user(i, &(data32->need_merge));
	err |= put_user(i, &(data->need_merge));

	err |= get_user(u, &(data32->tigger_mode));
	err |= put_user(u, &(data->tigger_mode));

	return err;
}

static int compat_put_disp_session_config(compat_disp_session_config __user *data32,
					  disp_session_config __user *data)
{

	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data->type));
	err |= put_user(u, &(data32->type));

	err |= get_user(u, &(data->device_id));
	err |= put_user(u, &(data32->device_id));

	err |= get_user(u, &(data->mode));
	err |= put_user(u, &(data32->mode));

	err |= get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->user));
	err |= put_user(u, &(data32->user));

	err |= get_user(u, &(data->present_fence_idx));
	err |= put_user(u, &(data32->present_fence_idx));

	err |= get_user(u, &(data->dc_type));
	err |= put_user(u, &(data32->dc_type));

	err |= get_user(i, &(data->need_merge));
	err |= put_user(i, &(data32->need_merge));

	err |= get_user(u, &(data->tigger_mode));
	err |= put_user(u, &(data32->tigger_mode));

	return err;
}

static int compat_get_disp_present_fence(compat_disp_present_fence __user *data32,
					 disp_present_fence __user *data)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->present_fence_fd));
	err |= put_user(u, &(data->present_fence_fd));

	err |= get_user(u, &(data32->present_fence_index));
	err |= put_user(u, &(data->present_fence_index));

	return err;
}

static int compat_put_disp_present_fence(compat_disp_present_fence __user *data32,
					 disp_present_fence __user *data)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->present_fence_fd));
	err |= put_user(u, &(data32->present_fence_fd));

	err |= get_user(u, &(data->present_fence_index));
	err |= put_user(u, &(data32->present_fence_index));

	return err;
}

static int compat_get_disp_buffer_info(compat_disp_buffer_info __user *data32,
				       disp_buffer_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->layer_id));
	err |= put_user(u, &(data->layer_id));

	err |= get_user(u, &(data32->layer_en));
	err |= put_user(u, &(data->layer_en));

	err |= get_user(i, &(data32->ion_fd));
	err |= put_user(i, &(data->ion_fd));

	err |= get_user(u, &(data32->cache_sync));
	err |= put_user(u, &(data->cache_sync));

	err |= get_user(u, &(data32->index));
	err |= put_user(u, &(data->index));

	err |= get_user(i, &(data32->fence_fd));
	err |= put_user(i, &(data->fence_fd));

	err |= get_user(u, &(data32->interface_index));
	err |= put_user(u, &(data->interface_index));

	err |= get_user(i, &(data32->interface_fence_fd));
	err |= put_user(i, &(data->interface_fence_fd));

	return err;
}

static int compat_put_disp_buffer_info(compat_disp_buffer_info __user *data32,
				       disp_buffer_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->layer_id));
	err |= put_user(u, &(data32->layer_id));

	err |= get_user(u, &(data->layer_en));
	err |= put_user(u, &(data32->layer_en));

	err |= get_user(i, &(data->ion_fd));
	err |= put_user(i, &(data32->ion_fd));

	err |= get_user(u, &(data->cache_sync));
	err |= put_user(u, &(data32->cache_sync));

	err |= get_user(u, &(data->index));
	err |= put_user(u, &(data32->index));

	err |= get_user(i, &(data->fence_fd));
	err |= put_user(i, &(data32->fence_fd));

	err |= get_user(u, &(data->interface_index));
	err |= put_user(u, &(data32->interface_index));

	err |= get_user(i, &(data->interface_fence_fd));
	err |= put_user(i, &(data32->interface_fence_fd));

	return err;

}

static int compat_get_disp_input_config(compat_disp_input_config __user *data32,
					disp_input_config __user *data)
{
	compat_uint_t u;
	compat_uptr_t p;
	compat_ushort_t s;
	uint8_t c;
	int err;

	err = get_user(p, (unsigned long *)&(data32->src_base_addr));
	err |= put_user(p, (unsigned long *)&(data->src_base_addr));

	err |= get_user(p, (unsigned long *)&(data32->src_phy_addr));
	err |= put_user(p, (unsigned long *)&(data->src_phy_addr));

	err |= get_user(u, &(data32->buffer_source));
	err |= put_user(u, &(data->buffer_source));

	err |= get_user(u, &(data32->security));
	err |= put_user(u, &(data->security));

	err |= get_user(u, &(data32->src_fmt));
	err |= put_user(u, &(data->src_fmt));

	err |= get_user(u, &(data32->src_alpha));
	err |= put_user(u, &(data->src_alpha));

	err |= get_user(u, &(data32->dst_alpha));
	err |= put_user(u, &(data->dst_alpha));

	err |= get_user(u, &(data32->yuv_range));
	err |= put_user(u, &(data->yuv_range));

	err |= get_user(u, &(data32->layer_rotation));
	err |= put_user(u, &(data->layer_rotation));

	err |= get_user(u, &(data32->layer_type));
	err |= put_user(u, &(data->layer_type));

	err |= get_user(u, &(data32->video_rotation));
	err |= put_user(u, &(data->video_rotation));

	err |= get_user(u, &(data32->next_buff_idx));
	err |= put_user(u, &(data->next_buff_idx));

	err |= get_user(u, &(data32->src_fence_fd));
	err |= put_user(u, &(data->src_fence_fd));

	err |= get_user(p, (unsigned long *)&(data32->src_fence_struct));
	err |= put_user(p, (unsigned long *)&(data->src_fence_struct));

	err |= get_user(u, &(data32->src_color_key));
	err |= put_user(u, &(data->src_color_key));

	err |= get_user(u, &(data32->frm_sequence));
	err |= put_user(u, &(data->frm_sequence));

	err |= get_user(p, (unsigned long *)&(data32->dirty_roi_addr));
	err |= put_user(p, (unsigned long *)&(data->dirty_roi_addr));

	err |= get_user(s, &(data32->dirty_roi_num));
	err |= put_user(s, &(data->dirty_roi_num));

	err |= get_user(s, &(data32->src_pitch));
	err |= put_user(s, &(data->src_pitch));

	err |= get_user(s, &(data32->src_offset_x));
	err |= put_user(s, &(data->src_offset_x));

	err |= get_user(s, &(data32->src_offset_y));
	err |= put_user(s, &(data->src_offset_y));

	err |= get_user(s, &(data32->src_width));
	err |= put_user(s, &(data->src_width));

	err |= get_user(s, &(data32->src_height));
	err |= put_user(s, &(data->src_height));

	err |= get_user(s, &(data32->tgt_offset_x));
	err |= put_user(s, &(data->tgt_offset_x));

	err |= get_user(s, &(data32->tgt_offset_y));
	err |= put_user(s, &(data->tgt_offset_y));

	err |= get_user(s, &(data32->tgt_width));
	err |= put_user(s, &(data->tgt_width));

	err |= get_user(s, &(data32->tgt_height));
	err |= put_user(s, &(data->tgt_height));

	err |= get_user(c, &(data32->alpha_enable));
	err |= put_user(c, &(data->alpha_enable));

	err |= get_user(c, &(data32->alpha));
	err |= put_user(c, &(data->alpha));

	err |= get_user(c, &(data32->sur_aen));
	err |= put_user(c, &(data->sur_aen));

	err |= get_user(c, &(data32->src_use_color_key));
	err |= put_user(c, &(data->src_use_color_key));

	err |= get_user(c, &(data32->layer_id));
	err |= put_user(c, &(data->layer_id));

	err |= get_user(c, &(data32->layer_enable));
	err |= put_user(c, &(data->layer_enable));

	err |= get_user(c, &(data32->src_direct_link));
	err |= put_user(c, &(data->src_direct_link));

	err |= get_user(c, &(data32->isTdshp));
	err |= put_user(c, &(data->isTdshp));

	err |= get_user(c, &(data32->identity));
	err |= put_user(c, &(data->identity));

	err |= get_user(c, &(data32->connected_type));
	err |= put_user(c, &(data->connected_type));

	err |= get_user(c, &(data32->ext_sel_layer));
	err |= put_user(c, &(data->ext_sel_layer));

	return err;
}

static int compat_get_disp_session_input_config(compat_disp_session_input_config __user *data32,
						disp_session_input_config __user *data)
{
	compat_uint_t u;
	int err;
	int j;

	err = get_user(u, &(data32->setter));
	err |= put_user(u, &(data->setter));

	err |= get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->config_layer_num));
	err |= put_user(u, &(data->config_layer_num));

	for (j = 0; j < ARRAY_SIZE(data32->config); j++)
		err |= compat_get_disp_input_config(&data32->config[j], &data->config[j]);

	return err;
}

static int compat_get_disp_output_config(compat_disp_output_config __user *data32,
					 disp_output_config __user *data)
{
	compat_uint_t u;
	compat_uptr_t p;
	int err;

	err = get_user(p, (unsigned long *)&(data32->va));
	err |= put_user(p, (unsigned long *)&(data->va));

	err |= get_user(p, (unsigned long *)&(data32->pa));
	err |= put_user(p, (unsigned long *)&(data->pa));

	err |= get_user(u, &(data32->fmt));
	err |= put_user(u, &(data->fmt));

	err |= get_user(u, &(data32->x));
	err |= put_user(u, &(data->x));

	err |= get_user(u, &(data32->y));
	err |= put_user(u, &(data->y));

	err |= get_user(u, &(data32->width));
	err |= put_user(u, &(data->width));

	err |= get_user(u, &(data32->height));
	err |= put_user(u, &(data->height));

	err |= get_user(u, &(data32->pitch));
	err |= put_user(u, &(data->pitch));

	err |= get_user(u, &(data32->pitchUV));
	err |= put_user(u, &(data->pitchUV));

	err |= get_user(u, &(data32->security));
	err |= put_user(u, &(data->security));

	err |= get_user(u, &(data32->buff_idx));
	err |= put_user(u, &(data->buff_idx));

	err |= get_user(u, &(data32->interface_idx));
	err |= put_user(u, &(data->interface_idx));

	err |= get_user(u, &(data32->src_fence_fd));
	err |= put_user(u, &(data->src_fence_fd));

	err |= get_user(p, (unsigned long *)&(data32->src_fence_struct));
	err |= put_user(p, (unsigned long *)&(data->src_fence_struct));

	err |= get_user(u, &(data32->frm_sequence));
	err |= put_user(u, &(data->frm_sequence));

	return err;
}

static int compat_get_disp_session_output_config(compat_disp_session_output_config __user *data32,
						 disp_session_output_config __user *data)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= compat_get_disp_output_config(&data32->config, &data->config);

	return err;
}

static int compat_get_disp_session_info(compat_disp_session_info __user *data32,
					disp_session_info __user *data)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->maxLayerNum));
	err |= put_user(u, &(data->maxLayerNum));

	err |= get_user(u, &(data32->isHwVsyncAvailable));
	err |= put_user(u, &(data->isHwVsyncAvailable));

	err |= get_user(u, &(data32->displayType));
	err |= put_user(u, &(data->displayType));

	err |= get_user(u, &(data32->displayWidth));
	err |= put_user(u, &(data->displayWidth));

	err |= get_user(u, &(data32->displayHeight));
	err |= put_user(u, &(data->displayHeight));

	err |= get_user(u, &(data32->displayFormat));
	err |= put_user(u, &(data->displayFormat));

	err |= get_user(u, &(data32->displayMode));
	err |= put_user(u, &(data->displayMode));

	err |= get_user(u, &(data32->vsyncFPS));
	err |= put_user(u, &(data->vsyncFPS));

	err |= get_user(u, &(data32->physicalWidth));
	err |= put_user(u, &(data->physicalWidth));

	err |= get_user(u, &(data32->physicalHeight));
	err |= put_user(u, &(data->physicalHeight));

	err |= get_user(u, &(data32->physicalWidthUm));
	err |= put_user(u, &(data->physicalWidthUm));

	err |= get_user(u, &(data32->physicalHeightUm));
	err |= put_user(u, &(data->physicalHeightUm));

	err |= get_user(u, &(data32->isConnected));
	err |= put_user(u, &(data->isConnected));

	err |= get_user(u, &(data32->isHDCPSupported));
	err |= put_user(u, &(data->isHDCPSupported));

	err |= get_user(u, &(data32->isOVLDisabled));
	err |= put_user(u, &(data->isOVLDisabled));

	err |= get_user(u, &(data32->is3DSupport));
	err |= put_user(u, &(data->is3DSupport));

	err |= get_user(u, &(data32->const_layer_num));
	err |= put_user(u, &(data->const_layer_num));

	err |= get_user(u, &(data32->updateFPS));
	err |= put_user(u, &(data->updateFPS));

	err |= get_user(u, &(data32->is_updateFPS_stable));
	err |= put_user(u, &(data->is_updateFPS_stable));

	return err;
}

static int compat_put_disp_session_info(compat_disp_session_info __user *data32,
					disp_session_info __user *data)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->maxLayerNum));
	err |= put_user(u, &(data32->maxLayerNum));

	err |= get_user(u, &(data->isHwVsyncAvailable));
	err |= put_user(u, &(data32->isHwVsyncAvailable));

	err |= get_user(u, &(data->displayType));
	err |= put_user(u, &(data32->displayType));

	err |= get_user(u, &(data->displayWidth));
	err |= put_user(u, &(data32->displayWidth));

	err |= get_user(u, &(data->displayHeight));
	err |= put_user(u, &(data32->displayHeight));

	err |= get_user(u, &(data->displayFormat));
	err |= put_user(u, &(data32->displayFormat));

	err |= get_user(u, &(data->displayMode));
	err |= put_user(u, &(data32->displayMode));

	err |= get_user(u, &(data->vsyncFPS));
	err |= put_user(u, &(data32->vsyncFPS));

	err |= get_user(u, &(data->physicalWidth));
	err |= put_user(u, &(data32->physicalWidth));

	err |= get_user(u, &(data->physicalHeight));
	err |= put_user(u, &(data32->physicalHeight));

	err |= get_user(u, &(data->physicalWidthUm));
	err |= put_user(u, &(data32->physicalWidthUm));

	err |= get_user(u, &(data->physicalHeightUm));
	err |= put_user(u, &(data32->physicalHeightUm));

	err |= get_user(u, &(data->isConnected));
	err |= put_user(u, &(data32->isConnected));

	err |= get_user(u, &(data->isHDCPSupported));
	err |= put_user(u, &(data32->isHDCPSupported));

	err |= get_user(u, &(data->isOVLDisabled));
	err |= put_user(u, &(data32->isOVLDisabled));

	err |= get_user(u, &(data->is3DSupport));
	err |= put_user(u, &(data32->is3DSupport));

	err |= get_user(u, &(data->const_layer_num));
	err |= put_user(u, &(data32->const_layer_num));

	err |= get_user(u, &(data->updateFPS));
	err |= put_user(u, &(data32->updateFPS));

	err |= get_user(u, &(data->is_updateFPS_stable));
	err |= put_user(u, &(data32->is_updateFPS_stable));

	return err;
}

static int compat_get_disp_caps_info(compat_disp_caps_info __user *data32,
				     disp_caps_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;
	int j, k;

	err = get_user(u, &(data32->output_mode));
	err |= put_user(u, &(data->output_mode));

	err |= get_user(u, &(data32->output_pass));
	err |= put_user(u, &(data->output_pass));

	err |= get_user(u, &(data32->max_layer_num));
	err |= put_user(u, &(data->max_layer_num));

#ifdef CONFIG_FOR_SOURCE_PQ
	err |= get_user(u, &(data32->max_pq_num));
	err |= put_user(u, &(data->max_pq_num));
#endif

	err |= get_user(u, &(data32->disp_feature));
	err |= put_user(u, &(data->disp_feature));

	err |= get_user(i, &(data32->is_support_frame_cfg_ioctl));
	err |= put_user(i, &(data->is_support_frame_cfg_ioctl));

	err |= get_user(i, &(data32->is_output_rotated));
	err |= put_user(i, &(data->is_output_rotated));

	err |= get_user(i, &(data32->lcm_degree));
	err |= put_user(i, &(data->lcm_degree));

	for (j = 0; j < ARRAY_SIZE(data32->rsz_in_res_list); j++) {
		for (k = 0; k < ARRAY_SIZE(data32->rsz_in_res_list[j]); k++) {
			err |= get_user(u, &(data32->rsz_in_res_list[j][k]));
			err |= put_user(u, &(data->rsz_in_res_list[j][k]));
		}
	}

	return err;
}

static int compat_put_disp_caps_info(compat_disp_caps_info __user *data32,
				     disp_caps_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;
	int j, k;

	err = get_user(u, &(data->output_mode));
	err |= put_user(u, &(data32->output_mode));

	err |= get_user(u, &(data->output_pass));
	err |= put_user(u, &(data32->output_pass));

	err |= get_user(u, &(data->max_layer_num));
	err |= put_user(u, &(data32->max_layer_num));

#ifdef CONFIG_FOR_SOURCE_PQ
	err |= get_user(u, &(data->max_pq_num));
	err |= put_user(u, &(data32->max_pq_num));
#endif

	err |= get_user(u, &(data->disp_feature));
	err |= put_user(u, &(data32->disp_feature));

	err |= get_user(i, &(data->is_support_frame_cfg_ioctl));
	err |= put_user(i, &(data32->is_support_frame_cfg_ioctl));

	err |= get_user(i, &(data->is_output_rotated));
	err |= put_user(i, &(data32->is_output_rotated));

	err |= get_user(i, &(data->lcm_degree));
	err |= put_user(i, &(data32->lcm_degree));

	for (j = 0; j < ARRAY_SIZE(data32->rsz_in_res_list); j++) {
		for (k = 0; k < ARRAY_SIZE(data32->rsz_in_res_list[j]); k++) {
			err |= get_user(u, &(data->rsz_in_res_list[j][k]));
			err |= put_user(u, &(data32->rsz_in_res_list[j][k]));
		}
	}

	return err;
}

static int compat_get_disp_session_vsync_config(compat_disp_session_vsync_config __user *data32,
						disp_session_vsync_config __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	compat_u64 u64;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->vsync_cnt));
	err |= put_user(u, &(data->vsync_cnt));

	err |= get_user(u64, &(data32->vsync_ts));
	err |= put_user(u64, &(data->vsync_ts));

	err |= get_user(i, &(data32->lcm_fps));
	err |= put_user(i, &(data->lcm_fps));

	return err;
}

static int compat_put_disp_session_vsync_config(compat_disp_session_vsync_config __user *data32,
						disp_session_vsync_config __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	compat_u64 u64;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->vsync_cnt));
	err |= put_user(u, &(data32->vsync_cnt));

	err |= get_user(u64, &(data->vsync_ts));
	err |= put_user(u64, &(data32->vsync_ts));

	err |= get_user(i, &(data->lcm_fps));
	err |= put_user(i, &(data32->lcm_fps));

	return err;
}

static int compat_get_disp_session_buf_info(compat_disp_session_buf_info __user *data32,
					    disp_session_buf_info __user *data)
{
	compat_uint_t u;
	int err;
	int j;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	for (j = 0; j < ARRAY_SIZE(data32->buf_hnd); j++) {
		err = get_user(u, &(data32->buf_hnd[j]));
		err |= put_user(u, &(data->buf_hnd[j]));
	}

	return err;
}


int _compat_ioctl_create_session(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_config __user *data32;
	disp_session_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_CREATE_SESSION\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_CREATE_SESSION, (unsigned long)data);

	err = compat_put_disp_session_config(data32, data);
	if (err) {
		DISPERR("compat_put_disp_session_config fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_destroy_session(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_config __user *data32;
	disp_session_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_DESTROY_SESSION\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_DESTROY_SESSION, (unsigned long)data);

	return ret;
}

int _compat_ioctl_trigger_session(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_config __user *data32;
	disp_session_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_TRIGGER_SESSION\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_TRIGGER_SESSION, (unsigned long)data);

	return ret;
}

int _compat_ioctl_prepare_present_fence(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_present_fence __user *data32;
	disp_present_fence __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_PREPARE_PRESENT_FENCE\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_present_fence));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_present_fence(data32, data);
	if (err) {
		DISPERR("compat_get_disp_present_fence fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_GET_PRESENT_FENCE, (unsigned long)data);

	err = compat_put_disp_present_fence(data32, data);
	if (err) {
		DISPERR("compat_put_disp_present_fence fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_prepare_buffer(struct file *file, unsigned long arg, ePREPARE_FENCE_TYPE type)
{
	int ret = 0;
	int err = 0;

	compat_disp_buffer_info __user *data32;
	disp_buffer_info __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_PREPARE_BUFFER\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_buffer_info));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_buffer_info(data32, data);
	if (err) {
		DISPERR("compat_get_disp_buffer_info fail!\n");
		return err;
	}

	if (type == PREPARE_INPUT_FENCE)
		ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_PREPARE_INPUT_BUFFER, (unsigned long)data);
	else
		ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_PREPARE_OUTPUT_BUFFER, (unsigned long)data);

	err = compat_put_disp_buffer_info(data32, data);
	if (err) {
		DISPERR("compat_put_disp_buffer_info fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_set_input_buffer(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_input_config __user *data32;
	disp_session_input_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_SET_INPUT_BUFFER\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_input_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_input_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_input_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_INPUT_BUFFER, (unsigned long)data);

	return ret;
}

int _compat_ioctl_set_output_buffer(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_output_config __user *data32;
	disp_session_output_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_SET_OUTPUT_BUFFER\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_output_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_output_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_output_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_OUTPUT_BUFFER, (unsigned long)data);

	return ret;
}


int _compat_ioctl_get_info(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_info __user *data32;
	disp_session_info __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_GET_INFO\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_info));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_info(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_info fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_GET_SESSION_INFO, (unsigned long)data);

	err = compat_put_disp_session_info(data32, data);
	if (err) {
		DISPERR("compat_put_disp_session_info fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_get_is_driver_suspend(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_uint_t __user *data32;
	int __user data;

	DISPDBG("COMPAT_DISP_IOCTL_GET_IS_DRIVER_SUSPEND\n");

	data32 = compat_ptr(arg);

	if (get_user(data, data32)) {
		DISPERR("compat_get_is_driver_suspend fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_GET_IS_DRIVER_SUSPEND, (unsigned long)data);

	return ret;
}

int _compat_ioctl_get_display_caps(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_caps_info __user *data32;
	disp_caps_info __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_GET_DISPLAY_CAPS\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_caps_info));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_caps_info(data32, data);
	if (err) {
		DISPERR("compat_get_disp_caps_info fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_GET_DISPLAY_CAPS, (unsigned long)data);

	err = compat_put_disp_caps_info(data32, data);
	if (err) {
		DISPERR("compat_put_disp_caps_info fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_wait_vsync(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_vsync_config __user *data32;
	disp_session_vsync_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_WAIT_VSYNC\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_vsync_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_vsync_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_vsync_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_WAIT_FOR_VSYNC, (unsigned long)data);

	err = compat_put_disp_session_vsync_config(data32, data);
	if (err) {
		DISPERR("compat_put_disp_session_vsync_config fail!\n");
		return err;
	}

	return ret;
}

int _compat_ioctl_set_vsync(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_uint_t __user *data32;
	unsigned int __user data;

	DISPDBG("COMPAT_DISP_IOCTL_SET_VSYNC\n");

	data32 = compat_ptr(arg);

	if (get_user(data, data32)) {
		DISPERR("compat_get_fps fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_VSYNC_FPS, (unsigned long)data);

	return ret;
}

int _compat_ioctl_insert_session_buffers(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_buf_info __user *data32;
	disp_session_buf_info __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_INSERT_SESSION_BUFFERS\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_buf_info));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_buf_info(data32, data);
	if (err) {
		DISPERR("compat_get_disp_session_buf_info fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_INSERT_SESSION_BUFFERS, (unsigned long)data);

	return ret;
}

int _compat_ioctl_screen_freeze(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_uint_t __user *data32;
	unsigned int __user data;

	DISPDBG("COMPAT_DISP_IOCTL_SCREEN_FREEZE\n");

	data32 = compat_ptr(arg);

	if (get_user(data, data32)) {
		DISPERR("compat_screen_freeze fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SCREEN_FREEZE, (unsigned long)data);

	return ret;
}

int _compat_ioctl_set_session_mode(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_config __user *data32;
	disp_session_config __user *data;

	DISPDBG("COMPAT_DISP_IOCTL_SET_SESSION_MODE\n");

	data32 = compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_config(data32, data);
	if (err) {
		DISPERR("compat_set_session_mode fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_SESSION_MODE, (unsigned long)data);

	return ret;
}

#endif
