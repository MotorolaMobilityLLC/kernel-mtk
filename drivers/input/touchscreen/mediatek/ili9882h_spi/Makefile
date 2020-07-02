BUILD_INFAE := spi
BUILD_PLATFORM := mtk
BUILD_MODULE := n

ccflags-y += -Wall

ifeq ($(BUILD_PLATFORM),mtk)
platform=ili9882_mtk
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/oreo_9881n/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/oreo_9881n/firmware/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
endif

ifeq ($(BUILD_PLATFORM),qcom)
platform=ili9882_qcom
ccflags-y += -I$(srctree)/drivers/input/touchscreen/oreo_9881n/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/oreo_9881n/firmware/
endif

ifeq ($(BUILD_INFAE),i2c)
interface=ili9882_i2c
fwupdate=ili9882_flash
endif

ifeq ($(BUILD_INFAE),spi)
interface=ili9882_spi
fwupdate=ili9882_hostdl
endif

ifeq ($(BUILD_MODULE),n)
obj-y += ili9882.o \
	$(interface).o \
	$(platform).o \
	ili9882_ic.o \
	ili9882_touch.o \
	ili9882_mp.o \
	$(fwupdate).o \
	ili9882_node.o
else
	obj-m += ilitek.o
	ilitek-y := ili9882.o \
		$(interface).o \
		$(platform).o \
		ili9882_ic.o \
		ili9882_touch.o \
		ili9882_mp.o \
		$(fwupdate).o \
		ili9882_node.o

KERNEL_DIR= /home/likewise-open/ILI/1061279/workplace/rk3288_sdk/kernel
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
endif
