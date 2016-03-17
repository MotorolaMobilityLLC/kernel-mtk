obj-y += \
    vdec_drv_if.o \
    venc_drv_if.o

ccflags-y += \
    -I$(srctree)/include/ \
    -I$(srctree)/drivers/media/platform/mediatek \
    -I$(srctree)/drivers/media/platform/mediatek/include \
    -I$(srctree)/drivers/media/platform/mediatek/h264_dec/vpu \
    -I$(srctree)/drivers/media/platform/mediatek/vp8_dec/vpu \
    -I$(srctree)/drivers/media/platform/mediatek/mpeg4_dec/vpu

ifeq ($(CONFIG_MEDIATEK_VPU),y)
ccflags-y += \
    -I$(srctree)/drivers/media/platform/mediatek/vp9_dec/vpu \
    -I$(srctree)/drivers/media/platform/mediatek/vp8_enc/vpu \
    -I$(srctree)/drivers/media/platform/mediatek/h264_enc/vpu
endif
