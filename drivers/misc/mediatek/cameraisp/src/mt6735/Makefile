#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/

ifneq ($(CONFIG_ARM64), y)
# For arm32
  ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
    obj-y += cameraisp_dummy2.o
    obj-y += camera_isp_D2.o
    obj-y += camera_sysram_D2.o
    obj-y += camera_pipe_mgr_D2.o
  else
    obj-y += camera_isp_D1.o
    obj-y += camera_sysram_D1.o
    obj-y += camera_fdvt_D1.o
  endif
else
# For arm64
  ifeq ($(strip $(CONFIG_ARCH_MT6735M)),y)
    obj-y += cameraisp_dummy2.o
    obj-y += camera_isp_D2.o
    obj-y += camera_sysram_D2.o
    obj-y += camera_pipe_mgr_D2.o
  else
    obj-y += camera_isp_D1.o
    obj-y += camera_sysram_D1.o
    obj-y += camera_fdvt_D1.o
  endif
endif

$(info cameraisp drv by platform $(platform_drv))
