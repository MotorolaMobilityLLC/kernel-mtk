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

#ifndef __DRV_CLK_MT6735_PG_H
#define __DRV_CLK_MT6735_PG_H

enum subsys_id {
	SYS_MD1 = 0,
	SYS_MD2 = 1,
	SYS_CONN = 2,
	SYS_DIS = 3,
	SYS_MFG = 4,
	SYS_ISP = 5,
	SYS_VDE = 6,
	SYS_VEN = 7,
	NR_SYSS = 8,
};

struct pg_callbacks {
	void (*before_off)(enum subsys_id sys);
	void (*after_on)(enum subsys_id sys);
};

/* register new pg_callbacks and return previous pg_callbacks. */
extern struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb);

#endif /* __DRV_CLK_MT6735_PG_H */
