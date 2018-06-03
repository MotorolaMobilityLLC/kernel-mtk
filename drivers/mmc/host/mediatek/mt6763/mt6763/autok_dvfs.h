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

#ifndef _AUTOK_DVFS_H_
#define _AUTOK_DVFS_H_

/* #define SDIO_HQA */

#include <mtk_vcorefs_manager.h>
#include <mtk_spm_vcore_dvfs.h>
#include "autok.h"

#define SDIO_DVFS_TIMEOUT       (HZ/100 * 5)    /* 10ms x5 */

/* Enable later@Peter */
/* #define SDIO_HW_DVFS_CONDITIONAL */

enum AUTOK_VCORE {
	AUTOK_VCORE_LOW = 0,
	AUTOK_VCORE_HIGH,
	AUTOK_VCORE_NUM
};

/**********************************************************
* Function Declaration                                    *
**********************************************************/
extern int sdio_autok_res_exist(struct msdc_host *host);
extern int sdio_autok_res_apply(struct msdc_host *host, int vcore);
extern int sdio_autok_res_save(struct msdc_host *host, int vcore, u8 *res);
extern void sdio_autok_wait_dvfs_ready(void);
extern int emmc_execute_dvfs_autok(struct msdc_host *host, u32 opcode, u8 *res);
extern int sd_execute_dvfs_autok(struct msdc_host *host, u32 opcode, u8 *res);
extern void sdio_execute_dvfs_autok(struct msdc_host *host);


extern int autok_res_check(u8 *res_h, u8 *res_l);
extern void sdio_set_hw_dvfs(int vcore, int done, struct msdc_host *host);
extern void sdio_dvfs_reg_restore(struct msdc_host *host);

#endif /* _AUTOK_DVFS_H_ */

