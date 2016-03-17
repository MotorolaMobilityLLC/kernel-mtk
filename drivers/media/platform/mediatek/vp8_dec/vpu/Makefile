obj-y +=  vdec_vp8_if.o vdec_vp8_vpu.o vdec_vp8_debug.o

ccflags-y += -DDISPLAY_ALL_FRAME
#ccflags-y += -DDBG_VDEC_VP8
#ccflags-y += -DDBG_VDEC_VP8_DUMP_DISPLAY_FRAME

ccflags-y += \
    -I$(srctree)/include/ \
    -I$(srctree)/drivers/media/platform/mediatek \
    -I$(srctree)/drivers/media/platform/mediatek/include \
    -I$(srctree)/drivers/media/platform/mediatek/vp8_dec \
    -I$(srctree)/drivers/media/platform/mediatek/vp8_dec/vpu
