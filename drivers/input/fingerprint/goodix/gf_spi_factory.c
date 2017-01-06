/* Goodix's GF316M/GF318M/GF3118M/GF518M/GF5118M/
 *	GF516M/GF816M/GF3208/GF5206/GF5216/GF5208
 *  fingerprint sensor linux driver for factory test
 *
 * 2010 - 2015 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <net/sock.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

/* MTK header */
#include "mt_spi.h"
#include "mt_spi_hal.h"
#include "mt_gpio.h"
#include "mach/gpio_const.h"

#include "gf_spi_tee.h"

int  gf_ioctl_spi_init_cfg_cmd(struct mt_chip_conf *mcc, unsigned long arg)
{
	return 0;
}

int gf_ioctl_transfer_raw_cmd(struct gf_device *gf_dev,
			      unsigned long arg, unsigned int bufsiz)
{
	return 0;
}

