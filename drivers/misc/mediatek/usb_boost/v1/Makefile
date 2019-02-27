ccflags-y += -I$(srctree)/drivers/misc/mediatek/usb_boost
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ifeq ($(CONFIG_MTK_QOS_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/devfreq/
endif
obj-y := usb_boost_plat.o
