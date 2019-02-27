#
# Copyright (C) 2017 MediaTek Inc.
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

#$(call all-subdir-src-or-makefile)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
subdir-ccflags-y += -I$(srctree)/drivers/mmc/host/mediatek/$(MTK_PLATFORM)
obj-$(CONFIG_MTK_ICE_DEBUG) +=ice_debug/
obj-$(CONFIG_MTK_BASE_POWER) += base/
ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
obj-y	+= hwmon/
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS) += alsps/
endif

obj-$(CONFIG_MTK_BASE_POWER) += base/
obj-$(CONFIG_MTK_FREQ_HOPPING) += freqhopping/
obj-$(CONFIG_MMC_MTK_SDIO) += mtk_sdio/
obj-$(CONFIG_MTK_COMBO) += connectivity/
obj-$(CONFIG_MTK_UNIFIED_DEBUG_INTERFACE) += udi/
obj-$(CONFIG_MTK_DEVAPC_DRIVER) += devapc/
obj-$(CONFIG_MTK_SERIAL)	+= uart/
obj-$(CONFIG_MTK_GPT_SCHEME_SUPPORT)	+= partition/
obj-$(CONFIG_MTK_SYSENV)	+= sysenv/
obj-$(CONFIG_MTK_MT6306_GPIO_SUPPORT) += gpio/
obj-$(CONFIG_MTK_GIC_EXT) += ext_gic/
obj-$(CONFIG_MTK_PMIC_WRAP_HAL) += pmic_wrap/
obj-$(CONFIG_MTK_BOOT) += boot/
obj-$(CONFIG_MTK_BOOT_REASON) += boot_reason/
obj-$(CONFIG_MTK_CHIP) += chip/
obj-$(CONFIG_MTK_SELINUX_AEE_WARNING)	+= selinux_warning/
obj-y += power/
obj-$(CONFIG_MTK_PMIC_NEW_ARCH)	+= pmic/
obj-y += devinfo/
obj-$(CONFIG_MTK_LCM) += lcm/
obj-$(CONFIG_MTK_FB) += video/
obj-$(CONFIG_MTK_SYNC) += sync/
obj-$(CONFIG_MTK_SECURITY_SW_SUPPORT) +=masp/
obj-$(CONFIG_MTK_BLOCK_TAG) +=blocktag/
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gcpu/
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gud/
obj-y += sspm/
obj-$(CONFIG_MMPROFILE) += mmp/
obj-$(CONFIG_MTK_M4U)	+= m4u/
obj-$(CONFIG_MTK_SMI_EXT)	+= smi/
obj-$(CONFIG_MTK_USB_TYPEC) += typec/
obj-$(CONFIG_MTK_GPU_SUPPORT) += gpu/
obj-$(CONFIG_MTK_RTC)	+= rtc/
obj-$(CONFIG_MICROTRUST_TEE_SUPPORT) += teei/
obj-$(CONFIG_MTK_RAM_CONSOLE) += ram_console/
obj-$(CONFIG_MTK_CMDQ) += cmdq/
obj-$(CONFIG_RT_FLASHLIGHT)     += flashlight/richtek/
obj-$(CONFIG_RT_REGMAP) += rt-regmap/
obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += videocodec/
obj-$(CONFIG_USB_MU3D_DRV)	+= mu3d/
obj-$(CONFIG_MTK_SYS_CIRQ) += cirq/
