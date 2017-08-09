#
# Makefile for misc devices that really don't fit anywhere else.

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_PLATFORM_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)
MTK_PROJECT_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/camera/camera \

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

ccflags-y += -I$(MTK_CUSTOM_PATH)/camera/$(MTK_PLATFORM) \
             -I$(MTK_CUSTOM_PATH)/i2c/$(MTK_PLATFORM) \
             -I$(MTK_CUSTOM_PATH)/imgsensor/inc \
             -I$(MTK_CUSTOM_PATH)/imgsensor/src/$(MTK_PLATFORM)/camera_hw \

define FILTER_PROJECT_DRV
ifeq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/imgsensor/$(1)),)
platform_drv += $(1)
endif
endef

obj-y				+= kd_sensorlist.o
obj-y				+= camera_hw/kd_camera_hw.o

$(foreach drv,$(subst $\",,$(CONFIG_CUSTOM_KERNEL_IMGSENSOR)),$(eval $(call FILTER_PROJECT_DRV,$(drv))))
custom_kernel_imgsensor := $(addsuffix /, $(shell echo $(platform_drv)))
obj-y += $(custom_kernel_imgsensor)
$(info imgsensor drv by platform $(platform_drv))
