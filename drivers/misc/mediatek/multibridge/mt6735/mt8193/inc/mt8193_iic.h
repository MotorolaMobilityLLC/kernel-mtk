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

#ifndef MT8193_IIC_H
#define MT8193_IIC_H

/*----------------------------------------------------------------------------*/
/* IIC APIs                                                                   */
/*----------------------------------------------------------------------------*/
int mt8193_i2c_read(u16 addr, u32 *data);
int mt8193_i2c_write(u16 addr, u32 data);

#endif /* MT8193_IIC_H */

