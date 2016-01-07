# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y	+= heart_rate.o

ifeq ($(CONFIG_CUSTOM_KERNEL_HEART_RATE_SENSOR),"OTHER_VENDOR")
obj-y += other_vendor/
endif
