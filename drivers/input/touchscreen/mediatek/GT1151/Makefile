# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT1151/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT1151/include/$(CONFIG_GT1151_FIRMWARE)/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT1151/include/$(CONFIG_GT1151_CONFIG)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -DCONFIG_GTP_CUSTOM_CFG

ifeq ($(CONFIG_TOUCHSCREEN_MTK_USE_MODULE), y)
obj-m	+=  gt1151.o
gt1151-y := gt1x_extents.o gt1x_generic.o gt1x_tools.o gt1x_tpd.o gt1x_update.o
else
obj-y	+=  gt1x_extents.o
obj-y	+=  gt1x_generic.o
obj-y	+=  gt1x_tools.o
obj-y	+=  gt1x_tpd.o
obj-y	+=  gt1x_update.o
endif
