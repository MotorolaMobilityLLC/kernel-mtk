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

ccflags-y += -I$(srctree)/drivers/misc/mediatek/irq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/cpuidle_v1/

ifeq ($(CONFIG_ARCH_MT6757),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
endif

ccflags-$(CONFIG_MTK_GPU_SPM_DVFS_SUPPORT) += -DMTK_GPU_SPM

obj-y += mt_pm_init.o
obj-y += mt_static_power.o
obj-y += mt_vcorefs_manager.o
obj-y += mt_vcorefs_governor.o

obj-$(CONFIG_ARCH_MT6757) += mt_clkmgr.o
ifneq ($(CONFIG_FPGA_EARLY_PORTING), y)

obj-y += mt_clkbuf_ctl.o
obj-y += mt_dcm.o
obj-y += mt_clkmgr_common.o

#obj-$(CONFIG_ARCH_MT6757) += mt_spm_dpidle.o
#obj-$(CONFIG_ARCH_MT6757M) += mt_spm_dpidle.o
#obj-$(CONFIG_ARCH_MT6753) += mt_spm_dpidle.o
#obj-$(CONFIG_ARCH_MT6757) += mt_spm_fs.o
#obj-$(CONFIG_ARCH_MT6757) += mt_spm_sodi.o
#obj-$(CONFIG_ARCH_MT6757M) += mt_spm_sodi.o
#obj-y += mt_spm_mtcmos.o
#obj-y += mt_pbm.o
obj-y += mt_picachu.o
obj-y += mt_cpufreq.o
obj-y += mt_cpufreq_hybrid.o
obj-y += mt_udi.o
obj-y += mt_gpufreq.o
#obj-$(CONFIG_ARCH_MT6753) += mt_gpufreq.o
obj-y += mt_eem.o
obj-y += mt_eem_turbo.o

ifneq ($(CONFIG_ARM64), y)

obj-y += mt-smp.o
obj-y += hotplug.o
obj-y += mt-headsmp.o
obj-y += mt_hotplug.o

else

endif

endif
