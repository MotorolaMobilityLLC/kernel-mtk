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

#add kernel source code path as head file search path
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
#workaround for include mmc/card/queue.h and mmc/core/core.h
ccflags-y += -I$(srctree)/drivers/mmc

obj-$(CONFIG_MMC_MTK) += msdc.o

msdc-y += sd.o mt_sd_misc.o
msdc-y += msdc_tune.o
msdc-y += dbg.o
msdc-y += msdc_partition.o
msdc-y += autok.o
msdc-y += autok_dvfs.o

msdc-y += msdc_io.o
ifeq ($(CONFIG_MTK_FPGA),y)
msdc-y += msdc_fpga_io.o
endif

ifeq ($(CONFIG_MTK_AEE_IPANIC),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(MTK_PLATFORM)
msdc-y += mt_dump.o
endif

ifeq ($(CONFIG_MTK_EMMC_SUPPORT_OTP),y)
msdc-y += emmc_otp.o
endif
