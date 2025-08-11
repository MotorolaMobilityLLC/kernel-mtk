/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 */
#ifndef FSA4480_I2C_H
#define FSA4480_I2C_H

#include <linux/of.h>
#include <linux/notifier.h>

enum fsa_function {
	FSA_MIC_GND_SWAP,
	FSA_TYPEC_ACCESSORY_AUDIO,
	FSA_TYPEC_ACCESSORY_NONE,
	FSA_EVENT_MAX,
};

int fsa4480_switch_event(enum fsa_function event);

#endif /* FSA4480_I2C_H */