
#include "compat_mtk_disp_mgr.h"

#include "disp_drv_log.h"
#include "debug.h"
#include "primary_display.h"
#include "display_recorder.h"
#include "mtkfb_fence.h"
#include "disp_drv_platform.h"


#ifdef CONFIG_COMPAT
static int compat_get_disp_caps_info(
		struct compat_disp_caps_info __user *data32,
		disp_caps_info __user *data)
{
	compat_uint_t u;
	int err = 0;

	err = get_user(u, &(data32->output_mode));
	err |= put_user(u, &(data->output_mode));

	err |= get_user(u, &(data32->output_pass));
	err |= put_user(u, &(data->output_pass));

	err |= get_user(u, &(data32->max_layer_num));
	err |= put_user(u, &(data->max_layer_num));

	return err;
}

static int compat_put_disp_caps_info(
		struct compat_disp_caps_info __user *data32,
		disp_caps_info __user *data __user)
{
	compat_uint_t u;
	int err = 0;

	err = get_user(u, &(data->output_mode));
	err |= put_user(u, &(data32->output_mode));

	err |= get_user(u, &(data->output_pass));
	err |= put_user(u, &(data32->output_pass));

	err |= get_user(u, &(data->max_layer_num));
	err |= put_user(u, &(data32->max_layer_num));

	return err;
}

