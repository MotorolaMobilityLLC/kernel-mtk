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

obj-$(CONFIG_MTK_AEE_MRDUMP) += mrdump.o
mrdump-y := mrdump_full.o mrdump_setup.o
obj-$(CONFIG_ARM) += mrdump_arm.o
obj-$(CONFIG_ARM64) += mrdump_arm64.o

obj-$(CONFIG_MTK_AEE_IPANIC) += mrdump_mini.o
