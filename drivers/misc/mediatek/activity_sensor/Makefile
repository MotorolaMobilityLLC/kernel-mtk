include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y	+= activity.o

ifeq ($(CONFIG_CUSTOM_KERNEL_ACTIVITY_SENSOR),"OTHER_VENDOR")
obj-y += other_vendor/
endif
