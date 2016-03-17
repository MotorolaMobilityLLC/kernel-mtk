/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MEDIATEK_HDMI_DPI_REG_H
#define __MEDIATEK_HDMI_DPI_REG_H

#define DPI_EN 0x00
#define EN	0
#define EN_MASK (0x1 << 0)

#define DPI_RET 0x04
#define RST 0
#define RST_MASK (0x1 << 0)

#define DPI_INTEN 0x08
#define INT_VSYNC_EN	(0x1 << 0)
#define INT_VDE_EN	(0x1 << 1)
#define INT_UNDERFLOW_EN	(0x1 << 2)

#define DPI_INTSTA 0x0C
#define INT_VSYNC_STA	(0x1 << 0)
#define INT_VDE_STA	(0x1 << 1)
#define INT_UNDERFLOW_STA	(0x1 << 2)

#define DPI_CON	0x10
#define BG_ENABLE				0
#define BG_ENABLE_MASK			(0x1 << 0)
#define IN_RB_SWAP				1
#define IN_RB_SWAP_MASK			(0x1 << 1)
#define INTL_EN					2
#define INTL_EN_MASK			(0x1 << 2)
#define TDFP_EN					3
#define TDFP_EN_MASK			(0x1 << 3)
#define CLPF_EN					4
#define CLPF_EN_MASK			(0x1 << 4)
#define YUV422_EN				5
#define YUV422_EN_MASK			(0x1 << 5)
#define CSC_ENABLE				6
#define CSC_ENABLE_MASK			(0x1 << 6)
#define R601_SEL				7
#define R601_SEL_MASK			(0x1 << 7)
#define EMBSYNC_EN				8
#define EMBSYNC_EN_MASK			(0x1 << 8)
#define VS_LODD_EN				16
#define VS_LODD_EN_MASK			(0x1 << 16)
#define VS_LEVEN_EN				17
#define VS_LEVEN_EN_MASK		(0x1 << 17)
#define VS_RODD_EN				18
#define VS_RODD_EN_MASK			(0x1 << 18)
#define VS_REVEN				19
#define VS_REVEN_MASK			(0x1 << 19)
#define FAKE_DE_LODD			20
#define FAKE_DE_LODD_MASK		(0x1 << 20)
#define FAKE_DE_LEVEN			21
#define FAKE_DE_LEVEN_MASK		(0x1 << 21)
#define FAKE_DE_RODD			22
#define FAKE_DE_RODD_MASK		(0x1 << 22)
#define FAKE_DE_REVEN			23
#define FAKE_DE_REVEN_MASK		(0x1 << 23)

#define DPI_OUTPUT_SETTING 0x14
#define CH_SWAP					0
#define CH_SWAP_MASK			(0x7 << 0)
#define SWAP_RGB 0x00
#define SWAP_GBR 0x01
#define SWAP_BRG 0x02
#define SWAP_RBG 0x03
#define SWAP_GRB 0x04
#define SWAP_BGR 0x05
#define BIT_SWAP				3
#define BIT_SWAP_MASK			(0x1 << 3)
#define B_MASK					4
#define B_MASK_MASK				(0x1 << 4)
#define G_MASK					5
#define G_MASK_MASK				(0x1 << 5)
#define R_MASK					6
#define R_MASK_MASK				(0x1 << 6)
#define DE_MASK					8
#define DE_MASK_MASK			(0x1 << 8)
#define HS_MASK					9
#define HS_MASK_MASK			(0x1 << 9)
#define VS_MASK					10
#define VS_MASK_MASK			(0x1 << 10)
#define DE_POL					12
#define DE_POL_MASK				(0x1 << 12)
#define HSYNC_POL				13
#define HSYNC_POL_MASK			(0x1 << 13)
#define VSYNC_POL				14
#define VSYNC_POL_MASK			(0x1 << 14)
#define CK_POL					15
#define CK_POL_MASK				(0x1 << 15)
#define OEN_OFF					16
#define OEN_OFF_MASK			(0x1 << 16)
#define EDGE_SEL				17
#define EDGE_SEL_MASK			(0x1 << 17)
#define OUT_BIT					18
#define OUT_BIT_MASK			(0x3 << 18)
#define OUT_BIT_8			0x00
#define OUT_BIT_10			0x01
#define OUT_BIT_12			0x02
#define OUT_BIT_16			0x03
#define YC_MAP					20
#define YC_MAP_MASK				(0x7 << 20)
#define YC_MAP_RGB			0x00
#define YC_MAP_CYCY			0x04
#define YC_MAP_YCYC			0x05
#define YC_MAP_CY			0x06
#define YC_MAP_YC			0x07

