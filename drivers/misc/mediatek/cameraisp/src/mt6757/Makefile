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
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk

ifneq ($(CONFIG_ARM64), y)
# For arm32
  obj-y += camera_isp.o
  #obj-y += camera_sysram.o
  obj-y += camera_fdvt.o
  #obj-y += camera_eis.o
  #obj-y += camera_dpe.o
  obj-y += camera_tsf.o
else
# For arm64
  obj-y += camera_isp.o
  #obj-y += camera_sysram.o
  obj-y += camera_fdvt.o
  #obj-y += camera_eis.o
  #obj-y += camera_dpe.o
  obj-y += camera_tsf.o
endif

$(info cameraisp drv by platform $(platform_drv))
