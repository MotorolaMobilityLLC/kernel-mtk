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
#include "mediatek_hdmi_hw.h"
#include "mediatek_hdmi_regs.h"
#include "mediatek_hdmi.h"

#include <linux/delay.h>
#include <linux/hdmi.h>
#include <linux/io.h>

#define MTK_HDMI_READ_BANK(bank) \
static u32 mtk_hdmi_read_##bank(struct mediatek_hdmi *hdmi, \
				u32 offset) \
{ \
	return readl(hdmi->bank##_regs + offset); \
}

#define MTK_HDMI_WRITE_BANK(bank) \
static void mtk_hdmi_write_##bank(struct mediatek_hdmi *hdmi, \
				  u32 offset, u32 val) \
{ \
	writel(val, hdmi->bank##_regs + offset); \
}

#define MTK_HDMI_MASK_BANK(bank) \
static void mtk_hdmi_mask_##bank(struct mediatek_hdmi *hdmi, \
				 u32 offset,  u32 val, u32 mask) \
{ \
	u32 tmp = mtk_hdmi_read_##bank(hdmi, offset) & ~mask; \
	tmp |= (val & mask); \
	mtk_hdmi_write_##bank(hdmi, offset, tmp); \
}

#define MTK_HDMI_ACCESS_BANK(bank) \
		MTK_HDMI_READ_BANK(bank) \
		MTK_HDMI_WRITE_BANK(bank) \
		MTK_HDMI_MASK_BANK(bank)

MTK_HDMI_ACCESS_BANK(grl)
MTK_HDMI_ACCESS_BANK(pll)
MTK_HDMI_ACCESS_BANK(sys)
MTK_HDMI_ACCESS_BANK(cec)

static u32 internal_read_reg(phys_addr_t addr)
{
	void __iomem *ptr = NULL;
	u32 val;

	ptr = ioremap(addr, 0x4);
	val = readl(ptr);
	iounmap(ptr);
	return val;
}

static void internal_write_reg(phys_addr_t addr, size_t val)
{
	void __iomem *ptr = NULL;

	ptr = ioremap(addr, 0x4);
	writel(val, ptr);
	iounmap(ptr);
}

#define internal_read(addr) internal_read_reg(addr)
#define internal_write(addr, val) internal_write_reg(addr, val)
#define internal_write_msk(addr, val, msk) \
	internal_write((addr), \
	(internal_read(addr) & (~(msk))) | ((val) & (msk)))

static const u8 PREDIV[3][4] = {
	{0x0, 0x0, 0x0, 0x0},	/* 27Mhz */
	{0x1, 0x1, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x1, 0x1, 0x1}	/* 148Mhz */
};

static const u8 TXDIV[3][4] = {
	{0x3, 0x3, 0x3, 0x2},	/* 27Mhz */
	{0x2, 0x1, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x0, 0x0, 0x0}	/* 148Mhz */
};

static const u8 FBKSEL[3][4] = {
	{0x1, 0x1, 0x1, 0x1},	/* 27Mhz */
	{0x1, 0x0, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x0, 0x1, 0x1}	/* 148Mhz */
};

static const u8 FBKDIV[3][4] = {
	{19, 24, 29, 19},	/* 27Mhz */
	{19, 24, 14, 19},	/* 74Mhz */
	{19, 24, 14, 19}	/* 148Mhz */
};

static const u8 DIVEN[3][4] = {
	{0x2, 0x1, 0x1, 0x2},	/* 27Mhz */
	{0x2, 0x2, 0x2, 0x2},	/* 74Mhz */
	{0x2, 0x2, 0x2, 0x2}	/* 148Mhz */
};

static const u8 HTPLLBP[3][4] = {
	{0xc, 0xc, 0x8, 0xc},	/* 27Mhz */
	{0xc, 0xf, 0xf, 0xc},	/* 74Mhz */
	{0xc, 0xf, 0xf, 0xc}	/* 148Mhz */
};

static const u8 HTPLLBC[3][4] = {
	{0x2, 0x3, 0x3, 0x2},	/* 27Mhz */
	{0x2, 0x3, 0x3, 0x2},	/* 74Mhz */
	{0x2, 0x3, 0x3, 0x2}	/* 148Mhz */
};

static const u8 HTPLLBR[3][4] = {
	{0x1, 0x1, 0x0, 0x1},	/* 27Mhz */
	{0x1, 0x2, 0x2, 0x1},	/* 74Mhz */
	{0x1, 0x2, 0x2, 0x1}	/* 148Mhz */
};

