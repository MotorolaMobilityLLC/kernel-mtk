#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

obj-$(CONFIG_COMMON_CLK_MEDIATEK_NEW) += clk-mtk.o clk-pll.o clk-gate.o clk-mux.o clk-apmixed.o
obj-$(CONFIG_COMMON_CLK_MT6799) += clk-mt6799.o clk-mt6799-pg.o mt_clkmgr.o mt_clkdbg.o clk-mtk-v1.o
obj-$(CONFIG_COMMON_CLK_MT6763) += clk-mt6763.o clk-mt6763-pg.o clk-mtk-v1.o
obj-$(CONFIG_COMMON_CLK_MT6759) += clk-mt6759.o clk-mt6759-pg.o mt6759_clkmgr.o clk-mtk-v1.o
obj-$(CONFIG_COMMON_CLK_MT8167) += clk-mt8167.o clkchk_mt8167.o
obj-$(CONFIG_RESET_CONTROLLER) += reset.o

obj-$(CONFIG_COMMON_CLK_MEDIATEK_V1) += clk-mtk-v1.o clk-pll-v1.o clk-gate-v1.o clk-mux.o
obj-$(CONFIG_COMMON_CLK_MT6757) += clk-mt6757.o clk-mt6757-pll.o clk-mt6757-pg.o

ifeq ($(CONFIG_DEBUG_FS), y)
obj-$(CONFIG_COMMON_CLK_MEDIATEK_NEW) += clkdbg.o
obj-$(CONFIG_COMMON_CLK_MT8167) += clkdbg_mt8167.o
obj-$(CONFIG_COMMON_CLK_MT6759) += clkdbg_mt6759.o
endif
