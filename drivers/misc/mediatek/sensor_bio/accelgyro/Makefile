# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensor_bio/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensor_bio/include

obj-y   +=  accelgyro.o accelgyro_factory.o

obj-$(CONFIG_MTK_BMI160_SPI)   +=  bmi160-spi/
