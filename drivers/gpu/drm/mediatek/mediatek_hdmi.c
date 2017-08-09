/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <drm/drm_edid.h>
#include <linux/clk.h>
#include "mediatek_hdmi.h"
#include "mediatek_hdmi_hw.h"

static u8 mtk_hdmi_aud_get_chnl_count_from_type(enum hdmi_aud_channel_type
						channel_type)
{
	u8 output_chan_number = 0;

	switch (channel_type) {
	case HDMI_AUD_CHAN_TYPE_1_0:
	case HDMI_AUD_CHAN_TYPE_1_1:
	case HDMI_AUD_CHAN_TYPE_2_0:
		output_chan_number = 2;
		break;
	case HDMI_AUD_CHAN_TYPE_2_1:
	case HDMI_AUD_CHAN_TYPE_3_0:
		output_chan_number = 3;
		break;
	case HDMI_AUD_CHAN_TYPE_3_1:
	case HDMI_AUD_CHAN_TYPE_4_0:
	case HDMI_AUD_CHAN_TYPE_3_0_LRS:
		output_chan_number = 4;
		break;
	case HDMI_AUD_CHAN_TYPE_4_1:
	case HDMI_AUD_CHAN_TYPE_5_0:
	case HDMI_AUD_CHAN_TYPE_3_1_LRS:
	case HDMI_AUD_CHAN_TYPE_4_0_CLRS:
		output_chan_number = 5;
		break;
	case HDMI_AUD_CHAN_TYPE_5_1:
	case HDMI_AUD_CHAN_TYPE_6_0:
	case HDMI_AUD_CHAN_TYPE_4_1_CLRS:
	case HDMI_AUD_CHAN_TYPE_6_0_CS:
	case HDMI_AUD_CHAN_TYPE_6_0_CH:
	case HDMI_AUD_CHAN_TYPE_6_0_OH:
	case HDMI_AUD_CHAN_TYPE_6_0_CHR:
		output_chan_number = 6;
		break;
	case HDMI_AUD_CHAN_TYPE_6_1:
	case HDMI_AUD_CHAN_TYPE_6_1_CS:
	case HDMI_AUD_CHAN_TYPE_6_1_CH:
	case HDMI_AUD_CHAN_TYPE_6_1_OH:
	case HDMI_AUD_CHAN_TYPE_6_1_CHR:
	case HDMI_AUD_CHAN_TYPE_7_0:
	case HDMI_AUD_CHAN_TYPE_7_0_LH_RH:
	case HDMI_AUD_CHAN_TYPE_7_0_LSR_RSR:
	case HDMI_AUD_CHAN_TYPE_7_0_LC_RC:
	case HDMI_AUD_CHAN_TYPE_7_0_LW_RW:
	case HDMI_AUD_CHAN_TYPE_7_0_LSD_RSD:
	case HDMI_AUD_CHAN_TYPE_7_0_LSS_RSS:
	case HDMI_AUD_CHAN_TYPE_7_0_LHS_RHS:
	case HDMI_AUD_CHAN_TYPE_7_0_CS_CH:
	case HDMI_AUD_CHAN_TYPE_7_0_CS_OH:
	case HDMI_AUD_CHAN_TYPE_7_0_CS_CHR:
	case HDMI_AUD_CHAN_TYPE_7_0_CH_OH:
	case HDMI_AUD_CHAN_TYPE_7_0_CH_CHR:
	case HDMI_AUD_CHAN_TYPE_7_0_OH_CHR:
	case HDMI_AUD_CHAN_TYPE_7_0_LSS_RSS_LSR_RSR:
	case HDMI_AUD_CHAN_TYPE_8_0_LH_RH_CS:
		output_chan_number = 7;
		break;

	case HDMI_AUD_CHAN_TYPE_7_1:
	case HDMI_AUD_CHAN_TYPE_7_1_LH_RH:
	case HDMI_AUD_CHAN_TYPE_7_1_LSR_RSR:
	case HDMI_AUD_CHAN_TYPE_7_1_LC_RC:
	case HDMI_AUD_CHAN_TYPE_7_1_LW_RW:
	case HDMI_AUD_CHAN_TYPE_7_1_LSD_RSD:
	case HDMI_AUD_CHAN_TYPE_7_1_LSS_RSS:
	case HDMI_AUD_CHAN_TYPE_7_1_LHS_RHS:
	case HDMI_AUD_CHAN_TYPE_7_1_CS_CH:
	case HDMI_AUD_CHAN_TYPE_7_1_CS_OH:
	case HDMI_AUD_CHAN_TYPE_7_1_CS_CHR:
	case HDMI_AUD_CHAN_TYPE_7_1_CH_OH:
	case HDMI_AUD_CHAN_TYPE_7_1_CH_CHR:
	case HDMI_AUD_CHAN_TYPE_7_1_OH_CHR:
	case HDMI_AUD_CHAN_TYPE_7_1_LSS_RSS_LSR_RSR:
		output_chan_number = 8;

	default:
		output_chan_number = 2;
		break;
	}

	return output_chan_number;
}