#define NCTS_BYTES          0x07
static const u8 HDMI_NCTS[7][9][NCTS_BYTES] = {
	{{0x00, 0x00, 0x69, 0x78, 0x00, 0x10, 0x00},
	 {0x00, 0x00, 0xd2, 0xf0, 0x00, 0x10, 0x00},
	 {0x00, 0x03, 0x37, 0xf9, 0x00, 0x2d, 0x80},
	 {0x00, 0x01, 0x22, 0x0a, 0x00, 0x10, 0x00},
	 {0x00, 0x06, 0x6f, 0xf3, 0x00, 0x2d, 0x80},
	 {0x00, 0x02, 0x44, 0x14, 0x00, 0x10, 0x00},
	 {0x00, 0x01, 0xA5, 0xe0, 0x00, 0x10, 0x00},
	 {0x00, 0x06, 0x6F, 0xF3, 0x00, 0x16, 0xC0},
	 {0x00, 0x03, 0x66, 0x1E, 0x00, 0x0C, 0x00}
	 },
	{{0x00, 0x00, 0x75, 0x30, 0x00, 0x18, 0x80},
	 {0x00, 0x00, 0xea, 0x60, 0x00, 0x18, 0x80},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x45, 0xac},
	 {0x00, 0x01, 0x42, 0x44, 0x00, 0x18, 0x80},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x22, 0xd6},
	 {0x00, 0x02, 0x84, 0x88, 0x00, 0x18, 0x80},
	 {0x00, 0x01, 0xd4, 0xc0, 0x00, 0x18, 0x80},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x11, 0x6B},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x12, 0x60}
	 },
	{{0x00, 0x00, 0x69, 0x78, 0x00, 0x18, 0x00},
	 {0x00, 0x00, 0xd2, 0xf0, 0x00, 0x18, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0x2d, 0x80},
	 {0x00, 0x01, 0x22, 0x0a, 0x00, 0x18, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0x16, 0xc0},
	 {0x00, 0x02, 0x44, 0x14, 0x00, 0x18, 0x00},
	 {0x00, 0x01, 0xA5, 0xe0, 0x00, 0x18, 0x00},
	 {0x00, 0x04, 0x4A, 0xA2, 0x00, 0x16, 0xC0},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x14, 0x00}
	 },
	{{0x00, 0x00, 0x75, 0x30, 0x00, 0x31, 0x00},
	 {0x00, 0x00, 0xea, 0x60, 0x00, 0x31, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x8b, 0x58},
	 {0x00, 0x01, 0x42, 0x44, 0x00, 0x31, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x45, 0xac},
	 {0x00, 0x02, 0x84, 0x88, 0x00, 0x31, 0x00},
	 {0x00, 0x01, 0xd4, 0xc0, 0x00, 0x31, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x22, 0xD6},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x24, 0xC0}
	 },
	{{0x00, 0x00, 0x69, 0x78, 0x00, 0x30, 0x00},
	 {0x00, 0x00, 0xd2, 0xf0, 0x00, 0x30, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0x5b, 0x00},
	 {0x00, 0x01, 0x22, 0x0a, 0x00, 0x30, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0x2d, 0x80},
	 {0x00, 0x02, 0x44, 0x14, 0x00, 0x30, 0x00},
	 {0x00, 0x01, 0xA5, 0xe0, 0x00, 0x30, 0x00},
	 {0x00, 0x04, 0x4A, 0xA2, 0x00, 0x2D, 0x80},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x28, 0x80}
	 },
	{{0x00, 0x00, 0x75, 0x30, 0x00, 0x62, 0x00},
	 {0x00, 0x00, 0xea, 0x60, 0x00, 0x62, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x01, 0x16, 0xb0},
	 {0x00, 0x01, 0x42, 0x44, 0x00, 0x62, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x8b, 0x58},
	 {0x00, 0x02, 0x84, 0x88, 0x00, 0x62, 0x00},
	 {0x00, 0x01, 0xd4, 0xc0, 0x00, 0x62, 0x00},
	 {0x00, 0x03, 0x93, 0x87, 0x00, 0x45, 0xAC},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x49, 0x80}
	 },
	{{0x00, 0x00, 0x69, 0x78, 0x00, 0x60, 0x00},
	 {0x00, 0x00, 0xd2, 0xf0, 0x00, 0x60, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0xb6, 0x00},
	 {0x00, 0x01, 0x22, 0x0a, 0x00, 0x60, 0x00},
	 {0x00, 0x02, 0x25, 0x51, 0x00, 0x5b, 0x00},
	 {0x00, 0x02, 0x44, 0x14, 0x00, 0x60, 0x00},
	 {0x00, 0x01, 0xA5, 0xe0, 0x00, 0x60, 0x00},
	 {0x00, 0x04, 0x4A, 0xA2, 0x00, 0x5B, 0x00},
	 {0x00, 0x03, 0xC6, 0xCC, 0x00, 0x50, 0x00}
	 }
};

void mtk_hdmi_hw_vid_black(struct mediatek_hdmi *hdmi,
			   bool black)
{
	if (black)
		mtk_hdmi_mask_grl(hdmi, VIDEO_CFG_4,
				  GEN_RGB, VIDEO_SOURCE_SEL);
	else
		mtk_hdmi_mask_grl(hdmi,
				  VIDEO_CFG_4, NORMAL_PATH, VIDEO_SOURCE_SEL);
}

void mtk_hdmi_hw_make_reg_writabe(struct mediatek_hdmi *hdmi,
				  bool enable)
{
	if (enable) {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, HDMI_PCLK_FREE_RUN,
				  HDMI_PCLK_FREE_RUN);
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG1C, HDMI_ON | ANLG_ON,
				  HDMI_ON | ANLG_ON);
	} else {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, 0, HDMI_PCLK_FREE_RUN);
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG1C, 0, HDMI_ON | ANLG_ON);
	}
}

void mtk_hdmi_hw_1p4_verseion_enable(struct mediatek_hdmi *hdmi,
				     bool enable)
{
	if (enable)
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, 0, HDMI2P0_EN);
	else
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, HDMI2P0_EN, HDMI2P0_EN);
}

bool mtk_hdmi_hw_is_hpd_high(struct mediatek_hdmi *hdmi)
{
	unsigned int status;

	status = mtk_hdmi_read_cec(hdmi, RX_EVENT);
	return (HDMI_PORD & status) == HDMI_PORD &&
			(HDMI_HTPLG & status) == HDMI_HTPLG;
}

void mtk_hdmi_hw_aud_mute(struct mediatek_hdmi *hdmi, bool mute)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
	if (mute)
		val |= AUDIO_ZERO;
	else
		val &= ~AUDIO_ZERO;
	mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
}

void mtk_hdmi_hw_reset(struct mediatek_hdmi *hdmi, bool reset)
{
	if (reset) {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG1C, HDMI_RST, HDMI_RST);
	} else {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG1C, 0, HDMI_RST);
		mtk_hdmi_mask_grl(hdmi, GRL_CFG3, 0, CFG3_CONTROL_PACKET_DELAY);
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG1C, ANLG_ON, ANLG_ON);
	}
}

