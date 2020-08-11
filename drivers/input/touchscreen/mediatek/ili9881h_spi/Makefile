BUILD_INFAE := spi
BUILD_PLATFORM := mtk
BUILD_MODULE := n

ccflags-y += -Wall

ifeq ($(BUILD_PLATFORM),mtk)
platform=ilitek_plat_mtk
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/oreo/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/oreo/firmware/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
endif

ifeq ($(BUILD_PLATFORM),qcom)
platform=ilitek_plat_qcom
ccflags-y += -I$(srctree)/drivers/input/touchscreen/oreo/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/oreo/firmware/
endif

ifeq ($(BUILD_INFAE),i2c)
interface=ilitek_i2c
fwupdate=ilitek_flash
endif

ifeq ($(BUILD_INFAE),spi)
interface=ilitek_spi
fwupdate=ilitek_hostdl
endif

ifeq ($(BUILD_MODULE),n)
obj-y += ilitek_main.o \
	$(interface).o \
	$(platform).o \
	ilitek_ic.o \
	ilitek_touch.o \
	ilitek_mp.o \
	$(fwupdate).o \
	ilitek_node.o
else
	obj-m += ilitek.o
	ilitek-y := ilitek_main.o \
		$(interface).o \
		$(platform).o \
		ilitek_ic.o \
		ilitek_touch.o \
		ilitek_mp.o \
		$(fwupdate).o \
		ilitek_node.o

KERNEL_DIR= /home/likewise-open/ILI/1061279/workplace/rk3288_sdk/kernel
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
endif
