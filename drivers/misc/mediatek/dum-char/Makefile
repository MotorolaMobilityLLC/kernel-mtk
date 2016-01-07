ifeq ($(CONFIG_DUM_CHAR_V2),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/nand/v2/
obj-y += v2/
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/nand/v1/
obj-y += v1/
endif