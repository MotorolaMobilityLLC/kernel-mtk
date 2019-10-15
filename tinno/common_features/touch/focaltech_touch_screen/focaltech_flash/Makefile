#drivers/input/touchscreen/mediatek/focaltech_ts/focaltech_flash
#
#Makefile for test module
#
   
PROJECT := $(subst full_,,$(TARGET_PRODUCT))
ccflags-y += -I$(srctree)/tinno/$(PROJECT)/touchpanel/focaltech_touch_screen/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base

obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS) += focaltech_upgrade_ft5446.o