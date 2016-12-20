#drivers/input/touchscreen/mediatek/focaltech_ts/focaltech_test
#
# Makefile for test module
#
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)/focaltech_test/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base/

obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) += focaltech_test_ft5446.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) += focaltech_test_config_ft5446.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) += focaltech_test_supported_ic.o

