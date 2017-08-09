MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/uart/$(MTK_PLATFORM)

obj-$(CONFIG_MTK_SERIAL) := uart.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
