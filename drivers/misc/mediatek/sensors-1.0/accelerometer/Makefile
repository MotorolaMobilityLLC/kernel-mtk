# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/include

obj-y   +=  accel.o accel_factory.o

obj-$(CONFIG_MTK_ACCELHUB)		+=	accelhub/
obj-$(CONFIG_MTK_BMI160_SPI)   +=  bmi160-spi/
obj-$(CONFIG_MTK_MC3410_I2C) += mc3410-i2c/
obj-$(CONFIG_MTK_MPU6515A)		+= mpu6515/
obj-$(CONFIG_MTK_BMA222E)		+= bma222e/
obj-$(CONFIG_MTK_MPU6050G)   +=  mpu6050g/
