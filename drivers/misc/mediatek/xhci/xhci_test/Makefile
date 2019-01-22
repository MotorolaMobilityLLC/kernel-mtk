#
# Makefile for USB Host Controller Drivers
#


ccflags-y += -I$(srctree)/drivers/misc/mediatek/kernel/include \
             -I$(srctree)/drivers/misc/mediatek/mu3d/hal \
             -I$(srctree)/drivers/misc/mediatek/mu3phy

ccflags-$(CONFIG_USB_DEBUG) += -DDEBUG

xhci-hcd-test-$(CONFIG_MTK_TEST_XHCI) := xhci.o xhci-mem.o xhci-ring.o xhci-hub.o xhci-dbg.o
xhci-hcd-test-$(CONFIG_MTK_TEST_XHCI) += mtk-protocol.o mtk-test.o mtk-test-lib.o mtk-usb-hcd.o xhci-mtk-scheduler.o xhci-mtk-power.o

obj-$(CONFIG_MTK_TEST_XHCI) += xhci-hcd-test.o
