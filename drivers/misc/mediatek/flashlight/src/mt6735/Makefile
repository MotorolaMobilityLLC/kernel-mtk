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

ifneq ($(CONFIG_MTK_FLASHLIGHT_USE_MODULE), y)
obj-y				+= kd_flashlightlist.o
obj-y				+= strobe_main_sid2_part2.o
obj-y				+= strobe_sub_sid2_part1.o
obj-y				+= strobe_sub_sid2_part2.o
obj-y				+= sub_strobe.o
obj-y				+= strobe_main_sid1_part2.o
obj-y				+= strobe_part_id.o
obj-y				+= strobe_sub_sid1_part2.o

ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight/strobe_main_sid2_part1.c),)
obj-y				+= strobe_main_sid2_part1.o
endif


#ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight),)
#ifeq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight),)
custom_kernel_flashlight := $(addsuffix /, $(shell echo $(CONFIG_CUSTOM_KERNEL_FLASHLIGHT)))

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/flashlight/src/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/constant_flashlight),)
obj-y				+= $(ARCH_MTK_PROJECT)/$(custom_kernel_flashlight)
else
obj-y				+= $(custom_kernel_flashlight)
endif

else
CFILES += kd_flashlightlist.o
CFILES += strobe_main_sid2_part2.o
CFILES += strobe_sub_sid2_part1.o
CFILES += strobe_sub_sid2_part2.o
CFILES += sub_strobe.o
CFILES += strobe_main_sid1_part2.o
CFILES += strobe_part_id.o
CFILES += strobe_sub_sid1_part2.o

ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight/strobe_main_sid2_part1.c),)
CFILES += strobe_main_sid2_part1.o
endif

ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight),)
#ifeq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/flashlight),)

custom_kernel_flashlight := $(shell echo $(CONFIG_CUSTOM_KERNEL_FLASHLIGHT))
obj-y += $(custom_kernel_flashlight)/

ifeq ($(CONFIG_CUSTOM_KERNEL_FLASHLIGHT),dummy_flashlight)
#CFILES += $(custom_kernel_flashlight)/dummy_flashlight.o
else
#CFILES += $(custom_kernel_flashlight)/leds_strobe.o
endif
endif

obj-m := flashlight_top.o
flashlight_top-objs := $(CFILES)
endif