#define DPI_SIZE 0x18
#define HSIZE		0
#define HSIZE_MASK	(0x1FFF << 0)
#define VSIZE	16
#define VSIZE_MASK	(0x1FFF << 16)

#define DPI_DDR_SETTING 0x1C
#define DDR_EN			(0x1 << 0)
#define DDDR_SEL	(0x1 << 1)
#define DDR_4PHASE	(0x1 << 2)
#define DDR_WIDTH		(0x3 << 4)
#define DDR_PAD_MODE	(0x1 << 8)

#define DPI_TGEN_HWIDTH 0x20
#define HPW 0
#define HPW_MASK	(0xFFF << 0)

#define DPI_TGEN_HPORCH 0x24
#define HBP 0
#define HBP_MASK (0xFFF << 0)
#define HFP 16
#define HFP_MASK (0xFFF << 16)

#define DPI_TGEN_VWIDTH 0x28
#define VPW			0
#define VPW_MASK	(0xFFF << 0)
#define VPW_HALF	16
#define VPW_HALF_MASK	(0x1 << 16)

#define DPI_TGEN_VPORCH 0x2C
#define VBP	0
#define VBP_MASK	(0xFFF << 0)
#define VFP		16
#define VFP_MASK		(0xFFF << 16)

#define DPI_BG_HCNTL 0x30
#define BG_RIGHT	(0x1FFF << 0)
#define BG_LEFT		(0x1FFF << 16)

#define DPI_BG_VCNTL 0x34
#define BG_BOT	(0x1FFF << 0)
#define BG_TOP	(0x1FFF << 16)

#define DPI_BG_COLOR 0x38
#define BG_B	(0xF << 0)
#define BG_G	(0xF << 8)
#define BG_R	(0xF << 16)

#define DPI_FIFO_CTL 0x3C
#define FIFO_VALID_SET	(0x1F << 0)
#define FIFO_RST_SEL	(0x1 << 8)

#define DPI_STATUS 0x40
#define VCOUNTER (0x1FFF << 0)
#define DPI_BUSY (0x1 << 16)
#define OUTEN	 (0x1 << 17)
#define FIELD	 (0x1 << 20)
#define TDLR	(0x1 << 21)

#define DPI_TMODE 0x44
#define DPI_OEN_ON	(0x1 << 0)

#define DPI_CHECKSUM 0x48
#define DPI_CHECKSUM_MASK	(0xFFFFFF << 0)
#define DPI_CHECKSUM_READY (0x1 << 30)
#define DPI_CHECKSUM_EN	(0x1 << 31)

#define DPI_DUMMY 0x50
#define DPI_DUMMY_MASK (0xFFFFFFFF << 0)

#define DPI_TGEN_VWIDTH_LEVEN 0x68
#define VPW_LEVEN	0
#define VPW_LEVEN_MASK (0xFFF << 0)
#define VPW_HALF_LEVEN 16
#define VPW_HALF_LEVEN_MASK (0xFFF << 16)

#define DPI_TGEN_VPORCH_LEVEN 0x6C
#define VBP_LEVEN		0
#define VBP_LEVEN_MASK	(0xFFF << 0)
#define VFP_LEVEN	16
#define VFP_LEVEN_MASK	(0xFFF << 16)

#define DPI_TGEN_VWIDTH_RODD 0x70
#define VPW_RODD		0
#define VPW_RODD_MASK		(0xFFF << 0)
#define VPW_HALF_RODD	16
#define VPW_HALF_RODD_MASK	(0x1 << 16)

