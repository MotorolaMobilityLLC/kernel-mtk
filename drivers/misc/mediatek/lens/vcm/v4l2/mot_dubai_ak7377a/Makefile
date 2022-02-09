#
# Makefile for misc devices that really don't fit anywhere else.
#


MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek


subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/v4l2/mot_dubai_ak7377a/inc
subdir-ccflags-y += -I$(MTK_CUSTOM_PATH)/lens/vcm/v4l2/mot_dubai_ak7377a/




#obj-$(CONFIG_MTK_V4L2_LENS) += mot_dubai_ak7377a.o

obj-$(CONFIG_MTK_V4L2_LENS) += mot_dubai_ak7377a.o
mot_dubai_ak7377a-objs += mot_dubai_ak7377.o
mot_dubai_ak7377a-objs += dw9781_circle_test.o
mot_dubai_ak7377a-objs += dw9781_i2c.o
mot_dubai_ak7377a-objs += dw9781.o

$(info mot_dubai_ak7377a-objs : $(mot_dubai_ak7377a-objs))
