ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
obj-$(CONFIG_CUSTOM_KERNEL_SENSORHUB)	+= sensorHub/
obj-$(CONFIG_MTK_HWMON)	    += hwmon/
obj-$(CONFIG_CUSTOM_KERNEL_ALSPS) += alsps/
ifeq ($(CONFIG_CUSTOM_KERNEL_ACCELGYRO),y)
obj-y += accelgyro/
else
obj-$(CONFIG_CUSTOM_KERNEL_ACCELEROMETER) += accelerometer/
obj-$(CONFIG_CUSTOM_KERNEL_GYROSCOPE) += gyroscope/
endif
obj-$(CONFIG_CUSTOM_KERNEL_MAGNETOMETER) += magnetometer/
obj-$(CONFIG_CUSTOM_KERNEL_BAROMETER) += barometer/
obj-$(CONFIG_CUSTOM_KERNEL_BIOMETRIC_SENSOR) += biometric/
else
obj-y += dummy.o
endif
