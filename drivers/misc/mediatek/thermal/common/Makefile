THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/

ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6735"))
ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D1
endif

ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D2
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D3
endif
endif

obj-  := dummy.o
ifeq ($(CONFIG_THERMAL), y)
else
    obj-y += mtk_thermal_dummy.o
endif

obj-$(CONFIG_THERMAL) += mtk_thermal_platform.o
obj-$(CONFIG_THERMAL) += ap_thermal_limit.o


obj-y += thermal_zones/
obj-y += coolers/