/*
static int compat_get_disp_session_layer_num_config(
		struct compat_disp_session_layer_num_config __user *data32,
		disp_session_layer_num_config __user *data)
{
	compat_uint_t u;
	int err = 0;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->max_layer_num));
	err |= put_user(u, &(data->max_layer_num));

	return err;
}

static int compat_put_disp_session_layer_num_config(
		struct compat_disp_session_layer_num_config __user *data32,
		disp_session_layer_num_config __user *data)
{
	compat_uint_t u;
	int err = 0;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->max_layer_num));
	err |= put_user(u, &(data32->max_layer_num));

	return err;
}
*/
static int compat_get_disp_session_output_config(compat_disp_session_output_config * data32 __user,
					disp_session_output_config * data __user)
{
	compat_uint_t u;
	compat_uptr_t p;
	int err = 0;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(p, &(data32->config.va));
	err |= put_user(p, &(data->config.va));

	err |= get_user(p, &(data32->config.pa));
	err |= put_user(p, &(data->config.pa));

	err |= get_user(u, &(data32->config.fmt));
	err |= put_user(u, &(data->config.fmt));

	err |= get_user(u, &(data32->config.x));
	err |= put_user(u, &(data->config.x));

	err |= get_user(u, &(data32->config.y));
	err |= put_user(u, &(data->config.y));

	err |= get_user(u, &(data32->config.width));
	err |= put_user(u, &(data->config.width));

	err |= get_user(u, &(data32->config.height));
	err |= put_user(u, &(data->config.height));

	err |= get_user(u, &(data32->config.pitch));
	err |= put_user(u, &(data->config.pitch));

	err |= get_user(u, &(data32->config.pitchUV));
	err |= put_user(u, &(data->config.pitchUV));

	err |= get_user(u, &(data32->config.security));
	err |= put_user(u, &(data->config.security));

	err |= get_user(u, &(data32->config.buff_idx));
	err |= put_user(u, &(data->config.buff_idx));

	err |= get_user(u, &(data32->config.interface_idx));
	err |= put_user(u, &(data->config.interface_idx));

	err |= get_user(u, &(data32->config.frm_sequence));
	err |= put_user(u, &(data->config.frm_sequence));

	return err;
}
/*
static int compat_put_disp_session_output_config(
		struct compat_disp_session_output_config __user *data32,
		disp_session_output_config __user *data)
{
	compat_uint_t u;
	compat_uptr_t p;
	int err = 0;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(p, &(data->config.va));
	err |= put_user(p, &(data32->config.va));

	err |= get_user(p, &(data->config.pa));
	err |= put_user(p, &(data32->config.pa));

	err |= get_user(u, &(data->config.fmt));
	err |= put_user(u, &(data32->config.fmt));

	err |= get_user(u, &(data->config.x));
	err |= put_user(u, &(data32->config.x));

	err |= get_user(u, &(data->config.y));
	err |= put_user(u, &(data32->config.y));

	err |= get_user(u, &(data->config.width));
	err |= put_user(u, &(data32->config.width));

	err |= get_user(u, &(data->config.height));
	err |= put_user(u, &(data32->config.height));

	err |= get_user(u, &(data->config.pitch));
	err |= put_user(u, &(data32->config.pitch));

	err |= get_user(u, &(data->config.pitchUV));
	err |= put_user(u, &(data32->config.pitchUV));

	err |= get_user(u, &(data->config.security));
	err |= put_user(u, &(data32->config.security));

	err |= get_user(u, &(data->config.buff_idx));
	err |= put_user(u, &(data32->config.buff_idx));

	err |= get_user(u, &(data->config.interface_idx));
	err |= put_user(u, &(data32->config.interface_idx));

	err |= get_user(u, &(data->config.frm_sequence));
	err |= put_user(u, &(data32->config.frm_sequence));

	return err;
}
*/
static int compat_get_disp_session_input_config(
		struct compat_disp_session_input_config __user *data32,
		disp_session_input_config __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	compat_long_t l;
	compat_uptr_t p;
	int err;
	int j;

	err = get_user(u, &(data32->setter));
	err |= put_user(u, &(data->setter));

	err |= get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(l, &(data32->config_layer_num));
	err |= put_user(l, &(data->config_layer_num));

	for (j = 0; j < ARRAY_SIZE(data32->config); j++) {
		err |= get_user(u, &(data32->config[j].layer_id));
		err |= put_user(u, &(data->config[j].layer_id));

		err |= get_user(u, &(data32->config[j].layer_enable));
		err |= put_user(u, &(data->config[j].layer_enable));

		err |= get_user(u, &(data32->config[j].buffer_source));
		err |= put_user(u, &(data->config[j].buffer_source));

		err |= get_user(p, &(data32->config[j].src_base_addr));
		err |= put_user(p, &(data->config[j].src_base_addr));

		err |= get_user(p, &(data32->config[j].src_phy_addr));
		err |= put_user(p, &(data->config[j].src_phy_addr));

		err |= get_user(u, &(data32->config[j].src_direct_link));
		err |= put_user(u, &(data->config[j].src_direct_link));

		err |= get_user(u, &(data32->config[j].src_fmt));
		err |= put_user(u, &(data->config[j].src_fmt));

		err |= get_user(u, &(data32->config[j].src_use_color_key));
		err |= put_user(u, &(data->config[j].src_use_color_key));

		err |= get_user(u, &(data32->config[j].src_pitch));
		err |= put_user(u, &(data->config[j].src_pitch));

		err |= get_user(u, &(data32->config[j].src_offset_x));
		err |= put_user(u, &(data->config[j].src_offset_x));

		err |= get_user(u, &(data32->config[j].src_offset_y));
		err |= put_user(u, &(data->config[j].src_offset_y));

		err |= get_user(u, &(data32->config[j].src_width));
		err |= put_user(u, &(data->config[j].src_width));

		err |= get_user(u, &(data32->config[j].src_height));
		err |= put_user(u, &(data->config[j].src_height));

		err |= get_user(u, &(data32->config[j].tgt_offset_x));
		err |= put_user(u, &(data->config[j].tgt_offset_x));

		err |= get_user(u, &(data32->config[j].tgt_offset_y));
		err |= put_user(u, &(data->config[j].tgt_offset_y));

		err |= get_user(u, &(data32->config[j].tgt_width));
		err |= put_user(u, &(data->config[j].tgt_width));

		err |= get_user(u, &(data32->config[j].tgt_height));
		err |= put_user(u, &(data->config[j].tgt_height));

		err |= get_user(u, &(data32->config[j].layer_rotation));
		err |= put_user(u, &(data->config[j].layer_rotation));

		err |= get_user(u, &(data32->config[j].layer_type));
		err |= put_user(u, &(data->config[j].layer_type));

		err |= get_user(u, &(data32->config[j].video_rotation));
		err |= put_user(u, &(data->config[j].video_rotation));

		err |= get_user(u, &(data32->config[j].isTdshp));
		err |= put_user(u, &(data->config[j].isTdshp));

		err |= get_user(u, &(data32->config[j].next_buff_idx));
		err |= put_user(u, &(data->config[j].next_buff_idx));

		err |= get_user(i, &(data32->config[j].identity));
		err |= put_user(i, &(data->config[j].identity));

		err |= get_user(i, &(data32->config[j].connected_type));
		err |= put_user(i, &(data->config[j].connected_type));

		err |= get_user(u, &(data32->config[j].security));
		err |= put_user(u, &(data->config[j].security));

		err |= get_user(u, &(data32->config[j].alpha_enable));
		err |= put_user(u, &(data->config[j].alpha_enable));

		err |= get_user(u, &(data32->config[j].alpha));
		err |= put_user(u, &(data->config[j].alpha));

		err |= get_user(u, &(data32->config[j].sur_aen));
		err |= put_user(u, &(data->config[j].sur_aen));

		err |= get_user(u, &(data32->config[j].src_alpha));
		err |= put_user(u, &(data->config[j].src_alpha));

		err |= get_user(u, &(data32->config[j].dst_alpha));
		err |= put_user(u, &(data->config[j].dst_alpha));

		err |= get_user(i, &(data32->config[j].frm_sequence));
		err |= put_user(i, &(data->config[j].frm_sequence));

		err |= get_user(u, &(data32->config[j].yuv_range));
		err |= put_user(u, &(data->config[j].yuv_range));
	}
	return err;
}
/*
static int compat_put_disp_session_input_config(
		struct compat_disp_session_input_config __user *data32,
		disp_session_input_config __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	compat_long_t l;
	compat_uptr_t p;
	int err;
	int j;

	err = get_user(u, &(data->setter));
	err |= put_user(u, &(data32->setter));

	err |= get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(l, &(data->config_layer_num));
	err |= put_user(l, &(data32->config_layer_num));

	for (j = 0; j < 8; j++) {
		err |= get_user(u, &(data->config[j].layer_id));
		err |= put_user(u, &(data32->config[j].layer_id));

		err |= get_user(u, &(data->config[j].layer_enable));
		err |= put_user(u, &(data32->config[j].layer_enable));

		err |= get_user(u, &(data->config[j].buffer_source));
		err |= put_user(u, &(data32->config[j].buffer_source));

		err |= get_user(p, &(data->config[j].src_base_addr));
		err |= put_user(p, &(data32->config[j].src_base_addr));

		err |= get_user(p, &(data->config[j].src_phy_addr));
		err |= put_user(p, &(data32->config[j].src_phy_addr));

		err |= get_user(u, &(data->config[j].src_direct_link));
		err |= put_user(u, &(data32->config[j].src_direct_link));

		err |= get_user(u, &(data->config[j].src_fmt));
		err |= put_user(u, &(data32->config[j].src_fmt));

		err |= get_user(u, &(data->config[j].src_use_color_key));
		err |= put_user(u, &(data32->config[j].src_use_color_key));

		err |= get_user(u, &(data->config[j].src_pitch));
		err |= put_user(u, &(data32->config[j].src_pitch));

		err |= get_user(u, &(data->config[j].src_offset_x));
		err |= put_user(u, &(data32->config[j].src_offset_x));

		err |= get_user(u, &(data->config[j].src_offset_y));
		err |= put_user(u, &(data32->config[j].src_offset_y));

		err |= get_user(u, &(data->config[j].src_width));
		err |= put_user(u, &(data32->config[j].src_width));

		err |= get_user(u, &(data->config[j].src_height));
		err |= put_user(u, &(data32->config[j].src_height));

		err |= get_user(u, &(data->config[j].tgt_offset_x));
		err |= put_user(u, &(data32->config[j].tgt_offset_x));

		err |= get_user(u, &(data->config[j].tgt_offset_y));
		err |= put_user(u, &(data32->config[j].tgt_offset_y));

		err |= get_user(u, &(data->config[j].tgt_width));
		err |= put_user(u, &(data32->config[j].tgt_width));

		err |= get_user(u, &(data->config[j].tgt_height));
		err |= put_user(u, &(data32->config[j].tgt_height));

		err |= get_user(u, &(data->config[j].layer_rotation));
		err |= put_user(u, &(data32->config[j].layer_rotation));

		err |= get_user(u, &(data->config[j].layer_type));
		err |= put_user(u, &(data32->config[j].layer_type));

		err |= get_user(u, &(data->config[j].video_rotation));
		err |= put_user(u, &(data32->config[j].video_rotation));

		err |= get_user(u, &(data->config[j].isTdshp));
		err |= put_user(u, &(data32->config[j].isTdshp));

		err |= get_user(u, &(data->config[j].next_buff_idx));
		err |= put_user(u, &(data32->config[j].next_buff_idx));

		err |= get_user(i, &(data->config[j].identity));
		err |= put_user(i, &(data32->config[j].identity));

		err |= get_user(i, &(data->config[j].connected_type));
		err |= put_user(i, &(data32->config[j].connected_type));

		err |= get_user(u, &(data->config[j].security));
		err |= put_user(u, &(data32->config[j].security));

		err |= get_user(u, &(data->config[j].alpha_enable));
		err |= put_user(u, &(data32->config[j].alpha_enable));

		err |= get_user(u, &(data->config[j].alpha));
		err |= put_user(u, &(data32->config[j].alpha));

		err |= get_user(u, &(data->config[j].sur_aen));
		err |= put_user(u, &(data32->config[j].sur_aen));

		err |= get_user(u, &(data->config[j].src_alpha));
		err |= put_user(u, &(data32->config[j].src_alpha));

		err |= get_user(u, &(data->config[j].dst_alpha));
		err |= put_user(u, &(data32->config[j].dst_alpha));

		err |= get_user(i, &(data->config[j].frm_sequence));
		err |= put_user(i, &(data32->config[j].frm_sequence));

		err |= get_user(u, &(data->config[j].yuv_range));
		err |= put_user(u, &(data32->config[j].yuv_range));
	}
	return err;
}
*/
static int compat_get_disp_session_vsync_config(
			struct compat_disp_session_vsync_config __user *data32,
			disp_session_vsync_config __user *data)
{
	compat_uint_t u;
	compat_long_t l;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->vsync_cnt));
	err |= put_user(u, &(data->vsync_cnt));

	err |= get_user(l, &(data32->vsync_ts));
	err |= put_user(l, &(data->vsync_ts));

	return err;
}
/*
static int compat_put_disp_session_vsync_config(
		struct compat_disp_session_vsync_config __user *data32,
					disp_session_vsync_config __user *data)
{
	compat_uint_t u;
	compat_long_t l;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->vsync_cnt));
	err |= put_user(u, &(data32->vsync_cnt));

	err |= get_user(l, &(data->vsync_ts));
	err |= put_user(l, &(data32->vsync_ts));

	return err;
}
*/

