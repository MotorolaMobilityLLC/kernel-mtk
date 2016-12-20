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

vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o


obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += vcodec_kernel_driver.o


ccflags-y += \
    -I$(srctree)/drivers/misc/mediatek/videocodec/$(MTK_PLATFORM)/common/drv/inc		 \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include   \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include/kernel   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach   \
    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
    -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox \
    -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys \
    -I$(srctree)/drivers/misc/mediatek/video/include \
    -I$(srctree)/drivers/misc/mediatek/lcm/inc \
    -I$(srctree)/drivers/misc/mediatek/cmdq/v2/ \
    -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/ \
    -I$(srctree)/sound/soc/mediatek/common/ \
    -I$(srctree)/sound/soc/mediatek/$(MTK_PLATFORM)/ \
    -I$(srctree)/sound/soc/mediatek/codec/mt6351/ \

