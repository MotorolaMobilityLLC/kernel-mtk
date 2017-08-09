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
#ifndef _MEDIATEK_HDMI_HW_H
#define _MEDIATEK_HDMI_HW_H

#include <linux/types.h>
#include <linux/hdmi.h>
#include "mediatek_hdmi.h"

void mtk_hdmi_hw_vid_black(struct mediatek_hdmi *hdmi_context,
			   bool black);
void mtk_hdmi_hw_aud_mute(struct mediatek_hdmi *hdmi_context,
			  bool mute);
void mtk_hdmi_hw_on_off_tmds(struct mediatek_hdmi *hdmi_context,
			     bool on);
void mtk_hdmi_hw_send_info_frame(struct mediatek_hdmi *hdmi_context,
				 u8 *buffer, u8 len);
void mtk_hdmi_hw_send_aud_packet(struct mediatek_hdmi *hdmi_context,
				 bool enable);
int mtk_hdmi_hw_set_clock(struct mediatek_hdmi *hctx, u32 clock);
void mtk_hdmi_hw_set_pll(struct mediatek_hdmi *hdmi_context,
			 u32 clock,
			 enum HDMI_DISPLAY_COLOR_DEPTH depth);
void mtk_hdmi_hw_config_sys(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_set_deep_color_mode(struct mediatek_hdmi
				     *hdmi_context,
				     enum HDMI_DISPLAY_COLOR_DEPTH depth);
void mtk_hdmi_hw_send_av_mute(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_send_av_unmute(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_ncts_enable(struct mediatek_hdmi *hdmi_context,
			     bool on);
void mtk_hdmi_hw_aud_set_channel_swap(struct mediatek_hdmi
				      *hdmi_context,
				      enum hdmi_aud_channel_swap_type swap);
void mtk_hdmi_hw_aud_raw_data_enable(struct mediatek_hdmi
				     *hdmi_context, bool enable);
void mtk_hdmi_hw_aud_set_bit_num(struct mediatek_hdmi *hdmi_context,
				 enum hdmi_audio_sample_size bit_num);
void mtk_hdmi_hw_aud_set_high_bitrate(struct mediatek_hdmi
				      *hdmi_context, bool enable);
void mtk_hdmi_phy_aud_dst_normal_double_enable(struct mediatek_hdmi
					       *hdmi_context, bool enable);
void mtk_hdmi_hw_aud_dst_enable(struct mediatek_hdmi *hdmi_context,
				bool enable);
void mtk_hdmi_hw_aud_dsd_enable(struct mediatek_hdmi *hdmi_context,
				bool enable);
void mtk_hdmi_hw_aud_set_i2s_fmt(struct mediatek_hdmi *hdmi_context,
				 enum hdmi_aud_i2s_fmt i2s_fmt);
void mtk_hdmi_hw_aud_set_i2s_chan_num(struct mediatek_hdmi
				      *hdmi_context,
				      enum hdmi_aud_channel_type channel_type,
				      u8 channel_count);
void mtk_hdmi_hw_aud_set_input_type(struct mediatek_hdmi *hdmi_context,
				    enum hdmi_aud_input_type input_type);
void mtk_hdmi_hw_aud_set_channel_status(struct mediatek_hdmi
					*hdmi_context, u8 *l_chan_status,
					u8 *r_chan_staus,
					enum hdmi_audio_sample_frequency
					aud_hdmi_fs);
void mtk_hdmi_hw_aud_src_enable(struct mediatek_hdmi *hdmi_context,
				bool enable);
void mtk_hdmi_hw_aud_set_mclk(struct mediatek_hdmi *hdmi_context,
			      enum hdmi_aud_mclk mclk);
void mtk_hdmi_hw_aud_src_off(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_aud_src_reenable(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_aud_aclk_inv_enable(struct mediatek_hdmi
				      *hdmi_context, bool enable);
void mtk_hdmi_hw_aud_set_ncts(struct mediatek_hdmi *hdmi_context,
			      enum HDMI_DISPLAY_COLOR_DEPTH depth,
			      enum hdmi_audio_sample_frequency freq,
			      int clock);
bool mtk_hdmi_hw_is_hpd_high(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_make_reg_writabe(struct mediatek_hdmi *hdmi_context,
				  bool enable);
void mtk_hdmi_hw_reset(struct mediatek_hdmi *hdmi_context, bool reset);
void mtk_hdmi_hw_enable_notice(struct mediatek_hdmi *hdmi_context,
			       bool enable_notice);
void mtk_hdmi_hw_write_int_mask(struct mediatek_hdmi *hdmi_context,
				u32 int_mask);
void mtk_hdmi_hw_enable_dvi_mode(struct mediatek_hdmi *hdmi_context,
				 bool enable);
void mtk_hdmi_hw_ncts_auto_write_enable(struct mediatek_hdmi
					 *hdmi_context, bool enable);
void mtk_hdmi_hw_msic_setting(struct mediatek_hdmi *hdmi_context,
			      struct drm_display_mode *mode);
void mtk_hdmi_hw_1p4_verseion_enable(struct mediatek_hdmi
				      *hdmi_context, bool enable);
void mtk_hdmi_hw_htplg_irq_enable(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_hw_clear_htplg_irq(struct mediatek_hdmi *hdmi_context);

#endif /*  */
