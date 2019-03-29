/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __EDMC_HW_REG_H__
#define __EDMC_HW_REG_H__

//eDMC
//#define APU_EDM_CTL_0 0x19050000


enum OP_MODE {
	MODE_FILL       = 0x3,
	MODE_NORMAL     = 0x4,
	MODE_RGATORGBA  = 0xA,
	MODE_FP32TOFIX8 = 0xB,
	MODE_FIX8TOFP32 = 0xD
};

//HW limitation
#define SRC_TILE_WIDTH_MAX 4096
#define DST_TILE_WIDTH_MAX 4096
#define TILE_HEIGHT_MAX 2048

#define DMA_SW_RST            (1 << 4)
#define RST_PROT_EN           (1 << 6)
#define MT8163E2_ECO_DISABLE  (1 << 7)
#define RST_PROT_IDLE         (1 << 8)
#define AXI_PROT_EN           (1 << 9)
#define CLK_ENABLE            (1 << 10)

#define APU_EDM_CTL_0              (0x0000)
#define APU_EDM_CFG_0              (0x0004)
#define APU_EDM_INT_MASK           (0x0008)
#define APU_EDM_DESP_STATUS        (0x000C)
#define APU_EDM_INT_STATUS         (0x0010)
#define APU_EDM_ERR_INT_MASK       (0x0014)
#define APU_EDM_ERR_STATUS         (0x0018)
#define APU_EDM_ERR_INT_STATUS     (0x001C)

#define APU_DESP0_SRC_TILE_WIDTH   (0x0C00)
#define APU_DESP0_DEST_TILE_WIDTH  (0x0C04)
#define APU_DESP0_TILE_HEIGHT      (0x0C08)
#define APU_DESP0_SRC_STRIDE       (0x0C0C)
#define APU_DESP0_DEST_STRIDE      (0x0C10)
#define APU_DESP0_SRC_ADDR_0       (0x0C14)
#define APU_DESP0_DEST_ADDR_0      (0x0C1C)
#define APU_DESP0_CTL_0            (0x0C24)
#define APU_DESP0_CTL_1            (0x0C28)
#define APU_DESP0_FILL_VALUE       (0x0C2C)
#define APU_DESP0_ID               (0x0C30)
#define APU_DESP0_RANGE_SCALE      (0x0C34)
#define APU_DESP0_MIN_FP32         (0x0C38)

#define APU_DESP1_SRC_TILE_WIDTH   (0x0C40)
#define APU_DESP1_DEST_TILE_WIDTH  (0x0C44)
#define APU_DESP1_TILE_HEIGHT      (0x0C48)
#define APU_DESP1_SRC_STRIDE       (0x0C4C)
#define APU_DESP1_DEST_STRIDE      (0x0C50)
#define APU_DESP1_SRC_ADDR_0       (0x0C54)
#define APU_DESP1_DEST_ADDR_0      (0x0C5C)
#define APU_DESP1_CTL_0            (0x0C64)
#define APU_DESP1_CTL_1            (0x0C68)
#define APU_DESP1_FILL_VALUE       (0x0C6C)
#define APU_DESP1_ID               (0x0C70)
#define APU_DESP1_RANGE_SCALE      (0x0C74)
#define APU_DESP1_MIN_FP32         (0x0C78)

#define APU_DESP2_SRC_TILE_WIDTH   (0x0C80)
#define APU_DESP2_DEST_TILE_WIDTH  (0x0C84)
#define APU_DESP2_TILE_HEIGHT      (0x0C88)
#define APU_DESP2_SRC_STRIDE       (0x0C8C)
#define APU_DESP2_DEST_STRIDE      (0x0C90)
#define APU_DESP2_SRC_ADDR_0       (0x0C94)
#define APU_DESP2_DEST_ADDR_0      (0x0C9C)
#define APU_DESP2_CTL_0            (0x0CA4)
#define APU_DESP2_CTL_1            (0x0CA8)
#define APU_DESP2_FILL_VALUE       (0x0CAC)
#define APU_DESP2_ID               (0x0CB0)
#define APU_DESP2_RANGE_SCALE      (0x0CB4)
#define APU_DESP2_MIN_FP32         (0x0CB8)

#define APU_DESP3_SRC_TILE_WIDTH   (0x0CC0)
#define APU_DESP3_DEST_TILE_WIDTH  (0x0CC4)
#define APU_DESP3_TILE_HEIGHT      (0x0CC8)
#define APU_DESP3_SRC_STRIDE       (0x0CCC)
#define APU_DESP3_DEST_STRIDE      (0x0CD0)
#define APU_DESP3_SRC_ADDR_0       (0x0CD4)
#define APU_DESP3_DEST_ADDR_0      (0x0CDC)
#define APU_DESP3_CTL_0            (0x0CE4)
#define APU_DESP3_CTL_1            (0x0CE8)
#define APU_DESP3_FILL_VALUE       (0x0CEC)
#define APU_DESP3_ID               (0x0CF0)
#define APU_DESP3_RANGE_SCALE      (0x0CF4)
#define APU_DESP3_MIN_FP32         (0x0CF8)


#endif
