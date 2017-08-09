#include $(srctree)/drivers/misc/mediatek/Makefile.custom
CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_IMGSENSOR) | tr a-z A-Z))
ccflags-y += $(CUSTOM_KERNEL_FLAGS)

CONFIG_MTK_PLATFORM_SENSOR := $(CONFIG_MTK_PLATFORM)

ifeq ($(CONFIG_ARCH_MT6735M),y)
CONFIG_MTK_PLATFORM_SENSOR := mt6735m
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
CONFIG_MTK_PLATFORM_SENSOR := mt6753
endif

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM_SENSOR))/

