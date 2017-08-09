/*
 *  fan53528_system.h
 *  OS. dependent utility Header for FAN53528
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __FAN53528_SYSTEM_H
#define __FAN53528_SYSTEM_H

#include <linux/kernel.h>

#define FAN53528_PRINT(format, args...)     pr_info(format, ##args)
#define FAN53528_ERR(format, args...)          pr_err(format, ##args)

struct fan53528_system_intf {
	void (*lock)(void);
	void (*unlock)(void);
	int (*io_read)(unsigned char addr, unsigned char *val);
	int (*io_write)(unsigned char addr, unsigned char value);
};

extern struct fan53528_system_intf fan53528_intf;
extern int drambuck_intf_init(void);

#endif /* __FAN53528_SYSTEM_H */
