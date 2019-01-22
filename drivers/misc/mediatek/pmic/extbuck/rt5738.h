/*
 *  Include header file to Richtek RT5738 Regulator Driver
 *
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_RT5738_H
#define __LINUX_RT5738_H

#ifdef CONFIG_RT_REGMAP
#include "../../rt-regmap/rt-regmap.h"
#endif /* CONFIG_RT_REGMAP */

struct rt5738_chip {
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex io_lock;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
#endif /* #ifdef CONFIG_RT_REGMAP */
};

#define RT5738_REG_VSEL0	(0x00)
#define RT5738_REG_VSEL1	(0x01)
#define RT5738_REG_CTRL1	(0x02)
#define RT5738_REG_ID1		(0x03)
#define RT5738_REG_ID2		(0x04)
#define RT5738_REG_MONITOR	(0x05)
#define RT5738_REG_CTRL2	(0x06)
#define RT5738_REG_CTRL3	(0x07)
#define RT5738_REG_CTRL4	(0x08)

extern int rt5738_read_byte(void *client, uint32_t addr, uint32_t *val);
extern int rt5738_write_byte(void *client, uint32_t addr, uint32_t value);
extern int rt5738_assign_bit(void *client, uint32_t reg,
					uint32_t  mask, uint32_t data);
#define rt5738_set_bit(i2c, reg, mask) \
	rt5738_assign_bit(i2c, reg, mask, mask)

#define rt5738_clr_bit(i2c, reg, mask) \
	rt5738_assign_bit(i2c, reg, mask, 0x00)

extern int rt5738_regulator_init(struct rt5738_chip *chip);
extern int rt5738_regulator_deinit(struct rt5738_chip *chip);

#define RT5738_INFO(format, args...) pr_info(format, ##args)
#define RT5738_ERR(format, args...)	pr_err(format, ##args)

#endif /* __LINUX_RT5738 */