static int compat_get_disp_session_info(
		struct compat_disp_session_info __user *data32,
					disp_session_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data32->session_id));
	err |= put_user(u, &(data->session_id));

	err |= get_user(u, &(data32->maxLayerNum));
	err |= put_user(u, &(data->maxLayerNum));

	err |= get_user(u, &(data32->isHwVsyncAvailable));
	err |= put_user(u, &(data->isHwVsyncAvailable));

	err |= get_user(i, &(data32->displayType));
	err |= put_user(i, &(data->displayType));

	err |= get_user(u, &(data32->displayWidth));
	err |= put_user(u, &(data->displayWidth));

	err |= get_user(u, &(data32->displayHeight));
	err |= put_user(u, &(data->displayHeight));

	err |= get_user(i, &(data32->displayFormat));
	err |= put_user(i, &(data->displayFormat));

	err |= get_user(u, &(data32->displayMode));
	err |= put_user(u, &(data->displayMode));

	err |= get_user(i, &(data32->vsyncFPS));
	err |= put_user(i, &(data->vsyncFPS));

	err |= get_user(u, &(data32->physicalWidth));
	err |= put_user(u, &(data->physicalWidth));

	err |= get_user(i, &(data32->physicalHeight));
	err |= put_user(i, &(data->physicalHeight));


	err |= get_user(i, &(data32->isConnected));
	err |= put_user(i, &(data->isConnected));

	err |= get_user(u, &(data32->isHDCPSupported));
	err |= put_user(u, &(data->isHDCPSupported));

	err |= get_user(i, &(data32->isOVLDisabled));
	err |= put_user(i, &(data->isOVLDisabled));

	err |= get_user(i, &(data32->is3DSupport));
	err |= put_user(i, &(data->is3DSupport));

	err |= get_user(u, &(data32->const_layer_num));
	err |= put_user(u, &(data->const_layer_num));

	return err;

}

