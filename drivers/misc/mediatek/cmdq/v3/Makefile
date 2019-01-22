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

# common driver code for CMDQ

ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/ \
             -I$(srctree)/drivers/misc/mediatek/mmp/ \
             -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include

MDP_PLATFORM_DIR=$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)
platformdir_exists=$(shell if [ -d ${MDP_PLATFORM_DIR} ]; then echo "y"; else echo "n"; fi;)
ifeq ($(platformdir_exists), y)
ccflags-y += -I$(MDP_PLATFORM_DIR)/
else
ccflags-y += -DCMDQ_COMMON_ENG_SUPPORT
endif

# Enable AEE
ccflags-y += -DCMDQ_AEE_READY

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

ifneq ($(TARGET_BUILD_VARIANT),eng)
ccflags-y += -D_MTK_USER_
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -D_CMDQ_TEST_PROC_
ccflags-y += -DCMDQ_PROFILE_LOCK
endif

# secure path
ifeq ($(strip $(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)),y)
ifeq ($(strip $(CONFIG_TRUSTONIC_TEE_SUPPORT)),y)
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/inc/cmdq_sec_dr \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v3/inc/cmdq_sec_tl

ccflags-y += -DCMDQ_SECURE_PATH_SUPPORT
ccflags-y += -DCMDQ_SECURE_PATH_NORMAL_IRQ
ccflags-y += -DCMDQ_EVENT_SVP_BACKUP
endif
endif

# platform dependent definition
ifeq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6758"))
ccflags-$(CONFIG_MTK_SMI_EXT) += -DCMDQ_CONFIG_SMI
endif
ccflags-y += -DCMDQ_SPECIAL_ESD_PRIORITY
ccflags-y += -D_CMDQ_DISABLE_MARKER_
ccflags-y += -DCMDQ_PROFILE_MMP

ifneq ($(strip $(CONFIG_MTK_GMO_RAM_OPTIMIZE)),y)
ccflags-y += -DCMDQ_LARGE_MAX_FIRSTERROR_BUFFER
endif

# driver module
obj-y += cmdq_record.o
obj-y += cmdq_core.o
obj-y += cmdq_virtual.o
obj-y += cmdq_device.o
obj-y += cmdq_driver.o
obj-y += cmdq_mmp.o
obj-y += cmdq_mdp_common.o
obj-y += cmdq_prof.o
obj-y += cmdq_test.o
obj-y += cmdq_sec.o

ifeq ($(platformdir_exists), y)
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