static int mtk_hdmi_video_black(struct mediatek_hdmi *hdmi_context)
{
	if (hdmi_context->vid_black)
		return -EINVAL;

	mtk_hdmi_hw_vid_black(hdmi_context, true);

	hdmi_context->vid_black = true;
	return 0;
}

static int mtk_hdmi_video_unblack(struct mediatek_hdmi *hdmi_context)
{
	if (!hdmi_context->vid_black)
		return -EINVAL;

	mtk_hdmi_hw_vid_black(hdmi_context, false);

	hdmi_context->vid_black = false;
	return 0;
}

static int mtk_hdmi_audio_mute(struct mediatek_hdmi *hdmi_context)
{
	if (hdmi_context->aud_mute)
		return -EINVAL;

	mtk_hdmi_hw_aud_mute(hdmi_context, true);

	hdmi_context->aud_mute = true;
	return 0;
}

static int mtk_hdmi_audio_unmute(struct mediatek_hdmi *hdmi_context)
{
	if (!hdmi_context->aud_mute)
		return -EINVAL;

	mtk_hdmi_hw_aud_mute(hdmi_context, false);

	hdmi_context->aud_mute = false;
	return 0;
}

int mtk_hdmi_signal_on(struct mediatek_hdmi *hdmi_context)
{
	if (hdmi_context->output)
		return -EINVAL;

	mtk_hdmi_hw_on_off_tmds(hdmi_context, true);

	hdmi_context->output = true;
	return 0;
}

int mtk_hdmi_signal_off(struct mediatek_hdmi *hdmi_context)
{
	if (!hdmi_context->output)
		return -EINVAL;

	mtk_hdmi_hw_on_off_tmds(hdmi_context, false);

	hdmi_context->output = false;
	return 0;
}

static int mtk_hdmi_video_change_vpll(struct mediatek_hdmi
				       *hdmi_context, u32 clock,
				       enum HDMI_DISPLAY_COLOR_DEPTH depth)
{
	int ret;

	ret = mtk_hdmi_hw_set_clock(hdmi_context, clock);
	if (ret) {
		mtk_hdmi_err("change vpll failed!\n");
		return ret;
	}
	mtk_hdmi_hw_set_pll(hdmi_context, clock, depth);
	mtk_hdmi_hw_config_sys(hdmi_context);
	mtk_hdmi_hw_set_deep_color_mode(hdmi_context, depth);
	return 0;
}

static void mtk_hdmi_video_set_display_mode(struct mediatek_hdmi
					    *hdmi_context,
					    struct drm_display_mode *mode)
{
	mtk_hdmi_hw_reset(hdmi_context, true);
	mtk_hdmi_hw_reset(hdmi_context, false);
	mtk_hdmi_hw_enable_notice(hdmi_context, true);
	mtk_hdmi_hw_write_int_mask(hdmi_context, 0xff);
	mtk_hdmi_hw_enable_dvi_mode(hdmi_context, hdmi_context->dvi_mode);
	mtk_hdmi_hw_ncts_auto_write_enable(hdmi_context, true);

	mtk_hdmi_hw_msic_setting(hdmi_context, mode);
}

