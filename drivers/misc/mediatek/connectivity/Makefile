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

# Connectivity combo driver
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
subdir-ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
ifeq ($(CONFIG_ARM64), y)
subdir-ccflags-y += -D CONFIG_MTK_WCN_ARM64
endif

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=1
else
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=0
endif
    subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
	subdir-ccflags-y += -D CONFIG_OF

    obj-y += common/
    obj-$(CONFIG_MTK_COMBO_BT) += bt/
    obj-$(CONFIG_MTK_COMBO_WIFI) += wlan/
    obj-$(CONFIG_MTK_COMBO_GPS) += gps/

    # MINI PLATFORM VERSION = Android O
    MINI_PLATFORM_VER = O

    ifndef TARGET_PLATFORM_VERSION
        $(warning TARGET_PLATFORM_VERSION is not set, define it as A)
        TARGET_PLATFORM_VERSION = A
    endif

    # compare $(PLATFORM_VERSION)>=$(MINI_PLATFORM_VER)
    ifeq ($(MINI_PLATFORM_VER), $(word 1, $(sort $(MINI_PLATFORM_VER) $(TARGET_PLATFORM_VERSION))))
        # Do Nothing, move to standalone repo
        $(warning Android($(TARGET_PLATFORM_VERSION) >= $(MINI_PLATFORM_VER)) ignore build-in)
    else
        $(warning Android($(TARGET_PLATFORM_VERSION) < $(MINI_PLATFORM_VER)) do build-in)
        obj-$(CONFIG_MTK_FMRADIO) += fmradio/
    endif

# Otherwise we were called directly from the command line;
# invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
