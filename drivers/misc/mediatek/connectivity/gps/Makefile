# drivers/barcelona/gps/Makefile
#
# Makefile for the Barcelona GPS driver.
#
# Copyright (C) 2004,2005 TomTom BV <http://www.tomtom.com/>
# Author: Dimitry Andric <dimitry.andric@tomtom.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
ifneq ($(filter "MT6620E3",$(CONFIG_MTK_COMBO_CHIP)),)
        ccflags-y += -DSOC_CO_CLOCK_FLAG=0
        ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=0
        ccflags-y += -DREMOVE_MK_NODE=1
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/combo/include \
	-I$(src)/../common/combo/linux/include
endif
ifneq ($(filter "MT6628",$(CONFIG_MTK_COMBO_CHIP)),)
        ccflags-y += -DSOC_CO_CLOCK_FLAG=0
        ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=0
        ccflags-y += -DREMOVE_MK_NODE=1
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/combo/include \
	-I$(src)/../common/combo/linux/include
endif
ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
        ccflags-y += -DSOC_CO_CLOCK_FLAG=0
        ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=0
        ccflags-y += -DREMOVE_MK_NODE=1
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/combo/include \
	-I$(src)/../common/combo/linux/include
endif

ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
        ccflags-y += -DSOC_CO_CLOCK_FLAG=1
        ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=1
        ccflags-y += -DREMOVE_MK_NODE=0
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/conn_soc/include \
	-I$(src)/../common/conn_soc/linux/include \
	-I$(src)/../common/conn_soc/core/include \
	-I$(src)/../common/conn_soc/$(MTK_PLATFORM)/include \
	-I$(src)/../common/conn_soc/linux/pri/include \
        -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach
endif

obj-$(CONFIG_MTK_COMBO_GPS)	+= stp_chrdev_gps.o
obj-$(CONFIG_MTK_GPS)		+= gps.o
obj-$(CONFIG_MTK_GPS_EMI)		+= gps_emi.o

# EOF