static int mtk_hdmi_aud_enable_packet(struct mediatek_hdmi
				       *hdmi_context, bool enable)
{
	mtk_hdmi_hw_send_aud_packet(hdmi_context, enable);
	return 0;
}

static int mtk_hdmi_aud_on_off_hw_ncts(struct mediatek_hdmi
					*hdmi_context, bool on)
{
	mtk_hdmi_hw_ncts_enable(hdmi_context, on);
	return 0;
}

static int mtk_hdmi_aud_set_input(struct mediatek_hdmi *hdmi_context)
{
	u8 chan_count = 0;

	mtk_hdmi_hw_aud_set_channel_swap(hdmi_context, HDMI_AUD_SWAP_LFE_CC);
	mtk_hdmi_hw_aud_raw_data_enable(hdmi_context, true);

	if (hdmi_context->aud_input_type == HDMI_AUD_INPUT_SPDIF &&
	    hdmi_context->aud_codec == HDMI_AUDIO_CODING_TYPE_DST) {
		mtk_hdmi_hw_aud_set_bit_num(hdmi_context,
					    HDMI_AUDIO_SAMPLE_SIZE_24);
	} else if (hdmi_context->aud_i2s_fmt == HDMI_I2S_MODE_LJT_24BIT) {
		hdmi_context->aud_i2s_fmt = HDMI_I2S_MODE_LJT_16BIT;
	}

	mtk_hdmi_hw_aud_set_i2s_fmt(hdmi_context, hdmi_context->aud_i2s_fmt);
	mtk_hdmi_hw_aud_set_bit_num(hdmi_context, HDMI_AUDIO_SAMPLE_SIZE_24);

	mtk_hdmi_hw_aud_set_high_bitrate(hdmi_context, false);
	mtk_hdmi_phy_aud_dst_normal_double_enable(hdmi_context, false);
	mtk_hdmi_hw_aud_dst_enable(hdmi_context, false);

	if (hdmi_context->aud_input_type == HDMI_AUD_INPUT_SPDIF) {
		mtk_hdmi_hw_aud_dsd_enable(hdmi_context, false);
		if (hdmi_context->aud_codec == HDMI_AUDIO_CODING_TYPE_DST) {
			mtk_hdmi_phy_aud_dst_normal_double_enable(hdmi_context,
								  true);
			mtk_hdmi_hw_aud_dst_enable(hdmi_context, true);
		}

		chan_count = mtk_hdmi_aud_get_chnl_count_from_type
						 (HDMI_AUD_CHAN_TYPE_2_0);
		mtk_hdmi_hw_aud_set_i2s_chan_num(hdmi_context,
						 HDMI_AUD_CHAN_TYPE_2_0,
						 chan_count);
		mtk_hdmi_hw_aud_set_input_type(hdmi_context,
					       HDMI_AUD_INPUT_SPDIF);
	} else {
		mtk_hdmi_hw_aud_dsd_enable(hdmi_context, false);
		chan_count =
			mtk_hdmi_aud_get_chnl_count_from_type(
			hdmi_context->aud_input_chan_type);
		mtk_hdmi_hw_aud_set_i2s_chan_num(
			hdmi_context,
			hdmi_context->aud_input_chan_type,
			chan_count);
		mtk_hdmi_hw_aud_set_input_type(hdmi_context,
					       HDMI_AUD_INPUT_I2S);
	}
	return 0;
}

static int mtk_hdmi_aud_set_src(struct mediatek_hdmi *hdmi_context,
				 struct drm_display_mode *mode)
{
	mtk_hdmi_aud_on_off_hw_ncts(hdmi_context, false);

