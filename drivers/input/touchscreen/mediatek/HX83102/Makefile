# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/HX83102/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/i2c/$(MTK_PLATFORM)/

# Makefile for the Himax touchscreen drivers.

obj-$(CONFIG_TOUCHSCREEN_HIMAX_I2C)   	+= himax_platform.o himax_ic.o himax_common.o himax_debug.o
obj-$(CONFIG_TOUCHSCREEN_HIMAX_ITO_TEST)   	+= 	himax_ito_test.o
