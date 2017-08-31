# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensor_bio/hwmon/include
obj-y   +=  gyroscope.o gyro_factory.o


obj-$(CONFIG_MTK_GYROHUB)      +=  gyrohub/
obj-$(CONFIG_MTK_BMI160_SPI)   +=  bmi160gy-spi/
