#
# Copyright (C) 2017 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.
#

ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ifeq ($(filter $(CONFIG_MACH_MT6763) $(CONFIG_MACH_MT6758), y), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/cpuidle_v2/
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/cpuidle_v3/
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include/mcdi_v1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/

obj-y += mtk_mcdi.o
obj-y += mtk_mcdi_state.o
obj-y += mtk_mcdi_profile.o

ifeq ($(CONFIG_MACH_MT6763), y)
obj-y += mtk_mcdi_mt6763.o
endif
ifeq ($(CONFIG_MACH_MT6758), y)
obj-y += mtk_mcdi_mt6758.o
endif
ifeq ($(CONFIG_MACH_MT6771), y)
obj-y += mtk_mcdi_mt6771.o
endif
ifeq ($(CONFIG_MACH_MT6775), y)
obj-y += mtk_mcdi_mt6775.o
endif

