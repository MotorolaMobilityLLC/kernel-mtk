obj-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += mtk_vcodec_dec_drv.o\
				       mtk_vcodec_dec.o\
				       mtk_vcodec_intr.o\
				       mtk_vcodec_dec_pm.o\
				       mtk_vcodec_util.o\
				       mtk_vcodec_enc_drv.o\
				       mtk_vcodec_enc.o\
				       mtk_vcodec_enc_pm.o\
				       mtkbuf-dma-cache-sg.o

ifeq ($(CONFIG_VIDEO_MEDIATEK_VCODEC),y)
    obj-$(CONFIG_MTK_IOMMU) += mtk_vcodec_iommu.o
endif

ccflags-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += -DDEBUG
ccflags-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += -Werror
#subdir-ccflags-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += -DY_C_SEPARATE
#subdir-ccflags-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += -DVDEC_UFO_SUPPORT

obj-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += common/ h264_dec/ h264_enc/ vp8_dec/ vp8_enc/ vp9_dec/ mpeg4_dec/
ccflags-y += -I$(srctree)/drivers/media/platform/mediatek/include

obj-$(CONFIG_MEDIATEK_VPU) += mtk_vpuc_core.o

