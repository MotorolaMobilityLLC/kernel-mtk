# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
obj-y   +=  alsps.o
obj-y   +=  alsps_factory.o
obj-y   +=  aal_control.o

ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
obj-$(CONFIG_MTK_ALSPSHUB)      +=  alspshub/
else
obj-$(CONFIG_MTK_CM36558)	+=  cm36558/
obj-$(CONFIG_MTK_CM36652_NEW)	+=  cm36652-new/
endif
