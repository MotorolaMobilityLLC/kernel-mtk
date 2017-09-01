/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
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

#ifndef __LINUX_RT5081A_PMU_RGBLED_H
#define __LINUX_RT5081A_PMU_RGBLED_H

enum {
	RT5081A_PMU_LED1 = 0,
	RT5081A_PMU_LED2,
	RT5081A_PMU_LED3,
	RT5081A_PMU_LED4,
	RT5081A_PMU_MAXLED,
};


struct rt5081a_pmu_rgbled_platdata {
	const char *led_name[RT5081A_PMU_MAXLED];
	const char *led_default_trigger[RT5081A_PMU_MAXLED];
};

#define RT5081A_LED_MODEMASK (0x60)
#define RT5081A_LED_MODESHFT (5)

#define RT5081A_LED_PWMDUTYSHFT (0)
#define RT5081A_LED_PWMDUTYMASK (0x1F)
#define RT5081A_LED_PWMDUTYMAX (31)

#define RT5081A_LEDTR1_MASK (0xF0)
#define RT5081A_LEDTR1_SHFT (4)
#define RT5081A_LEDTR2_MASK (0x0F)
#define RT5081A_LEDTR2_SHFT (0)
#define RT5081A_LEDTF1_MASK (0xF0)
#define RT5081A_LEDTF1_SHFT (4)
#define RT5081A_LEDTF2_MASK (0x0F)
#define RT5081A_LEDTF2_SHFT (0)
#define RT5081A_LEDTON_MASK (0xF0)
#define RT5081A_LEDTON_SHFT (4)
#define RT5081A_LEDTOFF_MASK (0x0F)
#define RT5081A_LEDTOFF_SHFT (0)
#define RT5081A_LEDBREATH_MAX (15)

#endif
