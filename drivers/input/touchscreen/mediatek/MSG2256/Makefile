#include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder


ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include/
obj-y	+= mstar_drv_common.o
obj-y	+= mstar_drv_ic_fw_porting_layer.o
obj-y	+= mstar_drv_main.o
obj-y	+= mstar_drv_platform_interface.o
obj-y	+= mstar_drv_platform_porting_layer.o
obj-y	+= mstar_drv_fw_control.o
obj-y	+= mstar_drv_mp_test.o
obj-y	+= mstar_drv_mtk.o
obj-y	+= mstar_drv_utility_adaption.o
obj-y	+=  mstar_drv_jni_interface.o
