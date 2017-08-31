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

#ifndef __DRV_CLK_MT6799_PG_H
#define __DRV_CLK_MT6799_PG_H
#if 0
enum subsys_id {
	SYS_MD1 = 0,
	SYS_CONN = 1,
	SYS_DIS = 2,/*mm0, mm1*/
	SYS_MFG = 3,
	SYS_ISP = 4,
	SYS_VDE = 5,
	SYS_VEN = 6,
	SYS_MFG_ASYNC = 7,
	SYS_AUDIO = 8,
	SYS_CAM = 9,
	SYS_C2K = 10,
	SYS_MDSYS_INTF_INFRA = 11,
	SYS_MFG_CORE1 = 12,
	SYS_MFG_CORE0 = 13,
	NR_SYSS = 14,
};
#endif
enum subsys_id {
	SYS_MD1 = 0,
	SYS_MM0 = 1,/*mm0, mm1*/
	SYS_MM1 = 2,
	SYS_MFG0 = 3,
	SYS_MFG1 = 4,
	SYS_MFG2 = 5,
	SYS_MFG3 = 6,
	SYS_ISP = 7,
	SYS_VDE = 8,
	SYS_VEN = 9,
	SYS_IPU_SHUTDOWN = 10,
	SYS_IPU_SLEEP = 11,
	SYS_AUDIO = 12,
	SYS_CAM = 13,
	SYS_C2K = 14,
	SYS_MJC = 15,
	NR_SYSS = 16,
};

struct pg_callbacks {
	struct list_head list;
	void (*before_off)(enum subsys_id sys);
	void (*after_on)(enum subsys_id sys);
};

/* register new pg_callbacks and return previous pg_callbacks. */
extern struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb);
extern int spm_topaxi_protect(unsigned int mask_value, int en);
extern void switch_mfg_clk(int src);
extern void subsys_if_on(void);
extern void mtcmos_force_off(void);
extern void mm0_mtcmos_patch(int on);
extern void ven_mtcmos_patch(int on);
extern void ipu_mtcmos_patch(int on);
extern void isp_mtcmos_patch(int on);
extern void mjc_mtcmos_patch(int on);
extern void vde_mtcmos_patch(int on);
extern void cam_mtcmos_patch(int on);
extern void check_mjc_clk_sts(void);
/*ram console api*/
/*
*[0] bus protect reg
*[1] pwr_status
*[2] pwr_status 2
*[others] local function use
*/
#ifdef CONFIG_MTK_RAM_CONSOLE
extern void aee_rr_rec_clk(int id, u32 val);
#endif
#endif/* __DRV_CLK_MT6799_PG_H */
