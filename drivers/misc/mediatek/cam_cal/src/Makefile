$(info cam_cal:makefile start)
include $(srctree)/drivers/misc/mediatek/Makefile.custom

define FILTER_PROJECT_DRV
$(info cam_cal:FILTER_PROJECT_DRV start)
$(info $(srctree)/drivers/misc/mediatek/cam_cal/src/$(subst ",,$(CONFIG_MTK_PLATFORM))/$(1))
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/cam_cal/src/$(subst ",,$(CONFIG_MTK_PLATFORM))/$(1)),)
platform_cam_cal_drv += $(subst ",,$(CONFIG_MTK_PLATFORM))/$(1)/
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/$(1)/
else ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/cam_cal/src/common/$(1)),)
common_cam_cal_drv += common/$(1)/
obj-y += common/$(1)/
else
without_cam_cal_drv += $(1)
endif
endef

cam_cal_drv = $(subst ",,$(CONFIG_CUSTOM_KERNEL_CAM_CAL_DRV))
$(info cam_cal_drv =$(cam_cal_drv))
$(foreach drv,$(cam_cal_drv),$(eval $(call FILTER_PROJECT_DRV,$(drv))))
$(info cam_cal: platform_cam_cal_drv = $(platform_cam_cal_drv))
$(info cam_cal: common_cam_cal_drv = $(common_cam_cal_drv))
$(info cam_cal: can't find cam_cal_drv = $(without_cam_cal_drv))