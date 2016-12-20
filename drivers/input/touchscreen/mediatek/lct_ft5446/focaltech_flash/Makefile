#drivers/input/touchscreen/mediatek/focaltech_ts/focaltech_flash
#
#Makefile for test module
#
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base

obj-y += focaltech_upgrade_ft5446.o
obj-y += focaltech_upgrade_test.o
obj-y += focaltech_upgrade_app_bin_check.o

