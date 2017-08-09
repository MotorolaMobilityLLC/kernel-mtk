#
# Copyright (C) 2016 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.
#

obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_ipi_driver.o
obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_messenger_ipi.o
obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_ipi_client_phone_call.o audio_ipi_client_playback.o
obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_dma_buf_control.o

#include scp_ipi.h
ifeq ($(CONFIG_MTK_TINYSYS_SCP_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
endif

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_audio_ipi_driver.o += -DDEBUG
CFLAGS_audio_messenger_ipi.o += -DDEBUG
CFLAGS_audio_ipi_client_phone_call.o += -DDEBUG
CFLAGS_audio_ipi_client_playback.o += -DDEBUG
CFLAGS_audio_dma_buf_control.o += -DDEBUG
endif
