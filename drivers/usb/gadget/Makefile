#
# USB peripheral controller drivers
#
subdir-ccflags-$(CONFIG_USB_GADGET_DEBUG)	:= -DDEBUG
subdir-ccflags-$(CONFIG_USB_GADGET_VERBOSE)	+= -DVERBOSE_DEBUG
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/usb_boost
ifdef CONFIG_MTPROF
ccflags-y += -Idrivers/misc/mediatek/mtprof/
endif
ifeq ($(CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT),y)
subdir-ccflags-y += -I$(srctree)/drivers/usb/gadget/function
endif

ccflags-y	+= -I$(srctree)/drivers/usb/gadget/udc -I$(srctree)/drivers/usb/gadget/function
ccflags-y += -I$(srctree)/drivers/misc/mediatek/c2k_usb/
ccflags-y += -I$(srctree)/drivers/usb/gadget/

ifeq ($(CONFIG_MTK_MD_DIRECT_TETHERING_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pkt_track/mod-pkt_track/include \
             -I$(srctree)/drivers/misc/mediatek/eccci \
             -I$(srctree)/drivers/misc/mediatek/eccci/$(MTK_PLATFORM)
endif


obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o configfs.o u_f.o

g_android-y			:= android.o

obj-$(CONFIG_USB_GADGET)	+= udc/ function/ legacy/

obj-$(CONFIG_USB_G_ANDROID)	+= g_android.o

