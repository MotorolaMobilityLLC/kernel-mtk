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


obj-$(CONFIG_ODM1_NFC_PN548) += lct_pn548/
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_ODM2_NFC_PN548) += pn548/
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
