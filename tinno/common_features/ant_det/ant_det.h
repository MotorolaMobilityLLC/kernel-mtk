/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 */

#ifndef __LINUX_ANT_DET_CONTROL_H
#define __LINUX_ANT_DET_CONTROL_H

#include <linux/types.h>

struct ant_det_data {
	int gpio;
	int irq;
	struct delayed_work det_work;
	struct input_dev *input;
	struct regulator *vdd;
};

	
#define ANT_DET_CMD_PWR_CTRL	0xdeadbeef


#endif /* __LINUX_ANT_DET_CONTROL_H */

