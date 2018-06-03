# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include

obj-y   +=  humidity.o humidity_factory.o

obj-$(CONFIG_MTK_HMDYHUB)      +=  hmdyhub/
