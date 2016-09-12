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

ccflags-y += -I$(srctree)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/ccci_util/$(MTK_PLATFORM)
ifeq ($(CONFIG_MTK_PMIC_CHIP_MT6353), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/
endif
# always build-in parts
obj-$(CONFIG_MTK_CCCI_DEVICES) += ccci_off.o
