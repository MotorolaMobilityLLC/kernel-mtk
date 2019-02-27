# SPDX-License-Identifier: GPL-2.0

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

# include path
ccflags-y += -Werror -I$(srctree)/sound/soc/mediatek/$(MTK_PLATFORM)/

# MENU Governor Predict(low power)
ifeq ($(CONFIG_MTK_ACAO_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include/
endif

# spm
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include

# platform code
obj-$(CONFIG_SND_SOC_MTK_AUDIO_DSP) += audio_mem_control.o
