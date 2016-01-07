#
# Makefile for external display driver.
#
# include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/ext_disp/  \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/common/test/     \
             -I$(srctree)/drivers/misc/mediatek/video/     \
             -I$(srctree)/drivers/misc/mediatek/video/include/ \
             -I$(srctree)/drivers/misc/mediatek/video/common/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
             -I$(srctree)/drivers/misc/mediatek/sync/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/$(CONFIG_CUSTOM_KERNEL_HDMI)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v2/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/hdmi/inc/ \
             -I$(srctree)/drivers/misc/mediatek/multibridge/mt6735/mt8193/inc/ \
             -I$(srctree)/drivers/misc/mediatek/leds/ \
             -I$(srctree)/drivers/misc/mediatek/leds/mt6735 \
             -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/misc/mediatek/lcm/inc

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
                    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk

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

ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI), "MT8193")
ccflags-y +=	-DHDMI_MT8193_SUPPORT
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/mt8193/
endif

obj-y :=  \
    external_display.o \
    extd_multi_control.o \
    extd_hdmi.o \
    mtk_extd_mgr.o   \
    extd_debug.o \
    extd_utils.o \
    extd_epd.o \
    extd_factory.o
