# UFSHCD makefile

ifeq ($(CONFIG_MTK_UFS_BOOTING),y)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/scsi/ufs/mediatek/$(MTK_PLATFORM)
endif

obj-$(CONFIG_SCSI_UFS_QCOM) += ufs-qcom.o
obj-$(CONFIG_SCSI_UFSHCD) += ufshcd.o
obj-$(CONFIG_SCSI_UFSHCD_PCI) += ufshcd-pci.o
obj-$(CONFIG_SCSI_UFSHCD_PLATFORM) += ufshcd-pltfrm.o
obj-$(CONFIG_MTK_UFS_BOOTING) += ufs-mtk.o
obj-$(CONFIG_MTK_UFS_BOOTING) += mediatek/$(MTK_PLATFORM)/ufs-mtk-platform.o
obj-$(CONFIG_MTK_UFS_BLOCK_IO_LOG) += ufs-mtk-block.o
obj-$(CONFIG_MTK_UFS_BOOTING) += ufs-dbg.o
