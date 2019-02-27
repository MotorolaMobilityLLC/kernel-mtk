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
    obj-$(CONFIG_MTK_COMBO_GPS) += gps/

    # MINI PLATFORM VERSION = Android O
    MINI_PLATFORM_VER = O

    $(warning TARGET_PLATFORM_VERSION: [$(TARGET_PLATFORM_VERSION)])
    ifndef TARGET_PLATFORM_VERSION
        # check kernel_modules/connectivity folder exist ==> Android O1 above
        CONSYS_DIR = $(TOP)/vendor/mediatek/kernel_modules/connectivity
        ifeq (,$(wildcard $(CONSYS_DIR)))
            # $(CONSYS_DIR) is not existed
            $(warning TARGET_PLATFORM_VERSION is not set, define it as A)
            TARGET_PLATFORM_VERSION = A
        else
            # $(CONSYS_DIR) is existed
            $(warning TARGET_PLATFORM_VERSION is not set, define it as OMR1)
            TARGET_PLATFORM_VERSION = OMR1
        endif
    endif

    ifeq ($(TARGET_PLATFORM_VERSION),OPR1)
        # force build-in
        MINI_PLATFORM_VER = OPR2
    endif

    # gen3 still uses built-in module
    ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
        TARGET_PLATFORM_VERSION = A
    endif
    # gen3 still uses built-in module
    ifneq ($(filter "CONSYS_6797" "CONSYS_6759" "CONSYS_6758",$(CONFIG_MTK_COMBO_CHIP)),)
        TARGET_PLATFORM_VERSION = A
    endif
    # gen4 also needs built-in module
    ifneq ($(filter "MT6632",$(CONFIG_MTK_COMBO_CHIP)),)
        TARGET_PLATFORM_VERSION = A
    endif

    # compare $(PLATFORM_VERSION)>=$(MINI_PLATFORM_VER)
    ifeq ($(MINI_PLATFORM_VER), $(word 1, $(sort $(MINI_PLATFORM_VER) $(TARGET_PLATFORM_VERSION))))
        # Do Nothing, move to standalone repo
        $(warning Android($(TARGET_PLATFORM_VERSION) >= $(MINI_PLATFORM_VER)) ignore build-in)
        obj-y += common/connectivity_build_in_adapter.o
    else
        $(warning Android($(TARGET_PLATFORM_VERSION) < $(MINI_PLATFORM_VER)) do build-in)
        obj-$(CONFIG_MTK_FMRADIO) += fmradio/
        obj-$(CONFIG_MTK_COMBO_WIFI) += wlan/
    endif

# Otherwise we were called directly from the command line;
# invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
