
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/alsps/inc/

obj-y    +=  focaltech_ex_fun.o
obj-y    +=  focaltech_flash.o
obj-y    +=  focaltech_gesture.o
obj-y    +=  focaltech_core.o
obj-y    +=  focaltech_esdcheck.o
obj-y    +=  focaltech_ex_mode.o
obj-y    +=  focaltech_proximity.o
obj-y    +=  focaltech_i2c.o
obj-y    +=  focaltech_point_report_check.o

obj-y    +=  focaltech_flash/
