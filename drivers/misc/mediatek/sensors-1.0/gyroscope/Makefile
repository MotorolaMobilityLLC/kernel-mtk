# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/include
obj-y   +=  gyroscope.o gyro_factory.o


obj-$(CONFIG_MTK_GYROHUB)      +=  gyrohub/
obj-$(CONFIG_MTK_BMI160_SPI)   +=  bmi160gy-spi/
obj-$(CONFIG_MTK_ITG1010)   +=  ITG1010/
obj-$(CONFIG_MTK_MPU6515G)	+= mpu6515/
obj-$(CONFIG_MTK_MPU6050GY)   +=  mpu6050gy/
