
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

ccflags-y += \
	-I$(srctree)/drivers/misc/mediatek/base/power/include/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v3/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)\
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/	\
	-I$(srctree)/drivers/misc/mediatek/pmic/	\
	-I$(srctree)/drivers/misc/mediatek/pmic/include/mt6335	\
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/eem_v1/inc	\
	-I$(srctree)/include/linux/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/ppm_v2/src/mach/$(MTK_PLATFORM)/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/ppm_v2/inc/	\

ifeq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/$(CONFIG_MTK_PLATFORM)
endif

obj-y += mtk_eem.o
obj-y += mtk_eem_platform.o
obj-y += mtk_eem_internal.o

