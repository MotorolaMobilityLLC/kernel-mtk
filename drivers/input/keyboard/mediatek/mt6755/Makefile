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

subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/

ifeq ($(CONFIG_MT_SND_SOC_6755),y)
	EXTRA_CFLAGS += -I$(srctree)/sound/soc/mediatek/common_define
	EXTRA_CFLAGS += -I$(srctree)/sound/soc/mediatek/mt6755
endif

obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o
