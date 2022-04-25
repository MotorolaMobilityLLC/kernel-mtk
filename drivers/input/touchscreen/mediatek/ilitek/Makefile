BUILD_INFAE := spi
BUILD_PLATFORM := mtk
BUILD_MODULE := n

ccflags-y += -Wall

ifneq ($(CONFIG_TOUCHSCREEN_ILITEK_MODEL), )
ccflags-y += -DTOUCHSCREEN_ILITEK_MODEL="\"$(CONFIG_TOUCHSCREEN_ILITEK_MODEL)\""
endif

ifeq ($(BUILD_PLATFORM),mtk)
platform=ilitek_v3_mtk
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/firmware/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
endif



obj-y += ilitek_v3.o \
	ilitek_v3_spi.o \
	ilitek_v3_mtk.o \
	ilitek_v3_ic.o \
	ilitek_v3_touch.o \
	ilitek_v3_mp.o \
	ilitek_v3_hostdl.o \
	ilitek_v3_node.o


KERNEL_DIR= /home/likewise-open/ILI/1061279/workplace/rk3288_sdk/kernel
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
