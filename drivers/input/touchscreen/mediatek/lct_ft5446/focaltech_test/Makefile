#drivers/input/touchscreen/mediatek/focaltech_ts/focaltech_test
#
#Makefile for test module
#
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
	
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) += src/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) += supported_ic/

