ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-$(CONFIG_TOUCHSCREEN_CHIPONE) += chipone-tddi.o

chipone-tddi-objs := cts_tpd_driver.o cts_i2c_driver.o cts_core.o cts_platform.o cts_sfctrlv2.o cts_spi_flash.o cts_firmware.o cts_tool.o cts_sysfs.o cts_test.o

