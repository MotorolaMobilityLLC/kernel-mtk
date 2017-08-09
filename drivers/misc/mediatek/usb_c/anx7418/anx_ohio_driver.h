/*
 * Copyright(c) 2014, Analogix Semiconductor. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ANX_OHIO_DRV_H
#define _ANX_OHIO_DRV_H

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/async.h>

#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#define LOG_TAG "Ohio"

int ohio_read_reg(uint8_t slave_addr, uint8_t offset, uint8_t *buf);
int ohio_write_reg(uint8_t slave_addr, uint8_t offset, uint8_t value);

bool ohio_is_connected(void);
bool ohio_chip_detect(void);
unchar is_cable_detected(void);

void ohio_power_standby(void);
void ohio_hardware_poweron(void);
void ohio_hardware_powerdown(void);

extern u8 misc_status;

#endif
