/*
 * Header of Richtek Flash LED Driver
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef LINUX_LEDS_RTFLED_H
#define LINUX_LEDS_RTFLED_H
#include "rt-flashlight.h"

struct rt_fled_dev;
typedef int (*rt_hal_fled_init)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_suspend)
		(struct rt_fled_dev *fled_dev, pm_message_t state);
typedef int (*rt_hal_fled_resume)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_set_mode)
		(struct rt_fled_dev *fled_dev, flashlight_mode_t mode);
typedef int (*rt_hal_fled_get_mode)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_strobe)(struct rt_fled_dev *fled_dev);

/* Return value : -EINVAL => selector parameter is
 *				out of range, otherwise current in uA
 */
typedef int (*rt_hal_fled_torch_current_list)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_strobe_current_list)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_timeout_level_list)
				(struct rt_fled_dev *fled_dev, int selector);
/* Return value : -EINVAL => selector parameter is
 *					out of range, otherwise voltage in mV
 */
typedef int (*rt_hal_fled_lv_protection_list)
			(struct rt_fled_dev *fled_dev, int selector);
/* Return value : -EINVAL => selector parameter is
 * out of range, otherwise time in ms
 */
typedef int (*rt_hal_fled_strobe_timeout_list)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_set_torch_current)
	(struct rt_fled_dev *fled_dev, int min_uA, int max_uA, int *selector);
typedef int (*rt_hal_fled_set_strobe_current)
	(struct rt_fled_dev *fled_dev, int min_uA, int max_uA, int *selector);
typedef int (*rt_hal_fled_set_timeout_level)
	(struct rt_fled_dev *fled_dev, int min_uA, int max_uA, int *selector);
typedef int (*rt_hal_fled_set_lv_protection)
	(struct rt_fled_dev *fled_dev, int min_mV, int max_mV, int *selector);
typedef int (*rt_hal_fled_set_strobe_timeout)
	(struct rt_fled_dev *fled_dev, int min_ms, int max_ms, int *selector);
typedef int (*rt_hal_fled_set_torch_current_sel)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_set_strobe_current_sel)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_set_timeout_level_sel)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_set_lv_protection_sel)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_set_strobe_timeout_sel)
				(struct rt_fled_dev *fled_dev, int selector);
typedef int (*rt_hal_fled_get_torch_current_sel)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_strobe_current_sel)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_timeout_level_sel)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_lv_protection_sel)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_strobe_timeout_sel)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_torch_current)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_strobe_current)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_timeout_level)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_lv_protection)(struct rt_fled_dev *fled_dev);
typedef int (*rt_hal_fled_get_strobe_timeout)(struct rt_fled_dev *fled_dev);
/* Return value : not ready, return 0, ready return 1,
 * if failed, return -errno, see definitions in errno.h
 */
typedef int (*rt_hal_fled_get_is_ready)(struct rt_fled_dev *fled_dev);
typedef void (*rt_hal_fled_shutdown)(struct rt_fled_dev *fled_dev);

struct rt_fled_hal {
	rt_hal_fled_init fled_init;
	rt_hal_fled_suspend fled_suspend;
	rt_hal_fled_resume fled_resume;
	rt_hal_fled_set_mode fled_set_mode;
	rt_hal_fled_get_mode fled_get_mode;
	rt_hal_fled_strobe fled_strobe;
	rt_hal_fled_torch_current_list  fled_troch_current_list;
	rt_hal_fled_strobe_current_list fled_strobe_current_list;
	rt_hal_fled_timeout_level_list fled_timeout_level_list;
	rt_hal_fled_lv_protection_list fled_lv_protection_list;
	rt_hal_fled_strobe_timeout_list fled_strobe_timeout_list;
	/* method to set */
	rt_hal_fled_set_torch_current_sel fled_set_torch_current_sel;
	rt_hal_fled_set_strobe_current_sel fled_set_strobe_current_sel;
	rt_hal_fled_set_timeout_level_sel fled_set_timeout_level_sel;
	rt_hal_fled_set_lv_protection_sel fled_set_lv_protection_sel;
	rt_hal_fled_set_strobe_timeout_sel fled_set_strobe_timeout_sel;
	/* method to set, optional */
	rt_hal_fled_set_torch_current fled_set_torch_current;
	rt_hal_fled_set_strobe_current fled_set_strobe_current;
	rt_hal_fled_set_timeout_level fled_set_timeout_level;
	rt_hal_fled_set_lv_protection fled_set_lv_protection;
	rt_hal_fled_set_strobe_timeout fled_set_strobe_timeout;
	/* method to get */
	rt_hal_fled_get_torch_current_sel fled_get_torch_current_sel;
	rt_hal_fled_get_strobe_current_sel fled_get_strobe_current_sel;
	rt_hal_fled_get_timeout_level_sel fled_get_timeout_level_sel;
	rt_hal_fled_get_lv_protection_sel fled_get_lv_protection_sel;
	rt_hal_fled_get_strobe_timeout_sel fled_get_strobe_timeout_sel;
	/* method to get, optional*/
	rt_hal_fled_get_torch_current fled_get_torch_current;
	rt_hal_fled_get_strobe_current fled_get_strobe_current;
	rt_hal_fled_get_timeout_level fled_get_timeout_level;
	rt_hal_fled_get_lv_protection fled_get_lv_protection;
	rt_hal_fled_get_strobe_timeout fled_get_strobe_timeout;
	rt_hal_fled_get_is_ready fled_get_is_ready;
	/* PM shutdown, optional */
	rt_hal_fled_shutdown fled_shutdown;
};

typedef struct rt_fled_dev {
	struct rt_fled_hal *hal;
	struct flashlight_device *flashlight_dev;
	const struct flashlight_properties *init_props;
	char *name;
	char *chip_name;
} rt_fled_dev_t;


/* Public functions
 * argument
 *   @name : Flash LED's name;pass NULL menas "rt-flash-led"
 */

rt_fled_dev_t *rt_fled_get_dev(const char *name);

/* Usage :
 * fled_dev = rt_fled_get_dev("FlashLED1");
 * fled_dev->hal->fled_set_strobe_current(fled_dev,
 *                                        150, 200);
 */

#endif /*LINUX_LEDS_RTFLED_H*/
