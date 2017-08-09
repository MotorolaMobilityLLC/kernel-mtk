obj-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += vdec_h264_if.o vdec_h264_vpu.o vdec_h264_debug.o

#ccflags-y += -DDBG_VDEC_H264
#ccflags-y += -DDBG_VPU_MSG_CHECKSUM
#ccflags-y += -DH264_DECODE_BENCHMARK
#ccflags-y += -DDBG_VDEC_H264_DUMP_DISPLAY_FRAME
#ccflags-y += -DDBG_VDEC_H264_DUMP_INPUT_FILE

ccflags-y += \
    -I$(srctree)/include \
    -I$(srctree)/drivers/media/platform/mediatek \
    -I$(srctree)/drivers/media/platform/mediatek/include \
    -I$(srctree)/drivers/media/platform/mediatek/h264_dec \
    -I$(srctree)/drivers/media/platform/mediatek/h264_dec/vpu
