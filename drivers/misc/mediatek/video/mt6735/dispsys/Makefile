# drivers/dispsys

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/ \
             -I$(srctree)/drivers/misc/mediatek/video/common/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/ \
             -I$(srctree)/drivers/misc/mediatek/base/power/mt6735/

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM) \
                    -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/

subdir-ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735/
obj-y += $(subst ",,mt6735)/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6735m/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735m/
obj-y += $(subst ",,mt6735m)/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6753/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6753/
obj-y += $(subst ",,mt6753)/
endif

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735/
obj-y += $(subst ",,mt6735)/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6735m/
obj-y += $(subst ",,mt6735m)/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6753/
obj-y += $(subst ",,mt6753)/
endif

ifeq ($(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT),y)
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
ccflags-y += \
	 -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/public \
	 -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/include \
	 -I$(srctree)/../vendor/trustonic/platform/$(MTK_PLATFORM)/trustlets/tplay/Tltplay/public

endif
endif

#driver
obj-y += ddp_color.o
obj-y += ddp_wdma.o
obj-y += ddp_aal.o
obj-y += ddp_pwm.o
obj-y += ddp_gamma.o
obj-y += ddp_dither.o
obj-y += ddp_od.o
obj-y += ddp_dsi.o
# obj-y += ddp_dpi.o

#common
obj-y += ddp_drv.o
obj-y += ddp_path.o
obj-y += ddp_debug.o
obj-y += ddp_dump.o

obj-y += ddp_manager.o
obj-y += ddp_irq.o
obj-y += ddp_info.o
obj-y += ddp_color_format.o

obj-y += ddp_mmp.o
obj-y += ddp_met.o

ifneq ($(CONFIG_ARCH_MT6753),y)
obj-y += display_recorder.o
endif

ifeq ($(CONFIG_MMPROFILE),y)
ccflags-y += -DDEFAULT_MMP_ENABLE
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -D_MTK_USER_
endif

ifeq ($(MTK_MIRAVISION_SUPPORT), yes)
    ccflags-y += -DCONFIG_FOR_SOURCE_PQ
endif

ccflags-y += -DMET_USER_EVENT_SUPPORT

ifeq ($(CONFIG_CUSTOM_KERNEL_HDMI), "MT8193")
ccflags-y +=	-DHDMI_MT8193_SUPPORT
endif

# EOF
