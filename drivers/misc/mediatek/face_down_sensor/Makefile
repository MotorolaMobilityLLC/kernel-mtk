# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

obj-y	+= face_down.o

ifeq ($(CONFIG_CUSTOM_KERNEL_FACE_DOWN_SENSOR),"OTHER_VENDOR")
obj-y += other_vendor/
endif
