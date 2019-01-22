# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
obj-y   +=  rgbw.o
#obj-y   +=  rgbw_factory.o

ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
else
obj-$(CONFIG_MTK_CM36652_NEW_RGBW)	+=  cm36652-new/
endif
