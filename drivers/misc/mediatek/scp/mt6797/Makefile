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

obj-y += scp.o
scp-y := scp_helper.o scp_excep.o scp_ipi.o scp_irq.o scp_logger.o scp_dvfs.o

# include emi_mpu.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
# include vcorefs_manager.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)/

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_scp_logger.o += -DDEBUG
CFLAGS_scp_helper.o += -DDEBUG
CFLAGS_scp_excep.o += -DDEBUG
CFLAGS_scp_ipi.o += -DDEBUG
CFLAGS_scp_irq.o += -DDEBUG
endif
