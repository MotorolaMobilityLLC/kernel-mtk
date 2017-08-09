obj-y += ged/
obj-y += hal/

MTK_GPU_VERSION = $(subst ",,$(CONFIG_MTK_GPU_VERSION))
$(info *MTK_GPU_VERSION 1 = $(word 1,$(MTK_GPU_VERSION)))

# GPU R5P0
ifeq ($(CONFIG_MTK_GPU_R5P0),y)
    obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
else ifneq ($(word 1,$(MTK_GPU_VERSION)),)
    obj-y += gpu_$(word 1,$(MTK_GPU_VERSION))/
else
    obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
