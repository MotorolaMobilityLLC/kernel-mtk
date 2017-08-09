include $(srctree)/drivers/misc/mediatek/imgsensor/src/Makefile.custom

CONFIG_MTK_PLATFORM_SENSOR := $(CONFIG_MTK_PLATFORM)

ifeq ($(CONFIG_ARCH_MT6735M),y)
CONFIG_MTK_PLATFORM_SENSOR := mt6735m
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
CONFIG_MTK_PLATFORM_SENSOR := mt6753
endif

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM_SENSOR))/