	if (hdmi_context->aud_input_type == HDMI_AUD_INPUT_I2S) {
		switch (hdmi_context->aud_hdmi_fs) {
		case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
		case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
		case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
		case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
		case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
			mtk_hdmi_hw_aud_src_off(hdmi_context);
			/* mtk_hdmi_hw_aud_src_enable(hdmi_context, false); */
			mtk_hdmi_hw_aud_set_mclk(hdmi_context,
						 hdmi_context->aud_mclk);
			mtk_hdmi_hw_aud_aclk_inv_enable(hdmi_context, false);
			break;
		default:
			break;
		}
	} else {
		switch (hdmi_context->iec_frame_fs) {
		case HDMI_IEC_32K:
			hdmi_context->aud_hdmi_fs =
			    HDMI_AUDIO_SAMPLE_FREQUENCY_32000;
			mtk_hdmi_hw_aud_src_off(hdmi_context);
			mtk_hdmi_hw_aud_set_mclk(hdmi_context,
						 HDMI_AUD_MCLK_128FS);
			mtk_hdmi_hw_aud_aclk_inv_enable(hdmi_context, false);
			break;
		case HDMI_IEC_48K:
			hdmi_context->aud_hdmi_fs =
			    HDMI_AUDIO_SAMPLE_FREQUENCY_48000;
			mtk_hdmi_hw_aud_src_off(hdmi_context);
			mtk_hdmi_hw_aud_set_mclk(hdmi_context,
						 HDMI_AUD_MCLK_128FS);
			mtk_hdmi_hw_aud_aclk_inv_enable(hdmi_context, false);
			break;
		case HDMI_IEC_44K:
			hdmi_context->aud_hdmi_fs =
			    HDMI_AUDIO_SAMPLE_FREQUENCY_44100;
			mtk_hdmi_hw_aud_src_off(hdmi_context);
			mtk_hdmi_hw_aud_set_mclk(hdmi_context,
						 HDMI_AUD_MCLK_128FS);
			mtk_hdmi_hw_aud_aclk_inv_enable(hdmi_context, false);
			break;
		default:
			break;
		}
	}
	mtk_hdmi_hw_aud_set_ncts(hdmi_context, hdmi_context->depth,
				 hdmi_context->aud_hdmi_fs, mode->clock);

	mtk_hdmi_hw_aud_src_reenable(hdmi_context);
	return 0;
}

static int mtk_hdmi_aud_set_chnl_status(struct mediatek_hdmi
					 *hdmi_context)
{
	mtk_hdmi_hw_aud_set_channel_status(hdmi_context,
					   hdmi_context->hdmi_l_channel_state,
					   hdmi_context->hdmi_r_channel_state,
					   hdmi_context->aud_hdmi_fs);
	return 0;
}

static int mtk_hdmi_aud_output_config(struct mediatek_hdmi
				       *hdmi_context,
				       struct drm_display_mode *mode)
{
	mtk_hdmi_audio_mute(hdmi_context);
	mtk_hdmi_aud_enable_packet(hdmi_context, false);

	mtk_hdmi_aud_set_input(hdmi_context);
	mtk_hdmi_aud_set_src(hdmi_context, mode);
	mtk_hdmi_aud_set_chnl_status(hdmi_context);

	usleep_range(50, 100);

	mtk_hdmi_aud_on_off_hw_ncts(hdmi_context, true);
	mtk_hdmi_aud_enable_packet(hdmi_context, true);
	mtk_hdmi_audio_unmute(hdmi_context);
	return 0;
}

static int mtk_hdmi_setup_av_mute_packet(struct mediatek_hdmi
					  *hdmi_context)
{
	mtk_hdmi_hw_send_av_mute(hdmi_context);
	return 0;
}

static int mtk_hdmi_setup_av_unmute_packet(struct mediatek_hdmi
					    *hdmi_context)
{
	mtk_hdmi_hw_send_av_unmute(hdmi_context);
	return 0;
}

static int mtk_hdmi_setup_avi_infoframe(struct mediatek_hdmi
					 *hdmi_context,
					 struct drm_display_mode *mode)
{
	struct hdmi_avi_infoframe frame;
	u8 buffer[17];
	ssize_t err;

	err = drm_hdmi_avi_infoframe_from_display_mode(&frame, mode);
	if(err) {
		mtk_hdmi_err
		    (" failed, err = %ld\n",
		     err);
		return err;
	}

	err = hdmi_avi_infoframe_pack(&frame, buffer, sizeof(buffer));
	if(err) {
		mtk_hdmi_err("hdmi_avi_infoframe_pack failed, err = %ld\n",
			     err);
		return err;
	}

	mtk_hdmi_hw_send_info_frame(hdmi_context, buffer, sizeof(buffer));
	return 0;
}

