PROJECT := $(subst full_,,$(TARGET_PRODUCT))
ccflags-y += -I$(srctree)/tinno/$(PROJECT)/touchpanel/focaltech_touch_screen/
#$(warning   -----------------------TARGET_PRODUCT:  $(TARGET_PRODUCT)-----------------------------)
#$(warning   -----------------------PROJECT:  $(PROJECT)-----------------------------)
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/alsps/inc

obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_ex_fun.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_flash.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_gesture.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_core.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_esdcheck.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_ex_mode.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_proximity.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_i2c.o
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)    +=  focaltech_point_report_check.o

obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS) +=     focaltech_test/
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS) +=     focaltech_flash/
