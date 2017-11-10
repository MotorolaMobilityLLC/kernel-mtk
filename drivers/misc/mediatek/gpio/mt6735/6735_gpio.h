/*
* Copyright (C) 2016 MediaTek Inc.
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
#ifndef _6735_GPIO_H
#define _6735_GPIO_H

#include "mt_gpio_base.h"
#include <linux/slab.h>
#include <linux/device.h>


extern ssize_t mt_gpio_show_pin(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t mt_gpio_store_pin(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
struct device_node *get_gpio_np(void);
extern long gpio_pull_select_unsupport[MAX_GPIO_PIN];
extern long gpio_pullen_unsupport[MAX_GPIO_PIN];
extern long gpio_smt_unsupport[MAX_GPIO_PIN];
void mt_get_md_gpio_debug(char *str);
#endif   /*_6735_GPIO_H*/
