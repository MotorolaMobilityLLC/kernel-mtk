/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef __KEYHINT_H
#define __KEYHINT_H

#include <linux/key.h>

#define KH_FLAG_INITIALIZED    (1 << 0)

struct kh_dev {
	unsigned long *kh;
	unsigned long *kh_last_access;
	unsigned int kh_num_slot;
	unsigned int kh_unit_per_key;
	unsigned int kh_active_slot;
	unsigned int flags;
};

#ifdef CONFIG_HIE
int kh_get_hint(struct kh_dev *dev, const char *key,
		int *need_update);
int kh_register(struct kh_dev *dev, unsigned int key_bits,
		unsigned int key_slot);
int kh_reset(struct kh_dev *dev);
#else
static inline int kh_get_hint(struct kh_dev *dev, const char *key,
			      int *need_update)
{
	return 0;
}

static inline int kh_register(struct kh_dev *dev, unsigned int key_bits,
			      unsigned int key_slot)
{
	return 0;
}

static inline int kh_reset(struct kh_dev *dev)
{
	return 0;
}
#endif

#endif

