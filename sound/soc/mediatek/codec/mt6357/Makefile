// SPDX-License-Identifier: GPL-2.0
ifneq ($(CONFIG_MACH_MT6739),y)
subdir-ccflags-y += -DAUDIO_USING_WRAP
endif

ifeq ($(CONFIG_SND_SOC_AW87XXX),y)
subdir-ccflags-y += -DCONFIG_SND_SOC_AW87XXX
endif

# PMIC object
obj-y += mtk-soc-codec-6357.o mtk-auddrv-ana.o
