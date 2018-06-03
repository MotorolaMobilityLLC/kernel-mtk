ccflags-y += -I$(srctree)/drivers/misc/mediatek/pkt_track/mod-fastpath/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pkt_track/mod-pkt_track/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/usb/gadget/
ccflags-y += -I$(srctree)/drivers/usb/gadget/function/

#include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y += fastpath.o

fastpath-y += src/fastpath.o src/dev.o
