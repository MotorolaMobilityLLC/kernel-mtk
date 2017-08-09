#
# Makefile for misc devices that really don't fit anywhere else.
#

obj-$(CONFIG_MTK_LCM) += mt65xx_lcm_list.o

LCM_LISTS := $(subst ",,$(CONFIG_CUSTOM_KERNEL_LCM))
obj-$(CONFIG_MTK_LCM) += $(foreach LCM,$(LCM_LISTS),$(LCM)/)

subdir-ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc
subdir-ccflags-$(CONFIG_MTK_VIDEOX) += -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM) \
                                       -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM)/mt6735 \
                                       -I$(srctree)/drivers/misc/mediatek/videox/$(MTK_PLATFORM) \
                                       -I$(srctree)/drivers/misc/mediatek/video/

LCM_DEFINES := $(shell echo $(CONFIG_CUSTOM_KERNEL_LCM) | tr a-z A-Z)
DEFINES += $(foreach LCM,$(LCM_DEFINES),$(LCM))
DEFINES += MTK_LCM_PHYSICAL_ROTATION=\"$(MTK_LCM_PHYSICAL_ROTATION)\"
ccflags-$(CONFIG_MTK_LCM) += $(addprefix -D, $(DEFINES))
