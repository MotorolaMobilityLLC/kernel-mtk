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

#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include "mt_gpio_ext.h"

/* #define MAX_GPIO_REG_BITS      16 */
/* #define MAX_GPIO_MODE_PER_REG  5 */
/* #define GPIO_MODE_BITS         3 */
#define GPIOEXT_BASE        (0x0)	/* PMIC GPIO base. */

/* static GPIOEXT_REGS *gpioext_reg = (GPIOEXT_REGS*)(GPIOEXT_BASE); */
/* set extend GPIO */
/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_ext(unsigned long pin, unsigned long dir)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_ext(unsigned long pin, unsigned long enable)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_ext(unsigned long pin, unsigned long enable)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_ext(unsigned long pin, unsigned long enable)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_ext(unsigned long pin, unsigned long select)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_ext(unsigned long pin, unsigned long enable)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_ext(unsigned long pin, unsigned long output)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_ext(unsigned long pin, unsigned long mode)
{
	dump_stack();
	return -1;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_ext(unsigned long pin)
{
	dump_stack();
	return -1;
}
