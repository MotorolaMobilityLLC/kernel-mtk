#
# Copyright (C) 2017 MediaTek Inc.
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

#
# Makefile for misc devices that really don't fit anywhere else.
#
ifneq ($(CONFIG_PINCTRL_MT6799),y)
# to use mtk legacy pinctrl
subdir-ccflags-y += -I$(DRVGEN_OUT)/inc
endif
MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \

obj-y		+= imgsensor_cfg_table.o

obj-y		+= regulator/
obj-y		+= gpio/

