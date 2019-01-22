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

ifeq ($(CONFIG_MACH_MT6739), y)
        ccflags-y += -DTOUCH_VCORE_OPP=0
endif

ifeq ($(CONFIG_MACH_MT6771), y)
		ccflags-y += -DCONFIG_MTK_FPSGO_V2
		ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/fpsgo_v2/fstb/
endif
ifeq ($(CONFIG_MACH_MT6775), y)
		ccflags-y += -DCONFIG_MTK_FPSGO_V2
		ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/fpsgo_v2/fstb/
endif

ifeq ($(CONFIG_MTK_FPSGO_FBT_UX),y)
        obj-y += fbc.o
else
        ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/performance/perfmgr/$(MTK_PLATFORM)/),)
            obj-y += touch_boost.o
        endif
endif

ccflags-y += \
	-I$(srctree)/drivers/misc/mediatek/base/power/include \
	-I$(srctree)/kernel/trace/ \

