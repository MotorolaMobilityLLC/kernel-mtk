#
# Copyright (C) 2016 MediaTek Inc.
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

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_MT6759) += 1.0/
obj-$(CONFIG_MACH_MT6799) += 1.0/
obj-$(CONFIG_MACH_MT6758) += 1.0/
obj-$(CONFIG_MACH_MT6775) += 2.0/
obj-$(CONFIG_MACH_MT6771) += 2.0/
#subdir-ccflags-y += -DMTK_VPU_EMULATOR

MTK_VPU_CORE := 1
ifneq (,$(filter $(strip $(MTK_PLATFORM)), mt6775))
ifeq ($(CONFIG_MTK_FPGA),y)
MTK_VPU_CORE := 2
else ifeq ($(CONFIG_FPGA_EARLY_PORTING),y)
MTK_VPU_CORE := 2
else
MTK_VPU_CORE := 3
endif
else ifneq (,$(filter $(strip $(MTK_PLATFORM)), mt6771))
MTK_VPU_CORE := 2
subdir-ccflags-y += -DMTK_VPU_SMI_DEBUG_ON
else ifneq (,$(filter $(strip $(MTK_PLATFORM)), mt6758 mt6799))
MTK_VPU_CORE := 1
else
endif
subdir-ccflags-y += -DMTK_VPU_CORE=$(MTK_VPU_CORE)

#ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-y += -DMTK_VPU_DVT
#endif

# FPGA environment
ifeq ($(CONFIG_MTK_FPGA),y)
subdir-ccflags-y += -DMTK_VPU_FPGA_PORTING
else ifeq ($(CONFIG_FPGA_EARLY_PORTING),y)
subdir-ccflags-y += -DMTK_VPU_FPGA_PORTING
endif
