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

ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/spm_v1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/cpuidle_v1/

ifeq ($(CONFIG_ARCH_MT6580),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
endif

obj-y += mt_spm_mtcmos.o
obj-y += mt_clkmgr_common.o
obj-y += mt_clkmgr.o
obj-y += mt_golden_setting.o
obj-y += mt_clkbuf_ctl.o
obj-y += mt_pm_init.o
obj-y += mt_sleep.o
obj-y += mt_cpufreq.o
obj-y += mt_gpufreq.o
obj-y += mt_ptp.o
obj-y += mt_dcm.o

obj-y += mt-smp.o
obj-y += hotplug.o
obj-y += mt-headsmp.o
obj-y += mt_hotplug.o

obj-y += mt_hotplug_strategy_main.o
obj-y += mt_hotplug_strategy_core.o
obj-y += mt_hotplug_strategy_procfs.o
obj-y += mt_hotplug_strategy_cpu.o
obj-y += mt_hotplug_strategy_api.o
obj-y += mt_hotplug_strategy_algo.o

ifeq ($(CONFIG_MICROTRUST_TEE_SUPPORT),y)
include $(srctree)/drivers/misc/mediatek/teei/Makefile.include
endif
