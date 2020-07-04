ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)

ccflags-y += -I$(srctree)/drivers/misc/mediatek/teei/300/tz_driver/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/teei/300/common/include/

obj-$(CONFIG_MICROTRUST_FP_FPSENSOR)	+= fpsensor_spi_tee.o
