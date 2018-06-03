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

ifeq ($(CONFIG_MACH_MT6763),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/
endif

ccflags-$(CONFIG_MTK_GPU_SPM_DVFS_SUPPORT) += -DMTK_GPU_SPM

obj-$(CONFIG_MACH_MT6763) += mtk_clkmgr.o

ifneq ($(CONFIG_FPGA_EARLY_PORTING), y)

obj-$(CONFIG_MACH_MT6763) += mtk_gpufreq.o
obj-y += mtk_clkbuf_ctl.o
obj-y += mtk_dcm_autogen.o
obj-y += mtk_dcm.o
#obj-y += mt_clkmgr_common.o

#obj-$(CONFIG_MACH_MT6763) += mt_spm_dpidle.o
#obj-$(CONFIG_MACH_MT6763M) += mt_spm_dpidle.o
#obj-$(CONFIG_MACH_MT6753) += mt_spm_dpidle.o
#obj-$(CONFIG_MACH_MT6763) += mt_spm_fs.o
#obj-$(CONFIG_MACH_MT6763) += mt_spm_sodi.o
#obj-$(CONFIG_MACH_MT6763M) += mt_spm_sodi.o
#obj-y += mt_spm_mtcmos.o
#obj-y += mt_pbm.o
obj-y += mtk_picachu.o
obj-y += mtk_cpufreq.o
obj-y += mtk_cpufreq_hybrid.o
obj-y += mtk_udi.o
#obj-y += mt_gpufreq.o
#obj-$(CONFIG_MACH_MT6753) += mt_gpufreq.o
obj-y += mtk_eem.o
obj-y += mtk_eem_turbo.o
obj-y += mtk_lowpower.o
#obj-y += mtk_vcorefs_manager.o
#obj-y += mtk_vcorefs_governor.o
obj-y += mtk_static_power.o

endif