#define DPI_TGEN_VPORCH_RODD 0x74
#define VBP_RODD		0
#define VBP_RODD_MASK		(0xFFF << 0)
#define VFP_RODD	16
#define VFP_RODD_MASK	(0xFFF << 16)

#define DPI_TGEN_VWIDTH_REVEN 0x78
#define VPW_REVEN			0
#define VPW_REVEN_MASK		(0xFFF << 0)
#define VPW_HALF_REVEN		16
#define VPW_HALF_REVEN_MASK	(0x1 << 16)

#define DPI_TGEN_VPORCH_REVEN	0x7C
#define VBP_REVEN			0
#define VBP_REVEN_MASK		(0xFFF << 0)
#define VFP_REVEN			16
#define VFP_REVEN_MASK		(0xFFF << 16)

#define DPI_ESAV_VTIMING_LODD 0x80
#define ESAV_VOFST_LODD		(0xFFF << 0)
#define ESAV_VWID_LODD		(0xFFF << 16)

#define DPI_ESAV_VTIMING_LEVEN	0x84
#define ESAV_VOFST_LEVEN	(0xFFF << 0)
#define ESAV_VWID_LEVEN		(0xFFF << 16)

#define DPI_ESAV_VTIMING_RODD	0x88
#define ESAV_VOFST_RODD		(0xFFF << 0)
#define ESAV_VWID_RODD		(0xFFF << 16)

#define DPI_ESAV_VTIMING_REVEN	0x8C
#define ESAV_VOFST_REVEN	(0xFFF << 0)
#define ESAV_VWID_REVEN		(0xFFF << 16)

#define DPI_ESAV_FTIMING	0x90
#define ESAV_FOFST_ODD	(0xFFF << 0)
#define ESAV_FOFST_EVEN		(0xFFF << 16)

#define DPI_CLPF_SETTING	0x94
#define CLPF_TYPE		(0x3 << 0)
#define ROUND_EN		(0x1 << 4)

#define DPI_Y_LIMIT		0x98
#define Y_LIMINT_BOT		0
#define Y_LIMINT_BOT_MASK	(0xFFF << 0)
#define Y_LIMINT_TOP		16
#define Y_LIMINT_TOP_MASK	(0xFFF << 16)

#define DPI_C_LIMIT		0x9C
#define C_LIMIT_BOT		0
#define C_LIMIT_BOT_MASK		(0xFFF << 0)
#define C_LIMIT_TOP				16
#define C_LIMIT_TOP_MASK	(0xFFF << 16)

#define DPI_YUV422_SETTING 0xA0
#define UV_SWAP		(0x1 << 0)
#define CR_DELSEL	(0x1 << 4)
#define CB_DELSEL	(0x1 << 5)
#define Y_DELSEL	(0x1 << 6)
#define DE_DELSEL	(0x1 << 7)

#define DPI_EMBSYNC_SETTING 0xA4
#define EMBSYNC_R_CR_EN	(0x1 << 0)
#define EMPSYNC_G_Y_EN	(0x1 << 1)
#define EMPSYNC_B_CB_EN	(0x1 << 2)
#define ESAV_F_INV		(0x1 << 4)
#define ESAV_V_INV		(0x1 << 5)
#define ESAV_H_INV		(0x1 << 6)
#define ESAV_CODE_MAN	(0x1 << 8)
#define VS_OUT_SEL		(0x7 << 12)

#define DPI_ESAV_CODE_SET0 0xA8
#define ESAV_CODE0			(0xFFF << 0)
#define ESAV_CODE1			(0xFFF << 16)

#define DPI_ESAV_CODE_SET1 0xAC
#define ESAV_CODE2			(0xFFF << 0)
#define ESAV_CODE3_MSB		(0x1 << 16)

#define DPI_H_FRE_CON 0xE0
#define H_FRE_2N (0x1 << 25)
#define H_FRE_MASK (0x1 << 25)
#endif /*  */
