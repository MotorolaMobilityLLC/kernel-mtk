ccflags-y += -Wall

ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ILITEK_V3/firmware/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-y += ilitek_v3.o \
	ilitek_v3_spi.o \
	ilitek_v3_mtk.o \
	ilitek_v3_ic.o \
	ilitek_v3_touch.o \
	ilitek_v3_mp.o \
	ilitek_v3_hostdl.o \
	ilitek_v3_node.o
