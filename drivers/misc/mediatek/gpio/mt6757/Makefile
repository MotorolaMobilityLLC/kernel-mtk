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

ifeq ($(CONFIG_MTK_FPGA),y)
obj-y	:= mt_gpio_fpga.o
else
obj-y	:= mt_gpio_base.o
endif
obj-y	+= mt_gpio_affix.o mt_gpio_debug.o mt_gpio_ext.o
ifeq ($(CONFIG_GPIOLIB),y)
obj-y	+= mt_gpio_base_linux.o
endif
