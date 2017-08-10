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


ifneq ($(MTK_ROOT_BUILD),)
    include $(MTK_ROOT_BUILD)/Makefile
endif

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/trustzone/utils/tlutils
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
    obj-y += secwidevinemdw.o
endif
