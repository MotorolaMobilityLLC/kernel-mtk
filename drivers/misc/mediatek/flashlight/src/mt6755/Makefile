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


obj-$(CONFIG_MTK_FLASHLIGHT)    += flashlight_top.o

flashlight_top-y				+= kd_flashlightlist.o
flashlight_top-y				+= strobe_main_sid2_part1.o
flashlight_top-y				+= strobe_main_sid2_part2.o
flashlight_top-y				+= strobe_sub_sid2_part1.o
flashlight_top-y				+= strobe_sub_sid2_part2.o
flashlight_top-y				+= sub_strobe.o
flashlight_top-y				+= strobe_main_sid1_part2.o
flashlight_top-y				+= strobe_part_id.o
flashlight_top-y				+= strobe_sub_sid1_part2.o

obj-$(CONFIG_MTK_FLASHLIGHT)    += flashlight_device.o

custom_kernel_flashlight := $(shell echo $(CONFIG_CUSTOM_KERNEL_FLASHLIGHT))

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/flashlight/src/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/constant_flashlight),)
flashlight_device-y				+= $(ARCH_MTK_PROJECT)/$(custom_kernel_flashlight)/leds_strobe.o
else
flashlight_device-y				+= $(custom_kernel_flashlight)/leds_strobe.o
endif


