#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

obj-y +=	ddp_ovl.o              \
		ddp_rdma.o             \
		ddp_wdma.o             \
		ddp_dsi.o              \
		ddp_dpi.o              \
		ddp_clkmgr.o           \
		ddp_drv.o              \
		ddp_path.o             \
		ddp_dump.o             \
		ddp_manager.o          \
		ddp_irq.o              \
		ddp_info.o             \
		ddp_color_format.o     \
		ddp_pwm_mux.o          \
		ddp_mmp.o              \
		ddp_met.o              \
		mtkfb_console.o        \
		disp_assert_layer.o    \
		mtkfb.o                \
		disp_debug.o           \
		primary_display.o      \
		disp_lowpower.o        \
		disp_recovery.o        \
		disp_lcm.o             \
		disp_utils.o           \
		mtk_disp_mgr.o         \
		mtk_ovl.o              \
		compat_mtk_disp_mgr.o  \
		mtk_mira.o             \
		disp_helper.o          \
		disp_dts_gpio.o        \
		layering_rule.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/                     \
             -I$(srctree)/drivers/misc/mediatek/video/common/                      \
             -I$(srctree)/drivers/misc/mediatek/video/common/rdma10/               \
             -I$(srctree)/drivers/misc/mediatek/video/common/wdma10/               \
             -I$(srctree)/drivers/misc/mediatek/video/common/color20/              \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/             \
             -I$(srctree)/drivers/misc/mediatek/sync/                              \
             -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)                \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v2/                           \
             -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/           \
             -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/mach/ \
             -I$(srctree)/drivers/staging/android/ion/                             \
             -I$(srctree)/drivers/staging/android/ion/mtk/                         \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/                          \
	     -I$(srctree)/drivers/misc/mediatek/ext_disp/v1/                       \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/$(MTK_PLATFORM)/          \
             -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/        \
             -I$(srctree)/drivers/misc/mediatek/base/power/include/                \
             -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/               \
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat                    \
             -I$(srctree)/drivers/misc/mediatek/mmp/                               \
             -I$(srctree)/drivers/misc/mediatek/video/common/layering_rule_base/v1/

ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc/

ifeq ($(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT),y)
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
ccflags-y += \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/trustzone/tplay/Tltplay/public/

endif
endif

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

ifeq ($(CONFIG_MMPROFILE),y)
ccflags-y += -DDEFAULT_MMP_ENABLE
endif

ccflags-y += -DMET_USER_EVENT_SUPPORT

#ccflags-y += -DDEBUG
