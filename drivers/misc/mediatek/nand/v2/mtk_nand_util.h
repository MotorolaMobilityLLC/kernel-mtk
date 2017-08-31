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

#ifndef __MTK_NAND_UTIL_H
#define __MTK_NAND_UTIL_H

#include "nand_device_define.h"

extern flashdev_info_t devinfo;

extern bool init_pmt_done;

extern bool mtk_block_istlc(u64 addr);
extern void mtk_slc_blk_addr(u64 addr, u32 *blk_num, u32 *page_in_block);
bool mtk_is_normal_tlc_nand(void);
int mtk_nand_tlc_block_mark(struct mtd_info *mtd, struct nand_chip *chip, u32 mapped_block);
extern int mtk_nand_write_tlc_block_hw(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, u32 mapped_block);
#if defined(CONFIG_MTK_TLC_NAND_SUPPORT)
extern void mtk_pmt_reset(void);
extern bool mtk_nand_IsBMTPOOL(loff_t logical_address);
extern u64 OFFSET(u32 block);
#endif

#endif
