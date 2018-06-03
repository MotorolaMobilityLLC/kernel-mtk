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

PVRSRVKM_NAME = $(PVRSRV_MODNAME)

$(PVRSRVKM_NAME)-y += \
	services/system/rgx_mtk/ion_support.o \
	services/system/rgx_mtk/mtk_pp.o \
	services/system/rgx_mtk/sysconfig.o \
	services/system/$(PVR_SYSTEM)/mtk_mfgsys.o \
	services/system/$(PVR_SYSTEM)/mtk_mfg_counter.o \
	services/system/common/env/linux/interrupt_support.o

ccflags-y += \
	-I$(TOP)/include/system/rgx_mt6739 \
	-I$(TOP)/services/system/common/env/linux \
	-I$(TOP)/services/linux/include \
	-I$(TOP)/kernel/drivers/staging/imgtec \
	-I$(srctree)/drivers/staging/android/ion \
	-I$(srctree)/drivers/staging/android/ion/mtk \
	-I$(srctree)/drivers/misc/mediatek/gpu/ged/include \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat \
	-I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
