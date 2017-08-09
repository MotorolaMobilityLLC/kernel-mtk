# drivers/dispsys/mt6797m

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/mt6797m/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6797m/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/mt6797m/

ifeq ($(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT),y)
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
ccflags-y += \
	 -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/public \
	 -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/include \
	 -I$(srctree)/../vendor/trustonic/platform/$(MTK_PLATFORM)/trustlets/tplay/Tltplay/public

endif
endif

#driver
obj-y += ddp_ovl.o
obj-y += ddp_rdma.o

# EOF
