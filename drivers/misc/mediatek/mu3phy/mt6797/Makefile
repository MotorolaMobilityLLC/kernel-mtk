ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3d/drv \
             -I$(srctree)/drivers/misc/mediatek/mu3d/hal \
             -I$(srctree)/drivers/misc/mediatek/mu3phy

ifeq ($(CONFIG_USB_DEBUG),y)
	EXTRA_CFLAGS += -DDEBUG
endif

#For USB HQA Driving Tuning Mode 1 Settings
#EXTRA_CFLAGS += -DMTK_USB_MODE1

obj-$(CONFIG_PROJECT_PHY) += mtk-phy-asic.o
