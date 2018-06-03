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

BASE_TOP = $(srctree)/drivers/misc/mediatek/performance/fpsgo_v2/base
MTK_TOP = $(srctree)/drivers/misc/mediatek/

obj-$(CONFIG_MTK_FPSGO) += \
	src/fpsgo_base.o \

ccflags-y += \
	-I$(srctree)/include/ \
	-I$(MTK_TOP)/include/ \
	-I$(BASE_TOP)/include/ \