void mtk_hdmi_hw_enable_notice(struct mediatek_hdmi *hdmi,
			       bool enable_notice)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG2);
	if (enable_notice)
		val |= CFG2_NOTICE_EN;
	else
		val &= ~CFG2_NOTICE_EN;
	mtk_hdmi_write_grl(hdmi, GRL_CFG2, val);
}

void mtk_hdmi_hw_write_int_mask(struct mediatek_hdmi *hdmi,
				u32 int_mask)
{
	mtk_hdmi_write_grl(hdmi, GRL_INT_MASK, int_mask);
}

void mtk_hdmi_hw_enable_dvi_mode(struct mediatek_hdmi *hdmi,
				 bool enable)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG1);
	if (enable)
		val |= CFG1_DVI;
	else
		val &= ~CFG1_DVI;
	mtk_hdmi_write_grl(hdmi, GRL_CFG1, val);
}

u32 mtk_hdmi_hw_get_int_type(struct mediatek_hdmi *hdmi)
{
	return mtk_hdmi_read_grl(hdmi, GRL_INT);
}

void mtk_hdmi_hw_on_off_tmds(struct mediatek_hdmi *hdmi,
			     bool on)
{
	if (on) {
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, RG_HDMITX_PLL_AUTOK_EN,
				  RG_HDMITX_PLL_AUTOK_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON0, RG_HDMITX_PLL_POSDIV,
				  RG_HDMITX_PLL_POSDIV);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, 0, RG_HDMITX_MHLCK_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, RG_HDMITX_PLL_BIAS_EN,
				  RG_HDMITX_PLL_BIAS_EN);
		usleep_range(100, 150);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON0, RG_HDMITX_PLL_EN,
				  RG_HDMITX_PLL_EN);
		usleep_range(100, 150);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, RG_HDMITX_PLL_BIAS_LPF_EN,
				  RG_HDMITX_PLL_BIAS_LPF_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, RG_HDMITX_PLL_TXDIV_EN,
				  RG_HDMITX_PLL_TXDIV_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, RG_HDMITX_SER_EN,
				  RG_HDMITX_SER_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, RG_HDMITX_PRD_EN,
				  RG_HDMITX_PRD_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, RG_HDMITX_DRV_EN,
				  RG_HDMITX_DRV_EN);
		usleep_range(100, 150);
	} else {
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, 0, RG_HDMITX_DRV_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, 0, RG_HDMITX_PRD_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, 0, RG_HDMITX_SER_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1,
				  0, RG_HDMITX_PLL_TXDIV_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1,
				  0, RG_HDMITX_PLL_BIAS_LPF_EN);
		usleep_range(100, 150);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON0, 0, RG_HDMITX_PLL_EN);
		usleep_range(100, 150);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, 0, RG_HDMITX_PLL_BIAS_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON0, 0, RG_HDMITX_PLL_POSDIV);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON1, 0, RG_HDMITX_PLL_AUTOK_EN);
		usleep_range(100, 150);
	}
}

void mtk_hdmi_hw_send_info_frame(struct mediatek_hdmi *hdmi,
				 u8 *buffer, u8 len)
{
	u32 val;
	u32 ctrl_reg = GRL_CTRL;
	int i;
	u8 *frame_data;
	u8 frame_type;
	u8 frame_ver;
	u8 frame_len;
	u8 checksum;
	int ctrl_frame_en = 0;

	frame_type = *buffer;
	buffer += 1;
	frame_ver = *buffer;
	buffer += 1;
	frame_len = *buffer;
	buffer += 1;
	checksum = *buffer;
	buffer += 1;
	frame_data = buffer;

	mtk_hdmi_info
	    ("frame_type:0x%x,frame_ver:0x%x,frame_len:0x%x,checksum:0x%x\n",
	     frame_type, frame_ver, frame_len, checksum);

	if (HDMI_INFOFRAME_TYPE_AVI == frame_type) {
		ctrl_frame_en = CTRL_AVI_EN;
		ctrl_reg = GRL_CTRL;
	} else if (HDMI_INFOFRAME_TYPE_SPD == frame_type) {
		ctrl_frame_en = CTRL_SPD_EN;
		ctrl_reg = GRL_CTRL;
	} else if (HDMI_INFOFRAME_TYPE_AUDIO == frame_type) {
		ctrl_frame_en = CTRL_AUDIO_EN;
		ctrl_reg = GRL_CTRL;
	} else if (HDMI_INFOFRAME_TYPE_VENDOR == frame_type) {
		ctrl_frame_en = VS_EN;
		ctrl_reg = GRL_ACP_ISRC_CTRL;
	}

	mtk_hdmi_mask_grl(hdmi, ctrl_reg, 0, ctrl_frame_en);
	mtk_hdmi_write_grl(hdmi, GRL_INFOFRM_TYPE, frame_type);
	mtk_hdmi_write_grl(hdmi, GRL_INFOFRM_VER, frame_ver);
	mtk_hdmi_write_grl(hdmi, GRL_INFOFRM_LNG, frame_len);

	mtk_hdmi_write_grl(hdmi, GRL_IFM_PORT, checksum);
	for (i = 0; i < frame_len; i++)
		mtk_hdmi_write_grl(hdmi, GRL_IFM_PORT, frame_data[i]);

	val = mtk_hdmi_read_grl(hdmi, ctrl_reg);
	val |= ctrl_frame_en;
	mtk_hdmi_write_grl(hdmi, ctrl_reg, val);
}

void mtk_hdmi_hw_send_aud_packet(struct mediatek_hdmi *hdmi,
				 bool enable)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_SHIFT_R2);
	if (enable)
		val &= ~AUDIO_PACKET_OFF;
	else
		val |= AUDIO_PACKET_OFF;
	mtk_hdmi_write_grl(hdmi, GRL_SHIFT_R2, val);
}

