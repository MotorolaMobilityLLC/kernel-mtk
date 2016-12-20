# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
obj-y   +=  accel.o accel_factory.o

ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
obj-$(CONFIG_MTK_ACCELHUB)		+=	accelhub/
else
obj-$(CONFIG_MTK_LSM6DS3HG)   +=  lsm6ds3hg/
endif
