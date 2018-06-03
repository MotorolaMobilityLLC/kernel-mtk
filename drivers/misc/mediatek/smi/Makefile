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
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/


BUILD_ALL_FILES := true
SUPPORT_MET_SMI := false
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6757" "kiboplus"))
ccflags-y += -DSMI_OLY
ccflags-y += -DSMI_MMU_V2
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
obj-y += mmdvfs_mgr_v2.o
obj-y += smi_common.o
obj-y += smi_configuration.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6799"))
ccflags-y += -DSMI_WHI
ccflags-y += -DSMI_MMU_V2
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
obj-y += mmdvfs_mgr_v3.o
obj-y += mmdvfs_profile_adaptor.o
obj-y += mtk-smi.o
obj-y += smi_configuration.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6759"))
ccflags-y += -DSMI_ALA
ccflags-y += -DSMI_MMU_V2
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
obj-y += mmdvfs_mgr_v3.o
obj-y += mmdvfs_profile_adaptor.o
obj-y += mtk-smi.o
obj-y += smi_configuration_mt6759.o
SUPPORT_MET_SMI := true
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6763"))
ccflags-y += -DSMI_BIA
ccflags-y += -DSMI_MMU_V2
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
obj-y += mmdvfs_mgr_v3.o
obj-y += mmdvfs_profile_adaptor.o
obj-y += mtk-smi.o
obj-y += smi_configuration_mt6763.o
SUPPORT_MET_SMI := true
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6758"))
ccflags-y += -DSMI_VIN
ccflags-y += -DSMI_MMU_V2
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
obj-y += mtk-smi.o
obj-y += smi_configuration_mt6758.o
obj-y += mmdvfs_mgr_default.o
# SUPPORT_MET_SMI := true

else
ccflags-y += -DSMI_DUMMY
obj-y += mmdvfs_mgr_default.o
BUILD_ALL_FILES := false
endif

ifeq (true, $(SUPPORT_MET_SMI))
obj-y += $(subst ",,met)/
endif

ifeq (true, $(BUILD_ALL_FILES))
obj-y += smi_debug.o
obj-y += smi_info_util.o
obj-y += smi_config_util.o
obj-y += smi_internal.o
obj-y += mmdvfs_mgr_util.o
endif

else
obj-y += $(subst ",,variant)/
endif
