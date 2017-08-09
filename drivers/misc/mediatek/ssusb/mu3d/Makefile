#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/usb/host
ccflags-y += -I$(src)/../include/

ifeq ($(CONFIG_USB_DEBUG),y)
	ccflags-y	+= -DDEBUG
endif

ccflags-y += -DUSE_SSUSB_QMU
#EXTRA_CFLAGS += -DCONFIG_MTK_FPGA
#obj-y := musb_hdrc.o
obj-y := musb_hdrc.o
musb_hdrc-y := musb_core.o
musb_hdrc-y += musb_gadget_ep0.o musb_gadget.o
musb_hdrc-y += musb_init.o mt_usb.o
musb_hdrc-$(CONFIG_SYSFS) += ssusb_sysfs.o
musb_hdrc-y += mu3d_hal_qmu_drv.o mu3d_hal_usb_drv.o

musb_hdrc-y	+= xhci-mtk.o
