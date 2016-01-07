################################################################################
# COMMON PART
# 1. xxx.h/c: Source files that is common for all chips
# 2. IPvx/: Common part for a Chipset Groups
#
# When one new chip coming, you can select an existing common part, ex. IPv1 or
# you can create a new group, ex. IPv2
################################################################################



#####################################
# MT67xx chipset group
#obj-$(CONFIG_ARCH_MT6735) += IPv1/
#obj-$(CONFIG_ARCH_MT6580) += IPv1/
#obj-$(CONFIG_ARCH_MT6755) += IPv1/


#####################################
# Add new chipset groups here


#####################################
# Common Source files for all chips
obj-$(CONFIG_MTK_FB) += 			\
				data_rgb888_64x64.o		\
				data_rgb888_64x64_golden.o \
				mtkfb_fence.o \
				fbconfig_kdebug.o

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753) $(CONFIG_ARCH_MT6580)))
obj-$(CONFIG_MTK_FB) += mtkfb.o
endif

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6797)))
obj-y += color20/
endif

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6797) $(CONFIG_ARCH_MT6755) $(CONFIG_ARCH_MT6752) $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753) $(CONFIG_ARCH_MT6580)))
obj-y += aal20/
obj-y += corr10/
endif

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6797) $(CONFIG_ARCH_MT6753)))
obj-y += od10/
endif

# Todo: newer chip
ifeq (y, $(filter y, $(CONFIG_ARCH_MT6752) $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753) $(CONFIG_ARCH_MT6580) $(CONFIG_ARCH_MT6795) $(CONFIG_ARCH_MT6595)))
obj-y += rdma10/
obj-y += wdma10/
endif
# Todo: newer chip

ifeq (y, $(filter y, $(CONFIG_ARCH_MT6755) $(CONFIG_ARCH_MT6752) $(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753) $(CONFIG_ARCH_MT6580) $(CONFIG_ARCH_MT6795) $(CONFIG_ARCH_MT6595)))
obj-y += color20/
endif

ccflags-y += \
				-I$(srctree)/drivers/misc/mediatek/video/include     \
				-I$(srctree)/drivers/misc/mediatek/video/common/IPv1     \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox \
				-I$(srctree)/drivers/misc/mediatek/sync/      \
				-I$(srctree)/drivers/misc/mediatek/cmdq/v2/ \
				-I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/


ifneq ($(CONFIG_MTK_FB), y)
obj-y += mtkfb_dummy.o
endif

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
                    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735

ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735m/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735m/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6753/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6753/
endif

ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/
endif
ifeq ($(CONFIG_MTK_INTERNAL_MHL_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mhltx/
endif
ifeq ($(CONFIG_MTK_DITHERING_SUPPORT), y)
ccflags-y += -DDITHERING_SUPPORT
endif
