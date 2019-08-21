# include header files for MTK
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ipio_touch_driver/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

ccflags-y += -Wall

obj-y += config.o \
		 i2c.o \
		 firmware.o \
		 finger_report.o \
		 mp_test.o \
		 flash.o \
		 protocol.o \
		 parser.o \
		 gesture.o \
		 spi.o
