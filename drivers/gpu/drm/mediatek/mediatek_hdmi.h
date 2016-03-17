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
#ifndef _MEDIATEK_HDMI_CTRL_H
#define _MEDIATEK_HDMI_CTRL_H

#include <linux/types.h>
#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/hdmi.h>
#include <linux/clk.h>
#include <drm/drm_crtc.h>
#include <linux/regmap.h>
#include "mediatek_hdmi_display_core.h"
#include <drm/mediatek/mediatek_hdmi_audio.h>

enum mediatek_hdmi_clk_id {
	MEDIATEK_HDMI_CLK_CEC,
	MEDIATEK_HDMI_CLK_TVDPLL,
	MEDIATEK_HDMI_CLK_DPI_SEL,
	MEDIATEK_HDMI_CLK_DPI_DIV2,
	MEDIATEK_HDMI_CLK_DPI_DIV4,
	MEDIATEK_HDMI_CLK_DPI_DIV8,
	MEDIATEK_HDMI_CLK_HDMI_SEL,
	MEDIATEK_HDMI_CLK_HDMI_DIV1,
	MEDIATEK_HDMI_CLK_HDMI_DIV2,
	MEDIATEK_HDMI_CLK_HDMI_DIV3,
	MEDIATEK_HDMI_CLK_HDMI_PIXEL,
	MEDIATEK_HDMI_CLK_HDMI_PLL,
	MEDIATEK_HDMI_CLK_DPI_PIXEL,
	MEDIATEK_HDMI_CLK_DPI_ENGING,
	MEDIATEK_HDMI_CLK_AUD_BCLK,
	MEDIATEK_HDMI_CLK_AUD_SPDIF,
	MEDIATEK_HDMI_CLK_COUNT
};

struct mediatek_hdmi {
	struct drm_device *drm_dev;
	struct drm_encoder encoder;
	struct drm_connector conn;
	struct i2c_adapter *ddc_adpt;
	struct task_struct *kthread;
	struct clk *clk[MEDIATEK_HDMI_CLK_COUNT];
	#if defined(CONFIG_DEBUG_FS)
	struct dentry *debugfs;
	#endif
	struct mediatek_hdmi_audio_data audio_data;
	struct platform_device_info audio_pdev_info;
	struct platform_device *audio_pdev;
	struct mediatek_hdmi_display_node *display_node;
	bool dvi_mode;
	int flt_n_5v_gpio;
	int flt_n_5v_irq;
	bool hpd;
	int irq;
	void __iomem *sys_regs;
	void __iomem *grl_regs;
	void __iomem *pll_regs;
	void __iomem *cec_regs;
	bool init;
	enum HDMI_DISPLAY_COLOR_DEPTH depth;
	enum hdmi_colorspace csp;
	bool aud_mute;
	bool audio_enable;
	bool vid_black;
	bool output;
	enum hdmi_audio_coding_type aud_codec;
	enum hdmi_audio_sample_frequency aud_hdmi_fs;
	enum hdmi_audio_sample_size aud_sampe_size;
	enum hdmi_aud_input_type aud_input_type;
	enum hdmi_aud_i2s_fmt aud_i2s_fmt;
	enum hdmi_aud_mclk aud_mclk;
	enum hdmi_aud_iec_frame_rate iec_frame_fs;
	enum hdmi_aud_channel_type aud_input_chan_type;
	u8 hdmi_l_channel_state[6];
	u8 hdmi_r_channel_state[6];
	struct mutex hdmi_mutex;/*to do */
};

static inline int mtk_hdmi_clk_sel_div(struct mediatek_hdmi *hdmi,
				       enum mediatek_hdmi_clk_id id,
				       enum mediatek_hdmi_clk_id parent_id)
{
	int ret;

	ret = clk_set_parent(hdmi->clk[id], hdmi->clk[parent_id]);
	if (ret)
		mtk_hdmi_err("enable clk %d failed!\n", id);

	return ret;
}

static inline int mtk_hdmi_clk_set_rate(struct mediatek_hdmi *hdmi,
					enum mediatek_hdmi_clk_id id,
					unsigned long rate)
{
	int ret;

	ret = clk_set_rate(hdmi->clk[id], rate);
	if (ret)
		mtk_hdmi_err("enable clk %d failed!\n", id);

	return ret;
}

static inline void mtk_hdmi_clk_disable(struct mediatek_hdmi *hdmi,
					enum mediatek_hdmi_clk_id id)
{
	clk_disable_unprepare(hdmi->clk[id]);
}

static inline int mtk_hdmi_clk_enable(struct mediatek_hdmi *hdmi,
				      enum mediatek_hdmi_clk_id id)
{
	int ret;

	ret = clk_prepare_enable(hdmi->clk[id]);
	if (ret)
		mtk_hdmi_err("enable clk %d failed!\n", id);

	return ret;
}

#define hdmi_ctx_from_encoder(e)	\
	container_of(e, struct mediatek_hdmi, encoder)
#define hdmi_ctx_from_conn(e)	\
	container_of(e, struct mediatek_hdmi, conn)
int mtk_hdmi_output_init(void *private_data);
int mtk_hdmi_hpd_high(struct mediatek_hdmi *hdmi_context);
void mtk_hdmi_htplg_irq_clr(struct mediatek_hdmi *hdmi_context);
int mtk_hdmi_signal_on(struct mediatek_hdmi *hdmi_context);
int mtk_hdmi_signal_off(struct mediatek_hdmi *hdmi_context);
int mtk_hdmi_output_set_display_mode(struct drm_display_mode *display_mode,
				      void *private_data);
void mtk_hdmi_audio_enable(struct mediatek_hdmi *hctx);
void mtk_hdmi_audio_disabe(struct mediatek_hdmi *hctx);
int mtk_hdmi_audio_set_param(struct mediatek_hdmi *hctx,
			      struct hdmi_audio_param *param);
int mtk_hdmi_detect_dvi_monitor(struct mediatek_hdmi *hctx);
#if defined(CONFIG_DEBUG_FS)
int mtk_drm_hdmi_debugfs_init(struct mediatek_hdmi *hdmi_context);
void mtk_drm_hdmi_debugfs_exit(struct mediatek_hdmi *hdmi_context);
#else
int mtk_drm_hdmi_debugfs_init(struct mediatek_hdmi *hdmi_context)
{
	return 0;
}
void mtk_drm_hdmi_debugfs_exit(struct mediatek_hdmi *hdmi_context)
{

}
#endif
#endif /*  */