void mtk_hdmi_hw_set_pll(struct mediatek_hdmi *hdmi,
			 u32 clock,
			 enum HDMI_DISPLAY_COLOR_DEPTH depth)
{
	unsigned int v4valueclk;
	unsigned int v4valued2;
	unsigned int v4valued1;
	unsigned int v4valued0;
	unsigned int pix;

	if (clock <= 27000000)
		pix = 0;
	else if (clock <= 74000000)
		pix = 1;
	else
		pix = 2;

	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((PREDIV[pix][depth]) << PREDIV_SHIFT),
			  RG_HDMITX_PLL_PREDIV);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0, RG_HDMITX_PLL_POSDIV,
			  RG_HDMITX_PLL_POSDIV);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  (0x1 << PLL_IC_SHIFT), RG_HDMITX_PLL_IC);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  (0x1 << PLL_IR_SHIFT), RG_HDMITX_PLL_IR);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON1,
			  ((TXDIV[pix][depth]) << PLL_TXDIV_SHIFT),
			  RG_HDMITX_PLL_TXDIV);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((FBKSEL[pix][depth]) << PLL_FBKSEL_SHIFT),
			  RG_HDMITX_PLL_FBKSEL);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((FBKDIV[pix][depth]) << PLL_FBKDIV_SHIFT),
			  RG_HDMITX_PLL_FBKDIV);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON1,
			  ((DIVEN[pix][depth]) << PLL_DIVEN_SHIFT),
			  RG_HDMITX_PLL_DIVEN);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((HTPLLBP[pix][depth]) << PLL_BP_SHIFT),
			  RG_HDMITX_PLL_BP);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((HTPLLBC[pix][depth]) << PLL_BC_SHIFT),
			  RG_HDMITX_PLL_BC);
	mtk_hdmi_mask_pll(hdmi, HDMI_CON0,
			  ((HTPLLBR[pix][depth]) << PLL_BR_SHIFT),
			  RG_HDMITX_PLL_BR);

	v4valueclk = internal_read(0x10206000 + 0x4c8);
	v4valueclk = v4valueclk >> 24;
	v4valued2 = internal_read(0x10206000 + 0x4c8);
	v4valued2 = (v4valued2 & 0x00ff0000) >> 16;
	v4valued1 = internal_read(0x10206000 + 0x530);
	v4valued1 = (v4valued1 & 0x00000fc0) >> 6;
	v4valued0 = internal_read(0x10206000 + 0x530);
	v4valued0 = v4valued0 & 0x3f;

	if ((v4valueclk == 0) || (v4valued2 == 0) ||
	    (v4valued1 == 0) || (v4valued0 == 0)) {
		v4valueclk = 0x30;
		v4valued2 = 0x30;
		v4valued1 = 0x30;
		v4valued0 = 0x30;
	}

	if ((pix == 2) &&
	    (depth != HDMI_DEEP_COLOR_24BITS)) {
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, RG_HDMITX_PRD_IMP_EN,
				  RG_HDMITX_PRD_IMP_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x6 << PRD_IBIAS_CLK_SHIFT),
				  RG_HDMITX_PRD_IBIAS_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x6 << PRD_IBIAS_D2_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x6 << PRD_IBIAS_D1_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x6 << PRD_IBIAS_D0_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D0);

		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, (0xf << DRV_IMP_EN_SHIFT),
				  RG_HDMITX_DRV_IMP_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valueclk << DRV_IMP_CLK_SHIFT),
				  RG_HDMITX_DRV_IMP_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued2 << DRV_IMP_D2_SHIFT),
				  RG_HDMITX_DRV_IMP_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued1 << DRV_IMP_D1_SHIFT),
				  RG_HDMITX_DRV_IMP_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued0 << DRV_IMP_D0_SHIFT),
				  RG_HDMITX_DRV_IMP_D0);

		mtk_hdmi_mask_pll(hdmi, HDMI_CON5,
				  (0x1c << DRV_IBIAS_CLK_SHIFT),
				  RG_HDMITX_DRV_IBIAS_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0x1c << DRV_IBIAS_D2_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0x1c << DRV_IBIAS_D1_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0x1c << DRV_IBIAS_D0_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D0);
	} else {
		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, 0, RG_HDMITX_PRD_IMP_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x3 << PRD_IBIAS_CLK_SHIFT),
				  RG_HDMITX_PRD_IBIAS_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x3 << PRD_IBIAS_D2_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x3 << PRD_IBIAS_D1_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON4, (0x3 << PRD_IBIAS_D0_SHIFT),
				  RG_HDMITX_PRD_IBIAS_D0);

		mtk_hdmi_mask_pll(hdmi, HDMI_CON3, (0x0 << DRV_IMP_EN_SHIFT),
				  RG_HDMITX_DRV_IMP_EN);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valueclk << DRV_IMP_CLK_SHIFT),
				  RG_HDMITX_DRV_IMP_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued2 << DRV_IMP_D2_SHIFT),
				  RG_HDMITX_DRV_IMP_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued1 << DRV_IMP_D1_SHIFT),
				  RG_HDMITX_DRV_IMP_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON6,
				  (v4valued0 << DRV_IMP_D0_SHIFT),
				  RG_HDMITX_DRV_IMP_D0);

		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0xa << DRV_IBIAS_CLK_SHIFT),
				  RG_HDMITX_DRV_IBIAS_CLK);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0xa << DRV_IBIAS_D2_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D2);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0xa << DRV_IBIAS_D1_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D1);
		mtk_hdmi_mask_pll(hdmi, HDMI_CON5, (0xa << DRV_IBIAS_D0_SHIFT),
				  RG_HDMITX_DRV_IBIAS_D0);
	}
}

