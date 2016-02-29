#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(strip $(CONFIG_ARCH_MT6735)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D1.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D2.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

ifeq ($(strip $(CONFIG_ARCH_MT6753)),y)
vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver_D3.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o
endif

obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += vcodec_kernel_driver.o


ccflags-y += \
    -I$(srctree)/drivers/misc/mediatek/videocodec/$(MTK_PLATFORM)/common/drv/inc		 \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include   \
    -I$(srctree)/drivers/misc/mediatek/videocodec/include/kernel   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat   \
    -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach   \
    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
    -I$(srctree)/drivers/misc/mediatek/smi