static int mtk_hdmi_setup_spd_infoframe(struct mediatek_hdmi
					 *hdmi_context, const char *vendor,
					 const char *product)
{
	struct hdmi_spd_infoframe frame;
	u8 buffer[29];
	ssize_t err;

	err = hdmi_spd_infoframe_init(&frame, vendor, product);
	if(err) {
		mtk_hdmi_err("hdmi_spd_infoframe_init failed! ,err = %ld\n",
			     err);
		return err;
	}

	err = hdmi_spd_infoframe_pack(&frame, buffer, sizeof(buffer));
	if(err) {
		mtk_hdmi_err("hdmi_spd_infoframe_pack failed! ,err = %ld\n",
			     err);
		return err;
	}

	mtk_hdmi_hw_send_info_frame(hdmi_context, buffer, sizeof(buffer));
	return 0;
}

static int mtk_hdmi_setup_audio_infoframe(struct mediatek_hdmi
					   *hdmi_context)
{
	struct hdmi_audio_infoframe frame;
	u8 buffer[14];
	ssize_t err;

	err = hdmi_audio_infoframe_init(&frame);
	if(err) {
		mtk_hdmi_err("hdmi_audio_infoframe_init failed! ,err = %ld\n",
			     err);
		return err;
	}

	frame.coding_type = HDMI_AUDIO_CODING_TYPE_STREAM;
	frame.sample_frequency = HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM;
	frame.sample_size = HDMI_AUDIO_SAMPLE_SIZE_STREAM;
	frame.channels =
	    mtk_hdmi_aud_get_chnl_count_from_type(hdmi_context->
						  aud_input_chan_type);

	err = hdmi_audio_infoframe_pack(&frame, buffer, sizeof(buffer));
	if(err) {
		mtk_hdmi_err("hdmi_audio_infoframe_pack failed! ,err = %ld\n",
			     err);
		return err;
	}

	mtk_hdmi_hw_send_info_frame(hdmi_context, buffer, sizeof(buffer));
	return 0;
}

static int mtk_hdmi_setup_vendor_specific_infoframe(struct
						     mediatek_hdmi
						     *hdmi_context,
						     struct drm_display_mode
						     *mode)
{
	struct hdmi_vendor_infoframe frame;
	u8 buffer[10];
	ssize_t err;

	err = drm_hdmi_vendor_infoframe_from_display_mode(&frame, mode);
	if(err) {
		mtk_hdmi_err
		    ("failed! err = %ld\n",
		     err);
		return err;
	}

	err = hdmi_vendor_infoframe_pack(&frame, buffer, sizeof(buffer));
	if(err) {
		mtk_hdmi_err("hdmi_vendor_infoframe_pack failed! ,err = %ld\n",
			     err);
		return err;
	}

	mtk_hdmi_hw_send_info_frame(hdmi_context, buffer, sizeof(buffer));
	return 0;
}

int mtk_hdmi_hpd_high(struct mediatek_hdmi *hdmi_context)
{
	return mtk_hdmi_hw_is_hpd_high(hdmi_context);
}

void mtk_hdmi_htplg_irq_clr(struct mediatek_hdmi *hdmi_context)
{
	mtk_hdmi_hw_clear_htplg_irq(hdmi_context);
}

int mtk_hdmi_output_init(void *private_data)
{
	struct mediatek_hdmi *hdmi_context = private_data;

	if (hdmi_context->init)
		return -EINVAL;

	hdmi_context->csp = HDMI_COLORSPACE_RGB;
	hdmi_context->depth = HDMI_DEEP_COLOR_24BITS;
	hdmi_context->aud_mute = false;
	hdmi_context->output = true;
	hdmi_context->vid_black = false;

	hdmi_context->aud_codec = HDMI_AUDIO_CODING_TYPE_PCM;
	hdmi_context->aud_hdmi_fs = HDMI_AUDIO_SAMPLE_FREQUENCY_48000;
	hdmi_context->aud_sampe_size = HDMI_AUDIO_SAMPLE_SIZE_16;
	hdmi_context->aud_input_type = HDMI_AUD_INPUT_I2S;
	hdmi_context->aud_i2s_fmt = HDMI_I2S_MODE_I2S_24BIT;
	hdmi_context->aud_mclk = HDMI_AUD_MCLK_128FS;
	hdmi_context->iec_frame_fs = HDMI_IEC_48K;
	hdmi_context->aud_input_chan_type = HDMI_AUD_CHAN_TYPE_2_0;
	hdmi_context->hdmi_l_channel_state[2] = 2;
	hdmi_context->hdmi_r_channel_state[2] = 2;

	mtk_hdmi_hw_make_reg_writabe(hdmi_context, true);
	mtk_hdmi_hw_1p4_verseion_enable(hdmi_context, true);
	mtk_hdmi_hw_htplg_irq_enable(hdmi_context);

	hdmi_context->init = true;

	return 0;
}

