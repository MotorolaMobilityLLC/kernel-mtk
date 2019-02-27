subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/

obj-$(CONFIG_MTK_FREQ_HOPPING) := mt_freqhopping_drv.o

ifeq ($(CONFIG_MTK2712_FREQ_HOPPING),y)
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/mt2712/
obj-y += mt2712/
endif

ifeq ($(CONFIG_MTK8173_FREQ_HOPPING),y)
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/freqhopping/mt8173/
obj-y += mt8173/
endif
