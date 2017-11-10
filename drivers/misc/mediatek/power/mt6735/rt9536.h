/*
 * Charging IC driver (rt9536)
 *
 * Copyright (C) 2010 LGE, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RT9536_CHARGER_H__
#define __RT9536_CHARGER_H__

struct rt9536_platform_data {
	/* gpio */
	int en_set;
	int chgsb;
};

#endif /* __RT9536_CHARGER_H__ */
