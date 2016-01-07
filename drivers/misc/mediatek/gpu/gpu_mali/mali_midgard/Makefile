MTK_GPU_VERSION = $(subst ",,$(CONFIG_MTK_GPU_VERSION))
$(info *MTK_GPU_VERSION 3 = $(word 3,$(MTK_GPU_VERSION)))

ifneq ($(word 3,$(MTK_GPU_VERSION)),)
    obj-y += mali-$(word 3,$(MTK_GPU_VERSION))/drivers/
else
    obj-y += mali-EAC/drivers/
endif