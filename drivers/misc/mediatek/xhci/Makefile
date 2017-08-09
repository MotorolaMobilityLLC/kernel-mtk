#
# Makefile for USB Host Lib
#

ccflags-y += -I$(srctree)/drivers/usb/host
ccflags-y += -I$(srctree)/drivers/usb/core
ccflags-y += -I$(srctree)/drivers/misc/mediatek/xhci
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3phy
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3phy/$(CONFIG_MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3d/drv
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3d/hal
ccflags-y += -I$(srctree)/drivers/usb_c/inc

ifeq ($(CONFIG_USB_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_USB_XHCI_MTK) += xhci-mtk-power.o xhci-mtk-driver.o xhci-mtk-attrs.o
