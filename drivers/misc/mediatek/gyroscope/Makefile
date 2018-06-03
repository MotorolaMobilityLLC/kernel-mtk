# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
obj-y   +=  gyroscope.o gyro_factory.o

ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
obj-$(CONFIG_MTK_GYROHUB)      +=  gyrohub/
else
obj-$(CONFIG_MTK_LSM6DS3HGY)   +=  lsm6ds3hgy/
obj-$(CONFIG_MTK_BMI160GY_SPI)   +=  bmi160gy-spi/
obj-$(CONFIG_MTK_BMI160GY_I2C)   +=  bmi160gy-i2c/
endif