int mtk_hdmi_hw_set_clock(struct mediatek_hdmi *hctx, u32 clock)
{
	int ret;

	ret = mtk_hdmi_clk_sel_div(hctx,
				   MEDIATEK_HDMI_CLK_HDMI_SEL,
				   MEDIATEK_HDMI_CLK_HDMI_DIV1);
	if (ret) {
		mtk_hdmi_err("clk sel div failed!\n");
		return ret;
	}

	if (clock <= 27000000) {
		mtk_hdmi_clk_set_rate(hctx,
				      MEDIATEK_HDMI_CLK_TVDPLL,
				      clock * 8 * 3);
		mtk_hdmi_clk_sel_div(hctx,
				     MEDIATEK_HDMI_CLK_DPI_SEL,
				     MEDIATEK_HDMI_CLK_DPI_DIV8);
	} else if (clock <= 74000000) {
		mtk_hdmi_clk_set_rate(hctx,
				      MEDIATEK_HDMI_CLK_TVDPLL,
				      clock * 4 * 3);
		mtk_hdmi_clk_sel_div(hctx,
				     MEDIATEK_HDMI_CLK_DPI_SEL,
				     MEDIATEK_HDMI_CLK_DPI_DIV4);
	} else {
		mtk_hdmi_clk_set_rate(hctx,
				      MEDIATEK_HDMI_CLK_TVDPLL,
				      clock * 4 * 3);
		mtk_hdmi_clk_sel_div(hctx,
				     MEDIATEK_HDMI_CLK_DPI_SEL,
				     MEDIATEK_HDMI_CLK_DPI_DIV4);
	}

	if (hctx->aud_input_type == HDMI_AUD_INPUT_SPDIF)
		mtk_hdmi_clk_disable(hctx, MEDIATEK_HDMI_CLK_AUD_BCLK);
	else
		mtk_hdmi_clk_enable(hctx, MEDIATEK_HDMI_CLK_AUD_BCLK);

	return 0;
}

void mtk_hdmi_hw_config_sys(struct mediatek_hdmi *hdmi)
{
	mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20,
			  0, HDMI_OUT_FIFO_EN | MHL_MODE_ON);
	mdelay(2);
	mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, HDMI_OUT_FIFO_EN,
			  HDMI_OUT_FIFO_EN | MHL_MODE_ON);

	internal_write_msk(0x10209040, 0x3 << 16, 0x7 << 16);
}

void mtk_hdmi_hw_set_deep_color_mode(struct mediatek_hdmi *hdmi,
				     enum HDMI_DISPLAY_COLOR_DEPTH depth)
{
	u32 val = 0;

	switch (depth) {
	case HDMI_DEEP_COLOR_24BITS:
		val = COLOR_8BIT_MODE;
		break;
	case HDMI_DEEP_COLOR_30BITS:
		val = COLOR_10BIT_MODE;
		break;
	case HDMI_DEEP_COLOR_36BITS:
		val = COLOR_12BIT_MODE;
		break;
	case HDMI_DEEP_COLOR_48BITS:
		val = COLOR_16BIT_MODE;
		break;
	default:
		val = COLOR_8BIT_MODE;
		break;
	}

	if (val == COLOR_8BIT_MODE) {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, val,
				  DEEP_COLOR_MODE_MASK | DEEP_COLOR_EN);
	} else {
		mtk_hdmi_mask_sys(hdmi, HDMI_SYS_CFG20, val | DEEP_COLOR_EN,
				  DEEP_COLOR_MODE_MASK | DEEP_COLOR_EN);
	}
}

void mtk_hdmi_hw_send_av_mute(struct mediatek_hdmi *hdmi)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG4);
	val &= ~CTRL_AVMUTE;
	mtk_hdmi_write_grl(hdmi, GRL_CFG4, val);

	mdelay(2);

	val |= CTRL_AVMUTE;
	mtk_hdmi_write_grl(hdmi, GRL_CFG4, val);
}

void mtk_hdmi_hw_send_av_unmute(struct mediatek_hdmi *hdmi)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG4);
	val |= CFG4_AV_UNMUTE_EN;
	val &= ~CFG4_AV_UNMUTE_SET;
	mtk_hdmi_write_grl(hdmi, GRL_CFG4, val);

	mdelay(2);

	val &= ~CFG4_AV_UNMUTE_EN;
	val |= CFG4_AV_UNMUTE_SET;
	mtk_hdmi_write_grl(hdmi, GRL_CFG4, val);
}

void mtk_hdmi_hw_ncts_enable(struct mediatek_hdmi *hdmi,
			     bool on)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_CTS_CTRL);
	if (on)
		val &= ~CTS_CTRL_SOFT;
	else
		val |= CTS_CTRL_SOFT;
	mtk_hdmi_write_grl(hdmi, GRL_CTS_CTRL, val);
}

void mtk_hdmi_hw_ncts_auto_write_enable(struct mediatek_hdmi
					*hdmi, bool enable)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_DIVN);
	if (enable)
		val |= NCTS_WRI_ANYTIME;
	else
		val &= ~NCTS_WRI_ANYTIME;
	mtk_hdmi_write_grl(hdmi, GRL_DIVN, val);
}

void mtk_hdmi_hw_msic_setting(struct mediatek_hdmi *hdmi,
			      struct drm_display_mode *mode)
{
	mtk_hdmi_mask_grl(hdmi, GRL_CFG4, 0, CFG_MHL_MODE);

	if (mode->flags & DRM_MODE_FLAG_INTERLACE &&
	    mode->clock == 74250 &&
	    mode->vdisplay == 1080)
		mtk_hdmi_mask_grl(hdmi, GRL_CFG2, 0, MHL_DE_SEL);
	else
		mtk_hdmi_mask_grl(hdmi, GRL_CFG2, MHL_DE_SEL, MHL_DE_SEL);
}

