#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \

MTK_PLATFORM := $(strip $(MTK_PLATFORM))

# remove the _l318 postfix, get the pure project name
ARCH_MTK_PROJECT := $(subst _l318,,$(ARCH_MTK_PROJECT))
# strip
ARCH_MTK_PROJECT := $(strip $(ARCH_MTK_PROJECT))
$(warning "After remove postfix and strip, ARCH_MTK_PROJECT:$(ARCH_MTK_PROJECT)")

ifeq ($(wildcard $(MTK_CUSTOM_PATH)/imgsensor/src/$(MTK_PLATFORM)/camera_hw/camera_project/$(ARCH_MTK_PROJECT)),)  
$(warning "camera power on/off Use default kd_camera_hw, ARCH_MTK_PROJECT:$(ARCH_MTK_PROJECT)")                  
obj-y				+= kd_camera_hw.o
else
$(warning "camera power on/off Use project folder: $(ARCH_MTK_PROJECT)")
obj-y 				+= camera_project/$(ARCH_MTK_PROJECT)/
endif
