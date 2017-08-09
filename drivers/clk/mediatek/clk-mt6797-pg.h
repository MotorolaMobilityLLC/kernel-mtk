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

#ifndef __DRV_CLK_MT6797_PG_H
#define __DRV_CLK_MT6797_PG_H

enum subsys_id {
	SYS_MD1 = 0,
	SYS_CONN = 1,
	SYS_DIS = 2,
	SYS_ISP = 3,
	SYS_VDE = 4,
	SYS_MFG_CORE3 = 5,
	SYS_MFG_CORE2 = 6,
	SYS_MFG_CORE1 = 7,
	SYS_MFG_CORE0 = 8,
	SYS_MFG = 9,
	SYS_MFG_ASYNC = 10,
	SYS_MJC = 11,
	SYS_VEN = 12,
	SYS_AUDIO = 13,
	SYS_C2K = 14,
	NR_SYSS = 15,
};

struct pg_callbacks {
	void (*before_off)(enum subsys_id sys);
	void (*after_on)(enum subsys_id sys);
};

/* register new pg_callbacks and return previous pg_callbacks. */
extern struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb);
extern int spm_topaxi_protect(unsigned int mask_value, int en);
/*ram console api*/
/*
[0] bus protect reg
[1] pwr_status
[2] pwr_status 2
[others] local function use
*/
#ifdef CONFIG_MTK_RAM_CONSOLE
extern void aee_rr_rec_clk(int id, u32 val);
#endif
#endif				/* __DRV_CLK_MT6755_PG_H */