void mtk_hdmi_hw_aud_set_channel_swap(struct mediatek_hdmi
				      *hdmi,
				      enum hdmi_aud_channel_swap_type swap)
{
	u8 swap_bit;

	switch (swap) {
	case HDMI_AUD_SWAP_LR:
		swap_bit = LR_SWAP;
		break;
	case HDMI_AUD_SWAP_LFE_CC:
		swap_bit = LFE_CC_SWAP;
		break;
	case HDMI_AUD_SWAP_LSRS:
		swap_bit = LSRS_SWAP;
		break;
	case HDMI_AUD_SWAP_RLS_RRS:
		swap_bit = RLS_RRS_SWAP;
		break;
	case HDMI_AUD_SWAP_LR_STATUS:
		swap_bit = LR_STATUS_SWAP;
		break;
	default:
		swap_bit = LFE_CC_SWAP;
		break;
	}
	mtk_hdmi_mask_grl(hdmi, GRL_CH_SWAP, swap_bit, 0xff);
}

void mtk_hdmi_hw_aud_raw_data_enable(struct mediatek_hdmi *hdmi,
				     bool enable)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_MIX_CTRL);
	if (enable)
		val |= MIX_CTRL_FLAT;
	else
		val &= ~MIX_CTRL_FLAT;

	mtk_hdmi_write_grl(hdmi, GRL_MIX_CTRL, val);
}

void mtk_hdmi_hw_aud_set_bit_num(struct mediatek_hdmi *hdmi,
				 enum hdmi_audio_sample_size bit_num)
{
	u32 val = 0;

	if (bit_num == HDMI_AUDIO_SAMPLE_SIZE_16)
		val = AOUT_16BIT;
	else if (bit_num == HDMI_AUDIO_SAMPLE_SIZE_20)
		val = AOUT_20BIT;
	else if (bit_num == HDMI_AUDIO_SAMPLE_SIZE_24)
		val = AOUT_24BIT;

	mtk_hdmi_mask_grl(hdmi, GRL_AOUT_BNUM_SEL, val, 0x03);
}

void mtk_hdmi_hw_aud_set_i2s_fmt(struct mediatek_hdmi *hdmi,
				 enum hdmi_aud_i2s_fmt i2s_fmt)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG0);
	val &= ~0x33;

	switch (i2s_fmt) {
	case HDMI_I2S_MODE_RJT_24BIT:
		val |= (CFG0_I2S_MODE_RTJ | CFG0_I2S_MODE_24BIT);
		break;
	case HDMI_I2S_MODE_RJT_16BIT:
		val |= (CFG0_I2S_MODE_RTJ | CFG0_I2S_MODE_16BIT);
		break;
	case HDMI_I2S_MODE_LJT_24BIT:
		val |= (CFG0_I2S_MODE_LTJ | CFG0_I2S_MODE_24BIT);
		break;
	case HDMI_I2S_MODE_LJT_16BIT:
		val |= (CFG0_I2S_MODE_LTJ | CFG0_I2S_MODE_16BIT);
		break;
	case HDMI_I2S_MODE_I2S_24BIT:
		val |= (CFG0_I2S_MODE_I2S | CFG0_I2S_MODE_24BIT);
		break;
	case HDMI_I2S_MODE_I2S_16BIT:
		val |= (CFG0_I2S_MODE_I2S | CFG0_I2S_MODE_16BIT);
		break;
	default:
		break;
	}
	mtk_hdmi_write_grl(hdmi, GRL_CFG0, val);
}

void mtk_hdmi_hw_aud_set_high_bitrate(struct mediatek_hdmi
				      *hdmi, bool enable)
{
	u32 val = 0;

	if (enable) {
		val = mtk_hdmi_read_grl(hdmi, GRL_AOUT_BNUM_SEL);
		val |= HIGH_BIT_RATE_PACKET_ALIGN;
		mtk_hdmi_write_grl(hdmi, GRL_AOUT_BNUM_SEL, val);

		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val |= HIGH_BIT_RATE;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	} else {
		val = mtk_hdmi_read_grl(hdmi, GRL_AOUT_BNUM_SEL);
		val &= ~HIGH_BIT_RATE_PACKET_ALIGN;
		mtk_hdmi_write_grl(hdmi, GRL_AOUT_BNUM_SEL, val);

		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val &= ~HIGH_BIT_RATE;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	}
}

void mtk_hdmi_phy_aud_dst_normal_double_enable(struct mediatek_hdmi
					       *hdmi, bool enable)
{
	u32 val = 0;

	if (enable) {
		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val |= DST_NORMAL_DOUBLE;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	} else {
		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val &= ~DST_NORMAL_DOUBLE;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	}
}

void mtk_hdmi_hw_aud_dst_enable(struct mediatek_hdmi *hdmi,
				bool enable)
{
	u32 val = 0;

	if (enable) {
		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val |= SACD_DST;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	} else {
		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val &= ~SACD_DST;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	}
}

void mtk_hdmi_hw_aud_dsd_enable(struct mediatek_hdmi *hdmi,
				bool enable)
{
	u32 val = 0;

	if (!enable) {
		val = mtk_hdmi_read_grl(hdmi, GRL_AUDIO_CFG);
		val &= ~SACD_SEL;
		mtk_hdmi_write_grl(hdmi, GRL_AUDIO_CFG, val);
	}
}

