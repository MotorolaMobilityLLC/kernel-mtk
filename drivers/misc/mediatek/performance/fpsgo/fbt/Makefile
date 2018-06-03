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
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

FBT_TOP = $(srctree)/drivers/misc/mediatek/performance/fpsgo/fbt
MTK_TOP = $(srctree)/drivers/misc/mediatek/

ccflags-y += -DNR_FREQ_CPU=16

obj-$(CONFIG_MACH_MT6739) += platform/mt6739/fbt_cpu_platform.o
obj-$(CONFIG_MACH_MT6758) += platform/mt6758/fbt_cpu_platform.o
obj-$(CONFIG_MACH_MT6763) += platform/mt6763/fbt_cpu_platform.o
obj-$(CONFIG_MACH_MT6771) += platform/mt6771/fbt_cpu_platform.o

obj-$(CONFIG_MTK_FPSGO_FBT_GAME) += \
	src/fbt_main.o \
	src/fbt_base.o \
	src/fbt_notifier.o \
	src/fbt_cpu.o \
	src/xgf.o

ccflags-y += \
	-I$(srctree)/include/ \
	-I$(MTK_TOP)/include/ \
	-I$(FBT_TOP)/include/ \
	-I$(MTK_TOP)/base/power/include/ \
	-I$(MTK_TOP)/base/power/$(MTK_PLATFORM)/ \
