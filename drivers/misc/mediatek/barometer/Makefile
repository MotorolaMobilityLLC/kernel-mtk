# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y   +=  barometer.o barometer_factory.o

obj-$(CONFIG_MTK_BMP180) += BMP180/
obj-$(CONFIG_MTK_BMP180_NEW) += BMP180-new/
