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

obj-y += hw_watchpoint_aarch32.o
obj-y += mt_dbg_aarch32.o

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_mt_dbg_aarch32.o := -DDEBUG
CFLAGS_hw_watchpoint_aarch32.o := -DDEBUG
endif
