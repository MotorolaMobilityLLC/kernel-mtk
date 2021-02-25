# SPDX-License-Identifier: GPL-2.0

MTK_CUSTOM_PATH := $(srctree)/drivers/misc/mediatek
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
CAM_CAL_SRC_PATH := $(MTK_CUSTOM_PATH)/cam_cal/src
CAM_CAL_PLATFORM_SRC_PATH := $(CAM_CAL_SRC_PATH)/$(MTK_PLATFORM)
ifeq ($(CONFIG_ARCH_MOTO_PROJECT),"")
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
CUSTOM_SRC_LIST := cam_cal_list
else
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MOTO_PROJECT))
ccflags-y += -D$(MTK_PLATFORM) -D$(ARCH_MTK_PROJECT)
CUSTOM_SRC_LIST := cam_cal_list custom_eeprom_driver
endif
COMMON_VERSION := v2

subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \
                    -I$(MTK_CUSTOM_PATH)/imgsensor/src/$(CONFIG_MTK_PLATFORM)/camera_hw \
                    -I$(MTK_CUSTOM_PATH)/imgsensor/src/common/v2 \
                    -I$(MTK_CUSTOM_PATH)/cam_cal/inc \
                    -I$(CAM_CAL_PLATFORM_SRC_PATH) \
                    -I$(CAM_CAL_SRC_PATH)/common/$(COMMON_VERSION)
subdir-ccflags-y += -I$(srctree)/drivers/i2c/busses/

# Common code
obj-y += ../common/$(COMMON_VERSION)/eeprom_driver.o
obj-y += ../common/$(COMMON_VERSION)/eeprom_driver_moto.o
obj-y += ../common/$(COMMON_VERSION)/eeprom_i2c_common_driver.o
obj-y += ../common/$(COMMON_VERSION)/eeprom_utils.o

# Platform code
obj-y += eeprom_i2c_custom_driver.o

CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_IMGSENSOR) | tr a-z A-Z))
subdir-ccflags-y += $(CUSTOM_KERNEL_FLAGS)

define INC_CUSTOM_SRC
ifeq ($(wildcard $(CAM_CAL_PLATFORM_SRC_PATH)/camera_project/$(ARCH_MTK_PROJECT)/$(1).c),)
obj-y += $(1).o
else
obj-y += camera_project/$(ARCH_MTK_PROJECT)/$(1).o
endif
endef

# Project or platform code
$(foreach src,$(CUSTOM_SRC_LIST),$(eval $(call INC_CUSTOM_SRC,$(src))))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
