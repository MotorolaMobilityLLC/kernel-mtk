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

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \

#$(warning "CONFIG_TABLET_PROJECT:$(CONFIG_TABLET_PROJECT)")
ifeq ($(CONFIG_TABLET_PROJECT),y)
# is tablet project
obj-y				+= tb_kd_camera_hw.o
else
# not tablet project
obj-y				+= kd_camera_hw.o
endif
