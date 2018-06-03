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
#ifndef __MTK_NAND_OPS_TEST_H__
#define __MTK_NAND_OPS_TEST_H__

#include "mtk_nand_ops.h"

/* UNIT TEST RELATED */
/* #define MTK_NAND_CHIP_TEST */
/* #define MTK_NAND_CHIP_DUMP_DATA_TEST */
#define MTK_NAND_CHIP_MULTI_PLANE_TEST
/* #define MTK_NAND_READ_COMPARE */
#define MTK_NAND_PERFORMANCE_TEST
#define MTK_NAND_MARK_BAD_TEST 1
#define MTK_NAND_SIM_ERR 1

extern struct mtk_nand_data_info *data_info;

extern bool mtk_isbad_block(unsigned int block);
extern int mtk_nand_exec_read_page(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				u8 *pFDMBuf);
extern int mtk_nand_exec_write_page_hw(struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8 *pPageBuf,
				 u8 *pFDMBuf);
extern int mtk_nand_write_tlc_block_hw_split(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *main_buf, uint8_t *extra_buf, u32 mapped_block, u32 page_in_block, u32 size);
extern flashdev_info gn_devinfo;

void mtk_chip_unit_test(void);

#if MTK_NAND_SIM_ERR
int sim_nand_err(enum operation_types op, int block, int page);
#endif

#endif
