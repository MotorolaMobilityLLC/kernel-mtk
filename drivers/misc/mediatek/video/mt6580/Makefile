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

obj-$(CONFIG_MTK_FB) += mtkfb_console.o        \
			disp_assert_layer.o    \
			primary_display.o      \
			disp_lcm.o             \
			disp_utils.o           \
			disp_debug.o           \
			mtk_disp_mgr.o         \
			mtk_mira.o             \
			mtk_ovl.o              \
			disp_dts_gpio.o        \
			disp_helper.o          \
			ddp_ovl.o              \
			ddp_rdma_ex.o          \
			ddp_wdma_ex.o          \
			ddp_dsi.o              \
			ddp_drv.o              \
			ddp_path.o             \
			ddp_dump.o             \
			ddp_manager.o          \
			ddp_irq.o              \
			ddp_info.o             \
			ddp_pwm_mux.o          \
			ddp_color_format.o     \
			ddp_mmp.o              \
			ddp_met.o              \
			ddp_misc.o

ccflags-y +=	-I$(srctree)/drivers/misc/mediatek/video/include/                     \
		-I$(srctree)/drivers/misc/mediatek/video/common/                      \
		-I$(srctree)/drivers/misc/mediatek/video/common/rdma10/               \
		-I$(srctree)/drivers/misc/mediatek/video/common/wdma10/               \
		-I$(srctree)/drivers/misc/mediatek/cmdq/v2/                           \
		-I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/           \
		-I$(srctree)/drivers/misc/mediatek/externaldisplay/$(MTK_PLATFORM)    \
		-I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/mach/ \
		-I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)         \
		-I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v1/         \
		-I$(srctree)/drivers/misc/mediatek/base/power/include/                \
		-I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/               \
		-I$(srctree)/drivers/misc/mediatek/mmp/                               \
		-I$(srctree)/drivers/misc/mediatek/sync/

ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
ccflags-y += -I$(srctree)/drivers/staging/android/ion \
             -I$(srctree)/drivers/staging/android/ion/mtk
ccflags-y += -DMET_USER_EVENT_SUPPORT

ifeq ($(CONFIG_MMPROFILE),y)
ccflags-y += -DDEFAULT_MMP_ENABLE
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
