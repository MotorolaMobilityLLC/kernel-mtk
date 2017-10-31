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

ifeq ($(strip $(CONFIG_ARCH_MT6735)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D1.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D2.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

ifeq ($(strip $(CONFIG_ARCH_MT6753)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D3.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += vcodec_kernel_driver.o


ccflags-y += \
    -I$(srctree)/drivers/misc/mediatek/videocodec/$(MTK_PLATFORM)/common/drv/inc		 \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include   \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include/kernel   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach   \
    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
    -I$(srctree)/drivers/misc/mediatek/smi