void mtk_hdmi_hw_aud_set_i2s_chan_num(struct mediatek_hdmi
				      *hdmi,
				      enum hdmi_aud_channel_type channel_type,
				      u8 channel_count)
{
	u8 val_1, val_2, val_3, val_4;

	if (channel_count == 2) {
		val_1 = 0x04;
		val_2 = 0x50;
	} else if (channel_count == 3 || channel_count == 4) {
		if (channel_count == 4 &&
		    (channel_type == HDMI_AUD_CHAN_TYPE_3_0_LRS ||
		    channel_type == HDMI_AUD_CHAN_TYPE_4_0)) {
			val_1 = 0x14;
		} else {
			val_1 = 0x0c;
		}
		val_2 = 0x50;
	} else if (channel_count == 6 || channel_count == 5) {
		if (channel_count == 6 &&
		    channel_type != HDMI_AUD_CHAN_TYPE_5_1 &&
		    channel_type != HDMI_AUD_CHAN_TYPE_4_1_CLRS) {
			val_1 = 0x3c;
			val_2 = 0x50;
		} else {
			val_1 = 0x1c;
			val_2 = 0x50;
		}
	} else if (channel_count == 8 || channel_count == 7) {
		val_1 = 0x3c;
		val_2 = 0x50;
	} else {
		val_1 = 0x04;
		val_2 = 0x50;
	}

	val_3 = 0xc6;
	val_4 = 0xfa;

	mtk_hdmi_write_grl(hdmi, GRL_CH_SW0, val_2);
	mtk_hdmi_write_grl(hdmi, GRL_CH_SW1, val_3);
	mtk_hdmi_write_grl(hdmi, GRL_CH_SW2, val_4);
	mtk_hdmi_write_grl(hdmi, GRL_I2S_UV, val_1);
}

void mtk_hdmi_hw_aud_set_input_type(struct mediatek_hdmi *hdmi,
				    enum hdmi_aud_input_type input_type)
{
	u32 val = 0;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG1);
	if (input_type == HDMI_AUD_INPUT_I2S &&
	    (val & CFG1_SPDIF) == CFG1_SPDIF) {
		val &= ~CFG1_SPDIF;
	} else if (input_type == HDMI_AUD_INPUT_SPDIF &&
		(val & CFG1_SPDIF) == 0) {
		val |= CFG1_SPDIF;
	}
	mtk_hdmi_write_grl(hdmi, GRL_CFG1, val);
}

void mtk_hdmi_hw_aud_set_channel_status(struct mediatek_hdmi
					*hdmi, u8 *l_chan_status,
					u8 *r_chan_staus,
					enum hdmi_audio_sample_frequency
					aud_hdmi_fs)
{
	u8 l_status[5];
	u8 r_status[5];
	u8 val = 0;

	l_status[0] = l_chan_status[0];
	l_status[1] = l_chan_status[1];
	l_status[2] = l_chan_status[2];
	r_status[0] = r_chan_staus[0];
	r_status[1] = r_chan_staus[1];
	r_status[2] = r_chan_staus[2];

	l_status[0] &= ~0x02;
	r_status[0] &= ~0x02;

	val = l_chan_status[3] & 0xf0;
	switch (aud_hdmi_fs) {
	case HDMI_AUDIO_SAMPLE_FREQUENCY_32000:
		val |= 0x03;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_44100:
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_88200:
		val |= 0x08;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_96000:
		val |= 0x0a;
		break;
	case HDMI_AUDIO_SAMPLE_FREQUENCY_48000:
		val |= 0x02;
		break;
	default:
		val |= 0x02;
		break;
	}
	l_status[3] = val;
	r_status[3] = val;

	val = l_chan_status[4];
	val |= ((~(l_status[3] & 0x0f)) << 4);
	l_status[4] = val;
	r_status[4] = val;

	val = l_status[0];
	mtk_hdmi_write_grl(hdmi, GRL_I2S_C_STA0, val);
	mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_0, val);

	val = r_status[0];
	mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_0, val);

	val = l_status[1];
	mtk_hdmi_write_grl(hdmi, GRL_I2S_C_STA1, val);
	mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_1, val);

	val = r_status[1];
	mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_1, val);

	val = l_status[2];
	mtk_hdmi_write_grl(hdmi, GRL_I2S_C_STA2, val);
	mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_2, val);

	val = r_status[2];
	mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_2, val);

	val = l_status[3];
	mtk_hdmi_write_grl(hdmi, GRL_I2S_C_STA3, val);
	mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_3, val);

	val = r_status[3];
	mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_3, val);

	val = l_status[4];
	mtk_hdmi_write_grl(hdmi, GRL_I2S_C_STA4, val);
	mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_4, val);

	val = r_status[4];
	mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_4, val);

	for (val = 0; val < 19; val++) {
		mtk_hdmi_write_grl(hdmi, GRL_L_STATUS_5 + val * 4, 0);
		mtk_hdmi_write_grl(hdmi, GRL_R_STATUS_5 + val * 4, 0);
	}
}

void mtk_hdmi_hw_aud_src_reenable(struct mediatek_hdmi *hdmi)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_MIX_CTRL);
	if (val & MIX_CTRL_SRC_EN) {
		val &= ~MIX_CTRL_SRC_EN;
		mtk_hdmi_write_grl(hdmi, GRL_MIX_CTRL, val);
		usleep_range(255, 512);
		val |= MIX_CTRL_SRC_EN;
		mtk_hdmi_write_grl(hdmi, GRL_MIX_CTRL, val);
	}
}

void mtk_hdmi_hw_aud_src_off(struct mediatek_hdmi *hdmi)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_MIX_CTRL);
	val &= ~MIX_CTRL_SRC_EN;
	mtk_hdmi_write_grl(hdmi, GRL_MIX_CTRL, val);
	mtk_hdmi_write_grl(hdmi, GRL_SHIFT_L1, 0x00);
}

void mtk_hdmi_hw_aud_set_mclk(struct mediatek_hdmi *hdmi,
			      enum hdmi_aud_mclk mclk)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG5);
	val &= CFG5_CD_RATIO_MASK;

	switch (mclk) {
	case HDMI_AUD_MCLK_128FS:
		val |= CFG5_FS128;
		break;
	case HDMI_AUD_MCLK_256FS:
		val |= CFG5_FS256;
		break;
	case HDMI_AUD_MCLK_384FS:
		val |= CFG5_FS384;
		break;
	case HDMI_AUD_MCLK_512FS:
		val |= CFG5_FS512;
		break;
	case HDMI_AUD_MCLK_768FS:
		val |= CFG5_FS768;
		break;
	default:
		val |= CFG5_FS256;
		break;
	}
	mtk_hdmi_write_grl(hdmi, GRL_CFG5, val);
}

