ifeq ($(CONFIG_MTK_LSM6DS3_K52),y)

ccflags-y += -I$(srctree)/drivers/misc/mediatek/accelerometer/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt6735/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/step_counter
obj-y	:=  lsm6ds3-int.o

endif
