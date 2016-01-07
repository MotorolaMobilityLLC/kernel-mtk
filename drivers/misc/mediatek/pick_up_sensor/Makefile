# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y	+= pick_up.o

ifeq ($(CONFIG_CUSTOM_KERNEL_PICK_UP_SENSOR),"OTHER_VENDOR")
obj-y += other_vendor/
endif
