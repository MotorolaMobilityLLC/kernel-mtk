obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_ipi_driver.o
obj-$(CONFIG_MTK_TINYSYS_SCP_SUPPORT) += audio_messenger_ipi.o

#include scp_ipi.h
ifeq ($(CONFIG_MTK_TINYSYS_SCP_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
endif

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_audio_ipi_driver.o += -DDEBUG
CFLAGS_audio_messenger_ipi.o += -DDEBUG
endif
