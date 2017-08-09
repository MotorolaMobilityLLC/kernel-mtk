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
#ifndef _MTK_HDMI_PHY_H
#define _MTK_HDMI_PHY_H

void mtk_hdmi_phy_set_pll(struct phy *phy, u32 clock,
			  enum hdmi_display_color_depth depth);

#endif /* _MTK_HDMI_PHY_H */
