/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-12-22 File created.
 */

#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#include "frsm-dev.h"

static void frsm_mutex_lock(void);
static void frsm_mutex_unlock(void);

static int frsm_reg_read(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t *pval);
static int frsm_reg_write(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t val);
static int frsm_reg_update_bits(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t mask, uint16_t val);
static int frsm_reg_bulk_read(struct frsm_dev *frsm_dev,
		uint8_t reg, void *buf, int buf_size);
static int frsm_reg_bulk_write(struct frsm_dev *frsm_dev,
		uint8_t reg, const void *buf, int buf_size);
static int frsm_reg_read_status(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t *pval);
static int frsm_reg_wait_stable(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t mask, uint16_t val);
static int frsm_reg_dump(struct frsm_dev *frsm_dev, uint8_t reg_max);
static int frsm_reg_write_table(struct frsm_dev *frsm_dev,
		const struct reg_table *reg);

static struct fwm_table *frsm_get_fwm_table(struct frsm_dev *frsm_dev,
		int type);
static int frsm_get_fwm_string(struct frsm_dev *frsm_dev,
		int offset, const char **pstr);
static int frsm_get_scene_count(struct frsm_dev *frsm_dev);
static int frsm_get_scene_name(struct frsm_dev *frsm_dev,
		int scene_id, const char *name);
static int frsm_write_reg_table(struct frsm_dev *frsm_dev, uint16_t offset);
static int frsm_write_model_table(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t offset);
static int frsm_write_effect_table(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t offset);
static int frsm_firmware_init_sync(struct frsm_dev *frsm_dev, const char *name);
static int frsm_firmware_init_async(struct frsm_dev *frsm_dev,
		const char *name);

static int frsm_mntr_switch(struct frsm_dev *frsm_dev, bool enable);
static int frsm_mntr_init(struct frsm_dev *frsm_dev);
static void frsm_mntr_deinit(struct frsm_dev *frsm_dev);

static int frsm_stub_set_spkr_prot(struct frsm_dev *frsm_dev);
static int frsm_stub_start_up(struct frsm_dev *frsm_dev);
static int frsm_stub_unmute(struct frsm_dev *frsm_dev);
static int frsm_stub_mntr_switch(struct frsm_dev *frsm_dev, bool enable);
static int frsm_stub_mute(struct frsm_dev *frsm_dev);
static int frsm_stub_tsmute(struct frsm_dev *frsm_dev, bool mute);
static int frsm_stub_shut_down(struct frsm_dev *frsm_dev);
static int frsm_stub_set_channel(struct frsm_dev *frsm_dev);
static int frsm_stub_set_volume(struct frsm_dev *frsm_dev);

static int frsm_send_event(struct frsm_dev *frsm_dev, enum frsm_event event);
#if IS_ENABLED(CONFIG_SND_SOC_FRSM_AMP)
static int frsm_send_amp_event(int event, void *buf, int buf_size);
#endif

#endif // __INTERNAL_H__
