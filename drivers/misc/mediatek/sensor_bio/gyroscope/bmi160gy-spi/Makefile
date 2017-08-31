ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensor_bio/gyroscope/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensor_bio/hwmon/include
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)

#inlcude SPI Interface Start
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
	subdir-ccflags-y += -I$(srctree)/drivers/spi/trustzone/spi/Tlspi/inc
	include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
endif
#include SPI Interface End

obj-y	:=  bmi160_gyro.o

