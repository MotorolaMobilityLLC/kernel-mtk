include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y += mtkfb_console.o        \
		disp_assert_layer.o    \
		mtkfb.o                \
		mtkfb_vsync.o          \
		debug.o                \
		primary_display.o      \
		disp_lcm.o             \
		disp_utils.o           \
		mtkfb_fence.o          \
		mtk_disp_mgr.o         \
		compat_mtk_disp_mgr.o \
		mtk_mira.o             \
		mtk_ovl.o              \
		disp_helper.o          \
		disp_dts_gpio.o          \
		fbconfig_kdebug_x.o

#obj-y += mtkfb_dummy.o

ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/misc/mediatek/ext_disp \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/$(MTK_PLATFORM)


ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/
endif
ifeq ($(CONFIG_MTK_INTERNAL_MHL_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mhltx/
endif
ifeq ($(CONFIG_MTK_DITHERING_SUPPORT), y)
ccflags-y += -DDITHERING_SUPPORT
endif

ifeq ($(CONFIG_MTK_GPU_SUPPORT), y)
ccflags-y += -DHWGPU_SUPPORT
endif

#ccflags-y += -DDEBUG
