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


ifeq ($(CONFIG_MTK_MULTIBRIDGE_SUPPORT),y)

ccflags-y += -I$(srctree)/drivers/misc/mediatek/multibridge/$(CONFIG_MTK_PLATFORM)/mt8193/inc

obj-y   +=  mt8193_ckgen.o
obj-y   +=  mt8193_ckgen_vfy.o
obj-y   +=  mt8193_gpio.o
obj-y   +=  mt8193_iic.o
obj-y   +=  mt8193_pinmux.o
endif