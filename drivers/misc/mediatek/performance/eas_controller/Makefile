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

MTK_TOP = $(srctree)/drivers/misc/mediatek/
ccflags-y += \
	-I$(MTK_TOP)/base/power/include/ \
	-I$(MTK_TOP)/base/power/$(MTK_PLATFORM)/ \
	-I$(MTK_TOP)/dramc/$(MTK_PLATFORM)/ \
	-I$(srctree)/drivers/misc/mediatek/performance/eas_controller/include/ \

ifeq ($(CONFIG_MTK_FPSGO), y)
        ifneq ($(CONFIG_MTK_CM_MGR), y)
                ifeq ($(CONFIG_MACH_MT6739), y)
                        obj-y += src/platform/mt6739/eas_ctrl.o
                        ccflags-y += -DCPI_THRES=250
                endif

                ifeq ($(CONFIG_MACH_MT6763), y)
                        obj-y += src/platform/mt6763/eas_ctrl.o
                        ccflags-y += -DCPI_THRES=250
                endif

                ifeq ($(CONFIG_MACH_MT6758), y)
                        obj-y += src/platform/mt6758/eas_ctrl.o
                        ccflags-y += -DCPI_THRES=250
                endif

                ifeq ($(CONFIG_MACH_MT6771), y)
                        obj-y += src/platform/mt6771/eas_ctrl.o
                        ccflags-y += -DCPI_THRES=250
                endif

                ifeq ($(CONFIG_MACH_MT6775), y)
                        obj-y += src/platform/mt6775/eas_ctrl.o
                        ccflags-y += -DCPI_THRES=250
                endif
        endif
endif

ifeq ($(CONFIG_MACH_MT6771), y)
      obj-y += src/platform/mt6771/eas_ctrl_extend.o
endif

ifeq ($(CONFIG_MACH_MT6775), y)
      obj-y += src/platform/mt6771/eas_ctrl_extend.o
endif

obj-y += src/eas_controller.o
