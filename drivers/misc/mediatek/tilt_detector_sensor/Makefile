# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y	+= tilt_detector.o
ifeq ($(CONFIG_MTK_SCP_SENSORHUB_V1),y)
obj-$(CONFIG_MTK_TILTDETECTHUB)		+=	tiltdetecthub/
endif
ifeq ($(CONFIG_CUSTOM_KERNEL_TILT_DETECTOR_SENSOR),"OTHER_VENDOR")
obj-y += other_vendor/
endif
