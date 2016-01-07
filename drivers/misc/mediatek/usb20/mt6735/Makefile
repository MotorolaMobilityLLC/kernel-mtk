#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
ccflags-$(CONFIG_USB_MTK_HDRC) += -I$(srctree)/drivers/misc/mediatek/usb20

#QMU Flags
ccflags-$(CONFIG_MTK_MUSB_QMU_SUPPORT) += -DMUSB_QMU_SUPPORT
##ccflags-$(CONFIG_MTK_MUSB_QMU_SUPPORT) += -DQMU_TASKLET

# for battery related
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat

# for DVFS control usage
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)

obj-$(CONFIG_USB_MTK_HDRC)	:= usb20.o usb20_phy.o
obj-$(CONFIG_USB_MTK_HDRC)  += usb20_host.o
obj-$(CONFIG_USBIF_COMPLIANCE)   += usb20_otg_if.o
obj-$(CONFIG_USB_MTK_HDRC)	+= usb20_phy_debugfs.o
