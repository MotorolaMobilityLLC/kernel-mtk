/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2024. All rights reserved.
 * 2024-06-03 File created.
 */

#ifndef __SPKR_AMP_MNGR_H__
#define __SPKR_AMP_MNGR_H__

#include <linux/list.h>
#include <linux/device.h>
#include <sound/soc.h>

#define SPKR_STRING_MAX  (32)
#define SPKR_AMP_MAX     (8)

struct spkr_amp_ops {
	int (*dev_init)(int id, bool force);
	int (*set_mode)(int id, int mode);
	int (*amp_switch)(int id, bool on);
	int (*dev_deinit)(int id);
};

struct spkr_amp {
	struct list_head list;
	const char *chip_model;
	int id;
	struct spkr_amp_ops amp_ops;
};

struct spkr_amp_mngr {
	struct device *dev;
	int ndev_dts;
	int ndev_list;
	int dapm_register;
	char spkr_switch[SPKR_AMP_MAX];
	char amp_mode[SPKR_AMP_MAX];
	const char *spk_prefix;
};

int spkr_amp_dev_register(struct spkr_amp *spkr_amp);
int spkr_amp_dapm_init(struct snd_soc_card *card);

#endif // __SPKR_AMP_MNGR_H__
