# SPDX-License-Identifier: GPL-2.0
# Mediatek UFS extended features makefile
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/scsi/ufs/
ifneq ($(filter y m, $(CONFIG_TRUSTONIC_TEE_SUPPORT)),)
include $(srctree)/drivers/tee/gud/Makefile.include
endif

obj-$(CONFIG_SCSI_UFS_MEDIATEK) += ufs-mediatek-mod.o
ufs-mediatek-mod-y 			+= ../ufs-mediatek.o ../ufs-mediatek-mcq.o
ifeq ($(CONFIG_UFSHID_V3),y)
ufs-mediatek-mod-$(CONFIG_UFSFEATURE) 	+= ../V3/ufsfeature.o ../V3/ufsfeature-mimic.o
ufs-mediatek-mod-$(CONFIG_UFSHID) 	+= ../V3/ufshid.o
else
ufs-mediatek-mod-$(CONFIG_UFSFEATURE) 	+= ufsfeature.o ufsfeature-mimic.o
ufs-mediatek-mod-$(CONFIG_UFSHID) 	+= ufshid.o
endif
ufs-mediatek-mod-$(CONFIG_SCSI_SKHID) += ufs-manual-gc.o
ufs-mediatek-mod-$(CONFIG_UFSSHPB) 	+= ufsshpb.o
ufs-mediatek-mod-$(CONFIG_UFSTW) 	+= ufstw.o
ufs-mediatek-mod-$(CONFIG_UFSRINGBUF)	+= ufsringbuf.o
ufs-mediatek-mod-$(CONFIG_FSCRYPT_WRAPED_KEY_MODE_SUPPORT) += ../ufshcd-moto-crypto.o
