
ifeq ($(CONFIG_MTK_MULTIBRIDGE_SUPPORT),y)

ccflags-y += -I$(srctree)/drivers/misc/mediatek/multibridge/$(CONFIG_MTK_PLATFORM)/mt8193/inc

obj-y   +=  mt8193_ckgen.o
obj-y   +=  mt8193_ckgen_vfy.o
obj-y   +=  mt8193_gpio.o
obj-y   +=  mt8193_iic.o
obj-y   +=  mt8193_pinmux.o
endif