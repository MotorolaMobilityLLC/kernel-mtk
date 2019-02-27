#just for mtk platform head file, kernel version less than kernle 4.0
#mtk平台，内核版本小于4.0参考头文件
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)

#just for mtk platform head file, kernel version larger than kernle 4.0
#mtk平台，内核版本大于4.0参考头文件
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include

#just for beanpod v2 compatible
#豆荚 V2 TEE平台，兼容头文件
ccflags-y += -I$(srctree)/drivers/misc/mediatek/teei/V1.0/tz_vfs

#甁砵 TEE 平台，兼容头文件
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/tkcore/core

#sunwave driver file
obj-y          := sf_ctl.o sf_hw.o

