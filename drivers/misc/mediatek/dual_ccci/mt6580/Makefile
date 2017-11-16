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

ifeq ($(CONFIG_MTK_CCCI_EXT), y)

CCCI_CORE_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/dual_ccci/$(MTK_PLATFORM)

ifeq ($(CCCI_CORE_DRIVER_DIR),)
$(error Invalid ccci source path, please check)
endif

obj-$(CONFIG_MTK_CCCI_DRIVER) += ccci_plat.o
ccci_plat-y :=src/ccci_platform.o
ccci_plat-y +=src/ccci_settings.o

#ldflags-y += --whole-archive $(ext-libs)  --no-whole-archive

ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dual_ccci/include
ccflags-y  += -I$(CCCI_CORE_DRIVER_DIR)/include -I$(srctree)/$(obj)/include

clean-files += $(patsubst $(obj)/%,%,$(wildcard *.o)\
	 $(wildcard .*cmd) $(wildcard *~))

clean-files += $(patsubst $(obj)/%,%,$(wildcard src/*.o)\
	 $(wildcard src/.*cmd) $(wildcard src/*~))

ifeq ($(CONFIG_MTK_ENABLE_MD1), y)
ccflags-y  += -DMTK_ENABLE_MD1
endif
ifeq ($(CONFIG_MTK_ENABLE_MD2), y)
ccflags-y  += -DMTK_ENABLE_MD2
endif
ifeq ($(CONFIG_MTK_SECURITY_SW_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/masp/asfv2/asf_export_inc
ccflags-y  += -DENABLE_MD_IMG_SECURITY_FEATURE
endif
ifeq ($(CONFIG_MTK_SEC_MODEM_NVRAM_ANTI_CLONE), y)
ccflags-y  += -DMTK_SEC_MODEM_NVRAM_ANTI_CLONE
endif

else
obj-  := dummy.o
endif

#obj-  := dummy.o
#my-dir := $(dir $(lastword $(MAKEFILE_LIST)))
#
##CCCI_PLATFORM_DRIVER_FILES := $(filter-out %.h,$(wildcard $(CCCI_PLATFORM_DRIVER_DIR)/*.c)\
##			$(wildcard $(CCCI_PLATFORM_DRIVER_DIR)/prebuilt/lib*.o))
#
#my-files := $(patsubst $(obj)/%,%,$(wildcard $(my-dir)/*.c)  $(wildcard $(my-dir)/prebuilt/*.o))
#ifeq ($(CONFIG_MTK_EMMC_SUPPORT),y)
#	my-libs := $(wildcard $(my-dir)prebuilt/libasp_EMMC.a)
#else
#	my-libs := $(wildcard $(my-dir)prebuilt/libasp_NAND.a)
#endif
##my-libs :=
##$(info  lib===$(my-libs))
#ifneq ($(my-libs),)
#
#ldflags-y += -L$(my-dir) --whole-archive $(my-libs)  --no-whole-archive
##--start-group --end-group
#endif
#ccflags-y  +=-I$(my-dir)/include
#ccci-y +=$(my-files:.c=.o)
#
#clean-files += $(patsubst $(obj)/%,%,$(wildcard $(my-dir)/*.o)\
#	 $(wildcard $(my-dir)/.*cmd) $(wildcard $(my-dir)/*~))
