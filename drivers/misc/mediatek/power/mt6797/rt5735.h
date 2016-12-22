/*
 *  drivers/misc/mediatek/power/mt6797/rt5735.h
 *  Include header file to Richtek RT5735 Regulator Driver
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_RT5735_REGULATOR_H
#define __LINUX_RT5735_REGULATOR_H

#define RT5735_PRODUCT_ID 0x10

enum {
	RT5735_REG_PID = 0x03,
	RT5735_REG_RANGE1_START = RT5735_REG_PID,
	RT5735_REG_RID,
	RT5735_REG_FID,
	RT5735_REG_VID,
	RT5735_REG_RANGE1_END = RT5735_REG_VID,
	RT5735_REG_PROGVSEL1 = 0x10,
	RT5735_REG_RANGE2_START = RT5735_REG_PROGVSEL1,
	RT5735_REG_PROGVSEL0,
	RT5735_REG_PGOOD,
	RT5735_REG_TIME,
	RT5735_REG_COMMAND,
	RT5735_REG_RANGE2_END = RT5735_REG_COMMAND,
	RT5735_REG_LIMCONF = 0x16,
	RT5735_REG_RANGE3_START = RT5735_REG_LIMCONF,
	RT5735_REG_RANGE3_END = RT5735_REG_LIMCONF,
	RT5735_REG_MAX,
};

/* RT5735_REG_PROGVSEL1: 0x10  RT5735_REG_PROGVSEL0: 0x11 */
#define RT5735_VSEL_PROGMASK	0x7f
#define RT5735_VSEL_PROGSHFT	0
#define RT5735_VSEL_ENMASK	0x80

/* RT5735_REG_PGOOD : 0x12 */
#define RT5735_DISCHG_MASK	0x10
#define RT5735_PGDVS_MASK	0x02
#define RT5735_PGDCDC_MASK	0x01

/* RT5735_REG_TIME : 0x13 */
#define RT5735_DVSUP_MASK	0x1c
#define RT5735_DVSUP_SHFT	2

/* RT5735_REG_COMMAND : 0x14 */
#define RT5735_PWMSEL0_MASK	0x80
#define RT5735_PWMSEL1_MASK	0x40
#define RT5735_DVSMODE_MASK	0x20

/* RT5735_REG_LIMCONF : 0x16 */
#define RT5735_DVSDOWN_MASK	0x06
#define RT5735_DVSDOWN_SHFT	1
#define RT5735_IOC_MASK		0xc0
#define RT5735_IOC_SHFT		6
#define RT5735_TPWTH_MASK	0x30
#define RT5735_TPWTH_SHFT	4
#define RT5735_REARM_MASK	0x01

struct rt5735_platform_data {
	u32 pgdvs:1;
	u32 pgdcdc:1;
	u32 dvs_up:3;
	u32 dvs_down:3;
	u32 dvs_mode:1;
	u32 ioc:2;
	u32 tpwth:2;
	u32 rearm:1;
};

#endif /* #ifndef __LINUX_RT5735_REGULATOR_H */
