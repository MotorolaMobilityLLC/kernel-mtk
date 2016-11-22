#tuwenzan@wind-mobi.com add at 20161110 begin
#
# Makefile for the input misc drivers.
#
ccflags-y += -I$(srctree)/drivers/input/touchscreen/sx9301/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/wdt/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/wdt/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/i2c/$(MTK_PLATFORM)/
# Each configuration option enables a list of files.

obj-$(CONFIG_INPUT_SX9310)		+= sx9310.o
#tuwenzan@wind-mobi.com add at 20161110 end
