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

#
# Makefile for misc devices that really don't fit anywhere else.
#

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
$(info xuli add camera  ARCH_MTK_PROJECT: $(ARCH_MTK_PROJECT))
IMGSENSOR_DRIVER_PATH  := $(srctree)/drivers/misc/mediatek/imgsensor/src
################__AMEND_FOR_DE_INFO_PATH
TINNO_PRODUCT_DEV_INFO_PATH  := $(srctree)/tinno/common_features/dev_info/
################_AMEND_END
COMMON_VERSION := v1

subdir-ccflags-y +=  -I$(IMGSENSOR_DRIVER_PATH)/common/$(COMMON_VERSION) \
	-I$(srctree)/drivers/i2c/busses/ \
	-I$(srctree)/drivers/misc/mediatek/mmdvfs/
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_hw.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_i2c.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_legacy.o
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_proc.o
ifeq (,$(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION)))
$(info xuli sensorlist1 $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION))
obj-y				+= ../common/$(COMMON_VERSION)/imgsensor_sensor_list.o
else
$(info sensorlist2 camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION) )
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION)/imgsensor_sensor_list.o
endif

define FILTER_DRV
ifeq ($(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION)/$(1)),)
ifeq ($(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/$(1)),)

$(info xuli platfrom  $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION))
$(info $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM) )
common_drv += ../common/$(COMMON_VERSION)/$(1)/
else
platform_drv += $(1)/
endif
else
project_drv += ./camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION)/$(1)/
$(info xuli select build custom  imgsensor camera_project/$(ARCH_MTK_PROJECT)/$(COMMON_VERSION)/$(1) )
endif
endef


$(foreach drv,$(subst $\",,$(CONFIG_CUSTOM_KERNEL_IMGSENSOR)),$(eval $(call FILTER_DRV,$(drv))))

ifneq ($(common_drv),)
$(info xuli imgsensor drv by common $(common_drv))
endif
obj-y				+= $(common_drv)


ifneq ($(platform_drv),)
$(info xuli imgsensor drv by platform $(platform_drv))
endif
obj-y				+= $(platform_drv)
obj-y				+= imgsensor_clk.o
ifneq ($(project_drv),)
$(info imgsensor drv by platform $(project_drv))
endif
obj-y				+= $(project_drv)

ifeq (,$(wildcard $(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw))
subdir-ccflags-y		+= -I$(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_hw
obj-y				+= camera_hw/
ifneq ($(project_drv),)
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/
endif
else
obj-y				+= camera_project/$(ARCH_MTK_PROJECT)/camera_hw/
subdir-ccflags-y		+= -I$(IMGSENSOR_DRIVER_PATH)/$(MTK_PLATFORM)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw
endif

subdir-ccflags-y +=  -I$(TINNO_PRODUCT_DEV_INFO_PATH)

#subdir-ccflags-y +=  -DNO_CLK_METER
