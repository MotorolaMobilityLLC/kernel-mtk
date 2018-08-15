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

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/

ifeq ($(CONFIG_MACH_MT6761),y)
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/mt6765/
else
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
endif

ccflags-$(CONFIG_MTK_M4U) += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-$(CONFIG_IOMMU_API) += -I$(srctree)/drivers/iommu
ccflags-$(CONFIG_IOMMU_API) += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include

ifneq ($(CONFIG_ARM64), y)
# For arm32
  obj-y += camera_isp.o
#  obj-y += camera_sysram.o
else
# For arm64
  obj-y += camera_isp.o
#  obj-y += camera_sysram.o
endif


$(info cameraisp drv by platform $(platform_drv))
