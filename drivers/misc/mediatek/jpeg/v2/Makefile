# drivers/image/jpeg/Makefile

include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y = -DSDEBUG_T

ifneq ($(CONFIG_ARCH_MT6735M),y)
  ccflags-y += -DJPEG_DEC_DRIVER
endif

#ccflags-y += -I$(MTK_PATH_SOURCE)/drivers/jpeg/
ccflags-y += \
  -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)

ifeq ($(CONFIG_ARCH_MT6735),y)
  ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735/
endif

ifeq ($(CONFIG_ARCH_MT6735M),y)
  ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735m/
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
  ccflags-y += -DJPEG_DEC_IRQ_ENHANCE
  ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6753/
endif

#obj-$(CONFIG_MTK_JPEG) += jpeg_driver.o
obj-y += jpeg_driver.o

jpeg_driver-objs := jpeg_cmdq.o jpeg_drv_enc.o jpeg_drv_dec.o jpeg_drv.o

# EOF
