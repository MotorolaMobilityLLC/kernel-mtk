#
# Makefile for the kernel flashlight device drivers.
#
IMGSENSOR_FLASHLIGHT_PATH  := $(srctree)/drivers/misc/mediatek/flashlight

flashlight-y := flashlight-core.o
#flashlight-y += flashlight-device.o

obj-$(CONFIG_MTK_FLASHLIGHT)             += flashlight.o
obj-$(CONFIG_MTK_V4L2_FLASHLIGHT)        += mtk-composite.o
obj-$(CONFIG_MTK_FLASHLIGHT_LED191)      += flashlights-led191.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3642)      += flashlights-lm3642.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3643)      += flashlights-lm3643.o
obj-$(CONFIG_MTK_FLASHLIGHT_LM3644)      += flashlights-lm3644.o
obj-$(CONFIG_MTK_FLASHLIGHT_RT4505)      += flashlights-rt4505.o
obj-$(CONFIG_MTK_FLASHLIGHT_RT5081)      += flashlights-rt5081.o
obj-$(CONFIG_MTK_FLASHLIGHT_MT6336)      += flashlights-mt6336.o
obj-$(CONFIG_MTK_FLASHLIGHT_MT6370)      += flashlights-mt6370.o
ifeq ($(CONFIG_MTK_FLASHLIGHT_LISBON), y)
  obj-y  += mot-lisbon-flashlights-mt6360.o
else
  obj-$(CONFIG_MTK_FLASHLIGHT_MT6360)      += flashlights-mt6360.o
endif
obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY)       += flashlights-dummy.o
#obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY_GPIO)  += flashlights-dummy-gpio.o

ccflags-$(CONFIG_MTK_FLASHLIGHT_DEBUG)     := -DDEBUG
ccflags-$(CONFIG_MTK_FLASHLIGHT_LED191)    += -DCONFIG_MTK_FLASHLIGHT_LED191
ccflags-$(CONFIG_MTK_FLASHLIGHT_PT)        += -DCONFIG_MTK_FLASHLIGHT_PT
ccflags-$(CONFIG_MTK_FLASHLIGHT_PT_STRICT) += -DCONFIG_MTK_FLASHLIGHT_PT_STRICT
ccflags-$(CONFIG_MTK_FLASHLIGHT_DLPT)      += -DCONFIG_MTK_FLASHLIGHT_DLPT

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ifeq ($(CONFIG_ARCH_MOTO_PROJECT),)
MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
else
MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MOTO_PROJECT))
endif

MTK_CUSTOM_PATH := $(srctree)/drivers/misc/mediatek

ccflags-y += -D$(MTK_PLATFORM) -D$(MTK_PROJECT)

subdir-ccflags-${CONFIG_MACH_MT6833} += \
	-I$(MTK_CUSTOM_PATH)/include/mt-plat/mt6833/include/mach

subdir-ccflags-y += \
	-I$(MTK_CUSTOM_PATH)/include/mt-plat \
	-I$(MTK_CUSTOM_PATH)/include/mt-plat/$(MTK_PLATFORM)/include/mach \
	-I$(MTK_CUSTOM_PATH)/pmic/include \
	-I$(MTK_CUSTOM_PATH)/pmic

define CUSTOM_FLASHLIGHT_DRV
ifeq ($(wildcard $(IMGSENSOR_FLASHLIGHT_PATH)/camera_project/$(MTK_PROJECT)/$(1).c),)
flashlight-y += flashlight-device.o
obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY_GPIO)  += flashlights-dummy-gpio.o
else
flashlight-y += ./camera_project/$(MTK_PROJECT)/flashlight-device.o
obj-$(CONFIG_MTK_FLASHLIGHT_DUMMY_GPIO)  += ./camera_project/$(MTK_PROJECT)/flashlights-dummy-gpio.o
endif
endef

CUSTOM_FLASHLIGHT_SRC_LIST := flashlight-device flashlights-dummy-gpio
$(foreach drv,$(subst $\",,$(CUSTOM_FLASHLIGHT_SRC_LIST)),$(eval $(call CUSTOM_FLASHLIGHT_DRV,$(drv))))
MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
