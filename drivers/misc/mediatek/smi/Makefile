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

ifneq ($(CONFIG_MTK_SMI_VARIANT),y)
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/

BUILD_ALL_FILES := true
SUPPORT_MET_SMI := false

ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6758"))
ccflags-y += -DSMI_VIN
obj-y += smi_configuration_mt6758.o
SUPPORT_MET_SMI := false

else
ccflags-y += -DSMI_DUMMY
BUILD_ALL_FILES := false
endif

ifeq (true, $(BUILD_ALL_FILES))
obj-y += mtk-smi.o
obj-y += smi_debug.o
obj-y += smi_info_util.o
obj-y += smi_config_util.o
endif

ifeq (true, $(SUPPORT_MET_SMI))
obj-y += $(subst ",,met)/
endif

else
obj-y += $(subst ",,variant)/
endif
