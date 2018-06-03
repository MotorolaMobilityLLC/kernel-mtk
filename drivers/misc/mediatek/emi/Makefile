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

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

ccflags-$(CONFIG_MTK_EMI) += -I$(srctree)/drivers/misc/mediatek/emi/$(MTK_PLATFORM)
ccflags-$(CONFIG_MTK_EMI) += -I$(srctree)/drivers/misc/mediatek/emi/submodule

ifeq ($(CONFIG_MACH_MT6763), y)
obj-$(CONFIG_MTK_EMI) += emi_ctrl_v1.o
endif

obj-$(CONFIG_MTK_EMI) += submodule/
obj-$(CONFIG_MTK_EMI) += $(MTK_PLATFORM)/