void mtk_hdmi_audio_enable(struct mediatek_hdmi *hctx)
{
	mtk_hdmi_aud_enable_packet(hctx, true);
	hctx->audio_enable = true;
}

void mtk_hdmi_audio_disabe(struct mediatek_hdmi *hctx)
{
	mtk_hdmi_aud_enable_packet(hctx, false);
	hctx->audio_enable = false;
}

int mtk_hdmi_audio_set_param(struct mediatek_hdmi *hctx,
			      struct hdmi_audio_param *param)
{
	if (!hctx->audio_enable) {
		mtk_hdmi_err("hdmi audio is in disable state!\n");
		return -EINVAL;
	}

	hctx->aud_codec = param->aud_codec;
	hctx->aud_hdmi_fs = param->aud_hdmi_fs;
	hctx->aud_sampe_size = param->aud_sampe_size;
	hctx->aud_input_type = param->aud_input_type;
	hctx->aud_i2s_fmt = param->aud_i2s_fmt;
	hctx->aud_mclk = param->aud_mclk;
	hctx->iec_frame_fs = param->iec_frame_fs;
	hctx->aud_input_chan_type = param->aud_input_chan_type;
	memcpy(hctx->hdmi_l_channel_state, param->hdmi_l_channel_state,
	       sizeof(hctx->hdmi_l_channel_state));
	memcpy(hctx->hdmi_r_channel_state, param->hdmi_r_channel_state,
	       sizeof(hctx->hdmi_r_channel_state));

	return mtk_hdmi_aud_output_config(hctx, &hctx->display_node->mode);
}

int mtk_hdmi_detect_dvi_monitor(struct mediatek_hdmi *hctx)
{
	return hctx->dvi_mode;
}

int mtk_hdmi_output_set_display_mode(struct drm_display_mode *display_mode,
				      void *private_data)
{
	struct mediatek_hdmi *hdmi_context = private_data;
	int ret;

	if (!hdmi_context->init) {
		mtk_hdmi_err("doesn't init hdmi control context!\n");
		return -EINVAL;
	}

	mtk_hdmi_video_black(hdmi_context);
	mtk_hdmi_audio_mute(hdmi_context);
	mtk_hdmi_setup_av_mute_packet(hdmi_context);
	mtk_hdmi_signal_off(hdmi_context);

	ret = mtk_hdmi_video_change_vpll(hdmi_context, display_mode->clock * 1000,
				   hdmi_context->depth);
	if (ret){
		mtk_hdmi_err("set vpll failed!\n");
		return ret;
	}
	mtk_hdmi_video_set_display_mode(hdmi_context, display_mode);

	mtk_hdmi_signal_on(hdmi_context);
	mtk_hdmi_aud_output_config(hdmi_context, display_mode);

	mtk_hdmi_setup_audio_infoframe(hdmi_context);
	mtk_hdmi_setup_avi_infoframe(hdmi_context, display_mode);
	mtk_hdmi_setup_spd_infoframe(hdmi_context, "mediatek", "chromebook");
	if (display_mode->flags & DRM_MODE_FLAG_3D_MASK) {
		mtk_hdmi_setup_vendor_specific_infoframe(hdmi_context,
							 display_mode);
	}

	mtk_hdmi_video_unblack(hdmi_context);
	mtk_hdmi_audio_unmute(hdmi_context);
	mtk_hdmi_setup_av_unmute_packet(hdmi_context);

	return 0;
}