static int compat_put_disp_session_info(
		struct compat_disp_session_info __user *data32,
		disp_session_info __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(u, &(data->session_id));
	err |= put_user(u, &(data32->session_id));

	err |= get_user(u, &(data->maxLayerNum));
	err |= put_user(u, &(data32->maxLayerNum));

	err |= get_user(u, &(data->isHwVsyncAvailable));
	err |= put_user(u, &(data32->isHwVsyncAvailable));

	err |= get_user(i, &(data->displayType));
	err |= put_user(i, &(data32->displayType));

	err |= get_user(u, &(data->displayWidth));
	err |= put_user(u, &(data32->displayWidth));

	err |= get_user(u, &(data->displayHeight));
	err |= put_user(u, &(data32->displayHeight));

	err |= get_user(i, &(data->displayFormat));
	err |= put_user(i, &(data32->displayFormat));

	err |= get_user(u, &(data->displayMode));
	err |= put_user(u, &(data32->displayMode));

	err |= get_user(i, &(data->vsyncFPS));
	err |= put_user(i, &(data32->vsyncFPS));

	err |= get_user(u, &(data->physicalWidth));
	err |= put_user(u, &(data32->physicalWidth));

	err |= get_user(i, &(data->physicalHeight));
	err |= put_user(i, &(data32->physicalHeight));


	err |= get_user(i, &(data->isConnected));
	err |= put_user(i, &(data32->isConnected));

	err |= get_user(u, &(data->isHDCPSupported));
	err |= put_user(u, &(data32->isHDCPSupported));

	err |= get_user(i, &(data->isOVLDisabled));
	err |= put_user(i, &(data32->isOVLDisabled));

	err |= get_user(i, &(data->is3DSupport));
	err |= put_user(i, &(data32->is3DSupport));
	return err;

}

