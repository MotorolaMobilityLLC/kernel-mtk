# drivers/image/jpeg/Makefile

#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y = -DSDEBUG_T

ifneq ($(CONFIG_ARCH_MT6735M),y)
  ccflags-y += -DJPEG_DEC_DRIVER
endif

#ccflags-y += -I$(MTK_PATH_SOURCE)/drivers/jpeg/
ifeq ($(CONFIG_ARCH_MT8173),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/hv1/mt8173/
endif

ifneq ($(CONFIG_ARCH_MT8173),y)
ccflags-y += \
  -I$(srctree)/drivers/misc/mediatek/cmdq/v2/ \
  -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
endif

ifeq ($(CONFIG_ARCH_MT8173),y)
#ccflags-y += -DJPEG_DEV
ccflags-y += -DJPEG_PM_DOMAIN_ENABLE
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
  ccflags-y += -DJPEG_DEC_IRQ_ENHANCE
endif

#obj-$(CONFIG_MTK_JPEG) += jpeg_driver.o
obj-y += jpeg_driver.o

jpeg_driver-objs := jpeg_cmdq.o jpeg_drv_enc.o jpeg_drv_dec.o jpeg_drv.o

# EOF
