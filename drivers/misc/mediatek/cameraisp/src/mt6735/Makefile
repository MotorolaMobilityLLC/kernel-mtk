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

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/

ifneq ($(CONFIG_ARM64), y)
# For arm32
  ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
    obj-y += cameraisp_dummy2.o
    obj-y += camera_isp_D2.o
    obj-y += camera_sysram_D2.o
    obj-y += camera_pipe_mgr_D2.o
  else
    obj-y += camera_isp_D1.o
    obj-y += camera_sysram_D1.o
    obj-y += camera_fdvt_D1.o
  endif
else
# For arm64
  ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
    obj-y += cameraisp_dummy2.o
    obj-y += camera_isp_D2.o
    obj-y += camera_sysram_D2.o
    obj-y += camera_pipe_mgr_D2.o
  else
    obj-y += camera_isp_D1.o
    obj-y += camera_sysram_D1.o
    obj-y += camera_fdvt_D1.o
  endif
endif

$(info cameraisp drv by platform $(platform_drv))