void mtk_hdmi_hw_aud_aclk_inv_enable(struct mediatek_hdmi *hdmi,
				     bool enable)
{
	u32 val;

	val = mtk_hdmi_read_grl(hdmi, GRL_CFG2);
	if (enable)
		val |= 0x80;
	else
		val &= ~0x80;
	mtk_hdmi_write_grl(hdmi, GRL_CFG2, val);
}

static void do_hdmi_hw_aud_set_ncts(struct mediatek_hdmi *hdmi,
				    enum HDMI_DISPLAY_COLOR_DEPTH depth,
				    enum hdmi_audio_sample_frequency freq,
				    int pix)
{
	unsigned char val[NCTS_BYTES];
	unsigned int temp;
	int i = 0;

	mtk_hdmi_write_grl(hdmi, GRL_NCTS, 0);
	mtk_hdmi_write_grl(hdmi, GRL_NCTS, 0);
	mtk_hdmi_write_grl(hdmi, GRL_NCTS, 0);
	memset(val, 0, sizeof(val));

	if (depth == HDMI_DEEP_COLOR_24BITS) {
		for (i = 0; i < NCTS_BYTES; i++) {
			if ((freq < 8) && (pix < 9))
				val[i] = HDMI_NCTS[freq - 1][pix][i];
		}
		temp = (val[0] << 24) | (val[1] << 16) |
			(val[2] << 8) | (val[3]);	/* CTS */
	} else {
		for (i = 0; i < NCTS_BYTES; i++) {
			if ((freq < 7) && (pix < 9))
				val[i] = HDMI_NCTS[freq - 1][pix][i];
		}

		temp =
		    (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | (val[3]);

		if (depth == HDMI_DEEP_COLOR_30BITS)
			temp = (temp >> 2) * 5;
		else if (depth == HDMI_DEEP_COLOR_36BITS)
			temp = (temp >> 1) * 3;
		else if (depth == HDMI_DEEP_COLOR_48BITS)
			temp = (temp << 1);

		val[0] = (temp >> 24) & 0xff;
		val[1] = (temp >> 16) & 0xff;
		val[2] = (temp >> 8) & 0xff;
		val[3] = (temp) & 0xff;
	}

	for (i = 0; i < NCTS_BYTES; i++)
		mtk_hdmi_write_grl(hdmi, GRL_NCTS, val[i]);
}

void mtk_hdmi_hw_aud_set_ncts(struct mediatek_hdmi *hdmi,
			      enum HDMI_DISPLAY_COLOR_DEPTH depth,
			      enum hdmi_audio_sample_frequency freq, int clock)
{
	int pix = 0;

	switch (clock) {
	case 27000:
		pix = 0;
		break;
	case 74175:
		pix = 2;
		break;
	case 74250:
		pix = 3;
		break;
	case 148350:
		pix = 4;
		break;
	case 148500:
		pix = 5;
		break;
	default:
		pix = 0;
		break;
	}

	mtk_hdmi_mask_grl(hdmi, DUMMY_304, AUDIO_I2S_NCTS_SEL_64,
			  AUDIO_I2S_NCTS_SEL);
	do_hdmi_hw_aud_set_ncts(hdmi, depth, freq, pix);
}

void mtk_hdmi_hw_htplg_irq_enable(struct mediatek_hdmi *hdmi)
{
	mtk_hdmi_mask_cec(hdmi, CEC_CKGEN, 0, PDN);

	mtk_hdmi_mask_cec(hdmi, CEC_CKGEN, CEC_32K_PDN, CEC_32K_PDN);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, HDMI_PORD_INT_32K_CLR,
			  HDMI_PORD_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, RX_INT_32K_CLR, RX_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, HDMI_HTPLG_INT_32K_CLR,
			  HDMI_HTPLG_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_PORD_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, RX_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_HTPLG_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_PORD_INT_32K_EN);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, RX_INT_32K_EN);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_HTPLG_INT_32K_EN);

	mtk_hdmi_mask_cec(hdmi, RX_EVENT, HDMI_PORD_INT_EN, HDMI_PORD_INT_EN);
	mtk_hdmi_mask_cec(hdmi, RX_EVENT, HDMI_HTPLG_INT_EN, HDMI_HTPLG_INT_EN);
}

void mtk_hdmi_hw_clear_htplg_irq(struct mediatek_hdmi *hdmi)
{
	mtk_hdmi_mask_cec(hdmi, TR_CONFIG, CLEAR_CEC_IRQ, CLEAR_CEC_IRQ);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL,
			  HDMI_HTPLG_INT_CLR, HDMI_HTPLG_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL,
			  HDMI_PORD_INT_CLR, HDMI_PORD_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL,
			  HDMI_FULL_INT_CLR, HDMI_FULL_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, HDMI_PORD_INT_32K_CLR,
			  HDMI_PORD_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, RX_INT_32K_CLR, RX_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, HDMI_HTPLG_INT_32K_CLR,
			  HDMI_HTPLG_INT_32K_CLR);
	udelay(5);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL, 0, HDMI_HTPLG_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL, 0, HDMI_PORD_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, TR_CONFIG, 0, CLEAR_CEC_IRQ);
	mtk_hdmi_mask_cec(hdmi, NORMAL_INT_CTRL, 0, HDMI_FULL_INT_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_PORD_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, RX_INT_32K_CLR);
	mtk_hdmi_mask_cec(hdmi, RX_GEN_WD, 0, HDMI_HTPLG_INT_32K_CLR);
}