static int compat_get_disp_buffer_info(
		struct compat_disp_buffer_info __user *data32,
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

static int compat_put_disp_buffer_info(
		struct compat_disp_buffer_info __user *data32,
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

static int compat_get_disp_present_fence(compat_disp_present_fence * data32 __user,
									disp_present_fence * data __user)
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


static int compat_put_disp_present_fence(
		struct compat_disp_present_fence __user *data32,
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


static int compat_get_disp_session_config(
			struct compat_disp_session_config __user *data32,
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

	return err;
}

static int compat_put_disp_session_config(
		struct compat_disp_session_config __user *data32,
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

	return err;
}

#if 0
static void compat_convert_get_disp_session_config(
	struct compat_disp_session_config *data32,
	disp_session_config *data){
	data->type = data32->type;
	data->device_id = data32->device_id;
	data->mode = data32->mode;
	data->session_id = data32->session_id;
	data->user = data32->user;
	data->present_fence_idx = data32->present_fence_idx;
	data->dc_type = data32->dc_type;
	data->need_merge = data32->need_merge;
}

static void compat_convert_put_disp_session_config(
		struct compat_disp_session_config *data32,
		disp_session_config *data)
{
	data32->type = data->type;
	data32->device_id = data->device_id;
	data32->mode = data->mode;
	data32->session_id = data->session_id;
	data32->user = data->user;
	data32->present_fence_idx = data->present_fence_idx;
	data32->dc_type = data->dc_type;
	data32->need_merge = data->need_merge;
}
#endif

int _compat_ioctl_create_session(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_CREATE_SESSION\n");

#if 0
	unsigned long argd;
	struct compat_disp_session_config compat_session_config;
	disp_session_config data;

	argd = (unsigned long)compat_ptr(arg);

	if (copy_from_user(&compat_session_config, (void __user *)argd, sizeof(compat_session_config))) {
		DISPERR("[DISPMGR]: copy_from_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	} else {
		compat_convert_get_disp_session_config(&compat_session_config, &data);
		if ((data.type == DISP_SESSION_MEMORY) && (get_ovl1_to_mem_on() == false)) {
			DISPMSG("[DISPMGR]: _ioctl_create_session! line:%d  %d\n", __LINE__, get_ovl1_to_mem_on());
			return -EFAULT;
		}

		if (disp_create_session(&data) != 0)
			ret = -EFAULT;

		compat_convert_put_disp_session_config(&compat_session_config, &data);
		if (copy_to_user((void *)argd __user, &compat_session_config, sizeof(compat_session_config))) {
			DISPERR("[DISPMGR]: copy_to_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}
	}
	return ret;

#else

	struct compat_disp_session_config __user *data32;
	disp_session_config __user *data;

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
#endif
	return ret;
}

int _compat_ioctl_destroy_session(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_DESTROY_SESSION\n");

	struct compat_disp_session_config *data32 __user;
	disp_session_config *data __user;

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

	DISPCHECK("COMPAT_DISP_IOCTL_TRIGGER_SESSION\n");

	struct compat_disp_session_config *data32 __user;

	disp_session_config *data __user;

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

	DISPCHECK("COMPAT_DISP_IOCTL_TRIGGER_SESSION\n");

	struct compat_disp_present_fence *data32 __user;

	disp_present_fence *data __user;

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

int _compat_ioctl_get_info(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_GET_INFO\n");

	compat_disp_session_info *data32 __user;

	disp_session_info *data __user;

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


int _compat_ioctl_prepare_buffer(struct file *file, unsigned long arg, ePREPARE_FENCE_TYPE type)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_PREPARE_BUFFER\n");

	compat_disp_buffer_info *data32 __user;

	disp_buffer_info *data __user;

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

	if (PREPARE_INPUT_FENCE == type)
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


int _compat_ioctl_wait_vsync(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_WAIT_VSYNC\n");

	compat_disp_session_vsync_config *data32 __user;

	disp_session_vsync_config *data __user;

	data32 = (compat_disp_session_vsync_config *)compat_ptr(arg);

	data = compat_alloc_user_space(sizeof(disp_session_vsync_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_vsync_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp_buffer_info fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_WAIT_FOR_VSYNC, (unsigned long)data);

	return ret;
}

int _compat_ioctl_set_input_buffer(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_SET_INPUT_BUFFER\n");

	compat_disp_session_input_config *data32 __user;

	disp_session_input_config *data __user;

	data32 = (compat_disp_session_input_config *)compat_ptr(arg);
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

int _compat_ioctl_get_display_caps(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_caps_info *data32 __user;

	disp_caps_info *data __user;

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

int _compat_ioctl_set_vsync(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;
	int *pdata;

	compat_uint_t *data32 __user;

	unsigned int data __user;

	data32 = compat_ptr(arg);

	if (get_user(data, data32)) {
		DISPERR("compat_get_fps fail!\n");
		return err;
	}
	pdata = &data;
	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_VSYNC_FPS, (unsigned long)pdata);
	return ret;
}

int _compat_ioctl_set_session_mode(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	compat_disp_session_config *data32 __user;
	disp_session_config *data __user;

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

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_SESSION_MODE, (unsigned long)data);

	return ret;
}

int _compat_ioctl_set_output_buffer(struct file *file, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	DISPCHECK("COMPAT_DISP_IOCTL_SET_OUTPUT_BUFFER\n");

	compat_disp_session_output_config *data32 __user;
	disp_session_output_config *data __user;

	data32 = (compat_disp_session_output_config *)compat_ptr(arg);
	data = compat_alloc_user_space(sizeof(disp_session_output_config));

	if (data == NULL) {
		DISPERR("compat_alloc_user_space fail!\n");
		return -EFAULT;
	}

	err = compat_get_disp_session_output_config(data32, data);
	if (err) {
		DISPERR("compat_get_disp__session_output_config fail!\n");
		return err;
	}

	ret = file->f_op->unlocked_ioctl(file, DISP_IOCTL_SET_OUTPUT_BUFFER, (unsigned long)data);

	return ret;
}


#endif
