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
subdir-ccflags-y += -Werror
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
obj-$(CONFIG_MTK_ICE_DEBUG) +=ice_debug/
ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
obj-y	+= hwmon/
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS) += alsps/
endif
obj-$(CONFIG_MTK_FREQ_HOPPING) += freqhopping/
obj-$(CONFIG_MMC_MTK_SDIO) += mtk_sdio/
obj-$(CONFIG_MTK_COMBO) += connectivity/
obj-$(CONFIG_MTK_UNIFIED_DEBUG_INTERFACE) += udi/
obj-$(CONFIG_MTK_DEVAPC_DRIVER) += devapc/
