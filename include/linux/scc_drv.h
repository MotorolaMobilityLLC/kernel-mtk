/* 
 * Header of Switching Charger Class Device Driver
 *
 * Copyright (C) 2018 TINNO Corp.
 * Jake.Liang <jake.liang@tinno.com>
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

#ifndef LINUX_POWER_SCC_DRV_H
#define LINUX_POWER_SCC_DRV_H

enum {
	SPEED__0 = 0,
	SPEED__1,
	SPEED__2,
	SPEED__3,
	SPEED__4,
	SPEED__5,
	SPEED__6,
	SPEED__7,
	SPEED__8,
	SPEED__9,
	SPEED__10,
	SPEED__11,
	SPEED__12,
	SPEED_MAX
};

void scc_init_speed_current_map(const char *node_str);
#if 0
void scc_create_file(void);
#endif
int scc_create_file(struct device *dev);
int scc_get_current(void);

// Below interface need implement by platform.
int scc_get_lcd_brightness(void );
int scc_get_temp(void );
int scc_get_capacity(void );

#endif /* LINUX_POWER_SCC_DRV_H */
