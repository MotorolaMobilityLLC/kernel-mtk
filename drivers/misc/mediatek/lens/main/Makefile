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

#
# Makefile for misc devices that really don't fit anywhere else.
#

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek


subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/main/inc
subdir-ccflags-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += -I$(MTK_CUSTOM_PATH)/lens/main/common/lc898122af
subdir-ccflags-$(CONFIG_MTK_LENS_LC898212XDAF_SUPPORT) += -I$(MTK_CUSTOM_PATH)/lens/main/common/lc898212xdaf/inc
subdir-ccflags-$(CONFIG_MTK_LENS_LC898212XDAF_SUPPORT) += -I$(MTK_CUSTOM_PATH)/lens/main/common/lc898212xdaf_f/inc
subdir-ccflags-$(CONFIG_MTK_LENS_BU63165AF_SUPPORT) += -I$(MTK_CUSTOM_PATH)/lens/main/common/bu63165af/inc
subdir-ccflags-$(CONFIG_MTK_LENS_BU63169AF_SUPPORT) += -I$(MTK_CUSTOM_PATH)/lens/main/common/bu63169af/inc

ifeq ($(CONFIG_MTK_LENS_USE_MODULE), y)

MAIN_CFILES += main_lens.c

ifeq ($(CONFIG_MTK_LENS_AD5820AF_SUPPORT),y)
MAIN_CFILES += common/ad5820af/AD5820AF.c
endif

ifeq ($(CONFIG_MTK_LENS_DW9714AF_SUPPORT),y)
MAIN_CFILES += common/dw9714af/DW9714AF.c
endif
ifeq ($(CONFIG_MTK_LENS_LC898212AF_SUPPORT),y)
MAIN_CFILES += common/lc898212af/LC898212AF.c
endif

obj-m := mainaf.o
mainaf-objs := $(MAIN_CFILES:.c=.o)

else

obj-y      += main_lens.o
obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)      += common/fm50af/
obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT)    += common/bu6424af/
obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT)    += common/bu6429af/
obj-$(CONFIG_MTK_LENS_BU63165AF_SUPPORT)  += common/bu63165af/
obj-$(CONFIG_MTK_LENS_BU63169AF_SUPPORT)  += common/bu63169af/
obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)    += common/dw9714af/
obj-$(CONFIG_MTK_LENS_DW9814AF_SUPPORT)    += common/dw9814af/
obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)    += common/dw9718af/
obj-$(CONFIG_MTK_LENS_DW9718SAF_SUPPORT)    += common/dw9718saf/
obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT)  += common/lc898212af/
obj-$(CONFIG_MTK_LENS_LC898212XDAF_SUPPORT)  += common/lc898212xdaf/
obj-$(CONFIG_MTK_LENS_LC898214AF_SUPPORT)  += common/lc898214af/
obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT)  += common/lc898122af/
obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT)    += common/ad5820af/
obj-$(CONFIG_MTK_LENS_WV511AAF_SUPPORT)    += common/wv511aaf/
obj-$(CONFIG_MTK_LENS_AK7371AF_SUPPORT)    += common/ak7371af/

endif