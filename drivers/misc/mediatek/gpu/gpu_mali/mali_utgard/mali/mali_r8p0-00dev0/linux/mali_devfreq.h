/*
 * Copyright (C) 2011-2017 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 *
 *
 */
#ifndef _MALI_DEVFREQ_H_
#define _MALI_DEVFREQ_H_

int mali_devfreq_init(struct mali_device *mdev);

void mali_devfreq_term(struct mali_device *mdev);

#endif
