#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/leds/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include

obj-$(CONFIG_MTK_LEDS) += leds_drv.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

