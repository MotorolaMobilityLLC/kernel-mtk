MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat


obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_meter.o mt_battery_common.o mt_switch_charging.o mt_bm_data.o

ifeq ($(CONFIG_MTK_BQ24297_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += bq24297.o
endif

ifeq ($(CONFIG_MTK_BQ25890_SUPPORT),y)
    obj-$(CONFIG_MTK_SMART_BATTERY) += bq25890.o
endif

obj-$(CONFIG_MTK_PMIC) += mt6397_fgadc.o
