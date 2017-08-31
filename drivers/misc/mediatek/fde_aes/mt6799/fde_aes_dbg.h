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

#ifndef __MT_FDE_AES_DBG_H__
#define __MT_FDE_AES_DBG_H__

#include <linux/types.h>

enum {
	FDE_AES_NORMAL = 0,
	FDE_AES_WR_REG  = 1,
	FDE_AES_DUMP = 2,
	FDE_AES_DUMP_ALL = 3,
	FDE_AES_EN_MSG = 4,
	FDE_AES_EN_FDE = 5,
	FDE_AES_EN_RAW = 6,
	FDE_AES_CK_RANGE = 7,
	FDE_AES_EN_SW_CRYPTO = 8,
};

/* ============================================================================== */
/* FDE_AES_DBG Exported Function */
/* ============================================================================== */
int fde_aes_proc_init(void);
u32 fde_aes_check_cmd(u32 cmd, u32 p1, u32 p2);

#endif /* __MT_FDE_AES_DBG_H__ */
