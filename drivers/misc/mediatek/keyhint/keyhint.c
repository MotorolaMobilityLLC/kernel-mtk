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

#include <linux/key.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <mt-plat/keyhint.h>

/* #define KH_DEBUG */

#ifdef KH_DEBUG
#define kh_info(fmt, ...)  pr_info(fmt, ##__VA_ARGS__)
#else
#define kh_info(fmt, ...)
#endif

int kh_register(struct kh_dev *dev, unsigned int key_bits,
		unsigned int key_slot)
{
	int size;

	if (dev->flags & KH_FLAG_INITIALIZED) {
		pr_info("already registered, dev 0x%p\n", dev);
		return -1;
	}

	if (key_bits % (sizeof(unsigned int) * BITS_PER_LONG)) {
		pr_info("key_bits %u shall be multiple of %u\n",
			key_bits, BITS_PER_LONG);
	}

	size = ((key_bits / BITS_PER_BYTE) / sizeof(unsigned long)) * key_slot;

	pr_info("key_bits=%u, key_slot=%u, size=%u bytes\n",
		key_bits, key_slot, size);

	dev->kh = kzalloc(size, GFP_KERNEL);

	size = key_slot * sizeof(unsigned long);

	dev->kh_last_access = kzalloc(size, GFP_KERNEL);

	if (dev->kh && dev->kh_last_access) {
		dev->kh_num_slot = key_slot;
		dev->kh_unit_per_key =
			(key_bits / BITS_PER_BYTE) /
			sizeof(unsigned long);
		dev->kh_active_slot = 0;
		pr_info("register ok, dev=0x%p, unit_per_key=%d\n",
			dev, dev->kh_unit_per_key);

		dev->flags |= KH_FLAG_INITIALIZED;
		return 0;
	}

	pr_info("register fail, dev=0x%p\n", dev);

	return -1;
}

static unsigned int kh_get_free_slot(struct kh_dev *dev)
{
	int i, min_slot;
	unsigned long min_time = LONG_MAX;

	if (dev->kh_active_slot < dev->kh_num_slot) {
		dev->kh_active_slot++;

		kh_info("new, slot=%d\n", (dev->kh_active_slot - 1));

		return (dev->kh_active_slot - 1);
	}

	min_slot = dev->kh_active_slot;

	for (i = 0; i < dev->kh_active_slot; i++) {
		if (dev->kh_last_access[i] < min_time) {
			min_time = dev->kh_last_access[i];
			min_slot = i;
		}
	}

	kh_info("vic, slot=%d, min_time=%lu\n", min_slot, min_time);

	return min_slot;
}

int kh_get_hint(struct kh_dev *dev, const char *key, int *need_update)
{
	int i, j, matched, matched_slot;
	unsigned long *ptr_kh, *ptr_key;

	if (!dev->kh || !need_update) {
		kh_info("get, err, key=0x%lx\n", *(unsigned long *)key);
		return -1;
	}

	/* round 1: simple match */

	matched = 0;
	matched_slot = 0;
	ptr_kh = (unsigned long *)dev->kh;
	ptr_key = (unsigned long *)key;

	for (i = 0; i < dev->kh_active_slot; i++) {

		if (*ptr_kh == *ptr_key) {
			matched_slot = i;
			matched++;
		}

		ptr_kh += dev->kh_unit_per_key;
	}

	if (matched == 1) {

		/* fully match rest part to ensure 100% matched */

		ptr_kh = (unsigned long *)dev->kh;
		ptr_kh += (dev->kh_unit_per_key * matched_slot);

		for (i = 0; i < dev->kh_unit_per_key - 1; i++) {

			ptr_kh++;
			ptr_key++;

			if (*ptr_kh != *ptr_key) {

				matched = 0;
				break;
			}
		}

		if (matched) {
			*need_update = 0;
			dev->kh_last_access[matched_slot] = jiffies;

			kh_info("get, 1, %d, key=0x%lx\n",
				matched_slot, *(unsigned long *)key);

			return matched_slot;
		}
	}

	/* round 2: full match if simple match finds multiple targets */

	if (matched) {

		matched = 0;

		for (i = 0; i < dev->kh_active_slot; i++) {

			ptr_kh = (unsigned long *)dev->kh;
			ptr_kh += (i * dev->kh_unit_per_key);
			ptr_key = (unsigned long *)key;

			for (j = 0; j < dev->kh_unit_per_key; j++) {
				if (*ptr_kh++ != *ptr_key++)
					break;
			}

			if (j == dev->kh_unit_per_key) {
				*need_update = 0;
				dev->kh_last_access[i] = jiffies;

				kh_info("get, 2, %d, key=0x%lx\n",
					i, *(unsigned long *)key);

				return i;
			}
		}
	}

	/* nothing matched, add new hint */

	j = kh_get_free_slot(dev);
	ptr_kh = (unsigned long *)dev->kh;
	ptr_kh += (j * dev->kh_unit_per_key);
	ptr_key = (unsigned long *)key;

	for (i = 0; i < dev->kh_unit_per_key; i++)
		*ptr_kh++ = *ptr_key++;

	dev->kh_last_access[j] = jiffies;

	*need_update = 1;

	kh_info("get, n, %d, key=0x%lx\n", j, *(unsigned long *)key);

	return j;
}

int kh_reset(struct kh_dev *dev)
{
	if (!dev->kh)
		return -1;

	dev->kh_active_slot = 0;

	kh_info("rst, dev=0x%p\n", dev);

	return 0;
}

MODULE_AUTHOR("Stanley Chu <stanley.chu@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Key Hint");

