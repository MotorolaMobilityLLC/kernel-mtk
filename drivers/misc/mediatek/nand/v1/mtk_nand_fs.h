/*
 * Copyright (C) 2017 MediaTek Inc.
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
#ifndef __MTK_NAND_PROC_H__
#define __MTK_NAND_PROC_H__


#define PROCNAME	"driver/nand"
extern int g_i4Interrupt;
extern bool DDR_INTERFACE;
extern int g_i4InterruptRdDMA;
extern int g_i4Homescreen;
extern int init_module_mem(void *buf, int size);
int mtk_nand_fs_init(void);

void mtk_nand_fs_exit(void);

#endif
