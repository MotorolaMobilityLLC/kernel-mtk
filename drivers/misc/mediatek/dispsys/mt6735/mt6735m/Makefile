# drivers/dispsys/mt6735m
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/ \
             -I$(srctree)/drivers/misc/mediatek/videox/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/videox/$(MTK_PLATFORM)/mt6735m/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735m/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/cmdq/ \
             -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM)/mt6735m/

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
