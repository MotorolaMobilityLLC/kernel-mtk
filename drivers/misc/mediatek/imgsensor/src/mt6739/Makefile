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

#
# Makefile for misc devices that really don't fit anywhere else.
#

define FILTER_PLATFORM_DRV
ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(1)),)
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/$(1)),)
platform_drv += $(1)
endif
endif
endef

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))


$(foreach drv,$(subst $\",,$(CONFIG_CUSTOM_KERNEL_IMGSENSOR)),$(eval $(call FILTER_PLATFORM_DRV,$(drv))))
custom_kernel_imgsensor_by_platform := $(addsuffix /, $(shell echo $(platform_drv)))

ifneq ($(platform_drv),)
$(info imgsensor drv by platform $(platform_drv))
endif

ifeq (,$(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw))
obj-y				+= camera_hw/
endif
ifneq (,$(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)))
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/
endif
obj-y				+= imgsensor_clk.o
obj-y				+= $(custom_kernel_imgsensor_by_platform)
