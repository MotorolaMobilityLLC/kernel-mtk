################################################################################
# MT67xx Chip Set Group
#
#
################################################################################

include $(srctree)/drivers/misc/mediatek/Makefile.custom


obj-$(CONFIG_MTK_FB) +=


ccflags-y += \
				-I$(srctree)/drivers/misc/mediatek/video/include     \
				-I$(srctree)/drivers/misc/mediatek/video/common     \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM) \
				-I$(srctree)/drivers/misc/mediatek/sync/      \
				-I$(srctree)/drivers/misc/mediatek/m4u/       \
				-I$(srctree)/drivers/misc/mediatek/cmdq/v2/ \
				-I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/ \
				-I$(srctree)/drivers/misc/mediatek/ext_disp/$(MTK_PLATFORM) \
				-I$(srctree)/drivers/staging/android/ion

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/
endif
ifeq ($(CONFIG_MTK_INTERNAL_MHL_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mhltx/
endif
ifeq ($(CONFIG_MTK_DITHERING_SUPPORT), y)
ccflags-y += -DDITHERING_SUPPORT
endif

ifeq ($(CONFIG_MTK_GPU_SUPPORT), y)
ccflags-y += -DHWGPU_SUPPORT
endif

