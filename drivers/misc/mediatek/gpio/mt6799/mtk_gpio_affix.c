/*
 * Copyright (C) 2015 MediaTek Inc.
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


#include <linux/types.h>
#include "mt-plat/sync_write.h"
#include <linux/types.h>
#include <linux/device.h>
/* #include <mach/mt_gpio.h> */
#include <mt-plat/mtk_gpio_core.h>
#include <6757_gpio.h>


void mt_gpio_pin_decrypt(unsigned long *cipher)
{
	/* just for debug, find out who used pin number directly */
	if ((*cipher & (0x80000000)) == 0) {
		GPIOERR("GPIO%u HARDCODE warning!!!\n", (unsigned int)(*cipher));
		dump_stack();
		/* return; */
	}

	/* GPIOERR("Pin magic number is %x\n",*cipher); */
	*cipher &= ~(0x80000000);
}


/*---------------------------------------------------------------------------*/
static DEVICE_ATTR(pin,      0664, mt_gpio_show_pin,   mt_gpio_store_pin);
/*---------------------------------------------------------------------------*/
static struct device_attribute *gpio_attr_list[] = {
	&dev_attr_pin,
};

/*---------------------------------------------------------------------------*/
int mt_gpio_create_attr(struct device *dev)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(gpio_attr_list)/sizeof(gpio_attr_list[0]));

	if (!dev)
		return -EINVAL;
	for (idx = 0; idx < num; idx++) {
		if (device_create_file(dev, gpio_attr_list[idx]))
			break;
	}
	return err;
}
/*---------------------------------------------------------------------------*/
int mt_gpio_delete_attr(struct device *dev)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(gpio_attr_list)/sizeof(gpio_attr_list[0]));

	if (!dev)
		return -EINVAL;
	for (idx = 0; idx < num; idx++)
		device_remove_file(dev, gpio_attr_list[idx]);
	return err;
}
