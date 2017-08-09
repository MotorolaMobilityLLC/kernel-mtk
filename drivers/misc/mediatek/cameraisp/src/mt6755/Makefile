#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/

ifneq ($(CONFIG_ARM64), y)
# For arm32
  obj-y += camera_isp.o
  #obj-y += camera_sysram.o
  obj-y += camera_fdvt.o
  #obj-y += camera_eis.o
else
# For arm64
  obj-y += camera_isp.o
  #obj-y += camera_sysram.o
  obj-y += camera_fdvt.o
  #obj-y += camera_eis.o
endif
$(info cameraisp drv by platform $(platform_drv))
