
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/alsps/inc/

obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_TS)    +=  mp_main.o
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_TS)    +=  mp_misc.o
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_TS)    +=  mp_parse.o
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_TS)    +=  mp_result.o
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX_TS)    +=  mp_test.o


