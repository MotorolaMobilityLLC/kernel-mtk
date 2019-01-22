# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
obj-y   +=  accel.o accel_factory.o

ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
obj-$(CONFIG_MTK_ACCELHUB)		+=	accelhub/
else
obj-$(CONFIG_MTK_BMA2XX)   +=  bma2xx/
obj-$(CONFIG_MTK_MC3433)   +=  mc3433/
obj-$(CONFIG_MTK_LSM6DS3HG)   +=  lsm6ds3hg/
obj-$(CONFIG_MTK_BMI160_SPI)   +=  bmi160-spi/
obj-$(CONFIG_MTK_BMI160_I2C)   +=  bmi160-i2c/
endif
