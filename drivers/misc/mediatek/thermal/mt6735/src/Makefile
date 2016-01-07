THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D1
obj-$(CONFIG_THERMAL) += mtk_tc_D1.o
endif

ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D2
obj-$(CONFIG_THERMAL) += mtk_tc_D2.o
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc/D3
obj-$(CONFIG_THERMAL) += mtk_tc_D3.o
endif

obj-$(CONFIG_THERMAL) += mtk_pmic_efuse.o
