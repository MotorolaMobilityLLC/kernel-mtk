
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ft8716/include/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ft8716/include/$(CONFIG_FT5X0X_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base

obj-y	+=  focaltech_core.o
obj-y	+=  focaltech_ctl.o
obj-y	+=  focaltech_ex_fun.o
obj-y	+=  focaltech_flash.o
obj-y	+=  focaltech_Gesture.o ./ft_gesture_lib.a

