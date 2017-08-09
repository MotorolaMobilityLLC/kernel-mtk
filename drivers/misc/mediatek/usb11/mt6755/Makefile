include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y := musbfsh_core.o
obj-$(CONFIG_MTK_USBFSH) += musbfsh_mt65xx.o musbfsh_debugfs.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/usb11/
ccflags-y += -I$(srctree)/drivers/usb/core/
