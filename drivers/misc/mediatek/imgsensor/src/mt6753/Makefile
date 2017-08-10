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
#include $(srctree)/drivers/misc/mediatek/imgsensor/src/Makefile.custom


define FILTER_PROJECT_DRV
ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(1)),)
platform_drv += $(1)
else
project_drv += camera_project/$(ARCH_MTK_PROJECT)/$(1)
endif
endef

ifeq ($(CONFIG_ARCH_MT6753),y)
MTK_PLATFORM := mt6753
endif

# MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

obj-y				+= kd_sensorlist.o

$(info -= project path  $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/ =-)

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/imgsensor/src/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw),)
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/camera_hw/
$(info by porject kd_camera_hw)
else
obj-y                           += camera_hw/
endif

$(foreach drv,$(subst $\",,$(CONFIG_CUSTOM_KERNEL_IMGSENSOR)),$(eval $(call FILTER_PROJECT_DRV,$(drv))))
custom_kernel_imgsensor_by_platform := $(addsuffix /, $(shell echo $(platform_drv)))
custom_kernel_imgsensor_by_project := $(addsuffix /, $(shell echo $(project_drv)))

obj-y += $(custom_kernel_imgsensor_by_platform)
obj-y += $(custom_kernel_imgsensor_by_project)

$(info imgsensor drv by platform $(platform_drv))
$(info imgsensor drv by project $(project_drv))
