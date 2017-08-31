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

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/nand/v1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/nand/v1/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/arch/arm/include/asm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
obj-$(CONFIG_MTK_MTD_NAND) += mtk_nand.o nand_device_list.o toshiba_device_feature.o hynix_device_feature.o
obj-$(CONFIG_MTK_MTD_NAND) += sandisk_device_feature.o micron_device_feature.o mtk_nand_device_feature.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o mtk_nand_fs.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o
obj-$(CONFIG_MNTL_SUPPORT) += mtk_nand_ops.o mtk_nand_ops_test.o
