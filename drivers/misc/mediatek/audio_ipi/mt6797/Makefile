obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_messenger_ipi.o

#include scp_ipi.h
ifeq ($(CONFIG_MTK_TINYSYS_SCP_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(MTK_PLATFORM)
endif

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_audio_messenger_ipi.o += -DDEBUG
endif
