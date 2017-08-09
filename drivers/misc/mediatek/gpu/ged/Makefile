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

GED_TOP = $(srctree)/drivers/misc/mediatek/gpu/ged
MTK_TOP = $(srctree)/drivers/misc/mediatek/

VER:=eng

ifneq ($(strip $(TARGET_BUILD_VARIANT)),)
ifneq ($(strip $(TARGET_BUILD_VARIANT)),eng)
VER:=user
ccflags-y += -DGED_LOG_SIZE_LIMITED
endif
endif

ccflags-y += \
	-I$(srctree)/include/ \
    -I$(MTK_TOP)/include/ \
    -I$(GED_TOP)/include/ \
    -I$(MTK_TOP)/base/power/$(subst ",,$(CONFIG_MTK_PLATFORM))/
	
$(info $$CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT is [${CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT}])
    
ifeq ($(CONFIG_MTK_GPU_COMMON_DVFS_SUPPORT),y)
ccflags-y += -DGED_DVFS_ENABLE \
             -DENABLE_COMMON_DVFS \
	     -DENABLE_TIMER_BACKUP \
             -DGED_DVFS_DEBUG_BUF
endif

ccflags-y += \
	-include $(GED_TOP)/include/config_kernel_$(VER).h

obj-y += ged.o

ged-y += \
	src/ged_base.o \
	src/ged_main.o \
	src/ged_debugFS.o \
	src/ged_log.o \
	src/ged_hal.o \
	src/ged_bridge.o \
	src/ged_profile_dvfs.o \
	src/ged_monitor_3D_fence.o \
	src/ged_notify_sw_vsync.o \
	src/ged_hashtable.o \
	src/ged_thread.o \
	src/ged_dvfs.o

