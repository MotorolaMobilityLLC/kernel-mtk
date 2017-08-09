#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ifneq ($(CONFIG_PINCTRL_MT6797),y)
# to use mtk legacy pinctrl
subdir-ccflags-y += -I$(DRVGEN_OUT)/inc
endif
MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \

obj-y				+= kd_camera_hw.o
