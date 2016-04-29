/*
 * wm_adsp.h  --  Wolfson ADSP support
 *
 * Copyright 2012 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __WM_ADSP_H
#define __WM_ADSP_H

#include <linux/circ_buf.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/compress_driver.h>

#include "wmfw.h"

#define WM_ADSP2_REGION_0 BIT(0)
#define WM_ADSP2_REGION_1 BIT(1)
#define WM_ADSP2_REGION_2 BIT(2)
#define WM_ADSP2_REGION_3 BIT(3)
#define WM_ADSP2_REGION_4 BIT(4)
#define WM_ADSP2_REGION_5 BIT(5)
#define WM_ADSP2_REGION_6 BIT(6)
#define WM_ADSP2_REGION_7 BIT(7)
#define WM_ADSP2_REGION_8 BIT(8)
#define WM_ADSP2_REGION_9 BIT(9)
#define WM_ADSP2_REGION_1_9 (WM_ADSP2_REGION_1 | \
		WM_ADSP2_REGION_2 | WM_ADSP2_REGION_3 | \
		WM_ADSP2_REGION_4 | WM_ADSP2_REGION_5 | \
		WM_ADSP2_REGION_6 | WM_ADSP2_REGION_7 | \
		WM_ADSP2_REGION_8 | WM_ADSP2_REGION_9)
#define WM_ADSP2_REGION_ALL (WM_ADSP2_REGION_0 | WM_ADSP2_REGION_1_9)

struct wm_adsp;

struct wm_adsp_region {
	int type;
	unsigned int base;
};

struct wm_adsp_alg_region {
	struct list_head list;
	unsigned int alg;
	int type;
	unsigned int base;
};

struct wm_adsp_buffer_region {
	unsigned int offset;
	unsigned int cumulative_size;
	unsigned int mem_type;
	unsigned int base_addr;
};

struct wm_adsp_buffer_region_def {
	unsigned int mem_type;
	unsigned int base_offset;
	unsigned int size_offset;
};

struct wm_adsp_fw_caps {
	u32 id;
	struct snd_codec_desc desc;
	int num_host_regions;
	struct wm_adsp_buffer_region_def *host_region_defs;
};

struct wm_adsp_fw_defs {
	const char *name;
	const char *file;
	const char *binfile;
	int compr_direction;
	int num_caps;
	struct wm_adsp_fw_caps *caps;
};

struct wm_adsp_fw_features {
	bool shutdown:1;
	bool ez2control_trigger:1;
	bool host_read_buf:1;
};

struct wm_adsp_compr_buf {
	struct mutex lock;
	struct wm_adsp *dsp;
	struct wm_adsp_buffer_region *host_regions;
	u32 host_buf_ptr;
	u32 error;
	u32 irq_ack;
	int read_index;
	int avail;
};

struct wm_adsp_compr {
	struct wm_adsp *dsp;
	struct wm_adsp_compr_buf *buf;

	u32 *capt_buf;

	u32 irq_watermark;
	int max_read_words;

	size_t copied_total;

	struct snd_compr_stream *stream;

	unsigned int sample_rate;
};

struct wm_adsp {
	const char *part;
	char part_rev;
	int num;
	int type;
	int rev;
	struct device *dev;
	struct regmap *regmap;
	struct snd_soc_card *card;

	int base;
	int sysclk_reg;
	int sysclk_mask;
	int sysclk_shift;

	unsigned int rate_cache;
	struct mutex rate_lock;
	int (*rate_put_cb) (struct wm_adsp *adsp, unsigned int mask,
			    unsigned int val);

	struct list_head alg_regions;

	int fw_id;
	int fw_id_version;

	const struct wm_adsp_region *mem;
	int num_mems;

	int fw;
	int fw_ver;
	u32 running;

	struct mutex ctl_lock;

	struct wm_adsp_compr_buf compr_buf;

	int num_firmwares;
	struct wm_adsp_fw_defs *firmwares;

	struct list_head ctl_list;

	struct wm_adsp_fw_features fw_features;

	struct mutex *fw_lock;
	struct work_struct boot_work;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_root;
	struct mutex debugfs_lock;
	char *wmfw_file_name;
	char *bin_file_name;
#endif

	unsigned int lock_regions;

	unsigned int (*hpimp_cb)(struct device *dev);
};

#define WM_ADSP1(wname, num) \
	SND_SOC_DAPM_PGA_E(wname, SND_SOC_NOPM, num, 0, NULL, 0, \
		wm_adsp1_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)

#define WM_ADSP2(wname, num, event_fn) \
{	.id = snd_soc_dapm_dai_link, .name = wname " Preloader", \
	.reg = SND_SOC_NOPM, .shift = num, .event = event_fn, \
	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD }, \
{	.id = snd_soc_dapm_out_drv, .name = wname, \
	.reg = SND_SOC_NOPM, .shift = num, .event = wm_adsp2_event, \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD }

extern const struct snd_kcontrol_new wm_adsp_fw_controls[];

int wm_adsp1_init(struct wm_adsp *dsp);
int wm_adsp2_init(struct wm_adsp *dsp, struct mutex *fw_lock);
void wm_adsp2_remove(struct wm_adsp *dsp);
int wm_adsp2_codec_probe(struct wm_adsp *dsp, struct snd_soc_codec *codec);
int wm_adsp2_codec_remove(struct wm_adsp *dsp, struct snd_soc_codec *codec);
int wm_adsp1_event(struct snd_soc_dapm_widget *w,
		   struct snd_kcontrol *kcontrol, int event);

int wm_adsp2_early_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event,
			 unsigned int freq);

int wm_adsp2_lock(struct wm_adsp *adsp, unsigned int regions);
irqreturn_t wm_adsp2_bus_error(struct wm_adsp *adsp);

int wm_adsp2_event(struct snd_soc_dapm_widget *w,
		   struct snd_kcontrol *kcontrol, int event);

static inline bool wm_adsp_fw_has_voice_trig(const struct wm_adsp *dsp)
{
	return dsp->fw_features.ez2control_trigger;
}

extern int wm_adsp_compr_irq(struct wm_adsp_compr *compr, bool *trigger);
extern int wm_adsp_compr_open(struct wm_adsp_compr *compr,
				struct snd_compr_stream *stream);
extern int wm_adsp_compr_free(struct snd_compr_stream *stream);
extern int wm_adsp_compr_set_params(struct snd_compr_stream *stream,
				    struct snd_compr_params *params);
extern int wm_adsp_compr_get_caps(struct snd_compr_stream *stream,
				  struct snd_compr_caps *caps);
extern int wm_adsp_compr_trigger(struct snd_compr_stream *stream, int cmd);
extern int wm_adsp_compr_pointer(struct snd_compr_stream *stream,
				 struct snd_compr_tstamp *tstamp);
extern int wm_adsp_compr_copy(struct snd_compr_stream *stream,
			      char __user *buf, size_t count);
extern void wm_adsp_compr_init(struct wm_adsp *dsp, struct wm_adsp_compr *compr);
extern void wm_adsp_compr_destroy(struct wm_adsp_compr *compr);

#endif

